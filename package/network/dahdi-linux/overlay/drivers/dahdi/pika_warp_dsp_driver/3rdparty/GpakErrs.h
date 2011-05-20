/*
 * Copyright (c) 2002 - 2007, Adaptive Digital Technologies, Inc.
 *
 * File Name: GpakErrs.h
 *
 * Description:
 *   This file contains DSP reply status codes used by G.PAK API functions to
 *   indicate specific errors. This file is common to both G.PAK API and
 *   G.PAK DSP code.
 *
 * Version: 1.0
 *
 * Revision History:
 *   10/17/01 - Initial release.
 *   07/03/02 - Updates for conferencing.
 *   06/15/04 - Tone type updates.
 *   1/2007   - Combined C54 and C64
 */

#ifndef _GPAKERRS_H  /* prevent multiple inclusion */
#define _GPAKERRS_H

// General DSP API interface API error codes
typedef enum GPAK_DspCommStat_t {
    DSPSuccess = 0, DSPNullIFAddr, DSPHostStatusError,  
    DSPCmdLengthError, DSPCmdTimeout,  DSPRplyLengthError, DSPRplyTimeout,
    DSPRplyValue,      DSPNoConnection
} GPAK_DspCommStat_t;



/* Write System Parameters reply status codes. */
typedef enum GPAK_SysParmsStat_t {
    Sp_Success = 0,             /* System Parameters written successfully */

    /* AGC parameters errors. */
    Sp_AgcNotConfigured = 1,    /* AGC is not configured in system */
    Sp_BadAgcTargetPower,   /* invalid AGC Target Power value */
    Sp_BadAgcLossLimit,     /* invalid AGC Maximum Attenuation value */
    Sp_BadAgcGainLimit,     /* invalid AGC Max Positive Gain value */
    Sp_BadAgcLowSignal,

    /* VAD parameters errors. */
    Sp_VadNotConfigured = 10,    /* VAD is not configured in system */
    Sp_BadVadNoiseFloor,    /* invalid VAD Noise Floor value */
    Sp_BadVadHangTime,      /* invalid VAD Report Delay Time value */
    Sp_BadVadWindowSize,    /* invalid VAD Analysis Window Size value */

    /* PCM Echo Canceller parameters errors. */
    Sp_PcmEcNotConfigured = 20,     /* PCM Ecan is not configured in system */
    Sp_BadPcmEcTapLength,     /* invalid PCM Ecan Num Taps */
    Sp_BadPcmEcNlpType,       /* invalid PCM Ecan NLP Type */
    Sp_BadPcmEcDblTalkThresh, /* invalid PCM Ecan Double Talk threshold */
    Sp_BadPcmEcNlpThreshold,  /* invalid PCM Ecan NLP threshold */
    Sp_BadPcmEcCngThreshold,  /* invalid PCM Ecan CNG Noise */
    Sp_BadPcmEcAdaptLimit,    /* invalid PCM Ecan Adapt Limit */
    Sp_BadPcmEcCrossCorrLim,  /* invalid PCM Ecan Cross Corr Limit */
    Sp_BadPcmEcNumFirSegs,    /* invalid PCM Ecan Num FIR Segments */
    Sp_BadPcmEcFirSegLen,     /* invalid PCM Ecan FIR Segment Length */
    Sp_BadPcmEcNLPUpperConv,
    Sp_BadPcmEcNLPUpperUnConv,
    Sp_BadPcmEcMaxSupp,
    Sp_BadPcmEcFIRTapCheck,
    Sp_BadPcmEcMaxDblTalkThresh,

    /* Packet Echo Canceller parameters errors. */
    Sp_PktEcNotConfigured = 40,    /* Pkt Ecan is not configured in system */
    Sp_BadPktEcTapLength,     /* invalid Pkt Ecan Num Taps */
    Sp_BadPktEcNlpType,       /* invalid Pkt Ecan NLP Type */
    Sp_BadPktEcDblTalkThresh, /* invalid Pkt Ecan Double Talk threshold */
    Sp_BadPktEcNlpThreshold,  /* invalid Pkt Ecan NLP threshold */
    Sp_BadPktEcCngThreshold,  /* invalid Pkt Ecan CNG Noise */
    Sp_BadPktEcAdaptLimit,    /* invalid Pkt Ecan Adapt Limit */
    Sp_BadPktEcCrossCorrLim,  /* invalid Pkt Ecan Cross Corr Limit */
    Sp_BadPktEcNumFirSegs,    /* invalid Pkt Ecan Num FIR Segments */
    Sp_BadPktEcFirSegLen,     /* invalid Pkt Ecan FIR Segment Length */
    Sp_BadPktEcNLPUpperConv,
    Sp_BadPktEcNLPUpperUnConv,
    Sp_BadPktEcMaxSupp,
    Sp_BadPktEcFIRTapCheck,
    Sp_BadPktEcMaxDblTalkThresh,

    /* Conference parameters errors. */
    Sp_BadConfDominant = 60,       /* invalid Num Dominant Conference Members */
    Sp_BadMaxConfNoiseSup,    /* invalid Max Conference Noise Suppress */

    //  Noise Suppression errors 
    Sp_NoiseSuppressionNotConfigured = 70,
    Sp_BadVadNoiseCeiling,
    Sp_BadNoiseAttenuation,

    Sp_CidNotConfigured = 80,
    Sp_BadCidNumSeizureBytes,
    Sp_BadCidNumMarkBytes,
    Sp_BadCidNumPostBytes,
    Sp_BadCidFskType,
    Sp_BadCidFskLevel

} GPAK_SysParmsStat_t;

