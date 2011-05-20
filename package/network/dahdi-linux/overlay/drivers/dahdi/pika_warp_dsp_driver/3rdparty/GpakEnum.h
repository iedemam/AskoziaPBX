/*
 * Copyright (c) 2002 - 2007, Adaptive Digital Technologies, Inc.
 *
 * File Name: Gpakenum.h
 *
 * Description:
 *   This file contains common enumerations related to G.PAK application
 *   software.  It is used by both the DSP and host interface software.
 *
 * Version: 1.0
 *
 * Revision History:
 *   11/07/01 - Initial release.
 *   07/03/02 - Added test modes.
 *   06/15/04 - Tone type updates.
 *   04/2005  - Added GSM_EFR
 *    1/2007  - Combined C54 and C64
 */

#ifndef _GPAKENUM_H  /* prevent multiple inclusion */
#define _GPAKENUM_H

#include "adt_typedef.h"

/* G.PAK Serial Port Ids. */

//  NOTE: DSP code requires SerialPort1-3 to be set to 0-2
typedef enum GpakSerialPort_t {
    SerialPortNull = 0xffu,     /* null serial port */
    SerialPort1 = 0,        /* first PCM serial stream port */
    SerialPort2 = 1,        /* second PCM serial stream port */
    SerialPort3 = 2,        /* third PCM serial stream port */
    McBSPPort0 = 0,
    McBSPPort1 = 1,
    McBSPPort2 = 2,
    McASPPort0 = 0,
    McASPPort1 = 1,
    AudioPort1 = 3,
    AudioPort2 = 4
} GpakSerialPort_t;

/* G.PAK serial port Slot Configuration selection codes. */
typedef enum GpakSlotCfg_t {
    SlotCfgAllActive = 0,     /* all time slots used */
    SlotCfgMultiFloat = 1,    /* inactive float */
    SlotCfgMultiHigh  = 3,    /* inactive high */
    SlotCfgNone = 4           /* no time slots used */
} GpakSlotCfg_t;

/* G.PAK serial port Companding Mode codes. */
typedef enum GpakCompandModes { 
    cmpPCMU=0,              /* u-Law */
    cmpPCMA=1,              /* A-Law */
    cmpNone16=2,            /* Linear 16 */
    cmpL8PcmU=3,            /* sw U-law, voice / data */
    cmpL8PcmA=4,            /* sw A-law, voice / data */
    cmpNone8=5              /* none, 8-bit word size */
} GpakCompandModes;

typedef enum fsPolarity {
    fsActLow=0,      // active low frame sync polarity
    fsActHigh=1      // active high frame sync polarity
} fsPolarity;

typedef enum clkEdge {
    clkRising=0,      // data clocked (or framesync sampled) on rising edge of clock signal
    clkFalling=1      // data clocked (or framesync sampled) on falling edge of clock signal
} clkEdge;

typedef enum clkMode {
    clkDoubleRate=0,     // clock signal is a double rate clock (2 sample clock edges per data bit)
    clkSingleRate=1      // clock signal is a single rate clock (1 sample clock edge per data bit)
} clkMode;

typedef enum dataRate {
    rate_8Mbps=0,        // 8 Mb per second option  (all 8 links are active, each link runs at 8 Mbps)
    rate_16Mbps=1,       // 16 Mb per second option (only 4 links are active, each link runs at 16 Mbps)
    rate_32Mbps=2        // 32 Mb per second option (only 2 links are active, each link runs at 32 Mbps)
} dataRate;

typedef enum clkFsSrc {
    clkFsA=0,           // clock and frame sync source are the A-pins
    clkFsB=1            // clock and frame sync source are the B-pins
} clkFsSrc;

typedef enum txDisState {
    highz=0,   // Slot is driven by another TDM device.
    low=1,     // Slot is driven low.
    high=2     // Slot is driven high.
} txDisState;

