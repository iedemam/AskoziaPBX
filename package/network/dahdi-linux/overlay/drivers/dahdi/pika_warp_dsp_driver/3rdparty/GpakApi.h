/*
 * Copyright (c) 2001 - 2007, Adaptive Digital Technologies, Inc.
 *
 * File Name: GpakApi.h
 *
 * Description:
 *   This file contains the function prototypes and data types for the user
 *   API functions that communicate with DSPs executing G.PAK software. The
 *   file is used by application software in the host processor connected to
 *   G.PAK DSPs via a Host Port Interface.
 *
 * Version: 2.0
 *
 * Revision History:
 *   10/17/01 - Initial release.
 *   07/03/02 - Conferencing and test mode updates.
 *   06/15/04 - Tone type updates.
 *   1/2007   - Combined C54 and C64
 */

#ifndef _GPAKAPI_H  /* prevent multiple inclusion */
#define _GPAKAPI_H

#include "adt_typedef.h"
#include "GpakEnum.h"
#include "GpakErrs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define API_VERSION_ID      0x0510u     // G.PAK API Version Id
#define MIN_DSP_VERSION_ID  0x0402u     // the minimum G.PAK DSP Version Id that is fully compatible with the current API code

#define MAX_CIDPAYLOAD_I8 256           // Defined by MAX_MESSAGE_BYTES of ADT_CIDUser.h 



typedef void *GPAK_FILE_ID;  /* G.PAK Download file identifier */


extern GPAK_DspCommStat_t DSPError[];



int VerifyReply (ADT_UInt16 *pMsgBufr, ADT_UInt32 CheckMode, ADT_UInt16 CheckValue);

//}===============================================================
//  Predefined host<->DSP interface addresses and offsets
//{===============================================================
//
// Address at location defined by DSP_IFBLK_ADDRESS points to interface structure 
//  The host must first read the address of the interface structure from the
//  well defined location specified by DSP_IFBL_ADDRESS.  Access to individual
//  interface structure elements can then be made by adding the approriate offsets
//  to this address

//  C54xx processors (C5502, C5510, ...) should define DSP_TYPE as 54
//  C64xx processors (C6412, C6416. DM642, ...) should define DSP_TYPE as 64
//  C64+ processors (C6424, C6437, and C6452) should define DSP_TYPE as 6424, 6437, or 6452
//
#if (DSP_TYPE == 64 || DSP_TYPE == 6424  || DSP_TYPE == 6437 || DSP_TYPE == 6452 || DSP_TYPE == 3530)

   // Identification of host interface block address
   #if (DSP_TYPE == 64)
       #define DSP_IFBLK_ADDRESS 0x0200
   #elif (DSP_TYPE == 6424 || DSP_TYPE == 6437)
       #define DSP_IFBLK_ADDRESS 0x10800000
   #elif (DSP_TYPE == 6452)
       #define DSP_IFBLK_ADDRESS 0x00A00000
    #elif (DSP_TYPE == 3530)
       #define DSP_IFBLK_ADDRESS 0x10f04000
   #endif
   #define DSP_WORD_SIZE_I8 4 // in bytes
   #define DSP_ADDR_SIZE_I8 4 // in bytes
   #define DSP_DLADDR_SIZE_I8 4 // in bytes
   // C64 addressing consists of 1 bytes per MAU.  Offsets are in bytes, 4 bytes per transfer
   #define CircAddress(base,wds)  (base + (wds)*2)      
   #define CircTxSize(wds)        ((((wds) + 1) / 2))  // Round up to determine transfer units required
   #define CircAvail(wds)          (wds)               // Round down to determine transfer units available
   #define BytesToCircUnits(bytes) ((((bytes) + 3) / 4) * 2) // Round bytes up to 32-bit transfers
   #define BytesToTxUnits(bytes)   (((bytes) + 3) / 4) // Round bytes up to 32-bit transfers
   #define BytesToMAUs(bytes)      (bytes)


   #define CMD_MSG_PNTR_OFFSET     0   /* Command Msg Pointer */
   #define REPLY_MSG_PNTR_OFFSET   4   /* Reply Msg Pointer */
   #define EVENT_MSG_PNTR_OFFSET   8   /* Event Msg Pointer */
   #define PKT_BUFR_MEM_OFFSET    12   /* Packet Buffer memory */
   #define DSP_STATUS_OFFSET      16   /* DSP Status */
   #define VERSION_ID_OFFSET      20   /* G.PAK Dsp Version Id */
   #define MAX_CMD_MSG_LEN_OFFSET 24   /* Max Cmd Msg Length */
   #define CMD_MSG_LEN_OFFSET     28   /* Command Msg Length */
   #define REPLY_MSG_LEN_OFFSET   32   /* Reply Msg Length */
   #define EVENT_MSG_LEN_OFFSET   36   /* Event Msg Length */
   #define NUM_CHANNELS_OFFSET    40   /* Num Built Channels */
   #define CPU_USAGE_OFFSET       44   /* CPU Usage statistics */
   #define RTP_CLOCK_OFFSET       72   /* RTP Clock */
   #define API_VERSION_ID_OFFSET  76   /* API version Id */
   #define CPU_PKUSAGE_OFFSET     80   /* CPU Peak Usage statistics */


   #define DSP_INIT_STATUS  0x55555555UL   /* DSP Initialized status value */
   #define HOST_INIT_STATUS 0xAAAAAAAAUL	/* Host Initialized status value */

   typedef ADT_UInt32 DSP_Word;            // Size of DSP word
   typedef ADT_UInt32 DSP_Address;         // Size of DSP address pointer

#ifdef DEBUG_XFER
   #define gpakReadDsp(d,a,s,b)  {                                          \
      DEBUG_XFER (" Read[%d]: %8x:%3d  ", d, a, s);                         \
      gpakReadDspMemory32  (d, (DSP_Address) (a), s, b);                    \
      DEBUG_XFER ("%8x  %8x  %8x\n", (b)[0], (b)[1], (b)[2]);               \
   }
   #define gpakReadDsp32(d,a,s,b)  {                                        \
      DEBUG_XFER (" Read[%d]: %8x:%3d  ", d, a, s);                         \
      gpakReadDspMemory32  (d, (DSP_Address) (a), s, b);                    \
      DEBUG_XFER ("%8x  %8x  %8x\n", (b)[0], (b)[1], (b)[2]);               \
   }
   #define gpakReadDspNoSwap(d,a,s,b)  {                                    \
      DEBUG_XFER (" Read[%d]: %8x:%3d  ", d, a, s);                         \
      gpakReadDspNoSwap32  (d, (DSP_Address) (a), s, b);                    \
      DEBUG_XFER ("%8x  %8x  %8x\n", (b)[0], (b)[1], (b)[2]);               \
   }
 

   #define gpakWriteDsp(d,a,s,b) {                                            \
      DEBUG_XFER ("Write[%d]: %8x:%3d  %8x  %8x  %8x\n", d, a, s, (b)[0], (b)[1], (b)[2]); \
      gpakWriteDspMemory32 (d, (DSP_Address) (a), s, b);                     \
   }
   #define gpakWriteDspNoSwap(d,a,s,b) {                                            \
      DEBUG_XFER ("Write[%d]: %8x:%3d  %8x  %8x  %8x\n", d, a, s, (b)[0], (b)[1], (b)[2]); \
      gpakWriteDspNoSwap32 (d, (DSP_Address) (a), s, b);                     \
   }
#else
   #define gpakReadDsp(d,a,s,b)        gpakReadDspMemory32  (d, (DSP_Address) (a), s, b)
   #define gpakReadDsp32(d,a,s,b)      gpakReadDspMemory32  (d, (DSP_Address) (a), s, b)
   #define gpakWriteDsp(d,a,s,b)       gpakWriteDspMemory32 (d, (DSP_Address) (a), s, b)
   #define gpakReadDspNoSwap(d,a,s,b)  gpakReadDspNoSwap32  (d, (DSP_Address) (a), s, b)
   #define gpakWriteDspNoSwap(d,a,s,b) gpakWriteDspNoSwap32 (d, (DSP_Address) (a), s, b)
#endif


   #define MAX_DSP_ADDRESS 0xF0000000l

   #define DL_HDR_SIZE 7

   typedef struct circBufr_t {
      DSP_Word BufrBase;
      DSP_Word BufrSize;
      DSP_Word PutIndex;
      DSP_Word TakeIndex;
      DSP_Word Reserved;
   } CircBufr_t;
   #define CB_BUFR_PUT_INDEX       8   /* addressing offset for circular buffer */
   #define CB_BUFR_TAKE_INDEX     12   /* addressing offset for circular buffer */
/* Circular packet buffer information structure offsets. */
   #define CB_INFO_STRUCT_LEN 5       /* circbuffer info struct 4 words  */

#elif (DSP_TYPE == 54) 
   #define DSP_WORD_SIZE_I8 2 // in bytes
   #define DSP_ADDR_SIZE_I8 2 // in bytes
   #define DSP_DLADDR_SIZE_I8 3 // in bytes
   // C54 addressing consists of 2 bytes per MAU. Offsets are in 16-bit words
   #define CircAddress(base,wds)   (base + wds)
   #define CircTxSize(wds)         (wds)
   #define CircAvail(wds)          (wds)
   #define BytesToCircUnits(bytes) ((bytes + 1) / 2)  // Round bytes up to 16-bit transfers
   #define BytesToTxUnits(bytes)   ((bytes + 1) / 2)  // Round bytes up to 16-bit transfers
   #define BytesToMAUs(bytes)      ((bytes + 1) / 2)

   #define DSP_IFBLK_ADDRESS 0x0100 

   #define CMD_MSG_PNTR_OFFSET     0   /* Command Msg Pointer */
   #define REPLY_MSG_PNTR_OFFSET   1   /* Reply Msg Pointer */
   #define EVENT_MSG_PNTR_OFFSET   2   /* Event Msg Pointer */
   #define PKT_BUFR_MEM_OFFSET     3   /* Packet Buffer memory */
   #define DSP_STATUS_OFFSET       4   /* DSP Status */
   #define VERSION_ID_OFFSET       5   /* G.PAK Version Id */
   #define MAX_CMD_MSG_LEN_OFFSET  6   /* Max Cmd Msg Length */
   #define CMD_MSG_LEN_OFFSET      7   /* Command Msg Length */
   #define REPLY_MSG_LEN_OFFSET    8   /* Reply Msg Length */
   #define EVENT_MSG_LEN_OFFSET    9   /* Event Msg Length */
   #define NUM_CHANNELS_OFFSET    10   /* Num Built Channels */
   #define CPU_USAGE_OFFSET       11   /* CPU Usage statistics */
   #define RTP_CLOCK_OFFSET       18   /* RTP Clock */
   #define API_VERSION_ID_OFFSET  19   /* API version Id */
   #define CPU_PKUSAGE_OFFSET     20   /* CPU Peak Usage statistics */

   #define DSP_INIT_STATUS  0x5555U    /* DSP Initialized status value */
   #define HOST_INIT_STATUS 0xAAAAU	   /* Host Initialized status value */

   typedef ADT_UInt16 DSP_Word;        // Size of DSP word
   typedef ADT_UInt16 DSP_Address;     // Size of DSP address pointer

   #define gpakReadDsp(d,a,s,b)        gpakReadDspMemory16  (d, (DSP_Address) (a), s, b)
   #define gpakWriteDsp(d,a,s,b)       gpakWriteDspMemory16 (d, (DSP_Address) (a), s, b)
   #define gpakReadDspNoSwap(d,a,s,b)  gpakReadDspNoSwap16  (d, (DSP_Address) (a), s, b)
   #define gpakWriteDspNoSwap(d,a,s,b) gpakWriteDspNoSwap16 (d, (DSP_Address) (a), s, b)
   #define gpakReadDsp32(d,a,s,b)      gpakReadDspMemory32  (d, (DSP_Address) (a), s, b)

   #define MAX_DSP_ADDRESS 0xFF00

   #define DL_HDR_SIZE 6

   typedef struct circBufr_t {
      DSP_Word BufrBase;
      DSP_Word BufrSize;
      DSP_Word PutIndex;
      DSP_Word TakeIndex;
      DSP_Word Reserved;
   } CircBufr_t;
   #define CB_BUFR_PUT_INDEX       2   /* addressing offset for circular buffer */
   #define CB_BUFR_TAKE_INDEX      3   /* addressing offset for circular buffer */
/* Circular packet buffer information structure offsets. */
   #define CB_INFO_STRUCT_LEN 5       /* circbuffer info struct 4 words  */
