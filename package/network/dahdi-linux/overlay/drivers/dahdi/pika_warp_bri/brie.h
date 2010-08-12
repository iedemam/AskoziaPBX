#include <linux/module.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

#include <dahdi/kernel.h>

#include "pikadma.h"
#include "warped24succ.h"

/* FPGA defines */
#define FPGA_CONFIG		0x0000 /* Configuration */
#define FPGA_PFT		0x000C /* Termination on BRI */
#define FPGA_RESET		0x0014 /* Reset control */
#define FPGA_DIAG		0x0018 /* Board Diagnostics */
#define FPGA_REV		0x001C /* FPGA load type and revision */
#define FPGA_CIAR		0x0024 /* Codec Indirect Register */
#define FPGA_ICCR		0x002c /* Interrupt Cause/Clear */
#define FPGA_IMR		0x0030 /* Interrupt Mask */
#define FPGA_STAT_LINES		0x0034 /* Status Lines */
#define FPGA_DMA_SG_IDX		0x0E10 /* DMA SG List Index */

/* xhfc board defines */
#define MAX_XHFC_SPANS	4	/* This is per chip */
#define CHAN_PER_SPAN	4	/* D, B1, B2, PCM */
#define MAX_CHAN	(MAX_XHFC_SPANS * CHAN_PER_SPAN)

#define MAX_SPANS		8

/* flags in _u16  span mode */
#define SPAN_UNUSED		0x0000
#define SPAN_MODE_NT		0x0001
#define SPAN_MODE_TE		0x0002
#define SPAN_MODE_S0		0x0004
#define SPAN_MODE_UP		0x0008
#define SPAN_MODE_EXCH_POL	0x0010
#define SPAN_MODE_LOOP_B1	0x0020
#define SPAN_MODE_LOOP_B2	0x0040
#define SPAN_MODE_LOOP_D	0x0080
#define SPAN_MODE_ENDPOINT	0x0100
#define SPAN_MODE_STARTED       0x1000

#define SPAN_MODE_LOOPS		0xE0	/* mask span mode Loop B1/B2/D */


/* NT / TE defines */
#define CLK_DLY_TE	0x0e	/* CLKDEL in TE mode */
#define CLK_DLY_NT	0x6c	/* CLKDEL in NT mode */
#define STA_ACTIVATE	0x60	/* start activation   in A_SU_WR_STA */
#define STA_DEACTIVATE	0x40	/* start deactivation in A_SU_WR_STA */
#define XHFC_TIMER_T3	2000	/* 2s activation timer T3 */
#define XHFC_TIMER_T4	500	/* 500ms deactivation timer T4 */

/* xhfc Layer1 Flags (stored in xhfc_port_t->l1_flags) */
#define HFC_L1_ACTIVATING	1
#define HFC_L1_ACTIVATED	2

#define BRIE_CHANS_PER_SPAN 3

/* DMA - this must match wa_dma.h but we cannot include wa_dma.h */
#define FRAMES_PER_BUFFER	320 /* 40ms / (125us per frame) */
#define FRAMES_PER_TRANSFER	8
#define FRAMESIZE		64
#define DMA_CARD_ID		7

/* span struct for each S/U span */
struct xhfc_span {
	int span_id; /* Physical span id */
	int id; /* 0 based, no gaps */
	struct xhfc *xhfc;
	struct xhfc_chan *d_chan;
	struct xhfc_chan *b1_chan;
	struct xhfc_chan *b2_chan;

	int timeslot;

	atomic_t open_count;

	/* hdlc transmit data */
	int tx_idx;
	int tx_size;
	int tx_frame;
	u8  tx_buf[128];

	/* hdlc receive data */
	int rx_idx;
	u8  rx_buf[128];

	u16 mode;		/* NT/TE + ST/U */
	u8 state;
	u_long	l1_flags;
	struct timer_list t3_timer;	/* for activation/deactivation */
	struct timer_list t4_timer;	/* for activation/deactivation */
	struct timer_list nt_timer;

	/* Alarm state for dahdi */
	int newalarm;
	struct timer_list alarm_timer;

	/* chip registers */
	reg_a_su_ctrl0 su_ctrl0;
	reg_a_su_ctrl1 su_ctrl1;
	reg_a_su_ctrl2 su_ctrl2;

	/* dahdi */
	struct dahdi_span span;
	struct dahdi_chan *chans[BRIE_CHANS_PER_SPAN]; /* Individual channels */
	struct dahdi_chan *sigchan; /* the signaling channel for this span */
};


/* channel struct for each fifo */
struct xhfc_chan {
	int id;
	struct xhfc_span *span;

	unsigned char writechunk[DAHDI_CHUNKSIZE];
	unsigned char readchunk[DAHDI_CHUNKSIZE];

	struct dahdi_chan chan;
};

struct xhfc {
	__u8		chipnum;	/* global chip number */
	__u8		modidx;	/* module index 0 = mod a, 1 = mod b */
	struct brie *dev; /* back pointer to g_brie */

	int num_spans;		/* number of S and U interfaces */
	int max_fifo;		/* always 4 fifos per span */
	__u8 max_z;		/* fifo depth -1 */

	struct xhfc_chan *chan;	/* one each D/B/PCM channel */

	/* chip registers */
	reg_r_irq_ctrl		irq_ctrl;
	reg_r_misc_irqmsk	misc_irqmask;	/* mask of enabled interrupt
						   sources */
	reg_r_misc_irq		misc_irq;	/* collect interrupt status
						   bits */

	reg_r_su_irqmsk		su_irqmask;	/* mask of line interface
						   state change interrupts */
	reg_r_su_irq		su_irq;		/* collect interrupt status
						   bits */
	__u32 fifo_irq;		/* fifo bl irq */
	__u32 fifo_irqmask;	/* fifo bl irq */

	/* Debugging */
	u8 r_af0_oview;
	u8 r_bert_sta;
	u32 rx_fifo_errs;
	u32 tx_fifo_errs;
};

struct brie {
	void __iomem *fpga;
	int irq;

	int loopback;

	int pcm_master;

	struct tasklet_struct brie_bh;

	u8 moda;
	u8 modb;
	u8  num_xhfcs;
	struct xhfc *xhfc;

	struct xhfc_span *spans[MAX_SPANS];

	/* DMA */
	void *tx_buf;
	void *rx_buf;
};


static inline __u8 read_xhfc(struct xhfc *xhfc, __u8 reg)
{
	__u8 data = 0;

	fpga_read_indirect(xhfc->dev->fpga, xhfc->chipnum, reg, &data);
	return data;
}

static inline void write_xhfc(struct xhfc *xhfc, __u8 reg, __u8 value)
{
	fpga_write_indirect(xhfc->dev->fpga, xhfc->chipnum, reg, value);
}

static inline void xhfc_waitbusy(struct xhfc *xhfc)
{
	while (read_xhfc(xhfc, R_STATUS) & M_BUSY)
		cpu_relax();
}

static inline __init void xhfc_selfifo(struct xhfc *xhfc, __u8 fifo)
{
	write_xhfc(xhfc, R_FIFO, fifo);
	xhfc_waitbusy(xhfc);
}

static inline void xhfc_inc_f(struct xhfc *xhfc)
{
	write_xhfc(xhfc, A_INC_RES_FIFO, M_INC_F);
	xhfc_waitbusy(xhfc);
}

static inline __init void xhfc_resetfifo(struct xhfc *xhfc)
{
	write_xhfc(xhfc, A_INC_RES_FIFO, M_RES_FIFO | M_RES_FIFO_ERR);
	xhfc_waitbusy(xhfc);
}