typedef enum loopbackMode {
    noLoopback=0,           // loopback mode disabled
    internalLoopback=1,     // internal looback enabled (SUI Tx out to SUI Rx in)
    externalLoopback=2      // external loopback enabled (SUI Rx in to SUI Tx out)
} loopbackMode;

/* G.PAK Channel Type codes. */
typedef enum GpakChanType {
    inactive=0,             /* channel inactive */
    pcmToPacket=1,          /* PCM to Packet */
    pcmToPcm=2,             /* PCM to PCM */
    packetToPacket=3,       /* Packet to Packet */
    circuitData=4,          /* Circuit Data */
    conferencePcm=5,        /* Conference PCM */
    conferencePacket=6,     /* Conference Packet */
    conferenceComposite=7,  /* Conference Composite */
    loopbackCoder=8,        /* Loopback Coder */
    conferenceMultiRate=9   /* Packet to Conference (mismatched rates) */
} GpakChanType;


/* G.PAK Active/Inactive selection codes. */
typedef enum GpakActivation {
    Disabled=0,             /* Inactive */
    Enabled=1               /* Active */
} GpakActivation;

/* G.PAK Tone types. */
typedef ADT_UInt16 GpakToneTypes;   /* tone type flags */
#define Null_tone         0u   /* no tone detection or relay */
#define DTMF_tone     0x0001u 
#define MFR1_tone     0x0002u 
#define MFR2Fwd_tone  0x0004u // MFR2-Forward detection enable
#define MFR2Back_tone 0x0008u // MFR2-Reverse detection enable
#define CallProg_tone 0x0010u 
#define Tone_Relay    0x0020u 
#define Tone_Regen    0x0040u
#define Tone_Squelch  0x0080u
#define Tone_Generate 0x0100u
#define FAX_tone      0x0200u   // FAX tones (CED/CNG)
#define Notify_Host   0x0400u   // Notify host of any detected tones
#define Arbit_tone    0x0800u   // Arbitrary tone detector
#define CED_tone      FAX_tone  // Detect CED tones
#define CNG_tone      FAX_tone  // Detect CNG tones
#define ToneDetect (DTMF_tone | MFR1_tone | MFR2Fwd_tone | MFR2Back_tone | CallProg_tone)
#define FaxDetect  (CED_tone | CNG_tone)

/* G.PAK Frame Sizes. */
typedef enum GpakFrameSizes {
    FrameSize_Unknown=0,
    FrameSize_1_ms=8,       /* 1 msec = 8 samples */
    FrameSize_2_5_ms=20,    /* 2.5 msec = 20 samples */
    FrameSize_5_ms=40,      /* 5 msec = 40 samples */
    FrameSize_10_ms=80,     /* 10 msec = 80 samples */
    FrameSize_20_ms=160,    /* 20 msec = 160 samples */
    FrameSize_22_5_ms=180,  /* 22.5 msec = 180 samples */
    FrameSize_30_ms=240,    /* 30 msec = 240 samples */

    FrameSize_1_0ms=8,       /* 1 msec = 8 samples */
    FrameSize_2_5ms=20,    /* 2.5 msec = 20 samples */
    FrameSize_5_0ms=40,      /* 5 msec = 40 samples */
    FrameSize_10_0ms=80,     /* 10 msec = 80 samples */
    FrameSize_20_0ms=160,    /* 20 msec = 160 samples */
    FrameSize_22_5ms=180,  /* 22.5 msec = 180 samples */
    FrameSize_30_0ms=240     /* 30 msec = 240 samples */
} GpakFrameSizes;

/* G.PAK Packet Profile codes. */
typedef enum GpakProfiles {
    RTPAVP=0,               /* RTP Audio/Video Profile */
    AAL2Trunking=1          /* AAL2 Trunking */
} GpakProfiles;

/* G.PAK Packet Payload Classes. */
typedef enum GpakPayloadClass {
    PayClassNone=0,       /* null payload */
    PayClassAudio,        /* GpakCodecs payload */
    PayClassSilence,      /* SID payload */
    PayClassTone,         /* GpakRTPPkts or GpakAAL2Pkts payload */
    PayClassGpak,         /* GpakEvents payload */
    PayClassG729B,        /* G729 Silence frame */
    PayClassT38,          /* T38 payload */
    PayClassStartTone,    /* Start of tone packet */
    PayClassEndTone       /* End of tone packet */
} GpakPayloadClass;

