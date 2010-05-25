#ifndef __WARP_PARMS_H__
#define __WARP_PARMS_H__

#include "pkh_trunk.h"

/* Maximum # of channels on a WARP of a given kind (FXS|FXO) */
#define MAX_NUM_CHAN            9

/* Maximum # of modules on a WARP (for now) */
#define MAX_MODULE_COUNT        2

/* Possible locations of all WARP modules */
#define MODULE_INTERNAL         1
#define MODULE_OB_AUDIO         2
#define MODULE_PORT_A           4
#define MODULE_PORT_B           8

/* Currently supported modules for DAHDI are FXO and FXS only */
#define MODULE_TYPE_FXO         0x01
#define MODULE_TYPE_FXS         0x02
#define MODULE_TYPE_BRI         0x03
#define MODULE_TYPE_GSM         0x04
#define MODULE_TYPE_NONE        0x0F

/* Debounce timing parameters */
#define OFFHOOK_DEBOUNCE_MS	500
#define HOOKFLASH_DEBOUNCE_MS	500
#define ONHOOK_DEBOUNCE_MS	500
#define REVERSAL_DEBOUNCE_MS	600
#define DISCONNECT_DEBOUNCE_MS	500
#define OHT_TIMER_DELAY		6000

/* Other defines */
#define MODULE_TYPE_MASK        0x0F

/* Trunk event codes */
#define TRUNK_EVENT_BASE		0
#define TRUNK_EVENT_HOOKFLASH		(TRUNK_EVENT_BASE + (1 << 0))
#define TRUNK_EVENT_DIALED		(TRUNK_EVENT_BASE + (1 << 1))
#define TRUNK_EVENT_REVERSAL		(TRUNK_EVENT_BASE + (1 << 2))
#define TRUNK_EVENT_LCSO		(TRUNK_EVENT_BASE + (1 << 3))
#define TRUNK_EVENT_DROPOUT		(TRUNK_EVENT_BASE + (1 << 4))
#define TRUNK_EVENT_LOF			(TRUNK_EVENT_BASE + (1 << 5))
#define TRUNK_EVENT_RX_OVERLOAD		(TRUNK_EVENT_BASE + (1 << 6))
#define TRUNK_EVENT_RING_OFF		(TRUNK_EVENT_BASE + (1 << 8))
#define TRUNK_EVENT_RING_ON		(TRUNK_EVENT_BASE + (1 << 9))
#define TRUNK_EVENT_BELOW_THRESHOLD	(TRUNK_EVENT_BASE + (1 << 10))
#define TRUNK_EVENT_ABOVE_THRESHOLD	(TRUNK_EVENT_BASE + (1 << 11))

#endif /* __WARP_PARMS_H__ */