#elif (DSP_TYPE == 55) 
   #define DSP_WORD_SIZE_I8 2 // in bytes
   #define DSP_ADDR_SIZE_I8 4 // in bytes
   #define DSP_DLADDR_SIZE_I8 3 // in bytes

   // C55 data addressing consists of 2 bytes per MAU. Offsets are in 16-bit words
   #define CircAddress(base,wds)   (base + wds)
   #define CircTxSize(wds)         (wds)
   #define CircAvail(wds)          (wds)
   #define BytesToCircUnits(bytes) ((bytes + 1) / 2)  // Round bytes up to 16-bit transfers
   #define BytesToTxUnits(bytes)   ((bytes + 1) / 2)  // Round bytes up to 16-bit transfers
   #define BytesToMAUs(bytes)      ((bytes + 1) / 2)

   #define DSP_IFBLK_ADDRESS 0x0100 

   #define CMD_MSG_PNTR_OFFSET     0   /* Command Msg Pointer */
   #define REPLY_MSG_PNTR_OFFSET   2   /* Reply Msg Pointer */
   #define EVENT_MSG_PNTR_OFFSET   4   /* Event Msg Pointer */
   #define PKT_BUFR_MEM_OFFSET     6   /* Packet Buffer memory */
   #define DSP_STATUS_OFFSET       8   /* DSP Status */
   #define VERSION_ID_OFFSET       9   /* G.PAK Version Id */
   #define MAX_CMD_MSG_LEN_OFFSET 10   /* Max Cmd Msg Length */
   #define CMD_MSG_LEN_OFFSET     11   /* Command Msg Length */
   #define REPLY_MSG_LEN_OFFSET   12   /* Reply Msg Length */
   #define EVENT_MSG_LEN_OFFSET   13   /* Event Msg Length */
   #define NUM_CHANNELS_OFFSET    14   /* Num Built Channels */
   #define CPU_USAGE_OFFSET       15   /* CPU Usage statistics */
   #define RTP_CLOCK_OFFSET       22   /* RTP Clock */
   #define API_VERSION_ID_OFFSET  24   /* API version Id */
   #define CPU_PKUSAGE_OFFSET     25   /* CPU Peak Usage statistics */

   #define DSP_INIT_STATUS  0x5555U    /* DSP Initialized status value */
   #define HOST_INIT_STATUS 0xAAAAU	   /* Host Initialized status value */

   typedef ADT_UInt16 DSP_Word;        // Size of DSP word
   typedef ADT_UInt32 DSP_Address;     // Size of DSP address pointer

   #define gpakReadDsp(d,a,s,b)        gpakReadDspMemory16  (d, (DSP_Address) (a), s, b)
   #define gpakWriteDsp(d,a,s,b)       gpakWriteDspMemory16 (d, (DSP_Address) (a), s, b)
   #define gpakReadDspNoSwap(d,a,s,b)  gpakReadDspNoSwap16  (d, (DSP_Address) (a), s, b)
   #define gpakWriteDspNoSwap(d,a,s,b) gpakWriteDspNoSwap16 (d, (DSP_Address) (a), s, b)
   #define gpakReadDsp32(d,a,s,b)      gpakReadDspMemory32  (d, (DSP_Address) (a), s, b)

   #define MAX_DSP_ADDRESS 0x7FFF00

   #define DL_HDR_SIZE 6


   typedef struct circBufr_t {
      ADT_UInt32 BufrBase;
      DSP_Word BufrSize;
      DSP_Word PutIndex;
      DSP_Word TakeIndex;
      DSP_Word Reserved;
   } CircBufr_t;
   #define CB_BUFR_PUT_INDEX       3   /* addressing offset for circular buffer */
   #define CB_BUFR_TAKE_INDEX      4   /* addressing offset for circular buffer */
/* Circular packet buffer information structure offsets. */
   #define CB_INFO_STRUCT_LEN 6       /* circbuffer info struct 6 words  */
#else

   #error DSP_TYPE must be defined as 54, 55, 64, 6424, 6437, 6452, or 3530

#endif

#if (DSP_ADDR_SIZE_I8 == 4)
   #define gpakReadDspAddr(d,a,s,b)        gpakReadDspMemory32(d, (DSP_Address) (a), s, b)
#elif (DSP_ADDR_SIZE_I8 == 2)
   #define gpakReadDspAddr(d,a,s,b)        gpakReadDspMemory16(d, (DSP_Address) (a), s, b)
#else
   #error DSP_ADDR_SIZE_I8 is not defined as 4, or 2
#endif

extern unsigned int MaxDspCores;
extern unsigned int MaxChannelAlloc;
extern unsigned int MaxWaitLoops;
extern unsigned int DownloadI8;
extern unsigned int HostBigEndian;

extern DSP_Word    MaxChannels[];  /* max num channels */
extern DSP_Word    MaxCmdMsgLen[];  /* max Cmd msg length (octets) */

extern DSP_Address pDspIfBlk[];          /* DSP address of interface blocks */
extern DSP_Address pEventFifoAddress[];  /* Event circular buffers */

/* Download buffers */
extern ADT_Word   DlByteBufr_[];     /* Dowload byte buf */
extern DSP_Word   DlWordBufr[];      /* Dowload word buffer */




// Echo Canceller state variables.

/* Echo Canceller definitions used to size the GpakEcanState_t buffers */
/* Do Not modify these values: they are sized for DSP worst-case Ecan allocation */
//#define VARIANT_A         /* let this be defined for short-tail 'variant A' */
#define MAX_EC_TAP_LEN 1024  /* max tap length in sample units */
#define MAX_EC_FIR_SEGS 3    /* max number of FIR segments */
#define MAX_EC_SEG_LEN 64    /* mac FIR segment length in sample units */

#ifndef VARIANT_A
#define EC_STATE_DATA_LEN ((MAX_EC_FIR_SEGS * (2 + MAX_EC_SEG_LEN)) + (MAX_EC_TAP_LEN / 4))
#else
#define EC_STATE_DATA_LEN (MAX_EC_TAP_LEN)
#endif
/* definition of an Echo canceller State information structure */
typedef struct
{
    ADT_Int16 Size;                         // structure size
    ADT_Int16 TapLength;                    // Num Taps (tail length)
    ADT_Int16 CoefQ;                        // CoefficientQ
    ADT_Int16 BGCoefQ;                      // Background CoefficientQ
    ADT_Int16 NumFirSegments;               // Num FIR Segments
    ADT_Int16 FirSegmentLen;                // FIR Segment Length
    ADT_Int16 Data[EC_STATE_DATA_LEN];      // state data
} GpakEcanState_t;
#define ECAN_STATE_LEN_I16 (EC_STATE_DATA_LEN + 6)


/* Miscellaneous definitions. */
#define MAX_MSGLEN_I32       (5 + (ECAN_STATE_LEN_I16+1)/2)
#define MSG_BUFFER_SIZE      MAX_MSGLEN_I32         /* size of Host msg buffers (32-bit words) */
#define MSG_BUFFER_ELEMENTS  (MAX_MSGLEN_I32 * 2)   // max number of I16 elements in Msg buffer

#define EVENT_BUFFER_SIZE 144   /* size of DSP Word buffer (16-bit words) holds 16 words plus a 256 byte CID payload */

#define PackBytes(hi,lo) ((ADT_UInt16) (((hi) << 8) | ((lo) & 0xff)))
#define Byte1(a) (((a) >> 8) & 0xff)
#define Byte0(a) ((a) & 0xff)

//}===============================================================
//  Common type definitions and structures
//{===============================================================
typedef enum GpakApiStatus_t {
   GpakApiSuccess = 0,  GpakApiParmError,          GpakApiInvalidChannel,   
   GpakApiInvalidDsp,   GpakApiCommFailure,        GpakApiBufferTooSmall,
   GpakApiNoPayload,    GpakApiBufferFull,         GpakApiReadError,
   GpakApiInvalidFile,  GpakApiUnknownReturnData,  GpakApiNoQue,
   GpakApiInvalidPaylen
} GpakApiStatus_t;

typedef enum  GpakChannelTypes {
   GpakVoiceChannel = 0,
   GpakDataChannel
} GpakChannelTypes;

typedef struct pktHdr {
   ADT_UInt16 OctetsInPayload;
   ADT_UInt16 ChannelId;
   ADT_UInt16 PayloadClass;
   ADT_UInt16 PayloadType;
   ADT_UInt32 TimeStamp;
} PktHdr_t;

typedef struct pkt {
   PktHdr_t   Hdr;
   ADT_UInt8  PayloadData[240];
} Packet_t;


//}===============================================================
//    Host download of DSP image
//{===============================================================
typedef enum gpakDownloadStatus_t {   // Host return status
    GdlSuccess       = GpakApiSuccess,         /* DSP download successful */
    GdlFileReadError = GpakApiReadError,       /* error reading Download file */
    GdlInvalidFile   = GpakApiInvalidFile,     /* invalid Download file content */
    GdlInvalidDsp    = GpakApiInvalidDsp       /* invalid DSP */
} gpakDownloadStatus_t;

/*
 * gpakDownloadDsp - Download a DSP's Program and initialized Data memory.
 *
 * FUNCTION
 *  This function reads a DSP's Program and Data memory image from the
 *  specified file and writes the image to the DSP's memory.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
extern gpakDownloadStatus_t gpakDownloadDsp ( 
    ADT_UInt32 DspId,        /* DSP Identifier (0 to MaxDSPCores-1) */
    void *FileId     /* G.PAK Download File Identifier */
    );


//}===============================================================
//    System status and configuration
//{===============================================================

typedef struct GpakSystemConfig_t {
    ADT_UInt32 GpakVersionId;    /* G.PAK Version Identification */
    ADT_UInt16 MaxChannels;      /* maximum number of channels */
    ADT_UInt16 ActiveChannels;   /* number of configured channels */
    ADT_UInt32 CfgEnableFlags;   /* Configuration Enable Flags */
    GpakProfiles PacketProfile;  /* Packet Profile type */
    GpakRTPPkts  RtpTonePktType; /* RTP Tone Packet type */
    ADT_UInt16 CfgFramesChans;   /* cfgd frame sizes and chan types */


    ADT_UInt16 Port1NumSlots;   /* Port 1's time slots on stream */
    ADT_UInt16 Port2NumSlots;   /* Port 2's time slots on stream */
    ADT_UInt16 Port3NumSlots;   /* Port 3's time slots on stream */
    ADT_UInt16 Port1SupSlots;   /* Port 1's num supported time slots */
    ADT_UInt16 Port2SupSlots;   /* Port 2's num supported time slots */
    ADT_UInt16 Port3SupSlots;   /* Port 3's num supported time slots */

	// PCM Echo canceller
    ADT_UInt16 NumPcmEcans;     /* Number of PCM Echo Cancellers */
    ADT_UInt16 PcmMaxTailLen;   /* Tail length (ms) */
    ADT_UInt16 PcmMaxFirSegs;   /* max FIR segments */
    ADT_UInt16 PcmMaxFirSegLen; /* max FIR segment len (ms) */

    // PKT Echo canceller
    ADT_UInt16 NumPktEcans;     /* Number of Packet Echo Cancellers */
    ADT_UInt16 PktMaxTailLen;   /* Tail length (ms) */
    ADT_UInt16 PktMaxFirSegs;   /* max FIR segments */
    ADT_UInt16 PktMaxFirSegLen; /* max FIR segment len (ms) */

    ADT_UInt16 MaxConferences;  /* maximum number of conferences */
    ADT_UInt16 MaxNumConfChans; /* max number channels per conference */
    ADT_UInt16 SelfTestStatus;  /* Self Test status flags */
    ADT_UInt16 MaxToneDetTypes; /* Max concurrent tone detect types */
    ADT_UInt16 AECInstances;    // Max acoustic echo canceller instances
    ADT_UInt16 AECTailLen;      // Max tail length for acoustic echo canceller
} GpakSystemConfig_t;

typedef struct GpakECParams_t {
    /* PCM and Packet Echo Canceller parameters. */
    ADT_Int16 TailLength;      // Tail length (ms) 
    ADT_Int16 NlpType;         // NLP Type 
    ADT_Int16 AdaptEnable;     // Adapt Enable flag 
    ADT_Int16 G165DetEnable;   // G165 Detect Enable flag 
    ADT_Int16 DblTalkThresh;   // Double Talk threshold      (  0..18 dB) 
    ADT_Int16 NlpThreshold;    // NLP threshold              (  0..90 dB) 
    ADT_Int16 NlpUpperLimitThreshConv;  // NLP converge      (  0..90 dB)
    ADT_Int16 NlpUpperLimitThreshPreconv; // NLP preconverge (  0..90 dB)
    ADT_Int16 NlpMaxSupp;      // Max suppression            (  0..90 dB)
    ADT_Int16 CngThreshold;    // CNG Noise threshold        (-96..0 dBm)
    ADT_Int16 AdaptLimit;      // Adaption frequency          (% of framesize)
    ADT_Int16 CrossCorrLimit;  // Cross correlation frequency (% of framesize)
    ADT_Int16 NumFirSegments;  // Num FIR Segments           (1..3)
    ADT_Int16 FirSegmentLen;   // FIR Segment Length         (ms)
    ADT_Int16 FirTapCheckPeriod; // Frequency to check tails (2..1000 ms)
    ADT_Int16 MaxDoubleTalkThres; // Maximum double-talk threshold (  0..50 dB) 
    ADT_Int16 MixedFourWireMode;	// Enable 4-wire (echo-free) lines flag
    ADT_Int16 ReconvergenceCheckEnable; // Enable reconvergence checking flag
} GpakECParams_t;