/* G.PAK Audio Class (Codec) payload type codes. */
typedef enum GpakCodecs {
    PCMU_64=0,     /* 64 kbps u-Law PCM */
    PCMA_64=1,     /* 64 kbps A-Law PCM */
    L8=2,          /* 8 bit linear */
    L16=3,         /* 16 bit linear */
    G722_64=4,     /* 64 kbps G.722 */
    G722_56=5,     /* 56 kbps G.722 */
    G722_48=6,     /* 48 kbps G.722 */
    G723_63=7,     /* 6.3 kbps G.723 */
    G723_53=8,     /* 5.3 kbps G.723 */
    G723A=9,       /* G.723A */
    G726_40=10,    /* 40 kbps G.726 */
    G726_32=11,    /* 32 kbps G.726 */
    G726_24=12,    /* 24 kbps G.726 */
    G726_16=13,    /* 16 kbps G.726 */
    G728_16=14,    /* 16 kbps G.728 */
    G728_12=15,    /* 12.8 kbps G.728 */
    G728_9=16,     /* 9.6 kbps G.728 */
    G729=17,       /* G.729 */
    G729AB=18,     /* G.729AB */
    G729D=19,      /* G.729D */
    G729E=20,      /* G.729E */
    GSM=21,        /* GSM */
    CircuitPkt=22, /* Circuit Data */
    PCMU_56=23,    /* 56 kbps u-Law PCM */
    PCMU_48=24,    /* 48 kbps u-Law PCM */
    PCMA_56=25,    /* 56 kbps A-Law PCM */
    PCMA_48=26,    /* 48 kbps A-Law PCM */
    MELP=27,       /* 2.4 kbps MELP */
    AMR_BASE=100,  // AMR Base
    AMR_475=100,   //     4.75
    AMR_515=101,   //     5.15
    AMR_590=102,   //     5.9
    AMR_670=103,   //     6.7
    AMR_740=104,   //     7.4
    AMR_795=105,   //     7.95
    AMR_1020=106,  //    10.2
    AMR_1220=107,  //    12.2
    ADT_4800=250,  /* ADT-4800 */
    DefaultCodec = 253,  // For RTP assist
    CustomCodec =254,
    NullCodec=255  /* no Codec selected */
} GpakCodecs;

/* RTP's Tone Class payload type codes. */
typedef enum GpakRTPPkts {
    EventPkt=128,           /* Event packet */
    TonePkt=129,             /* Tone packet */
    NoTonePkt=255           /* No tone packets */
} GpakRTPPkts;

/* AAL2's payload type codes. */
typedef enum GpakAAL2Pkts {
    StatePkt=256,
    DialedDigitsPkt=257,    /* Dialed Digits packet */
    CASPkt=258,
    T30_PreamPkt=259,
    EPTPkt=260,
    TrainingPkt=261,
    FaxIdlePkt=262,
    T30_DataPkt=263,
    FaxImagePkt=264
} GpakAAL2Pkts;

/* G.PAK Class payload type codes. */
typedef enum GpakEvents {
    toneDetected=0,         /* A DTMF/MF tone has been detected */
    packetWriteOverflow=1,  /* Not enough room to write in packet output buffer */
    packetReadUnderflow=2,  /* Not enough data in packet input buffer */
    invalidToneDetected=3,  /* An invalid tone was detected */
    invalidPacketHeader=4   /* Invalid packet header received */
} GpakEvents;

