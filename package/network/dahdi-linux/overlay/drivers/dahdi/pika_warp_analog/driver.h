#ifndef DRIVER_H
#define DRIVER_H

#include "pkh_phone.h"
#include "hmpiointerface.h"
#include "hmpioconvert.h"
#include "si3215_registers.h"
#include "pk_debug.h"
#include "pikataco.h"


/* BAR0 definitions - all registers 32 bit unless specified. */
/* DEC = Digital Expansion Connector */
#define BAR0_CFG_PRMCSR        0x0000 /* PCIe Config Primary */
#define BAR0_CONFIG            0x0000 /* FPGA Config - Warp only */
#define BAR0_CFG_DEVCSR        0x0004 /* PCIe Config Device */
#define BAR0_CFG_LINKCSR       0x0008 /* PCIe Config Link */
#define BAR0_TACO_LEDS         0x0008 /* LEDs on Taco */
#define BAR0_CFG_MSICSR        0x000C /* Power Management Capabilities (BYTE) */
#define BAR0_TACO_PFT          0x000C /* PFT control/supervision */
#define BAR0_CFG_LTSSM         0x0010 /* PCIe Link Training Status (BYTE) */
#define BAR0_RESET             0x0014 /* Reset control */
#define BAR0_DIAG              0x0018 /* Board Diagnostics */
#define BAR0_FPGA_REV          0x001C /* FPGA load type and revision */
#define BAR0_FIAR              0x0020 /* Flash Indirect Register */
#define BAR0_CIAR              0x0024 /* Codec Indirect Register */
#define BAR0_PLL_CONTROL       0x0028 /* PLL Control */
#define BAR0_ICCR              0x002c /* Interrupt Cause/Clear */
#define BAR0_IMR               0x0030 /* Interrupt Mask */
#define BAR0_STAT_LINES        0x0034 /* Status Lines */
#define BAR0_CLK_DIAG          0x0038 /* Clock Diagnostics */
#define BAR0_SYS_TICK          0x003C /* System Tick Register */
#define BAR0_ABV_THRSH_STAT    0x0040 /* Above Threshold Status */
#define BAR0_BEL_THRSH_STAT    0x0044 /* Below Threshold Status */
#define BAR0_RING_ON_STAT      0x0048 /* Ring On Detection Status */
#define BAR0_RING_OFF_STAT     0x004C /* Ring Off Detection Status */
#define BAR0_RX_OVERLOAD_STAT  0x0050 /* Receive Level Overload Status */
#define BAR0_LOSS_FRM_STAT     0x0054 /* SiLabs Loss of Frame Status */
#define BAR0_BILL_TONE_STAT    0x0058 /* Billing Tone Detection Status */
#define BAR0_DROP_OUT_DET_STAT 0x005C /* Dropout Detect Status */
#define BAR0_L_C_OVLD_STAT     0x0060 /* Loop Current Sense Overload Status */
#define BAR0_REV_STAT          0x0064 /* Receive Level Overload Status */
#define BAR0_PULSE_STAT        0x0068 /* Pulse Dialing Status */
#define BAR0_HKFLSH_STAT       0x006C /* Hook Flash Status */
#define BAR0_HKFLSH_START      0x0080 /* Hook-flash Start */
#define BAR0_HKFLSH_STOP       0x0084 /* Hook-flash Stop */
#define BAR0_HKFLSH_TIME       0x0088 /* Hook Flash Time (ms) */
#define BAR0_THRSH_DET_DBNC_TIME 0x0090 /* Threshold Detect Debounce Time */
#define BAR0_THRSH_INT_MASK    0x0094 /* Threshold Interrupt Mask */

#define BAR0_RNG_INACT_STAT    0x0100
#define BAR0_RNG_ACT_STAT      0x0104
#define BAR0_Q6_PWR_ALRM_STAT  0x0108
#define BAR0_Q5_PWR_ALRM_STAT  0x010C
#define BAR0_Q4_PWR_ALRM_STAT  0x0110
#define BAR0_Q3_PWR_ALRM_STAT  0x0114
#define BAR0_Q2_PWR_ALRM_STAT  0x0118
#define BAR0_Q1_PWR_ALRM_STAT  0x011C
#define BAR0_LC_TRANS_STAT     0x0120
#define BAR0_RNG_TRP_STAT      0x0124
#define BAR0_SILABS_IND_STAT   0x0128

