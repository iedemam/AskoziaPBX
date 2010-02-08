/* $Id: zap_xhfc_su.h,v 1.6 2007/08/29 08:04:24 martinb1 Exp $
 *
 * Zaptel driver for Colognechip xHFC chip
 *
 * Authors : Martin Bachem, Joerg Ciesielski
 * Contact : info@colognechip.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef _XHFC_SU_H_
#define _XHFC_SU_H_

#include <linux/timer.h>
#include <dahdi/kernel.h>
#include "xhfc24succ.h"
#include "auerask_cp3k_mb.h"	

#define DRIVER_NAME "AUERASK_CP3K_BRI"

#ifndef CHIP_ID_2S4U
#define CHIP_ID_2S4U	0x62
#endif
#ifndef CHIP_ID_4SU
#define CHIP_ID_4SU	0x63
#endif
#ifndef CHIP_ID_1SU
#define CHIP_ID_1SU	0x60
#endif
#ifndef CHIP_ID_2SU
#define CHIP_ID_2SU	0x61
#endif
	
#define LOOP_TIMEOUT 200

/* define bridge for chip register access */	
#define BRIDGE_UNKNOWN	0
#define BRIDGE_PCI2PI	1 /* used at Cologne Chip AG's Evaluation Card */
#define BRIDGE		BRIDGE_CP3K_SPI

/* max. number of S/U ports per XHFC controller */
#define MAX_PORT	4

/* flags in _u16  port mode */
#define PORT_UNUSED		0x0000
#define PORT_MODE_NT		0x0001
#define PORT_MODE_TE		0x0002
#define PORT_MODE_S0		0x0004
#define PORT_MODE_UP		0x0008
#define PORT_MODE_EXCH_POL	0x0010
#define PORT_MODE_LOOP_B1       0x0020
#define PORT_MODE_LOOP_B2       0x0040
#define PORT_MODE_LOOP_D        0x0080
#define NT_TIMER		0x8000

/* NT / TE defines */
#define NT_T1_COUNT	25	/* number of 4ms interrupts for G2 timeout */
#define CLK_DLY_TE	0x0e	/* CLKDEL in TE mode */
#define CLK_DLY_NT	0x6c	/* CLKDEL in NT mode */
#define STA_ACTIVATE	0x60	/* start activation   in A_SU_WR_STA */
#define STA_DEACTIVATE	0x40	/* start deactivation in A_SU_WR_STA */
#define LIF_MODE_NT	0x04	/* Line Interface NT mode */
#define HFC_TIMER_T3	8000	/* 8s activation timer T3 */
#define HFC_TIMER_T4	500	/* 500ms deactivation timer T4 */
#define HFC_TIMER_OFF	-1	/* timer disabled */

/* xhfc Layer1 physical commands */
#define HFC_L1_ACTIVATE_TE		0x01
#define HFC_L1_FORCE_DEACTIVATE_TE	0x02
#define HFC_L1_ACTIVATE_NT		0x03
#define HFC_L1_DEACTIVATE_NT		0x04
#define HFC_L1_TESTLOOP_B1		0x05
#define HFC_L1_TESTLOOP_B2		0x06
#define HFC_L1_TESTLOOP_D               0x07

/* xhfc Layer1 Flags (stored in xhfc_port_t->l1_flags) */
#define HFC_L1_ACTIVATING	1
#define HFC_L1_ACTIVATED	2

#define FIFO_MASK_TX	0x55555555
#define FIFO_MASK_RX	0xAAAAAAAA


#ifndef MAX_DFRAME_LEN_L1
#define MAX_DFRAME_LEN_L1 300
#endif


/* DEBUG flags, use combined value for module parameter debug=x */
#define DEBUG_HFC_INIT		0x0001
#define DEBUG_HFC_MODE		0x0002
#define DEBUG_HFC_S0_STATES	0x0004
#define DEBUG_HFC_IRQ		0x0008
#define DEBUG_HFC_FIFO_ERR	0x0010
#define DEBUG_HFC_DTRACE	0x2000
#define DEBUG_HFC_BTRACE	0x4000	/* very(!) heavy messageslog load */
#define DEBUG_HFC_FIFO		0x8000	/* very(!) heavy messageslog load */

//#define USE_F0_COUNTER	1	/* akkumulate F0 counter diff every irq */
#define TRANSP_PACKET_SIZE 0	/* minium tranparent packet size for transmittion to upper layer */


struct _xhfc_hw;


/* port struct for each S/U port */
typedef struct {
	__u8 idx;		/* port idx in hw->port[idx] */
	struct _xhfc_hw *hw;	/* back pointer to xhfc_hw */
	__u16 mode;		/* NT/TE / ST/U */
	u_long	l1_flags;
	
	/* Zaptel span/chan related buffers */
	struct dahdi_span zspan;
	struct dahdi_chan *zchans[3];
	struct dahdi_chan _chans[3];

	int drx_indx;
	int dtx_indx;
	__u8 brxbuf[2][DAHDI_CHUNKSIZE];
	__u8 btxbuf[2][DAHDI_CHUNKSIZE];
	__u8 drxbuf[MAX_DFRAME_LEN_L1];
	__u8 dtxbuf[MAX_DFRAME_LEN_L1];	

	/* layer1 ISDN timer */
	int nt_timer;
	int t3; /* timer 3 for activation */
	int t4; /* timer 4 for deactivation */

	/* chip registers */
	reg_a_su_rd_sta	su_state;
	reg_a_su_ctrl0 su_ctrl0;
	reg_a_su_ctrl1 su_ctrl1;
	reg_a_su_ctrl2 su_ctrl2;
	reg_a_st_ctrl3 st_ctrl3;
} xhfc_port_t;



/**********************/
/* hardware structure */
/**********************/
typedef struct _xhfc_hw {
	
	struct list_head list;
	spinlock_t lock;

        __u8 chip_id;
        __u8 chip_rev;

	int cardnum;
        __u8 slot;              /* our slot the device is plugged in (or on-board) */
	__u8 param_idx;		/* used to access module param arrays */
	int ifnum;
	int irq;
	int iobase;
	int nt_mode;
        void __iomem * base_address; // SPI base address

	char card_name[60];

	int num_ports;		/* number of S and U interfaces */
	int max_fifo;		/* always 4 fifos per port */
	__u8 max_z;		/* fifo depth -1 */

	xhfc_port_t port[MAX_PORT]; /* one for each Line intercace */

	/* chip registers */
  	reg_r_irq_ctrl 		irq_ctrl;
  	reg_r_su_irq		su_irq;		/* collect interrupt status bits */

	__u32 fifo_irq;		/* fifo bl irq */
	__u32 fifo_irqmsk;	/* fifo bl irq */
} xhfc_hw;


#endif				/* _XHFC_SU_H_ */
