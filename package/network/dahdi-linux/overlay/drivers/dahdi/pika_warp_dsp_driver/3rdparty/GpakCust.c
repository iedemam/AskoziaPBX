/*
 * Copyright (c) 2002, Adaptive Digital Technologies, Inc.
 *
 * File Name: GpakCust.c
 *
 * Description:
 *   This file contains host system dependent functions to support generic
 *   G.PAK API functions. The file is integrated into the host processor
 *   connected to G.PAK DSPs via a Host Port Interface.
 *
 *   Note: This file needs to be modified by the G.PAK system integrator.
 *
 * Version: 1.0
 *
 * Revision History:
 *   10/17/01 - Initial release.
 *
 */

#if defined(__KERNEL__)
#include <linux/kernel.h>
#endif

#include "GpakCust.h"
#include "GpakApi.h"
#include "warp-dsp.h"


unsigned int MaxDspCores  = MAX_DSP_CORES;
unsigned int MaxChannelAlloc  = MAX_CHANNELS;
unsigned int MaxWaitLoops = MAX_WAIT_LOOPS;
unsigned int DownloadI8   = DOWNLOAD_BLOCK_I8;

#if defined (HOST_BIG_ENDIAN)
unsigned int HostBigEndian   = 1;
#elif defined (HOST_LITTLE_ENDIAN)
unsigned int HostBigEndian   = 0;
#else
   #error HOST_ENDIAN_MODE must be defined as either BIG or LITTLE
#endif

/* DSP Related values */
DSP_Address  ifBlkAddress = DSP_IFBLK_ADDRESS;                // Interface block address

GPAK_DspCommStat_t DSPError[MAX_DSP_CORES];

DSP_Word    MaxChannels[MAX_DSP_CORES]  = { 0 };  /* max num channels */
DSP_Word    MaxCmdMsgLen[MAX_DSP_CORES] = { 0 };  /* max Cmd msg length (octets) */

DSP_Address pDspIfBlk[MAX_DSP_CORES] = { 0 };          /* DSP address of interface blocks */
DSP_Address pEventFifoAddress[MAX_DSP_CORES] = { 0 };  /* Event circular buffers */


DSP_Address pPktInBufr [MAX_DSP_CORES][MAX_CHANNELS];     /* Pkt In circular buffers */
DSP_Address pPktOutBufr[MAX_DSP_CORES][MAX_CHANNELS];     /* Pkt Out circular buffers */

/* Download buffers */
ADT_Word   DlByteBufr_[(DOWNLOAD_BLOCK_I8 + sizeof(int))/sizeof(int)];                /* Dowload byte buf */
DSP_Word   DlWordBufr [(DOWNLOAD_BLOCK_I8 + sizeof(DSP_Word))/sizeof(DSP_Word)];      /* Dowload word buffer */