#define BAR0_TDM_DMA_CTRL      0x0E00 /* TDM DMA Control/Status */
#define BAR0_TDM_DMA_HOST_PTR  0x0E04 /* TDM DMA Current Host Buffer */
#define BAR0_TDM_DMA_SZ        0x0E08 /* TDM DMA Transfer Size (frames) */
#define BAR0_TDM_DMA_JIT       0x0E0C /* TDM DMA Jitter Buffer (frames) */
#define BAR0_DMA_SG_IDX        0x0E10 /* DMA SG List Index Register */
#define BAR0_DEC_DMA_CTRL      0x0F00 /* DEC DMA Control/Status */
#define BAR0_DEC_DMA_HOST_PTR  0x0F04 /* DEC DMA Current Host Buffer */
#define BAR0_DEC_DMA_SZ        0x0F08 /* DEC DMA Transfer Size (frames) */
#define BAR0_DEC_DMA_JIT       0x0F0C /* DEC DMA Jitter Buffer (frames) */
#define BAR0_DEC_DMS_SG_IDX    0x0F10 /* DEC DMA Scatter Gather List Index */
#define BAR0_DIAL_STR_ARRAY    0x1000 /* 24 Dial string arrays */
#define BAR0_START_DIAL        0x1C00 /* Pulse Dialing Start */
#define BAR0_STOP_DIAL         0x1C04 /* Pulse Dialing Stop */
#define BAR0_PD_MAKE_TIME      0x1C08 /* Pulse Dial Make Time (ms) */
#define BAR0_PD_BREAK_TIME     0x1C0C /* Pulse Dial Break Time (ms) */
#define BAR0_PD_INTER_DIG_TIME 0x1C10 /* Pulse Dial Inter-Digit Time (ms) */
#define BAR0_PD_PAUSE_TIME     0x1C14 /* Pulse Dial Pause Time (ms) */
#define BAR0_SG_LIST_ENTRY     0x2000 /* Scatter Gather List Entry */

#define BAR0_EXP_PRODUCT_ID    0x100000 /* Expansion Module product id */

#ifdef CONFIG_WARP
/* These are the bits we must not touch */
#define IMR_MASK       0x203EF000
#else
#define IMR_MASK        0
#endif

/* These bits are handled by the FPGA and should not be acked by the
 * driver.
 */
#define IMR_FPGA_MASK 0xa0000fff

#ifdef _WIN32

#define inline _inline

#define BAR0_READ(pdx, reg) \
	READ_REGISTER_ULONG((ULONG *)(pdx->bar0 + reg))
#define BAR0_WRITE(pdx, reg, value) \
	WRITE_REGISTER_ULONG((ULONG *)(pdx->bar0 + reg), value)
#define BAR0_READ_BYTE(pdx, reg) \
	READ_REGISTER_UCHAR((UCHAR *)(pdx->bar0 + reg))

#elif defined(__PPC)

#define BAR0_READ(pdx, reg) \
	in_be32((unsigned __iomem *)(pdx->bar0 + reg))
#define BAR0_WRITE(pdx, reg, value) \
	out_be32((unsigned __iomem *)(pdx->bar0 + reg), value)

#else

#define BAR0_READ(pdx, reg) readl(pdx->bar0 + reg)
#define BAR0_WRITE(pdx, reg, value) writel(value, pdx->bar0 + reg)
#define BAR0_READ_BYTE(pdx, reg) readb(pdx->bar0 + reg)

#endif

#define ICCR_MAX_LINES  24 /* 16 + 8 */
#define ICCR_MAX_TRUNKS 24 /* 16 + 8 */
#define ICCR_MAX_PHONES 24 /* 16 + 8 */
#define ICCR_TRUNK_INTS 12
#define ICCR_PHONE_INTS  4


/* Ring generation defines */
/* 12 seconds at 400 msec per */
#define TIC_PERIOD            400 /* 30 bytes */
#define BYTES_PER_CADENCE    (12000 / TIC_PERIOD)
/* BLAW - Fix me.  Define somewhere common! */

struct line_event {
	unsigned char  type;
#define LINE_EVENT   0
#define PLL_EVENT    1
#define DMA_EVENT    2
#define CLOCK_EVENT  3
#define PFT_EVENT    4
#define BUTTON_EVENT 5
	unsigned char  line;
	unsigned short mask;
};
#define LINE_RING_MAX 1000