/* G.PAK Test Mode codes. */
typedef enum GpakTestMode {
    // 0 - 99 User defined tests
    
    // Tests performed at DSP
    DisableTest=100,    /* Test Mode disabled (normal operation) */
    SyncPcmInput,       /* synchronized PCM input data */
    TogglePCMInTone,
    TogglePCMOutTone,
    StackFreeAtBottom,
    StackFreeAtTop,
    AddressValue,
    PacketLoopBack,     // Loopback packets on single channel
    PacketCrossOver,    // Cross over packets between two channels
    UserDefined,
    InternalChannelLink,
    PacketChain,        // Chain packets with adjacent channels
    TdmFixedValue,      // source a fixed value onto a TDM timeslot
    TdmLoopback,        // put a serial port into loopback mode
 
    HostTest=200,      // Tests performed at host
    ToggleG711Tone,
    PacketLoss,
    ParameterRange, 
    UnconfiguredOptions,
    DumpFaxBuffers,
    CIDSendMsg,         // send a debug CID message
    UploadEcState,      // Upload Ec State from DSP
    DownloadEcState,    // Download Ec State to DSP
    StartEcCapt,        // Start Ec Capture
    AecControl,
    CustomVarLen        // Custom, variable-length gpak message
} GpakTestMode;

/* G.PAK T38 Fax Mode */
typedef enum GpakFaxMode_t {
    disabled=0,
    faxOnly,
    faxVoice
} GpakFaxMode_t;

/* G.PAK T.38 Fax Transport */
typedef enum GpakFaxTransport_t {
    faxUdptl=0,
    faxRtp,
    faxTcp,
    faxT38Only
} GpakFaxTransport_t;

