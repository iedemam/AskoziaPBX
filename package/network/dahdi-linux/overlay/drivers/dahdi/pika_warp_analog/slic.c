#include "driver.h"
#include "si3215_registers.h"


#define CHECK_PHONE(pdx, p) do {					\
		if (((unsigned)(p)) >= ICCR_MAX_LINES)			\
			return -EINVAL;					\
		if ((pdx->phone_mask & (1 << (unsigned)(p))) == 0)	\
			return -EINVAL;					\
	} while (0)

#define IS_PHONE(pdx, p) (pdx->phone_mask & (1 << (unsigned)(p)))

static int ValidateSilabsData(PDEVICE_EXTENSION pdx, int channel,
			      int reg, int val)
{
	u8 data;

	if (read_silabs(pdx, channel, reg, &data)) {
		PK_PRINT("FXS %d: Unable to read reg %x\n", channel, reg);
		return 0;
	}

	if (data != val) {
		PK_PRINT("%d: FXS %d Reg %x value %x != %x expected.\n",
			 pdx->id, channel, reg, data, val);
		return 0;
	}

	return 1;
}

static int ValidateLineState(PDEVICE_EXTENSION pdx, int channel)
{
	return ValidateSilabsData(pdx, channel,
				  LINE_STATE, LINE_STATE_DEFAULT_VAL);
}

/* All writes to the LINE_STATE should go through this function. */
static inline void set_line_state(PDEVICE_EXTENSION pdx,
				     int line, unsigned state)
{
	write_silabs(pdx, line, LINE_STATE, state);
#undef LINE_STATE
}

void daytona_pll_fxs(PDEVICE_EXTENSION pdx)
{
	/* Set reset to known good default state */
	BAR0_WRITE(pdx, BAR0_RESET, 0x39);

	mdelay(1);

	/* Take PLL out of reset */
	BAR0_WRITE(pdx, BAR0_RESET, 0x3B);

	delay(20); /* wait for clocks to stabilize */

	if (debug_clocking)
		PK_PRINT("%d: daytona_pll_fxs\n", pdx->id);

	daytona_silabs_reset(pdx);
}

#if 0
#define PHONE_MASK_SIZE PKH_BOARD_MAX_NUMBER_OF_PHONES
#define phone_for_each(i)						\
	for (i = find_first_bit(&pdx->phone_mask, PHONE_MASK_SIZE);	\
	     i < PHONE_MASK_SIZE;					\
	     i = find_next_bit(&pdx->phone_mask, PHONE_MASK_SIZE, i + 1))
#endif

static inline void disable_phone(PDEVICE_EXTENSION pdx, int chan)
{
	pdx->phone_mask &= ~(1 << chan);
}