typedef struct GpakAGCParms_t {
    ADT_Int16 TargetPower;       // Target Power          (-30..0  dBm) 
    ADT_Int16 LossLimit;         // Maximum Attenuation   (-23..0  dB) 
    ADT_Int16 GainLimit;         // Maximum Positive Gain (  0..23 dB)
    ADT_Int16 LowSignal;         // Low Signal leven      (-65..-20 dBm)
} GpakAGCParms_t;

typedef struct GpakCIDParms_t {
    ADT_Int16   formatBurstFlag;    // (Tx only) 1==Full formatting, 0==partial formatting
    ADT_Int16   numSeizureBytes;    // (Tx only) num seizure bytes to prepend
    ADT_Int16   numMarkBytes;       // (Tx only) num mark bytes to prepend
    ADT_Int16   numPostambleBytes;  // (Tx only) num postamble bytes to append
    GpakFskType fskType;            // (Tx and Rx) v23 or bell 202 
    ADT_Int16   fskLevel;           // (Tx only) Tx fsk level (0...-19 dBm)
    ADT_UInt16  msgType;            // (Tx only) message type field (valid if format==1)
} GpakCIDParms_t;

typedef struct GpakSystemParms_t {

    /* AGC parameters- A and B sides */
    GpakAGCParms_t AGC_A;
    GpakAGCParms_t AGC_B;

    /* VAD parameters. */
    ADT_Int16 VadNoiseFloor;        /* Noise Floor Threshold (-96..0 dBm) */
    ADT_Int16 VadHangTime;          /* Speech Hangover Time  (0..4000 msec) */
    ADT_Int16 VadWindowSize;        /* Analysis Window Size  (1..30 msec) */

    /* PCM Echo Canceller parameters. */
    GpakECParams_t PcmEc;

    /* Packet Echo Canceller parameters. */
    GpakECParams_t PktEc;

    /* Conference parameters. */
    ADT_Int16 ConfNumDominant;      /* Number of Dominant Conference Members */
    ADT_Int16 MaxConfNoiseSuppress; /* maximum Conference Noise Suppression */

    /* Caller ID parameters */
    GpakCIDParms_t Cid;
    ADT_UInt16  VadReport;          /* 1 == vad state reporting enabled */
} GpakSystemParms_t;

typedef struct GpakAsyncEventData_t {
   GpakDeviceSide_t   ToneDevice;   // Device detecting tone
   union {
      struct toneEvent {   //  For EventToneDetect
        GpakToneCodes_t    ToneCode;       // detected tone code
        ADT_UInt16         ToneDuration;   // tone duration
      } toneEvent;

      struct recordEvent {    //  For EventRecordingStopped and EventRecordingBufferFull
        ADT_UInt32         RecordLength;  // Actual length of recording
      } recordEvent;
      
      struct captureData {
         ADT_UInt32  AAddr;
         ADT_UInt32  ALenI32;       

         ADT_UInt32  BAddr;
         ADT_UInt32  BLenI32;       

         ADT_UInt32  CAddr;
         ADT_UInt32  CLenI32;       
         ADT_UInt32  DAddr;
         ADT_UInt32  DLenI32;       
         
      } captureData;

      struct rxCidEvent {
         ADT_UInt16   Length;                     // length (octets) of Rx CID payload
         ADT_UInt8    Payload[MAX_CIDPAYLOAD_I8]; // Rx CID payload data
      } rxCidEvent;

      struct dspDebugEvent {
         ADT_UInt16 DebugCode;	        // DSP-specific debug code
         ADT_UInt16 DebugLengthI16;	// length of debug data (octets)
         ADT_UInt16 DebugData[EVENT_BUFFER_SIZE - 10]; // debug data buffer
      } dspDebugEvent;
   } aux;
} GpakAsyncEventData_t;


typedef struct GpakTestData_t {
   union {
        ADT_UInt16 TestParm;  // single parameter

        /* source a fixed pattern onto a channel's tdm timeslot */
        /* Used when Test Mode is TdmFixedValue */
        struct fixedVal {
            ADT_UInt16       ChannelId;     /* Channel Id */
            GpakSerialPort_t PcmOutPort;    /* PCM Output Serial Port Id */
            ADT_UInt16       PcmOutSlot;    /* PCM Output Time Slot */
            ADT_UInt16       Value;         /* 16-bit value  */
            GpakActivation   State; 		/* activation state */
        } fixedVal;

        /* Put a Serial port into loopback mode */
        /* Used when Test Mode is TdmLoopback */
        struct tdmLoop {
            GpakSerialPort_t PcmOutPort;    /* PCM Output Serial Port Id */
            GpakActivation   State; 	    /* activation state */
        } tdmLoop;        
  } u;
} GpakTestData_t;

/*
 * gpakGetSystemConfig - Read a DSP's System Configuration.
 *
 * FUNCTION
 *  This function reads a DSP's System Configuration information.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
typedef enum gpakGetSysCfgStatus_t {   // Host return status
    GscSuccess        = GpakApiSuccess,      /* System Config read successfully */
    GscInvalidDsp     = GpakApiInvalidDsp,   /* invalid DSP */
    GscDspCommFailure = GpakApiCommFailure   /* failed to communicate with DSP */
} gpakGetSysCfgStatus_t;

extern gpakGetSysCfgStatus_t gpakGetSystemConfig (
    ADT_UInt32 DspId,            /* DSP Identifier (0 to MaxDSPCores-1) */
    GpakSystemConfig_t *sysCfg  /* pointer to System Config info var */
    );

extern void gpakParseSystemConfig (ADT_UInt16 *Msg, GpakSystemConfig_t *sysCfg);

/*
 * gpakReadSystemParms - Read a DSP's System Parameters.
 *
 * FUNCTION
 *  This function reads a DSP's System Parameters information.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
typedef enum  gpakReadSysParmsStatus_t {   // Host return status
    RspSuccess        = GpakApiSuccess,      /* System Parameters read successfully */
    RspInvalidDsp     = GpakApiInvalidDsp,   /* invalid DSP */
    RspDspCommFailure = GpakApiCommFailure   /* failed to communicate with DSP */
} gpakReadSysParmsStatus_t;


extern gpakReadSysParmsStatus_t gpakReadSystemParms (
    ADT_UInt32 DspId,                /* DSP Identifier (0 to MaxDSPCores-1) */
    GpakSystemParms_t *pSysParms    /* pointer to System Parms info var */
    );
extern void gpakParseSystemParms (ADT_Int16 *pMsgBuffer, GpakSystemParms_t *pSysParms);


/*
 * gpakWriteSystemParms - Write a DSP's System Parameters.
 *
 * FUNCTION
 *  This function writes a DSP's System Parameters information.
 *
 *    Note:
 *      Writing of all AGC related parameters occurs only if the AgcUpdate
 *      parameter is non zero.
 *      Writing of all VAD related parameters occurs only if the VadUpdate
 *      parameter is non zero.
 *      Writing of all PCM Echo Canceller related parameters occurs only if the
 *      PcmEcUpdate parameter is non zero.
 *      Writing of all Packet Echo Canceller related parameters occurs only if
 *      the PktEcUpdate parameter is non zero.
 *      Writing of all Conference related parameters occurs only if the
 *      ConfUpdate parameter is non zero.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
typedef enum gpakWriteSysParmsStatus_t {   // Host return status
    WspSuccess        = GpakApiSuccess,        /* System Parameters written successfully */
    WspParmError      = GpakApiParmError,      /* Write System Parms's Parameter error */
    WspInvalidDsp     = GpakApiInvalidDsp,     /* invalid DSP */
    WspDspCommFailure = GpakApiCommFailure     /* failed to communicate with DSP */
} gpakWriteSysParmsStatus_t;

extern gpakWriteSysParmsStatus_t gpakWriteSystemParms (
    ADT_UInt32 DspId,                /* DSP Identifier (0 to MaxDSPCores-1) */
    GpakSystemParms_t *pSysParms,   /* pointer to System Parms info var */
    ADT_UInt16 AgcUpdate,           /* flag indicating AGC parms update: */
              /*  0 == no update, 1 == A-device update, 2 == B-device update, 3 == update A and B */

    ADT_UInt16 VadUpdate,           /* flag indicating VAD parms update */
    ADT_UInt16 PcmEcUpdate,         /* flag indicating PCM Ecan parms update */
    ADT_UInt16 PktEcUpdate,         /* flag indicating Pkt Ecan parms update */
    ADT_UInt16 ConfUpdate,          /* flag indicating Conference prms update */
    ADT_UInt16 CIDUpdate,           /* flag indicating CID parms update */
    GPAK_SysParmsStat_t *pStatus    /* pointer to Write System Parms Status */
    );
    
extern ADT_UInt32 gpakFormatSystemParms (ADT_Int16 *Msg, GpakSystemParms_t *sysPrms,
    ADT_UInt16 AgcUpdate,   ADT_UInt16 VadUpdate, ADT_UInt16 PcmEcUpdate,
    ADT_UInt16 PktEcUpdate, ADT_UInt16 ConfUpdate, ADT_UInt16 CIDUpdate);




/* 
 *  gpakReadEventFIFOMessage 
 *
 * FUNCTION
 *  This function reads the G.PAK event buffer.  It is called by the host
 *  to read the next unread event within the FIFO queue.  This routine is 
 *  normally trigger by a  host interrupt, but may be called periodically to 
 *  poll the DSP for events.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
typedef enum gpakReadEventFIFOMessageStat_t {   // Host return status

    RefEventAvail     = GpakApiSuccess,            /* Successful */
    RefNoEventAvail   = GpakApiNoPayload,          /* No data ready */
    RefInvalidDsp     = GpakApiInvalidDsp,         /* Invalid DSP */
    RefDspCommFailure = GpakApiCommFailure,        /* Failed to communicate with DSP */
    RefInvalidEvent   = GpakApiUnknownReturnData,  /* Invalid event */
    RefNoEventQue     = GpakApiNoQue               /* DSP not configured with an event queue. */

} gpakReadEventFIFOMessageStat_t;


extern gpakReadEventFIFOMessageStat_t gpakReadEventFIFOMessage (
      ADT_UInt32 DspId,                      /* DSP Identifier (0 to MaxDSPCores-1) */
      ADT_Int16 *pChannelId,                /* pointer to channel Id */
      GpakAsyncEventCode_t *pEventCode,     /* pointer to event type */
      GpakAsyncEventData_t *pEventData);    /* pointer to event structure */


/* 
 * gpakReadCpuUsage - Read CPU usage statistics from a DSP.
 *
 * FUNCTION
 *  This function reads the CPU usage statistics from a DSP's memory. The
 *  average CPU usage in units of .1 percent are obtained for each of the frame
 *  rates.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
typedef enum gpakCpuUsageStatus_t {   // Host return status
    GcuSuccess        = GpakApiSuccess,      /* usage statistics read successfully */
    GcuInvalidDsp     = GpakApiInvalidDsp,   /* invalid DSP */
    GcuDspCommFailure = GpakApiCommFailure   /* failed to communicate with DSP */
} gpakCpuUsageStatus_t;

extern gpakCpuUsageStatus_t gpakReadCpuUsage (
    ADT_UInt32 DspId,         /* DSP Identifier (0 to MaxDSPCores-1) */
    ADT_UInt32 *AvgCpuUsage,  /* pointer to Average frame usage array */
    ADT_UInt32 *PeakCpuUsage  /* pointer to Peak frame usage array */
    );

extern ADT_Int32 gpakReadRTPClock (ADT_UInt32 DspId);

//}===============================================================
//    Serial port configuration
//{===============================================================