// Tone values
typedef enum GpakToneCodes_t {
    tdsDtmfDigit1   = 0,     /* DTMF Digit 1 */
    tdsDtmfDigit2   = 1,     /* DTMF Digit 2 */
    tdsDtmfDigit3   = 2,     /* DTMF Digit 3 */
    tdsDtmfDigitA   = 3,     /* DTMF Digit A */
    tdsDtmfDigit4   = 4,     /* DTMF Digit 4 */
    tdsDtmfDigit5   = 5,     /* DTMF Digit 5 */
    tdsDtmfDigit6   = 6,     /* DTMF Digit 6 */
    tdsDtmfDigitB   = 7,     /* DTMF Digit B */
    tdsDtmfDigit7   = 8,     /* DTMF Digit 7 */
    tdsDtmfDigit8   = 9,     /* DTMF Digit 8 */
    tdsDtmfDigit9   = 10,    /* DTMF Digit 9 */
    tdsDtmfDigitC   = 11,    /* DTMF Digit C */
    tdsDtmfDigitSt  = 12,    /* DTMF Digit * */
    tdsDtmfDigit0   = 13,    /* DTMF Digit 0 */
    tdsDtmfDigitPnd = 14,    /* DTMF Digit # */
    tdsDtmfDigitD   = 15,    /* DTMF Digit D */

    tdsMfr1Digit0   = 20,    /* MFR1 Digit 1            */
    tdsMfr1Digit1   = 21,    /* MFR1 Digit 2            */
    tdsMfr1Digit3   = 22,    /* MFR1 Digit 4            */
    tdsMfr1Digit6   = 23,    /* MFR1 Digit 7            */
    tdsMfr1Code10   = 24,    /* MFR1 Code 11 Operator   */
    tdsMfr1Digit2   = 25,    /* MFR1 Digit 3            */
    tdsMfr1Digit4   = 26,    /* MFR1 Digit 5            */
    tdsMfr1Digit7   = 27,    /* MFR1 Digit 8            */
    tdsMfr1Code11   = 28,    /* MFR1 Code 10 Operator   */
    tdsMfr1Digit5   = 29,    /* MFR1 Digit 6            */
    tdsMfr1Digit8   = 30,    /* MFR1 Digit 9            */
    tdsMfr1KPTermSt = 31,    /* MFR1 KP Terminal Start  */
    tdsMfr1Digit9   = 32,    /* MFR1 Digit 0            */
    tdsMfr1KPTranTr = 33,    /* MFR1 KP Transit Traffic */
    tdsMfr1STEndPls = 34,    /* MFR1 ST End Of Pulsing  */

    tdsMfr2Fwd1   = 40,      /* MFR2 Forward Digit 1    */
    tdsMfr2Fwd2   = 41,      /* MFR2 Forward Digit 2    */
    tdsMfr2Fwd4   = 42,      /* MFR2 Forward Digit 4    */
    tdsMfr2Fwd7   = 43,      /* MFR2 Forward Digit 7    */
    tdsMfr2Fwd11  = 44,      /* MFR2 Forward Digit 11   */
    tdsMfr2Fwd3   = 45,      /* MFR2 Forward Digit 3    */
    tdsMfr2Fwd5   = 46,      /* MFR2 Forward Digit 5    */
    tdsMfr2Fwd8   = 47,      /* MFR2 Forward Digit 8    */
    tdsMfr2Fwd12  = 48,      /* MFR2 Forward Digit 12   */
    tdsMfr2Fwd6   = 49,      /* MFR2 Forward Digit 6    */
    tdsMfr2Fwd9   = 50,      /* MFR2 Forward Digit 9    */
    tdsMfr2Fwd13  = 51,      /* MFR2 Forward Digit 13   */
    tdsMfr2Fwd10  = 52,      /* MFR2 Forward Digit 10   */
    tdsMfr2Fwd14  = 53,      /* MFR2 Forward Digit 14   */
    tdsMfr2Fwd15  = 54,      /* MFR2 Forward Digit 15   */

    tdsMfr2Bck1   = 60,      /* MFR2 Backward Digit 1   */
    tdsMfr2Bck2   = 61,      /* MFR2 Backward Digit 2   */
    tdsMfr2Bck4   = 62,      /* MFR2 Backward Digit 4   */
    tdsMfr2Bck7   = 63,      /* MFR2 Backward Digit 7   */
    tdsMfr2Bck11  = 64,      /* MFR2 Backward Digit 11  */
    tdsMfr2Bck3   = 65,      /* MFR2 Backward Digit 3   */
    tdsMfr2Bck5   = 66,      /* MFR2 Backward Digit 5   */
    tdsMfr2Bck8   = 67,      /* MFR2 Backward Digit 8   */
    tdsMfr2Bck12  = 68,      /* MFR2 Backward Digit 12  */
    tdsMfr2Bck6   = 69,      /* MFR2 Backward Digit 6   */
    tdsMfr2Bck9   = 70,      /* MFR2 Backward Digit 9   */
    tdsMfr2Bck13  = 71,      /* MFR2 Backward Digit 13  */
    tdsMfr2Bck10  = 72,      /* MFR2 Backward Digit 10  */
    tdsMfr2Bck14  = 73,      /* MFR2 Backward Digit 14  */
    tdsMfr2Bck15  = 74,      /* MFR2 Backward Digit 15  */

    tdsCprgDialTone     = 80, /* Call progress Dial Tone */
    tdsCprgBusySignal   = 81, /* Call progress Busy */
    tdsCprgAudibleRing  = 82, /* Call progress Ring */
    tdsCprgIntercept    = 83, /* Call progress Intercept */
    tdsCprgCallWaiting  = 84, /* Call progress Call Waiting */

    tdsG164   = 90, /* G.164 2100 Hz */
    tdsG165   = 91, /* G.165 2100 Hz with phase reversals */
    tdsFaxCng = 92, /* 1100 Hz Fax CNG tone */
    
    tdsNoToneDetected = 100,  /* No Tone Detected */
    tdsNoChange = 101
} GpakToneCodes_t;

typedef enum GpakDeviceSide_t {
    ADevice,           // A device side
    BDevice            // B device side
} GpakDeviceSide_t;

