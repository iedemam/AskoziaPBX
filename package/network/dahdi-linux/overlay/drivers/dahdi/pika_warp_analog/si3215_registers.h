/*******************************************************************************
**
**    Filename:  Si3215_Registers.h
**
**    Contents:  Si3215 Register offsets
**
**
**     History: October 2, 2006 - Creation: David Modderman
**
**     Copyright 2006-2008 PIKA Technologies Inc.
**
*******************************************************************************/

#ifndef SI3215REGS_H
#define SI3215REGS_H


/* Direct Registers */
#define	SPI_MODE                  0x00
#define	PCM_MODE                  0x01   /* Audio Format - mulaw/alaw/8kHz */
#define	PCM_XMIT_START_COUNT_LSB	0x02
#define	PCM_XMIT_START_COUNT_MSB	0x03
#define	PCM_RCV_START_COUNT_LSB		0x04
#define	PCM_RCV_START_COUNT_MSB		0x05
#define	DIO                       0x06
#define	AUDIO_LOOPBACK            0x08
#define	AUDIO_GAIN                0x09
#define	LINE_IMPEDANCE      0x0A
#define	HYBRID              0x0B
#define	RESERVED12          0x0C
#define	RESERVED13          0x0D
#define	PWR_DOWN1           0x0E
#define	PWR_DOWN2           0x0F
#define	RESERVED16          0x10
#define	RESERVED17          0x11
#define	INTRPT_STATUS1      0x12
#define	INTRPT_STATUS2      0x13
#define	INTRPT_STATUS3      0x14
#define	INTRPT_MASK1				0x15
#define	INTRPT_MASK2				0x16
#define	INTRPT_MASK3				0x17
#define	DTMF_DIGIT					0x18
#define	RESERVED25					0x19
#define	RESERVED26					0x1A
#define	RESERVED27					0x1B
#define	I_DATA_LOW					0x1C
#define	I_DATA_HIGH					0x1D
#define	I_ADDRESS					  0x1E
#define	I_STATUS					  0x1F
#define	OSC1						    0x20
#define	OSC2						    0x21
#define	RING_OSC_CTL				0x22
#define	PULSE_OSC					  0x23
#define	OSC1_ON__LO					0x24
#define	OSC1_ON_HI					0x25
#define	OSC1_OFF_LO					0x26
#define	OSC1_OFF_HI					0x27
#define	OSC2_ON__LO					0x28
#define	OSC2_ON_HI					0x29
#define	OSC2_OFF_LO					0x2A
#define	OSC2_OFF_HI					0x2B
#define	PULSE_ON_LO					0x2C
#define	PULSE_ON_HI					0x2D
#define	PULSE_OFF_LO				0x2E
#define	PULSE_OFF_HI				0x2F
#define	RING_ON_LO					0x30
#define	RING_ON_HI					0x31
#define	RING_OFF_LO					0x32
#define	RING_OFF_HI					0x33
#define	FSK_DATA					  0x34
#define	RESERVED53					0x35
#define	RESERVED54					0x36
#define	RESERVED55					0x37
#define	RESERVED56					0x38
#define	RESERVED57					0x39
#define	RESERVED58					0x3A
#define	RESERVED59					0x3B
#define	RESERVED60					0x3C
#define	RESERVED61					0x3D
#define	RESERVED62					0x3E
#define	RNG_CLOS_DBNC				0x3F
#define LINE_STATE          0x40 /* Linefeed Control. See LINE_STATE_* */
#define	BIAS_SQUELCH				0x41
#define	BAT_FEED					  0x42
#define	AUTO_STATE					0x43
#define	LOOP_STAT					  0x44
#define	LOOP_DEBOUNCE				0x45
#define	RT_DEBOUNCE					0x46
#define	LOOP_I_LIMIT				0x47
#define	ON_HOOK_V					  0x48
#define	COMMON_V					  0x49
#define	BAT_V_HI					  0x4A
#define	BAT_V_LO					  0x4B
#define	PWR_STAT_DEV				0x4C
#define	PWR_STAT					  0x4D
#define	LOOP_V_SENSE				0x4E
#define	LOOP_I_SENSE				0x4F
#define	TIP_V_SENSE					0x50
#define	RING_V_SENSE				0x51
#define	BAT_V_HI_SENSE		  0x52
#define	BAT_V_LO_SENSE		  0x53
#define	IQ1							0x54
#define	IQ2							0x55
#define	IQ3							0x56
#define	IQ4							0x57
#define	IQ5							0x58
#define	IQ6							0x59
#define	RESERVED90					0x5A
#define	RESERVED91					0x5B
#define	DCDC_PWM_OFF				0x5C
#define	DCDC						    0x5D
#define	DCDC_PW_OFF					0x5E
#define	RESERVED95					0x5F
#define	CALIBR1						  0x60
#define	CALIBR2						  0x61
#define	RING_GAIN_CAL				0x62
#define	TIP_GAIN_CAL				0x63
#define	DIFF_I_CAL					0x64
#define	COMMON_I_CAL				0x65
#define	I_LIMIT_GAIN_CAL		0x66
#define	ADC_OFFSET_CAL			0x67
#define	DAC_ADC_OFFSET			0x68
#define	DAC_OFFSET_CAL			0x69
#define	COMMON_BAL_CAL			0x6A
#define	DC_PEAK_CAL					0x6B
#define	ENHANCE						  0x6C


