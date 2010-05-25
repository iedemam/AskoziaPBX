#include "driver.h"
#include "pksystemver.h"
#include "hmpioconvert.h"
#include "wa_common.h"

#ifdef _WIN32
int debug;

static _inline int copy_to_user(void *to, void *from, int size)
{
	memcpy(to, from, size);
	return 0;
}

#define IRQ_NONE 0
#define IRQ_HANDLED 1
#define EFAULT 1

#define list_for_each_entry(p, head, unused) for (p = *(head); p; p = p->list)

#endif


void imr_set(PDEVICE_EXTENSION pdx, unsigned set)
{
	unsigned imr = BAR0_READ(pdx, BAR0_IMR);
	set &= ~IMR_MASK;
	BAR0_WRITE(pdx, BAR0_IMR, imr | set);
	pdx->imr |= set;
}

void imr_clr(PDEVICE_EXTENSION pdx, unsigned clr)
{
	unsigned imr = BAR0_READ(pdx, BAR0_IMR);
	clr &= ~IMR_MASK;
	BAR0_WRITE(pdx, BAR0_IMR, imr & ~clr);
	pdx->imr &= ~clr;
}

static inline void set_state(PDEVICE_EXTENSION pdx, int trunk, int state)
{
	pdx->line_state[trunk] |= state;
}


static inline void clr_state(PDEVICE_EXTENSION pdx, int trunk, int state)
{
	pdx->line_state[trunk] &= ~state;
}


void daytona_reset(PDEVICE_EXTENSION pdx)
{
	unsigned reg, val;
	int i;

	/* Disable and ack all interrupts. */
	imr_clr(pdx, ~IMR_MASK);
	val = BAR0_READ(pdx, BAR0_ICCR);
	val &= ~IMR_MASK;
	BAR0_WRITE(pdx, BAR0_ICCR, val);

	/* Disable DMA. */
	BAR0_WRITE(pdx, BAR0_DEC_DMA_CTRL, 0);

	/* Make sure clock is back to master. */
	daytona_set_clock(pdx, CLOCK_MASTER);

	if (pdx->trunk_mask) {
		unsigned mask = pdx->trunk_mask;

		/* Put all lines onhook - set onhook monitor off */
		/* Due to a bug in fpga <= 1.3.3.6 we not use BCAST */
		for (i = 0; mask; ++i, mask >>= 1)
			if (mask & 1)
				write_silabs(pdx, i, 5, 0);

		/* Disable ring detection in Silabs. */
		write_silabs(pdx, BCAST, 3, 0);
	}

	daytona_power_down_phones(pdx);

	/* Clear out all the delays */
	memset(pdx->trunk_delay, 0, sizeof(pdx->trunk_delay));
	memset(pdx->reversal_delay, 0, sizeof(pdx->reversal_delay));
	memset(pdx->hookflash_delay, 0, sizeof(pdx->hookflash_delay));
	memset(pdx->disconnect_delay, 0, sizeof(pdx->disconnect_delay));
	memset(pdx->disconnect_hookstate_delay, 0,
	       sizeof(pdx->disconnect_hookstate_delay));
	memset(pdx->onhook_tx_off_delay, 0, sizeof(pdx->onhook_tx_off_delay));

	/* Clear all status registers */
	for (reg = BAR0_ABV_THRSH_STAT; reg <= BAR0_HKFLSH_STAT; reg += 4)
		BAR0_WRITE(pdx, reg, 0);

	pdx->ring_head = pdx->ring_tail = 0;

	/* Just in case the diag was set... */
	val = BAR0_READ(pdx, BAR0_DIAG);
	BAR0_WRITE(pdx, BAR0_DIAG, val & 1);

#ifdef CONFIG_WARP
	memset(pdx->line_state, 0, sizeof(pdx->line_state));
#endif

	if (dbg)
		PK_PRINT("%d: daytona_reset called (%d)\n",
			 pdx->id, pdx->cardid);
}


int daytona_init(PDEVICE_EXTENSION pdx)
{
	int rc;
	unsigned prod;
	unsigned fxo_mask = 0, fxs_mask = 0, exp_mask = 0;

	if (dbg)
		PK_PRINT("%d: daytona_init started...\n", pdx->id);

	pdx->phone_mask = 0;
	pdx->info.numberOfPhones = 0;

	switch (pdx->card_type) {
	case 0x110:
		daytona_pll_fxo(pdx);
		fxo_mask = 0x00ffff;
		exp_mask = 0xff0000;
		break;
	case 0x115:
		daytona_pll_fxs(pdx);
		fxs_mask = 0x00fff;
		exp_mask = 0xff000;
		break;
	case 0x210:
		return taco_probe(pdx);
	default:
		PK_PRINT("daytona_init: invalid cardID %x\n", pdx->card_type);
		return -1;
	}


	/* Handle expansion module if present */
	prod = BAR0_READ(pdx, BAR0_FPGA_REV);
	if (prod & (1 << 17)) {
		prod = BAR0_READ(pdx, BAR0_EXP_PRODUCT_ID);
		if (prod == 9900840)
			fxo_mask |= exp_mask;
		else if (prod == 9900850)
			fxs_mask |= exp_mask;
		else if (prod == 0xffffffff)
			PK_PRINT("FPGA says there is a module, "
				 "but it does not have a product id\n");
	}

	if (fxo_mask) {
		rc = daytona_configure_fxo(pdx, fxo_mask);
		if (rc)
			return rc;
	}
	if (fxs_mask) {
		rc = daytona_configure_fxs(pdx, fxs_mask);
		if (rc)
			return rc;
	}

	return 0;
}