DSP_Address *getPktInBufrAddr  (int DspID, int channel) {
   return &pPktInBufr[DspID][channel];
}
DSP_Address *getPktOutBufrAddr (int DspID, int channel) {
   return &pPktOutBufr[DspID][channel];
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakReadDspMemory8 - Read DSP memory.
 *
 * FUNCTION
 *  This function reads a contiguous block of bytes from DSP memory starting at
 *  the specified address.
 *
 * RETURNS
 *  nothing
 *
 */
void gpakReadDspMemory8(
    ADT_UInt32   DspId,         /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    DSP_Address DspAddress,    /* DSP's memory address of first word */
    ADT_UInt32  NumBytes,      /* number of contiguous bytes to read */
    ADT_UInt8  *pByteValues    /* pointer to array of byte values variable */
    )
{
	/* This function needs to be implemented by the G.PAK system integrator. */
	printk("%s() : Not implemented yet!", __FUNCTION__);
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakReadDspMemory16 - Read DSP memory.
 *
 * FUNCTION
 *  This function reads a contiguous block of 16-bit word from DSP memory starting at
 *  the specified address.
 *
 * RETURNS
 *  nothing
 *
 */
void gpakReadDspMemory16(
    ADT_UInt32   DspId,           /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    DSP_Address DspAddress,      /* DSP's memory address of first word */
    ADT_UInt32  NumShorts,       /* number of contiguous short to read */
    ADT_UInt16  *pShortValues    /* pointer to array of byte values variable */
    )
{
	/*  Byte swapping is required for big endian hosts */
	warp_dsp_read_memory_16(DspId, DspAddress, NumShorts, pShortValues);
}

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
void gpakReadDspNoSwap16(
    ADT_UInt32   DspId,           /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    DSP_Address DspAddress,      /* DSP's memory address of first word */
    ADT_UInt32  NumShorts,       /* number of contiguous 32-bit vals to read */
    ADT_UInt16 *pShortValues     /* pointer to array of 32-bit values variable */
    )
{
	/*  Byte swapping is required for big endian hosts */
	warp_dsp_read_memory_16_noswap(DspId, DspAddress, NumShorts, pShortValues);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakReadDspMemory32 - Read DSP memory.
 *
 * FUNCTION
 *  This function reads a contiguous block of 32-bit values from DSP memory 
 *  starting at the specified address. 
 *
 * RETURNS
 *  nothing
 *
 */
void gpakReadDspMemory32(
    ADT_UInt32   DspId,           /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    DSP_Address DspAddress,      /* DSP's memory address of first word */
    ADT_UInt32  NumLongs,        /* number of contiguous 32-bit vals to read */
    ADT_UInt32 *pLongValues      /* pointer to array of 32-bit values variable */
    )
{
	/*  Byte swapping is required for big endian hosts */
	warp_dsp_read_memory_32(DspId, DspAddress, NumLongs, pLongValues);
}

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
void gpakReadDspNoSwap32(
    ADT_UInt32   DspId,           /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    DSP_Address DspAddress,      /* DSP's memory address of first word */
    ADT_UInt32  NumLongs,        /* number of contiguous 32-bit vals to read */
    ADT_UInt32 *pLongValues      /* pointer to array of 32-bit values variable */
    )
{
	/*  Packets are stored in big-endian format and do not need to be swapped  */
	warp_dsp_read_memory_32_noswap(DspId, DspAddress, NumLongs, pLongValues);
}

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
void gpakWriteDspMemory8(
    ADT_UInt32   DspId,          /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    DSP_Address DspAddress,     /* DSP's memory address of first word */
    ADT_UInt32  NumBytes,       /* number of contiguous bytes to write */
    ADT_UInt8  *pByteValues     /* pointer to array of byte values to write */
    )
{
	/* This function needs to be implemented by the G.PAK system integrator */
	printk("%s() : Not implemented yet!", __FUNCTION__);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakWriteDspMemory16 - Write DSP memory.
 *
 * FUNCTION
 *  This function writes a contiguous block of words to DSP memory starting at
 *  the specified address.
 *
 * RETURNS
 *  nothing
 *
 */
void gpakWriteDspMemory16(
    ADT_UInt32 DspId,            /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    DSP_Address DspAddress,     /* DSP's memory address of first word */
    ADT_UInt32 NumShorts,       /* number of contiguous words to write */
    ADT_UInt16 *pShortValues    /* pointer to array of word values to write */
    )
{
	/*  Byte swapping is required for big endian hosts */
	warp_dsp_write_memory_16(DspId, DspAddress, NumShorts, pShortValues);
}

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
void gpakWriteDspNoSwap16(
    ADT_UInt32 DspId,            /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    DSP_Address DspAddress,     /* DSP's memory address of first word */
    ADT_UInt32 NumShorts,       /* number of contiguous words to write */
    ADT_UInt16 *pShortValues    /* pointer to array of word values to write */
    )
{
	/*  Packets are stored in big-endian format and do not need to be swapped  */
	warp_dsp_write_memory_16_noswap(DspId, DspAddress, NumShorts, pShortValues);
}
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
void gpakWriteDspMemory32(
    ADT_UInt32 DspId,            /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    DSP_Address DspAddress,     /* DSP's memory address of first word */
    ADT_UInt32 NumLongs,        /* number of contiguous 32-bit vals to write */
    ADT_UInt32 *pLongValues     /* pointer to array of 32-bit values to write */
    )
{
	/*  Byte swapping is required for big endian hosts */
	warp_dsp_write_memory_32(DspId, DspAddress, NumLongs, pLongValues);
}

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
void gpakWriteDspNoSwap32(
    ADT_UInt32 DspId,            /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    DSP_Address DspAddress,     /* DSP's memory address of first word */
    ADT_UInt32 NumLongs,        /* number of contiguous longs to write */
    ADT_UInt32 *pLongValues     /* pointer to array of long values to write */
    )
{
	/*  Packets are stored in big-endian format and do not need to be swapped  */
	warp_dsp_write_memory_32_noswap(DspId, DspAddress, NumLongs, pLongValues);
}

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
void gpakHostDelay(ADT_UInt32 delayType)
{
	warp_dsp_delay(delayType);
}


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
void gpakLockAccess(
    ADT_UInt32 DspId                   /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    )
{
	warp_dsp_lock(DspId);
}


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
void gpakUnlockAccess(
    ADT_UInt32 DspId                   /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    )
{
	warp_dsp_unlock(DspId);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakCmdLockAccess - Lock access to the specified DSP.
 *
 * FUNCTION
 *  This function aquires exclusive access to the specified DSP's Command Buffer
 *
 * RETURNS
 *  nothing
 *
 */
void gpakCmdLockAccess(
    ADT_UInt32 DspId                   /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    )
{
	warp_dsp_cmd_lock(DspId);
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * gpakCmdUnlockAccess - Unlock access to the specified DSP Command Buffer
 *
 * FUNCTION
 *  This function releases exclusive access to the specified DSP's Command Buffer
 *
 * RETURNS
 *  nothing
 *
 */
void gpakCmdUnlockAccess(
    ADT_UInt32 DspId                   /* DSP Identifier (0 to MAX_DSP_CORES-1) */
    )
{
	warp_dsp_cmd_unlock(DspId);
}

/* TODO : document this stuff --> get command write access */
void gpakCmdGetCmdWriteAccess(ADT_UInt32 DspId)
{
	warp_dsp_get_dsp_command_buffer_write_lock(DspId);
}

/* TODO : document this stuff --> get command write access */
void gpakCmdGetCmdReadAccess(ADT_UInt32 DspId)
{
	warp_dsp_get_dsp_command_buffer_read_lock(DspId);
}

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
int gpakReadFile(
    void*      FileId,        /* G.PAK Download File Identifier */
    ADT_UInt8 *pBuffer,	/* pointer to buffer for storing bytes */
    ADT_UInt32 NumBytes       /* number of bytes to read */
    )
{
    return warp_dsp_read_firmware((int)FileId, pBuffer, NumBytes);
}


