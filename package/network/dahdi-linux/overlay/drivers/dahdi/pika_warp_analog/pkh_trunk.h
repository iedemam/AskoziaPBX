#ifndef PKH_TRUNK_H
#define PKH_TRUNK_H

#include <asm/types.h>

 /* The PKH_TTrunkInfo structure contains information about a trunk
 * returned by PKH_TRUNK_GetInfo.
 */
typedef struct PKH_TTrunkInfo {
  s32  lineVoltage;    /*  TIP/RING voltage in V, in 1 V increments. */
  u32 loopCurrent;     /*  Line current in mA, in 1 mA increments. */
} PKH_TTrunkInfo;

/*
 * ------------------------------------------------------------------
 * CONSTANTS
 * ------------------------------------------------------------------
 */

/*
 * Values for PKH_TTrunkConfig.internationalControl.  For regions other than North America and
 * Europe, contact PIKA for the correct setting.
 */
#define PKH_TRUNK_INTERNATIONAL_CONTROL_NA  0x00c000A3 /* Trunk configuration for North American applications */
#define PKH_TRUNK_INTERNATIONAL_CONTROL_EU  0x00c202A3 /* Trunk configuration for European Union applications */

/*
 *  Values for PKH_TTrunkConfig.compandMode
 */
#define PKH_TRUNK_AUDIO_MULAW                        0 /* Trunk configuration for mu-Law encoding */
#define PKH_TRUNK_AUDIO_ALAW                         1 /* Trunk configuration for A-Law encoding */

/*
 *  Values for PKH_TTrunkConfig.samplingRate. Currently available: 8 kHz.
 */
#define PKH_TRUNK_AUDIO_8KHZ                         0 /* Trunk configuration for 8 kHz encoding */

/*
 *  Values for PKH_TTrunkConfig.voltageThreshold.
 */
#define PKH_TRUNK_THRESHOLD_NORMAL                   4 /* Standard threshold value for non-logging applications. */
#define PKH_TRUNK_THRESHOLD_24V_LOGGING             16 /* Standard threshold value for logging applications with a nominal -24 VDC onhook voltage. */
#define PKH_TRUNK_THRESHOLD_48V_LOGGING             33 /* Standard threshold value for logging applications with a nominal -48 VDC onhook voltage. */

/*
 * Valid values for the detectionMask for PKH_TTrunkConfig.detectionMask
 * OR the values together as required. The default is: 
 *     PKH_TRUNK_RING_DETECTION | PKH_TRUNK_THRESHOLD_DETECTION
 */

/* Events may be signaled by the telephone network by reversing the voltage.  
   Set the PKH_TRUNK_REVERSAL_DETECTION in the detection mask to receive 
   PKH_EVENT_TRUNK_REVERSAL events when voltage reversals are detected.
 */
#define PKH_TRUNK_REVERSAL_DETECTION   0x004
/* Loop Current Sense Overload. AoH generateS a PKH_EVENT_TRUNK_LCSO
   event when the loop current exceeds the maximum loop current allowed 
   and the PKH_TRUNK_LCSO bit is set in the detection mask.
 */
#define PKH_TRUNK_LCSO                 0x008
/* Drop Out Detect. AoH generates a PKH_EVENT_TRUNK_DOD event
   when the loop current on the line drops below the minimum operating point 
   of the line interface and the PKH_TRUNK_DOD bit is set in the detection mask.
 */
#define PKH_TRUNK_DOD                  0x010
/* Loss of Frame. PKH_EVENT_TRUNK_LOF events indicate a serious hardware 
   failure on the Analog Gateway board. If the board fails and the 
   PKH_TRUNK_LOF bit is set in the detection mask, PKH_EVENT_TRUNK_LOF 
   events are generated.
 */
#define PKH_TRUNK_LOF                  0x020
/* PKH_EVENT_TRUNK_RX_OVERLOAD events indicate receive CODEC 
   overloads due to large audio signals on the line.  Set the 
   PKH_TRUNK_RX_OVERLOAD bit in the detection mask to receive these events.
 */
#define PKH_TRUNK_RX_OVERLOAD          0x040
/* When ringing is detected and the PKH_TRUNK_RING_DETECTION bit is set in 
   the detection mask, PKH_EVENT_TRUNK_RING_OFF and PKH_EVENT_TRUNK_RING_ON 
   events are generated.
 */
#define PKH_TRUNK_RING_DETECTION       0x300
/* When  the PKH_TRUNK_THRESHOLD_DETECTION bit is set in the detection mask,
   PKH_EVENT_TRUNK_ABOVE_THRESHOLD and PKH_EVENT_TRUNK_BELOW_THRESHOLD events
   are generated whenever the line voltage goes above and below the threshold
   voltage for more than the thresholdDetectDebounceTime.
 */
#define PKH_TRUNK_THRESHOLD_DETECTION  0xC00
  
/* The following defines are deprecated from AoH 2.2 onwards. */

/* Trunk configuration for North American applications. */ 
#define PKH_TRUNK_NA 0x00c000A3
/* Trunk configuration for European applications. */ 
#define PKH_TRUNK_EU 0x00c202A3
/* Trunk configuration for mu-Law encoding */ 
#define PKH_AUDIO_MULAW     0
/* Trunk configuration for A-Law encoding */ 
#define PKH_AUDIO_ALAW      1
/* Trunk configuration for 8 KHz encoding */ 
#define PKH_AUDIO_8KHZ      0


/*
 * ------------------------------------------------------------------
 * Type Defintions
 * ------------------------------------------------------------------
 */

/*
 * The PKH_THookSwitch enumerated type lists the hookswitch actions
 * supported by the PKH_TRUNK_SetHookSwitch call.
 */
typedef enum PKH_TTHookSwitch {
  PKH_TRUNK_ONHOOK = 0,  /* Onhook hookstate */
  PKH_TRUNK_OFFHOOK,     /* Offhook hookstate */
  PKH_TRUNK_HOOKFLASH    /* Hookflash hookstate */
} PKH_THookSwitch;

#endif // PKH_TRUNK_H