int daytona_configure_fxs(PDEVICE_EXTENSION pdx, unsigned mask)
{
	u8	data;
	int	phone, val, i;

	/* BLAW - delay before checking default values for certain
	 * registers
	 */
	delay(8);

	/* phone_for_each needs this set. Reset every time to test for
	 * disabled phones. */
	pdx->phone_mask = mask;

	/* Verifing communications with Silabs parts. */
	phone_for_each(phone) {
		int failed = 0;

		/* Verify default value of Silabs register HYBRID */
		if (!ValidateSilabsData(pdx, phone, HYBRID,
					HYBRID_DEFAULT_VAL))
			++failed;

		/* Verify default value of Silabs register LINE_STATE */
		if (!ValidateLineState(pdx, phone))
			++failed;

		/* Read Silabs device revision */
		read_silabs(pdx, phone, SPI_MODE, &data);
		if (data == 0xff || data < 0x2) {
			PK_PRINT("Revision of phone %d Si3215 is %X.\n",
				  phone, (data & 0x0F));
			++failed;
		}

		if (failed)
			disable_phone(pdx, phone);
	}

	/* Put initial values into indirect registers */
	phone_for_each(phone) {
		WriteSilabsIndirect(pdx, phone, OSC1_COEF, 0x0000);
		WriteSilabsIndirect(pdx, phone, OSC1X, 0x0000);
		WriteSilabsIndirect(pdx, phone, OSC1Y, 0x0000);
		WriteSilabsIndirect(pdx, phone, OSC2_COEF, 0x0000);
		WriteSilabsIndirect(pdx, phone, OSC2X, 0x0000);
		WriteSilabsIndirect(pdx, phone, OSC2Y, 0x0000);
		WriteSilabsIndirect(pdx, phone, RING_V_OFF, 0x0000);
		WriteSilabsIndirect(pdx, phone, RING_OSC_COEF, 0x7EFD);
		WriteSilabsIndirect(pdx, phone, RING_X, 0x0152);
		WriteSilabsIndirect(pdx, phone, RING_Y, 0x0000);

		/* unity digital gain = 0x4000 */
		WriteSilabsIndirect(pdx, phone, RECV_DIGITAL_GAIN, 0x4000);
		WriteSilabsIndirect(pdx, phone, XMIT_DIGITAL_GAIN, 0x4000);

		WriteSilabsIndirect(pdx, phone, LOOP_CLOSE_THRESH, 0x1000);
		WriteSilabsIndirect(pdx, phone, RING_TRIP_THRESH, 0x3600);
		WriteSilabsIndirect(pdx, phone, COMMON_MIN_THRESH, 0x1000);
		WriteSilabsIndirect(pdx, phone, COMMON_MAX_THRESH, 0x0080);
		WriteSilabsIndirect(pdx, phone, PWR_ALARM_Q1Q2, 0x07C0);
		WriteSilabsIndirect(pdx, phone, PWR_ALARM_Q3Q4, 0x2600);
		WriteSilabsIndirect(pdx, phone, PWR_ALARM_Q5Q6, 0x1B80);
		WriteSilabsIndirect(pdx, phone, CM_BIAS_RINGING, 0x0000);
		WriteSilabsIndirect(pdx, phone, DCDC_MIN_V, 0x0600);
		WriteSilabsIndirect(pdx, phone, LOOP_CLOSE_THRESH_LOW, 0x0C00);

		/* set to 0x8000, will get set to final value later */
		WriteSilabsIndirect(pdx, phone, LOOP_CLOSURE_FILTER, 0x8000);
		WriteSilabsIndirect(pdx, phone, RING_TRIP_FILTER,    0x8000);
		WriteSilabsIndirect(pdx, phone, THERM_LP_POLE_Q1Q2,  0x8000);
		WriteSilabsIndirect(pdx, phone, THERM_LP_POLE_Q3Q4,  0x8000);
		WriteSilabsIndirect(pdx, phone, THERM_LP_POLE_Q5Q6,  0x8000);
	}

	/* Take ProSlic out of loopback mode */
	phone_for_each(phone)
		write_silabs(pdx, phone, AUDIO_LOOPBACK , 0x00);

	/* Enable enhanced features */
	phone_for_each(phone)
		write_silabs(pdx, phone, ENHANCE , 0xA3);

	/* Set Vbat high value (from power consumption spreadsheet) */
	phone_for_each(phone)
		write_silabs(pdx, phone, BAT_V_HI , 0x2E);

	/* Set Vbat low value */
	phone_for_each(phone)
		write_silabs(pdx, phone, BAT_V_LO , 0x10);

	/* *********************************
	 * Perform DC to DC calibration
	 * *********************************/
	/* Set value for register 92 (from power consumption
	 * spreadsheet) so the DC-DC converter will operate at
	 * 73kHz. */
	phone_for_each(phone)
		write_silabs(pdx, phone, DCDC_PWM_OFF , 0xE1);

	/* Set value for register 93 (from power consumption spreadsheet) */
	phone_for_each(phone)
		write_silabs(pdx, phone, DCDC , 0x17);

	/* Turn on DC-DC converters */
	phone_for_each(phone)
		write_silabs(pdx, phone, PWR_DOWN1 , 0x00);

	/* Let battery voltage settle */
	delay(25);

	/* Read battery voltage. Expect 65.8V to 70V. */
	/* Note: It seems to take much longer from a cold start. */
	phone_for_each(phone) {
		for (i = 0; i < 100; ++i) {
			read_silabs(pdx, phone, BAT_V_HI_SENSE, &data);
			if (data >= 175 && data <= 186)
				break;
			delay(1);
		}
		if (data < 175) {
			PK_PRINT("%d: Battery voltage out of range %d\n",
				 phone, data);
			if (data < 150)
				disable_phone(pdx, phone);
		}
	}

	/* Start DC-DC converter calibration */
	phone_for_each(phone)
		write_silabs(pdx, phone, DCDC, 0x94);

	/* Read if calibration is complete */
	phone_for_each(phone) {
		i = 0;
		do {
			read_silabs(pdx, phone, DCDC, &data);
			delay(1);
			if (++i > 10) { /* Usually 1 */
				PK_PRINT("%d: DC to DC calibration failed.\n",
					 phone);
				disable_phone(pdx, phone);
				break;
			}
		} while (data & 0x80);
		if (i > 5) /* SAM DBG */
			PK_PRINT("%d: Warning DC to DC %d\n", phone, i);
	}

	/* ***************************************
	 * End of    Perform DC to DC calibration
	 * ***************************************/


	/* Set state to open */
	phone_for_each(phone)
		set_line_state(pdx, phone, 0);

	/* You can only calibrate once per reset or you get spurious
	 * power alarms. This is only a problem on warped boards.
	 */
	if (!pdx->calibrated) {
		/* Start ProSlic calibration */
		phone_for_each(phone) {
			write_silabs(pdx, phone, CALIBR2, 0x1E);
			write_silabs(pdx, phone, CALIBR1, 0x47);
		}

		/* Check if calibration complete */
		delay(350); /* Usually takes 355 to 357ms */
		phone_for_each(phone) {
			i = 0;
			do {
				read_silabs(pdx, phone, CALIBR1, &data);
				delay(1);
				if (++i > 1000) {
					PK_PRINT("%d: Calibration failed.\n",
						 phone);
					disable_phone(pdx, phone);
					break;
				}
			} while (data & 0x47);
			if (i > 50) /* SAM DBG */
				PK_PRINT("%d: Warning calibrate %d\n",
					 phone, i);
		}

		pdx->calibrated++;
	}

	/* *********************************************
	 * Perform longitudinal balance calibration
	 * *********************************************/
	phone_for_each(phone)
		/* enable undocumented common-mode calibration error
		 * interrupt
		 */
		write_silabs(pdx, phone, INTRPT_MASK3, 0x04);

	phone_for_each(phone) {
		read_silabs(pdx, phone, LOOP_STAT, &data);
		if (data & 1) {
			PK_PRINT("%d: Channel is not on-hook.\n", phone);
			disable_phone(pdx, phone);
		} else {
			write_silabs(pdx, phone, CALIBR2, 0x01);
			write_silabs(pdx, phone, CALIBR1, 0x40);
		}
	}

	phone_for_each(phone) {
		i = 0;
		do {
			read_silabs(pdx, phone, CALIBR1, &data);
			delay(1);
			if (++i > 10) { /* Usually takes 1 */
				PK_PRINT("%d: Final calibration failed.\n",
					 phone);
				disable_phone(pdx, phone);
				break;
			}
		} while (data & 0x40); /* bits 6 0 if complete */
		if (i > 5) /* SAM DBG */
			PK_PRINT("%d: Warning complete %d\n", phone, i);
	}

	/* Check if any errors occurred */
	phone_for_each(phone) {
		read_silabs(pdx, phone, INTRPT_STATUS3, &data);
		if (data & 0x04) {
			PK_PRINT("%d: Longitudinal balance calibration "
				 "error occurred.\n", phone);
			disable_phone(pdx, phone);
		}
	}

	/* *************************************************
	 * END OF   Perform longitudinal balance calibration
	 * *************************************************/

	/* *********************************************
	 * Perform manual calibration
	 * *********************************************/

	if (!pdx->manual_calibration_done) {
		int ring_gain_cmask = 0;
		int tip_gain_cmask = 0;

		PK_PRINT("Performing Calibration. Please wait.\n");

		/* Set initial value of Tip calibration register */
		/* while Ring gets calibrated */
		phone_for_each(phone) {
			write_silabs(pdx, phone, TIP_GAIN_CAL, 0x10);
			pdx->ring_gain_value[phone] = 0xFF;
		}

		phone_for_each(phone) {
			write_silabs(pdx, phone, RING_GAIN_CAL, 0x10);
			pdx->tip_gain_value[phone] = 0xFF;
		}

		/* perform ring gain calibration in parallel */
		for (i = 0x1F; i >= 0; i--) {

			/* go through all uncalibrated lines */
			phone_for_each(phone) {
				if (!(ring_gain_cmask & (1 << phone)))
					write_silabs(pdx,
							phone,
							RING_GAIN_CAL,
							i);
			}

			/* wait for 40 milliseconds for all phones */
			mdelay(40);

			/* go through all calibration results */
			phone_for_each(phone) {
				if (!(ring_gain_cmask & (1 << phone))) {
					read_silabs(pdx, phone, IQ5, &data);
					if (data == 0) {
						pdx->ring_gain_value[phone] = i;
						ring_gain_cmask |= (1 << phone);
					}
				}
			}
		}

		/* perform tip gain calibration in parallel */
		for (i = 0x1F; i >= 0 ; i--) {

			/* go through all uncalibrated lines */
			phone_for_each(phone) {
				if (!(tip_gain_cmask & (1 << phone)))
					write_silabs(pdx,
							phone,
							TIP_GAIN_CAL,
							i);
			}

			/* wait for 40 milliseconds for all phones */
			mdelay(40);

			/* go through all calibration results */
			phone_for_each(phone) {
				if (!(tip_gain_cmask & (1 << phone))) {
					read_silabs(pdx, phone, IQ6, &data);
					if (data == 0) {
						pdx->tip_gain_value[phone] = i;
						tip_gain_cmask |= (1 << phone);
					}
				}
			}
		}

		/* mark it as done */
		pdx->manual_calibration_done = 1;
		PK_PRINT("Calibration Complete.\n");
	}

	/* put all calibration values in place */
	phone_for_each(phone) {
		if (pdx->ring_gain_value[phone] != 0xFF) {
			write_silabs(pdx,
					phone,
					RING_GAIN_CAL,
					pdx->ring_gain_value[phone]);
		} else {
			PK_PRINT("ERROR calibrating RING GAIN for chan %d\n",
				phone);
		}
		if (pdx->tip_gain_value[phone] != 0xFF) {
			write_silabs(pdx,
					phone,
					TIP_GAIN_CAL,
					pdx->tip_gain_value[phone]);
		} else {
			PK_PRINT("ERROR calibrating TIP GAIN for chan %d\n",
				phone);
		}
	}


	/* *************************************************
	 * END OF   Perform manual calibration
	 * *************************************************/


	/* Flush out energy accumulators
	 * (not documented in data sheet, only in AN35)
	 */
	phone_for_each(phone) {
		WriteSilabsIndirect(pdx, phone, 75, 0x0000);
		WriteSilabsIndirect(pdx, phone, 76, 0x0000);
		WriteSilabsIndirect(pdx, phone, 77, 0x0000);
		WriteSilabsIndirect(pdx, phone, 78, 0x0000);
		WriteSilabsIndirect(pdx, phone, 79, 0x0000);
		WriteSilabsIndirect(pdx, phone, 80, 0x0000);
		WriteSilabsIndirect(pdx, phone, 81, 0x0000);
		WriteSilabsIndirect(pdx, phone, 82, 0x0000);
		WriteSilabsIndirect(pdx, phone, 84, 0x0000);
		WriteSilabsIndirect(pdx, phone, 208, 0x0000);
		WriteSilabsIndirect(pdx, phone, 209, 0x0000);
		WriteSilabsIndirect(pdx, phone, 210, 0x0000);
		WriteSilabsIndirect(pdx, phone, 211, 0x0000);
	}

	/* Assign PCM timeslots to phones */
	phone_for_each(phone) {
		val = (phone << 4) | 1;
		write_silabs(pdx, phone, PCM_XMIT_START_COUNT_LSB, val & 0xff);
		write_silabs(pdx, phone, PCM_XMIT_START_COUNT_MSB, val >> 8);
		write_silabs(pdx, phone, PCM_RCV_START_COUNT_LSB, val & 0xff);
		write_silabs(pdx, phone, PCM_RCV_START_COUNT_MSB, val >> 8);
	}

	/* Enable 8-bit u-law pcm */
	phone_for_each(phone)
		write_silabs(pdx, phone, PCM_MODE, 0x28);

	/* Configure voice path */
	phone_for_each(phone) {
		write_silabs(pdx, phone, AUDIO_LOOPBACK, 0x00);
		/* default to -3.5dB in each direction. */
		write_silabs(pdx, phone, AUDIO_GAIN, 0x05);
		write_silabs(pdx, phone, LINE_IMPEDANCE, 0x08);
		write_silabs(pdx, phone, HYBRID, HYBRID_DEFAULT_VAL);
	}

	/* Configure ringing oscillator */
	phone_for_each(phone)
		write_silabs(pdx, phone, RING_OSC_CTL, 0);

	/* Set external transistor bias levels */
	phone_for_each(phone)
		write_silabs(pdx, phone, BIAS_SQUELCH, 0x61);

	/* Set loop current limit to 20mA */
	phone_for_each(phone) {
		write_silabs(pdx, phone, LOOP_I_LIMIT, 0x00);
		write_silabs(pdx, phone, LOOP_DEBOUNCE, 120);
	}

	/* Set final filter coefficients */
	phone_for_each(phone) {
		WriteSilabsIndirect(pdx, phone, LOOP_CLOSURE_FILTER, 0x8000);
		WriteSilabsIndirect(pdx, phone, RING_TRIP_FILTER, 0x0320);
		WriteSilabsIndirect(pdx, phone, THERM_LP_POLE_Q1Q2, 0x008C);
		WriteSilabsIndirect(pdx, phone, THERM_LP_POLE_Q3Q4, 0x0100);
		WriteSilabsIndirect(pdx, phone, THERM_LP_POLE_Q5Q6, 0x0010);
	}

	/* Clear FSK coefficients */
	phone_for_each(phone) {
		WriteSilabsIndirect(pdx, phone, FSK_X_0, 0x0000);
		WriteSilabsIndirect(pdx, phone, FSK_COEFF_0, 0x0000);
		WriteSilabsIndirect(pdx, phone, FSK_X_1, 0x0000);
		WriteSilabsIndirect(pdx, phone, FSK_COEFF_1, 0x0000);
		WriteSilabsIndirect(pdx, phone, FSK_X_01, 0x0000);
		WriteSilabsIndirect(pdx, phone, FSK_X_10, 0x0000);
	}

	/* HACK We need to hit each linefeed control with a forward
	 * onhook transmission (2) or we cannot ring the phone until
	 * we go offhook once.
	 *
	 * On the taco we get power alarms if we don't do this.
	 */
	phone_for_each(phone) {
		set_line_state(pdx, phone, 2);
		set_line_state(pdx, phone, 0);
	}

	/* Enable Ring Trip, Loop Closure Transition, and Power alarms
	 * in the IMR
	 */
	imr_set(pdx, 0x7000000);

	/* Clear the ring generators */
	memset(pdx->ring_cadences, 0, sizeof(pdx->ring_cadences));

	/* Set initial clock mode */
	if (pdx->cardid != -1) {
		if (get_master_clock(pdx) != -1) {
			if (pdx->cardid == get_master_clock(pdx))
				daytona_set_clock(pdx, CLOCK_MASTER);
			else
				daytona_set_clock(pdx, CLOCK_SLAVE);
		}
	}

	/* Set the board info fields after disabling all bad lines. */
	pdx->info.phoneMask = pdx->phone_mask;
	pdx->info.numberOfPhones = 0;
	phone_for_each(phone)
		++pdx->info.numberOfPhones;

	/* If we have at least one phone start the ring ticker */
	if (pdx->phone_mask)
		DELAY_SET(&pdx->ring_ticker, 100);

	return 0;
}