int daytona_silabs_reset(PDEVICE_EXTENSION pdx)
{
	unsigned reset;

#ifdef CONFIG_WARP
	unsigned config;

	/* Turn off PCM clocks to the Audio port.
	 * WARP only - PCIe bus read only reg on GW. */
	config = BAR0_READ(pdx, BAR0_CONFIG);
	config &= ~1;
	BAR0_WRITE(pdx, BAR0_CONFIG, config);
#endif

	/* Take Silabs out of reset. Pulse Silabs reset Low for min of
	 * 2.5 us.
	 *
	 * Warped Note: We also reset the BRI and the GSM modules
	 * since they are affected by the SPI reset. */
	reset = BAR0_READ(pdx, BAR0_RESET);
	reset &= ~FPGA_RESET_CODEC_BIT;  /* Needed for GW and WARP */
#ifdef CONFIG_WARP
	/* WARP only - clears clock recovery on GW - See MI6809 */
	reset &= ~FPGA_RESET_BRI_BIT;
	/* WARP Only - not used on GW */
	reset &= ~FPGA_RESET_GSM_BIT;
#endif
	reset |= FPGA_RESET_SPI_BIT; /* Needed for GW and WARP */
	BAR0_WRITE(pdx, BAR0_RESET, reset);

	udelay(3);

	reset |= FPGA_RESET_CODEC_BIT; /* Needed for GW and WARP */
#ifdef CONFIG_WARP
	/* WARP only - clears clock recovery on GW - See MI6809 */
	reset |= FPGA_RESET_BRI_BIT;
	/* WARP Only - not used on GW */
	reset |= FPGA_RESET_GSM_BIT;
#endif
	BAR0_WRITE(pdx, BAR0_RESET, reset);

	/*  Wait for the Silabs PLL to lock */
	mdelay(2); /* not needed for WARP, but required for GW cards */

	/* Release Spi_Arbiter reset (it will switch the silabs mode
	 * from 2-byte to 3-byte)
	 */
	reset &= ~FPGA_RESET_SPI_BIT; /* Needed for GW and WARP */
	BAR0_WRITE(pdx, BAR0_RESET, reset);

	return wait_for_ciar(pdx); /* Needed for both GW and Warp */
}


void daytona_pll_fxo(PDEVICE_EXTENSION pdx)
{
	unsigned val;

	/*  Set PLL to freerun mode */
	BAR0_WRITE(pdx, BAR0_PLL_CONTROL, 1);

	val = BAR0_READ(pdx, BAR0_RESET);

	/*  Take PLL out of rst */
	val |= 0x2;
	BAR0_WRITE(pdx, BAR0_RESET, val);
	delay(8); /* wait 8 ms for PLL to stabilize */

	if (debug_clocking)
		PK_PRINT("%d: daytona_pll_fxo\n", pdx->id);

	daytona_silabs_reset(pdx);
}

#define TRUNK_MASK_SIZE PKH_BOARD_MAX_NUMBER_OF_TRUNKS
#define trunk_for_each(i)						\
	for (i = find_first_bit(&pdx->trunk_mask, TRUNK_MASK_SIZE);	\
	     i < TRUNK_MASK_SIZE;					\
	     i = find_next_bit(&pdx->trunk_mask, TRUNK_MASK_SIZE, i + 1))

static inline void disable_trunk(PDEVICE_EXTENSION pdx, int chan)
{
	pdx->trunk_mask &= ~(1 << chan);
}

#ifdef CONFIG_WARP
int get_master_clock(PDEVICE_EXTENSION pdx) 
{ 
	return 0; 
}
#else
int get_master_clock(PDEVICE_EXTENSION pdx)
{
#ifdef _WIN32
	return (*(pdx->hspDevInfo.pFuncTbl->GetMasterClock))
		(pdx->hspDevInfo.pDevExt);
#else
	return K__HspGetMasterClock();
#endif
}
#endif