/* Configure Serial Ports reply status codes. */
typedef enum GPAK_PortConfigStat_t {
    Pc_Success = 0,             /* serial ports configured successfully */
    Pc_ChannelsActive = 1,      /* unable to configure while channels active  */
    Pc_TooManySlots0,
    Pc_TooManySlots1,
    Pc_TooManySlots2,

    Pc_NoSlots0,
    Pc_NoSlots1,
    Pc_NoSlots2,
    Pc_InvalidSlots0,
    Pc_InvalidSlots1,

    Pc_InvalidSlots2,
    Pc_UnconfiguredAPort1,    /* unconfigured mcAsp */
    Pc_UnconfiguredAPort2, 
    Pc_UnconfiguredASerPort1, /* unconfigured serializer */
    Pc_UnconfiguredASerPort2,

    Pc_UnequalTxRxASerPort1,  /* unequal # tx-rx serializers*/
    Pc_UnequalTxRxASerPort2,
    Pc_TooManyASlots1,        /* too many slots selected*/
    Pc_TooManyASlots2,     
    Pc_NoASlots1,             /* no audio slots selected*/

    Pc_NoASlots2,          
    Pc_InvalidCompandMode0,   /* invalid companding mode */
    Pc_InvalidCompandMode1,
    Pc_InvalidCompandMode2,
    Pc_InvalidDataDelay0,     /* invalid data delay for ports */

    Pc_InvalidDataDelay1,
    Pc_InvalidDataDelay2,
    Pc_InvalidBlockCombo0,         /* invalid combination of channel blocks */
    Pc_InvalidBlockCombo1,
    Pc_InvalidBlockCombo2,

    Pc_NoInterrupts

} GPAK_PortConfigStat_t;

