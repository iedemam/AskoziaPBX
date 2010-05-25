#ifndef PKH_PHONE_H
#define PKH_PHONE_H

#include "pkh_trunk.h"

/*
 *  Phone configuration settings for different regions.
 */
#define PKH_PHONE_INTERNATIONAL_CONTROL_NA   0x08  /* Phone configuration for North American applications (600 ohm line impedance). */
#define PKH_PHONE_INTERNATIONAL_CONTROL_EU   0x0C  /* Phone configuration for European Union applications (TBR021 line impedance). */

/*
 *  Phone compand mode configurations
 */
#define PKH_PHONE_AUDIO_MULAW                   0  /* Phone configuration for mu-Law compandMode. */
#define PKH_PHONE_AUDIO_ALAW                    1  /* Phone configuration for a-Law compandMode. */

/*
 *  Phone sampling rate configurations
 */
#define PKH_PHONE_AUDIO_8KHZ                    0  /* Phone configuration for 8 kHz samplingRate. */

/*
 * Mask bits for ringgenEvents that indicate which PKH_EVENT_PHONE_RING_* events are to be sent up to the
 * application.
 */
#define PKH_PHONE_MASK_RING_ON                0x1  /* Mask bit to enable PKH_EVENT_PHONE_RING_ON events.  */
#define PKH_PHONE_MASK_RING_OFF               0x2  /* Mask bit to enable PKH_EVENT_PHONE_RING_OFF events. */

/* 
 * Minimum, maximum and default values for PKH_TPhoneAudio gain values, in dB
 */ 
#define PKH_PHONE_AUDIO_GAIN_MIN            -50.0  /* Minimum gain (maximum attenuation) in dB */
#define PKH_PHONE_AUDIO_GAIN_DEFAULT          0.0  /* Default gain in dB */
#define PKH_PHONE_AUDIO_GAIN_MAX              6.0  /* Maximum gain in dB */

#endif /* PKH_PHONE_H */