#if (DSP_TYPE == 64 || DSP_TYPE == 6424  || DSP_TYPE == 6437 || DSP_TYPE == 6452 || DSP_TYPE == 3530)
typedef struct GpakPortConfig_t {
   GpakSlotCfg_t SlotsSelect1;         /* port 1 Slot selection */
   GpakActivation Port1Enable;
   ADT_Int32 Port1SlotMask0_31;
   ADT_Int32 Port1SlotMask32_63;
   ADT_Int32 Port1SlotMask64_95;
   ADT_Int32 Port1SlotMask96_127;

   GpakSlotCfg_t SlotsSelect2;         /* port 2 Slot selection */
   GpakActivation Port2Enable;
   ADT_Int32 Port2SlotMask0_31;
   ADT_Int32 Port2SlotMask32_63;
   ADT_Int32 Port2SlotMask64_95;
   ADT_Int32 Port2SlotMask96_127;
   
   GpakSlotCfg_t SlotsSelect3;         /* port 3 Slot selection */
   GpakActivation Port3Enable;
   ADT_Int32 Port3SlotMask0_31;
   ADT_Int32 Port3SlotMask32_63;
   ADT_Int32 Port3SlotMask64_95;
   ADT_Int32 Port3SlotMask96_127;

   ADT_Int16 TxDataDelay1, TxDataDelay2, TxDataDelay3;
   ADT_Int16 RxDataDelay1, RxDataDelay2, RxDataDelay3;
   ADT_Int16 ClkDiv1,      ClkDiv2,      ClkDiv3;
   ADT_Int16 FramePeriod1, FramePeriod2, FramePeriod3;
   ADT_Int16 FrameWidth1,  FrameWidth2,  FrameWidth3;
   GpakCompandModes Compand1,     Compand2,  Compand3;
   GpakActivation   SampRateGen1, SampRateGen2, SampRateGen3; 

   GpakActivation AudioPort1Enable;
   ADT_Int32 AudioPort1TxSerMask;  
   ADT_Int32 AudioPort1RxSerMask;  
   ADT_Int32 AudioPort1SlotMask;   

   GpakActivation AudioPort2Enable;
   ADT_Int32 AudioPort2TxSerMask;  
   ADT_Int32 AudioPort2RxSerMask;  
   ADT_Int32 AudioPort2SlotMask;   


} GpakPortConfig_t;
#elif (DSP_TYPE == 55)
typedef struct GpakPortConfig_t {
   GpakActivation Port1Enable;
   ADT_Int32 Port1SlotMask0_31;
   ADT_Int32 Port1SlotMask32_63;
   ADT_Int32 Port1SlotMask64_95;
   ADT_Int32 Port1SlotMask96_127;

   GpakActivation Port2Enable;
   ADT_Int32 Port2SlotMask0_31;
   ADT_Int32 Port2SlotMask32_63;
   ADT_Int32 Port2SlotMask64_95;
   ADT_Int32 Port2SlotMask96_127;
   
   GpakActivation Port3Enable;
   ADT_Int32 Port3SlotMask0_31;
   ADT_Int32 Port3SlotMask32_63;
   ADT_Int32 Port3SlotMask64_95;
   ADT_Int32 Port3SlotMask96_127;

   ADT_Int16 TxDataDelay1, TxDataDelay2, TxDataDelay3;
   ADT_Int16 RxDataDelay1, RxDataDelay2, RxDataDelay3;
   ADT_Int16 ClkDiv1,      ClkDiv2,      ClkDiv3;
   ADT_Int16 FramePeriod1, FramePeriod2, FramePeriod3;
   ADT_Int16 FrameWidth1,  FrameWidth2,  FrameWidth3;
   GpakCompandModes Compand1,     Compand2,  Compand3;
   GpakActivation   SampRateGen1, SampRateGen2, SampRateGen3; 
   
   ADT_UInt16 txClockPolarity1, txClockPolarity2, txClockPolarity3;
   ADT_UInt16 rxClockPolarity1, rxClockPolarity2, rxClockPolarity3;

} GpakPortConfig_t;

#elif (DSP_TYPE == 54)
typedef struct GpakPortConfig_t {
   GpakSlotCfg_t MultiMode[3];   // Multi channel mode

   ADT_Int16 LowBlk[3];          // Index of channel in low block
   ADT_Int16 LowMask[3];         // Mask of channels in low block
   ADT_Int16 HighBlk[3];         // Index of channel in low block
   ADT_Int16 HighMask[3];        // Mask of channels in low block

   GpakCompandModes Compand[3];  // Companding mode

   // Tx/Rx clock and frame sync polarities and source
   ADT_Bool  RxClockRise [3];
   ADT_Bool  RxClockInternal [3];

   ADT_Bool  TxClockFall [3];
   ADT_Bool  TxClockInternal [3];

   ADT_Bool  RxSyncLow [3];
   ADT_Bool  RxSyncInternal [3];

   ADT_Bool  TxSyncLow [3];
   ADT_Bool  TxSyncInternal [3];

   // Tx and Rx data delays
   ADT_Int16 TxDelay [3];
   ADT_Int16 RxDelay [3];

   ADT_Int16 ClkDiv [3];
   ADT_Int16 PulseWidth [3];


} GpakPortConfig_t;
#endif

/*
 * gpakConfigurePorts - Configure a DSP's serial ports.
 *
 * FUNCTION
 *  This function configures a DSP's serial ports.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
typedef enum gpakConfigPortStatus_t {    // Host return status
    CpsSuccess        = GpakApiSuccess,         /* Serial Ports configured successfully */
    CpsParmError      = GpakApiParmError,       /* Configure Ports Parameter error */
    CpsInvalidDsp     = GpakApiInvalidDsp,      /* invalid DSP */
    CpsDspCommFailure = GpakApiCommFailure   /* failed to communicate with DSP */
} gpakConfigPortStatus_t;

extern gpakConfigPortStatus_t gpakConfigurePorts (
    ADT_UInt32 DspId,         /* DSP Id (0 to MaxDSPCores-1) */
    GpakPortConfig_t *,      /* pointer to Port Config info */
    GPAK_PortConfigStat_t *  /* pointer to Port Config Status */
    );
extern ADT_UInt32 gpakFormatPortConfig (ADT_UInt16 *Msg, GpakPortConfig_t *);


//}===============================================================
//    Channel setup and teardown
//{===============================================================
//
//-----  Channel setup structures
//
//  PCM to PCM channel config
typedef struct PcmPcmCfg_t {
   GpakSerialPort_t    InPortA;            /* Input A  Serial Port Id */
   ADT_UInt16          InSlotA;            /*          Time Slot      */
   ADT_UInt16          InPinA;             /*          Serial Pin     */
   GpakSerialPort_t    OutPortA;           /* Output A Serial Port Id */
   ADT_UInt16          OutSlotA;           /*          Time Slot */
   ADT_UInt16          OutPinA;            /*          Serial Pin  */
   GpakSerialPort_t    InPortB;            /* Input B  Serial Port Id */
   ADT_UInt16          InSlotB;            /*          Time Slot */
   ADT_UInt16          InPinB;             /*          Serial Pin  */
   GpakSerialPort_t    OutPortB;           /* Output B Serial Port Id */
   ADT_UInt16          OutSlotB;           /*          Time Slot */
   ADT_UInt16          OutPinB;            /*          Serial Pin  */
   GpakActivation      EcanEnableA;        /* Input A Echo Cancel enable */
   GpakActivation      EcanEnableB;        /* Input B Echo Cancel enable */
   GpakActivation      AECEcanEnableA;     /* Acoustic Echo Cancel enable */
   GpakActivation      AECEcanEnableB;     /* Acoustic Echo Cancel enable */
   GpakFrameSizes      FrameSize;          /* Frame Size */
   GpakActivation      NoiseSuppressA;     /* Output A Noise suppression */
   GpakActivation      NoiseSuppressB;     /* Output B Noise suppression */

   GpakActivation      AgcEnableA;          /* Output A AGC enable */
   GpakActivation      AgcEnableB;          /*          AGC enable */
   GpakToneTypes       ToneTypesA;          /* Input A Tone Detect Types */
   GpakToneTypes       ToneTypesB;          /*         Tone Detect Types */
   GpakCidMode_t       CIdModeA;            /* Caller Id mode */
   GpakCidMode_t       CIdModeB;            /* Caller Id mode */
   GpakChannelTypes    Coding;              /* VoiceChannel or DataChannel */
   ADT_Int16           ToneGenGainG1A;      /* Gain Control Block 1A */
   ADT_Int16           OutputGainG2A;       /* Gain Control Block 2A */
   ADT_Int16           InputGainG3A;        /* Gain Control Block 3A */
   ADT_Int16           ToneGenGainG1B;      /* Gain Control Block 1B */
   ADT_Int16           OutputGainG2B;       /* Gain Control Block 2B */
   ADT_Int16           InputGainG3B;        /* Gain Control Block 3B */
} PcmPcmCfg_t;

//  PCM to PKT channel config
typedef struct PcmPktCfg_t {
   GpakSerialPort_t    PcmInPort;          /* PCM Input  Serial Port Id */
   ADT_UInt16          PcmInSlot;          /*            Time Slot */
   ADT_UInt16          PcmInPin;           /*            Serial Pin  */
   GpakSerialPort_t    PcmOutPort;         /* PCM Output Serial Port Id */
   ADT_UInt16          PcmOutSlot;         /*            Time Slot */
   ADT_UInt16          PcmOutPin;          /*            Serial Pin  */
   GpakCodecs          PktInCoding;        /* Packet In  Coding */
   GpakFrameSizes      PktInFrameSize;     /*            Frame Size */
   GpakCodecs          PktOutCoding;       /* Packet Out Coding */
   GpakFrameSizes      PktOutFrameSize;    /*            Frame Size */
   GpakActivation      PcmEcanEnable;      /* PCM Echo Cancel enable */
   GpakActivation      PktEcanEnable;      /* Packet Echo Cancel enable */
   GpakActivation      AECEcanEnable;      /* Acoustic Echo Cancel enable */
   GpakActivation      VadEnable;          /* VAD enable */
   GpakActivation      AgcEnable;          /* AGC enable */
   GpakToneTypes       ToneTypes;          /* Tone Detect Types */
   GpakCidMode_t       CIdMode;            /* Caller Id mode */
   GpakFaxMode_t       FaxMode;            /* T38 Fax Mode */
   GpakFaxTransport_t  FaxTransport;       /* T.38 Fax Transport Mode */
   GpakChannelTypes    Coding;             /* VoiceChannel or DataChannel */
   ADT_Int16           ToneGenGainG1;      /* Gain Control Block 2B */
   ADT_Int16           OutputGainG2;       /* Gain Control Block 2B */
   ADT_Int16           InputGainG3;        /* Gain Control Block 3B */
} PcmPktCfg_t;

// PKT to PKT channel config
typedef struct PktPktCfg_t {
   GpakCodecs          PktInCodingA;       /* Packet In  A Coding */
   GpakFrameSizes      PktInFrameSizeA;    /*            A Frame Size */
   GpakCodecs          PktOutCodingA;      /* Packet Out A Coding */
   GpakFrameSizes      PktOutFrameSizeA;   /*            A Frame Size */
   GpakActivation      EcanEnableA;        /* Pkt A Echo Cancel enable */
   GpakActivation      VadEnableA;         /*       VAD enable */
   GpakToneTypes       ToneTypesA;         /*       Tone Detect Types */
   GpakActivation      AgcEnableA;         /*       AGC Enable */
   ADT_UInt16          ChannelIdB;         /* Channel Id B */
   GpakCodecs          PktInCodingB;       /* Packet In B Coding */
   GpakFrameSizes      PktInFrameSizeB;    /*             Frame Size */
   GpakCodecs          PktOutCodingB;      /* Packet Out B Coding */
   GpakFrameSizes      PktOutFrameSizeB;   /*              Frame Size */
   GpakActivation      EcanEnableB;        /* Pkt B Echo Cancel enable */
   GpakActivation      VadEnableB;         /*       VAD enable */
   GpakToneTypes       ToneTypesB;         /*       Tone Detect Types */
   GpakActivation      AgcEnableB;         /*       AGC Enable */
} PktPktCfg_t;

// Ciruit data channel config
typedef struct CircuitCfg_t {
   GpakSerialPort_t    PcmInPort;          /* PCM Input Serial Port Id */
   ADT_UInt16  PcmInSlot;                  /*           Time Slot */
   ADT_UInt16  PcmInPin;                   /*           Serializer  */
   GpakSerialPort_t    PcmOutPort;         /* PCM Output Serial Port Id */
   ADT_UInt16  PcmOutSlot;                 /*            Time Slot */
   ADT_UInt16  PcmOutPin;                  /*            Serializer */
   ADT_UInt16  MuxFactor;                  /* Circuit Mux Factor */
} CircuitCfg_t;

// Conference to PCM channel config
typedef struct CnfPcmCfg_t {
   ADT_UInt16          ConferenceId;       /* Conference Identifier */
   GpakSerialPort_t    PcmInPort;          /* PCM Input  Serial Port Id */
   ADT_UInt16          PcmInSlot;          /*            Time Slot */
   ADT_UInt16          PcmInPin;           /*            Pin  */

   GpakSerialPort_t    PcmOutPort;         /* PCM Output Serial Port Id */
   ADT_UInt16          PcmOutSlot;         /*            Time Slot */
   ADT_UInt16          PcmOutPin;          /*            Pin  */

   GpakActivation      EcanEnable;         /* Echo Cancel enable */
   GpakActivation      AECEcanEnable;      /* Acoustic Echo Cancel enable */
   GpakActivation      AgcInEnable;        /* Input AGC enable */
   GpakActivation      AgcOutEnable;       /* Output AGC enable */

   GpakToneTypes       ToneTypes;          /* Tone Detect Types */
   ADT_Int16           ToneGenGainG1;      /* Gain Control Block 1 */
   ADT_Int16           OutputGainG2;       /* Gain Control Block 2 */
   ADT_Int16           InputGainG3;        /* Gain Control Block 3 */

} CnfPcmCfg_t;