//  Event messages
typedef enum GpakAsyncEventCode_t {
    EventDspReady = 0,              // Dsp out of reset and ready
    EventToneDetect = 1,            // Tone detection event
    EventRecordingStopped = 2,      // Voice Recording Stopped
    EventRecordingBufferFull = 3,   // Voice Record Buffer is Full
    EventPlaybackComplete = 4,      // Voice Playback is Complete
    EventWarning = 5,               // Warning Message
    
    EventFaxComplete = 6,
    EventCaptureComplete = 7,

    EventVadStateVoice = 8,             // Vad state Voice event
    EventVadStateSilence = 9,           // Vad state Silence event
    EventTxCidMessageComplete = 10,     // Cid Tx message complete event
    EventRxCidCarrierLock = 11,         // Cid Rx carrier lock event
    EventRxCidMessageComplete = 12,     // Cid Rx message complete event
    EventLoopbackTeardownComplete = 13, // Loopback coder teardown complete event
    EventDSPDebug = 14                  // DSP debug data event
} GpakAsyncEventCode_t;

typedef enum GpakWarning_t {
   WarnRxDmaSlip = 0,         // DMA out of sync with copy
   WarnTxDmaSlip = 1,         // DMA out of sync with copy
   WarnPortRestart = 2,       // Port restart
   WarnFrameSyncError = 3,    // Frame sync error
   WarnFrameTiming = 4,       // Frame took too long to process
   WarnFrameBuffers = 5       // Frame buffer realignment
} GpakWarning_t;

/* Tone generation types */
typedef enum GpakToneGenType_t {
    TgCadence = 0,      // Cadence tone
    TgContinuous = 1,   // Continuous tone
    TgBurst = 2         // Burst tone
} GpakToneGenType_t;

typedef enum GpakToneGenCmd_t {
    ToneGenStart = 0,   // Start tone generation 
    ToneGenStop = 1     // Stop tone generation 
} GpakToneGenCmd_t;

typedef enum GpakPlayRecordMode_t {
    RecordingL16,
    RecordingAlaw,
    RecordingUlaw
} GpakPlayRecordMode_t;


//  Playback-record
typedef enum GpakPlayRecordCmd_t {
    StartPlayBack,
    StartRecording,
    StopRecording
} GpakPlayRecordCmd_t;

typedef enum GpakPlayRecordState_t {
    PlayBack,
    Record,
    Inactive
} GpakPlayRecordState_t;


typedef enum { PK_None, PK_LSB, PK_MSB, PK_AMR, PK_CUSTOM 
} packFormat;


// NOTE:  Enabled must have least significant bit set to 1
typedef enum GpakVADMode { VadDisabled = 0, VadCodecEnabled=1, VadCngEnabled=3
} GpakVADMode;

/* Caller ID FSK Type */
typedef enum { 
    fskV23,           // V23
    fskBell202        // Bell 202
} GpakFskType;

/* G.PAK Caller Id Modes */
typedef enum
{
    CidDisabled = 0,        // Caller Id Disabled
    CidReceive = 1,         // Caller Id Receive Mode
    CidTransmit = 2         // Caller Id Transmit Mode
} GpakCidMode_t;

/* G.PAK Algorithm control commands */
typedef enum
{
    EnableEcan          = 0,    // Enable echo canceller
    BypassEcan          = 1,    // Bypass echo canceller
    ResetEcan           = 2,    // Reset echo canceller
    EnableVad           = 3,    // Enable Vad
    BypassVad           = 4,    // Bypass Vad
    EnableVAGC          = 5,    // Enable VAGC
    BypassVAGC          = 6,    // Bypass VAGC
    ModifyToneDetector  = 7,    // Modify Tone Detector Type
    EnableToneSuppress  = 8,    // Enable Tone Suppression
    BypassToneSuppress  = 9,    // Bypass Tone Suppression
    EnableEcanAdapt     = 10,   // Enable echo canceller's adaptation
    BypassEcanAdapt     = 11,   // Bypass echo canceller's adaptation 
    UpdateEcanNLP       = 12,   // Update echo canceller's NLP
    UpdateTdmOutGain    = 13,   // Tdm output Gain adjust
    UpdateTdmInGain     = 14    // Tdm input Gain adjust
} GpakAlgCtrl_t;
        
#endif  /* prevent multiple inclusion */