/* Card states */
#define STATE_ATTACHED 1


#ifdef _WIN32
#  include "daydevice.h"
#else
#  include "daytona.h"
#endif


#define CLOCK_UNITIALIZED 0
#define CLOCK_MASTER      1
#define CLOCK_SLAVE       2
#define CLOCK_SET_SLAVE   3

int daytona_init(PDEVICE_EXTENSION pdx);
void daytona_pll_fxo(PDEVICE_EXTENSION pdx);
void daytona_pll_fxs(PDEVICE_EXTENSION pdx);
int daytona_silabs_reset(PDEVICE_EXTENSION pdx);
int daytona_configure_fxo(PDEVICE_EXTENSION pdx, unsigned mask);
int daytona_configure_fxs(PDEVICE_EXTENSION pdx, unsigned mask);
int taco_probe(PDEVICE_EXTENSION pdx);

void daytona_reset(PDEVICE_EXTENSION pdx);
void daytona_set_clock(PDEVICE_EXTENSION pdx, int clock_mode);
int daytona_get_info(PDEVICE_EXTENSION pdx);

int daytona_trunk_set_config(PDEVICE_EXTENSION pdx, trunk_start_t *rqst);
int daytona_set_threshold(PDEVICE_EXTENSION pdx, u32 trunk,
			  u32 threshold);
int daytona_set_detect(PDEVICE_EXTENSION pdx, u32 trunk,
		       u32 mask);
int daytona_trunk_start(PDEVICE_EXTENSION pdx, u32 trunk);
int daytona_trunk_stop(PDEVICE_EXTENSION pdx, u32 trunk);
int daytona_hook_switch(PDEVICE_EXTENSION pdx, trunk_hookswitch_t *rqst);
int daytona_trunk_info(PDEVICE_EXTENSION pdx, u32 trunk,
		       PKH_TTrunkInfo *info);
int daytona_trunk_dial(PDEVICE_EXTENSION pdx, trunk_dial_t *rqst);
int daytona_trunk_set_audio(PDEVICE_EXTENSION pdx, trunk_audio_t *audio);

int daytona_phone_config(PDEVICE_EXTENSION pdx, u32 phone,
			 u32 intCtrl, u32 audioFormat);
int daytona_phone_start(PDEVICE_EXTENSION pdx, u32 phone);
int daytona_phone_stop(PDEVICE_EXTENSION pdx, u32 phone);
int daytona_phone_ringgen_start(PDEVICE_EXTENSION pdx, phone_ring_t *rqst);
int daytona_phone_ringgen_stop(PDEVICE_EXTENSION pdx, u32 phone);
int daytona_phone_reset(PDEVICE_EXTENSION pdx, u32 phone);
int daytona_phone_reversal(PDEVICE_EXTENSION pdx, int phone);
int daytona_phone_disconnect(PDEVICE_EXTENSION pdx, int phone);
int daytona_set_ring_pattern(PDEVICE_EXTENSION pdx, phone_ring_pattern_t *rqst);
int daytona_phone_set_onhook_tx(PDEVICE_EXTENSION pdx, phone_onhook_tx_t *rqst);
int daytona_phone_set_lf(PDEVICE_EXTENSION pdx, int phone, int direction);
int daytona_phone_info(PDEVICE_EXTENSION pdx, int phone,
		       phone_get_info_out_t *info);
int daytona_get_event(PDEVICE_EXTENSION pdx, char *buffer, int size);
int daytona_phone_set_audio(PDEVICE_EXTENSION pdx, phone_audio_t *audio);
int daytona_phone_get_diag(PDEVICE_EXTENSION pdx, int phone,
			   phone_get_diag_out_t *usrBuf);
int daytona_phone_set_diag(PDEVICE_EXTENSION pdx, phone_set_diag_t *diag);
void daytona_power_down_phones(PDEVICE_EXTENSION pdx);

void imr_set(PDEVICE_EXTENSION pdx, unsigned set);
void imr_clr(PDEVICE_EXTENSION pdx, unsigned clr);

/* Low-level */
int wait_for_ciar(PDEVICE_EXTENSION pdx);
int read_silabs(PDEVICE_EXTENSION pdx, unsigned trunk,
		unsigned reg, u8 *out);
int write_silabs(PDEVICE_EXTENSION pdx, unsigned trunk,
		 unsigned reg, unsigned data);