// Conference to PKT channel config
typedef struct CnfPktCfg_t {
   ADT_UInt16          ConferenceId;       /* Conference Identifier */
   GpakCodecs          PktInCoding;        /* Packet In Coding */
   GpakCodecs          PktOutCoding;       /* Packet Out Coding */
   GpakActivation      EcanEnable;         /* Echo Cancel enable */
   GpakActivation      VadEnable;          /* VAD enable */
   GpakActivation      AgcInEnable;        /* Input AGC enable */
   GpakActivation      AgcOutEnable;       /* Output AGC enable */
   GpakToneTypes       ToneTypes;          /* Tone Detect Types */
   ADT_Int16           ToneGenGainG1;      /* Gain Control Block 1 */
   ADT_Int16           OutputGainG2;       /* Gain Control Block 2 */
   ADT_Int16           InputGainG3;        /* Gain Control Block 3 */
   GpakFrameSizes      PktFrameSize;       /* Pkt Frame Size */

} CnfPktCfg_t;

// Composite conference channel config
typedef struct CnfCmpCfg_t {
   ADT_UInt16          ConferenceId;       /* Conference Identifier */
   GpakSerialPort_t    PcmOutPort;         /* PCM Output Serial Port Id */
   ADT_UInt16          PcmOutSlot;         /*            Time Slot */
   ADT_UInt16          PcmOutPin;          /*            Pin  */

   GpakCodecs          PktOutCoding;       /* Packet Out Coding */
   GpakActivation      VadEnable;          /* VAD enable */
} CnfCmpCfg_t;

// Loopback Coder channel config
typedef struct LbCdrCgf_t {
   GpakCodecs          PktInCoding;        /* Packet In Coding */
   GpakCodecs          PktOutCoding;       /* Packet Out Coding */
} LbCdrCgf_t;

// Main channel configuration structure
typedef union GpakChannelConfig_t {

    PcmPktCfg_t PcmPkt;    /* PCM to Packet channel type. */
    PcmPcmCfg_t PcmPcm;    /* PCM to PCM channel type. */
    PktPktCfg_t PktPkt;    /* Packet to Packet channel type. */

    CircuitCfg_t Circuit;  /* Circuit Data channel type. */

    CnfPcmCfg_t ConferPcm;  /* Conference PCM channel type. */
    CnfPktCfg_t ConferPkt;  /* Conference Packet channel type. */
    CnfCmpCfg_t ConferComp; /* Conference Composite channel type. */
    LbCdrCgf_t  LpbkCoder;  /* loopback Coder channel type */

} GpakChannelConfig_t;


/*
 * gpakConfigureChannel - Configure a DSP's Channel.
 *
 * FUNCTION
 *  This function configures a DSP's Channel.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
typedef enum gpakConfigChanStatus_t {    // Host return status
    CcsSuccess        = GpakApiSuccess,         /* Channel Configured successfully */
    CcsParmError      = GpakApiParmError,       /* Channel Config Parameter error */
    CcsInvalidChannel = GpakApiInvalidChannel,  /* invalid channel */
    CcsInvalidDsp     = GpakApiInvalidDsp,      /* invalid DSP */
    CcsDspCommFailure = GpakApiCommFailure      /* failed to communicate with DSP */
} gpakConfigChanStatus_t;

extern gpakConfigChanStatus_t gpakConfigureChannel (
    ADT_UInt32 DspId,                    /* DSP Id (0 to MaxDSPCores-1) */
    ADT_UInt16 ChannelId,               /* Channel Id (0 to MaxChannelAlloc-1) */
    GpakChanType ChannelType,           /* Channel Type */
    GpakChannelConfig_t *pChanConfig,   /* pointer to Channel Config info */
    GPAK_ChannelConfigStat_t *pStatus   /* pointer to Channel Config Status */
    );

extern ADT_UInt32 gpakFormatChannelConfig (   /* Returns message length in 16-bit words */
    ADT_UInt16 *Msg,                          /* Message buffer */
    GpakChannelConfig_t *pChan,               /* pointer to Channel Config structure */
    ADT_UInt16 ChannelId,                     /* channel Id */
    GpakChanType ChannelType);                /* channel type */


/*
 * gpakTearDownChannel - Tear Down a DSP's Channel.
 *
 * FUNCTION
 *  This function tears down a DSP's Channel.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
typedef enum gpakTearDownStatus_t {    // Host return status
    TdsSuccess        = GpakApiSuccess,         /* Channel Tear Down successful */
    TdsError          = GpakApiParmError,       /* Channel Tear Down error */
    TdsInvalidChannel = GpakApiInvalidChannel,  /* invalid channel */
    TdsInvalidDsp     = GpakApiInvalidDsp,      /* invalid DSP */
    TdsDspCommFailure = GpakApiCommFailure       /* failed to communicate with DSP */
} gpakTearDownStatus_t;

extern gpakTearDownStatus_t gpakTearDownChannel (
    ADT_UInt32 DspId,                    /* DSP Id (0 to MaxDSPCores-1) */
    ADT_UInt16 ChannelId,               /* Channel Id (0 to MaxChannelAlloc-1) */
    GPAK_TearDownChanStat_t *pStatus    /* pointer to Tear Down Status */
    );


//}===============================================================
//    Channel status 
//{===============================================================

//-------  Channel status
typedef struct GpakChannelStatus_t {
    ADT_UInt16 ChannelId;          /* Channel Identifier */
    GpakChanType ChannelType;      /* Channel Type */
    ADT_UInt16 NumPktsToDsp;       /* number of input packets recvd. by DSP */
    ADT_UInt16 NumPktsFromDsp;     /* number of output packets sent by DSP */
    ADT_UInt16 NumPktsToDspB;      /* number of input packets recvd. by DSP (B-side of pkt-pkt only) */
    ADT_UInt16 NumPktsFromDspB;    /* number of output packets sent by DSP (B-side of pkt-pkt only) */
    ADT_UInt16 BadPktHeaderCnt;    /* Invalid Packet Headers count */
    ADT_UInt16 PktOverflowCnt;     /* Packet Overflow count */
    ADT_UInt16 PktUnderflowCnt;    /* Packet Underflow count */
    ADT_UInt16 StatusFlags;        /* Status Flag bits */
    GpakChannelConfig_t ChannelConfig;     /* Channel Configuration */
} GpakChannelStatus_t;


/* gpakGetChannelStatus return status. */
typedef enum gpakGetChanStatStatus_t {
    CssSuccess        = GpakApiSuccess,         /* Successful */
    CssError          = GpakApiParmError,       /* Parameter error */
    CssInvalidChannel = GpakApiInvalidChannel,  /* Invalid channel */
    CssInvalidDsp     = GpakApiInvalidDsp,      /* Invalid DSP */
    CssDspCommFailure = GpakApiCommFailure      /* Failed to communicate with DSP */
} gpakGetChanStatStatus_t;
    

/*
 * gpakGetChannelStatus - Read a DSP's Channel Status.
 *
 * FUNCTION
 *  This function reads a DSP's Channel Status information for a particular
 *  channel.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
extern gpakGetChanStatStatus_t gpakGetChannelStatus (
    ADT_UInt32 DspId,                  /* DSP Identifier (0 to MaxDSPCores-1) */
    ADT_UInt16 ChannelId,             /* Channel Id (0 to MaxChannelAlloc-1) */
    GpakChannelStatus_t *pChanStatus, /* pointer to Channel Status info var */
    GPAK_ChannelStatusStat_t *pStatus /* pointer to Channel Status Status */
    );

extern gpakGetChanStatStatus_t gpakParseChanStatusResponse (
    ADT_UInt32 ReplyLength,                 /* length of response */
    ADT_UInt16 *pMsgBuffer,                  /* pointer to response buffer */
    GpakChannelStatus_t *pChanStatus,      /* pointer to Channel Status info var */
    GPAK_ChannelStatusStat_t *pStatus);    /* pointer to Channel Status Status */ 



//}===============================================================
//    Channel control
//{===============================================================
//
//---  Acoustic echo canceller control
//
typedef struct GpakAECParms_t {
    ADT_Int16       activeTailLengthMSec;
    ADT_Int16       totalTailLengthMSec;
    ADT_Int16	    maxTxNLPThresholddB;	// TxNLP Threshold (dB)
    ADT_Int16	    maxTxLossdB;			// Maximum TxNLP Attenuation (dB)
    ADT_Int16	    maxRxLossdB;			// Maximum RxNLP Attenuation (dB)
    ADT_Int16	    targetResidualLeveldBm;	// Target TxNLP Level when suppressing (dBm)
    ADT_Int16	    maxRxNoiseLeveldBm;		// Maximum Added Low Level Noise (dBm)
    ADT_Int16	    worstExpectedERLdB;		// Worst Expected Echo Return Loss (dB)
    GpakActivation  noiseReductionEnable;	// Noise Reduction Enable Flag (0 = disable)
    GpakActivation  cngEnable;              // comfort noise enable (1 = enable)
    GpakActivation  agcEnable;				// AGC Enable Flag (0 = disable)
    ADT_Int16	    agcMaxGaindB;			// AGC Maximum Gain (dB)
    ADT_Int16	    agcMaxLossdB;			// AGC Maximum Loss (dB)
    ADT_Int16	    agcTargetLeveldBm;		// AGC Target Power Level (dBm)
    ADT_Int16	    agcLowSigThreshdBm;		// AGC Low Level Signal Threshold (dBm)
    ADT_UInt16	    maxTrainingTimeMSec;    // Maximum Training Duration (msec)
    ADT_Int16	    rxSaturateLeveldBm;		// Rx Saturation Threshold (dBm)
    ADT_Int16	    trainingRxNoiseLeveldBm; // Training Level (dBm)
    ADT_Int16	    fixedGaindB10;			 // Fixed Gain (.1 dB)
} GpakAECParms_t;

/*
 * gpakReadAECParms
 *
 * FUNCTION
 *  This function reads a DSP's AEC parameters information.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
 typedef enum gpakAECParmsStatus_t {
    AECSuccess        = GpakApiSuccess,      /* System Parameters read successfully */
    AECInvalidDsp     = GpakApiInvalidDsp,   /* invalid DSP */
    AECDspCommFailure = GpakApiCommFailure,  /* failed to communicate with DSP */
    AECParmError      = GpakApiParmError    /* Write System Parms's Parameter error */
} gpakAECParmsStatus_t;


extern gpakAECParmsStatus_t gpakReadAECParms (
    ADT_UInt32 DspId,            /* DSP Identifier (0 to MaxDSPCores-1) */
    GpakAECParms_t *AECParms    /* pointer to System Parms info var */
    );

/*
 * gpakWriteAECParms
 *
 * FUNCTION
 *  This function writes a DSP's acoustic ecbo cancellar parameters
 *
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
extern gpakAECParmsStatus_t gpakWriteAECParms (
    ADT_UInt32           DspId,      /* DSP Identifier (0 to MaxDSPCores-1) */
    GpakAECParms_t      *AECParms,  /* pointer to AEC parameter structure  */
    GPAK_AECParmsStat_t *Status     /* pointer to AEC write status return  */
    );

//
//---  Noise reduction control
//
typedef struct GpakNoiseParams_t {
    /* VAD parameters. */
   ADT_Int16 LowThreshdB;        /* VAD Noise Floor Threshold (dBm) -96..0.  Level above
                                    which voice MAY be declared */
   ADT_Int16 HighThreshdB;       /* VAD Noise Ceiling Threshold (dBm) -96..0.  Level above
                                    which voice WILL be declared */
   ADT_Int16 HangMSec;           /* VAD Speech Hangover Time (msec) 0..32767 */
   ADT_Int16 WindowSize;         /* VAD Analysis Window Size (# samples) */


    /* Noise suppression parameters */
    ADT_Int16 MaxAtten;          /* Aggressiveness of noise suppression
                                       0  -  No suppression
                                      35  -  Most aggressive suppression  */
} GpakNoiseParams_t;

/*
 * gpakReadNoiseParms - Read a DSP's System Parameters.
 *
 * FUNCTION
 *  This function reads a DSP's System Parameters information.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
extern gpakReadSysParmsStatus_t gpakReadNoiseParms (
    ADT_UInt32 DspId,                  /* DSP Identifier (0 to MaxDSPCores-1) */
    GpakNoiseParams_t *pNoiseParms    /* pointer to System Parms info var */
    );


/*
 * gpakWriteNoiseParms - Write a DSP's Noise Parameters.
 *
 * FUNCTION
 *  This function writes a DSP's System Parameters information.
 *
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
extern gpakWriteSysParmsStatus_t gpakWriteNoiseParms (
    ADT_UInt32 DspId,                    /* DSP Identifier (0 to MaxDSPCores-1) */
    GpakNoiseParams_t   *pNoiseParms,   /* pointer to System Parms info var */
    GPAK_SysParmsStat_t *pStatus    /* pointer to Write System Parms Status */
    );



//}===============================================================
//    Conference configuration
//{===============================================================