EXPORT_SYMBOL(daytona_configure_fxs);


static inline void next_rcad(PDEVICE_EXTENSION pdx,
				struct ring_cadence **rcad)
{
	if (*rcad == &pdx->ring_cadences[BYTES_PER_CADENCE - 1])
		*rcad = pdx->ring_cadences;
	else
		(*rcad)++;
}

int daytona_phone_ringgen_start(PDEVICE_EXTENSION pdx, phone_ring_t *rqst)
{
	struct ring_cadence *rcad;
	unsigned char  *rpat;
	int i, j;
	int cad;
	int ring_test_time_per;
	int phone = rqst->phone;
	int ring_pattern = rqst->pat_index;
	unsigned long flags;

	CHECK_PHONE(pdx, phone);
	if (pdx->disabled_mask & (1 << phone))
		return -EBUSY;

	if (ring_pattern >= PKH_BOARD_MAX_RING_PATTERN_SIZE)
		return IOCTL_ERROR_INVALID_PARAM;

	spin_lock_irqsave(&pdx->ringgen_lock, flags);
	if ((pdx->line_state[phone] & STATE_BIT_OFFHOOK) ||
	    (pdx->line_state[phone] & STATE_BIT_RINGGEN_ENABLED)) {
		spin_unlock_irqrestore(&pdx->ringgen_lock, flags);
		return IOCTL_ERROR_INVALID_LINE_STATE;
	}
	spin_unlock_irqrestore(&pdx->ringgen_lock, flags);

	/* using the cadence ID..see if we can fit in somewhere we
	 * need to start at the next possible start slot
	 */

	cad = pdx->ring_slot;
	if (++cad >= BYTES_PER_CADENCE)
		cad = 0;

	ring_test_time_per = pdx->max_ring_latency / TIC_PERIOD;

	rcad = &pdx->ring_cadences[cad];
	for (i = 0; i < ring_test_time_per; ++i) {
		/* try all patterns to make sure we do not go over the limit */
		rpat = pdx->ring_patterns[ring_pattern];
		for (j = 0; j < BYTES_PER_CADENCE; j++) {
			if (*(rpat++) &&  /* this will require more load */
			    rcad->load >= pdx->max_ring_load) {
				/* this will not work out.. we need to
				 * move to the next choice in the
				 * cadence table
				 */
				break;
			}

			next_rcad(pdx, &rcad);
		}

		if (j >= BYTES_PER_CADENCE) {
			/* we found a fit starting at rcad */
			spin_lock_irqsave(&pdx->ringgen_lock, flags);
			rpat = pdx->ring_patterns[ring_pattern];
			for (j = 0; j < BYTES_PER_CADENCE; ++j) {
				rcad->cur_brush[phone] = *rpat;
				if (*rpat++)
					(rcad->load)++;

				next_rcad(pdx, &rcad);
			}

			DELAY_SET(&pdx->ring_timeout[phone],
				  msecs_to_tics(rqst->duration));

			pdx->line_state[phone] |= STATE_BIT_RINGGEN_ENABLED;
			/* paranoia - start with ringgen on but not ringing */
			pdx->line_state[phone] &= ~STATE_BIT_RINGING;

			spin_unlock_irqrestore(&pdx->ringgen_lock, flags);

			return 0;
		}

		/* move to the next possible slot */
		if (++cad >= BYTES_PER_CADENCE)
			cad = 0;
		rcad = &pdx->ring_cadences[cad];
	}

	if (dbg)
		PK_PRINT("Board %d RINGGEN BUSY\n", pdx->id);

	return IOCTL_ERROR_RINGGEN_BUSY;
}