int daytona_configure_fxo(PDEVICE_EXTENSION pdx, unsigned mask)
{
	u8 data;
	unsigned val, count = 0;
	int i;

	/* Reset these up front. */
	pdx->trunk_mask = mask;
	pdx->info.numberOfTrunks = 0;

	if (mask == 0)
		return 0;

	/*  SiLabs SW reset */
	write_silabs(pdx, BCAST, 1, 0x80);
	delay(2);

	/*  Power up line side devices */
	write_silabs(pdx, BCAST, 6, 0);

	/* Wait for line side devices to frame up - roughly 600us.  We wait
	 * a millisecond and then we usually get through the loop with count
	 * 0.
	 */
	delay(1);
	trunk_for_each(i) {
		data = 0;
		read_silabs(pdx, i, 11, &data);
		if (data) { /* Device present. */
			data = 0;
			while (1) {
				read_silabs(pdx, i, 12, &data);
				if (data & (1 << 6))
					break;
				if (++count > 1000) {
					PK_PRINT("SILAB FRAME UP FAILED!!!!\n");
					return -ETIMEDOUT;
				}
				ndelay(100);
			}
		}
	}

	/*  Initialize Registers */
	write_silabs(pdx, BCAST, 2, 7);

	/* All trunks onhook - onhook monitor on */
	/* Due to a bug in fpga <= 1.3.3.6 we not use BCAST */
	trunk_for_each(i)
		write_silabs(pdx, i, 5, 8);

	/* Line Voltage Force Disable - do not force to 0 */
	write_silabs(pdx, BCAST, 31, 0x21);

	/*  Set PCM TX/RX channel locations */
	trunk_for_each(i) {
		val = (i << 4) | 1;
		write_silabs(pdx, i, 34, val & 0xff);
		write_silabs(pdx, i, 35, val >> 8);
		write_silabs(pdx, i, 36, val & 0xff);
		write_silabs(pdx, i, 37, val >> 8);
	}
	write_silabs(pdx, BCAST, 33, 0x28);

	/*  Read Silabs revision to get number of lines */
	trunk_for_each(i) {
		data = 0;
		read_silabs(pdx, i, 11, &data);
		if ((data & 0xf0) == 0x30 && (data & 0xf) >= 4)
			++pdx->info.numberOfTrunks;
		else {
			disable_trunk(pdx, i);
			PK_PRINT("%d: FXO %d Bad silabs rev %x\n",
				 pdx->id, i, data);
		}
	}

	pdx->info.trunkMask = pdx->trunk_mask;

	/* Initialize the Ring Validation - Default is 512 of delay on ring
	 * detection. We want 0. We also reduce the ring confirmation for
	 * detecting 200ms bursts in the UK.
	 */
	write_silabs(pdx, BCAST, 22, 0x16);
	write_silabs(pdx, BCAST, 23, 0x09);
	write_silabs(pdx, BCAST, 24, 0x99);

	/* Set initial mode */
	if (pdx->cardid != -1) {
		if (get_master_clock(pdx) != -1) {
			if (pdx->cardid == get_master_clock(pdx))
				daytona_set_clock(pdx, CLOCK_MASTER);
			else
				daytona_set_clock(pdx, CLOCK_SLAVE);
		}
	}
	if (dbg)
		PK_PRINT("%d: daytona_init done. FPGA_REV %x\n",
			 pdx->id, BAR0_READ(pdx, BAR0_FPGA_REV));

	/* TT#4237 We don't know why we need it, but if we do not delay
	 * initial reads of the line voltage can be wrong.
	 */
	delay(100);

	/* BLAW - 11.3 in interface doc */
	/* Detection */

	/* This cannot be a broadcast due to a bug in fpga <= 1.3.3.6 */
	trunk_for_each(i)
		write_silabs(pdx, i, 43, 4);

	val = (pdx->threshold_debounce + 7) / 8;
	if (val == 0)
		val = 1;
	else
		val &= 0xff;
	BAR0_WRITE(pdx, BAR0_THRSH_DET_DBNC_TIME, val);

	/* enable global interrupts in each Silabs */
	write_silabs(pdx, BCAST, 2, 0x87);

	/* Enable line interface interrupts in FPGA */
	BAR0_WRITE(pdx, BAR0_THRSH_INT_MASK, pdx->trunk_mask);

	/* make sure threshold detect is taken out of reset. */
	val = BAR0_READ(pdx, BAR0_RESET) & ~0x8;
	BAR0_WRITE(pdx, BAR0_RESET, val);

	/* wait after to allow threshold det module to init */
	delay(1);

	return 0;
}

int daytona_get_info(PDEVICE_EXTENSION pdx)
{
	PKH_TBoardEEPROM *eeprom = &pdx->info.eeprom;

#ifdef CONFIG_WARP
	pdx->card_type = 0x210;
#else
	FLASH_FIELDS_STRUCT flash;

	if (read_flash(pdx, (u8 *)&flash)) {
		PK_PRINT("Unable to read eeprom\n");
		return -EIO;
	}

	/* Real card type */
	pdx->card_type = flash.CardID_LSW | (flash.CardID_MSW << 16);

	/* Fill in the eeprom data. */
	eeprom->serialNumber = flash.SerialNumber_LSW |
		(flash.SerialNumber_MSW << 16);
	eeprom->productRevision = flash.ProductRev;
	memcpy(eeprom->productNumber, flash.ProductNumber,
	       sizeof(flash.ProductNumber));
	memcpy(eeprom->licenseInfo, flash.LicensingInfo,
	       sizeof(flash.LicensingInfo));
#endif
	eeprom->cardID = pdx->card_type;

	/* Fill in the board info. */
	strcpy(pdx->info.driverVersion, PikaLoadVerString);
	pdx->info.FPGA_version = BAR0_READ(pdx, BAR0_FPGA_REV) & 0xffff;
#ifdef _WIN32
	/* SAM This should be moved to the win32 code once I find the
	 * correct place */
	pdx->info.busId  = PK_PCI_BUS(pdx);
	pdx->info.slotId = PK_PCI_SLOT(pdx);
	pdx->info.irql   = PK_PCI_IRQL(pdx);
#endif

	if ((pdx->info.FPGA_version & 0x10000) == 1) {
		PK_PRINT("RUNNING SAFE VERSION OF FPGA!\n");
		return -ENODEV;
	}

	return 0;
}


/* These are truely global accross boards. */
static unsigned long board_clock_changed;
static unsigned long board_clock_state; /* Set bit means slave */


static inline void handle_reversal(PDEVICE_EXTENSION pdx, int i)
{
	pdx->line_mask[i] &= ~4; /* do not send reversal now */
	if ((pdx->line_imr[i] & 4) &&
	    (pdx->line_state[i] & ~STATE_BIT_OFFHOOK) == 0) {
		if (!DELAY_ISSET(&pdx->reversal_delay[i]))
			/* start reversal debounce if not started before */
			DELAY_SET(&pdx->reversal_delay[i],
				  pdx->reversal_debounce);
	}
}

static inline void set_line_masks(PDEVICE_EXTENSION pdx, int set, unsigned mask)
{
	int i;

	for (i = 0; i < ICCR_MAX_LINES && mask; ++i, mask >>= 1)
		if (mask & 1) {
			pdx->line_mask[i] |= set;
			/* It is more efficient to clear this here where we know
			 * something has changed.
			 */
			pdx->line_mask[i] &= pdx->line_imr[i];

			/* This is all for reversals */
			switch (set) {
			case 0x200: /* RON */
				DELAY_CLR(&pdx->reversal_delay[i]);
				set_state(pdx, i, STATE_RINGING);
				break;
			case 0x100: /* ROFF */
				clr_state(pdx, i, STATE_RINGING);
				break;
			case 1: /* Hookflash */
				clr_state(pdx, i, STATE_HOOKFLASH);
				break;
			case 2: /* Dial */
				clr_state(pdx, i, STATE_DIALING);
				break;
			case 4: /* Reversal */
				if (pdx->reversal_debounce)
					handle_reversal(pdx, i);
				break;
			}
		}
}


