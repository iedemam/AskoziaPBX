/* -----------------------------------------------------------------
 *
 *  File: hmpfpgaconfig.h
 *
 *  Description:
 *  
 *  Defines the HMP configuration for the HMP FPGA device.
 *  This file is shared between user mode and kernel mode.
 *
 * ==================================================================
 * IF YOU DO NOT AGREE WITH THE FOLLOWING STATEMENT, YOU MUST
 * PROMPTLY RETURN THE SOFTWARE AND ANY ACCOMPANYING DOCUMENTATION
 * ("PRODUCT") TO PIKA TECHNOLOGIES INC. ("PIKA").
 *
 * Pika Technologies Inc. (hereinafter "PIKA") owns all title and
 * ownership rights to Software.  "Software" means any data processing
 * programs (hereinafter "programs") consisting of a series of
 * instructions or statements in object code form, including any
 * systemized collection of data in the form of a data base, and any
 * related proprietary materials such as flow charts, logic diagrams,
 * manuals media and listings provided for use with the programs.
 * User has no right, express or implied, to transfer, sell, provide
 * access to or dispose of Software to any third party without PIKA's
 * prior written consent.  User will not copy, reverse assemble,
 * reverse engineer, decompile, modify, alter, translate or display
 * the Software other than as expressly permitted by PIKA in writing.
 *
 * -----------------------------------------------------------------
*/

#ifndef HMPFPGACONFIG_H
#define HMPFPGACONFIG_H

#define MAX_FPGA_MESSAGE_SIZE  64

// Software events sent from kernel mode
typedef enum {
  swFpgaHandleError=0,
  swFpgaLastEvent
} eFpgaSwEventId;

// Software events structure sent from kernel mode
typedef struct {
  eFpgaSwEventId  eventId;
  tString         string;
} tFpgaSwEvent;

typedef enum {
  pllSourceRef1 = 0,
  pllSourceRef2,
  pllSourceInternal
} eFpgaPllSource;

// Hardware events sent from kernel mode
typedef enum {
  hwFpgaPllRef2Out=0,
  hwFpgaPllRef1Out,
  hwFpgaPllHoldOver,
  hwFpgaPllFLock,
  hwFpgaFrWrongSize,
  hwFpgaFrPciUnd,
  hwFpgaLastEvent
} eFpgaHwEventId;

// Hardware events structure sent from kernel mode
typedef struct {
  eFpgaHwEventId eventId;
  u8          state;
  eFpgaPllSource source;
} tFpgaHwEvent;

// FPGA message Event structure
typedef struct {
  u16  messageSize;
  u8   message[MAX_FPGA_MESSAGE_SIZE];
} tFpgaMsgEvent;

// Event sent from Kernel Mode
typedef struct {
  eEventType  eventType;
  union {
    tFpgaSwEvent sw;
    tFpgaHwEvent hw;
    tFpgaMsgEvent msg;
  }event;
} tFpgaReadEvent;

#endif // HMPFPGACONFIG_H