/* WARNING: must be called within ringgen spinlock Actually stops
 * ringing and restores lf.  You must set line_state depending on why
 * we're stopping.
 */
static void daytona_stop_ringing(PDEVICE_EXTENSION pdx, u32 phone)
{
	struct ring_cadence *rcad;
	int i;

	set_line_state(pdx, phone, pdx->lf[phone]);

	rcad = pdx->ring_cadences;

	for (i = 0; i < BYTES_PER_CADENCE; ++i, ++rcad)
		if (rcad->cur_brush[phone]) {
			if (rcad->load)
				--rcad->load;
			rcad->cur_brush[phone] = 0;
		}
}

/*
 *  Aborts ringing from ioctl and sets back to happy line feed state
 */
int daytona_phone_ringgen_stop(PDEVICE_EXTENSION pdx, u32 phone)
{
	unsigned long flags;

	CHECK_PHONE(pdx, phone);

	spin_lock_irqsave(&pdx->ringgen_lock, flags);

	if (!(pdx->line_state[phone] & STATE_BIT_RINGGEN_ENABLED)) {
		spin_unlock_irqrestore(&pdx->ringgen_lock, flags);
		return IOCTL_ERROR_INVALID_LINE_STATE;
	}

	/* stops and sets back to previous lf. */
	daytona_stop_ringing(pdx, phone);

	pdx->line_state[phone] &= ~STATE_BIT_RINGGEN_ENABLED;
	if (pdx->line_state[phone] & STATE_BIT_RINGING) {
		pdx->line_state[phone] &= ~STATE_BIT_RINGING;
		add_event(pdx, LINE_EVENT, phone, PHONE_EVENT_RING_OFF);
	}

	spin_unlock_irqrestore(&pdx->ringgen_lock, flags);
	return 0;
}

/* to be called only after you have acquired the ringgen_lock */
static inline void do_phone_reset_nolock(PDEVICE_EXTENSION pdx, int phone)
{
	daytona_stop_ringing(pdx, phone);

	/* Set it back on forward as default */
	daytona_phone_set_lf(pdx, phone, LINE_STATE_FORWARD_ACTIVE);
	set_line_state(pdx, phone, pdx->lf[phone]);
}

static inline void do_phone_reset(PDEVICE_EXTENSION pdx, int phone)
{
	unsigned long flags;

	spin_lock_irqsave(&pdx->ringgen_lock, flags);

	do_phone_reset_nolock(pdx, phone);

	spin_unlock_irqrestore(&pdx->ringgen_lock, flags);
}

int daytona_phone_reset(PDEVICE_EXTENSION pdx, u32 phone)
{
	CHECK_PHONE(pdx, phone);

	do_phone_reset(pdx, phone);

	pdx->line_state[phone] = 0; /* onhook, !ringgen */

	return 0;
}