/* This is the only place that changes the ring_head, except for a reset. */
int add_event_hmp(PDEVICE_EXTENSION pdx, unsigned type,
	      unsigned line, unsigned mask)
{
	struct line_event *event;
	unsigned long flags;

/* In Windows, an interrupt spinlock is required if this add_event is NOT called
   from an interrupt.  This locks against the ISR. */
#ifdef _WIN32
	KIRQL oldIrql;
	PK_BOOL locked = PK_FALSE;
	if (KeGetCurrentIrql() <= DISPATCH_LEVEL) {
		oldIrql = KeAcquireInterruptSpinLock(pdx->IntObject);
		locked = PK_TRUE;
	}
#else
	spin_lock_irqsave(&pdx->event_lock, flags);
#endif

	event = &pdx->line_ring[pdx->ring_head];
	event->type = type;
	event->line = line;
	event->mask = mask;
	if (++pdx->ring_head == LINE_RING_MAX)
		pdx->ring_head = 0;
	if (pdx->ring_head == pdx->ring_tail)
		if (++pdx->ring_tail == LINE_RING_MAX)
			pdx->ring_tail = 0;

#ifdef _WIN32
	if (locked)
		KeReleaseInterruptSpinLock(pdx->IntObject, oldIrql);
#else
	spin_unlock_irqrestore(&pdx->event_lock, flags);
#endif

	DAYDEV_wake_up(pdx);

	return 0;
}

/* the generic add event function calls the registered add_event() function for the corresponding device extension */
int add_event(PDEVICE_EXTENSION pdx, unsigned type,
	      unsigned line, unsigned mask)
{
	if (pdx->add_event_callback != NULL) {
		return pdx->add_event_callback(pdx, type, line, mask);
	} else {
		PK_PRINT ("discarding event type %d line %d mask 0x%08x for pdx->id %d (ERROR : no handler)\n",
			  type, line, mask, pdx->id);
		return (-1);
	}
}

//static inline void do_trunks(PDEVICE_EXTENSION pdx, unsigned iccr)
void do_trunks(PDEVICE_EXTENSION pdx, unsigned iccr)
{
	unsigned reg, set, stat_mask;
	int i;

	if (iccr & 0xfff) {
		reg = BAR0_HKFLSH_STAT;
		set = 1;
		for (i = 0; i < ICCR_TRUNK_INTS; ++i, set <<= 1, reg -= 4) {
			if (iccr & set) {
				stat_mask = BAR0_READ(pdx, reg);
				BAR0_WRITE(pdx, reg, ~stat_mask);
				set_line_masks(pdx, set, stat_mask);
			}
		}
	}

	/* Put the line events in the ring buffer. */
	for (i = 0; i < ICCR_MAX_LINES; ++i) {
		if (pdx->line_mask[i]) {
			add_event(pdx, LINE_EVENT, i, pdx->line_mask[i]);
			pdx->line_mask[i] = 0;
		}

		if (DELAY_UP(&pdx->trunk_delay[i])) {
			add_event(pdx, LINE_EVENT, i, 1 << 12);
			DELAY_CLR(&pdx->trunk_delay[i]);
			clr_state(pdx, i, STATE_HOOKING);
		}

		if (DELAY_UP(&pdx->reversal_delay[i])) {
			add_event(pdx, LINE_EVENT, i, 4);
			DELAY_CLR(&pdx->reversal_delay[i]);
		}
	}
}

static inline void alarmed(PDEVICE_EXTENSION pdx, int i)
{
	if ((pdx->phone_mask & (1 << i)) == 0) {
		if ((pdx->line_state[i] & STATE_POWER_ALARM) == 0) {
			PK_PRINT("%d.%02d: Spurious power alarm!\n",
				 pdx->id, i);
			/* mark as alarmed */
			pdx->line_state[i] = STATE_POWER_ALARM;
		}
		return; /* don't set timer */
	}

	if ((pdx->line_state[i] & STATE_POWER_ALARM) == 0) {
		add_event(pdx, LINE_EVENT, i, 8);
		pdx->disabled_mask |= 1 << i;
		PK_PRINT("%d.%02d: POWER ALARM!\n", pdx->id, i);
	}

	pdx->line_state[i] = STATE_POWER_ALARM; /* reset state completely */
	DELAY_SET(&pdx->power_alarm_timeout[i], POWER_ALARM_POLL_TIMEOUT);
}

static inline void do_power_alarms(PDEVICE_EXTENSION pdx)
{
	unsigned reg, stat_mask;
	unsigned alarms = 0;
	int i;

	for (reg = BAR0_Q6_PWR_ALRM_STAT;
	     reg <= BAR0_Q1_PWR_ALRM_STAT;
	     reg += 4) {
		stat_mask = BAR0_READ(pdx, reg);
		if (stat_mask) {
			BAR0_WRITE(pdx, reg, ~stat_mask);
			alarms |= stat_mask;
		}
	}

	if (alarms)
		for (i = 0; i < ICCR_MAX_LINES; ++i)
			if (alarms & (1 << i))
				alarmed(pdx, i);
}