/* 		Indirect Registers */
#define	OSC1_COEF         0x00
#define	OSC1X             0x01
#define	OSC1Y             0x02
#define	OSC2_COEF         0x03
#define	OSC2X             0x04
#define	OSC2Y             0x05
#define	RING_V_OFF        0x06
#define	RING_OSC_COEF     0x07
#define	RING_X            0x08
#define	RING_Y            0x09
#define	PULSE_ENVEL       0x0A
#define	PULSE_X           0x0B
#define	PULSE_Y           0x0C
#define	RECV_DIGITAL_GAIN     0x0D
#define	XMIT_DIGITAL_GAIN     0x0E
#define	LOOP_CLOSE_THRESH     0x0F
#define	RING_TRIP_THRESH      0x10
#define	COMMON_MIN_THRESH     0x11
#define	COMMON_MAX_THRESH     0x12
#define	PWR_ALARM_Q1Q2        0x13
#define	PWR_ALARM_Q3Q4        0x14
#define	PWR_ALARM_Q5Q6        0x15
#define	LOOP_CLOSURE_FILTER   0x16
#define	RING_TRIP_FILTER      0x17
#define	THERM_LP_POLE_Q1Q2    0x18
#define	THERM_LP_POLE_Q3Q4    0x19
#define	THERM_LP_POLE_Q5Q6    0x1A
#define	CM_BIAS_RINGING       0x1B
#define	DCDC_MIN_V             0x40
#define	LOOP_CLOSE_THRESH_LOW  0x42
#define FSK_X_0                0x45	/* x sign	fsk_x_0[15:0] */
#define FSK_COEFF_0            0x46	/* x sign fsk_coeff_0[15:0] */
#define FSK_X_1                0x47	/* x sign	fsk_x_1[15:0] */
#define FSK_COEFF_1            0x48	/* x sign fsk_coeff_1[15:0]	*/
#define FSK_X_01               0x49	/* x sign fsk_x_01[15:0] */
#define FSK_X_10               0x4A	/* x sign	fsk_x_10[15:0] */



#define AUDIO_LOOPBACK_DEFAULT_VAL	0x02
#define	HYBRID_DEFAULT_VAL          0x33
#define	LINE_STATE_DEFAULT_VAL      0x00

/*
 * Line Feed state values
 */
#define LINE_STATE_OPEN            0
#define LINE_STATE_FORWARD_ACTIVE  1
#define LINE_STATE_FD_ONHOOK_TX    2
#define LINE_STATE_TIP_OPEN        3
#define LINE_STATE_RINGING         4
#define LINE_STATE_REVERSE_ACTIVE  5
#define LINE_STATE_RV_ONHOOK_TX    6
#define LINE_STATE_RING_OPEN       7
/* logical line feed operations */
#define LF_FORWARD        8
#define LF_REVERSE        9
#define LF_FLIP          10
#define LF_ONHOOK_TX_ON  11
#define LF_ONHOOK_TX_OFF 12

/*
 * AUDIO_GAIN control bits
 */
#define AUDIO_GAIN_MUTE_RX  0x08
#define AUDIO_GAIN_MUTE_TX  0x10


/*
** This defines the neumomics for the Si324x registers
*/

/* enum REGISTERS {}; */


/*
** This defines the neumomics for the Si324x RAM locations
*/

/* enum SRAM {}; */

#endif
