/*
 * Copyright (c) 2001, Adaptive Digital Technologies, Inc.
 *
 * File Name: GpakHpi.h
 *
 * Description:
 *   This file contains common definitions related to the G.PAK interface
 *   between a host processor and a DSP processor via the Host Port Interface.
 *
 * Version: 1.0
 *
 * Revision History:
 *   10/17/01 - Initial release.
 *
 */

#ifndef _GPAKHPI_H  /* prevent multiple inclusion */
#define _GPAKHPI_H


/* Definition of G.PAK Command/Reply message type codes. */
#define MSG_NULL_REPLY 0            /* Null Reply (unsupported Command) */

#define MSG_SYS_CONFIG_RQST 1             /* System Configuration Request */
#define MSG_SYS_CONFIG_REPLY 2

#define MSG_READ_SYS_PARMS 3              /* Read System Parameters */
#define MSG_READ_SYS_PARMS_REPLY 4  

#define MSG_WRITE_SYS_PARMS 5             /* Write System Parameters */
#define MSG_WRITE_SYS_PARMS_REPLY 6
 
#define MSG_CONFIGURE_PORTS    7          /* Configure Serial Ports */
#define MSG_CONFIG_PORTS_REPLY 8

#define MSG_CONFIGURE_CHANNEL  9          /* Configure Channel */
#define MSG_CONFIG_CHAN_REPLY 10    

#define MSG_TEAR_DOWN_CHANNEL 11          /* Tear Down Channel */
#define MSG_TEAR_DOWN_REPLY   12      

#define MSG_CHAN_STATUS_RQST  13          /* Channel Status Request */
#define MSG_CHAN_STATUS_REPLY 14

#define MSG_CONFIG_CONFERENCE 15          /* Configure Conference */
#define MSG_CONFIG_CONF_REPLY 16    

#define MSG_TEST_MODE  17                 /* Configure/Perform Test Mode */
#define MSG_TEST_REPLY 18

#define MSG_WRITE_NOISE_PARAMS       19   /* Write Noise Suppression Parameters */
#define MSG_WRITE_NOISE_PARAMS_REPLY 20 

#define MSG_READ_NOISE_PARAMS       21    /* Read Noise Suppression Parameters */
#define MSG_READ_NOISE_PARAMS_REPLY 22

#define MSG_READ_AEC_PARMS        23      /* Read AEC Parameters */
#define MSG_READ_AEC_PARMS_REPLY  24  

#define MSG_WRITE_AEC_PARMS       25      /* Write AEC Parameters */
#define MSG_WRITE_AEC_PARMS_REPLY 26 

#define MSG_TONEGEN       27              /* generate a tone */
#define MSG_TONEGEN_REPLY 28

#define MSG_PLAY_RECORD          29       /* voice playback and record */
#define MSG_PLAY_RECORD_RESPONSE 30

#define MSG_RTP_CONFIG           31       /* RTP configuration paraemters */
#define MSG_RTP_CONFIG_REPLY     32

#define MSG_READ_ECAN_STATE        33     /* Read Ecan State */
#define MSG_READ_ECAN_STATE_REPLY  34     

#define MSG_WRITE_ECAN_STATE       35     /* Write Ecan State */
#define MSG_WRITE_ECAN_STATE_REPLY 36     

#define MSG_ALG_CONTROL            37     /* Algorithm Control */
#define MSG_ALG_CONTROL_REPLY      38     

#define MSG_WRITE_TXCID_DATA        39     /* Send CID TX MSG data */
#define MSG_WRITE_TXCID_DATA_REPLY  40     

#define MSG_CONFTONEGEN            41     /* Generate a conference tone */
#define MSG_CONFTONEGEN_REPLY      42     

#define MSG_READ_AGC_POWER         43     /* Read AGC Power */
#define MSG_READ_AGC_POWER_REPLY   44     

#define MSG_RESET_CHAN_STATS       45     /* Reset Channel Statistics */
#define MSG_RESET_CHAN_STATS_REPLY 46     

#define MSG_RESET_SYS_STATS        47     /* Reset System Statistics */
#define MSG_RESET_SYS_STATS_REPLY  48     

#define MSG_READ_MCBSP_STATS       49     /* Read McBSP Statistics */
#define MSG_READ_MCBSP_STATS_REPLY 50     

#define MSG_CONFIG_ARB_DETECTOR       51  /* Configure ARbitrary Detector */
#define MSG_CONFIG_ARB_DETECTOR_REPLY 52    

#define MSG_ACTIVE_ARB_DETECTOR       53  /* Configure ARbitrary Detector */
#define MSG_ACTIVE_ARB_DETECTOR_REPLY 54    

#define MSG_READ_CPU_USAGE            55
#define MSG_READ_CPU_USAGE_REPLY      56

#define MSG_SEND_DATATODSP            57
#define MSG_SEND_DATATODSP_REPLY      58

#define MSG_GET_DATAFROMDSP            59
#define MSG_GET_DATAFROMDSP_REPLY      60

#define MSG_TIME_OUT_REPLY     250   /* Message timed out */

typedef struct {
	short LowThreshdB;	// Threshold below which we declare noise
	short HighThreshdB;	// Threshold above which we declare speech
	short HangMSec;
	short FrameSize;
	short WindowSize;
} HostVAD_Params_t;

typedef struct {
	short MaxAttendB;
} HostNC_Params_t;

typedef struct{
	HostNC_Params_t  NC_Params;
	HostVAD_Params_t VAD_Params;
} HostNCAN_Params_t;


#endif  /* prevent multiple inclusion */