// internal
void daytona_phone_update_linestate(PDEVICE_EXTENSION pdx, int phone, int linefeed)
{
	/* update the ringing line state */
	if (linefeed == LINE_STATE_RINGING) 
		pdx->line_state[phone] |= (STATE_BIT_RINGING | STATE_BIT_RINGGEN_ENABLED);
	else
		pdx->line_state[phone] &= ~(STATE_BIT_RINGING | STATE_BIT_RINGGEN_ENABLED);

	/* update the onhook tx state */
	if ((linefeed == LINE_STATE_FD_ONHOOK_TX) || \
		(linefeed == LINE_STATE_RV_ONHOOK_TX))
		pdx->line_state[phone] |= STATE_BIT_ONHOOK_TX_ENABLED;
	else
		pdx->line_state[phone] &= ~STATE_BIT_ONHOOK_TX_ENABLED;

	/* update the disconnected state */
	if (linefeed == LINE_STATE_OPEN)
		pdx->line_state[phone] |= STATE_BIT_DISCONNECTED;
	else 
		pdx->line_state[phone] &= ~STATE_BIT_DISCONNECTED;

	/* update the forward reverse onhook state(s) */
	if ((linefeed == LINE_STATE_FD_ONHOOK_TX) || \
		(linefeed == LINE_STATE_RV_ONHOOK_TX))
		pdx->line_state[phone] |= STATE_BIT_ONHOOK_TX_ENABLED;
	else
		pdx->line_state[phone] &= ~STATE_BIT_ONHOOK_TX_ENABLED;


}

/* sets a given phone line to a given linefeed */
int daytona_phone_set_linefeed(PDEVICE_EXTENSION pdx, int phone, int linefeed)
{
        unsigned long flags;

        spin_lock_irqsave(&pdx->ringgen_lock, flags);
	
        daytona_phone_set_lf(pdx, phone, linefeed);
	daytona_phone_update_linestate(pdx, phone, linefeed);
        set_line_state(pdx, phone, pdx->lf[phone]);

        spin_unlock_irqrestore(&pdx->ringgen_lock, flags);

        return 0;
}

EXPORT_SYMBOL(daytona_phone_set_linefeed);


/* WARNING: Can be called from bh.
 * Use to to set lf.  Does a bit of logic sometimes.
 * Doesn't write to register! (You have to do that)
 */
int daytona_phone_set_lf(PDEVICE_EXTENSION pdx, int phone, int direction)
{
	CHECK_PHONE(pdx, phone);

	switch (direction) {
	/* absolute line states -> just wack! */
	case LINE_STATE_OPEN:
	case LINE_STATE_FORWARD_ACTIVE:
	case LINE_STATE_FD_ONHOOK_TX:
	case LINE_STATE_TIP_OPEN:
	case LINE_STATE_RINGING:
	case LINE_STATE_REVERSE_ACTIVE:
	case LINE_STATE_RV_ONHOOK_TX:
	case LINE_STATE_RING_OPEN:
		pdx->lf[phone] = direction;
		return 0;
	}

	/* Logic involved. */
	switch (direction) {
	case LF_FORWARD: /* keep onhook tx if on. */
		if (pdx->lf[phone] == LINE_STATE_FD_ONHOOK_TX ||
		    pdx->lf[phone] == LINE_STATE_RV_ONHOOK_TX)
			pdx->lf[phone] = LINE_STATE_FD_ONHOOK_TX;
		else
			pdx->lf[phone] = LINE_STATE_FORWARD_ACTIVE;
		break;

	case LF_REVERSE: /* keep onhook tx if on */
		if (pdx->lf[phone] == LINE_STATE_FD_ONHOOK_TX ||
		    pdx->lf[phone] == LINE_STATE_RV_ONHOOK_TX)
			pdx->lf[phone] = LINE_STATE_RV_ONHOOK_TX;
		else
			pdx->lf[phone] = LINE_STATE_REVERSE_ACTIVE;
		break;

	case LF_FLIP:
		switch (pdx->lf[phone]) {
		case LINE_STATE_FD_ONHOOK_TX:
			pdx->lf[phone] = LINE_STATE_RV_ONHOOK_TX;
			break;
		case LINE_STATE_FORWARD_ACTIVE:
			pdx->lf[phone] = LINE_STATE_REVERSE_ACTIVE;
			break;
		case LINE_STATE_RV_ONHOOK_TX:
			pdx->lf[phone] = LINE_STATE_FD_ONHOOK_TX;
			break;
		case LINE_STATE_REVERSE_ACTIVE:
			pdx->lf[phone] = LINE_STATE_FORWARD_ACTIVE;
			break;
		}
		break;

	case LF_ONHOOK_TX_ON:
		/* turns on onhook TX, while keeping reversal state */
		if (pdx->lf[phone] == LINE_STATE_FD_ONHOOK_TX ||
		    pdx->lf[phone] == LINE_STATE_FORWARD_ACTIVE)
			pdx->lf[phone] = LINE_STATE_FD_ONHOOK_TX;
		else {
			printk(KERN_CRIT "%s() : pdx->lf[%d] = %d\n",
				__FUNCTION__, phone, pdx->lf[phone]);
			pdx->lf[phone] = LINE_STATE_RV_ONHOOK_TX;

		}
		break;

	case LF_ONHOOK_TX_OFF:
		/* turns off onhook TX, while keeping reversal state */
		if (pdx->lf[phone] == LINE_STATE_FD_ONHOOK_TX ||
		    pdx->lf[phone] == LINE_STATE_FORWARD_ACTIVE)
			pdx->lf[phone] = LINE_STATE_FORWARD_ACTIVE;
		else
			pdx->lf[phone] = LINE_STATE_REVERSE_ACTIVE;
		break;
	}

	return 0;
}


int daytona_phone_reversal(PDEVICE_EXTENSION pdx, int phone)
{
	unsigned long flags;

	CHECK_PHONE(pdx, phone);

	spin_lock_irqsave(&pdx->ringgen_lock, flags);
	if (pdx->line_state[phone] & STATE_BIT_RINGING) {
		spin_unlock_irqrestore(&pdx->ringgen_lock, flags);
		return IOCTL_ERROR_INVALID_LINE_STATE;
	}

	daytona_phone_set_lf(pdx, phone, LF_FLIP);

	/* change line feed */
	set_line_state(pdx, phone, pdx->lf[phone]);

	spin_unlock_irqrestore(&pdx->ringgen_lock, flags);
	return 0;
}

EXPORT_SYMBOL(daytona_phone_reversal);

int daytona_phone_disconnect(PDEVICE_EXTENSION pdx, int phone)
{
	unsigned long flags;

	CHECK_PHONE(pdx, phone);

	spin_lock_irqsave(&pdx->ringgen_lock, flags);
	if (!(pdx->line_state[phone] & STATE_BIT_OFFHOOK) &&
	    !(pdx->line_state[phone] & STATE_BIT_DISCONNECTED)) {
		spin_unlock_irqrestore(&pdx->ringgen_lock, flags);
		return IOCTL_ERROR_INVALID_LINE_STATE;
	}

	pdx->line_state[phone] |= STATE_BIT_DISCONNECTED;

	/* don't write to lf variable */
	set_line_state(pdx, phone, LINE_STATE_OPEN);
	DELAY_SET(&pdx->disconnect_delay[phone], pdx->disconnect_debounce);

	spin_unlock_irqrestore(&pdx->ringgen_lock, flags);
	return 0;
}


