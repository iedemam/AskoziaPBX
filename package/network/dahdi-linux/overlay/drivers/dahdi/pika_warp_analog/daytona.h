#ifndef DAYTONA_H
#define DAYTONA_H

#include "pk_ioctl.h"
#include "os.h"

#include "hspconfig.h"

#ifdef __linux__
#include <linux/spinlock.h>
#include <linux/pci.h>
#include <asm/uaccess.h>

#include "pikacore.h"

extern struct list_head board_list;

void phone_tasklet(unsigned long arg);
void ring_tasklet(unsigned long arg);
void line_tasklet(unsigned long arg);
#else
#endif

#include "delay.h"

struct ring_cadence {
	unsigned char load;
	unsigned char cur_brush[ICCR_MAX_PHONES];
};


typedef struct DEVICE_EXTENSION {
#ifdef __linux__
	struct list_head list;
	atomic_t open_count;

	wait_queue_head_t read_queue;

	struct tasklet_struct phone_bh;
	struct tasklet_struct ring_bh;
	struct tasklet_struct line_bh;

	/* stat mask shadows */
	spinlock_t stat_lock;
	unsigned lc_mask; /* slic LC detect */
	unsigned rt_mask; /* slic Ring Trip detect */

# ifdef CONFIG_WARP
	struct platform_device *dev;
	struct task_struct *pft;
	unsigned pft_mask;
# else
	struct pci_dev *pdev;
# endif
#else
	WIN32_DEVICE_EXTENSION;
	HSPCFG_DEVINFO  hspDevInfo;
	IO_REMOVE_LOCK  RemoveLock;

	/* This must match the linux list_head name. */
	struct DEVICE_EXTENSION *list;

	KDPC phone_bh;
	KDPC ring_bh;
	KDPC line_bh;

	u32     syncIrql;
	KSPIN_LOCK  *isrSyncSpinLock;
#endif

	/* For Linux the id is the minor number. */
	int id;
	int card_type; /* real card type */

	s32      cardid;
	u32     card_state;

	u8 *bar0;

	int calibrated; /* only set on warped boards */
	int manual_calibration_done;	/* manual calibration done flag */
	u8 ring_gain_value[ICCR_MAX_PHONES];
	u8 tip_gain_value[ICCR_MAX_PHONES];

	struct line_event line_ring[LINE_RING_MAX];
	int ring_head, ring_tail;

	/* These need to be long for bitops */
	unsigned long trunk_mask;
	unsigned long phone_mask;
	unsigned long disabled_mask;

	PKH_TBoardInfo info;
	HSPCFG_DATA  hsp_config;

	unsigned line_mask[ICCR_MAX_LINES]; /* ISR only */
	unsigned imr;

	unsigned long tics;

	spinlock_t event_lock;

	/* DMA stuff */
	u8 *rx_buf, *tx_buf;
#ifndef CONFIG_WARP
# ifdef __linux__
	dma_addr_t dma_handle;
	int dma_size;
# endif
	u8 *dma_buf;
# ifdef NO_DMA
	struct timer_list dma_timer;
	int dma_timer_enabled;
# endif
#endif
	unsigned underrun_count; /* SAM DBG */

	int clock_mode;

	/* This state is needed for reversal debounce. */
	unsigned char line_state[ICCR_MAX_LINES];
	/* Phone states */
#define PHONE_STATE_DEFAULT         0   /* onhook, ringgen off, forward. */
#define STATE_BIT_OFFHOOK           0x01 /* must be 1 */
#define STATE_BIT_ONHOOK_TX_ENABLED 0x02
#define STATE_BIT_DISCONNECTED      0x04  /* valid only when offhook */
#define STATE_BIT_RINGGEN_ENABLED   0x10  /* IMPORTANT! - this bit
					   * cannot overlap with any
					   * trunk states */
#define STATE_BIT_RINGING           0x20
#define STATE_POWER_ALARM	    0x40
#define STATE_POWER_ALARM_DEBOUNCE  0x80
	/* Trunk states */
#define STATE_ONHOOK    0
#define STATE_OFFHOOK   STATE_BIT_OFFHOOK
#define STATE_HOOKING   2
#define STATE_RINGING   STATE_BIT_RINGING
#define STATE_HOOKFLASH 4
#define STATE_DIALING   8

	/* Every line can have it's own mask. */
	unsigned line_imr[ICCR_MAX_LINES];

	/* This is used for both the timeout and the debounce
	 * depending on the STATE_POWER_ALARM_DEBOUNCE state
	 */
#define POWER_ALARM_POLL_TIMEOUT 5000
#define POWER_ALARM_DEBOUNCE     1000
	struct delay power_alarm_timeout[ICCR_MAX_LINES];

	/* On/off hook delay in tics. */
	struct delay trunk_delay[ICCR_MAX_LINES];
	unsigned long onhook_debounce;
	unsigned long offhook_debounce;

	/* Reversal debounce in tics. */
	struct delay reversal_delay[ICCR_MAX_LINES];
	unsigned long reversal_debounce;

	/* ring generation */
	unsigned max_ring_load;
	unsigned max_ring_latency;
	u8 ring_patterns[PKH_BOARD_MAX_RING_PATTERNS][BYTES_PER_CADENCE];
	struct ring_cadence ring_cadences[BYTES_PER_CADENCE];
	struct delay ring_timeout[ICCR_MAX_PHONES];
	struct delay ring_ticker;
	unsigned     ring_slot;
	unsigned     ring_bit;

	/* Hardware loop current closure debounce time */
	unsigned      loop_debounce;

	/* This lock protects the cadence load variable.
	 * It also protects the changing of the ringgen state.
	 */
	spinlock_t ringgen_lock;

	/* Hookflash delay. */
	struct delay hookflash_delay[ICCR_MAX_LINES];
	unsigned long hookflash_debounce;

	/* Disconnect duration */
	struct delay disconnect_delay[ICCR_MAX_LINES];
	unsigned long disconnect_debounce;

	/* check current after disconnect to detect onhooks during
	 * disconnect.
	 */
	struct delay disconnect_hookstate_delay[ICCR_MAX_LINES];
	int disconnect_hookstate_debounce;

	/* Onhook Tx OFF delay */
	/* We need to delay before turning off Onhook TX. User mode tells us
	 * to turn it off once a WRITE process has finished writing to the
	 * media stream, but this does not mean that that data has been
	 * transmitted.  50 ms based on a 40 ms buffer + a bit.
	 */
#define ONHOOK_TX_OFF_DELAY         50
	struct delay onhook_tx_off_delay[ICCR_MAX_LINES];

	/* Stores line feed setting (except ringing and disconnect) */
	u8 lf[ICCR_MAX_PHONES];

	unsigned int threshold_debounce; /* set in FPGA */

	/* Callback function for event reporting */
	s32 (*add_event_callback)(struct DEVICE_EXTENSION *pdx, unsigned type, unsigned line, unsigned mask);

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


/* Handy macro in case tic size ever changes. */
#define msecs_to_tics(ms) (ms)

#define TRUNK_MASK_SIZE PKH_BOARD_MAX_NUMBER_OF_TRUNKS
#define trunk_for_each(i)						\
	for (i = find_first_bit(&pdx->trunk_mask, TRUNK_MASK_SIZE);	\
	     i < TRUNK_MASK_SIZE;					\
	     i = find_next_bit(&pdx->trunk_mask, TRUNK_MASK_SIZE, i + 1))

#define PHONE_MASK_SIZE PKH_BOARD_MAX_NUMBER_OF_PHONES
#define phone_for_each(i)                                               \
        for (i = find_first_bit(&pdx->phone_mask, PHONE_MASK_SIZE);     \
             i < PHONE_MASK_SIZE;                                       \
             i = find_next_bit(&pdx->phone_mask, PHONE_MASK_SIZE, i + 1))

#endif

/*
 * Force kernel coding standard on PIKA code
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