typedef enum gpakConfigConferStatus_t {   // Host return status
    CfsSuccess        = GpakApiSuccess,         /* Conference Configured successfully */
    CfsParmError      = GpakApiParmError,       /* Conference Config Parameter error */
    CfsInvalidDsp     = GpakApiInvalidDsp,      /* invalid DSP */
    CfsDspCommFailure = GpakApiCommFailure      /* failed to communicate with DSP */
} gpakConfigConferStatus_t;

/*
 * gpakConfigureConference - Configure a DSP's Conference.
 *
 * FUNCTION
 *  This function configures a DSP's Conference.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
extern gpakConfigConferStatus_t gpakConfigureConference (
    ADT_UInt32 DspId,                   /* DSP Id (0 to MaxDSPCores-1) */
    ADT_UInt16 ConferenceId,            /* Conference Id */
    GpakFrameSizes FrameSize,           /* Frame Size */
    GpakToneTypes InputToneTypes,       /* Input Tone Detection types */
    GPAK_ConferConfigStat_t *pStatus    /* pntr to Conference Config Status */
    );

//}===============================================================
//    Arbitrary Tone Detector configuration
//{===============================================================
typedef struct {
	ADT_UInt16 numDistinctFreqs;   // distinct freqs need to be detected
	ADT_UInt16 numTones;           // number of tones needs to be detected
	ADT_UInt16 minPower;           // miminum power of tone in dB, 0 == default level
	ADT_UInt16 maxFreqDeviation;   // frequency in spec range, , 0 == means default
} GpakArbTdParams_t;

typedef struct {
	ADT_UInt16 f1;           // freq 1 of tone (non-zero)
    ADT_UInt16 f2;           // freq 2 of tone (can be zero if single frequency)
    ADT_UInt16 index;        // detect index reported for this tone
} GpakArbTdToneIdx_t;

typedef enum gpakConfigArbToneStatus_t {
  CatSuccess         = GpakApiSuccess,         /* Arb Detector Configured successfully */
  CatParmError       = GpakApiParmError,       /* Arb Detector Config Parameter error */
  CatInvalidDsp      = GpakApiInvalidDsp,      /* invalid DSP */
  CatDspCommFailure  = GpakApiCommFailure      /* failed to communicate with DSP */
} gpakConfigArbToneStatus_t;

/*
 * gpakConfigArbToneDetect - Configure a DSP's Arbitrary Tone Detector.
 *
 * FUNCTION
 *  This function configures a DSP's Arbitrary Tone Detector.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
gpakConfigArbToneStatus_t gpakConfigArbToneDetect (
    ADT_UInt32                  DspId,        /* DSP Id (0 to MaxDSPCores-1) */
    ADT_UInt16                  ArbCfgId,     /* Arb Detector Configuration Id */
    GpakArbTdParams_t           *pArbParams,  /* pntr to Arb detector config params */
    GpakArbTdToneIdx_t          *pArbTones,   /* pntr to tones used by this configuration (16 max) */
    ADT_UInt16                  *pArbFreqs,   /* pntr to array of all distinct frequencies (32 max)*/
    GPAK_ConfigArbToneStat_t    *pStatus      /* pntr to Arb Tone detect Config Status */
    );


typedef enum gpakActiveArbToneStatus_t {
  AatSuccess         = GpakApiSuccess,         /* Arb Detector Configured successfully */
  AatParmError       = GpakApiParmError,       /* Arb Detector Config Parameter error */
  AatInvalidDsp      = GpakApiInvalidDsp,      /* invalid DSP */
  AatDspCommFailure  = GpakApiCommFailure      /* failed to communicate with DSP */
} gpakActiveArbToneStatus_t;

/*
 * gpakActiveArbToneDetect - Configure a DSP's Arbitrary Tone Detector.
 *
 * FUNCTION
 *  This function activates a DSP's Arbitrary Tone Detector.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
gpakActiveArbToneStatus_t gpakActiveArbToneDetect (
    ADT_UInt32                  DspId,        /* DSP Id (0 to MaxDSPCores-1) */
    ADT_UInt16                  ArbCfgId,     /* Arb Detector Configuration Id */
    GPAK_ActiveArbToneStat_t    *pStatus      /* pntr to Arb Tone detect Config Status */
    );

//}===============================================================
//    Packet transfers
//{===============================================================

/*
 * gpakSendPayloadToDsp - Send a Payload to a DSP's Channel.
 *
 * FUNCTION
 *  This function writes a Payload message into DSP memory for a particular
 *  channel.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
typedef enum gpakSendPayloadStatus_t {   // Host return status
    SpsSuccess        = GpakApiSuccess,         /* Success */
    SpsBufferFull     = GpakApiBufferFull,      /* channel's payload buffer full */
    SpsInvalidChannel = GpakApiInvalidChannel,  /* invalid channel */
    SpsInvalidDsp     = GpakApiInvalidDsp,      /* invalid DSP */
    SpsDspCommFailure = GpakApiCommFailure       /* failed to communicate with DSP */
} gpakSendPayloadStatus_t;

extern gpakSendPayloadStatus_t gpakSendPayloadToDsp (
    ADT_UInt32 DspId,               /* DSP Identifier (0 to MaxDSPCores-1) */
    ADT_UInt16 ChannelId,          /* Channel Id (0 to MaxChannelAlloc-1) */
    GpakPayloadClass PayloadClass, /* Payload Class */
    ADT_Int16  PayloadType,         /* Payload Type code */
    ADT_UInt8 *pPayloadData,       /* pointer to Payload buffer, 32-bit aligned */
    ADT_UInt16 PayloadSize         /* length of Payload data (octets) */
    );



/*
 * gpakGetPayloadFromDsp - Read a Payload from a DSP's Channel.
 *
 * FUNCTION
 *  This function reads a Payload message from DSP memory for a particular
 *  channel.
 *
 *  Note: The payload data is returned in the specified data buffer. The
 *        'pPayloadSize' parameter must be set to the size of the buffer on
 *        entry and is set to the number of octets stored in the buffer on exit
 *        if a payload is read successfully. If the buffer is too short for the
 *        data, a failure indication is returned and the payload remains unread.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
typedef enum gpakGetPayloadStatus_t {   // Host return status
    GpsSuccess        = GpakApiSuccess,         /* payload read successfully */
    GpsNoPayload      = GpakApiNoPayload,       /* there are no payloads ready */
    GpsInvalidChannel = GpakApiInvalidChannel,  /* invalid channel */
    GpsBufferTooSmall = GpakApiBufferTooSmall,     /* payload data buffer too small */
    GpsInvalidDsp     = GpakApiInvalidDsp,      /* invalid DSP */
    GpsDspCommFailure = GpakApiCommFailure      /* failed to communicate with DSP */
} gpakGetPayloadStatus_t;

extern gpakGetPayloadStatus_t gpakGetPayloadFromDsp (
    ADT_UInt32 DspId,                  /* DSP Identifier (0 to MaxDSPCores-1) */
    ADT_UInt16 ChannelId,             /* Channel Id (0 to MaxChannelAlloc-1) */
    GpakPayloadClass *pPayloadClass,  /* pointer to Payload Class var */
    ADT_Int16 *pPayloadType,  /* pointer to Payload Type code var */
    ADT_UInt8 *pPayloadData,  /* pointer to Payload buffer...32-bit aligned */
    ADT_UInt16 *pPayloadSize,  /* length of Payload data (octets) */
    ADT_UInt32 *pTimeStamp);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakSendPacketToDsp - Send an RTP packet to a DSP's Channel.
 *
 * FUNCTION
 *  Writes a Payload message into DSP memory for a specified channel.
 *
 * Inputs
 *   DspId      - Dsp identifier 
 * ChannelId    - Channel identifier
 * pData        - Pointer to RTP packet data.  MUST be aligned on DSP word boundary
 * PacketI8     - Byte count of RTP packet data.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
extern gpakSendPayloadStatus_t gpakSendPacketToDsp (
    ADT_UInt32 DspId,               /* DSP Identifier (0 to MaxDSPCores-1) */
    ADT_UInt16 ChannelId,          /* Channel Id (0 to MaxChannelAlloc-1) */
    ADT_UInt8 *pData,              /* pointer to RTP packet buffer...32-bit aligned */
    ADT_UInt16 PacketI8);          /* length of RTP packet data (octets) */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakGetPacketFromDsp - Read an RTP Packet from a DSP's Channel.
 *
 * FUNCTION
 *  Reads next available RTP packet from DSP memory.
 *
 *  Note: If the buffer is too short for the data, a failure indication is returned
 *        and all remaining packets are purged.
 * 
 * Inputs
 *     DspId      - Dsp identifier 
 * 
 * Outputs
 *   pChannelId   - Channel identifier
 *   pData        - Pointer to RTP packet data.  MUST be aligned on 32-bit boundary
 *
 * Updates
 *     PacketI8    - Before. Byte count of payload buffer size
 *                   After,  Byte count of actual payload
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
extern gpakGetPayloadStatus_t gpakGetPacketFromDsp (
    ADT_UInt32 DspId,               /* DSP Identifier (0 to MaxDSPCores-1) */
    ADT_UInt16 *pChannelId,        /* Channel Id (0 to MaxChannelAlloc-1) */
    ADT_UInt8  *pData,             /* pointer to RTP packet buffer...32-bit aligned */
    ADT_UInt16 *PacketI8);         /* pointer to length of RTP packet data (octets) */

//}===============================================================
//   Run-time control
//{===============================================================


/*
 * gpakTestMode - Configure/perform a DSP's Test mode.
 *
 * FUNCTION
 *  This function configures or performs a DSP's Test mode.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */

typedef enum  gpakTestStatus_t {   // Host return status
    CtmSuccess        = GpakApiSuccess,      /* Test mode configured/performed successfully */
    CtmParmError      = GpakApiParmError,    /* Test mode parameter error */
    CtmInvalidDsp     = GpakApiInvalidDsp,   /* invalid DSP */
    CtmDspCommFailure = GpakApiCommFailure,   /* failed to communicate with DSP */
    CtmCustomMsgError = GpakApiBufferTooSmall /* Custom message too large to send */
} gpakTestStatus_t;

extern ADT_UInt32 formatTestModeMsg(ADT_UInt16 *Msg, GpakTestMode TestModeId, 
                                       GpakTestData_t  *pTestParm, ADT_UInt16 buflenI16);
extern gpakTestStatus_t gpakTestMode (
    ADT_UInt32      DspId,        /* DSP Id (0 to MaxDSPCores-1) */
    GpakTestMode    TestModeId,   /* Test Mode Id */
    GpakTestData_t  *pTestParm,   /* test parameter (PCM sync byte, etc.) */
    ADT_UInt16      *pRespData,   /* pointer to response data from DSP */
    GPAK_TestStat_t *pStatus      /* pointer to Test Mode status */
    );

      
typedef struct GpakToneGenParms_t {
    GpakToneGenCmd_t    ToneCmd;        // tone command
    GpakDeviceSide_t    Device;         // Device generating tone 
    GpakToneGenType_t   ToneType;       // Tone Type
    ADT_UInt16          Frequency[4];   // Frequency (Hz)
    ADT_Int16           Level[4];        // Frequency's Level (1 dBm)
    ADT_UInt16          OnDuration[4];  // On Duration (msecs)
    ADT_UInt16          OffDuration[4]; // Off Duration (msecs)
} GpakToneGenParms_t;

typedef enum gpakGenToneStatus_t {             /* Host return status */
    TgcSuccess        = GpakApiSuccess,        /* Parameters written successfully */
    TgcParmError      = GpakApiParmError,      /* Parameter error */
    TgcInvalidDsp     = GpakApiInvalidDsp,     /* Invalid DSP */
    TgcDspCommFailure = GpakApiCommFailure,    /* Failed to communicate with DSP */
    TgcInvalidChannel = GpakApiInvalidChannel /* Invalid channel */
} gpakGenToneStatus_t;

extern ADT_UInt32 gpakFormatToneGenerator (
      ADT_UInt16 *Msg,                   /* pointer to message buffer */
      ADT_UInt16 ChannelId,            /* Channel Identifier */
      GpakToneGenParms_t *tone);       /* pointer to tone generation structure */

extern gpakGenToneStatus_t gpakToneGenerate (  /* Returns com link status */
      ADT_UInt32 DspId,                    /* DSP Identifier */
      ADT_UInt16 ChannelId,               /* Channel Identifier */
      GpakToneGenParms_t *Tone,           /* pointer to tone generation structure */
      GPAK_ToneGenStat_t *pStatus         /* pointer to DSP status reply */
      );

/*
 *   gpakSendPlayRecordMsg 
 *
 * FUNCTION
 *  This function is used to issue start/stop voice recording and start
 *  voice playback commands to the DSP.
 *
 *  Notes: The start playback command requires that the payload buffer 
 *         (after a reserved 32 byte header) is filled with voice data in the 
 *         format specified by the recording mode. 
 *
 *         Stop tone = tdsDtmfDigit<x> or tdsNoToneDetected  x=0..9
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */

typedef enum gpakPlayRecordStatus_t {   // Host return status
   PrsSuccess        = GpakApiSuccess,         /* payload read successfully */
   PrsInvalidDsp     = GpakApiInvalidDsp,      /* invalid DSP */
   PrsDspCommFailure = GpakApiCommFailure,     /* communication failure */
   PrsInvalidChannel = GpakApiInvalidChannel,  /* invalid channel */
   PrsBufferTooSmall = GpakApiBufferTooSmall,  /* record buffer too small */
   PrsParmError      = GpakApiParmError       /* parameter error */
} gpakPlayRecordStatus_t;

extern gpakPlayRecordStatus_t gpakSendPlayRecordMsg (
      ADT_UInt32            DspId,       /* DSP identifier */
      ADT_UInt16            ChannelId,   /* Channel identifier */
      GpakDeviceSide_t      DeviceSide,  /* Device A or B */
      GpakPlayRecordCmd_t   Cmd,         /* Playback/record command */
      DSP_Address           BufferAddr,  /* Playback buffer start address */
      ADT_UInt32            BufferLength, /* Playback buffer length */
      GpakPlayRecordMode_t  RecMode,     /* Record mode */
      GpakToneCodes_t       StopTone,    /* Stop tone code */
      GPAK_PlayRecordStat_t *pStatus     /* Pointer to DSP status reply */
      );

extern ADT_UInt32 gpakFormatPlayRecordMsg (
      ADT_UInt16 *Msg,                   /* pointer to message buffer */
      ADT_UInt16            ChannelId,   /* Channel identifier */
      GpakDeviceSide_t      DeviceSide,  /* Device A or B */
      GpakPlayRecordCmd_t   Cmd,         /* Playback/record command */
      DSP_Address           BufferAddr,  /* Playback buffer start address */
      ADT_UInt32            BufferLength, /* Playback buffer length */
      GpakPlayRecordMode_t  RecMode,      /* Record mode */
      GpakToneCodes_t       StopTone);    /* Stop tone code */



/*
 *   gpakSendRTPCfgMsg 
 *
 * FUNCTION
 *  This function is used to configure a channel's RTP parameters.
 *
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
typedef struct GpakRTPCfg_t {
   ADT_UInt16 JitterMode;        // Jitter buffer adaption mode
   ADT_UInt16 DelayTargetMinMS;  // Minimum delay (ms) at which target delay is recentered
   ADT_UInt16 DelayTargetMS;     // Initial delay (ms) assigned to jitter buffer
   ADT_UInt16 DelayTargetMaxMS;  // Maximum delay (ms) at which target delay is recentered

   ADT_UInt16 VoicePyldType;     // Payload type to assign to default voice codec payloads
   ADT_UInt16 CNGPyldType;       // Payload type to assign to comfort noise payloads
   ADT_UInt16 TonePyldType;      // Payload type to assign to tone relay payloads
   ADT_UInt16 T38PyldType;       // Payload type to assign to t.38 fax relay payloads

   ADT_UInt32 SSRC;              // Channel's transmitting SSRC
   ADT_UInt16 StartSequence;     // Channel's first transmitting sequence number

   ADT_UInt16 DestPort;          // Remote device's UDP port
   ADT_UInt32 DestIP;            // Remote device's IP address

} GpakRTPCfg_t;

typedef enum gpakRtpStatus_t {   // Host return status
   RtpSuccess        = GpakApiSuccess,         /* payload read successfully */
   RtpInvalidDsp     = GpakApiInvalidDsp,      /* invalid DSP */
   RtpDspCommFailure = GpakApiCommFailure,     /* communication failure */
   RtpInvalidChannel = GpakApiInvalidChannel,  /* invalid channel */
   RtpParmError      = GpakApiParmError        /* parameter error */
} gpakRtpStatus_t;

extern gpakRtpStatus_t gpakSendRTPMsg (
   ADT_UInt32            DspId,       /* DSP identifier */
   ADT_UInt16            ChannelId,   /* Channel identifier */
   GpakRTPCfg_t         *Cfg,         /* Pointer to RTP configuration data */
   GPAK_RTPConfigStat_t *dspStatus);   /* Pointer to DSP status reply */

extern ADT_UInt32 gpakFormatRTPMsg (
      ADT_UInt16 *Msg,                   /* pointer to message buffer */
      ADT_UInt16  ChannelId,             /* Channel identifier */
      GpakRTPCfg_t *Cfg);           /* Pointer to configuration structure */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakGetEcanState - Read an Echo Canceller's training state.
 *
 * FUNCTION
 *  This function reads an Echo Canceller's training state information.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */

/* gpakGetEcanState return status. */
typedef enum
{
    GesSuccess          = GpakApiSuccess,         /* Echo Canceller state read successfully */
    GesInvalidChannel   = GpakApiInvalidChannel,  /* invalid channel identifer */
    GesInvalidDsp       = GpakApiInvalidDsp,      /* invalid DSP */
    GesDspCommFailure   = GpakApiCommFailure      /* failed to communicate with DSP */
} gpakGetEcanStateStat_t;

extern void gpakParseEcanState (ADT_UInt16 *Msg, GpakEcanState_t *pEcanState);

extern gpakGetEcanStateStat_t gpakGetEcanState (
    ADT_UInt32          DspId,          /* DSP identifier */
    ADT_UInt16          ChannelId,		/* channel identifier */
    GpakEcanState_t     *pEcanState,    /* pointer to Ecan state variable */
    GpakDeviceSide_t    AorB            /* device side of canceller */
    );
	    

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakSetEcanParms - Configure an Echo Canceller.
 *
 * FUNCTION
 *  This function sets an Echo Canceller's configuration parameters.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */

/* gpakSetEcanState return status. */
typedef enum
{
    SesSuccess          = GpakApiSuccess,         /* Echo Canceller state read successfully */
    SesInvalidChannel   = GpakApiInvalidChannel,  /* invalid channel identifer */
    SesInvalidDsp       = GpakApiInvalidDsp,      /* invalid DSP */
    SesParmError        = GpakApiParmError,       /* invalid configuration parameter */
    SesDspCommFailure   = GpakApiCommFailure      /* failed to communicate with DSP */
} gpakSetEcanStateStat_t;

extern ADT_UInt32 formatSetEcanState(ADT_UInt16 *Msg, ADT_UInt16 ChannelId, GpakEcanState_t *pEcanState, GpakDeviceSide_t AorB);

extern gpakSetEcanStateStat_t gpakSetEcanState (
    ADT_UInt32          DspId,          /* DSP identifier */
    ADT_UInt16          ChannelId,		/* channel identifier */
    GpakEcanState_t     *pEcanState,    /* pointer to Ecan state variable */
    GpakDeviceSide_t    AorB            /* device side of canceller */
    );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakAlgControl - Control an Algorithm.
 *
 * FUNCTION
 *  This function controls an Algorithm
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */

/* gpakAlgControl return status. */
typedef enum
{
    AcSuccess           = GpakApiSuccess,         /* control successful */
    AcInvalidChannel    = GpakApiInvalidChannel,  /* invalid channel identifier */
    AcInvalidDsp        = GpakApiInvalidDsp,      /* invalid DSP */
    AcParmError         = GpakApiParmError,       /* invalid control parameter */
    AcDspCommFailure    = GpakApiCommFailure      /* failed to communicate with DSP */
} gpakAlgControlStat_t;

extern gpakAlgControlStat_t gpakAlgControl (
    ADT_UInt32            DspId,          /* DSP identifier */
    ADT_UInt16            ChannelId,	  /* channel identifier */
    GpakAlgCtrl_t         ControlCode,    /* algorithm control code */
    GpakToneTypes         DeactTonetype,  /* tone detector to deactivate */
    GpakToneTypes         ActTonetype,    /* tone detector to activate */
    GpakDeviceSide_t	  AorBSide,       /* A or B device side */
    ADT_UInt16            NLPsetting,     /* NLP value */
    ADT_Int16             GaindB,         /* Gain Value (dB), 1dB steps: -40...40 */
    GPAK_AlgControlStat_t *pStatus        /* pointer to return status */
    );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakSendCIDPayloadToDsp - Send a transmit payload to a DSP channel
 *
 * FUNCTION
 *  This function sends a CID transmit payload to the DSP.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */

/* gpakSendCIDPayloadToDsp return status. */
typedef enum
{
    ScpSuccess          = GpakApiSuccess,         /* operation successful */
    ScpInvalidChannel   = GpakApiInvalidChannel,  /* invalid channel identifier */
    ScpInvalidDsp       = GpakApiInvalidDsp,      /* invalid DSP */
    ScpInvalidPaylen    = GpakApiInvalidPaylen,   /* Invalid Payload Length */
    ScpDspCommFailure   = GpakApiCommFailure,     /* failed to communicate with DSP */
    ScpParmError        = GpakApiParmError        /* parameter error */
} gpakSendCIDPayloadToDspStat_t;

extern gpakSendCIDPayloadToDspStat_t gpakSendCIDPayloadToDsp (
    ADT_UInt32  DspId,           /* DSP identifier */
    ADT_UInt16  ChannelId,       /* channel identifier */
    ADT_UInt8   *pPayloadData,   /* pointer to Payload data */
    ADT_UInt16  PayloadSize, 	 /* length of Payload data (octets) */
    GpakDeviceSide_t AorBSide,   /* device side: A or B */
    GPAK_TxCIDPayloadStat_t *pStatus /* error status code */
    );


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakConferenceGenerateTone - Control tone generation.
 *
 * FUNCTION
 *  This function controls the generation of tones on the specified conference.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */

extern ADT_UInt32 gpakFormatConfToneGenerator (ADT_UInt16 *Msg, 
                                   ADT_UInt16 ConfId, 
                                   GpakToneGenParms_t *tone);

extern gpakGenToneStatus_t gpakConferenceGenerateTone (
    ADT_UInt32          DspId,      	/* DSP identifier */
    ADT_UInt16          ConfId,	    	/* conference identifier */
    GpakToneGenParms_t  *pToneParms,    /* pointer to tone gen parameters */
    GPAK_ToneGenStat_t  *pStatus 	    /* pointer to DSP status reply */
    );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakReadAgcInputPower - Read short term power
 * 
 * FUNCTION
 *  This function reads a channel's short term power computed by the AGC
 * 
 * RETURNS
 *  Status  code indicating success or a specific error.
 */

/* gpakReadAgcInputPower return status. */
typedef enum  
{
    AprSuccess          = GpakApiSuccess,         /* Agc input power read successful */
    AprInvalidChannel   = GpakApiInvalidChannel,  /* invalid channel identifier */
    AprInvalidDsp       = GpakApiInvalidDsp,      /* invalid DSP identifier */
    AprDspCommFailure   = GpakApiCommFailure,     /* failed to communicate with DSP */
    AprParmError        = GpakApiParmError        /* parameter error */
} gpakAGCReadInputPowerStat_t;

extern gpakAGCReadInputPowerStat_t gpakReadAgcInputPower (
    ADT_UInt32          DspId,      /* DSP identifier */
    ADT_UInt16          ChannelId,	/* channel identifier */
    ADT_Int16           *pPowerA,   /* pointer to store A-side Input Power (dBm) */
    ADT_Int16           *pPowerB,   /* pointer to store B-side Input Power (dBm) */
    GPAK_AGCReadStat_t  *pStatus    /* pointer to DSP status reply  */
    );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakResetChannelStats - Reset a channel's statistics
 * 
 * FUNCTION
 *  This function reset's a channel's statistics specified in the SelectMask
 * 
 * RETURNS
 *  Status  code indicating success or a specific error.
 */

typedef enum 
{
    RcsSuccess          = GpakApiSuccess,         /* Statistic reset successful */
    RcsInvalidChannel   = GpakApiInvalidChannel,  /* invalid channel identifier */
    RcsInvalidDsp       = GpakApiInvalidDsp,      /* invalid DSP identifier */
    RcsDspCommFailure   = GpakApiCommFailure      /* failed to communicate with DSP */
} gpakResetChannelStat_t;

/* bit mask definitions for channel statistics reset */
typedef struct resetMask {
    ADT_UInt16 reserve:15;          /* not used */
    ADT_UInt16 PacketStats:1;       /* 1 == reset packet statistics */
} resetCsMask;

extern gpakResetChannelStat_t  gpakResetChannelStats (
    ADT_UInt32          DspId,      /* DSP Identifier */
    ADT_UInt16          ChannelId,  /* channel identifier */
    resetCsMask	        SelectMask  /* reset bit Mask */
    );


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakResetSystemStats - Reset system statistics
 * 
 * FUNCTION
 *  This function reset's the system-level statistics specified in the SelectMask
 * 
 * RETURNS
 *  Status  code indicating success or a specific error.
 */

typedef enum 
{
    RssSuccess          = GpakApiSuccess,         /* Statistic reset successful */
    RssInvalidDsp       = GpakApiInvalidDsp,      /* invalid DSP identifier */
    RssDspCommFailure   = GpakApiCommFailure      /* failed to communicate with DSP */
} gpakResetSystemStat_t;