//static inline void do_phones(PDEVICE_EXTENSION pdx, unsigned iccr)
void do_phones(PDEVICE_EXTENSION pdx, unsigned iccr)
{
	unsigned stat_mask;

	if (iccr & 0x03000000) { /* Ring trip or LC */
#ifdef __linux__
		spin_lock(&pdx->stat_lock);
		if (iccr & 0x01000000) { /* Ring trip */
			stat_mask = BAR0_READ(pdx, BAR0_RNG_TRP_STAT);
			BAR0_WRITE(pdx, BAR0_RNG_TRP_STAT, ~stat_mask);
			pdx->rt_mask |= stat_mask;
		}
		if (iccr & 0x02000000) { /* LC */
			stat_mask = BAR0_READ(pdx, BAR0_LC_TRANS_STAT);
			BAR0_WRITE(pdx, BAR0_LC_TRANS_STAT, ~stat_mask);
			pdx->lc_mask |= stat_mask;
		}
		spin_unlock(&pdx->stat_lock);
#else
		/* SAM FIXME? */
#endif
		schedule_bh(&pdx->phone_bh);
	}

	/* Power alarms */
	if (iccr & 0x04000000)
		do_power_alarms(pdx);

	/* bottom half is required in Windows because of locking. */
	schedule_bh(&pdx->line_bh);

	/* handle ringing every 50ms */
	if (DELAY_UP(&pdx->ring_ticker)) {
		DELAY_SET(&pdx->ring_ticker, 50);
		schedule_bh(&pdx->ring_bh);
	}
}

#ifndef CONFIG_WARP
void do_dma(void *context, unsigned iccr)
{
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)context;

	if (iccr & 0x40000) { /* DMA underrun */
		PK_PRINT("%d: DMA underrun %d\n",
			 pdx->id, pdx->underrun_count);
		add_event(pdx, DMA_EVENT, 0, 0);
		if (++pdx->underrun_count >= 2) /* SAM DBG */
			daytona_dma_disable(pdx); /* SAM DBG */
	} else if (iccr & 0x80000) { /* DMA done */
		++pdx->tics;

		pdx->underrun_count = 0;

		daytona_dma_ISR(pdx);
	}
}
#endif

#ifdef __linux__
# if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
irqreturn_t daytona_ISR(int irq, void *context, struct pt_regs *regs)
# else
irqreturn_t daytona_ISR(int irq, void *context)
# endif
#else
BOOLEAN daytona_ISR(PKINTERRUPT Interrupt, void *context)
#endif
{
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)context;
	unsigned iccr, stat_mask;

	iccr = BAR0_READ(pdx, BAR0_ICCR);

	stat_mask = iccr & ~(IMR_FPGA_MASK | IMR_MASK);
	if (stat_mask)
		BAR0_WRITE(pdx, BAR0_ICCR, stat_mask);

	iccr &= pdx->imr; /* mask out ignored ints */

#ifdef CONFIG_WARP
	/* We need to update tics */
	pdx->tics = jiffies_to_msecs(jiffies);
#else
	/* On the warp we always have to run through the ISR since the
	 * phone and trunk code relies on the DMA done interrupt for
	 * timing releated features (e.g. offhook)
	 */
	if (iccr == 0)
		return IRQ_NONE;

	/* -------------------------------------------------------------- */
	/* DMA */

	do_dma(pdx, iccr);
#endif

	/* -------------------------------------------------------------- */
	/* PLL */

	if (iccr & 0x0013C000) /* PLL */
		add_event(pdx, PLL_EVENT, 0, iccr >> 12);

	/* -------------------------------------------------------------- */
	/* Phones */

	if (pdx->phone_mask)
		do_phones(pdx, iccr);

	/* -------------------------------------------------------------- */
	/* Trunks */

	if (pdx->trunk_mask)
		do_trunks(pdx, iccr);

	/* -------------------------------------------------------------- */
	/* Clocking */

	if (board_clock_changed & (1 << pdx->cardid)) {
		clear_bit(pdx->cardid, &board_clock_changed);
		add_event(pdx, CLOCK_EVENT, 0,
			  board_clock_state & (1 << pdx->cardid));
	}

	/* Do one last read to flush the acks. */
	BAR0_READ(pdx, BAR0_ICCR);

	return IRQ_HANDLED;
}

/* This is called from the read. This should be the only place that changes the
 * ring_tail except for a reset. However, the add_event will also push the tail
 * if it overflows.
 */
int daytona_get_event(PDEVICE_EXTENSION pdx, char *buffer, int size)
{
	struct line_event *event = &pdx->line_ring[pdx->ring_tail];
	TAnalogBoardMsg msg;

	if (size < sizeof(TAnalogBoardMsg))
		return -EINVAL;
	if (pdx->ring_head == pdx->ring_tail)
		return -ENOENT;

	memset(&msg, 0, sizeof(msg));

	switch (event->type) {
	case LINE_EVENT:
		msg.id = PK_DISPATCH_BOARD_LINE_MASK;
		msg.trunk = event->line;
		msg.mask = event->mask;
		break;
	case PLL_EVENT:
		msg.id = PK_DISPATCH_BOARD_PLL_MASK;
		msg.mask = event->mask;
		break;
	case DMA_EVENT:
		msg.id = PK_DISPATCH_BOARD_DMA_EVENT;
		break;
	case CLOCK_EVENT:
		msg.id = PK_DISPATCH_BOARD_CLOCK_EVENT;
		msg.mask = event->mask;
		break;
	case PFT_EVENT:
		msg.id = PK_DISPATCH_BOARD_PFT_EVENT;
		msg.trunk = event->line;
		break;
	case BUTTON_EVENT:
		msg.id = PK_DISPATCH_BOARD_BUTTON_EVENT;
		msg.trunk = event->line;
		break;
	default:
		PK_PRINT("daytona_get_event: BAD EVENT %d\n", event->type);
		if (++pdx->ring_tail >= LINE_RING_MAX)
			pdx->ring_tail = 0;
		return 0;
	}

	if (copy_to_user(buffer, &msg, sizeof(msg)))
		return -EFAULT;

	if (++pdx->ring_tail >= LINE_RING_MAX)
		pdx->ring_tail = 0;
	return sizeof(msg);
}