int daytona_phone_set_onhook_tx(PDEVICE_EXTENSION pdx, phone_onhook_tx_t *rqst)
{
	unsigned long flags;
	int phone = rqst->phone;

	CHECK_PHONE(pdx, phone);

	/* lock because checking and changing state */
	spin_lock_irqsave(&pdx->ringgen_lock, flags);

	if (rqst->enable) { /* turn on */
		pdx->line_state[phone] |= STATE_BIT_ONHOOK_TX_ENABLED;
		if (!(pdx->line_state[phone] & STATE_BIT_OFFHOOK)) {
			daytona_phone_set_lf(pdx, phone, LF_ONHOOK_TX_ON);
			if (!(pdx->line_state[phone] & STATE_BIT_RINGING))
				set_line_state(pdx, phone, pdx->lf[phone]);
		}

		/* in case we were waiting to turn off */
		DELAY_CLR(&pdx->onhook_tx_off_delay[phone]);
	} else {
		/* turn off
		 *
		 * Start timer.  We need to delay setting back to
		 * normal onhook state because of data left in media
		 * buffers. (~= 40 ms worth)
		 */
		if (!(pdx->line_state[phone] & STATE_BIT_OFFHOOK))  /* onhook */
			DELAY_SET(&pdx->onhook_tx_off_delay[phone],
				  ONHOOK_TX_OFF_DELAY);
		else /* offhook, just turn bit off in state */
			pdx->line_state[phone] &= ~STATE_BIT_ONHOOK_TX_ENABLED;
	}

	spin_unlock_irqrestore(&pdx->ringgen_lock, flags);

	if (dbg)
		PK_PRINT("daytona_phone_set_onhook_tx State=0x%x lf=0x%x\n",
			 pdx->line_state[phone], pdx->lf[phone]);
	return 0;
}

EXPORT_SYMBOL(daytona_phone_set_onhook_tx);

int daytona_phone_set_audio(PDEVICE_EXTENSION pdx, phone_audio_t *audio)
{
	u8 value;
	int phone = audio->phone;

	CHECK_PHONE(pdx, phone);

	/* Muting */
	read_silabs(pdx, phone, AUDIO_GAIN, &value);
	if (audio->muteRX)
		value |= AUDIO_GAIN_MUTE_RX;
	else
		value &= ~AUDIO_GAIN_MUTE_RX;
	if (audio->muteTX)
		value |= AUDIO_GAIN_MUTE_TX;
	else
		value &= ~AUDIO_GAIN_MUTE_TX;
	write_silabs(pdx, phone,  AUDIO_GAIN, value);

	/* Digital Programmable Gain/Attenuation.
	 *
	* These LOOK like they're backwards, but that's because
	* they're defined relative to the chip.
	*/
	WriteSilabsIndirect(pdx, phone, RECV_DIGITAL_GAIN, audio->gainTX);
	WriteSilabsIndirect(pdx, phone, XMIT_DIGITAL_GAIN, audio->gainRX);

	return 0;
}

int daytona_phone_get_diag(PDEVICE_EXTENSION pdx, int phone,
			   phone_get_diag_out_t *diag)
{
	u8 val;
	int i;
	CHECK_PHONE(pdx, phone);

	/* Read transistor currents */
	for (i = 0; i < 6; i++)
		read_silabs(pdx, phone, IQ1 + i,  &(diag->q[i]));

	/* Read battery voltages */
	read_silabs(pdx, phone, BAT_V_HI_SENSE,  &(diag->battery[0]));
	read_silabs(pdx, phone, BAT_V_LO_SENSE,  &(diag->battery[1]));

	/* Read audio loopback mode */
	read_silabs(pdx, phone, AUDIO_LOOPBACK,  &(diag->loopback));

	/* Read audio loopback mode */
	read_silabs(pdx, phone, AUDIO_GAIN, &val);
	diag->analog_gain = val & 0x0f;

	return 0;
}

int daytona_phone_set_diag(PDEVICE_EXTENSION pdx, phone_set_diag_t *diag)
{
	u8 value;
	int phone = diag->phone;

	CHECK_PHONE(pdx, phone);

	write_silabs(pdx, phone, AUDIO_LOOPBACK, diag->loopback);
	value = read_silabs(pdx, phone, AUDIO_GAIN, &value);
	value = (value & 0xf0) + diag->analog_gain;
	write_silabs(pdx, phone, AUDIO_GAIN, value);
	return 0;
}

static inline void handle_ring_off(PDEVICE_EXTENSION pdx, int line,
				      struct ring_cadence *rcad)
{
	if (DELAY_UP(&pdx->ring_timeout[line])) {
		/* the circuit has timed out without being answered */
		daytona_stop_ringing(pdx, line);
		pdx->line_state[line] &= ~STATE_BIT_RINGGEN_ENABLED;
		pdx->line_state[line] &= ~STATE_BIT_RINGING;
		add_event(pdx, LINE_EVENT, line, PHONE_EVENT_RING_TIMEOUT);
		DELAY_CLR(&pdx->ring_timeout[line]);
	} else if ((rcad->cur_brush[line] & pdx->ring_bit) == 0) {
		/* Normal Ring Off */
		 /* restore previous linefeed */
		set_line_state(pdx, line, pdx->lf[line]);
		if (pdx->line_state[line] & STATE_BIT_RINGING) {
			pdx->line_state[line] &= ~STATE_BIT_RINGING;
			add_event(pdx, LINE_EVENT, line, PHONE_EVENT_RING_OFF);
		}
	}
}

static inline void handle_ring_on(PDEVICE_EXTENSION pdx, int line,
				     struct ring_cadence *rcad)

{
	if (rcad->cur_brush[line] & pdx->ring_bit) {
		/* Ring On */
		set_line_state(pdx, line, LINE_STATE_RINGING);
		if (!(pdx->line_state[line] & STATE_BIT_RINGING)) {
			pdx->line_state[line] |= STATE_BIT_RINGING;
			add_event(pdx, LINE_EVENT, line, PHONE_EVENT_RING_ON);
		}
	}
}

static inline void handle_power_alarm(PDEVICE_EXTENSION pdx, int i)
{
	if (pdx->line_state[i] & STATE_POWER_ALARM_DEBOUNCE) {
		pdx->line_state[i] &=
			~(STATE_POWER_ALARM | STATE_POWER_ALARM_DEBOUNCE);
		pdx->disabled_mask &= ~(1 << i);
		add_event(pdx, PFT_EVENT, i, 0);
		PK_PRINT("%d.%02d: Power alarm off\n", pdx->id, i);
	} else { /* Timeout - reset the phone and try again. */
		/* We are already holding the lock in the ring_dpc()
		 * function calling handle_power_alarm() */
		do_phone_reset_nolock(pdx, i);
		pdx->line_state[i] |= STATE_POWER_ALARM_DEBOUNCE;
		DELAY_SET(&pdx->power_alarm_timeout[i], POWER_ALARM_DEBOUNCE);
	}
}