/* bit mask definitions for channel statistics reset */
typedef struct resetSsMask {
    ADT_UInt16 reserve:14;          /* not used */
    ADT_UInt16 FramingStats:1;      /* 1 == reset framing statistics */
    ADT_UInt16 CpuUsageStats:1;     /* 1 == reset Cpu Usage statistics */
} resetSsMask;

extern gpakResetSystemStat_t  gpakResetSystemStats (
    ADT_UInt32          DspId,      /* DSP Identifier */
    resetSsMask	        SelectMask  /* reset bit Mask */
    );


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakReadMcBspStats - Read McBsp statistics
 * 
 * FUNCTION
 *  This function reads McBsp Statistics
 * 
 * RETURNS
 *  Status  code indicating success or a specific error.
 */
typedef struct gpakMcBspStats_t {
    GpakActivation  Port1Status;           /* port 1 status */
    ADT_UInt32      Port1RxIntCount;       /* port 1 Rx Interrupt count */
    ADT_UInt16      Port1RxSlips;          /* port 1 Rx Slip count */
    ADT_UInt16      Port1RxDmaErrors;      /* port 1 Rx Dma error count */
    ADT_UInt32      Port1TxIntCount;        /* port 1 Tx interrupt count */
    ADT_UInt16      Port1TxSlips;          /* port 1 Tx Slip count */
    ADT_UInt16      Port1TxDmaErrors;      /* port 1 Tx Dma error count */
    ADT_UInt16      Port1FrameSyncErrors;  /* port 1 Frame Sync error count */
    ADT_UInt16      Port1RestartCount;     /* port 1 McBsp Restart count */

    GpakActivation  Port2Status;           /* port 2 status */
    ADT_UInt32      Port2RxIntCount;       /* port 2 Rx Interrupt count */
    ADT_UInt16      Port2RxSlips;          /* port 2 Rx Slip count */
    ADT_UInt16      Port2RxDmaErrors;      /* port 2 Rx Dma error count */
    ADT_UInt32      Port2TxIntCount;        /* port 2 Tx interrupt count */
    ADT_UInt16      Port2TxSlips;          /* port 2 Tx Slip count */
    ADT_UInt16      Port2TxDmaErrors;      /* port 2 Tx Dma error count */
    ADT_UInt16      Port2FrameSyncErrors;  /* port 2 Frame Sync error count */
    ADT_UInt16      Port2RestartCount;     /* port 2 McBsp Restart count */

    GpakActivation  Port3Status;           /* port 3 status */
    ADT_UInt32      Port3RxIntCount;       /* port 3 Rx Interrupt count */
    ADT_UInt16      Port3RxSlips;          /* port 3 Rx Slip count */
    ADT_UInt16      Port3RxDmaErrors;      /* port 3 Rx Dma error count */
    ADT_UInt32      Port3TxIntCount;        /* port 3 Tx interrupt count */
    ADT_UInt16      Port3TxSlips;          /* port 3 Tx Slip count */
    ADT_UInt16      Port3TxDmaErrors;      /* port 3 Tx Dma error count */
    ADT_UInt16      Port3FrameSyncErrors;  /* port 3 Frame Sync error count */
    ADT_UInt16      Port3RestartCount;     /* port 3 McBsp Restart count */
} gpakMcBspStats_t;

typedef enum
{
    RmsSuccess          = GpakApiSuccess,         /* Statistic read successful */
    RmsInvalidDsp       = GpakApiInvalidDsp,      /* invalid DSP identifier */
    RmsDspCommFailure   = GpakApiCommFailure      /* failed to communicate with DSP */
} gpakReadMcBspStats_t;

extern void gpakParseMcBspStats (ADT_UInt16 *Msg, gpakMcBspStats_t *FrameStats);
extern gpakReadMcBspStats_t gpakReadMcBspStats ( 
        ADT_UInt32         DspId,                /* DSP Identifier */
		gpakMcBspStats_t    *FrameStats           /* Pointer to Frame Stats. */
        );

        
extern GpakApiStatus_t gpakSendDataToDsp (ADT_UInt32 DspId, ADT_UInt16 ChannelId, ADT_UInt8 *pData, ADT_UInt16 PayloadI8);
extern GpakApiStatus_t gpakGetDataFromDsp (ADT_UInt32 DspId, ADT_UInt16 ChannelId, ADT_UInt16 *pData, ADT_UInt16 *PayloadI16);        
        
        
//}===============================================================
//     Custom platform routines
//{===============================================================

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakReadDspMemory8 - Read 8-bit values from DSP memory.
 *
 * FUNCTION
 *  This function reads a contiguous block of bytes from DSP memory starting at
 *  the specified address.
 *
 * RETURNS
 *  nothing
 *
 */
extern void gpakReadDspMemory8(
    ADT_UInt32   DspId,         /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    DSP_Address DspAddress,     /* DSP's memory address of first word */
    ADT_UInt32  NumBytes,       /* number of contiguous bytes to read */
    ADT_UInt8  *pByteValues    /* pointer to array of byte values variable */
    );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakReadDspMemory16 - Read 16-bit values from DSP memory.
 *
 * FUNCTION
 *  This function reads a contiguous block of words from DSP memory starting at
 *  the specified address.
 *
 * RETURNS
 *  nothing
 *
 */
extern void gpakReadDspMemory16(
    ADT_UInt32  DspId,          /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    DSP_Address DspAddress,     /* DSP's memory address of first word */
    ADT_UInt32  NumWords,       /* number of contiguous words to read */
    ADT_UInt16 *pWordValues    /* pointer to array of word values variable */
    );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakReadDspNoSwap16 - Read DSP packet memory without byte swapping.
 *
 * FUNCTION
 *  This function reads a contiguous block of 16-bit values from DSP memory 
 *  starting at the specified address.  DSP packet memory is in big-endian order
 *
 * RETURNS
 *  nothing
 *
 */
extern void gpakReadDspNoSwap16(
    ADT_UInt32  DspId,           /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    DSP_Address DspAddress,      /* DSP's memory address of first word */
    ADT_UInt32  NumShorts,       /* number of contiguous 16-bit vals to read */
    ADT_UInt16 *pShortValues     /* pointer to array of 16-bit values variable */
    );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakReadDspMemory32 - Read 32-bit values from DSP memory.
 *
 * FUNCTION
 *  This function reads a contiguous block of 32-bit values from DSP memory 
 *  starting at the specified address.
 *
 * RETURNS
 *  nothing
 *
 */
extern void gpakReadDspMemory32(
    ADT_UInt32  DspId,          /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    DSP_Address DspAddress,     /* DSP's memory address of first word */
    ADT_UInt32  NumLongs,       /* number of contiguous 32-bit vals to read */
    ADT_UInt32  *pLongValues    /* pointer to array of 32-bit values variable */
    );


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakReadDspNoSwap32 - Read DSP packet memory without byte swapping.
 *
 * FUNCTION
 *  This function reads a contiguous block of 32-bit values from DSP memory 
 *  starting at the specified address.  DSP packet memory is in big-endian order
 *
 * RETURNS
 *  nothing
 *
 */
extern void gpakReadDspNoSwap32(
    ADT_UInt32  DspId,           /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    DSP_Address DspAddress,      /* DSP's memory address of first word */
    ADT_UInt32  NumLongs,        /* number of contiguous 32-bit vals to read */
    ADT_UInt32 *pLongValues      /* pointer to array of 32-bit values variable */
    );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakWriteDspMemory8 - Write DSP memory.
 *
 * FUNCTION
 *  This function writes a contiguous block of bytes to DSP memory starting at
 *  the specified address.
 *
 * RETURNS
 *  nothing
 *
 */
extern void gpakWriteDspMemory8(
    ADT_UInt32  DspId,          /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    DSP_Address DspAddress,     /* DSP's memory address of first word */
    ADT_UInt32  NumBytes,       /* number of contiguous bytes to write */
    ADT_UInt8  *pByteValues    /* pointer to array of bytes values to write */
    );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakWriteDspMemory16 - Write DSP memory.
 *
 * FUNCTION
 *  This function writes a contiguous block of wordss to DSP memory starting at
 *  the specified address.
 *
 * RETURNS
 *  nothing
 *
 */
extern void gpakWriteDspMemory16(
    ADT_UInt32  DspId,          /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    DSP_Address DspAddress,     /* DSP's memory address of first word */
    ADT_UInt32  NumWords,       /* number of contiguous words to write */
    ADT_UInt16 *pWordValues    /* pointer to array of word values to write */
    );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakWriteDspNoSwap16 - Write DSP packet memory without byte swapping.
 *
 * FUNCTION
 *  This function writes a contiguous block of words to DSP memory starting at
 *  the specified address.
 *
 * RETURNS
 *  nothing
 *
 */
extern void gpakWriteDspNoSwap16(
    ADT_UInt32 DspId,            /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    DSP_Address DspAddress,     /* DSP's memory address of first word */
    ADT_UInt32 NumShorts,       /* number of contiguous words to write */
    ADT_UInt16 *pShortValues    /* pointer to array of word values to write */
    );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakWriteDspMemory32 - Write DSP memory.
 *
 * FUNCTION
 *  This function writes a contiguous block of 32-bit values to DSP memory 
 *  starting at the specified address.
 *
 * RETURNS
 *  nothing
 *
 */
extern void gpakWriteDspMemory32(
    ADT_UInt32  DspId,          /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    DSP_Address DspAddress,     /* DSP's memory address of first word */
    ADT_UInt32  NumLongs,       /* number of contiguous 32-bit vals to write */
    ADT_UInt32 *pLongValues    /* pointer to array of 32-bit values to write */
    );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakWriteDspNoSwap32 - Write DSP packet memory without byte swapping.
 *
 * FUNCTION
 *  This function writes a contiguous block of words to DSP memory starting at
 *  the specified address.
 *
 * RETURNS
 *  nothing
 *
 */
extern void gpakWriteDspNoSwap32(
    ADT_UInt32 DspId,            /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    DSP_Address DspAddress,     /* DSP's memory address of first word */
    ADT_UInt32 NumLongs,        /* number of contiguous longs to write */
    ADT_UInt32 *pLongValues     /* pointer to array of long values to write */
    );
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakHostDelay - Delay for a fixed time interval.
 *
 * FUNCTION
 *  This function delays for a fixed time interval before returning. The time
 *  interval is the Host Port Interface sampling period when polling a DSP for
 *  replies to command messages.
 *
 * RETURNS
 *  nothing
 *
 */
extern void gpakHostDelay(ADT_UInt32 delayType);


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakLockAccess - Lock access to the specified DSP.
 *
 * FUNCTION
 *  This function aquires exclusive access to the specified DSP.
 *
 * RETURNS
 *  nothing
 *
 */
extern void gpakLockAccess(
    ADT_UInt32 DspId                   /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    );


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakUnlockAccess - Unlock access to the specified DSP.
 *
 * FUNCTION
 *  This function releases exclusive access to the specified DSP.
 *
 * RETURNS
 *  nothing
 *
 */
extern void gpakUnlockAccess(
    ADT_UInt32 DspId                   /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    );

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakCmdLockAccess - Lock access to the specified DSP Command Buffer.
 *
 * FUNCTION
 *  This function aquires exclusive access to the specified DSP.
 *
 * RETURNS
 *  nothing
 *
 */
extern void gpakCmdLockAccess(
    ADT_UInt32 DspId                   /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    );


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakCmdUnlockAccess - Unlock access to the specified DSP Command Buffer.
 *
 * FUNCTION
 *  This function releases exclusive access to the specified DSP.
 *
 * RETURNS
 *  nothing
 *
 */
extern void gpakCmdUnlockAccess(
    ADT_UInt32 DspId                   /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    );




/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakReadFile - Read a block of bytes from a G.PAK Download file.
 *
 * FUNCTION
 *  This function reads a contiguous block of bytes from a G.PAK Download file
 *  starting at the current file position.
 *
 * RETURNS
 *  The number of bytes read from the file.
 *   -1 indicates an error occurred.
 *    0 indicates all bytes have been read (end of file)
 *
 */
extern int gpakReadFile(
    GPAK_FILE_ID FileId,    /* G.PAK Download File Identifier */
    ADT_UInt8  *pBuffer,	/* pointer to buffer for storing bytes */
    ADT_UInt32  NumBytes   /* number of bytes to read */
    );


extern void gpakI16Swap (ADT_UInt32 *Buff, int BuffI8);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * TODO : write comments about what these guys do 
 */
extern int gpakDspCheckCommandBufferAvailable(ADT_UInt32 DspId);
extern int gpakDspCheckCommandReplyAvailable(ADT_UInt32 DspId);

void gpakCmdGetCmdReadAccess(ADT_UInt32 DspId);
void gpakCmdGetCmdWriteAccess(ADT_UInt32 DspId);

#ifdef __cplusplus
}
#endif



#endif  /* prevent multiple inclusion */