/* -------------------------------------------------------------- */
/* TRUNK STUFF */

/* NOTE: It is up to user mode to validate. We just mush things into
 * place. We will truncate or default values if necessary. All we
 * validate is that things won't crash. */

#define CHECK_TRUNK(pdx, t) do {					\
		if (((unsigned)(t)) >= ICCR_MAX_LINES)			\
			return -EINVAL;					\
		if ((pdx->trunk_mask & (1 << (unsigned)(t))) == 0)	\
			return -EINVAL;					\
	} while (0)


int daytona_set_threshold(PDEVICE_EXTENSION pdx, u32 trunk,
			  u32 threshold)
{
	CHECK_TRUNK(pdx, trunk);
	return write_silabs(pdx, trunk, 43, threshold);
}

int daytona_set_detect(PDEVICE_EXTENSION pdx, u32 trunk, u32 mask)
{
	int i, rc;
	unsigned imr;
	u8 detect;

	CHECK_TRUNK(pdx, trunk);

	/* We must enable these in the SiLabs */
	detect = 1 << 2 | 1 << 5; /* LOF and LCSO always on */
	if (mask & PKH_TRUNK_REVERSAL_DETECTION)
		detect |= 1 << 0;
	if (mask & PKH_TRUNK_DOD)
		detect |= 1 << 3;
	if (mask & PKH_TRUNK_RX_OVERLOAD)
		detect |= 1 << 6;
	if (mask & PKH_TRUNK_RING_DETECTION)
		detect |= 1 << 7;
	rc = write_silabs(pdx, trunk, 3, detect);
	if (rc)
		return rc;

	mask |= 3; /* We always enable HKFL and DIAL */
	pdx->line_imr[trunk] = mask;

	imr = BAR0_READ(pdx, BAR0_IMR);
	mask = imr & ~0xFDC;
	for (i = 0; i < ICCR_MAX_LINES; ++i)
		mask |= pdx->line_imr[i];
	if (mask != imr) {
		pdx->imr = mask & ~IMR_MASK;
		BAR0_WRITE(pdx, BAR0_IMR, mask);
		if (dbg)
			PK_PRINT("%d: IMR was %08x now %08x (%08x)\n",
				 pdx->cardid, imr, mask, pdx->imr);
	}

	return 0;
}

int daytona_trunk_set_config(PDEVICE_EXTENSION pdx, trunk_start_t *rqst)
{
	u32 data;
	u32 trunk = rqst->trunk;

	CHECK_TRUNK(pdx, trunk);

	/* The internationalControl is stored as regs 16, 26, 30, 31 */
	/*                                       N.A. 00  C0  00  A3 */
	data = rqst->internationalControl;
	write_silabs(pdx, trunk, 31, data & 0xff);
	data >>= 8;
	write_silabs(pdx, trunk, 30, data & 0xff);
	data >>= 8;
	write_silabs(pdx, trunk, 26, data & 0xff);
	data >>= 8;
	write_silabs(pdx, trunk, 16, data & 0xff);

	/* The 8kHz is set in HmpCard_configure */
	data = (rqst->compandMode & PKH_TRUNK_AUDIO_ALAW) ? 0x20 : 0x28;
	write_silabs(pdx, trunk, 33, data);

	return 0;
}

int daytona_trunk_start(PDEVICE_EXTENSION pdx, u32 trunk)
{
	u8 source;
	int rc;

	CHECK_TRUNK(pdx, trunk);

	/* Clear DOD and Reversal ints if necessary */
	rc = read_silabs(pdx, trunk, 4, &source);
	if (rc == 0 && (source & 9)) {
		source &= ~9;
		write_silabs(pdx, trunk, 4, source);
	}

	return rc;
}


int daytona_trunk_stop(PDEVICE_EXTENSION pdx, u32 trunk)
{
	CHECK_TRUNK(pdx, trunk);

	daytona_set_detect(pdx, trunk, 0);

	return 0;
}