void write_silabs_indirect(void __iomem *fpga, 
		unsigned char channel, 
		unsigned char ind_reg_offset, 
		unsigned short data);
#define BCAST 0x80
static inline void WriteSilabsIndirect(PDEVICE_EXTENSION pdx, u8 channel,
			 u8 ind_reg_offset , u16 data)
{
        return write_silabs_indirect(pdx->bar0, channel, ind_reg_offset, data);
}

int read_flash(PDEVICE_EXTENSION pdx, u8 *flash);

void daytona_hook_timer(unsigned long arg);


/* Clocking */
void daytona_set_clock(PDEVICE_EXTENSION pdx, int clock_mode);
void daytona_clock_changed(int master);
void daytona_clock_tick(void);
void daytona_clock_reset(void);
int get_master_clock(PDEVICE_EXTENSION pdx);

/* DMA */
#ifdef CONFIG_WARP
int dma_init_module(PDEVICE_EXTENSION pdx);
void dma_exit_module(void);
#else
#define dma_init_module(p)
#define dma_exit_module
#endif
int daytona_dma_init(PDEVICE_EXTENSION pdx);
void daytona_dma_fini(PDEVICE_EXTENSION pdx);
int daytona_dma_enable(PDEVICE_EXTENSION pdx);
void daytona_dma_disable(PDEVICE_EXTENSION pdx);
void daytona_dma_ISR(PDEVICE_EXTENSION pdx);
void do_dma(void *context, unsigned iccr);

/* events */
int add_event(PDEVICE_EXTENSION pdx, unsigned type,
	      unsigned line, unsigned mask);

#ifdef __linux__
#include <linux/version.h>

# if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
irqreturn_t daytona_ISR(int irq, void *context, struct pt_regs *regs);
# else
irqreturn_t daytona_ISR(int irq, void *context);
# endif
static inline void DAYDEV_wake_up(PDEVICE_EXTENSION pdx)
{
  wake_up(&pdx->read_queue);
}
#elif defined(_WIN32)
BOOLEAN daytona_ISR(PKINTERRUPT interrupt, PVOID context);
static _inline void DAYDEV_wake_up(PDEVICE_EXTENSION pdx)
{
  KeInsertQueueDpc(&pdx->dpc, 0, 0);
}
#endif

int taco_audio_init(PDEVICE_EXTENSION pdx);
int taco_audio_fini(PDEVICE_EXTENSION pdx);
int taco_audio_gain(PDEVICE_EXTENSION pdx, audio_gain_t *rqst);

int add_event(PDEVICE_EXTENSION pdx, unsigned type,
	      unsigned line, unsigned mask);


/* -------------------------------------------------------------------- */
/* Some OS specific stuff */

#ifdef _WIN32
#define PK_PCI_BUS(pdx)  ((pdx)->PciData.BusNo)
#define PK_PCI_SLOT(pdx) ((pdx)->PciData.SlotNo)
#define PK_PCI_IRQL(pdx) ((pdx)->InterruptDesc.Level)

#define schedule_bh(bh)  KeInsertQueueDpc(bh, NULL, NULL)
#else
#define PK_PCI_BUS(pdx)  ((pdx)->pdev->bus->number)
#define PK_PCI_SLOT(pdx) PCI_SLOT((pdx)->pdev->devfn)
#define PK_PCI_IRQL(pdx) ((pdx)->pdev->irq >> 8)

#define schedule_bh(bh)  tasklet_schedule(bh)

extern const struct file_operations daytona_fops;

#endif

#ifdef DMA_NETLINK
int dmanl_init(void);
void dmanl_exit(void);
void dmanl_start(PDEVICE_EXTENSION pdx);
void dmanl_stop(PDEVICE_EXTENSION pdx);
void dma_netlink(PDEVICE_EXTENSION pdx, int index);
#else
#define dmanl_init()
#define dmanl_exit()
#define dmanl_start(pdx)
#define dmanl_stop(pdx)
#define dma_netlink(pdx, index)
#endif

#endif

#ifdef CONFIG_WARP
void taco_fan_error(void);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 25)
#define pika_dtm_register_shutdown dtm_register_shutdown
#define pika_dtm_unregister_shutdown dtm_unregister_shutdown
#endif
#endif

/* debug modes */
#define debug_clocking (debug & (1 << DBGT_BRD_CLOCKING))

/*
 * Force kernel coding standard on PIKA code
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
