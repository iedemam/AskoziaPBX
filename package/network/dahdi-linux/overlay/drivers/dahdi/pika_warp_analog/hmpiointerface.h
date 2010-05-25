#ifndef HMPIOINTERFACE_H
#define HMPIOINTERFACE_H

#ifdef _WIN32
#else
#  ifdef __KERNEL__
#    include <linux/in.h>
#  else
#    include <netinet/in.h>
#  endif
#endif  // _WIN32

//=====================
// PIKA INCLUDED FILES
//=====================

#include "hmpconfig.h"
#include "hmpfpgaconfig.h"
#include "hspconfig.h"
#include "pkh_trunk.h"

#define PKH_BOARD_MAX_RING_PATTERN_SIZE      30

//===================
// MACRO DEFINITIONS
//===================

#define EEPROM_PRODUCTNUM_ARRAY_SIZE    14
#define EEPROM_RESERVED_ARRAY_SIZE      219

#define MAX_CARD_MESSAGE_SIZE           512
#define HSP_EXCHANGE_DATA_BUFFER_SIZE   (4096 - sizeof(THspExchangeHeader))

typedef enum {
  TRUNK_GO_ONHOOK = 1,
  TRUNK_GO_OFFHOOK,
  TRUNK_GO_HOOKFLASH,
  TRUNK_STOP_HOOKFLASH,
  TRUNK_STOP_DIALING
} hookswitch_t;

typedef struct {
  u32 trunk;
  hookswitch_t state;
} trunk_hookswitch_t;

/*******************************************************************
*******************************************************************/

typedef struct {
  u32 trunk;
  s8 string[32];
} trunk_dial_t;

/*******************************************************************
*******************************************************************/

typedef struct {
  u32 line;
  u32 data;
} line_uint_t;

/*******************************************************************
*******************************************************************/

typedef struct {
  u32 trunk;
  u32 internationalControl;
  u32 compandMode;
} trunk_start_t;

/*******************************************************************
*******************************************************************/

typedef struct {
  u32 phone;
  u32 audioFormat;
  u32 internationalControl;
} phone_start_t;

/*******************************************************************
*******************************************************************/

typedef struct {
  u32 index;
  u8   pattern[PKH_BOARD_MAX_RING_PATTERN_SIZE];
  s32  size;
} phone_ring_pattern_t;

/*******************************************************************
*******************************************************************/

typedef struct {
  u32 phone;
  u32 pat_index;
  u32 duration;
} phone_ring_t;

/*******************************************************************
*******************************************************************/

typedef struct {
  u32 phone;
  u32 enable;
} phone_onhook_tx_t;

/*******************************************************************
*******************************************************************/

typedef struct {
  u32 phone;
  u32 muteRX;
  u32 muteTX;
  s32  gainRX;
  s32  gainTX;
} phone_audio_t ;

/*******************************************************************
*******************************************************************/

typedef struct {
  u32 phone;
  u8 voltage;
  u8 lineCurrent;
} phone_get_info_out_t;

/*******************************************************************
*******************************************************************/
typedef struct {
  u32 phone;
  u32 loopback;
  u32 analog_gain; // bits [0:3] in ANALOG_GAIN ctrl register
} phone_set_diag_t;

/*******************************************************************
*******************************************************************/
typedef struct {
  u32 phone;
  u8 q[6];
  u8 battery[2];
  u8 loopback;
  u8 analog_gain; // bits [0:3] in ANALOG_GAIN ctrl register
} phone_get_diag_out_t;

/*******************************************************************
*******************************************************************/
/* gains *must* be signed! */
typedef struct {
  int line;
  int rx_gain;
  int tx_gain;
  int mute;
  int mode;
} audio_gain_t;
#endif