#ifdef __linux__
void ring_tasklet(unsigned long arg)
#else
void ring_dpc(PKDPC dpc, PVOID arg, PVOID unused1, PVOID unused2)
#endif
{
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)arg;
	unsigned long flags;
	struct ring_cadence *rcad;
	int i;

	/* Update the ring_bit and the ring_slot if necessary. */
	pdx->ring_bit >>= 1;
	if (pdx->ring_bit == 0) {
		pdx->ring_bit = 1 << 7;
		if (++pdx->ring_slot >= BYTES_PER_CADENCE)
			pdx->ring_slot = 0;
	}

#ifdef _WIN32
	KeAcquireSpinLockAtDpcLevel(&pdx->ringgen_lock);
#else
	spin_lock_irqsave(&pdx->ringgen_lock, flags);
#endif

	rcad = &pdx->ring_cadences[pdx->ring_slot];

	/* Turn'em off first */
	for (i = 0; i < ICCR_MAX_PHONES; ++i)
		if (IS_PHONE(pdx, i) &&
		    (pdx->line_state[i] & STATE_BIT_RINGGEN_ENABLED))
			/* this circuit is currently ringing */
			handle_ring_off(pdx, i, rcad);

	/* Turn'em on last */
	for (i = 0; i < ICCR_MAX_PHONES; ++i)
		if (IS_PHONE(pdx, i) &&
		    (pdx->line_state[i] & STATE_BIT_RINGGEN_ENABLED))
			/* this circuit is currently ringing */
			handle_ring_on(pdx, i, rcad);

	/* Power alarms */
	for (i = 0; i < ICCR_MAX_LINES; ++i)
		if (pdx->line_state[i] & STATE_POWER_ALARM)
			if (DELAY_UP(&pdx->power_alarm_timeout[i]))
				handle_power_alarm(pdx, i);

#ifdef _WIN32
	KeReleaseSpinLockFromDpcLevel(&pdx->ringgen_lock);
#else
	spin_unlock_irqrestore(&pdx->ringgen_lock, flags);
#endif
}

EXPORT_SYMBOL(ring_tasklet);


static inline void handle_ringtrip(PDEVICE_EXTENSION pdx, int i)
{
	unsigned long flags;
	spin_lock_irqsave(&pdx->ringgen_lock, flags);

	daytona_stop_ringing(pdx, i);

	/* put back to normal feed if necessary */
	if (pdx->line_state[i] & STATE_BIT_ONHOOK_TX_ENABLED) {
		daytona_phone_set_lf(pdx, i, LF_ONHOOK_TX_OFF);
		set_line_state(pdx, i, pdx->lf[i]);
	}

	/* offhook, preserve onhook_tx, zero everything else. */
	pdx->line_state[i] = STATE_BIT_OFFHOOK |
		(pdx->line_state[i] & (STATE_BIT_ONHOOK_TX_ENABLED |
				       STATE_POWER_ALARM |
				       STATE_POWER_ALARM_DEBOUNCE));

	spin_unlock_irqrestore(&pdx->ringgen_lock, flags);

	add_event(pdx, LINE_EVENT, i, PHONE_EVENT_RING_OFF);
	add_event(pdx, LINE_EVENT, i, PHONE_EVENT_OFFHOOK); /* offhook */
}

static inline void handle_lc(PDEVICE_EXTENSION pdx, int i)
{
	u8 data;

	read_silabs(pdx, i, LOOP_STAT, &data);

	if ((data & 1) == 0) {
		/* LC off, could be onhook, start hookflash debounce */
		DELAY_SET(&pdx->hookflash_delay[i], pdx->hookflash_debounce);
		return;
	}

	if (pdx->line_state[i] & STATE_BIT_DISCONNECTED) {
		unsigned long flags;
		/* Now unset disconnect bit.  Do it here to mask out
		 * OFFHOOK event
		 */
		spin_lock_irqsave(&pdx->ringgen_lock, flags);
		pdx->line_state[i] &= ~STATE_BIT_DISCONNECTED;
		spin_unlock_irqrestore(&pdx->ringgen_lock, flags);
	} else if (pdx->line_state[i] & STATE_BIT_RINGGEN_ENABLED) {
		unsigned long flags;
		spin_lock_irqsave(&pdx->ringgen_lock, flags);
		daytona_stop_ringing(pdx, i);
		if (pdx->line_state[i] & STATE_BIT_RINGING)
			add_event(pdx, LINE_EVENT, i, PHONE_EVENT_RING_OFF);
		/* offhook, preserve onhook_tx setting */
		pdx->line_state[i] = STATE_BIT_OFFHOOK |
			(pdx->line_state[i] & (STATE_BIT_ONHOOK_TX_ENABLED |
					       STATE_POWER_ALARM |
					       STATE_POWER_ALARM_DEBOUNCE));
		/* put back to normal feed if necessary */
		if (pdx->line_state[i] & STATE_BIT_ONHOOK_TX_ENABLED) {
			daytona_phone_set_lf(pdx, i, LF_ONHOOK_TX_OFF);
			set_line_state(pdx, i, pdx->lf[i]);
		}
		spin_unlock_irqrestore(&pdx->ringgen_lock, flags);
		add_event(pdx, LINE_EVENT, i, PHONE_EVENT_OFFHOOK);
	} else if (DELAY_ISSET(&pdx->hookflash_delay[i])) {
		DELAY_CLR(&pdx->hookflash_delay[i]);
		add_event(pdx, LINE_EVENT, i, PHONE_EVENT_HOOKFLASH);
	} else {
		unsigned long flags;
		spin_lock_irqsave(&pdx->ringgen_lock, flags);

		/* put back to normal feed if necessary */
		if (pdx->line_state[i] & STATE_BIT_ONHOOK_TX_ENABLED) {
			daytona_phone_set_lf(pdx, i, LF_ONHOOK_TX_OFF);
			set_line_state(pdx, i, pdx->lf[i]);
		}
		/* offhook, preserve onhook_tx, zero everything else. */
		pdx->line_state[i] = STATE_BIT_OFFHOOK |
			(pdx->line_state[i] & (STATE_BIT_ONHOOK_TX_ENABLED |
					       STATE_POWER_ALARM |
					       STATE_POWER_ALARM_DEBOUNCE));

		spin_unlock_irqrestore(&pdx->ringgen_lock, flags);
		add_event(pdx, LINE_EVENT, i, PHONE_EVENT_OFFHOOK);
	}
}