/* Configure Channel reply status codes. */
typedef enum GPAK_ChannelConfigStat_t {
    Cc_Success = 0,             /* channel configured successfully */
    Cc_InvalidChannelType,      /* invalid Channel Type */
    Cc_InvalidChannelA,         /* invalid Channel A Id */
    Cc_ChannelActiveA,          /* Channel A is currently active */
    Cc_InvalidChannelB,         /* invalid Channel B Id */
    
    Cc_ChannelActiveB,          /* (5) Channel B is currently active */
    Cc_InvalidInputPortA,       /* invalid Input A Port */
    Cc_InvalidInputSlotA,       /* invalid Input A Slot */
    Cc_BusyInputSlotA,          /* busy Input A Slot */
    Cc_InvalidOutputPortA,      /* invalid Output A Port */
    
    Cc_InvalidOutputSlotA,     /* (10) invalid Output A Slot */
    Cc_BusyOutputSlotA,        /* busy Output A Slot */
    Cc_InvalidInputPortB,      /* invalid Input B Port */
    Cc_InvalidInputSlotB,      /* invalid Input B Slot */
    Cc_BusyInputSlotB,         /* busy Input B Slot */
    
    Cc_InvalidOutputPortB,     /* (15) invalid Output B Port */
    Cc_InvalidOutputSlotB,     /* invalid Output B Slot */
    Cc_BusyOutputSlotB,        /* busy Output B Slot */
    Cc_InvalidInputCktSlots,   /* invalid Input Circuit Slots */
    Cc_InvalidInCktPortSize,   /* invalid Input Circuit port word size */
    
    Cc_InvalidOutputCktSlots,  /* (20) invalid Output Circuit Slots */
    Cc_InvalidOutCktPortSize,  /* invalid Output Circuit port word size */
    Cc_InvalidFrameSize,       /* invalid Frame Size */
    Cc_InvalidPktInCodingA,    /* invalid Packet In A Coding */
    Cc_InvalidPktOutCodingA,   /* invalid Packet Out A Coding */
    
    Cc_InvalidPktInSizeA,      /* (25) invalid Packet In A Frame Size */
    Cc_InvalidPktOutSizeA,     /* invalid Packet Out A Frame Size */
    Cc_InvalidPktInCodingB,    /* invalid Packet In B Coding */
    Cc_InvalidPktOutCodingB,   /* invalid Packet Out B Coding */
    Cc_InvalidPktInSizeB,      /* invalid Packet In B Frame Size */
    
    Cc_InvalidPktOutSizeB,     /* (30) invalid Packet Out B Frame Size */
    Cc_PcmEchoCancelNotCfg,    /* PCM Echo Canceller not available */
    Cc_PktEchoCancelNotCfg,    /* Packet Echo Canceller not available */
    Cc_InvalidToneTypesA,      /* invalid Channel A Tone Types */
    Cc_InvalidToneTypesB,      /* invalid Channel B Tone Types */
    
    Cc_VadNotConfigured,       /* (35) VAD not configured */
    Cc_InsuffAgcResourcesAvail,  /* Insufficient AGC resources available */
    Cc_InvalidCktMuxFactor,    /* invalid Circuit Mux Factor */
    Cc_InvalidConference,      /* invalid Conference Id */
    Cc_ConfNotConfigured,      /* conference was not configured (setup) */
    
    Cc_ConferenceFull,         /* (40) conference is full */
    Cc_ChanTypeNotConfigured,  /* channel type was not configured */
    Cc_NoiseSuppressionNotConfigured,
    Cc_InvalidInputPinA,
    Cc_InvalidInputPinB,
    
    Cc_InvalidOutputPinA,      /* (45) */
    Cc_InvalidOutputPinB,
    Cc_AECEchoCancelNotCfg,    
    Cc_FaxRelayNotConfigured,
    Cc_FaxRelayInvalidMode,

    Cc_FaxRelayInvalidTransport, /* (50) */
    Cc_InsuffTGResourcesAvail,    /* not enough tone gen resources */   
    Cc_AgcNotConfigured,
    Cc_InsuffTDResourcesAvail,
    Cc_TxCIDNotAvail,

    Cc_RxCIDNotAvail,             /* (55) */
    Cc_LbcNotCfg,
    Cc_InsuffLbResourcesAvail,
    Cc_InsuffBuffsAvail,
    Cc_InvalidCore,
    Cc_InvalidPktCodingChanId,    /* (60) codec type not supported on this Gpak chan id*/
    Cc_InvalidPcmGpakChanID,      /* pcm channel is not supported on this Gpak chan id */
    Cc_InsuffCodecLicenseAvail    /* not enough codec license */

} GPAK_ChannelConfigStat_t;

/* Tear Down Channel reply status codes. */
typedef enum GPAK_TearDownChanStat_t {
    Td_Success = 0,                 /* channel torn down successfully */
    Td_InvalidChannel = 1,          /* invalid Channel Id */
    Td_ChannelNotActive = 2         /* channel is not active */
} GPAK_TearDownChanStat_t;

/* Channel Status reply status codes. */
typedef enum GPAK_ChannelStatusStat_t {
    Cs_Success = 0,                 /* channel status obtained successfully */
    Cs_InvalidChannel = 1           /* invalid Channel Id */
} GPAK_ChannelStatusStat_t;

/* Configure Conference reply status codes. */
typedef enum GPAK_ConferConfigStat_t {
    Cf_Success = 0,                 /* conference configured successfully */
    Cf_InvalidConference = 1,       /* invalid Conference Id */
    Cf_ConferenceActive = 2,        /* conference is currently active */
    Cf_InvalidFrameSize = 3,        /* invalid Frame Size */
    Cf_InvalidToneTypes = 4,        /* invalid Tone Detect Types */
    Cf_InvalidCore      = 5         /* invalid DSP core */
} GPAK_ConferConfigStat_t;

/* Test Mode reply status codes. */
typedef enum GPAK_TestStat_t {
    Tm_Success = 0,                 /* test mode successfull */
    Tm_InvalidTest = 1,             /* invalid Test Mode Id */
    Tm_InvalidParm = 2,             /* invalid Test Parameter */
    Tm_CustomError = 3              /* custom error code */
} GPAK_TestStat_t;

/* Write AEC Parameters reply status codes. */
typedef enum GPAK_AECParmsStat_t {
    AEC_Success = 0,             // AEC Parameters written successfully

    /* AEC parameters errors. */
    AEC_NotConfigured,           // AEC not configured
    AEC_BadTailLength,
    AEC_BadTxNLPThreshold,
    AEC_BadTxLoss,
    AEC_BadRxLoss,
    AEC_BadTargetResidualLevel,
    AEC_BadMaxRxNoiseLevel,
    AEC_BadWorstExpectedERL,
    AEC_BadAgcMaxGain,
    AEC_BadAgcMaxLoss,
    AEC_BadAgcTargetLevel,
    AEC_BadAgcLowSigThresh,
    AEC_BadMaxTrainingTime,
    AEC_BadRxSaturateLevel,
    AEC_BadTrainingRxNoiseLevel,
    AEC_BadFixedGain

} GPAK_AECParmsStat_t;

