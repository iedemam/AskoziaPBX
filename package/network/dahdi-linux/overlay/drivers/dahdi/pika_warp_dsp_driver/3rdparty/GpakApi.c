/*
 * Copyright (c) 2002 - 2007, Adaptive Digital Technologies, Inc.
 *
 * File Name: GpakApi.c
 *
 * Description:
 *   This file contains user API functions to communicate with DSPs executing
 *   G.PAK software. The file is integrated into the host processor connected
 *   to G.PAK DSPs via a Host Port Interface.
 *
 * Version: 4.1
 *
 * Revision History:
 *   10/17/01 - Initial release.
 *   06/15/04 - Tone type updates.
 *    1/2007  - Modified to allow both C54 and C64 access
 *
 */

#include <linux/kernel.h>
#include <asm/string.h>
#include "adt_typedef.h"
#include "GpakHpi.h"
#include "GpakApi.h"
#include "GpakEnum.h"
#ifndef _TMS320C55XX_
//#include <memory.h>
#endif
/* DSP Related values */
extern DSP_Address *getPktInBufrAddr  (int DspID, int channel);
extern DSP_Address *getPktOutBufrAddr (int DspID, int channel);
static void transferDlBlock_be (ADT_UInt32 DspId, DSP_Address Address, unsigned int NumWords);
static void transferDlBlock_le (ADT_UInt32 DspId, DSP_Address Address, unsigned int NumWords);
static void gpakPackCidPayload_be (ADT_UInt16 *Msg, ADT_UInt8 *pPayloadData, ADT_UInt16 PaylenI16);
static void gpakPackCidPayload_le (ADT_UInt16 *Msg, ADT_UInt8 *pPayloadData, ADT_UInt16 PaylenI16);
static void gpakI8Swap_be (ADT_UInt16 *in, ADT_UInt16 *out, ADT_UInt16 len);
static void gpakI8Swap_le (ADT_UInt16 *in, ADT_UInt16 *out, ADT_UInt16 len);
static void gpakI16Swap_be (ADT_UInt32 *Buff, int BuffI8);
static void gpakI16Swap_le (ADT_UInt32 *Buff, int BuffI8);

#define HighWord(a)            ((ADT_UInt16) ((a >> 16) & 0xffff))
#define LowWord(a)             ((ADT_UInt16) ( a        & 0xffff))
#define PackWords(hi,lo)       ((ADT_UInt32) ((((ADT_UInt32)hi) << 16) | (((ADT_UInt32)lo) & 0xffff)))

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
// Macro to reconstruct a 32-bit value from two 16-bit values.
// Parameter p32: 32-bit-wide destination
// Parameter p16: 16-bit-wide source array of length 2 words
#define RECONSTRUCT_LONGWORD(p32, p16) p32 = (DSP_Address)p16[0]<<16; \
								       p32 |= (DSP_Address)p16[1]

#define NO_ID_CHK 0
#define BYTE_ID 1
#define WORD_ID 2

extern DSP_Address ifBlkAddress;

//===================================================================================
//===================================================================================

//                  General access routines

//===================================================================================
//===================================================================================
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * ReadCircStruct 
 *
 * FUNCTION
 *  Transfers circular buffer structure from DSP to host memory 
 *
 * INPUTS
 *   DspId       - Dsp identifier 
 *   CircBufrAddr - Pointer to DSP memory circular buffer structure
 *
 * OUTPUT
 *   cBuff        - Pointer to host memory circular buffer structure
 *   I16Ready      - Size (16-bit words, DSP word padded) of data available for transfer
 *   I16Free       - Size (16-bit words, DSP word padded) of free space available for transfer
 *
 *   NOTE: Transfers are aligned to DSP word boundaries.  16-bit word sizes are
 *         rounded down to number of available 16-bit words available for transfer
 */