#ifdef __linux__
void phone_tasklet(unsigned long arg)
#else
void phone_dpc(PKDPC dpc, PVOID arg, PVOID unused1, PVOID unused2)
#endif
{
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)arg;
	unsigned lc_mask, rt_mask;
	unsigned long flags;
	int i;

#ifdef __linux__
	spin_lock_irqsave(&pdx->stat_lock, flags);
	rt_mask = pdx->rt_mask;
	lc_mask = pdx->lc_mask;
	pdx->rt_mask = 0;
	pdx->lc_mask = 0;
	spin_unlock_irqrestore(&pdx->stat_lock, flags);
#else
	/* SAM FIXME - we should pass these in */
	rt_mask = BAR0_READ(pdx, BAR0_RNG_TRP_STAT);
	if (rt_mask)
		BAR0_WRITE(pdx, BAR0_RNG_TRP_STAT, ~rt_mask); /* clear */
	lc_mask = BAR0_READ(pdx, BAR0_LC_TRANS_STAT);
	if (lc_mask)
		BAR0_WRITE(pdx, BAR0_LC_TRANS_STAT, ~lc_mask); /* clear */
#endif

	/* Ring trip */
	for (i = 0; rt_mask && i < ICCR_MAX_LINES; ++i, rt_mask >>= 1)
		if (rt_mask & 1)
			handle_ringtrip(pdx, i);

	/* LC */
	for (i = 0; lc_mask && i < ICCR_MAX_LINES; ++i, lc_mask >>= 1)
		if (lc_mask & 1)
			handle_lc(pdx, i);
}

EXPORT_SYMBOL(phone_tasklet);


static inline void handle_hookflash(PDEVICE_EXTENSION pdx, int line)
{
	unsigned long flags;

	DELAY_CLR(&pdx->hookflash_delay[line]);

	/* Onhook state, stop disconnecting, restore onhook tx lf */
	spin_lock_irqsave(&pdx->ringgen_lock, flags);

	pdx->line_state[line] &= ~STATE_BIT_OFFHOOK;

	if (pdx->line_state[line] & STATE_BIT_DISCONNECTED) {
		DELAY_CLR(&pdx->disconnect_delay[line]);
		pdx->line_state[line] &= ~STATE_BIT_DISCONNECTED;
	}

	if (pdx->line_state[line] & STATE_BIT_ONHOOK_TX_ENABLED)
		daytona_phone_set_lf(pdx, line, LF_ONHOOK_TX_ON);

	/* restoring line feed. */
	set_line_state(pdx, line, pdx->lf[line]);

	spin_unlock_irqrestore(&pdx->ringgen_lock, flags);

	add_event(pdx, LINE_EVENT, line, PHONE_EVENT_ONHOOK);
}

static inline void handle_disconnect(PDEVICE_EXTENSION pdx, int line)
{
	unsigned long flags;

	DELAY_CLR(&pdx->disconnect_delay[line]);

	/* Don't undo disconnect bit here!!!  Wait until LC on is
	 * received or we get an extra offhook
	 *
	 * Don't need to check for ringing, but need to undo onhook TX
	 * line feed
	 */
	spin_lock_irqsave(&pdx->ringgen_lock, flags);

	if (pdx->line_state[line] & STATE_BIT_ONHOOK_TX_ENABLED)
		daytona_phone_set_lf(pdx, line, LF_ONHOOK_TX_OFF);

	/* restore last lf state */
	set_line_state(pdx, line, pdx->lf[line]);

	spin_unlock_irqrestore(&pdx->ringgen_lock, flags);

	/* Delay before checking hookstate. */
	DELAY_SET(&pdx->disconnect_hookstate_delay[line],
		  pdx->disconnect_hookstate_debounce);
}

static inline void check_disconnect(PDEVICE_EXTENSION pdx, int line)
{
	unsigned long flags;
	u8 data;

	DELAY_CLR(&pdx->disconnect_hookstate_delay[line]);
	read_silabs(pdx, line, LOOP_I_SENSE, &data);

	if ((data & 0x3f) < 8) { /* |I| < 10ma = 1.25mA*8  is ONHOOK */
		/* start hookflash debounce */
		DELAY_SET(&pdx->hookflash_delay[line], pdx->hookflash_debounce);

		/* unset disconnect bit so we can detect LC ON
		 * again.
		 */
		spin_lock_irqsave(&pdx->ringgen_lock, flags);
		pdx->line_state[line] &= ~STATE_BIT_DISCONNECTED;
		spin_unlock_irqrestore(&pdx->ringgen_lock, flags);
	}
	/* if current < 10ma, wait until next LC on to unset
	 * disconnected bit.
	 */
}

static inline void check_offhook(PDEVICE_EXTENSION pdx, int line)
{
	unsigned long flags;

	DELAY_CLR(&pdx->onhook_tx_off_delay[line]);

	spin_lock_irqsave(&pdx->ringgen_lock, flags);

	pdx->line_state[line] &= ~STATE_BIT_ONHOOK_TX_ENABLED;
	if (!(pdx->line_state[line] & STATE_BIT_OFFHOOK)) {
		daytona_phone_set_lf(pdx, line, LF_ONHOOK_TX_OFF);
		if (!(pdx->line_state[line] & STATE_BIT_RINGING))
			set_line_state(pdx, line, pdx->lf[line]);
	}

	spin_unlock_irqrestore(&pdx->ringgen_lock, flags);
}


#ifdef __linux__
void line_tasklet(unsigned long arg)
#else
void line_dpc(PKDPC dpc, PVOID arg, PVOID unused1, PVOID unused2)
#endif
{
	int line;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)arg;

	/* Put the line events in the ring buffer. */
	for (line = 0; line < ICCR_MAX_LINES; ++line) {
		if (DELAY_UP(&pdx->hookflash_delay[line]))
			handle_hookflash(pdx, line);

		if (DELAY_UP(&pdx->disconnect_delay[line]))
			handle_disconnect(pdx, line);

		/* Check to see if there was an onhook during
		 * disconnect by checking current.
		 */
		if (DELAY_UP(&pdx->disconnect_hookstate_delay[line]))
			check_disconnect(pdx, line);

		/* Check to see if we have to turn OFF onhook TX state. */
		if (DELAY_UP(&pdx->onhook_tx_off_delay[line]))
			check_offhook(pdx, line);
	}
}

EXPORT_SYMBOL(line_tasklet);

/* Turn off dc/dc convertor */
void daytona_power_down_phones(PDEVICE_EXTENSION pdx)
{
	int i;
	unsigned mask = pdx->phone_mask;

	for (i = 0; mask; ++i, mask >>= 1)
		if (mask & 1) {
			write_silabs(pdx, i, PWR_DOWN1 , 0x10);
			set_line_state(pdx, i, 0);
		}
}

/*
 * Force kernel coding standard on PIKA code
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