int daytona_hook_switch(PDEVICE_EXTENSION pdx, trunk_hookswitch_t *rqst)
{
	u8 data;
	int trunk = rqst->trunk;

	CHECK_TRUNK(pdx, trunk);
	if (pdx->disabled_mask & (1 << trunk))
		return -EBUSY;

	switch (rqst->state) {
	case TRUNK_GO_ONHOOK:
		/* Make sure resistor auto-calibration is on before
		 * going onhook. */
		read_silabs(pdx, trunk, 25, &data);
		if (data & (1 << 5)) {
			data &= ~(1 << 5);
			write_silabs(pdx, trunk, 25, data);
		}
		/* We cannot use set_state here since it ors */
		pdx->line_state[trunk] = STATE_ONHOOK | STATE_HOOKING;
		write_silabs(pdx, trunk, 5, 8); /* onhook mon on */
		DELAY_SET(&pdx->trunk_delay[trunk], pdx->onhook_debounce);
		break;
	case TRUNK_GO_OFFHOOK:
		/* We cannot use set_state here since it ors */
		pdx->line_state[trunk] = STATE_OFFHOOK | STATE_HOOKING;
		write_silabs(pdx, trunk, 5, 1); /* onhook mon off */
		DELAY_SET(&pdx->trunk_delay[trunk], pdx->offhook_debounce);
		break;
	case TRUNK_GO_HOOKFLASH:
		set_state(pdx, trunk, STATE_HOOKFLASH);
		BAR0_WRITE(pdx, BAR0_HKFLSH_START, (1 << trunk));
		break;
	case TRUNK_STOP_HOOKFLASH:
		BAR0_WRITE(pdx, BAR0_HKFLSH_STOP, (1 << trunk));
		break;
	case TRUNK_STOP_DIALING:
		BAR0_WRITE(pdx, BAR0_STOP_DIAL, (1 << trunk));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}


int daytona_trunk_info(PDEVICE_EXTENSION pdx, u32 trunk,
		       PKH_TTrunkInfo *info)
{
	u8 data;

	CHECK_TRUNK(pdx, trunk);

	if (read_silabs(pdx, trunk, 28, &data))
		return -ETIMEDOUT;
	info->loopCurrent = data;

	if (read_silabs(pdx, trunk, 29, &data))
		return -ETIMEDOUT;
	info->lineVoltage = (signed char)data;

	return 0;
}


int daytona_trunk_dial(PDEVICE_EXTENSION pdx, trunk_dial_t *rqst)
{
	unsigned reg;
	int i;
	u8 data;

	CHECK_TRUNK(pdx, rqst->trunk);

	/* Write the digits - already converted. */
	reg = BAR0_DIAL_STR_ARRAY + rqst->trunk * 128;
	for (i = 0; i < 32; ++i, reg += 4)
		BAR0_WRITE(pdx, reg, rqst->string[i]);

	set_state(pdx, rqst->trunk, STATE_DIALING);

	/* Turn off resistor auto-calibration during pulse dialing. */
	read_silabs(pdx, rqst->trunk, 25, &data);
	data |= 1 << 5;
	write_silabs(pdx, rqst->trunk, 25, data);

	/* Start dialing... */
	BAR0_WRITE(pdx, BAR0_START_DIAL, (1 << rqst->trunk));

	return 0;
}

int daytona_trunk_set_audio(PDEVICE_EXTENSION pdx, trunk_audio_t *audio)
{
	write_silabs(pdx, audio->trunk, 15, audio->mute);
	write_silabs(pdx, audio->trunk, 38, audio->TXgain2);
	write_silabs(pdx, audio->trunk, 39, audio->RXgain2);
	write_silabs(pdx, audio->trunk, 40, audio->TXgain3);
	write_silabs(pdx, audio->trunk, 41, audio->RXgain3);
	return 0;
}

/* ------------------------------------------------------------ */
/* Phone stuff */

#define CHECK_PHONE(pdx, p) do {					\
		if (((unsigned)(p)) >= ICCR_MAX_LINES)			\
			return IOCTL_ERROR_INVALID_PARAM;		\
		if ((pdx->phone_mask & (1 << (unsigned)(p))) == 0)	\
			return IOCTL_ERROR_INVALID_PARAM;		\
	} while (0)


int daytona_phone_config(PDEVICE_EXTENSION pdx, u32 phone,
			 u32 intCtrl, u32 audioFormat)
{
	u8 data;

	CHECK_PHONE(pdx, phone);

	write_silabs(pdx, phone, LINE_IMPEDANCE, intCtrl);

	data = (audioFormat & PKH_PHONE_AUDIO_ALAW) ? 0x20 : 0x28;
	write_silabs(pdx, phone, PCM_MODE, data);

	/* No support for anything other than 8kHz now. */
	return 0;
}


int daytona_phone_start(PDEVICE_EXTENSION pdx, u32 phone)
{
	CHECK_PHONE(pdx, phone);

	pdx->line_state[phone] = 0;

	/* Set lc debounce in Silabs */
	write_silabs(pdx, phone, LOOP_DEBOUNCE, pdx->loop_debounce);

	/* Enable ints in the SiLabs. We enable them all and control them
	 * through the FPGA.
	 */
	write_silabs(pdx, phone, INTRPT_MASK2, 0xff);

	/* Enable RING TRIP/LC in the IMR */
	BAR0_WRITE(pdx, 0x124, 0);
	BAR0_WRITE(pdx, 0x120, 0);
	imr_set(pdx, 0x3000000);

	/* Enable ring trip detection */
	pdx->line_imr[phone] |= 0x1000000;

	/* Enable line */
	daytona_phone_reset(pdx, phone);

	/* When we set the LF to active forward, it takes 400us for
	 * the active forward to start and another 450us for it to
	 * stabilize. This delay gets us over the hump. This is mainly
	 * for arts and other loopback testing. */
	delay(2);

	return 0;
}


int daytona_phone_stop(PDEVICE_EXTENSION pdx, u32 phone)
{
	CHECK_PHONE(pdx, phone);

	daytona_phone_reset(pdx, phone);

	write_silabs(pdx, phone, 64, 0); /* disable */

	return 0;
}


int daytona_set_ring_pattern(PDEVICE_EXTENSION pdx, phone_ring_pattern_t *rqst)
{
	/* BLAW - FIX ME!  Reconcile with usermode. (BYTES_PER_CADENCE); */
	if (rqst->index >= PKH_BOARD_MAX_RING_PATTERNS)
		return IOCTL_ERROR_INVALID_PARAM;

	if (rqst->size > BYTES_PER_CADENCE)
		return IOCTL_ERROR_INVALID_PARAM;

	memcpy(pdx->ring_patterns[rqst->index], rqst->pattern, rqst->size);
	return 0;
}


int daytona_phone_info(PDEVICE_EXTENSION pdx, int phone,
		       phone_get_info_out_t *info)
{
	u8 data;

	CHECK_PHONE(pdx, phone);

	if (read_silabs(pdx, phone, LOOP_V_SENSE, &data))
		return -ETIMEDOUT;
	info->voltage = data;
	if (read_silabs(pdx, phone, LOOP_I_SENSE, &data))
		return -ETIMEDOUT;
	info->lineCurrent = data;

	return 0;
}


