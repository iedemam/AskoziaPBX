/* -----------------------------------------------------------------
 *
 *	File: hmpioconvert.h
 *
 *	Description:
 *	
 *	Defines the attributes for the HMP IO conversion from pkh_span.h to hmpiointerface.h.
 *
 *	History:
 *		Date		Name			Reason
 *		03/02/04	Alain Gauthier	Creation
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

#ifndef HMPIOCONVERT_H
#define HMPIOCONVERT_H

#include "hmpconfig.h"
#include "hmpiointerface.h"
#include "pkh_phone.h"

#if defined ( __cplusplus )
  extern "C" {
#endif

#define PK_DISPATCH_BOARD_FPGA_SW_EVENT  1
#define PK_DISPATCH_BOARD_FPGA_HW_EVENT  2
#define PK_DISPATCH_BOARD_FPGA_MSG_EVENT 3

#define PK_DISPATCH_FRAMER_STATUS      5
#define PK_DISPATCH_FRAMER_INFO        6
#define PK_DISPATCH_FRAMER_SW_EVENT	   7	
#define PK_DISPATCH_FRAMER_HW_EVENT	   8	
#define PK_DISPATCH_FRAMER_MSG_EVENT   9	
#define PK_DISPATCH_FRAMER_ABCD_EVENT  10  

#define PK_DISPATCH_PROTOCOL_SW_EVENT  13
#define PK_DISPATCH_PROTOCOL_HW_EVENT  14
#define PK_DISPATCH_PROTOCOL_MSG_EVENT 15

typedef s8	TSwString[32];

typedef u8	TDBoardData[500];

typedef struct {
	u32     dataSize;
	TDBoardData data;
} TDBoardMsgEvent;

typedef struct {
	u32   eventId;
	TSwString string;
} TDBoardSwEvent;

typedef struct {
	u32	eventId;
	u32	description[3];
} TDBoardHwEvent;


typedef struct
{
	u32 id;
	u32 adapter;
	u32 subAdapter;

	union {
		TDBoardSwEvent	sw;
		TDBoardHwEvent	hw;
		TDBoardMsgEvent	msg;
	}event;

} TDigitalBoardMsg;


#define PK_DISPATCH_BOARD_LINE_MASK    1
#define PK_DISPATCH_BOARD_PLL_MASK     2
#define PK_DISPATCH_BOARD_DMA_EVENT    3
#define PK_DISPATCH_BOARD_CLOCK_EVENT  4
#define PK_DISPATCH_BOARD_PFT_EVENT    5
#define PK_DISPATCH_BOARD_BUTTON_EVENT 6

// UM/KM phone event ids
#define PHONE_EVENT_OFFHOOK    0x01
#define PHONE_EVENT_ONHOOK     0x02
#define PHONE_EVENT_HOOKFLASH  0x10
#define PHONE_EVENT_RING_ON    0x20
#define PHONE_EVENT_RING_OFF   0x21
#define PHONE_EVENT_RING_TIMEOUT 0x04
#define PHONE_EVENT_POWER_ALARM 0x08

typedef struct
{
	u32 id;
	u32 trunk;
  u32 mask;
} TAnalogBoardMsg;


/* We cannot send floats to kernel */
typedef struct trunk_audio {
  u32  trunk;
  u8   mute;
  u8   TXgain2; // reg 38 +/- 1
  u8   RXgain2; // reg 39
  u8   TXgain3; // reg 40 +/- .1
  u8   RXgain3; // reg 41
} trunk_audio_t;

#if defined ( __cplusplus )
	}
#endif

#endif // HMPIOCONVERT_H