static void ReadCircStruct (ADT_UInt32 DspId, DSP_Address CircBufrAddr, CircBufr_t *cBuff,
	                  DSP_Word *I16sFree, DSP_Word *I16sReady) {

    DSP_Word    ElemReady;


	// Read circular buffer structure into host memory
   gpakReadDsp (DspId, CircBufrAddr,  CB_INFO_STRUCT_LEN, (DSP_Word *) cBuff);

	// Calculate elements (16-bit) available
	ElemReady = cBuff->PutIndex - cBuff->TakeIndex;
	if (cBuff->PutIndex < cBuff->TakeIndex)
		ElemReady += (cBuff->BufrSize);

	// Adjust
   *I16sReady = CircAvail (ElemReady);
	*I16sFree  = CircAvail (cBuff->BufrSize - (ElemReady + 2));
	return;
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * PurgeCircStruct
 *
 * FUNCTION
 *  Called on errors to purge circular buffer structure (TAKE = PUT)
 *
 * INPUTS
 *   DspId       - Dsp identifier 
 *   cBuff        - Pointer to host memory circular buffer structure
 *
 * OUTPUT
 *   CircBufrAddr - Pointer to DSP memory circular buffer structure
 *
 */

static void PurgeCircStruct (ADT_UInt32 DspId, DSP_Address CircBufrAddr, CircBufr_t *cBuff) {
	
    // Set take index to put index
    cBuff->TakeIndex = cBuff->PutIndex;
    gpakWriteDsp (DspId, CircBufrAddr + CB_BUFR_TAKE_INDEX, 1, &cBuff->TakeIndex);
	return;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * WriteCircBuffer - Write to a DSP circular buffer.
 *
 * FUNCTION
 *  Transfers block of 16 bits words from linear buffer to DSP circular buffer. 
 *  Put address is incremented by the number of 16-bit words written adjusting for buffer wrap.
 *
 * Inputs
 *   DspId       - Dsp identifier 
 *   cBuff       - Pointer to circular buffer structure
 *   hostBuff    - Address of host linear buffer
 *   I16Cnt      - Number of 16-bit words to transfer
 *   cBuffAddr   - non-Null update circular buffer on DSP
 *
 * Updates
 *   cBuff   - Before = Current state of circular buffer
 *           - After  = Put index updated
 *
 * RETURNS
 *  nothing
 *
 */
void WriteCircBuffer (ADT_UInt32 DspId, CircBufr_t *cBuff, DSP_Word *hostBuff, 
					       DSP_Word  I16Cnt, DSP_Address cBuffAddr) {

    DSP_Word    I16sBeforeWrap, I16sAfterWrap;
    DSP_Address PutAddr;
    ADT_UInt16 *pHostBuff;

    pHostBuff = (ADT_UInt16 *) hostBuff;
    PutAddr = CircAddress (cBuff->BufrBase, cBuff->PutIndex);
 
    //  If buffer wraps, divide into two transfers, else single transfer
    I16sBeforeWrap = cBuff->BufrSize - cBuff->PutIndex;   
    if (I16sBeforeWrap < I16Cnt) {
       I16sAfterWrap  = I16Cnt - I16sBeforeWrap;

       gpakWriteDsp (DspId, PutAddr,         CircTxSize (I16sBeforeWrap), (DSP_Word *) pHostBuff);
       gpakWriteDsp (DspId, cBuff->BufrBase, CircTxSize (I16sAfterWrap),  (DSP_Word *) &(pHostBuff[I16sBeforeWrap]));
       cBuff->PutIndex = I16sAfterWrap;
    } else {

        gpakWriteDsp (DspId, PutAddr,        CircTxSize (I16Cnt), (DSP_Word *) pHostBuff);
        if (I16Cnt == I16sBeforeWrap)
            cBuff->PutIndex = 0;  // At exact end of buffer. Next write at beginning
        else
            cBuff->PutIndex += I16Cnt;
    }
    if (cBuffAddr)
		gpakWriteDsp (DspId, cBuffAddr + CB_BUFR_PUT_INDEX,  1, &cBuff->PutIndex);

    return;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * ReadCircBuffer - Read from a DSP circular buffer.
 *
 * FUNCTION
 *  Tranfers block of 16-bit words from a DSP circular buffer to a linear buffer. The Take
 *  address is incremented by the number of bytes read adjusting for buffer wrap.
 *
 * Inputs
 *   DspId       - Dsp identifier 
 *   cBuff       - Pointer to circular buffer structure
 *   hostBuff    - Address of host linear buffer
 *   I16Cnt      - Number of 16-bit words to transfer
 *   cBuffAddr   - non-Null update circular buffer on DSP
 *
 * Updates
 *   cBuff   - Before = Current state of circular buffer
 *           - After  = Take index updated
 *
 * RETURNS
 *  nothing
 *
 */
void ReadCircBuffer (ADT_UInt32 DspId,  CircBufr_t *cBuff, DSP_Word *hostBuff,
					 DSP_Word I16Cnt, DSP_Address cBuffAddr) {

    ADT_UInt16 *pHostBuff;
    DSP_Word    I16sBeforeWrap, I16sAfterWrap;
    DSP_Address TakeAddr;

    pHostBuff = (ADT_UInt16 *) hostBuff;

    TakeAddr = CircAddress (cBuff->BufrBase, cBuff->TakeIndex);

    //  If buffer wraps, divide into two transfers, else single transfer
    I16sBeforeWrap = cBuff->BufrSize - cBuff->TakeIndex;
    if (I16sBeforeWrap < I16Cnt) {
       I16sAfterWrap  = I16Cnt - I16sBeforeWrap;

       gpakReadDsp (DspId, TakeAddr,        CircTxSize (I16sBeforeWrap), (DSP_Word *) pHostBuff);
       gpakReadDsp (DspId, cBuff->BufrBase, CircTxSize (I16sAfterWrap),  (DSP_Word *) &(pHostBuff[I16sBeforeWrap]));
       cBuff->TakeIndex = I16sAfterWrap;

    } else   {

       gpakReadDsp (DspId, TakeAddr,        CircTxSize (I16Cnt), (DSP_Word *) pHostBuff);
       if (I16Cnt == I16sBeforeWrap)
           cBuff->TakeIndex = 0;  // At exact end of buffer. Next write at beginning
       else
           cBuff->TakeIndex += I16Cnt;
    }
    if (cBuffAddr)
		gpakWriteDsp (DspId, cBuffAddr + CB_BUFR_TAKE_INDEX,  1, &cBuff->TakeIndex);
    return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * WriteCircBufferNoSwap - Write to a DSP circular buffer without byte swapping.
 *
 * FUNCTION
 *  Transfers block of 16 bits words from linear buffer to DSP circular buffer. 
 *  Put address is incremented by the number of 16-bit words written adjusting for buffer wrap.
 *
 * Inputs
 *   DspId       - Dsp identifier 
 *   cBuff       - Pointer to circular buffer structure
 *   hostBuff    - Address of host linear buffer
 *   I16Cnt      - Number of 16-bit words to transfer
 *   cBuffAddr   - non-Null update circular buffer on DSP
 *
 * Updates
 *   cBuff   - Before = Current state of circular buffer
 *           - After  = Put index updated
 *
 * RETURNS
 *  nothing
 *
 */
static void WriteCircBufferNoSwap (ADT_UInt32 DspId, CircBufr_t *cBuff, DSP_Word *hostBuff, 
            DSP_Word  I16Cnt, DSP_Address cBuffAddr) {
 
    DSP_Word    I16sBeforeWrap, I16sAfterWrap;
    DSP_Address PutAddr;
    ADT_UInt16 *pHostBuff;
 
    pHostBuff = (ADT_UInt16 *) hostBuff;
    PutAddr = CircAddress (cBuff->BufrBase, cBuff->PutIndex);
 
    //  If buffer wraps, divide into two transfers, else single transfer
    I16sBeforeWrap = cBuff->BufrSize - cBuff->PutIndex;   
    if (I16sBeforeWrap < I16Cnt) {
       I16sAfterWrap  = I16Cnt - I16sBeforeWrap;
 
       gpakWriteDspNoSwap (DspId, PutAddr,         CircTxSize (I16sBeforeWrap), (DSP_Word *) pHostBuff);
       gpakWriteDspNoSwap (DspId, cBuff->BufrBase, CircTxSize (I16sAfterWrap),  (DSP_Word *) &(pHostBuff[I16sBeforeWrap]));
       cBuff->PutIndex = I16sAfterWrap;
    } else {
 
        gpakWriteDspNoSwap (DspId, PutAddr,        CircTxSize (I16Cnt), (DSP_Word *) pHostBuff);
        if (I16Cnt == I16sBeforeWrap)
            cBuff->PutIndex = 0;  // At exact end of buffer. Next write at beginning
        else
            cBuff->PutIndex += I16Cnt;
    }
    if (cBuffAddr)
        gpakWriteDsp (DspId, cBuffAddr + CB_BUFR_PUT_INDEX,  1, &cBuff->PutIndex);
 
    return;
}
 

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * ReadCircBufferNoSwap - Read from a DSP circular buffer with swapping.
 *
 * FUNCTION
 *  Tranfers block of 16-bit words from a DSP circular buffer to a linear buffer. The Take
 *  address is incremented by the number of bytes read adjusting for buffer wrap.
 *
 * Inputs
 *   DspId       - Dsp identifier 
 *   cBuff       - Pointer to circular buffer structure
 *   hostBuff    - Address of host linear buffer
 *   I16Cnt      - Number of 16-bit words to transfer
 *   cBuffAddr   - non-Null update circular buffer on DSP
 *
 * Updates
 *   cBuff   - Before = Current state of circular buffer
 *           - After  = Take index updated
 *
 * RETURNS
 *  nothing
 *
 */
static void ReadCircBufferNoSwap (ADT_UInt32 DspId,  CircBufr_t *cBuff, DSP_Word *hostBuff,
					 DSP_Word I16Cnt, DSP_Address cBuffAddr) {

    ADT_UInt16 *pHostBuff;
    DSP_Word    I16sBeforeWrap, I16sAfterWrap;
    DSP_Address TakeAddr;

    pHostBuff = (ADT_UInt16 *) hostBuff;

    TakeAddr = CircAddress (cBuff->BufrBase, cBuff->TakeIndex);

    //  If buffer wraps, divide into two transfers, else single transfer
    I16sBeforeWrap = cBuff->BufrSize - cBuff->TakeIndex;
    if (I16sBeforeWrap < I16Cnt) {
       I16sAfterWrap  = I16Cnt - I16sBeforeWrap;

       gpakReadDspNoSwap (DspId, TakeAddr,        CircTxSize (I16sBeforeWrap),  pHostBuff);
       gpakReadDspNoSwap (DspId, cBuff->BufrBase, CircTxSize (I16sAfterWrap),   &(pHostBuff[I16sBeforeWrap]));
       cBuff->TakeIndex = I16sAfterWrap;

    } else   {

       gpakReadDspNoSwap (DspId, TakeAddr,        CircTxSize (I16Cnt), pHostBuff);
       if (I16Cnt == I16sBeforeWrap)
           cBuff->TakeIndex = 0;  // At exact end of buffer. Next write at beginning
       else
           cBuff->TakeIndex += I16Cnt;
    }
    if (cBuffAddr)
		gpakWriteDsp (DspId, cBuffAddr + CB_BUFR_TAKE_INDEX,  1, &cBuff->TakeIndex);
    return;
}
 


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * CheckDspReset - Check if the DSP was reset.
 *
 * FUNCTION
 *  Determines if the DSP was reset and is ready. If reset
 *  occurred, it reads interface parameters and calculates DSP addresses.
 *
 * Inputs
 *   DspId - Dsp identifier 
 *
 * RETURNS
 *  -1 = DSP is not ready.
 *   0 = Reset did not occur.
 *   1 = Reset occurred.
 *
 */
int CheckDspReset (ADT_UInt32 DspId) {

    DSP_Address IfBlockPntr;   /* Interface Block pointer */
    DSP_Address PktBufrMem;    /* address of Packet Buffer */

    DSP_Word    DspChannels;   /* number of DSP channels */
    DSP_Word    DspStatus;     /* DSP initialization status */
    DSP_Word    VersionId;     /* The version ID */
    DSP_Word    Temp;

    ADT_UInt16 i;

    /* Read Interface Block Address */
    IfBlockPntr = pDspIfBlk[DspId];
    if (IfBlockPntr == 0)
    gpakReadDspAddr (DspId, ifBlkAddress, 1, &IfBlockPntr);
    /* Pointer is zero -> return not ready. */
    if (IfBlockPntr == 0 || MAX_DSP_ADDRESS < IfBlockPntr) {
        DSPError [DspId] = DSPNullIFAddr;
        pDspIfBlk[DspId] = 0;
        MaxChannels[DspId] = 0;
        return -1;  // DSP not ready
    }

    /* Read the DSP's Status. from the Interface block */
    gpakReadDsp (DspId, IfBlockPntr + DSP_STATUS_OFFSET, 1, &DspStatus);

    if ((DspStatus == DSP_INIT_STATUS) ||
       ((DspStatus == HOST_INIT_STATUS) && (pDspIfBlk[DspId] == 0))) {

       /* DSP was reset, read the DSP's interface parameters (CmdMsgLen and MaxChannels)
          and calculate DSP addresses. (per channel PacketIn and PacketOut) */
        pDspIfBlk[DspId] = IfBlockPntr;

        gpakReadDsp (DspId, IfBlockPntr + MAX_CMD_MSG_LEN_OFFSET, 1, &Temp);
        MaxCmdMsgLen[DspId] = Temp * 4;    // Convert longs to bytes
        gpakReadDspAddr (DspId, IfBlockPntr + EVENT_MSG_PNTR_OFFSET,  1, &(pEventFifoAddress[DspId]));
        gpakReadDsp (DspId, IfBlockPntr + NUM_CHANNELS_OFFSET,    1, &DspChannels);
        if (DspChannels > MaxChannelAlloc)
            MaxChannels[DspId] = MaxChannelAlloc;
        else
            MaxChannels[DspId] = DspChannels;

        // Per channel packet in and packet out buffers
        gpakReadDspAddr (DspId, IfBlockPntr + PKT_BUFR_MEM_OFFSET, 1, &PktBufrMem);
        for (i = 0; i < MaxChannels[DspId]; i++) {
#if (DSP_WORD_SIZE_I8 == 4)
// in 64x DSP, the data access is in byte address
// Circ buffer word length is in 32bit word, so we have LEN*4
            *getPktOutBufrAddr (DspId, i) = PktBufrMem;
            *getPktInBufrAddr  (DspId, i)  = PktBufrMem + CB_INFO_STRUCT_LEN*4;
            PktBufrMem           += (CB_INFO_STRUCT_LEN*8);
#else
// in 55x DSP, the data access is in 16bit word address
// Circ buffer word length is in 16bit word, so we have LEN
            *getPktOutBufrAddr (DspId, i) = PktBufrMem;
            *getPktInBufrAddr  (DspId, i)  = PktBufrMem + CB_INFO_STRUCT_LEN;
            PktBufrMem           += (CB_INFO_STRUCT_LEN*2);
#endif
        }

        // 4_2 -----------------------------------------------------------------
        /* write API Version ID to DSP provided DSP version is compatible */
        gpakReadDsp (DspId, IfBlockPntr + VERSION_ID_OFFSET, 1, &VersionId);
        if (VersionId >= MIN_DSP_VERSION_ID) {
            VersionId = API_VERSION_ID;
            gpakWriteDsp (DspId, IfBlockPntr + API_VERSION_ID_OFFSET, 1, &VersionId);
        }
        // -------------------------------------------------------------------

        /* Set the DSP Status to indicate the host recognized the reset. */
        DspStatus = HOST_INIT_STATUS;
        gpakWriteDsp (DspId, IfBlockPntr + DSP_STATUS_OFFSET, 1, &DspStatus);
        return 1;  // Dsp ready, reset occured
    }
    
    if (DspStatus != HOST_INIT_STATUS) {
        DSPError [DspId] = DSPHostStatusError;
        pDspIfBlk[DspId] = 0;
        MaxChannels[DspId] = 0;
        return -1;  // DSP not ready
    }

    if (pDspIfBlk[DspId] == 0) {
       DSPError [DspId] = DSPNullIFAddr;
       MaxChannels[DspId] = 0;
       return -1;  // DSP not ready
    }

    return 0;   // DSP ready
}

/* check if the command buffer is available */
int gpakDspCheckCommandBufferAvailable(ADT_UInt32 DspId)
{    
    DSP_Word    CmdMsgLength;  /* Cmd message length (Dsp words) */
    DSP_Address DspOffset;   

    /* Check if the DSP was reset and is ready. */
    if (CheckDspReset(DspId) == -1)
        return (-1);  // Failure

    DspOffset = pDspIfBlk[DspId];

    /* Verify Command message is not pending */
    gpakReadDsp (DspId, DspOffset + CMD_MSG_LEN_OFFSET, 1,  &CmdMsgLength);
    if (CmdMsgLength != 0)
        return (0);  // DSP busy	

    return (1);
}

int gpakDspCheckCommandReplyAvailable(ADT_UInt32 DspId)
{
    DSP_Word    MsgBytes;  /* Cmd message length (Dsp words) */
    DSP_Address DspOffset;   

    /* Check if the DSP was reset and is ready. */
    if (CheckDspReset(DspId) == -1)
        return (-1);  // Failure

    DspOffset = pDspIfBlk[DspId];

    /* Check if reply message is there */
    gpakReadDsp (DspId, DspOffset + REPLY_MSG_LEN_OFFSET, 1,  &MsgBytes);
    if (MsgBytes == 0)
        return (0);  // DSP Response Not yet ready

    return (1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * WriteDspCmdMessage - Write a Host Command/Request message to DSP.
 *
 * FUNCTION
 *  Writes a Host Command/Request message into DSP memory and
 *  informs the DSP of the presence of the message.
 *
 * Inputs
 *   DspId    - Dsp identifier 
 *   pMessage - Message buffer (16-bit words, 32-bit aligned)
 *   ElemCnt  - Number of 16-bit words
 *
 * RETURNS
 *  -1 = Unable to write message (msg len or DSP Id invalid or DSP not ready)
 *   0 = Temporarily unable to write message (previous Cmd Msg busy)
 *   1 = Message written successfully
 *
 */
int WriteDspCmdMessage (ADT_UInt32 DspId, ADT_UInt32 *pMessage, ADT_UInt32 ElemCnt_I16) {

    DSP_Word    ByteCnt;
    DSP_Word    CmdMsgLength;  /* Cmd message length (Dsp words) */
    DSP_Address MsgBuf;        /* message buffer pointer */
    DSP_Address DspOffset;   
    /* Check if the DSP was reset and is ready. */
    if (CheckDspReset(DspId) == -1)
        return (-1);  // Failure

	ByteCnt = (DSP_Word) (ElemCnt_I16 * 2);

    /* Make sure the message length is valid. */
    if (ByteCnt < 1 || MaxCmdMsgLen[DspId] < ByteCnt) {
        DSPError[DspId] = DSPCmdLengthError;
        return (-1);  // Failure
    }

    DspOffset = pDspIfBlk[DspId];

    /* Verify Command message is not pending */
    gpakReadDsp (DspId, DspOffset + CMD_MSG_LEN_OFFSET, 1,  &CmdMsgLength);
    if (CmdMsgLength != 0)
        return (0);  // DSP busy	

    /* Purge previous reply message.  NOTE: CmdMessageLength = 0 at this point */
    gpakWriteDsp (DspId, DspOffset + REPLY_MSG_LEN_OFFSET, 1, &CmdMsgLength);

    /* Copy the Command message into DSP memory. */
    gpakReadDspAddr  (DspId, DspOffset + CMD_MSG_PNTR_OFFSET, 1, &MsgBuf);

#if (DSP_WORD_SIZE_I8 == 4)
    if (HostBigEndian)
        gpakI16Swap_be (pMessage, ByteCnt);
    else
        gpakI16Swap_le (pMessage, ByteCnt);
#endif

    gpakWriteDsp (DspId, MsgBuf, BytesToTxUnits (ByteCnt),  (DSP_Word *) pMessage);
 
    /* Notify DSP of Command message */
    CmdMsgLength = (DSP_Word) ElemCnt_I16;
    gpakWriteDsp (DspId, DspOffset + CMD_MSG_LEN_OFFSET, 1, &CmdMsgLength);

    return (1);  // Success
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * ReadDspReplyMessage - Read a DSP Reply message from DSP.
 *
 * FUNCTION
 *  Reads a DSP Reply message from DSP memory.
 *
 * Inputs
 *   DspId    - Dsp identifier 
 *
 *  Outputs
 *   pMessage - Message buffer (16-bit words, 32-bit aligned)
 *
 *  Update
 *   pElemCnt  - Pointer to number of 16-bit words 
 *               Before = buffer size
 *               After  = actual reply message
 *
 * RETURNS
 *  -1 = Unable to write message (msg len or DSP Id invalid or DSP not ready)
 *   0 = No message available (DSP Reply message empty)
 *   1 = Message read successfully (message and length stored in variables)
 *
 */
int ReadDspReplyMessage (ADT_UInt32 DspId, ADT_UInt32 *pMessage, ADT_UInt32 *pElemCnt) {

    DSP_Word MsgBytes;        /* message length */
    DSP_Address MsgBuf;     /* message buffer pointer */
    DSP_Address DspOffset;   

    /* Read transfer count (in DSP bytes) */
    DspOffset = pDspIfBlk[DspId];
    MsgBytes = 0;
    gpakReadDsp (DspId, DspOffset + REPLY_MSG_LEN_OFFSET, 1,  &MsgBytes);
    if (MsgBytes == 0)
        return (0);

    
    /* Verify reply buffer is large enough  */
    if (*pElemCnt * 2 < MsgBytes) {  // counts converted to bytes
       DSPError[DspId] = DSPRplyLengthError;
       return (-1);
    }

    /* Copy Reply message from DSP memory. */
    gpakReadDspAddr (DspId, DspOffset + REPLY_MSG_PNTR_OFFSET, 1,  &MsgBuf);

    gpakReadDsp (DspId, MsgBuf, BytesToTxUnits (MsgBytes), (DSP_Word *) pMessage);

#if (DSP_WORD_SIZE_I8 == 4)
    if (HostBigEndian)
        gpakI16Swap_be (pMessage, MsgBytes);
    else
        gpakI16Swap_le (pMessage, MsgBytes);
#endif

    /* return message length in 16-bit words */
    *pElemCnt = (ADT_UInt32) (MsgBytes / 2);

    /* Notify DSP that reply buffer is free */
    MsgBytes = 0;
    gpakWriteDsp (DspId, DspOffset + REPLY_MSG_LEN_OFFSET, 1, &MsgBytes);

    /* Return with an indication the message was read. */
    return (1);
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * VerifyReply - Verify the reply message is correct for the command sent.
 *
 * FUNCTION
 *  Verifies correct reply message content for the command that was just sent.
 *
 * Inputs
 *   pMsgBufr - Reply message buffer
 *   IdValue  - Expected id
 *   IdMode   -  0 = No id checking
 *               1 = ID in High byte of reply[1]
 *       Otherwise = ID in reply[1]
 *
 * RETURNS
 *  0 = Incorrect
 *  1 = Correct
 *
 */
int VerifyReply (ADT_UInt16 *pMsgBufr, ADT_UInt32 IdMode, ADT_UInt16 IdValue) {

    ADT_UInt16 ReturnID;

    if (IdMode == NO_ID_CHK) return 1;

    ReturnID = pMsgBufr[1];

    if (IdMode == BYTE_ID) ReturnID = (ADT_UInt16) Byte1 (ReturnID);
    return (ReturnID == IdValue);
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * TransactCmd - Send a command to the DSP and receive it's reply.
 *
 * FUNCTION
 *  Sends the specified command to the DSP and receives the DSP's  reply.
 *
 * Inputs
 *   DspId       - Dsp identifier 
 *   ElemCnt     - Size of command in 16-bit words
 *   ReplyType   - Expected reply type (ChannelStatus, SystemParameterStatus, ...)
 *   ReplyElemCnt - Expected size of reply in 16-bit words
 *   IdMode  - 0 = No id in reply
 *             1 = ID in High byte of reply[1]
 *             2 = ID in reply[1]
 *   IdValue - Expected value for returned ID
 *
 * Updates
 *    pMsgBufr - Before. Cmd buffer
 *               After.  Reply buffer
 *
 * RETURNS
 *  Length (16-bit words) of reply message (0 = Failure)
 *
 */
static ADT_UInt32  TransactCmd (ADT_UInt32 DspId,         ADT_UInt32 *pMsgBufr,
				 			    ADT_UInt32 ElemCnt,      ADT_UInt8   ExpectedReply,
								ADT_UInt32 MaxRplyElem,  ADT_UInt32 IdMode,      ADT_UInt16 IdValue) {

    int FuncStatus;              /* function status */
    unsigned int LoopCount;               /* wait loop counter */

    ADT_UInt32 ReplyElemCnt;     /* Reply message length (16-bits words) */
    ADT_UInt8  ReplyValue;       /* Reply type code */

    ADT_UInt16 *pWordBufr;
    ADT_UInt32 RetValue;         /* return status value */
    /* Default the return value to indicate a failure. */
    RetValue = 0;
    DSPError [DspId] = DSPSuccess;
    gpakCmdLockAccess (DspId);

    /* Write the command message to the DSP. */
    LoopCount = 0;
    while ((FuncStatus = WriteDspCmdMessage (DspId, pMsgBufr, ElemCnt)) != 1)   {
        if (FuncStatus == -1) break;
        if (++LoopCount > MaxWaitLoops) {
            DSPError[DspId] = DSPCmdTimeout;
            break;
        }
        gpakHostDelay(0);	// delay until the write buffer is available
    }

    if (FuncStatus == 1)  {  // Successful command transfer to DSP

        // Read the reply from the DSP.  Keep trying until successful or too many tries.
        for (LoopCount = 0; LoopCount < MaxWaitLoops; LoopCount++)  {
            ReplyElemCnt = MSG_BUFFER_ELEMENTS;
            FuncStatus = ReadDspReplyMessage (DspId, pMsgBufr, &ReplyElemCnt);
            if (FuncStatus == 0)  {
               gpakHostDelay(1);	// delay until the read buffer is available
               continue;  // Retry
            }
            
            if (FuncStatus == -1) break;

            pWordBufr = (ADT_UInt16 *) pMsgBufr;

            ReplyValue = Byte1 (pWordBufr[0]);
            if (ReplyValue == MSG_NULL_REPLY) 
               DSPError[DspId] = DSPRplyValue;
               
            else if (MaxRplyElem < ReplyElemCnt)
               DSPError[DspId] = DSPRplyLengthError;

            else if (!VerifyReply (pWordBufr, IdMode, IdValue) || (ReplyValue != ExpectedReply))
               DSPError[DspId] = DSPRplyValue;
            
            else {
               DSPError [DspId] = DSPSuccess;
               RetValue = ReplyElemCnt;
            }
            break;
        }
        if (FuncStatus == 0) 
            DSPError[DspId] = DSPRplyTimeout;
    }

    gpakCmdUnlockAccess (DspId);

    return RetValue;  // Length of reply message
}


static void DSPSync (ADT_UInt32 DspId) {

    if (MaxDspCores <= DspId)
        return;

    gpakLockAccess   (DspId);
    CheckDspReset    (DspId);
    gpakUnlockAccess (DspId);
    return;
}

//}==================================================================================
//{                  Status routines
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakGetSystemConfig - Read a DSP's System Configuration.
 *
 * FUNCTION
 *  Reads a DSP's System Configuration information.
 *
 * Inputs
 *   DspId       - Dsp identifier 
 *  sysCfg   - System configuration data structure pointer
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
void gpakParseSystemConfig (ADT_UInt16 *Msg, GpakSystemConfig_t *sysCfg) {

    /* Extract the System Configuration information from the message. */
	sysCfg->GpakVersionId   = PackWords (Msg[1], Msg[2]);
    sysCfg->MaxChannels     = (ADT_UInt16) Byte1 (Msg[3]);
    sysCfg->ActiveChannels  = (ADT_UInt16) Byte0 (Msg[3]);
    sysCfg->CfgEnableFlags  = PackWords (Msg[4], Msg[5]);
    sysCfg->PacketProfile   = (GpakProfiles) Msg[6];
    sysCfg->RtpTonePktType  = (GpakRTPPkts) Msg[7];
    sysCfg->CfgFramesChans  = Msg[8];
    sysCfg->Port1NumSlots   = Msg[9];
    sysCfg->Port2NumSlots   = Msg[10];
    sysCfg->Port3NumSlots   = Msg[11];
    sysCfg->Port1SupSlots   = Msg[12];
    sysCfg->Port2SupSlots   = Msg[13];
    sysCfg->Port3SupSlots   = Msg[14];
    sysCfg->PcmMaxTailLen   = Msg[15];
    sysCfg->PcmMaxFirSegs   = Msg[16];
    sysCfg->PcmMaxFirSegLen = Msg[17];
    sysCfg->PktMaxTailLen   = Msg[18];
    sysCfg->PktMaxFirSegs   = Msg[19];
    sysCfg->PktMaxFirSegLen = Msg[20];
    sysCfg->MaxConferences  = (ADT_UInt16) Byte1 (Msg[21]);
    sysCfg->MaxNumConfChans = (ADT_UInt16) Byte0 (Msg[21]);
    sysCfg->SelfTestStatus  = Msg[22];
    sysCfg->NumPcmEcans     = Msg[23];
    sysCfg->NumPktEcans     = Msg[24];
    sysCfg->MaxToneDetTypes = Msg[25];
    sysCfg->AECInstances    = Msg[26];
    sysCfg->AECTailLen      = Msg[27];
}

gpakGetSysCfgStatus_t gpakGetSystemConfig (ADT_UInt32 DspId, GpakSystemConfig_t *sysCfg) {

    ADT_UInt32 MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;



    if (MaxDspCores <= DspId)
        return GscInvalidDsp;

    Msg    = (ADT_UInt16 *) MsgBuffer;

    Msg[0] = MSG_SYS_CONFIG_RQST << 8;   // Request Message

    if (!TransactCmd (DspId, MsgBuffer, 1, MSG_SYS_CONFIG_REPLY, 28, NO_ID_CHK, 0))
        return GscDspCommFailure;

    gpakParseSystemConfig (Msg, sysCfg);
    return GscSuccess;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakReadSystemParms - Read a DSP's System Parameters.
 *
 * FUNCTION
 *  Reads a DSP's System Parameters information.
 *
 * Inputs
 *   DspId       - Dsp identifier 
 *  pSysParams   - System parameters data structure pointer
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
void gpakParseSystemParms (ADT_Int16 *Msg, GpakSystemParms_t *sysPrms) {

    ADT_Int16 temp;
   
    /* Extract the System Parameters information from the message. */
    sysPrms->AGC_A.TargetPower       = Msg[1];
    sysPrms->AGC_A.LossLimit         = Msg[2];
    sysPrms->AGC_A.GainLimit         = Msg[3];
    sysPrms->AGC_A.LowSignal         = Msg[34];

    sysPrms->VadNoiseFloor        = Msg[4];
    sysPrms->VadHangTime          = Msg[5];
    sysPrms->VadWindowSize        = Msg[6];

    temp                            = Msg[7]; 
    sysPrms->PcmEc.NlpType         = (ADT_Int16) (temp & 0x000F);
    sysPrms->PcmEc.AdaptEnable     = (ADT_Int16) ((temp >> 4) & 0x0001);
    sysPrms->PcmEc.G165DetEnable   = (ADT_Int16) ((temp >> 5) & 0x0001);
    sysPrms->PcmEc.MixedFourWireMode = (ADT_Int16) ((temp >> 6) & 0x0001);
    sysPrms->PcmEc.ReconvergenceCheckEnable = (ADT_Int16) ((temp >> 7) & 0x0001);

    sysPrms->PktEc.NlpType         = (ADT_Int16) ((temp >> 8) & 0x000F);
    sysPrms->PktEc.AdaptEnable     = (ADT_Int16) ((temp >> 12) & 0x0001);
    sysPrms->PktEc.G165DetEnable   = (ADT_Int16) ((temp >> 13) & 0x0001);
    sysPrms->PktEc.MixedFourWireMode = (ADT_Int16) ((temp >> 14) & 0x0001);
    sysPrms->PktEc.ReconvergenceCheckEnable = (ADT_Int16) ((temp >> 15) & 0x0001);

    sysPrms->PcmEc.TailLength      = Msg[8];
    sysPrms->PcmEc.DblTalkThresh   = Msg[9];
    sysPrms->PcmEc.NlpThreshold    = Msg[10];
    sysPrms->PcmEc.NlpUpperLimitThreshConv = Msg[11];
    sysPrms->PcmEc.NlpUpperLimitThreshPreconv= Msg[12];
    sysPrms->PcmEc.NlpMaxSupp        = Msg[13];
    sysPrms->PcmEc.CngThreshold    = Msg[14];
    sysPrms->PcmEc.AdaptLimit      = Msg[15];
    sysPrms->PcmEc.CrossCorrLimit  = Msg[16];
    sysPrms->PcmEc.NumFirSegments  = Msg[17];
    sysPrms->PcmEc.FirSegmentLen   = Msg[18];
    sysPrms->PcmEc.FirTapCheckPeriod = Msg[19];
    sysPrms->PcmEc.MaxDoubleTalkThres = Msg[46];
  
    sysPrms->PktEc.TailLength      = Msg[20];
    sysPrms->PktEc.DblTalkThresh   = Msg[21];
    sysPrms->PktEc.NlpThreshold    = Msg[22];
    sysPrms->PktEc.NlpUpperLimitThreshConv = Msg[23];
    sysPrms->PktEc.NlpUpperLimitThreshPreconv= Msg[24];
    sysPrms->PktEc.NlpMaxSupp        = Msg[25];
    sysPrms->PktEc.CngThreshold    = Msg[26];
    sysPrms->PktEc.AdaptLimit      = Msg[27];
    sysPrms->PktEc.CrossCorrLimit  = Msg[28];
    sysPrms->PktEc.NumFirSegments  = Msg[29];
    sysPrms->PktEc.FirSegmentLen   = Msg[30];
    sysPrms->PktEc.FirTapCheckPeriod = Msg[31];
    sysPrms->PktEc.MaxDoubleTalkThres = Msg[47];

    sysPrms->ConfNumDominant      = Msg[32];
    sysPrms->MaxConfNoiseSuppress = Msg[33];

    sysPrms->AGC_B.TargetPower       = Msg[35];
    sysPrms->AGC_B.LossLimit         = Msg[36];
    sysPrms->AGC_B.GainLimit         = Msg[37];
    sysPrms->AGC_B.LowSignal         = Msg[38];

    // 4_2 ---------------------------------------
    sysPrms->Cid.formatBurstFlag    = Msg[39] & 1;
    sysPrms->Cid.numSeizureBytes    = Msg[40];
    sysPrms->Cid.numMarkBytes       = Msg[41];
    sysPrms->Cid.numPostambleBytes  = Msg[42];
    sysPrms->Cid.fskType            = Msg[43];
    sysPrms->Cid.fskLevel           = Msg[44];
    sysPrms->Cid.msgType            = Msg[45];
    sysPrms->VadReport              = (Msg[39] & 2) >> 1;
    // -------------------------------------------

}


gpakReadSysParmsStatus_t gpakReadSystemParms (ADT_UInt32 DspId, GpakSystemParms_t *sysPrms) {

    ADT_UInt32 MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_Int16  *Msg;


    if (MaxDspCores <= DspId)
        return RspInvalidDsp;

    Msg = (ADT_Int16 *) MsgBuffer;

    Msg[0] = MSG_READ_SYS_PARMS << 8;

    if (!TransactCmd (DspId, MsgBuffer, 1, MSG_READ_SYS_PARMS_REPLY, 48, NO_ID_CHK, 0))
        return RspDspCommFailure;

    gpakParseSystemParms (Msg, sysPrms);

    
    /* Return with an indication that System Parameters info was obtained. */
    return RspSuccess;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakWriteSystemParms - Write a DSP's System Parameters.
 *
 * FUNCTION
 *  Writes selective System Parameters to a DSP.
 *
 * Inputs
 *   DspId       - Dsp identifier 
 *  sysPrms    - System parameters data structure pointer
 *  AgcUpdate    - 1  ==  Write A-device AGC parameters, 2 == Write B-device AGC parameters
 *  VadUpdate    - Non-zero .  Write VAD parameters
 *  PcmEcUpdate  - Non-zero .  Write Pcm Echo canceller parameters
 *  PktEcUpdate  - Non-zero .  Write Pkt Echo canceller parameters
 *  ConfUpdate   - Non-zero .  Write Conference parameters
 *
 * Outputs
 *   pStatus     - System parms status from DSP
 * 
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
ADT_UInt32 gpakFormatSystemParms (ADT_Int16 *Msg, GpakSystemParms_t *sysPrms,
    ADT_UInt16 AgcUpdate,   ADT_UInt16 VadUpdate, ADT_UInt16 PcmEcUpdate,
    ADT_UInt16 PktEcUpdate, ADT_UInt16 ConfUpdate, ADT_UInt16 CIDUpdate) {

    ADT_UInt32 MsgLenI16;

    /* Build the Write System Parameters message. */
    Msg[0] = MSG_WRITE_SYS_PARMS << 8;
    Msg[1] = 0;    

    if (AgcUpdate  & 1) {
        Msg[1] |= 1;
        Msg[2] = sysPrms->AGC_A.TargetPower;
        Msg[3] = sysPrms->AGC_A.LossLimit;
        Msg[4] = sysPrms->AGC_A.GainLimit;
        Msg[35] = sysPrms->AGC_A.LowSignal;
    }
    if (AgcUpdate  & 2) {
        Msg[1] |= 0x0020;
        Msg[36] = sysPrms->AGC_B.TargetPower;
        Msg[37] = sysPrms->AGC_B.LossLimit;
        Msg[38] = sysPrms->AGC_B.GainLimit;
        Msg[39] = sysPrms->AGC_B.LowSignal;
    }

    if (VadUpdate)    {
        Msg[1] |= 2;
        Msg[5] = sysPrms->VadNoiseFloor;
        Msg[6] = sysPrms->VadHangTime;
        Msg[7] = sysPrms->VadWindowSize;
    }
    if (PcmEcUpdate || PktEcUpdate)   {
        Msg[8] = (ADT_Int16)
                   ((sysPrms->PcmEc.NlpType & 0x000F) |
                   ((sysPrms->PcmEc.AdaptEnable << 4) & 0x0010) |
                   ((sysPrms->PcmEc.G165DetEnable << 5) & 0x0020) |
                   ((sysPrms->PcmEc.MixedFourWireMode << 6) & 0x0040) |
                   ((sysPrms->PcmEc.ReconvergenceCheckEnable << 7) & 0x0080) |
                   ((sysPrms->PktEc.NlpType << 8) & 0x0F00) |
                   ((sysPrms->PktEc.AdaptEnable << 12) & 0x1000) |
                   ((sysPrms->PktEc.G165DetEnable << 13) & 0x2000) |
                   ((sysPrms->PktEc.MixedFourWireMode << 14) & 0x4000) |
                   ((sysPrms->PktEc.ReconvergenceCheckEnable << 15) & 0x8000));
    }
    if (PcmEcUpdate)    {
        Msg[1] |= 4;
        Msg[9]  = sysPrms->PcmEc.TailLength;
        Msg[10] = sysPrms->PcmEc.DblTalkThresh;
        Msg[11] = sysPrms->PcmEc.NlpThreshold;
        Msg[12] = sysPrms->PcmEc.NlpUpperLimitThreshConv;
        Msg[13] = sysPrms->PcmEc.NlpUpperLimitThreshPreconv;
        Msg[14] = sysPrms->PcmEc.NlpMaxSupp;

        Msg[15] = sysPrms->PcmEc.CngThreshold;
        Msg[16] = sysPrms->PcmEc.AdaptLimit;
        Msg[17] = sysPrms->PcmEc.CrossCorrLimit;
        Msg[18] = sysPrms->PcmEc.NumFirSegments;
        Msg[19] = sysPrms->PcmEc.FirSegmentLen;
        Msg[20] = sysPrms->PcmEc.FirTapCheckPeriod;
        Msg[47] = sysPrms->PcmEc.MaxDoubleTalkThres;
    }
    if (PktEcUpdate)    {
        Msg[1] |= 8;
        Msg[21] = sysPrms->PktEc.TailLength;
        Msg[22] = sysPrms->PktEc.DblTalkThresh;
        Msg[23] = sysPrms->PktEc.NlpThreshold;
        Msg[24] = sysPrms->PktEc.NlpUpperLimitThreshConv;
        Msg[25] = sysPrms->PktEc.NlpUpperLimitThreshPreconv;
        Msg[26] = sysPrms->PktEc.NlpMaxSupp;

        Msg[27] = sysPrms->PktEc.CngThreshold;
        Msg[28] = sysPrms->PktEc.AdaptLimit;
        Msg[29] = sysPrms->PktEc.CrossCorrLimit;
        Msg[30] = sysPrms->PktEc.NumFirSegments;
        Msg[31] = sysPrms->PktEc.FirSegmentLen;
        Msg[32] = sysPrms->PktEc.FirTapCheckPeriod;
        Msg[48] = sysPrms->PktEc.MaxDoubleTalkThres;
    }
    if (ConfUpdate)  {
        Msg[1] |= 16;
        Msg[33] = sysPrms->ConfNumDominant;
        Msg[34] = sysPrms->MaxConfNoiseSuppress;
    }

    MsgLenI16 = 40;

    // 4_2 ---------------------------------------
    if (CIDUpdate) {
        Msg[1] |= 0x40;
        Msg[40] = sysPrms->Cid.formatBurstFlag;
        Msg[41] = sysPrms->Cid.numSeizureBytes;
        Msg[42] = sysPrms->Cid.numMarkBytes;
        Msg[43] = sysPrms->Cid.numPostambleBytes;
        Msg[44] = sysPrms->Cid.fskType;
        Msg[45] = sysPrms->Cid.fskLevel;
        Msg[46] = sysPrms->Cid.msgType;
    }

    if (sysPrms->VadReport)
        Msg[1] |= 0x80;

    MsgLenI16 += 9;

    return MsgLenI16;
}

gpakWriteSysParmsStatus_t gpakWriteSystemParms (ADT_UInt32 DspId, GpakSystemParms_t *sysPrms,
    ADT_UInt16 AgcUpdate,   ADT_UInt16 VadUpdate, ADT_UInt16 PcmEcUpdate,
    ADT_UInt16 PktEcUpdate, ADT_UInt16 ConfUpdate,  ADT_UInt16 CIDUpdate, 
    GPAK_SysParmsStat_t *pStatus) {

    ADT_UInt32 MsgLenI16;                        /* message length */
    ADT_UInt32 MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_Int16 *Msg;


    if (MaxDspCores <= DspId)
        return WspInvalidDsp;

    Msg = (ADT_Int16 *) &MsgBuffer[0];

    MsgLenI16 = gpakFormatSystemParms (Msg, sysPrms, AgcUpdate,   VadUpdate,
		                            PcmEcUpdate,  PktEcUpdate, ConfUpdate, 
                                    CIDUpdate);

    if (!TransactCmd (DspId, MsgBuffer, MsgLenI16, MSG_WRITE_SYS_PARMS_REPLY,  2, NO_ID_CHK, 0))
        return WspDspCommFailure;

    *pStatus = (GPAK_SysParmsStat_t) Byte1 (Msg[1]);
    if (*pStatus == Sp_Success)
        return WspSuccess;
    else
        return WspParmError;
}

//}==================================================================================
//{                  Configuration routines
//===================================================================================


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakConfigurePorts - Configure a DSP's serial ports.
 *
 * FUNCTION
 *  Configures a DSP's serial ports.
 *
 * Inputs
 *   DspId    - Dsp identifier 
 *   PortCfg  - Port configuration data structure pointer
 *
 * Outputs
 *    pStatus  - port configuration status from DSP
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */

#define Pack(Hi,Lo)  (ADT_UInt16) ((Hi)<<8) | ((Lo) & 0x00ff)


#if (DSP_TYPE == 54)

ADT_UInt32 gpakFormatPortConfig (ADT_UInt16 *MsgBuff, GpakPortConfig_t *PortCfg) {

	ADT_UInt8  Polarities, Delays;
    int port, wordCnt;
    ADT_UInt16 *Msg;

    /* Build the Configure Serial Ports message. */
    MsgBuff[0] = MSG_CONFIGURE_PORTS << 8;
    
	Msg = &MsgBuff[1];

	wordCnt = 1;
	for (port=0; port<3; port++, wordCnt += 6) {
	   Polarities = 0;

       if (PortCfg->RxClockRise[port])     Polarities |= 1;
       if (PortCfg->RxClockInternal[port]) Polarities |= 2;

       if (PortCfg->TxClockFall[port])     Polarities |= 4;
       if (PortCfg->TxClockInternal[port]) Polarities |= 8;

       if (PortCfg->RxSyncLow[port])       Polarities |= 0x10;
       if (PortCfg->RxSyncInternal[port])  Polarities |= 0x20;

       if (PortCfg->TxSyncLow[port])       Polarities |= 0x40;
       if (PortCfg->TxSyncInternal[port])  Polarities |= 0x80;

	   Delays = ((PortCfg->TxDelay[port] << 2) & 0xc)  | 
		         (PortCfg->RxDelay[port]       & 0x3);

   	*Msg++ = Pack (PortCfg->MultiMode[port], PortCfg->Compand[port]);
	   *Msg++ = Pack (PortCfg->LowBlk[port],    PortCfg->HighBlk[port]);
	   *Msg++ = Pack (Polarities, Delays);
	   *Msg++ = PortCfg->LowMask[port];
	   *Msg++ = PortCfg->HighMask[port];
	   *Msg++ = Pack (PortCfg->ClkDiv[port], PortCfg->PulseWidth[port]);
	}
    return wordCnt;
}
#elif ((DSP_TYPE == 64  || DSP_TYPE == 6424 || DSP_TYPE == 6437 || DSP_TYPE == 6452 || DSP_TYPE == 3530) )

ADT_UInt32 gpakFormatPortConfig (ADT_UInt16 *Msg, GpakPortConfig_t *PortCfg) {

    /* Build the Configure Serial Ports message. */
    Msg[0] = MSG_CONFIGURE_PORTS << 8;
    Msg[1] = 0;
    if (PortCfg->Port1Enable)        Msg[1] = 0x0001;
    if (PortCfg->Port2Enable)        Msg[1] |= 0x0002;
    if (PortCfg->Port3Enable)        Msg[1] |= 0x0004;
	if (PortCfg->AudioPort1Enable)   Msg[1] |= 0x0008;
    if (PortCfg->AudioPort2Enable)   Msg[1] |= 0x0010;

    Msg[2]  = (ADT_UInt16)((PortCfg->Port1SlotMask0_31  ) >> 16);  
    Msg[3]  = (ADT_UInt16)((PortCfg->Port1SlotMask0_31  ) & 0xffff);  
    Msg[4]  = (ADT_UInt16)((PortCfg->Port1SlotMask32_63 ) >> 16);  
    Msg[5]  = (ADT_UInt16)((PortCfg->Port1SlotMask32_63 ) & 0xffff);
    Msg[6]  = (ADT_UInt16)((PortCfg->Port1SlotMask64_95 ) >> 16);  
    Msg[7]  = (ADT_UInt16)((PortCfg->Port1SlotMask64_95 ) & 0xffff);
    Msg[8]  = (ADT_UInt16)((PortCfg->Port1SlotMask96_127) >> 16);  
    Msg[9]  = (ADT_UInt16)((PortCfg->Port1SlotMask96_127) & 0xffff);

    Msg[10] = (ADT_UInt16)((PortCfg->Port2SlotMask0_31  ) >> 16);  
    Msg[11] = (ADT_UInt16)((PortCfg->Port2SlotMask0_31  ) & 0xffff);
    Msg[12] = (ADT_UInt16)((PortCfg->Port2SlotMask32_63 ) >> 16);  
    Msg[13] = (ADT_UInt16)((PortCfg->Port2SlotMask32_63 ) & 0xffff);
    Msg[14] = (ADT_UInt16)((PortCfg->Port2SlotMask64_95 ) >> 16);  
    Msg[15] = (ADT_UInt16)((PortCfg->Port2SlotMask64_95 ) & 0xffff);
    Msg[16] = (ADT_UInt16)((PortCfg->Port2SlotMask96_127) >> 16);  
    Msg[17] = (ADT_UInt16)((PortCfg->Port2SlotMask96_127) & 0xffff);

    Msg[18] = (ADT_UInt16)((PortCfg->Port3SlotMask0_31  ) >> 16);  
    Msg[19] = (ADT_UInt16)((PortCfg->Port3SlotMask0_31  ) & 0xffff);
    Msg[20] = (ADT_UInt16)((PortCfg->Port3SlotMask32_63 ) >> 16);  
    Msg[21] = (ADT_UInt16)((PortCfg->Port3SlotMask32_63 ) & 0xffff);
    Msg[22] = (ADT_UInt16)((PortCfg->Port3SlotMask64_95 ) >> 16);  
    Msg[23] = (ADT_UInt16)((PortCfg->Port3SlotMask64_95 ) & 0xffff);
    Msg[24] = (ADT_UInt16)((PortCfg->Port3SlotMask96_127) >> 16);  
    Msg[25] = (ADT_UInt16)((PortCfg->Port3SlotMask96_127) & 0xffff);

    Msg[26] = (ADT_UInt16)((PortCfg->AudioPort1TxSerMask) >> 16);  
    Msg[27] = (ADT_UInt16)((PortCfg->AudioPort1TxSerMask) & 0xffff);
    Msg[28] = (ADT_UInt16)((PortCfg->AudioPort1RxSerMask) >> 16);  
    Msg[29] = (ADT_UInt16)((PortCfg->AudioPort1RxSerMask) & 0xffff);
    Msg[30] = (ADT_UInt16)((PortCfg->AudioPort1SlotMask ) >> 16);  
    Msg[31] = (ADT_UInt16)((PortCfg->AudioPort1SlotMask ) & 0xffff);

    Msg[32] = (ADT_UInt16)((PortCfg->AudioPort2TxSerMask) >> 16);  
    Msg[33] = (ADT_UInt16)((PortCfg->AudioPort2TxSerMask) & 0xffff);
    Msg[34] = (ADT_UInt16)((PortCfg->AudioPort2RxSerMask) >> 16);  
    Msg[35] = (ADT_UInt16)((PortCfg->AudioPort2RxSerMask) & 0xffff);
    Msg[36] = (ADT_UInt16)((PortCfg->AudioPort2SlotMask ) >> 16);  
    Msg[37] = (ADT_UInt16)((PortCfg->AudioPort2SlotMask ) & 0xffff);

    Msg[38] = PackBytes (PortCfg->ClkDiv2,     PortCfg->ClkDiv1);
    Msg[39] = PackBytes (PortCfg->FrameWidth1, PortCfg->ClkDiv3);
    Msg[40] = PackBytes (PortCfg->FrameWidth3, PortCfg->FrameWidth2);

    Msg[41] = (ADT_UInt16)(((PortCfg->FramePeriod1 << 4) & 0xfff0) |
                         ((PortCfg->Compand1 << 1) & 0xE) | (PortCfg->SampRateGen1 & 1) );
    Msg[42] = (ADT_UInt16)(((PortCfg->FramePeriod2 << 4) & 0xfff0) |
                         ((PortCfg->Compand2 << 1) & 0xE) | (PortCfg->SampRateGen2 & 1) );

    Msg[43] = (ADT_UInt16)(((PortCfg->FramePeriod3 << 4) & 0xfff0) |
                         ((PortCfg->Compand3 << 1) & 0xE) | (PortCfg->SampRateGen3 & 1) );

    Msg[44] = (ADT_UInt16 )( (PortCfg->TxDataDelay1 & 0x0003)       | ((PortCfg->RxDataDelay1 << 2) & 0x000c) |
                          ((PortCfg->TxDataDelay2 << 4) & 0x0030) | ((PortCfg->RxDataDelay2 << 6) & 0x00c0) |
                          ((PortCfg->TxDataDelay3 << 8) & 0x0300) | ((PortCfg->RxDataDelay3 << 10) & 0x0c00) );
    return 45;
}
#elif (DSP_TYPE == 55)

ADT_UInt32 gpakFormatPortConfig (ADT_UInt16 *Msg, GpakPortConfig_t *PortCfg) {

    /* Build the Configure Serial Ports message. */
    Msg[0] = MSG_CONFIGURE_PORTS << 8;
    Msg[1] = 0;
    if (PortCfg->Port1Enable)        Msg[1] = 0x0001;
    if (PortCfg->Port2Enable)        Msg[1] |= 0x0002;
    if (PortCfg->Port3Enable)        Msg[1] |= 0x0004;
	//if (PortCfg->AudioPort1Enable)   Msg[1] |= 0x0008;
    //if (PortCfg->AudioPort2Enable)   Msg[1] |= 0x0010;

    Msg[2]  = (ADT_UInt16)((PortCfg->Port1SlotMask0_31  ) >> 16);  
    Msg[3]  = (ADT_UInt16)((PortCfg->Port1SlotMask0_31  ) & 0xffff);  
    Msg[4]  = (ADT_UInt16)((PortCfg->Port1SlotMask32_63 ) >> 16);  
    Msg[5]  = (ADT_UInt16)((PortCfg->Port1SlotMask32_63 ) & 0xffff);
    Msg[6]  = (ADT_UInt16)((PortCfg->Port1SlotMask64_95 ) >> 16);  
    Msg[7]  = (ADT_UInt16)((PortCfg->Port1SlotMask64_95 ) & 0xffff);
    Msg[8]  = (ADT_UInt16)((PortCfg->Port1SlotMask96_127) >> 16);  
    Msg[9]  = (ADT_UInt16)((PortCfg->Port1SlotMask96_127) & 0xffff);

    Msg[10] = (ADT_UInt16)((PortCfg->Port2SlotMask0_31  ) >> 16);  
    Msg[11] = (ADT_UInt16)((PortCfg->Port2SlotMask0_31  ) & 0xffff);
    Msg[12] = (ADT_UInt16)((PortCfg->Port2SlotMask32_63 ) >> 16);  
    Msg[13] = (ADT_UInt16)((PortCfg->Port2SlotMask32_63 ) & 0xffff);
    Msg[14] = (ADT_UInt16)((PortCfg->Port2SlotMask64_95 ) >> 16);  
    Msg[15] = (ADT_UInt16)((PortCfg->Port2SlotMask64_95 ) & 0xffff);
    Msg[16] = (ADT_UInt16)((PortCfg->Port2SlotMask96_127) >> 16);  
    Msg[17] = (ADT_UInt16)((PortCfg->Port2SlotMask96_127) & 0xffff);

    Msg[18] = (ADT_UInt16)((PortCfg->Port3SlotMask0_31  ) >> 16);  
    Msg[19] = (ADT_UInt16)((PortCfg->Port3SlotMask0_31  ) & 0xffff);
    Msg[20] = (ADT_UInt16)((PortCfg->Port3SlotMask32_63 ) >> 16);  
    Msg[21] = (ADT_UInt16)((PortCfg->Port3SlotMask32_63 ) & 0xffff);
    Msg[22] = (ADT_UInt16)((PortCfg->Port3SlotMask64_95 ) >> 16);  
    Msg[23] = (ADT_UInt16)((PortCfg->Port3SlotMask64_95 ) & 0xffff);
    Msg[24] = (ADT_UInt16)((PortCfg->Port3SlotMask96_127) >> 16);  
    Msg[25] = (ADT_UInt16)((PortCfg->Port3SlotMask96_127) & 0xffff);

    Msg[26] = (ADT_UInt16)(PortCfg->txClockPolarity1);  
    Msg[27] = (ADT_UInt16)(PortCfg->txClockPolarity2);
    Msg[28] = (ADT_UInt16)(PortCfg->txClockPolarity3);  
    Msg[29] = (ADT_UInt16)(PortCfg->rxClockPolarity1);
    Msg[30] = (ADT_UInt16)(PortCfg->rxClockPolarity2);  
    Msg[31] = (ADT_UInt16)(PortCfg->rxClockPolarity3);

    //Msg[32] = (ADT_UInt16)((PortCfg->AudioPort2TxSerMask) >> 16);  
    //Msg[33] = (ADT_UInt16)((PortCfg->AudioPort2TxSerMask) & 0xffff);
    //Msg[34] = (ADT_UInt16)((PortCfg->AudioPort2RxSerMask) >> 16);  
    //Msg[35] = (ADT_UInt16)((PortCfg->AudioPort2RxSerMask) & 0xffff);
    //Msg[36] = (ADT_UInt16)((PortCfg->AudioPort2SlotMask ) >> 16);  
    //Msg[37] = (ADT_UInt16)((PortCfg->AudioPort2SlotMask ) & 0xffff);

    Msg[38] = PackBytes (PortCfg->ClkDiv2,     PortCfg->ClkDiv1);
    Msg[39] = PackBytes (PortCfg->FrameWidth1, PortCfg->ClkDiv3);
    Msg[40] = PackBytes (PortCfg->FrameWidth3, PortCfg->FrameWidth2);

    Msg[41] = (ADT_UInt16)(((PortCfg->FramePeriod1 << 4) & 0xfff0) |
                         ((PortCfg->Compand1 << 1) & 0xE) | (PortCfg->SampRateGen1 & 1) );
    Msg[42] = (ADT_UInt16)(((PortCfg->FramePeriod2 << 4) & 0xfff0) |
                         ((PortCfg->Compand2 << 1) & 0xE) | (PortCfg->SampRateGen2 & 1) );

    Msg[43] = (ADT_UInt16)(((PortCfg->FramePeriod3 << 4) & 0xfff0) |
                         ((PortCfg->Compand3 << 1) & 0xE) | (PortCfg->SampRateGen3 & 1) );

    Msg[44] = (ADT_UInt16 )( (PortCfg->TxDataDelay1 & 0x0003)       | ((PortCfg->RxDataDelay1 << 2) & 0x000c) |
                          ((PortCfg->TxDataDelay2 << 4) & 0x0030) | ((PortCfg->RxDataDelay2 << 6) & 0x00c0) |
                          ((PortCfg->TxDataDelay3 << 8) & 0x0300) | ((PortCfg->RxDataDelay3 << 10) & 0x0c00) );
    return 45;
}
#else
   #error DSP_TYPE must be defined
#endif

gpakConfigPortStatus_t gpakConfigurePorts (ADT_UInt32 DspId, GpakPortConfig_t *pPortConfig, 
                                           GPAK_PortConfigStat_t *pStatus) {

    ADT_UInt32 MsgLenI16;                        /* message length */
    ADT_UInt32 MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;


    if (MaxDspCores <= DspId)
        return CpsInvalidDsp;

    Msg =(ADT_UInt16 *) &MsgBuffer[0];

    MsgLenI16 = gpakFormatPortConfig (Msg, pPortConfig);

    if (!TransactCmd (DspId, MsgBuffer, MsgLenI16, MSG_CONFIG_PORTS_REPLY, 2, NO_ID_CHK, 0))
        return CpsDspCommFailure;

    /* Return success or failure . */
    *pStatus = (GPAK_PortConfigStat_t) Byte1 (Msg[1]);
    if (*pStatus == Pc_Success)
        return CpsSuccess;
    else
        return CpsParmError;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakConfigureChannel - Configure a DSP's Channel.
 *
 * FUNCTION
 *  Configures a DSP's Channel.
 *
 * Inputs
 *   DspId     - Dsp identifier 
 * ChannelId   - Channel identifier
 * ChannelType - Type of channel,  PCM-PCM, PCK-PKT,,,
 *  pChan      - Channel data structure pointer
 *
 * Outputs
 *    pStatus  - channel configuration status from DSP
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
ADT_UInt32 gpakFormatChannelConfig (ADT_UInt16 *Msg, GpakChannelConfig_t *pChan,
                                    ADT_UInt16 ChannelId, GpakChanType ChannelType) {

    ADT_UInt32 MsgLenI16;                     /* message length */
    int        Voice;

    Msg[0] = MSG_CONFIGURE_CHANNEL << 8;
    Msg[1] = PackBytes (ChannelId, ChannelType);

    /* Build the Configure Channel message based on the Channel Type. */
    switch (ChannelType) {
    case pcmToPacket:
        if (pChan->PcmPkt.Coding == GpakVoiceChannel) Voice = 1;
        else                                          Voice = 0;
        Msg[2] = PackBytes (pChan->PcmPkt.PcmInPort,    pChan->PcmPkt.PcmInSlot);
        Msg[3] = PackBytes (pChan->PcmPkt.PcmOutPort,   pChan->PcmPkt.PcmOutSlot);
        Msg[4] = PackBytes (pChan->PcmPkt.PktInCoding,  pChan->PcmPkt.PktInFrameSize);
        Msg[5] = PackBytes (pChan->PcmPkt.PktOutCoding, pChan->PcmPkt.PktOutFrameSize);
        Msg[6] = (ADT_UInt16) ( ( pChan->PcmPkt.PcmEcanEnable         << 15) |
                              ((pChan->PcmPkt.PktEcanEnable &    1) << 14) |
                              ((pChan->PcmPkt.VadEnable     &    1) << 13) |
                              ((pChan->PcmPkt.AgcEnable     &    1) << 12) |
                              ((pChan->PcmPkt.AECEcanEnable &    1) << 11) |
                              ((Voice                       &    1) << 10) );
        Msg[7] = PackBytes (pChan->PcmPkt.PcmInPin, pChan->PcmPkt.PcmOutPin);
        Msg[8] = (ADT_UInt16) ( ((pChan->PcmPkt.FaxMode     &    3) << 14) | 
                              ((pChan->PcmPkt.FaxTransport  &    3) << 12) |
                              ((pChan->PcmPkt.ToneTypes     & 0x0FFF)) );
        MsgLenI16 = 9;

        // 4_2 ---------------------------------------
        Msg[9] = (ADT_UInt16) (pChan->PcmPkt.CIdMode & 3);
        Msg[10] = pChan->PcmPkt.ToneGenGainG1;
        Msg[11] = pChan->PcmPkt.OutputGainG2;
        Msg[12] = pChan->PcmPkt.InputGainG3;
        MsgLenI16 += 4;
        // ---------------------- 

        break;

    case pcmToPcm:
        if (pChan->PcmPcm.Coding == GpakVoiceChannel) Voice = 1;
        else                                          Voice = 0;

        Msg[2] = PackBytes (pChan->PcmPcm.InPortA,  pChan->PcmPcm.InSlotA);
        Msg[3] = PackBytes (pChan->PcmPcm.OutPortA, pChan->PcmPcm.OutSlotA);
        Msg[4] = PackBytes (pChan->PcmPcm.InPortB,  pChan->PcmPcm.InSlotB);
        Msg[5] = PackBytes (pChan->PcmPcm.OutPortB, pChan->PcmPcm.OutSlotB);
        Msg[6] = PackBytes ( 
                        ( ( pChan->PcmPcm.EcanEnableA         << 7) |
                          ((pChan->PcmPcm.EcanEnableB    & 1) << 6) |
                          ((pChan->PcmPcm.AECEcanEnableA & 1) << 5) |
                          ((pChan->PcmPcm.AECEcanEnableB & 1) << 4) |
                          ((pChan->PcmPcm.AgcEnableA     & 1) << 3) |
                          ((pChan->PcmPcm.AgcEnableB     & 1) << 2) |
                          ( Voice                             << 1) ),
                           pChan->PcmPcm.FrameSize);
        Msg[7] = PackBytes (pChan->PcmPcm.InPinA,  pChan->PcmPcm.InPinB);
        Msg[8] = PackBytes (pChan->PcmPcm.OutPinA, pChan->PcmPcm.OutPinB);
        Msg[9]   = (pChan->PcmPcm.ToneTypesA & 0xFFF) | ((pChan->PcmPcm.NoiseSuppressA << 12) & 0x1000);
        Msg[10]  = (pChan->PcmPcm.ToneTypesB & 0xFFF) | ((pChan->PcmPcm.NoiseSuppressB << 12) & 0x1000);
        MsgLenI16 = 11;

        // 4_2 ---------------------------------------
        Msg[11] = (ADT_UInt16) (((pChan->PcmPcm.CIdModeA & 3) << 2) |
                                 (pChan->PcmPcm.CIdModeB & 3));
        Msg[12] = pChan->PcmPcm.ToneGenGainG1A;
        Msg[13] = pChan->PcmPcm.OutputGainG2A;
        Msg[14] = pChan->PcmPcm.InputGainG3A;
        Msg[15] = pChan->PcmPcm.ToneGenGainG1B;
        Msg[16] = pChan->PcmPcm.OutputGainG2B;
        Msg[17] = pChan->PcmPcm.InputGainG3B;
        MsgLenI16 += 7;
        // ---------------------------------

        break;

    case packetToPacket:
        Msg[2] = PackBytes (pChan->PktPkt.PktInCodingA,  pChan->PktPkt.PktInFrameSizeA);
        Msg[3] = PackBytes (pChan->PktPkt.PktOutCodingA, pChan->PktPkt.PktOutFrameSizeA);
        Msg[4] = PackBytes (
                     (  ( pChan->PktPkt.EcanEnableA      << 7) |
                        ((pChan->PktPkt.VadEnableA  & 1) << 6) |
                        ((pChan->PktPkt.EcanEnableB & 1) << 5) |
                        ((pChan->PktPkt.VadEnableB  & 1) << 4) |
                        ((pChan->PktPkt.AgcEnableA  & 1) << 3) |
                        ((pChan->PktPkt.AgcEnableB  & 1) << 2)),
                          pChan->PktPkt.ChannelIdB);
        Msg[5] = PackBytes (pChan->PktPkt.PktInCodingB,  pChan->PktPkt.PktInFrameSizeB);
        Msg[6] = PackBytes (pChan->PktPkt.PktOutCodingB, pChan->PktPkt.PktOutFrameSizeB);
        Msg[7] = pChan->PktPkt.ToneTypesA & 0xFFF;
        Msg[8] = pChan->PktPkt.ToneTypesB & 0xFFF;
        MsgLenI16 = 9;
        break;

    case circuitData:
        Msg[2] = PackBytes (pChan->Circuit.PcmInPort, pChan->Circuit.PcmInSlot);
        Msg[3] = PackBytes (pChan->Circuit.PcmOutPort, pChan->Circuit.PcmOutSlot);
        Msg[4] = (ADT_UInt16) (pChan->Circuit.MuxFactor << 8);
        Msg[5] =  PackBytes (pChan->Circuit.PcmInPin, pChan->Circuit.PcmOutPin);
        MsgLenI16 = 6;
        break;

    case conferencePcm:

        Msg[2] = PackBytes (pChan->ConferPcm.ConferenceId,
                        ( ((pChan->ConferPcm.AgcInEnable    & 1) << 5) |
                          ((pChan->ConferPcm.EcanEnable     & 1) << 4) |
                          ((pChan->ConferPcm.AECEcanEnable  & 1) << 3) ) 
                          );
        Msg[3] = PackBytes (pChan->ConferPcm.PcmInPort, pChan->ConferPcm.PcmInSlot);
        Msg[4] = PackBytes (pChan->ConferPcm.PcmOutPort, pChan->ConferPcm.PcmOutSlot);
        Msg[5] = PackBytes (pChan->ConferPcm.PcmInPin, pChan->ConferPcm.PcmOutPin);
        Msg[6]  = pChan->ConferPcm.ToneTypes & 0xFFF;
        MsgLenI16 = 7;

        // 4_2 ---------------------------------------
        Msg[7]  = pChan->ConferPcm.ToneGenGainG1;
        Msg[8]  = (ADT_UInt16) (pChan->ConferPcm.AgcOutEnable & 1);
        Msg[9] = pChan->ConferPcm.OutputGainG2;
        Msg[10] = pChan->ConferPcm.InputGainG3;
        MsgLenI16 += 4;
        // ------------------------------------

        break;

    case conferencePacket:
        Msg[2] = PackBytes (pChan->ConferPkt.ConferenceId,
                        ((pChan->ConferPkt.VadEnable  & 1) << 7) |
                        ((pChan->ConferPkt.AgcInEnable & 1) << 5) |
                        ((pChan->ConferPkt.EcanEnable & 1) << 4) );
        Msg[3] = PackBytes (pChan->ConferPkt.PktInCoding, pChan->ConferPkt.PktOutCoding);
        Msg[4] = pChan->ConferPkt.ToneTypes & 0xFFF;
        MsgLenI16 = 5;

        // 4_2 ---------------------------------------
        Msg[5]  = (ADT_UInt16) (pChan->ConferPkt.AgcOutEnable & 1);
        Msg[6]  = pChan->ConferPkt.ToneGenGainG1;
        Msg[7]  = pChan->ConferPkt.OutputGainG2;
        Msg[8]  = pChan->ConferPkt.InputGainG3;
        MsgLenI16 += 4;

        // 5_0 ------------------------------------
        Msg[9] = pChan->ConferPkt.PktFrameSize;
        MsgLenI16 += 1;

        break;

    case conferenceComposite:
        Msg[2] = PackBytes (pChan->ConferComp.ConferenceId,
                       ((pChan->ConferComp.VadEnable       & 1) << 7) );
        Msg[3] = PackBytes (pChan->ConferComp.PcmOutPort,   pChan->ConferComp.PcmOutSlot);
        Msg[4] = PackBytes (pChan->ConferComp.PktOutCoding, pChan->ConferComp.PcmOutPin);
        MsgLenI16 = 5;
        break;

    // 4_2 ---------------------------------------
    case loopbackCoder:
        Msg[2] = PackBytes (pChan->LpbkCoder.PktInCoding, pChan->LpbkCoder.PktOutCoding);
        MsgLenI16 = 3;
        break;
    // --------------------------------------------

    default:
        MsgLenI16 = 0;
        break;
    }
    return MsgLenI16;
}

gpakConfigChanStatus_t gpakConfigureChannel (ADT_UInt32 DspId, ADT_UInt16 ChannelId, 
              GpakChanType ChannelType, GpakChannelConfig_t *pChan, 
              GPAK_ChannelConfigStat_t *pStatus) {

    ADT_UInt32  MsgLenI16;                     /* message length */
    ADT_UInt32  MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;

    if (MaxDspCores <= DspId)
        return CcsInvalidDsp;

    if (pDspIfBlk[DspId] == 0)
       DSPSync (DspId);

    if (ChannelId >= MaxChannels[DspId])
        return CcsInvalidChannel;

    Msg    = (ADT_UInt16 *) &MsgBuffer[0];

    MsgLenI16 = gpakFormatChannelConfig (Msg, pChan, ChannelId, ChannelType);
    if (MsgLenI16 == 0) {
        *pStatus = Cc_InvalidChannelType;
        return CcsParmError;
    }

   if (!TransactCmd (DspId, MsgBuffer, MsgLenI16, MSG_CONFIG_CHAN_REPLY, 2, 
	                  BYTE_ID, (ADT_UInt16) ChannelId))
        return CcsDspCommFailure;

    /* Return success or failure  */
    *pStatus = (GPAK_ChannelConfigStat_t) Byte0 (Msg[1]);
    if (*pStatus == Cc_Success)  return CcsSuccess;
    else                         return CcsParmError;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakTearDownChannel - Tear Down a DSP's Channel.
 *
 * FUNCTION
 *  Tears down a DSP's Channel.
 *
 * Inputs
 *   DspId     - Dsp identifier 
 * ChannelId   - Channel identifier
 *
 * Outputs
 *    pStatus  - channel tear down status from DSP
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
gpakTearDownStatus_t gpakTearDownChannel (ADT_UInt32 DspId, ADT_UInt16 ChannelId,
                                          GPAK_TearDownChanStat_t *pStatus) {

    ADT_UInt32  MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;


    if (MaxDspCores <= DspId)
        return TdsInvalidDsp;

    if (pDspIfBlk[DspId] == 0)
       DSPSync (DspId);

    if (ChannelId >= MaxChannels[DspId])
        return TdsInvalidChannel;

    Msg = (ADT_UInt16 *) MsgBuffer;

    Msg[0] = MSG_TEAR_DOWN_CHANNEL << 8;
    Msg[1] = (ADT_UInt16) (ChannelId << 8);

    if (!TransactCmd (DspId, MsgBuffer, 2, MSG_TEAR_DOWN_REPLY, 2,
		              BYTE_ID, ChannelId))
        return TdsDspCommFailure;

    *pStatus = (GPAK_TearDownChanStat_t) Byte0 (Msg[1]);
    if (*pStatus == Td_Success)
        return TdsSuccess;
    else
        return TdsError;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakGetChannelStatus - Read a DSP's Channel Status.
 *
 * FUNCTION
 *  Reads a DSP's Channel Status information for a specified channel.
 *
 * Inputs
 *   DspId     - Dsp identifier 
 * ChannelId   - Channel identifier
 *
 * Outputs
 *    pChanStatus - Channel status data structure pointer
 *    pStatus  - channel status from DSP
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
#define PCMPkt   pChanStatus->ChannelConfig.PcmPkt
#define PCMPcm   pChanStatus->ChannelConfig.PcmPcm
#define PKTPkt   pChanStatus->ChannelConfig.PktPkt
#define CIRCData pChanStatus->ChannelConfig.Circuit
#define CNFPcm   pChanStatus->ChannelConfig.ConferPcm
#define CNFPkt   pChanStatus->ChannelConfig.ConferPkt
#define CNFCmp   pChanStatus->ChannelConfig.ConferComp
#define LPBCdr   pChanStatus->ChannelConfig.LpbkCoder

gpakGetChanStatStatus_t gpakParseChanStatusResponse (ADT_UInt32 ReplyLength, ADT_UInt16 *Msg, 
                       GpakChannelStatus_t *pChanStatus, GPAK_ChannelStatusStat_t *pStatus) {

    *pStatus = (GPAK_ChannelStatusStat_t) Byte0 (Msg[1]);
    if (*pStatus != Cs_Success)
        return CssError;

    pChanStatus->ChannelType = (GpakChanType) Byte1 (Msg[2]);
    if (pChanStatus->ChannelType == inactive) 
       return CssSuccess;

  	 if (ReplyLength < 7)
  		 return CssDspCommFailure;

    pChanStatus->BadPktHeaderCnt =  Msg[3];
    pChanStatus->PktOverflowCnt =   Msg[4];
    pChanStatus->PktUnderflowCnt =  Msg[5];
    pChanStatus->StatusFlags =      Msg[6];

    switch (pChanStatus->ChannelType)  {
    case pcmToPacket:
       if (ReplyLength < 14)
          return CssDspCommFailure;


       PCMPkt.PcmInPort =      (GpakSerialPort_t) Byte1 (Msg[7]);
       PCMPkt.PcmInSlot =                         Byte0 (Msg[7]);
       PCMPkt.PcmOutPort =     (GpakSerialPort_t) Byte1 (Msg[8]);
       PCMPkt.PcmOutSlot =                        Byte0 (Msg[8]);
       PCMPkt.PktInCoding =    (GpakCodecs)       Byte1 (Msg[9]);
       PCMPkt.PktInFrameSize = (GpakFrameSizes)   Byte0 (Msg[9]);
       PCMPkt.PktOutCoding =   (GpakCodecs)       Byte1 (Msg[10]);
       PCMPkt.PktOutFrameSize = (GpakFrameSizes)  Byte0 (Msg[10]);

       if (Msg[11] & 0x8000)  PCMPkt.PcmEcanEnable = Enabled;
       else                   PCMPkt.PcmEcanEnable = Disabled;

       if (Msg[11] & 0x4000)  PCMPkt.PktEcanEnable = Enabled;
       else                   PCMPkt.PktEcanEnable = Disabled;

       if (Msg[11] & 0x2000)  PCMPkt.VadEnable = Enabled;
       else                   PCMPkt.VadEnable = Disabled;

       if (Msg[11] & 0x1000)  PCMPkt.AgcEnable = Enabled;
       else                   PCMPkt.AgcEnable = Disabled;

       if (Msg[11] & 0x0800)  PCMPkt.AECEcanEnable = Enabled;
       else                   PCMPkt.AECEcanEnable = Disabled;

       if (Msg[11] & 0x0400)    PCMPkt.Coding = GpakVoiceChannel;
       else                            PCMPkt.Coding = GpakDataChannel;
 


       PCMPkt.PcmInPin =  Byte1 (Msg[12]);
       PCMPkt.PcmOutPin = Byte0 (Msg[12]);

       PCMPkt.FaxMode =          (GpakFaxMode_t) ((Msg[13] >> 14) & 0x0003);
       PCMPkt.FaxTransport =     (GpakFaxTransport_t) ((Msg[13] >> 12) & 0x0003);
       PCMPkt.ToneTypes =     (GpakToneTypes) (Msg[13] & 0x0FFF);

        // 4_2 ----------------------------------------------------
        PCMPkt.CIdMode       =   (GpakCidMode_t) (Msg[14] & 0x0003);
        PCMPkt.ToneGenGainG1 =   Msg[15];
        PCMPkt.OutputGainG2  =   Msg[16];
        PCMPkt.InputGainG3   =   Msg[17];
        pChanStatus->NumPktsToDsp    = Msg[18];
        pChanStatus->NumPktsFromDsp  = Msg[19];
        // ---------------------------------------------------------

       break;

    case pcmToPcm:
       if (ReplyLength < 16)
          return CssDspCommFailure;

       PCMPcm.InPortA =   (GpakSerialPort_t) Byte1 (Msg[7]);
       PCMPcm.InSlotA =                      Byte0 (Msg[7]);
       PCMPcm.OutPortA =  (GpakSerialPort_t) Byte1 (Msg[8]);
       PCMPcm.OutSlotA =                     Byte0 (Msg[8]);
       PCMPcm.InPortB =   (GpakSerialPort_t) Byte1 (Msg[9]);
       PCMPcm.InSlotB =                      Byte0 (Msg[9]);
       PCMPcm.OutPortB =  (GpakSerialPort_t) Byte1 (Msg[10]);
       PCMPcm.OutSlotB =                     Byte0 (Msg[10]);

       if (Msg[11] & 0x8000)    PCMPcm.EcanEnableA = Enabled;
       else                     PCMPcm.EcanEnableA = Disabled;

       if (Msg[11] & 0x4000)    PCMPcm.EcanEnableB = Enabled;
       else                     PCMPcm.EcanEnableB = Disabled;

       if (Msg[11] & 0x2000)    PCMPcm.AECEcanEnableA = Enabled;
       else                     PCMPcm.AECEcanEnableA = Disabled;

       if (Msg[11] & 0x1000)    PCMPcm.AECEcanEnableB = Enabled;
       else                     PCMPcm.AECEcanEnableB = Disabled;

       if (Msg[11] & 0x0800)    PCMPcm.AgcEnableA = Enabled;
       else                     PCMPcm.AgcEnableA = Disabled;

       if (Msg[11] & 0x0400)    PCMPcm.AgcEnableB = Enabled;
       else                     PCMPcm.AgcEnableB = Disabled;

       if (Msg[11] & 0x0200)    PCMPcm.Coding = GpakVoiceChannel;
       else                     PCMPcm.Coding = GpakDataChannel;

       PCMPcm.FrameSize =     (GpakFrameSizes) Byte0 (Msg[11]);

       PCMPcm.InPinA =  Byte1 (Msg[12]);
       PCMPcm.InPinB =  Byte0 (Msg[12]);
       PCMPcm.OutPinA = Byte1 (Msg[13]);
       PCMPcm.OutPinB = Byte0 (Msg[13]);

       PCMPcm.ToneTypesA =  (GpakToneTypes) (Msg[14] & 0xFFF);
       PCMPcm.ToneTypesB =  (GpakToneTypes) (Msg[15] & 0xFFF);
       PCMPcm.NoiseSuppressA = ((Msg[14] >> 12) & 0x0001);
       PCMPcm.NoiseSuppressB = ((Msg[15] >> 12) & 0x0001);

       // 4_2 ----------------------------------------------------
       PCMPcm.CIdModeA         = (GpakCidMode_t) ((Msg[16] >> 2) & 3);
       PCMPcm.CIdModeB         = (GpakCidMode_t) (Msg[16] & 3);
       PCMPcm.ToneGenGainG1A   = Msg[17];
       PCMPcm.OutputGainG2A    = Msg[18];
       PCMPcm.InputGainG3A     = Msg[19];
       PCMPcm.ToneGenGainG1B   = Msg[20];
       PCMPcm.OutputGainG2B    = Msg[21];
       PCMPcm.InputGainG3B     = Msg[22];
       pChanStatus->NumPktsToDsp    = 0;
       pChanStatus->NumPktsFromDsp  = 0;
       // --------------------------------------------------------
       break;

    case packetToPacket:
       if (ReplyLength < 14)
          return CssDspCommFailure;

       PKTPkt.PktInCodingA =           (GpakCodecs) Byte1 (Msg[7]);
       PKTPkt.PktInFrameSizeA =    (GpakFrameSizes) Byte0 (Msg[7]);
       PKTPkt.PktOutCodingA =          (GpakCodecs) Byte1 (Msg[8]);
       PKTPkt.PktOutFrameSizeA =   (GpakFrameSizes) Byte0 (Msg[8]);

       if (Msg[9] & 0x8000)  PKTPkt.EcanEnableA = Enabled;
       else                  PKTPkt.EcanEnableA = Disabled;

       if (Msg[9] & 0x4000)  PKTPkt.VadEnableA = Enabled;
       else                  PKTPkt.VadEnableA = Disabled;

       if (Msg[9] & 0x2000)  PKTPkt.EcanEnableB = Enabled;
       else                  PKTPkt.EcanEnableB = Disabled;

       if (Msg[9] & 0x1000)  PKTPkt.VadEnableB = Enabled;
       else                  PKTPkt.VadEnableB = Disabled;

       if (Msg[9] & 0x0800)  PKTPkt.AgcEnableA = Enabled;
       else                  PKTPkt.AgcEnableA = Disabled;

       if (Msg[9] & 0x0400)  PKTPkt.AgcEnableB = Enabled;
       else                  PKTPkt.AgcEnableB = Disabled;

       PKTPkt.ChannelIdB =                          Byte0 (Msg[9]);
       PKTPkt.PktInCodingB =           (GpakCodecs) Byte1 (Msg[10]);
       PKTPkt.PktInFrameSizeB =    (GpakFrameSizes) Byte0 (Msg[10]);
       PKTPkt.PktOutCodingB =          (GpakCodecs) Byte1 (Msg[11]);
       PKTPkt.PktOutFrameSizeB =   (GpakFrameSizes) Byte0 (Msg[11]);

       PKTPkt.ToneTypesA = (GpakToneTypes) (Msg[12] & 0xFFF);
       PKTPkt.ToneTypesB = (GpakToneTypes) (Msg[13] & 0xFFF);

       // 4_2 ---------------------------------
       pChanStatus->NumPktsToDsp    = Msg[14];
       pChanStatus->NumPktsFromDsp  = Msg[15];
       pChanStatus->NumPktsToDspB    = Msg[16];
       pChanStatus->NumPktsFromDspB  = Msg[17];
       // -------------------------------------
       break;

    case circuitData:
       if (ReplyLength < 11)
           return CssDspCommFailure;

       CIRCData.PcmInPort =  (GpakSerialPort_t) Byte1 (Msg[7]);
       CIRCData.PcmInSlot =                     Byte0 (Msg[7]);
       CIRCData.PcmOutPort = (GpakSerialPort_t) Byte1 (Msg[8]);
       CIRCData.PcmOutSlot =                    Byte0 (Msg[8]);
       CIRCData.MuxFactor =                     Byte1 (Msg[9]);
       CIRCData.PcmInPin =               Byte1 (Msg[10]);
       CIRCData.PcmOutPin =              Byte0 (Msg[10]);
       break;

    case conferencePcm:
       if (ReplyLength < 12)
        	 return CssDspCommFailure;

       CNFPcm.ConferenceId = Byte1 (Msg[7]);

       if (Msg[7] & 0x0020) CNFPcm.AgcInEnable = Enabled;
       else                 CNFPcm.AgcInEnable = Disabled;

       if (Msg[7] & 0x0010)  CNFPcm.EcanEnable = Enabled;
       else                  CNFPcm.EcanEnable = Disabled;

       if (Msg[7] & 0x0008)  CNFPcm.AECEcanEnable = Enabled;
       else                  CNFPcm.AECEcanEnable = Disabled;

       CNFPcm.PcmInPort =   (GpakSerialPort_t) Byte1 (Msg[8]);
       CNFPcm.PcmInSlot =                      Byte0 (Msg[8]);
       CNFPcm.PcmOutPort =  (GpakSerialPort_t) Byte1 (Msg[9]);
       CNFPcm.PcmOutSlot =                     Byte0 (Msg[9]);
       CNFPcm.PcmInPin =                Byte1 (Msg[10]);
       CNFPcm.PcmOutPin =               Byte0 (Msg[10]);

       CNFPcm.ToneTypes        =  (GpakToneTypes) (Msg[11] & 0xFFF);

       // 4_2 ----------------------------------------------------
       CNFPcm.AgcOutEnable  = Msg[12] & 1;
       CNFPcm.ToneGenGainG1 = Msg[13];
       CNFPcm.OutputGainG2  = Msg[14];
       CNFPcm.InputGainG3   = Msg[15];
       // --------------------------------------------------------
       break;

    case conferenceMultiRate:
    case conferencePacket:
       if (ReplyLength < 10)
        	 return CssDspCommFailure;

       CNFPkt.ConferenceId =   Byte1 (Msg[7]);

       if (Msg[7] & 0x0080) CNFPkt.VadEnable = Enabled;
       else                 CNFPkt.VadEnable = Disabled;

       if (Msg[7] & 0x0020) CNFPkt.AgcInEnable = Enabled;
       else                 CNFPkt.AgcInEnable = Disabled;

       if (Msg[7] & 0x0010) CNFPkt.EcanEnable = Enabled;
       else                 CNFPkt.EcanEnable = Disabled;

       CNFPkt.PktInCoding =  (GpakCodecs) Byte1 (Msg[8]);
       CNFPkt.PktOutCoding = (GpakCodecs) Byte0 (Msg[8]);
       CNFPkt.ToneTypes    = (GpakToneTypes) (Msg[9] & 0xFFF);

       // 4_2 ----------------------------------------------------
       CNFPkt.AgcOutEnable  = Msg[10] & 1;
       CNFPkt.ToneGenGainG1 = Msg[11];
       CNFPkt.OutputGainG2  = Msg[12];
       CNFPkt.InputGainG3   = Msg[13];
       pChanStatus->NumPktsToDsp    = Msg[14];
       pChanStatus->NumPktsFromDsp  = Msg[15];
       // --------------------------------------------------------

       if (ReplyLength < 17) CNFPkt.PktFrameSize = 0;
       else                  CNFPkt.PktFrameSize = Msg[16];

       break;

    case conferenceComposite:
       if (ReplyLength < 10)
          return CssDspCommFailure;

       CNFCmp.ConferenceId = Byte1 (Msg[7]);

       if (Msg[7] & 0x0080) CNFCmp.VadEnable = Enabled;
       else                 CNFCmp.VadEnable = Disabled;

       CNFCmp.PcmOutPort = (GpakSerialPort_t) Byte1 (Msg[8]);
       CNFCmp.PcmOutSlot =                    Byte0 (Msg[8]);
       CNFCmp.PktOutCoding =     (GpakCodecs) Byte1 (Msg[9]);
       CNFCmp.PcmOutPin =                     Byte0 (Msg[9]);
       break;

    // 4_2 ---------------------------------------
    case loopbackCoder:
       if (ReplyLength < 8)
          return CssDspCommFailure;

        LPBCdr.PktInCoding  = (GpakCodecs) Byte1 (Msg[7]);
        LPBCdr.PktOutCoding = (GpakCodecs) Byte0 (Msg[7]);
        pChanStatus->NumPktsToDsp    = Msg[8];
        pChanStatus->NumPktsFromDsp  = Msg[9];
        break;
    // --------------------------------------------

    default:
       return CssDspCommFailure;
    }
    return CssSuccess;
}

gpakGetChanStatStatus_t gpakGetChannelStatus (ADT_UInt32 DspId, ADT_UInt16 ChannelId,
    GpakChannelStatus_t *pChanStatus, GPAK_ChannelStatusStat_t *pStatus) {


    ADT_UInt32  MsgLenI16;                        /* message length */
    ADT_UInt32  MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;

    if (MaxDspCores <= DspId)
        return CssInvalidDsp;

    if (pDspIfBlk[DspId] == 0)
       DSPSync (DspId);

    if (ChannelId >= MaxChannels[DspId])
        return CssInvalidChannel;

    Msg = (ADT_UInt16 *) MsgBuffer;

    Msg[0] = MSG_CHAN_STATUS_RQST << 8;
    Msg[1] = (ADT_UInt16) (ChannelId << 8);

    MsgLenI16 = TransactCmd (DspId, MsgBuffer, 2, MSG_CHAN_STATUS_REPLY, MSG_BUFFER_ELEMENTS, 
	   				      BYTE_ID, ChannelId);
    if (!MsgLenI16)
        return CssDspCommFailure;

    pChanStatus->ChannelId = ChannelId;
    return gpakParseChanStatusResponse (MsgLenI16, Msg, pChanStatus, pStatus);

}

gpakAECParmsStatus_t gpakParseAecReadParmsResponse (ADT_UInt32 MsgLenI16, 
                            ADT_UInt16 *Msg, GpakAECParms_t   *AECParms) {

    AECParms->activeTailLengthMSec     = (ADT_Int16) Msg[1]; 
    AECParms->totalTailLengthMSec      = (ADT_Int16) Msg[2]; 
    AECParms->maxTxNLPThresholddB      = (ADT_Int16) Msg[3]; 
    AECParms->maxTxLossdB              = (ADT_Int16) Msg[4]; 
    AECParms->maxRxLossdB              = (ADT_Int16) Msg[5]; 
    AECParms->targetResidualLeveldBm   = (ADT_Int16) Msg[6]; 
    AECParms->maxRxNoiseLeveldBm       = (ADT_Int16) Msg[7]; 
    AECParms->worstExpectedERLdB       = (ADT_Int16) Msg[8]; 
    AECParms->noiseReductionEnable     = (GpakActivation) Msg[9]; 
    AECParms->cngEnable                = (GpakActivation) Msg[10];
    AECParms->agcEnable                = (GpakActivation) Msg[11];
    AECParms->agcMaxGaindB             = (ADT_Int16) Msg[12];
    AECParms->agcMaxLossdB             = (ADT_Int16) Msg[13];
    AECParms->agcTargetLeveldBm        = (ADT_Int16) Msg[14];
    AECParms->agcLowSigThreshdBm       = (ADT_Int16) Msg[15];
    AECParms->maxTrainingTimeMSec      = (ADT_UInt16) Msg[16];
    AECParms->rxSaturateLeveldBm       = (ADT_Int16) Msg[17];
    AECParms->trainingRxNoiseLeveldBm  = (ADT_Int16) Msg[18];  
    AECParms->fixedGaindB10            = (ADT_Int16) Msg[19];  

    return AECSuccess;
}

gpakAECParmsStatus_t gpakReadAECParms( ADT_UInt32        DspId, 
                                       GpakAECParms_t   *AECParms) {

    ADT_UInt32  MsgLenI16;                        /* message length */
    ADT_UInt32  MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;

    if (MaxDspCores <= DspId)
        return AECInvalidDsp;

    if (pDspIfBlk[DspId] == 0)
       DSPSync (DspId);

    Msg = (ADT_UInt16 *) MsgBuffer;

    Msg[0] = MSG_READ_AEC_PARMS << 8;

    MsgLenI16 = TransactCmd (DspId, MsgBuffer, 2, MSG_READ_AEC_PARMS_REPLY, MSG_BUFFER_ELEMENTS, 
	   				      BYTE_ID, 0);
    if (!MsgLenI16)
        return AECDspCommFailure;

    return gpakParseAecReadParmsResponse (MsgLenI16, Msg, AECParms);

}

ADT_UInt32 gpakFormatAecWriteParmsMsg (ADT_UInt16 *Msg, GpakAECParms_t   *AECParms) {

    /* Build the Configure Serial Ports message. */
    Msg[0]   = MSG_WRITE_AEC_PARMS  << 8;
    Msg[1]   = (ADT_UInt16) AECParms->activeTailLengthMSec;
    Msg[2]   = (ADT_UInt16) AECParms->totalTailLengthMSec;
    Msg[3]   = (ADT_UInt16) AECParms->maxTxNLPThresholddB;
    Msg[4]   = (ADT_UInt16) AECParms->maxTxLossdB;
    Msg[5]   = (ADT_UInt16) AECParms->maxRxLossdB;
    Msg[6]   = (ADT_UInt16) AECParms->targetResidualLeveldBm;
    Msg[7]   = (ADT_UInt16) AECParms->maxRxNoiseLeveldBm;
    Msg[8]   = (ADT_UInt16) AECParms->worstExpectedERLdB;
    Msg[9]   = (ADT_UInt16) AECParms->noiseReductionEnable;
    Msg[10]  = (ADT_UInt16) AECParms->cngEnable;
    Msg[11]  = (ADT_UInt16) AECParms->agcEnable;
    Msg[12]  = (ADT_UInt16) AECParms->agcMaxGaindB;
    Msg[13]  = (ADT_UInt16) AECParms->agcMaxLossdB;
    Msg[14]  = (ADT_UInt16) AECParms->agcTargetLeveldBm;
    Msg[15]  = (ADT_UInt16) AECParms->agcLowSigThreshdBm;
    Msg[16]  = (ADT_UInt16) AECParms->maxTrainingTimeMSec;
    Msg[17]  = (ADT_UInt16) AECParms->rxSaturateLeveldBm;
    Msg[18]  = (ADT_UInt16) AECParms->trainingRxNoiseLeveldBm;
    Msg[19]  = (ADT_UInt16) AECParms->fixedGaindB10;
    return 20;
}


gpakAECParmsStatus_t gpakWriteAECParms( ADT_UInt32           DspId, 
                                        GpakAECParms_t      *AECParms, 
                                        GPAK_AECParmsStat_t *Status) {

    ADT_UInt32 MsgLenI16;                     /* message length */
    ADT_UInt32 MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;


    if (MaxDspCores <= DspId)
        return AECInvalidDsp;

    Msg =(ADT_UInt16 *) &MsgBuffer[0];

    MsgLenI16 = gpakFormatAecWriteParmsMsg (Msg, AECParms);
    if (!TransactCmd (DspId, MsgBuffer, MsgLenI16, MSG_WRITE_AEC_PARMS_REPLY, 2,
                      BYTE_ID, (ADT_UInt16) 0))
        return AECDspCommFailure;

    /* Return success or failure . */
    *Status = (GPAK_AECParmsStat_t) Byte0 (Msg[1]);
    if (*Status == AEC_Success)  return AECSuccess;
    else                         return AECParmError;

}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakConfigureConference - Configure a DSP's Conference.
 *
 * FUNCTION
 *  Configures a DSP's Conference.
 *
 * Inputs
 *      DspId     - Dsp identifier 
 * ConferenceId   - Conference identifier
 * FrameSize      - Samples per conference frame
 * InputToneTypes - Bit map of tone types for tone detection
 *
 * Outputs
 *    pStatus  - conference configuration status from DSP
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
gpakConfigConferStatus_t gpakConfigureConference (ADT_UInt32 DspId, ADT_UInt16 ConferenceId,
            GpakFrameSizes FrameSize,  GpakToneTypes InputToneTypes,
            GPAK_ConferConfigStat_t *pStatus) {

    ADT_UInt32  MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;

    if (MaxDspCores <= DspId)
        return CfsInvalidDsp;

    Msg    = (ADT_UInt16 *) MsgBuffer;

    Msg[0] = MSG_CONFIG_CONFERENCE << 8;
    Msg[1] = (ADT_UInt16) PackBytes (ConferenceId, FrameSize);
    Msg[2] = (ADT_UInt16) InputToneTypes && 0xFFF;

    if (!TransactCmd (DspId, MsgBuffer, 3, MSG_CONFIG_CONF_REPLY, 2, 
		              BYTE_ID, ConferenceId))
        return CfsDspCommFailure;

    *pStatus = (GPAK_ConferConfigStat_t) Byte0 (Msg[1]);
    if (*pStatus == Cf_Success)
        return CfsSuccess;
    else
        return CfsParmError;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakConfigArbToneDetect - Configure a DSP's Arbitrary Tone Detector.
 *
 * FUNCTION
 *  This function configures a DSP's Arbitrary Tone Detector and sets it as the
 *  active configuration for subsequent channels that run the detector.
 *
 * Inputs
 *      DspId          - Dsp identifier 
 *      ArbCfgId       - Arb Detector configuration identifier
 *      pArbParams     - Arb Detector configuration params 
 *      pArbTones      - tones used by this configuration 
 *
 * Outputs
 *    pStatus  - conference configuration status from DSP
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
gpakConfigArbToneStatus_t gpakConfigArbToneDetect (
    ADT_UInt32 DspId, ADT_UInt16 ArbCfgId, GpakArbTdParams_t *pArbParams,
    GpakArbTdToneIdx_t *pArbTones,  ADT_UInt16 *pArbFreqs,
    GPAK_ConfigArbToneStat_t *pStatus) {
    
    ADT_UInt32  MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg, *Msg1, i, lenI16;

    if (MaxDspCores <= DspId)
        return CatInvalidDsp;

    Msg1    = (ADT_UInt16 *) MsgBuffer;
    Msg = Msg1;

    Msg[0] = MSG_CONFIG_ARB_DETECTOR << 8;
    Msg[1] = ArbCfgId;
 
    Msg[2] = (ADT_UInt16) PackBytes (pArbParams->numDistinctFreqs, pArbParams->numTones);
    Msg[3] = (ADT_UInt16) PackBytes (pArbParams->minPower, pArbParams->maxFreqDeviation);
    lenI16 = 4;

    // maximum of 32 distinct frequencies
    Msg  = &Msg1[lenI16];
    for (i=0; i<32; i++) {
        if (i < pArbParams->numDistinctFreqs) {
            Msg[i] = pArbFreqs[i];
        } else {
            Msg[i] = 0;
        }
    }
    lenI16 += 32;
    
    // maximum of 16 tones
    Msg  = &Msg1[lenI16];
    for (i=0; i<16; i++) {
        if (i < pArbParams->numTones) {
            Msg[3*i]   = pArbTones[i].f1;
            Msg[3*i+1] = pArbTones[i].f2;
            Msg[3*i+2] = pArbTones[i].index;
        } else {
            Msg[3*i]   = 0;
            Msg[3*i+1] = 0;
            Msg[3*i+2] = 0;
        }
    }
    lenI16 += 48;

    if (!TransactCmd (DspId, MsgBuffer, lenI16, MSG_CONFIG_ARB_DETECTOR_REPLY, 2, 
		              BYTE_ID, ArbCfgId))
        return CatDspCommFailure;

    Msg  = Msg1;
    *pStatus = (GPAK_ConfigArbToneStat_t) Byte0 (Msg[1]);
    if (*pStatus == Cat_Success)
        return CatSuccess;
    else
        return CatParmError;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakActivateArbToneDetect - Activate a DSP's Arbitrary Tone Detector.
 *
 * FUNCTION
 *  This function activates a DSP's Arbitrary Tone Detector. Any channels
 *  that are configured to run arbitrary tone detection will automatically
 *  use the arbitrary detector configuration that is currently active.
 *
 * Inputs
 *      DspId          - Dsp identifier 
 *      ArbCfgId       - Arb Detector configuration identifier
 *
 * Outputs
 *    pStatus  - conference configuration status from DSP
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
gpakActiveArbToneStatus_t gpakActiveArbToneDetect (
    ADT_UInt32 DspId, ADT_UInt16 ArbCfgId,
    GPAK_ActiveArbToneStat_t *pStatus) {
    
    ADT_UInt32  MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;

    if (MaxDspCores <= DspId)
        return AatInvalidDsp;

    Msg    = (ADT_UInt16 *) MsgBuffer;

    Msg[0] = MSG_ACTIVE_ARB_DETECTOR << 8;
    Msg[1] = ArbCfgId;
    if (!TransactCmd (DspId, MsgBuffer, 2, MSG_ACTIVE_ARB_DETECTOR_REPLY, 2, 
		              BYTE_ID, ArbCfgId))
        return AatDspCommFailure;

    *pStatus = (GPAK_ActiveArbToneStat_t) Byte0 (Msg[1]);
    if (*pStatus == Aat_Success)
        return AatSuccess;
    else
        return AatParmError;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakSendRTPMsg 
 *
 * FUNCTION
 *    Configure a channel's RTP parameters.
 *
 * Inputs
 *      DspId  - Dsp identifier 
 * ChannelId   - Channel identifier
 * Cfg         - Pointer to structure containing RTP configuration parameters
 *
 * Outputs
 *    pStatus  - RTP configuration status from DSP
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */

ADT_UInt32 gpakFormatRTPMsg (ADT_UInt16 *Msg,  ADT_UInt16  ChannelId, GpakRTPCfg_t *Cfg) {

    /* Build the Configure Serial Ports message. */
    Msg[0]  = MSG_RTP_CONFIG  << 8;
    Msg[1]  = PackBytes (Cfg->JitterMode, ChannelId);

    Msg[2]  = (ADT_UInt16)((Cfg->SSRC >> 16) & 0xffff);
    Msg[3]  = (ADT_UInt16)(Cfg->SSRC & 0xffff);
    Msg[4]  = Cfg->StartSequence;

    Msg[5]  = Cfg->DelayTargetMinMS;
    Msg[6]  = Cfg->DelayTargetMS;
    Msg[7]  = Cfg->DelayTargetMaxMS;
    
    Msg[8]  = PackBytes (Cfg->CNGPyldType, Cfg->VoicePyldType);
    Msg[9]  = PackBytes (Cfg->T38PyldType, Cfg->TonePyldType);

    Msg[10] = Cfg->DestPort;
    Msg[11] = (ADT_UInt16)((Cfg->DestIP >> 16) & 0xffff);
    Msg[12] = (ADT_UInt16)(Cfg->DestIP & 0xffff);
    return 13;
}

gpakRtpStatus_t gpakSendRTPMsg (ADT_UInt32 DspId,  ADT_UInt16 ChannelId, 
                                GpakRTPCfg_t *Cfg, GPAK_RTPConfigStat_t *dspStatus) {

    ADT_UInt32 MsgLenI16;                     /* message length */
    ADT_UInt32 MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;


    if (MaxDspCores <= DspId)
        return RtpInvalidDsp;

    Msg =(ADT_UInt16 *) &MsgBuffer[0];

    MsgLenI16 = gpakFormatRTPMsg (Msg, ChannelId, Cfg);
    if (!TransactCmd (DspId, MsgBuffer, MsgLenI16, MSG_RTP_CONFIG_REPLY, 2,
                      BYTE_ID, (ADT_UInt16) ChannelId))
        return RtpDspCommFailure;

    /* Return success or failure . */
    *dspStatus = (GPAK_RTPConfigStat_t) Byte0 (Msg[1]);
    if (*dspStatus == RTPSuccess)  return RtpSuccess;
    else                           return RtpParmError;
}

//}==================================================================================
//{                  Control routines
//===================================================================================

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakTestMode - Configure/perform a DSP's Test mode.
 *
 * FUNCTION
 *  Configures or performs a DSP's Test mode.
 *
 * Inputs
 *   DspId        - Dsp identifier 
 *   TestModeId   - Test identifier
 *   pTestParm    - pointer to TestMode-specific Test parameters
 *
 * Outputs
 *    pRespData - Response value pointer
 *    pStatus   - test mode status from DSP
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
ADT_UInt32 formatTestModeMsg(ADT_UInt16 *Msg, GpakTestMode TestModeId, 
                                       GpakTestData_t  *pTestParm, ADT_UInt16 buflenI16) {

    ADT_UInt32 MsgLenI16, CustLenI16, i;
    ADT_UInt16 *pCustMsg;

    Msg[0] = MSG_TEST_MODE << 8;
    Msg[1] = (ADT_UInt16) TestModeId;

    switch (TestModeId) {
        case TdmFixedValue:
            Msg[2] = pTestParm->u.fixedVal.ChannelId;
            Msg[3] = pTestParm->u.fixedVal.PcmOutPort;
            Msg[4] = pTestParm->u.fixedVal.PcmOutSlot;
            Msg[5] = pTestParm->u.fixedVal.Value;
            Msg[6] = pTestParm->u.fixedVal.State;
            MsgLenI16 = 7;
            break;

        case TdmLoopback:
            Msg[2] = pTestParm->u.tdmLoop.PcmOutPort;
            Msg[3] = pTestParm->u.tdmLoop.State;
            MsgLenI16 = 4;
            break;

        case CustomVarLen:
            MsgLenI16  = 2;
            pCustMsg = (ADT_UInt16 *)pTestParm;
            CustLenI16 = pCustMsg[1];
            if ((CustLenI16 + MsgLenI16) > buflenI16)
                return 0;
            for (i=0; i<CustLenI16; i++) {
                Msg[2+i] = pCustMsg[i];
            }
            MsgLenI16 += CustLenI16;
            break;
                    
        default:    
            Msg[2] = pTestParm->u.TestParm;
            MsgLenI16 = 3;
            break;
    };

    return MsgLenI16;
}

gpakTestStatus_t gpakTestMode (ADT_UInt32 DspId, GpakTestMode TestModeId,
                               GpakTestData_t  *pTestParm, ADT_UInt16 *pRespData,
                               GPAK_TestStat_t *pStatus) {

    ADT_UInt32  MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;
    ADT_UInt32 MsgLenI16;


    if (MaxDspCores <= DspId)
        return CtmInvalidDsp;

    Msg    = (ADT_UInt16 *) MsgBuffer;

    MsgLenI16 = formatTestModeMsg(Msg, TestModeId, pTestParm, sizeof(MsgBuffer)/sizeof(ADT_UInt16));

    if (MsgLenI16 == 0) return CtmCustomMsgError;
    
    if (!TransactCmd (DspId, MsgBuffer, MsgLenI16, MSG_TEST_REPLY, 4, 
		              WORD_ID, (ADT_UInt16) TestModeId))
        return CtmDspCommFailure;

    *pRespData = Msg[3];

    *pStatus = (GPAK_TestStat_t) Msg[2];
    if (*pStatus == Tm_Success)
        return CtmSuccess;
    else
        return CtmParmError;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakToneGenerate - Write tone generation paramaters to the DSP.
 *
 * FUNCTION
 *  This function writes the tone generation parameters into DSP memory 
 *  
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
ADT_UInt32 gpakFormatToneGenerator (ADT_UInt16 *Msg, ADT_UInt16 ChannelId, 
                                   GpakToneGenParms_t *tone) {

ADT_UInt32 MsgLenI16;

   /* Format the tone generation message */

    Msg[0] = (MSG_TONEGEN << 8);
    Msg[1] = ChannelId;
    Msg[2] = (ADT_UInt16) ( ((tone->ToneType  & 3) << 2) | 
                            ((tone->Device    & 1) << 1) | 
                            (tone->ToneCmd   & 1) );
    Msg[3] = tone->Frequency[0];
    Msg[4] = tone->Frequency[1];
    Msg[5] = tone->Frequency[2];
    Msg[6] = tone->Frequency[3];
    Msg[7] = tone->Level[0];
    Msg[8] = tone->OnDuration[0];
    Msg[9] = tone->OffDuration[0];
    MsgLenI16 = 10;

    // 4_2 ----------------------------------
    Msg[10] = tone->Level[1];
    Msg[11] = tone->Level[2];
    Msg[12] = tone->Level[3];
    Msg[13] = tone->OnDuration[1];
    Msg[14] = tone->OnDuration[2];
    Msg[15] = tone->OnDuration[3];
    Msg[16] = tone->OffDuration[1];
    Msg[17] = tone->OffDuration[2];
    Msg[18] = tone->OffDuration[3];
    MsgLenI16 += 9;
    // ----------------------------------

    return MsgLenI16;
}


gpakGenToneStatus_t gpakToneGenerate (ADT_UInt32 DspId, ADT_UInt16 ChannelId, 
               GpakToneGenParms_t *Tone, GPAK_ToneGenStat_t *pStatus) {

    ADT_UInt32  MsgLenI16;                        /* message length */
    ADT_UInt32  MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;

    if (MaxDspCores <= DspId)
        return TgcInvalidDsp;

    if (pDspIfBlk[DspId] == 0)
       DSPSync (DspId);

    if (ChannelId >= MaxChannels[DspId])
        return TgcInvalidChannel;

    Msg    = (ADT_UInt16 *) MsgBuffer;

    MsgLenI16 = gpakFormatToneGenerator (Msg, ChannelId, Tone);

    if (!TransactCmd (DspId, MsgBuffer, MsgLenI16, MSG_TONEGEN_REPLY, MsgLenI16, 
		              WORD_ID, ChannelId))
        return TgcDspCommFailure;

    *pStatus = (GPAK_ToneGenStat_t) Byte1 (Msg[2]);
    if (*pStatus == Tg_Success)  return TgcSuccess;
    else                         return TgcParmError;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakConferenceGenerateTone - Control tone generation.
 *
 * FUNCTION
 *  This function controls the generation of tones on the specified conference.
 *
 * INPUTS
 *  DspId,      	- DSP identifier
 *  ConfId,	    	- conference identifier
 *  *pToneParms,    - pointer to tone gen parameters
 *  *pStatus 	    - pointer to DSP status reply
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
ADT_UInt32 gpakFormatConfToneGenerator (ADT_UInt16 *Msg, ADT_UInt16 ConfId, 
                                   GpakToneGenParms_t *tone) {

ADT_UInt32 MsgLenI16;

   /* Format the tone generation message */

    Msg[0] = (MSG_CONFTONEGEN << 8);
    Msg[1] = ConfId;
    Msg[2] = (ADT_UInt16) ( ((tone->ToneType  & 3) << 2) | 
                            ((tone->Device    & 1) << 1) | 
                            (tone->ToneCmd   & 1) );
    Msg[3] = tone->Frequency[0];
    Msg[4] = tone->Frequency[1];
    Msg[5] = tone->Frequency[2];
    Msg[6] = tone->Frequency[3];
    Msg[7] = tone->Level[0];
    Msg[8] = tone->OnDuration[0];
    Msg[9] = tone->OffDuration[0];
    MsgLenI16 = 10;

    // ----------------------------------
    Msg[10] = tone->Level[1];
    Msg[11] = tone->Level[2];
    Msg[12] = tone->Level[3];
    Msg[13] = tone->OnDuration[1];
    Msg[14] = tone->OnDuration[2];
    Msg[15] = tone->OnDuration[3];
    Msg[16] = tone->OffDuration[1];
    Msg[17] = tone->OffDuration[2];
    Msg[18] = tone->OffDuration[3];
    MsgLenI16 += 9;
    // ----------------------------------

    return MsgLenI16;
}

gpakGenToneStatus_t gpakConferenceGenerateTone (ADT_UInt32 DspId, ADT_UInt16 ConfId, 
               GpakToneGenParms_t *Tone, GPAK_ToneGenStat_t *pStatus) {

    ADT_UInt32  MsgLenI16;                        /* message length */
    ADT_UInt32  MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;

    if (MaxDspCores <= DspId)
        return TgcInvalidDsp;

    Msg    = (ADT_UInt16 *) MsgBuffer;

    MsgLenI16 = gpakFormatConfToneGenerator (Msg, ConfId, Tone);

    if (!TransactCmd (DspId, MsgBuffer, MsgLenI16, MSG_CONFTONEGEN_REPLY, MsgLenI16, 
		              WORD_ID, ConfId))
        return TgcDspCommFailure;

    *pStatus = (GPAK_ToneGenStat_t) Byte1 (Msg[2]);
    if (*pStatus == Tg_Success)  return TgcSuccess;
    else                         return TgcParmError;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *   gpakSendPlayRecordMsg 
 *
 * FUNCTION
 *  This function is used to issue start/stop voice recording and start
 *  voice playback commands to the DSP.
 *
 *
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


ADT_UInt32 gpakFormatPlayRecordMsg (ADT_UInt16 *Msg, ADT_UInt16 ChanId,
                GpakDeviceSide_t Device,       GpakPlayRecordCmd_t Cmd,  
                DSP_Address      BuffAddr,     ADT_UInt32 BuffLen, 
                GpakPlayRecordMode_t RecMode,  GpakToneCodes_t StopTone) {
   
   Msg[0] = PackBytes (MSG_PLAY_RECORD, (int) Cmd);
   Msg[1] = ChanId;
   Msg[2] = HighWord (BuffAddr);
   Msg[3] = LowWord  (BuffAddr);
   Msg[4] = HighWord (BuffLen);
   Msg[5] = LowWord  (BuffLen);
   Msg[6] = PackBytes (RecMode, StopTone);
   Msg[7] = PackBytes (0, Device & 1);
   return 8;
}

gpakPlayRecordStatus_t gpakSendPlayRecordMsg (ADT_UInt32 DspId,
       ADT_UInt16 ChanId,        GpakDeviceSide_t Device, 
       GpakPlayRecordCmd_t Cmd,  DSP_Address      BuffAddr,
       ADT_UInt32 BuffLen,       GpakPlayRecordMode_t RecMode,
       GpakToneCodes_t StopTone, GPAK_PlayRecordStat_t *pStatus) {

    ADT_UInt32  MsgLenI16;                        /* message length */
    ADT_UInt32  MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;

    if (MaxDspCores <= DspId)
        return PrsInvalidDsp;

    if (pDspIfBlk[DspId] == 0)
       DSPSync (DspId);

    if (ChanId >= MaxChannels[DspId])
        return PrsInvalidChannel;

    Msg    = (ADT_UInt16 *) MsgBuffer;

    MsgLenI16 = gpakFormatPlayRecordMsg (Msg, ChanId, Device, Cmd, BuffAddr,
		                              BuffLen, RecMode, StopTone);

	 if (!TransactCmd (DspId, MsgBuffer, MsgLenI16, MSG_PLAY_RECORD_RESPONSE, 3, 
		               WORD_ID, ChanId))
        return PrsDspCommFailure;

    *pStatus = (GPAK_PlayRecordStat_t) Byte1 (Msg[2]);
    if (*pStatus == PrSuccess)  return PrsSuccess;
    else                        return PrsParmError;
}


//===================================================================================

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakReadCpuUsage - Read CPU usage statistics from a DSP.
 *
 * FUNCTION
 *  Reads the CPU usage statistics from a DSP's memory. The
 *  average CPU usage in units of .1 percent are obtained for each of the frame
 *  rates.
 *
 * Inputs
 *     DspId      - Dsp identifier 
 * 
 * Outputs
 *  pxxmsUsage - Per frame size pointers to receive CPU usage  (.1% granularity)
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
gpakCpuUsageStatus_t gpakReadCpuUsage (ADT_UInt32 DspId, ADT_UInt32  *AvgCpuUsage, ADT_UInt32  *PeakCpuUsage) {

    DSP_Word ReadBuffer[7];     /* DSP read buffer */
    int i;

    if (MaxDspCores <= DspId)
        return GcuInvalidDsp;

    gpakLockAccess (DspId);
    if (CheckDspReset (DspId) == -1) {
       gpakUnlockAccess (DspId);
       return GcuDspCommFailure;
    }

    /* Read the CPU Usage statistics from the DSP. */
    gpakReadDsp (DspId, pDspIfBlk[DspId] + CPU_USAGE_OFFSET, 7, ReadBuffer);

    /* Store the usage statistics in the specified variables. */
    for (i=0; i<7; i++)
        *AvgCpuUsage++  = (ADT_UInt32) ReadBuffer[i];

    /* Read the CPU peak Usage statistics from the DSP. */
    gpakReadDsp (DspId, pDspIfBlk[DspId] + CPU_PKUSAGE_OFFSET, 7, ReadBuffer);

    /* Store the usage statistics in the specified variables. */
    for (i=0; i<7; i++)
        *PeakCpuUsage++ = (ADT_UInt32) ReadBuffer[i];

    gpakUnlockAccess(DspId);

    return GcuSuccess;
}


ADT_Int32 gpakReadRTPClock (ADT_UInt32 DspId) {
    
    ADT_Int32 RTPClock;

    if (MaxDspCores <= DspId)
        return (0);

    gpakLockAccess (DspId);
    gpakReadDsp32 (DspId, pDspIfBlk[DspId] + RTP_CLOCK_OFFSET,  BytesToTxUnits (4), (ADT_UInt32 *) &RTPClock);
    gpakUnlockAccess (DspId);
    return RTPClock;
}

int gpakReadMaxChannels (ADT_UInt32 DspId, ADT_UInt32 *MaxChannels) {

    if (MaxDspCores <= DspId)
        return (-1);

    gpakLockAccess (DspId);
    if (CheckDspReset(DspId) == -1) {
        gpakUnlockAccess (DspId);
        return (-1);
    }
    gpakUnlockAccess (DspId);

    *MaxChannels = (ADT_UInt32) MaxChannels[DspId];
    return (0);
}

#if defined(LINUXOS) || defined(WIN32)
static int readCaptBuff(char *filename, ADT_UInt32 dsp, ADT_UInt32 theLenI32, ADT_UInt32 theAddr) {
   FILE *outFile;
   ADT_UInt32 value;
   ADT_UInt16 *p16 = (ADT_UInt16 *)&value;
   ADT_UInt32 lenI32, addr;

   lenI32 = theLenI32;
   addr = theAddr;

   outFile = fopen (filename, "wb");
   if (outFile == NULL) { 
      return -1;
   }

   do {
   
      gpakLockAccess (dsp);
      gpakReadDspMemory32 (dsp, addr, 1, (ADT_UInt32 *) &value);
      gpakUnlockAccess (dsp);
 
      fwrite (&p16[0], 1, 2, outFile);
      fwrite (&p16[1], 1, 2, outFile);
      addr += 4;
      lenI32 -= 1;
   } while (0 < lenI32);
 
   fclose (outFile);
   return 0;
}

static int dumpCapture   (ADT_UInt32 dsp, 
                            struct captureData *evt) {
   int rv = 0;

   rv |= readCaptBuff("CaptA.pcm", dsp, evt->ALenI32, evt->AAddr);
   rv |= readCaptBuff("CaptB.pcm", dsp, evt->BLenI32, evt->BAddr);
   rv |= readCaptBuff("CaptC.pcm", dsp, evt->CLenI32, evt->CAddr);
   rv |= readCaptBuff("CaptD.pcm", dsp, evt->DLenI32, evt->DAddr);

   return rv;
}
#endif /* LINUXOS || WIN32 */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakReadEventFIFOMessage - read from the event fifo
 * 
 * FUNCTION
 *  This function reads a single event from the event fifo if one is available
 * 
 * RETURNS
 *  Status  code indicating success or a specific error.
 *
 * Notes: This function should be called in a loop until the return status 
 *        indicates that the fifo is empty.
 *      
 *        If the event code equals "EventLoopbackTeardownComplete", then the 
 *        contents of *pChannelId hold the coderBlockId that was assigned to
 *        the loopback coder that was torn down.
 */
gpakReadEventFIFOMessageStat_t gpakReadEventFIFOMessage (ADT_UInt32 DspId,
            ADT_Int16 *pChannelId, GpakAsyncEventCode_t *pEventCode,
            GpakAsyncEventData_t *pEventData) {


    DSP_Address EvtAddr;         /* address of EventFIFO info structure */
    CircBufr_t cBuff;

    ADT_Int16 ChannelId;                     /* DSP's channel Id */
    DSP_Word I16Free, I16Ready, TxHdrSize;

    GpakAsyncEventCode_t EvtCode;            /* DSP's event code */
    DSP_Word EvtBuffer_[(EVENT_BUFFER_SIZE+1)/2];   /* Event buffer */
    ADT_UInt16 *EvtBuffer = (ADT_UInt16 *)EvtBuffer_;   /* Event buffer */
    DSP_Word EvtDataWds, EvtDataBytes;       /* Length of event data */
    ADT_UInt16 evtPaylen;
    ADT_UInt8 cidType, cidLen;

    ADT_Bool EvtError;    /* flag indicating error with event fifo msg  */

    if (MaxDspCores <= DspId)
        return (RefInvalidDsp);

    gpakLockAccess (DspId);

    if (CheckDspReset(DspId) == -1) {
        gpakUnlockAccess (DspId);
        return (RefDspCommFailure);
    }

	// Read circular buffer structure into host memory
    EvtAddr = pEventFifoAddress[DspId];
    if (EvtAddr == 0) {
       gpakUnlockAccess (DspId);
       return (RefNoEventQue);
    }

	TxHdrSize = BytesToCircUnits (8);
    ReadCircStruct (DspId, EvtAddr, &cBuff, &I16Free, &I16Ready);
    if (I16Ready < TxHdrSize) {
	   if (I16Ready != 0) PurgeCircStruct (DspId, EvtAddr, &cBuff);
       gpakUnlockAccess (DspId);
       return (RefNoEventAvail);
    }

    // Read event data from buffer
    ReadCircBuffer (DspId, &cBuff, &EvtBuffer_[0], TxHdrSize, EvtAddr);
#if (DSP_WORD_SIZE_I8 == 4)
    if (HostBigEndian)
        gpakI16Swap_be (&EvtBuffer_[0], 8);
    else
        gpakI16Swap_le (&EvtBuffer_[0], 8);
#endif
    ChannelId              = EvtBuffer[0];
    EvtCode                = (GpakAsyncEventCode_t) EvtBuffer[1];
 	EvtDataBytes           = EvtBuffer[2];
    EvtDataWds             = BytesToCircUnits (EvtDataBytes);
    pEventData->ToneDevice = (GpakDeviceSide_t) EvtBuffer[3];

    if (EVENT_BUFFER_SIZE < EvtDataWds) {
	   PurgeCircStruct (DspId, EvtAddr, &cBuff);
       gpakUnlockAccess (DspId);
       return (RefInvalidEvent);
    }

    EvtError = 0;
    switch (EvtCode) {
    case EventToneDetect:
       if ((EvtDataBytes != 2) && (EvtDataBytes != 4)) {
  	      PurgeCircStruct (DspId, EvtAddr, &cBuff);
          EvtError = 1;
          break;
       }

       ReadCircBuffer (DspId, &cBuff, (DSP_Word *) &EvtBuffer[0], EvtDataWds, EvtAddr);
#if (DSP_WORD_SIZE_I8 == 4)
       if (HostBigEndian)
            gpakI16Swap_be (&EvtBuffer_[0], EvtDataBytes);
       else
            gpakI16Swap_le (&EvtBuffer_[0], EvtDataBytes);
#endif
       pEventData->aux.toneEvent.ToneCode = (GpakToneCodes_t)  EvtBuffer[0];

       pEventData->aux.toneEvent.ToneDuration = 0;
       if (EvtDataBytes == 4)
            pEventData->aux.toneEvent.ToneDuration = (GpakToneCodes_t)  EvtBuffer[1];

       break;

    case EventDspReady:
    case EventPlaybackComplete:
    case EventWarning:
       break;

    case EventRecordingStopped:
    case EventRecordingBufferFull:
       if (EvtDataBytes != 4) {
  	      PurgeCircStruct (DspId, EvtAddr, &cBuff);
          EvtError = 1;
          break;
       }

       ReadCircBuffer (DspId, &cBuff, (DSP_Word *) &EvtBuffer[0], EvtDataWds, EvtAddr);

#if (DSP_WORD_SIZE_I8 == 4)
       if (HostBigEndian)
            gpakI16Swap_be (&EvtBuffer_[0], EvtDataBytes);                  
       else
            gpakI16Swap_le (&EvtBuffer_[0], EvtDataBytes);                  

       pEventData->aux.recordEvent.RecordLength = (ADT_UInt32) 
                   ((EvtBuffer[0] << 16) | (EvtBuffer[1] & 0xffff));
#endif
        break;

    case EventFaxComplete:
    case EventCaptureComplete:

       if (byteSize (struct captureData) < EvtDataBytes) {
  	      PurgeCircStruct (DspId, EvtAddr, &cBuff);
          EvtError = 1;
          break;
       }
       memset (&pEventData->aux.captureData, 0, sizeof (struct captureData));
       ReadCircBuffer (DspId, &cBuff, (DSP_Word *) &pEventData->aux.captureData, EvtDataWds, EvtAddr);
#if defined(LINUXOS) || defined(WIN32)
       dumpCapture (DspId, &pEventData->aux.captureData);             
#endif
       break;

    case EventVadStateVoice:
    case EventVadStateSilence:
    case EventTxCidMessageComplete:
    case EventRxCidCarrierLock:
    case EventLoopbackTeardownComplete:
        break;

    case EventRxCidMessageComplete:
       if (byteSize (struct rxCidEvent) < EvtDataBytes) {
  	      PurgeCircStruct (DspId, EvtAddr, &cBuff);
          EvtError = 1;
          break;
       }

       // read the RxCId paylen in I16word[0], followed by the cidMsgtype in HighByte and cidMsgLen in LowByte of I16word[1]
       ReadCircBufferNoSwap (DspId, &cBuff, (DSP_Word *) &EvtBuffer[0], 2, EvtAddr);

       if (HostBigEndian)
            gpakI8Swap_be (&EvtBuffer[0], &evtPaylen, 1);
       else
            gpakI8Swap_le (&EvtBuffer[0], &evtPaylen, 1);
      

       cidType = (ADT_UInt8) ((EvtBuffer[1] >> 8) & 0xFF);
       cidLen  = (ADT_UInt8) EvtBuffer[1] & 0xFF;

       // read the RxCId Msg data payload
       ReadCircBufferNoSwap (DspId, &cBuff, (DSP_Word *) &EvtBuffer[0], EvtDataWds-2, EvtAddr);
       pEventData->aux.rxCidEvent.Payload[0] = cidType;
       pEventData->aux.rxCidEvent.Payload[1] = cidLen;
       memcpy(&(pEventData->aux.rxCidEvent.Payload[2]), &EvtBuffer[0], evtPaylen);
       pEventData->aux.rxCidEvent.Length = evtPaylen;
       break;

    case EventDSPDebug:
       if (byteSize (struct dspDebugEvent) < EvtDataBytes) {
  	      PurgeCircStruct (DspId, EvtAddr, &cBuff);
          EvtError = 1;
          break;
       }
       ReadCircBuffer (DspId, &cBuff, (DSP_Word *) &EvtBuffer[0], EvtDataWds, EvtAddr);
#if (DSP_WORD_SIZE_I8 == 4)
       if (HostBigEndian)
            gpakI16Swap_be (&EvtBuffer_[0], EvtDataBytes);
       else
            gpakI16Swap_le (&EvtBuffer_[0], EvtDataBytes);
#endif
       pEventData->aux.dspDebugEvent.DebugCode = EvtBuffer[0];        
       pEventData->aux.dspDebugEvent.DebugLengthI16 = EvtBuffer[1];
       EvtDataBytes = pEventData->aux.dspDebugEvent.DebugLengthI16 * sizeof(ADT_UInt16); // Huafeng, look at later
       memcpy(pEventData->aux.dspDebugEvent.DebugData, &EvtBuffer[2], EvtDataBytes);
       break;

    default:
       PurgeCircStruct (DspId, EvtAddr, &cBuff);
	   EvtError = 1;
       break;
    };

    /* Unlock access to the DSP. */
    gpakUnlockAccess (DspId);

    if (EvtError) 
        return (RefInvalidEvent);

    *pChannelId = ChannelId;
    *pEventCode = EvtCode;
    return (RefEventAvail);

}


//}===================================================================================
//{                  Packet processing routines
//===================================================================================

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakSendPayloadToDsp - Send a Payload to a DSP's Channel.
 *
 * FUNCTION
 *  Writes a Payload message into DSP memory for a specified channel.
 *
 * Inputs
 *   DspId      - Dsp identifier 
 * ChannelId    - Channel identifier
 * PayloadClass - Audio, silence, tone, ...
 * PayloadType  - G711, G729, ...
 * pData        - Pointer to payload data.  MUST be aligned on DSP word boundary
 * PayloadI8    - Byte count of payload data.
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
gpakSendPayloadStatus_t gpakSendPayloadToDsp (ADT_UInt32 DspId, ADT_UInt16 ChannelId,
    GpakPayloadClass PayloadClass, ADT_Int16 PayloadType, ADT_UInt8 *pData,
    ADT_UInt16 PayloadI8) {


    PktHdr_t Hdr;

    CircBufr_t cBuff;
 
    DSP_Address CircBufrAddr;      /* address of Pkt In buffer info */
    DSP_Word  I16Free, I16Ready, HdrI16Sz, Pyld16Sz;


    if (DspId < 0 || MaxDspCores <= DspId)
        return SpsInvalidDsp;

    if (pDspIfBlk[DspId] == 0)
       DSPSync (DspId);

    if (MaxChannels[DspId] < ChannelId) {
        return SpsInvalidChannel;
    }

	// Read circular buffer structure into host memory
    gpakLockAccess (DspId);
    CircBufrAddr  = *getPktOutBufrAddr (DspId, ChannelId);
    ReadCircStruct (DspId, CircBufrAddr, &cBuff, &I16Free, &I16Ready);

	// Verify sufficient room with padding for non-word transfers
    HdrI16Sz  =  BytesToCircUnits (byteSize (PktHdr_t)); // Bytes to circular buffer units
    Pyld16Sz =  BytesToCircUnits (PayloadI8);       // Bytes to circular buffer units
    if (I16Free < HdrI16Sz + Pyld16Sz)  {
        gpakUnlockAccess (DspId);
        return SpsBufferFull;
    }

    
    Hdr.ChannelId       = ChannelId;
    Hdr.PayloadClass    = (ADT_UInt16) PayloadClass;
    Hdr.PayloadType     = PayloadType;
    Hdr.OctetsInPayload = PayloadI8;
    Hdr.TimeStamp       = 0;


    // Write header and payload
#if (DSP_WORD_SIZE_I8 == 4)
    if (HostBigEndian)
        gpakI16Swap_be ((ADT_UInt32 *) &Hdr, 8);
    else
        gpakI16Swap_le ((ADT_UInt32 *) &Hdr, 8);
#endif
    WriteCircBuffer (DspId, &cBuff, (DSP_Word *) &Hdr,  HdrI16Sz,  0);
    WriteCircBufferNoSwap (DspId, &cBuff, (DSP_Word *) pData, Pyld16Sz, CircBufrAddr);

    gpakUnlockAccess (DspId);

    return SpsSuccess;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakGetPayloadFromDsp - Read a Payload from a DSP's Channel.
 *
 * FUNCTION
 *  Reads a Payload message from DSP memory for a specified channel.
 *
 *  Note: If the buffer is too short for the data, a failure indication is returned
 *        and the payload remains unread.
 * 
 * Inputs
 *     DspId      - Dsp identifier 
 *   ChannelId    - Channel identifier
 * 
 * Outputs
 *   PayloadClass - Audio, silence, tone, ...
 *   PayloadType  - G711, G729, ...
 *   pPayloadData - Pointer to payload data.  MUST be aligned on 32-bit boundary
 *
 * Updates
 *     PayloadSize  - Before. Byte count of payload buffer size
 *                    After,  Byte count of actual payload
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */

gpakGetPayloadStatus_t gpakGetPayloadFromDsp (ADT_UInt32 DspId, ADT_UInt16 ChannelId,
                GpakPayloadClass *pPayloadClass, ADT_Int16  *pPayloadType,  
                ADT_UInt8 *pPayloadData,         ADT_UInt16 *pPayloadBytes,
				ADT_UInt32 *pTimeStamp) {

    Packet_t Pkt;
    CircBufr_t cBuff;

    DSP_Address CircBufrAddr;
    DSP_Word    I16Ready, I16Free, HdrI16Sz, Pyld16Sz;

    if (DspId < 0 || MaxDspCores <= DspId)
        return GpsInvalidDsp;

    if (pDspIfBlk[DspId] == 0)
       DSPSync (DspId);

    if (MaxChannels[DspId] < ChannelId) {
        return GpsInvalidChannel;
    }

	// Read circular buffer structure into host memory
    gpakLockAccess (DspId);
    CircBufrAddr  = *getPktInBufrAddr (DspId, ChannelId);
    ReadCircStruct (DspId, CircBufrAddr, &cBuff, &I16Free, &I16Ready);

    HdrI16Sz  = BytesToCircUnits (byteSize (PktHdr_t)); // Bytes to transfer units
    if (I16Ready < HdrI16Sz)  {
		if (I16Ready) PurgeCircStruct (DspId, CircBufrAddr, &cBuff);
        gpakUnlockAccess (DspId);
        return GpsNoPayload;
    }

    // Read payload header from DSP
    ReadCircBuffer (DspId, &cBuff, (DSP_Word *) &Pkt.Hdr, HdrI16Sz, 0);
#if (DSP_WORD_SIZE_I8 == 4)
    if (HostBigEndian)
        gpakI16Swap_be ((ADT_UInt32 *) &Pkt.Hdr, 8);
    else
        gpakI16Swap_le ((ADT_UInt32 *) &Pkt.Hdr, 8);
#endif
    Pyld16Sz = BytesToCircUnits (Pkt.Hdr.OctetsInPayload);       // Bytes to transfer units
    if (I16Ready < HdrI16Sz + Pyld16Sz) {
		PurgeCircStruct (DspId, CircBufrAddr, &cBuff);
        gpakUnlockAccess (DspId);
        return GpsNoPayload;
    }

    if (*pPayloadBytes < Pkt.Hdr.OctetsInPayload) {
		PurgeCircStruct (DspId, CircBufrAddr, &cBuff);
        gpakUnlockAccess (DspId);
        return GpsBufferTooSmall;
    }

    // Read payload data from DSP
    if (Pyld16Sz != 0)
       ReadCircBufferNoSwap (DspId, &cBuff, (DSP_Word *) pPayloadData, Pyld16Sz, CircBufrAddr);

    *pPayloadClass = (GpakPayloadClass) Pkt.Hdr.PayloadClass;
    *pPayloadType  = Pkt.Hdr.PayloadType;
    *pPayloadBytes = Pkt.Hdr.OctetsInPayload;
    *pTimeStamp    = Pkt.Hdr.TimeStamp;

    gpakUnlockAccess (DspId);
    return GpsSuccess;
}


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
typedef struct RTPHdr_t {
   ADT_UInt16 PktI8;
   ADT_UInt16 ChannelId;
} RTPHdr_t;

gpakSendPayloadStatus_t gpakSendPacketToDsp (ADT_UInt32 DspId, ADT_UInt16 ChannelId,
                                             ADT_UInt8 *pData,  ADT_UInt16 PacketI8) {


    RTPHdr_t Hdr;

    CircBufr_t cBuff;
 
    DSP_Address CircBufrAddr;      /* address of Pkt In buffer info */
    DSP_Word  I16Free, I16Ready, HdrI16Sz, PktI16Sz;


    if (DspId < 0 || MaxDspCores <= DspId)
        return SpsInvalidDsp;

    if (pDspIfBlk[DspId] == 0)
       DSPSync (DspId);

    if (MaxChannels[DspId] < ChannelId) {
        return SpsInvalidChannel;
    }

	// Read circular buffer structure into host memory
    CircBufrAddr  = *getPktOutBufrAddr (DspId, 0);
    gpakLockAccess (DspId);
    ReadCircStruct (DspId, CircBufrAddr, &cBuff, &I16Free, &I16Ready);

	// Verify sufficient room with padding for non-word transfers
    HdrI16Sz =  BytesToCircUnits (byteSize (RTPHdr_t)); // Bytes to circular buffer units
    PktI16Sz =  BytesToCircUnits (PacketI8);       // Bytes to circular buffer units
    if (I16Free < HdrI16Sz + PktI16Sz)  {
        gpakUnlockAccess (DspId);
        return SpsBufferFull;
    }
    
    Hdr.ChannelId  = ChannelId;
    Hdr.PktI8      = PacketI8;

    // Write header and payload
#if (DSP_WORD_SIZE_I8 == 4)
    if (HostBigEndian)
        gpakI16Swap_be ((ADT_UInt32 *) &Hdr, 4);
    else
        gpakI16Swap_le ((ADT_UInt32 *) &Hdr, 4);
#endif
    WriteCircBuffer (DspId, &cBuff, (DSP_Word *) &Hdr,  HdrI16Sz,  0);
    WriteCircBufferNoSwap (DspId, &cBuff, (DSP_Word *) pData, PktI16Sz, CircBufrAddr);

    gpakUnlockAccess (DspId);

    return SpsSuccess;
}

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

gpakGetPayloadStatus_t gpakGetPacketFromDsp (ADT_UInt32 DspId, ADT_UInt16 *pChannelId,
                                             ADT_UInt8 *pData,  ADT_UInt16 *pPacketI8) {

    RTPHdr_t Hdr;
    CircBufr_t cBuff;

    DSP_Address CircBufrAddr;
    DSP_Word    I16Ready, I16Free, HdrI16Sz, PktI16Sz;

    if (DspId < 0 || MaxDspCores <= DspId)
        return GpsInvalidDsp;


    if (pDspIfBlk[DspId] == 0)
       DSPSync (DspId);


    // Read circular buffer structure into host memory
    CircBufrAddr  = *getPktInBufrAddr (DspId, 0);
    gpakLockAccess (DspId);
    ReadCircStruct (DspId, CircBufrAddr, &cBuff, &I16Free, &I16Ready);

    HdrI16Sz  = BytesToCircUnits (byteSize(RTPHdr_t)); // Bytes to transfer units
    if (I16Ready < HdrI16Sz)  {
      if (I16Ready) PurgeCircStruct (DspId, CircBufrAddr, &cBuff);
        gpakUnlockAccess (DspId);
        return GpsNoPayload;
    }

    // Read RTP header from DSP
    ReadCircBuffer (DspId, &cBuff, (DSP_Word *) &Hdr, HdrI16Sz, 0);
#if (DSP_WORD_SIZE_I8 == 4)
    if (HostBigEndian)
        gpakI16Swap_be ((ADT_UInt32 *) &Hdr, 4);
    else
        gpakI16Swap_le ((ADT_UInt32 *) &Hdr, 4);
#endif

    PktI16Sz = BytesToCircUnits (Hdr.PktI8);       // Bytes to transfer units
    if (I16Ready < HdrI16Sz + PktI16Sz) {
        PurgeCircStruct (DspId, CircBufrAddr, &cBuff);
        gpakUnlockAccess (DspId);
        return GpsNoPayload;
    }

    if (*pPacketI8 < Hdr.PktI8) {
       PurgeCircStruct (DspId, CircBufrAddr, &cBuff);
       gpakUnlockAccess (DspId);
       return GpsBufferTooSmall;
    }

    // Read payload data from DSP
    if (PktI16Sz != 0)
       ReadCircBufferNoSwap (DspId, &cBuff, (DSP_Word *) pData, PktI16Sz, CircBufrAddr);

    *pChannelId = Hdr.ChannelId;
    *pPacketI8  = Hdr.PktI8;

    gpakUnlockAccess (DspId);
    return GpsSuccess;
}


//}===================================================================================
//{                 DSP download routines
//===================================================================================

/*
 * gpakDownloadDsp - Download a DSP's Program and initialized Data memory.
 *
 * FUNCTION
 *  This function reads a DSP's Program and Data memory image from the
 *  specified file and writes the image to the DSP's memory.
 *
 * Inputs
 *    DspId  - Dsp to receive download
 *   FileId  - File Handle
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */

gpakDownloadStatus_t gpakDownloadDsp (ADT_UInt32 DspId, void *FileId) {

    gpakDownloadStatus_t RetStatus;     /* function return status */
    DSP_Address Address;                /* DSP address */
	DSP_Word BlockSize;
    DSP_Word word;
    ADT_UInt8 *DlByteBufr = (ADT_UInt8 *)DlByteBufr_;

	ADT_UInt32 WordCount, NumRead, totalBytes;
    unsigned int NumWords;              /* number of words to read/write */
    unsigned int stat;

    if (MaxDspCores <= DspId)
        return GdlInvalidDsp;

    gpakLockAccess(DspId);

	// Initialize DSP's status variables
    totalBytes = 0;
    DSPError[DspId]     = 0;
    MaxChannels[DspId]  = 0;
    MaxCmdMsgLen[DspId] = 0;
    pEventFifoAddress[DspId] = 0;
    pDspIfBlk[DspId]    = 0;

    memset (getPktInBufrAddr (DspId, 0), 0, MaxChannelAlloc * sizeof (DSP_Address));
    memset (getPktOutBufrAddr(DspId, 0), 0, MaxChannelAlloc * sizeof (DSP_Address));

	// Disable DSP access
    word = 0;
    gpakWriteDsp (DspId, ifBlkAddress, 1, &word);

    RetStatus = GdlSuccess;
    while (RetStatus == GdlSuccess)  {

        /* Read a record header from the file. */
        NumRead = gpakReadFile (FileId, DlByteBufr, DL_HDR_SIZE);
        totalBytes += NumRead;
        if (NumRead == -1)  {
            RetStatus = GdlFileReadError;
            break;
        }
        if (NumRead != DL_HDR_SIZE)  {
            RetStatus = GdlInvalidFile;
            break;
        }

        /* Check for the End Of File record. */
        if (DlByteBufr[0] == (ADT_UInt8) 0xFF)
            break;

#if (DSP_DLADDR_SIZE_I8 == 4)
        /* Verify the record is for a valid memory type. */
        if (DlByteBufr[0] != 0x02)  {
            RetStatus = GdlInvalidFile;
            break;
        }

        Address = (DSP_Address) (((ADT_UInt32)DlByteBufr[1] << 24) & 0xff000000) |
                               (((ADT_UInt32)DlByteBufr[2] << 16) & 0x00ff0000) |       
                               (((ADT_UInt32)DlByteBufr[3] << 8)  & 0x0000ff00) |
                               ( (ADT_UInt32)DlByteBufr[4]        & 0x000000ff);

        WordCount =  (ADT_UInt16) ((DlByteBufr[5] << 8) & 0xff00) |
                                  ( DlByteBufr[6]       & 0x00ff);

#elif (DSP_DLADDR_SIZE_I8 == 3)
        /* Verify the record is for a valid memory type. */
        if ((DlByteBufr[0] != 0x00) && (DlByteBufr[0] != 0x01))  {
            RetStatus = GdlInvalidFile;
            break;
        }

        Address = (DSP_Address)(((ADT_UInt32)DlByteBufr[1] << 16) & 0x00ff0000) |       
                               (((ADT_UInt32)DlByteBufr[2] << 8)  & 0x0000ff00) |
                               ( (ADT_UInt32)DlByteBufr[3]        & 0x000000ff);

        WordCount =  (ADT_UInt16) ((DlByteBufr[4] << 8) & 0xff00) |
                                  ( DlByteBufr[5]       & 0x00ff);

#else                                  
 #error DSP_TYPE must be defined

#endif



        /* Read a block of words at a time from the file and write to the DSP's memory .*/
        while (WordCount != 0)      {
            if (WordCount < DownloadI8/sizeof (DSP_Word))
                NumWords = WordCount;
            else
                NumWords = DownloadI8/sizeof (DSP_Word);

            WordCount -= NumWords;
			   BlockSize  = NumWords * sizeof (DSP_Word);
            NumRead = gpakReadFile (FileId, DlByteBufr, BlockSize);
            totalBytes += NumRead;
            if (NumRead == -1)  {
                RetStatus = GdlFileReadError;
                break;
            }
            if (NumRead != BlockSize) {
                RetStatus = GdlInvalidFile;
                break;
            }

            if (HostBigEndian)
                transferDlBlock_be (DspId, Address, NumWords);
            else
                transferDlBlock_le (DspId, Address, NumWords);

            stat = memcmp (DlWordBufr, DlByteBufr, BlockSize);
            if (stat != 0) {
               RetStatus = GdlFileReadError;
               printk (KERN_ERR "Address failure: %x\n", (unsigned int)Address);
            }

            Address += BytesToMAUs (BlockSize);
        }
    }

    /* Unlock access to the DSP. */
    gpakUnlockAccess (DspId);

    /* Return with an indication of success or failure. */
    return RetStatus;
}



/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakGetEcanState - Read an Echo Canceller's training state.
 *
 * FUNCTION
 *  This function reads an Echo Canceller's training state information.
 * INPUTS
 *   DspId -          DSP identifier
 *   ChannelId  - 	  channel identifier
 *  *pEcanState -     pointer to Ecan state variable
 * 
 * OUTPUTS
 *  *pEcanState -     Ecan state variable data
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
void gpakParseEcanState (ADT_UInt16 *Msg, GpakEcanState_t *pEcanState) {
    
    pEcanState->Size            = Msg[2];
    pEcanState->TapLength       = Msg[3];
    pEcanState->CoefQ           = Msg[4];
    pEcanState->BGCoefQ         = Msg[5];
    pEcanState->NumFirSegments  = Msg[6];
    pEcanState->FirSegmentLen   = Msg[7];

    memcpy (pEcanState->Data, &Msg[8], EC_STATE_DATA_LEN * sizeof (ADT_UInt16));
}

gpakGetEcanStateStat_t gpakGetEcanState (ADT_UInt32  DspId, ADT_UInt16 ChannelId,
    GpakEcanState_t *pEcanState, GpakDeviceSide_t AorB) {

    ADT_UInt32 MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;
    ADT_UInt16 RplyLenI16;

    if (MaxDspCores <= DspId)
        return GesInvalidDsp;

    if (ChannelId >= MaxChannels[DspId])
        return GesInvalidChannel;

    Msg =(ADT_UInt16 *) &MsgBuffer[0];

    Msg[0] = MSG_READ_ECAN_STATE << 8;
    Msg[1] = (ADT_UInt16) (ChannelId << 8);
    Msg[2] = (ADT_UInt16) AorB;

    RplyLenI16  = ECAN_STATE_LEN_I16 + 2;
                                                                                           
    if (!TransactCmd (DspId, MsgBuffer, 3, MSG_READ_ECAN_STATE_REPLY, RplyLenI16, WORD_ID, (ADT_UInt16) ChannelId))
        return GesDspCommFailure;

    gpakParseEcanState (Msg, pEcanState);

    return GesSuccess;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakSetEcanState - Read an Echo Canceller's training state.
 *
 * FUNCTION
 *  This function writes an Echo Canceller's training state information.
 * INPUTS
 *   DspId -          DSP identifier
 *   ChannelId  - 	  channel identifier
 *  *pEcanState -     Ecan state variable data
 * 
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
ADT_UInt32 formatSetEcanState(ADT_UInt16 *Msg, ADT_UInt16 ChannelId, GpakEcanState_t *pEcanState, GpakDeviceSide_t AorB) {

    Msg[0] = MSG_WRITE_ECAN_STATE << 8;
    Msg[1] = (ADT_UInt16) (ChannelId << 8);
    Msg[2] = (ADT_UInt16)AorB;

    Msg[3] = pEcanState->Size;
    Msg[4] = pEcanState->TapLength;
    Msg[5] = pEcanState->CoefQ;
    Msg[6] = pEcanState->BGCoefQ;
    Msg[7] = pEcanState->NumFirSegments;
    Msg[8] = pEcanState->FirSegmentLen;

    memcpy (&Msg[9], pEcanState->Data, EC_STATE_DATA_LEN * sizeof (ADT_UInt16));

    return  (ECAN_STATE_LEN_I16 + 3);
}

gpakSetEcanStateStat_t gpakSetEcanState (ADT_UInt32  DspId, ADT_UInt16 ChannelId,
    GpakEcanState_t *pEcanState, GpakDeviceSide_t AorB) {

    ADT_UInt32 MsgLenI16;                     /* message length */
    ADT_UInt32 MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;

    if (MaxDspCores <= DspId)
        return SesInvalidDsp;

    if (ChannelId >= MaxChannels[DspId])
        return SesInvalidChannel;

    Msg =(ADT_UInt16 *) &MsgBuffer[0];

    MsgLenI16 = formatSetEcanState(Msg, ChannelId, pEcanState, AorB);

    if (!TransactCmd (DspId, MsgBuffer, MsgLenI16, MSG_WRITE_ECAN_STATE_REPLY, 2, WORD_ID, (ADT_UInt16) ChannelId))
        return SesDspCommFailure;

    return SesSuccess;
}
	    
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakAlgControl - Control an Algorithm.
 *
 * FUNCTION
 *  This function controls an Algorithm
 * 
 * INPUTS
 *   DspId,          - DSP identifier 
 *   ChannelId,	     - channel identifier 
 *   ControlCode,    - algorithm control code 
 *   DeactTonetype,  - tone detector to deactivate 
 *   ActTonetype,    - tone detector to activate 
 *   AorBSide,       - A or B device side 
 *   NLPsetting,     - NLP value 
 *   *pStatus        - pointer to return status 
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */

ADT_UInt32 formatAlgControl (ADT_UInt16 *Msg, ADT_UInt16 ChannelId,
    GpakAlgCtrl_t ControlCode, GpakToneTypes DeactTonetype,
    GpakToneTypes ActTonetype, GpakDeviceSide_t AorBSide, ADT_UInt16 NLPsetting,
    ADT_Int16 GaindB) {

    Msg[0] = MSG_ALG_CONTROL << 8;
    Msg[1] = (ADT_UInt16) (ChannelId << 8);
    Msg[2] = ((ControlCode & 0xff)      |
              ((NLPsetting & 0xf) << 8) |
              ((AorBSide & 0x1) << 12));
    Msg[3] = DeactTonetype;
    Msg[4] = ActTonetype;
    Msg[5] = GaindB;
    return 6;
}

gpakAlgControlStat_t gpakAlgControl (ADT_UInt32 DspId, ADT_UInt16 ChannelId,
    GpakAlgCtrl_t ControlCode, GpakToneTypes DeactTonetype,
    GpakToneTypes ActTonetype, GpakDeviceSide_t AorBSide, ADT_UInt16 NLPsetting,
    ADT_Int16 GaindB, GPAK_AlgControlStat_t *pStatus) {

    ADT_UInt32 MsgLenI16;                     /* message length */
    ADT_UInt32 MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;

    if (MaxDspCores <= DspId)
        return AcInvalidDsp;

    if (ChannelId >= MaxChannels[DspId])
        return AcInvalidChannel;

    Msg =(ADT_UInt16 *) &MsgBuffer[0];

    MsgLenI16 = formatAlgControl(Msg, ChannelId, ControlCode, DeactTonetype,
                     ActTonetype, AorBSide, NLPsetting, GaindB);  

    if (!TransactCmd (DspId, MsgBuffer, MsgLenI16, MSG_ALG_CONTROL_REPLY, 2, BYTE_ID, (ADT_UInt16) ChannelId))
        return AcDspCommFailure;

    /* Return success or failure  */
    *pStatus = (GPAK_AlgControlStat_t) Byte0 (Msg[1]);
    if (*pStatus == Ac_Success)  return AcSuccess;
    else                         return AcParmError;
}



/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakSendCIDPayloadToDsp - Send a transmit payload to a DSP channel
 *
 * FUNCTION
 *  This function sends a CID transmit payload to the DSP.
 *
 * INPUTS
 *    DspId             - DSP identifier
 *    ChannelId         - channel identifier
 *    *pPayloadData     - pointer to Payload data
 *    PayloadSize 	    - length of Payload data (octets)
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
gpakSendCIDPayloadToDspStat_t gpakSendCIDPayloadToDsp (ADT_UInt32  DspId,
       ADT_UInt16 ChannelId, ADT_UInt8 *pPayloadData, ADT_UInt16 PayloadSize,
       GpakDeviceSide_t AorBSide, GPAK_TxCIDPayloadStat_t *pStatus) {

    ADT_UInt32 MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt32 MsgLenI16;                     /* message length */
    ADT_UInt16 PaylenI16;
    ADT_UInt16 *Msg;

    /* Make sure the DSP Id is valid. */
    if (DspId >= MaxDspCores)
        return (ScpInvalidDsp);

    /* Make sure the Channel Id is valid. */
    if (ChannelId >= MaxChannels[DspId])
        return (ScpInvalidChannel);

    Msg =(ADT_UInt16 *) &MsgBuffer[0];
    Msg[0] = MSG_WRITE_TXCID_DATA << 8;
    Msg[1] = (ADT_UInt16) (ChannelId << 8);
    Msg[2] = PayloadSize;
    Msg[3] = AorBSide;

    PaylenI16 = (PayloadSize+1)/2;
    MsgLenI16 = 4 + PaylenI16; 

    if (BytesToTxUnits(PayloadSize+8) > MSG_BUFFER_SIZE)
        return ScpInvalidPaylen;

    if (HostBigEndian)
        gpakPackCidPayload_be (&Msg[4], pPayloadData, PaylenI16);
    else
        gpakPackCidPayload_le (&Msg[4], pPayloadData, PaylenI16);

    if (!TransactCmd (DspId, MsgBuffer, MsgLenI16, MSG_WRITE_TXCID_DATA_REPLY, 2, BYTE_ID, (ADT_UInt16) ChannelId))
        return ScpDspCommFailure;

    /* Return success or failure  */
    *pStatus = (GPAK_TxCIDPayloadStat_t) Byte0 (Msg[1]);
    if (*pStatus == Scp_Success)  return ScpSuccess;
    else                         return ScpParmError;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakReadAgcInputPower - Read short term power
 * 
 * FUNCTION
 *  This function reads a channel's short term power computed by the AGC
 * 
 * INPUTS
 *   DspId,      - DSP identifier 
 *   ChannelId,	 - channel identifier 
 *   *pPowerA,   - pointer to store A-side Input Power (dBm) 
 *   *pPowerB,   - pointer to store B-side Input Power (dBm) 
 *   *pStatus    - pointer to DSP status reply  
 * 
 * 
 * RETURNS
 *  Status  code indicating success or a specific error.
 */
gpakAGCReadInputPowerStat_t gpakReadAgcInputPower ( ADT_UInt32 DspId,
        ADT_UInt16 ChannelId, ADT_Int16 *pPowerA, ADT_Int16 *pPowerB,
        GPAK_AGCReadStat_t *pStatus) {

    ADT_UInt32 MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;

    if (MaxDspCores <= DspId)
        return AprInvalidDsp;

    if (ChannelId >= MaxChannels[DspId])
        return AprInvalidChannel;

    Msg = (ADT_UInt16 *) &MsgBuffer[0];
    Msg[0] = MSG_READ_AGC_POWER << 8;
    Msg[1] = (ADT_UInt16) (ChannelId << 8);

    if (!TransactCmd (DspId, MsgBuffer, 2, MSG_READ_AGC_POWER_REPLY, 4, BYTE_ID, (ADT_UInt16) ChannelId))
        return AprDspCommFailure;
    
    *pPowerA = Msg[2];
    *pPowerB = Msg[3];

    *pStatus = (GPAK_AGCReadStat_t) Byte0 (Msg[1]);
    if (*pStatus == Ar_Success)  return AprSuccess;
    else                         return AprParmError;

}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakResetChannelStats - Reset a channel's statistics
 * 
 * FUNCTION
 *  This function reset's a channel's statistics specified in the SelectMask
 * 
 * INPUTS
 *   DspId          - DSP Identifier
 *   ChannelId      - channel identifier
 *   SelectMask     - reset bit Mask
 * 
 * RETURNS
 *  Status  code indicating success or a specific error.
 */

gpakResetChannelStat_t  gpakResetChannelStats (ADT_UInt32 DspId,
            ADT_UInt16 ChannelId, resetCsMask SelectMask) {

    ADT_UInt32 MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;

    if (MaxDspCores <= DspId)
        return RcsInvalidDsp;

    if (ChannelId >= MaxChannels[DspId])
        return RcsInvalidChannel;

    Msg =(ADT_UInt16 *) &MsgBuffer[0];

    Msg[0] = MSG_RESET_CHAN_STATS << 8;
    Msg[1] = (ADT_UInt16) (ChannelId << 8);
    Msg[2] = 0;
    if (SelectMask.PacketStats) Msg[2] = 1;

    if (!TransactCmd (DspId, MsgBuffer, 3, MSG_RESET_CHAN_STATS_REPLY, 2, WORD_ID, (ADT_UInt16) ChannelId))
        return RcsDspCommFailure;

    return RcsSuccess;

}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakResetSystemStats - Reset system statistics
 * 
 * FUNCTION
 *  This function reset's the system statistics specified in the SelectMask
 * 
 * INPUTS
 *   DspId          - DSP Identifier
 *   SelectMask     - reset bit Mask
 * 
 * RETURNS
 *  Status  code indicating success or a specific error.
 */

gpakResetSystemStat_t  gpakResetSystemStats (ADT_UInt32 DspId,
                          resetSsMask SelectMask) {

    ADT_UInt32 MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;

    if (MaxDspCores <= DspId)
        return RssInvalidDsp;

    Msg =(ADT_UInt16 *) &MsgBuffer[0];

    Msg[0] = MSG_RESET_SYS_STATS << 8;
    Msg[1] = 0;
    Msg[2] = 0;
    if (SelectMask.FramingStats)  Msg[2] = 1;
    if (SelectMask.CpuUsageStats) Msg[2] |= (1 << 1);

    if (!TransactCmd (DspId, MsgBuffer, 3, MSG_RESET_SYS_STATS_REPLY, 2, NO_ID_CHK, 0))
        return RssDspCommFailure;

    return RssSuccess;

}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakReadMcBspStats - Read McBsp statistics
 * 
 * FUNCTION
 *  This function reads McBsp Statistics
 * INPUTS
 *      DspId,               DSP Identifier
 *		FrameStats           Pointer to store McBSP Statistics
 *
 * OUTPUTS 
 *		McBSP FrameStats   - McBSP Statistics from DSP
 *
 * RETURNS
 *  Status  code indicating success or a specific error.
 */
void gpakParseMcBspStats (ADT_UInt16 *Msg, gpakMcBspStats_t *FrameStats) {

    FrameStats->Port1Status             = Msg[2];
    FrameStats->Port1RxIntCount         = PackWords(Msg[3],Msg[4]);
    FrameStats->Port1RxSlips            = Msg[5];
    FrameStats->Port1RxDmaErrors        = Msg[6];
    FrameStats->Port1TxIntCount         = PackWords(Msg[7], Msg[8]);
    FrameStats->Port1TxSlips            = Msg[9];
    FrameStats->Port1TxDmaErrors        = Msg[10];
    FrameStats->Port1FrameSyncErrors    = Msg[11];
    FrameStats->Port1RestartCount       = Msg[12];
    
    FrameStats->Port2Status             = Msg[13];
    FrameStats->Port2RxIntCount         = PackWords(Msg[14],Msg[15]);
    FrameStats->Port2RxSlips            = Msg[16];
    FrameStats->Port2RxDmaErrors        = Msg[17];
    FrameStats->Port2TxIntCount          = PackWords(Msg[18], Msg[19]);
    FrameStats->Port2TxSlips            = Msg[20];
    FrameStats->Port2TxDmaErrors        = Msg[21];
    FrameStats->Port2FrameSyncErrors    = Msg[22];
    FrameStats->Port2RestartCount       = Msg[23];
    
    FrameStats->Port3Status             = Msg[24];
    FrameStats->Port3RxIntCount         = PackWords(Msg[25],Msg[26]);
    FrameStats->Port3RxSlips            = Msg[27];
    FrameStats->Port3RxDmaErrors        = Msg[28];
    FrameStats->Port3TxIntCount          = PackWords(Msg[29], Msg[30]);
    FrameStats->Port3TxSlips            = Msg[31];
    FrameStats->Port3TxDmaErrors        = Msg[32];
    FrameStats->Port3FrameSyncErrors    = Msg[33];
    FrameStats->Port3RestartCount       = Msg[34];
}

gpakReadMcBspStats_t gpakReadMcBspStats ( ADT_UInt32 DspId,
		gpakMcBspStats_t *FrameStats) {

    ADT_UInt32 MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;

    if (MaxDspCores <= DspId)
        return RmsInvalidDsp;

    Msg =(ADT_UInt16 *) &MsgBuffer[0];

    Msg[0] = MSG_READ_MCBSP_STATS << 8;
    Msg[1] = 0;

    if (!TransactCmd (DspId, MsgBuffer, 2, MSG_READ_MCBSP_STATS_REPLY, 35, NO_ID_CHK, 0))
        return RmsDspCommFailure;

    gpakParseMcBspStats (Msg, FrameStats);

    return RmsSuccess;
}

//}==================================================================================
//{                  Control routines
//===================================================================================

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakTestMode - Configure/perform a DSP's Test mode.
 *
 * FUNCTION
 *  Configures or performs a DSP's Test mode.
 *
 * Inputs
 *   DspId        - Dsp identifier 
 *   ChannelId   - Channel identifier
 *   pData        - Pointer to payload data.  MUST be aligned on DSP word boundary
 *   PayloadI8  - Byte count of payload data. It should be even number
 *
 * RETURNS
 *  Status code indicating success or a specific error.
 *
 */
#define MIN_SEND_DATA_LEN (24+8)
static ADT_UInt32 gpakFormatSendDataToDspMsg (ADT_UInt16 *Msg,  ADT_UInt16  ChannelId, ADT_UInt8 *pData, ADT_UInt16 PayloadI8) {
    DSP_Word  Pyld16Sz, i;
    Pyld16Sz =  BytesToMAUs (PayloadI8);       // Bytes to MSG buffer units

    /* Build the Configure Serial Ports message. */
    Msg[0]  = MSG_SEND_DATATODSP  << 8;
    Msg[1] = PackBytes (ChannelId, PayloadI8);

    for (i=0; i<Pyld16Sz; i++) {
	   Msg[2+i] = PackBytes (pData[2*i+1], pData[2*i]);
    }
    return (Pyld16Sz+2);
}

GpakApiStatus_t gpakSendDataToDsp (ADT_UInt32 DspId, ADT_UInt16 ChannelId, ADT_UInt8 *pData, ADT_UInt16 PayloadI8)
{
    ADT_UInt32  MsgLenI16;                     /* message length */
    ADT_UInt32  MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg;

    if (MaxDspCores <= DspId)
        return GpakApiInvalidDsp;
	if (PayloadI8 < MIN_SEND_DATA_LEN)
	    return GpakApiInvalidPaylen;
    if (pDspIfBlk[DspId] == 0)
       DSPSync (DspId);

    Msg    = (ADT_UInt16 *) &MsgBuffer[0];

    MsgLenI16 = gpakFormatSendDataToDspMsg (Msg, ChannelId, pData, PayloadI8);

    if (!TransactCmd (DspId, MsgBuffer, MsgLenI16, MSG_SEND_DATATODSP_REPLY, 2, 
	                  BYTE_ID, (ADT_UInt16) ChannelId))
        return GpakApiCommFailure;

    /* Return success or failure  */
    return (GpakApiStatus_t) Byte1 (Msg[1]);
}

GpakApiStatus_t gpakGetDataFromDsp (ADT_UInt32 DspId, ADT_UInt16 ChannelId, ADT_UInt16 *pData, ADT_UInt16 *PayloadI16)
{
    ADT_UInt32  MsgBuffer[MSG_BUFFER_SIZE];    /* message buffer */
    ADT_UInt16 *Msg, i;

    if (MaxDspCores <= DspId)
        return GpakApiInvalidDsp;
    if (pDspIfBlk[DspId] == 0)
       DSPSync (DspId);

    Msg    = (ADT_UInt16 *) &MsgBuffer[0];
    /* Build the Configure Serial Ports message. */
    Msg[0] = MSG_GET_DATAFROMDSP  << 8;
    Msg[1] = ChannelId << 8;

    if (!TransactCmd (DspId, MsgBuffer, 2, MSG_GET_DATAFROMDSP_REPLY, MSG_BUFFER_ELEMENTS, 
	                  BYTE_ID, (ADT_UInt16) ChannelId))
        return GpakApiCommFailure;

    /* Return success or failure  */

	*PayloadI16 = Msg[2];
	for(i = 0; i < *PayloadI16; i++) {
	   pData[i] = Msg[i+3];
	}
    return (GpakApiStatus_t) Byte0 (Msg[1]);
}

#if (DSP_TYPE == 64 || DSP_TYPE == 6424 || DSP_TYPE == 6437 || DSP_TYPE == 6452 || DSP_TYPE == 3530)
   #define EndianSwap(word) ((word >> 24) & 0x000000ff) |   \
                            ((word >>  8) & 0x0000ff00) |   \
                            ((word <<  8) & 0x00ff0000) |   \
                            ((word << 24) & 0xff000000)
#elif (DSP_TYPE == 54) || (DSP_TYPE == 55)

   #define EndianSwap(word) ((word >>  8) & 0x00ff)  |      \
                            ((word <<  8) & 0xff00)
#else
   #error DSP_TYPE must be defined as 54, 55, 64, 6424, 6437, or 6452
#endif


static void transferDlBlock_be (ADT_UInt32 DspId, DSP_Address Address, unsigned int NumWords) {
    ADT_UInt8 *DlByteBufr = (ADT_UInt8 *)DlByteBufr_;

    // Big endian mode does the swap in the Write and Read routines
    gpakWriteDsp (DspId, Address, NumWords, (DSP_Word *) DlByteBufr);
    gpakReadDsp  (DspId, Address, NumWords, (DSP_Word *) DlWordBufr);
}
static void transferDlBlock_le (ADT_UInt32 DspId, DSP_Address Address, unsigned int NumWords) {

DSP_Word word,  *Buff;
ADT_UInt8 *DlByteBufr = (ADT_UInt8 *)DlByteBufr_;
unsigned int i;

    // Manually swaps the download data if little endian host
    for (i = 0, Buff = (DSP_Word *) &DlByteBufr[0]; i < NumWords; i++, Buff++) {
       word = *Buff;

	   // Convert ENDIANESS
       DlWordBufr[i]  = EndianSwap(word);
        
    }
    gpakWriteDsp (DspId, Address, NumWords, (DSP_Word *) DlWordBufr);
    gpakReadDsp  (DspId, Address, NumWords, (DSP_Word *) DlByteBufr);
}

static void gpakPackCidPayload_be (ADT_UInt16 *Msg, ADT_UInt8 *pPayloadData, ADT_UInt16 PaylenI16) {  
    ADT_UInt16 i;

    for (i=0; i<PaylenI16; i++)
        Msg[i] = PackBytes (pPayloadData[2*i+1], pPayloadData[2*i]);
}
static void gpakPackCidPayload_le (ADT_UInt16 *Msg, ADT_UInt8 *pPayloadData, ADT_UInt16 PaylenI16) {  
    ADT_UInt16 i;

    for (i=0; i<PaylenI16; i++)
        Msg[i] = PackBytes (pPayloadData[2*i], pPayloadData[2*i+1]);
}

static void gpakI8Swap_be (ADT_UInt16 *in, ADT_UInt16 *out, ADT_UInt16 len) {

    ADT_UInt16 temp, i;

    for (i=0; i<len; i++) {
        temp = (*in << 8) & 0xFF00;
        *out = ((*in >> 8) & 0xFF);
        *out |= temp;
        in++; 
        out++;
    }
}
static void gpakI8Swap_le (ADT_UInt16 *in, ADT_UInt16 *out, ADT_UInt16 len) {

    ADT_UInt16 i;

    for (i=0; i<len; i++) {
        *out++ = *in++; 
    }
}

// Swap adjacent elements of a 16-bit integer array
// so they will be ordered correctly for big (host) to little (DSP) endian transfers
static void gpakI16Swap_be (ADT_UInt32 *Buff, int BuffI8)  { 
   ADT_UInt16 temp;
   ADT_UInt16 *buffHi = (ADT_UInt16 *) Buff; 
   ADT_UInt16 *buffLo = ((ADT_UInt16 *) (Buff))+1; 
   int i;

   BuffI8 += 3;
   BuffI8 /= 4;
   for (i=0; i<BuffI8; i++) {
      temp = *buffHi;
      *buffHi = *buffLo;
      *buffLo = temp;
      buffHi += 2;
      buffLo += 2;
   }
}
static void gpakI16Swap_le (ADT_UInt32 *Buff, int BuffI8)  { }