// tone generation replay status codes
typedef enum  GPAK_ToneGenStat_t {
    Tg_Success = 0,                 // success
    Tg_InvalidChannel = 1,          // invalid channel Id
    Tg_toneGenNotEnabledA = 2,      // A device tone gen not enabled
    Tg_toneGenNotEnabledB = 3,      // B device tone gen not enabled
    Tg_InvalidToneGenType = 4,      // invalid tonegen type
    Tg_InvalidFrequency = 5,        // invalid tone gen frequency
    Tg_InvalidOnTime = 6,           // invalid on-time
    Tg_InvalidLevel = 7,            // invalid level
    Tg_InactiveChannel = 8,         // inactive channel
    Tg_InvalidCore = 9
} GPAK_ToneGenStat_t;

// playback and record reply status codes
typedef enum GPAK_PlayRecordStat_t {
   PrSuccess = 0,
   PrInvalidChannel,
   PrInactiveChannel,
   PrUnsupportedCommand,
   PrUnsupportedRecordMode,
   PrToneDetectionUnsupported,
   PrInvalidDevice,
   PrInvalidStopTone,
   PrStartAddrOutOfRange,
   PrInvalidAddressAlignment
} GPAK_PlayRecordStat_t;

// RTP configuration reply status codes
typedef enum GPAK_RTPConfigStat_t {
  RTPSuccess,
  RTPTargetError,
  RTPJitterModeError,
  RTPChannelError,
  RTPNotConfigured
} GPAK_RTPConfigStat_t;

// Algorithm control return status codes
typedef enum  GPAK_AlgControlStat_t {
    Ac_Success = 0,           /* algorithm control is successfull */
    Ac_InvalidTone = 1,       /* invalid tone type specified */
    Ac_InvalidChannel = 2,    /* invalid channel identifier */
    Ac_InactiveChannel = 3,   /* channel is inactive */
    Ac_InvalidCode = 4,       /* invalid algorithm control code */
    Ac_ECNotEnabled = 5,      /* echo canceller was not allocated */
    Ac_ECInvalidNLP = 6,      /* Invalid NLP Update setting */
    Ac_PrevCmdPending = 7,    /* The previous control hasn't completed yet */
    Ac_TooManyActiveDetectorsA = 8, /* too many detectors activated on A-side */
    Ac_TooManyActiveDetectorsB = 9, /* too many detectors activated on B-side */
    Ac_InsuffDetectorsAvail = 10,   /* Not enough tone detector instances available */
    Ac_DetectorNotCfgrd = 11,  /* selected detector is not configured */
    Ac_InsuffEcLicenseAvail = 12 /* Not enough Ecan license available */
} GPAK_AlgControlStat_t;

// Read AGC Power return status codes
typedef enum GPAK_AGCReadStat_t {
    Ar_Success = 0,           /* AGC read is successfull on A and/or B side */
    Ar_InvalidChannel = 1,    /* invalid channel identifier */
    Ar_InactiveChannel = 2,   /* inactive channel identifier */
    Ar_AGCNotEnabled = 3      /* AGC disabled on both A and B sides */
} GPAK_AGCReadStat_t;

// Tx CID Payload status codes
typedef enum GPAK_TxCIDPayloadStat_t {
    Scp_Success = 0,           /* Cid Tx Payload Transfer successful  */
    Scp_InvalidChannel = 1,    /* invalid channel identifier */
    Scp_InactiveChannel = 2,   /* inactive channel identifier */
    Scp_TxCidNotEnabled = 3,   /* TxCID disabled on selected Device side */
    Scp_TxPayloadTooBig = 4    /* TxCID payload too large for DSP Msg Buffer */
} GPAK_TxCIDPayloadStat_t;

// Arbitrary tone detector status codes
typedef enum GPAK_ConfigArbToneStat_t {
    Cat_Success = 0,           /* Arb Detector Configuration successful  */
    Cat_InvalidID = 1,         /* Invalid Arb Detector Configuration Id  */
    Cat_InvalidNumFreqs = 2,   /* Invalid number of Frequencies */
    Cat_InvalidNumTones = 3,   /* Invalid number of tones */
    Cat_InvalidFreqs = 4,      /* Invalid Frequency specification */
    Cat_InvalidTones = 5       /* Invalid tone specification */
} GPAK_ConfigArbToneStat_t;

// Arbitrary tone detector status codes
typedef enum GPAK_ActiveArbToneStat_t {
    Aat_Success = 0,           /* Arb Detector Configuration successful  */
    Aat_InvalidID = 1,         /* Invalid Arb Detector Configuration Id  */
    Aat_NotConfigured = 2      /* The detector has not been configured  */
} GPAK_ActiveArbToneStat_t;
#endif  /* prevent multiple inclusion */
