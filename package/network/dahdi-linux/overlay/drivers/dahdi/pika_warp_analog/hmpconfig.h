/* -----------------------------------------------------------------
 *
 *  File: hmpconfig.h
 *
 *  Description:
 *
 *  Defines the HMP configuration for the HMP Digital Card Device.
 *
 *  History:
 *    Date        Name              Reason
 *    03/02/04    Alain Gauthier    Creation
 *    2005-08-29  Q. XIE            Modified for MC7.0
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

#ifndef HMPCONFIG_H
#define HMPCONFIG_H

//=====================
// PIKA INCLUDED FILES
//=====================

#include "hspconfig.h"

//============
// DATA TYPES
//============

typedef u8 tString[32];

typedef enum {
  ObjectState_Null = 0,
  ObjectState_Created,
  ObjectState_Initialized,
  ObjectState_Running,
  lastObjectState
} ObjectState;

typedef enum {
  disable=0,
  enable=1
} eControlState;

typedef eControlState   tEventControl;

typedef enum {
  swEvent=0,
  hwEvent,
  msgEvent
} eEventType;

typedef enum {
  notDefined=0,
  hmpDigitalCard,
  hmpPciBridge,
  hmpSeep,
  hmpWarpBri,
  idtQuadFramer,
  idtFramer,
  idtFramerHdlc,
  idtFramerABCD,
  idtPll,
  xilinxFPGA,
  lastType
} eHmpDeviceType;

#ifdef __KERNEL__
typedef enum {sw_error=0,sw_info,sw_debug} SwEventLevel;

#define MAX_SWEVENT_MESSAGE_SIZE 128
#endif

#endif // HMPCONFIG_H