/* ------------------------------------------------------------ */
/* Clock stuff */

#ifdef CONFIG_WARP
void daytona_set_clock(PDEVICE_EXTENSION pdx, int clock_mode) {}
#else
static void clock_set_master(PDEVICE_EXTENSION pdx)
{
	unsigned val;
	int count;

	if (debug_clocking)
		PK_PRINT("%d/%d: Set clock to master\n", pdx->id, pdx->cardid);

	pdx->clock_mode = CLOCK_MASTER;

	/* Disable PLL ints */
	imr_clr(pdx, 0x100000);
	/* Set PLL to freerun */
	val = BAR0_READ(pdx, BAR0_PLL_CONTROL) | 1;
	BAR0_WRITE(pdx, BAR0_PLL_CONTROL, val);

	/* Wait for PLL to actually be in freerun for upto 1ms */
	for (count = 0; count < 1000; ++count) {
		if (BAR0_READ(pdx, BAR0_STAT_LINES) & 0x20000)
			break;
		udelay(1);
	}

	/* Hold the recovery in reset. */
	val = BAR0_READ(pdx, BAR0_RESET) | 0x10;
	BAR0_WRITE(pdx, BAR0_RESET, val);
}


static void clock_set_slave(PDEVICE_EXTENSION pdx)
{
	int count;
	unsigned val;

	if (debug_clocking)
		PK_PRINT("%d/%d: Set clock to slave\n", pdx->id, pdx->cardid);

	pdx->clock_mode = CLOCK_SLAVE;

	/* Set PLL to Fast Lock & Free Run */
	BAR0_WRITE(pdx, BAR0_PLL_CONTROL, 0x9);

	/* Wait for PLL to actually be in freerun for upto 1ms */
	for (count = 0; count < 1000; ++count) {
		if (BAR0_READ(pdx, BAR0_STAT_LINES) & 0x20000)
			break;
		udelay(1);
	}

	/* Take the clock recovery out of reset. */
	val = BAR0_READ(pdx, BAR0_RESET);
	/* make sure clock recovery logic actually gets reset. */
	BAR0_WRITE(pdx, BAR0_RESET, val | 0x10);
	BAR0_WRITE(pdx, BAR0_RESET, val & ~0x10);

	/* remove pll from freerun */
	BAR0_WRITE(pdx, BAR0_PLL_CONTROL, 0x8);
	/* Enable PLL ints */
	imr_set(pdx, 0x100000);
}


void daytona_set_clock(PDEVICE_EXTENSION pdx, int clock_mode)
{
	if (clock_mode == CLOCK_MASTER) {
		if (pdx->clock_mode == CLOCK_MASTER)
			return;

		clock_set_master(pdx);

		clear_bit(pdx->cardid, &board_clock_state);
		set_bit(pdx->cardid, &board_clock_changed);
	} else { /* Sync for all slaves.*/
		if (pdx->clock_mode == CLOCK_MASTER) {
			set_bit(pdx->cardid, &board_clock_state);
			set_bit(pdx->cardid, &board_clock_changed);
			pdx->clock_mode = CLOCK_SET_SLAVE;
		}
	}
}


/* Called from HSP when clock master changes */
void daytona_clock_changed(int master)
{
	PDEVICE_EXTENSION pdx;

	list_for_each_entry(pdx, &board_list, list)
		if (pdx->card_state == STATE_ATTACHED)
			daytona_set_clock(pdx, pdx->cardid == master ?
					  CLOCK_MASTER : CLOCK_SLAVE);
		else
			daytona_set_clock(pdx, CLOCK_MASTER);
}

/* Called from HSP for every clock tick */
void daytona_clock_tick(void)
{
	PDEVICE_EXTENSION pdx;

	list_for_each_entry(pdx, &board_list, list) {
		switch (pdx->clock_mode) {
		case CLOCK_SET_SLAVE:
			clock_set_slave(pdx);
			break;
		case CLOCK_SLAVE:
			BAR0_WRITE(pdx, BAR0_SYS_TICK, 0xFEED);
			break;
		}
	}
}

/* Called from HSP every time the clock recovery logic needs to be reset. */
void daytona_clock_reset(void)
{
	PDEVICE_EXTENSION pdx;

	list_for_each_entry(pdx, &board_list, list) {
		if (pdx->clock_mode == CLOCK_SLAVE) {
			unsigned val = BAR0_READ(pdx, BAR0_RESET);

			/* reset clock recovery module */
			BAR0_WRITE(pdx, BAR0_RESET, val | 0x10);
			udelay(1);
			/* remove clock recovery module reset */
			BAR0_WRITE(pdx, BAR0_RESET, val & ~0x10);
		}
	}
}

#endif

EXPORT_SYMBOL(imr_set);
EXPORT_SYMBOL(imr_clr);
EXPORT_SYMBOL(daytona_silabs_reset);
EXPORT_SYMBOL(daytona_configure_fxo);
EXPORT_SYMBOL(add_event);
EXPORT_SYMBOL(do_trunks);
EXPORT_SYMBOL(do_phones);
EXPORT_SYMBOL(daytona_set_detect);
EXPORT_SYMBOL(daytona_trunk_set_config);
EXPORT_SYMBOL(daytona_trunk_start);
EXPORT_SYMBOL(daytona_trunk_stop);
EXPORT_SYMBOL(daytona_hook_switch);
EXPORT_SYMBOL(daytona_trunk_info);
EXPORT_SYMBOL(daytona_phone_config);
EXPORT_SYMBOL(daytona_phone_start);
EXPORT_SYMBOL(daytona_phone_stop);


/*
 * Force kernel coding standard on PIKA code
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
