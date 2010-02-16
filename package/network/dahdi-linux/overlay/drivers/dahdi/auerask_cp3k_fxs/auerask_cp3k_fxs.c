/*
 * Auerswald COMpact 3000 FXS Interface Driver for DAHDI Telephony interface
 *
 * Copyright (C) 2010, Auerswald GmbH & Co KG
 *
 * All rights reserved.
 *
 */

/*
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2 as published by the
 * Free Software Foundation. See the LICENSE file included with
 * this program for more details.
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/moduleparam.h>
#include <asm/io.h>
#include <dahdi/kernel.h>

#include "../auerask_cp3k_mb/auerask_cp3k_mb.h"

#define MOD_TYPE_FXS    0

/**
 *  timing definitions
*/
#define AUERFXS_INTF              1000          // interrupt frequency (Hz)
#define AUERFXS_PORTINT            100          // reprocess frequency of each FXS port (Hz)
#define AUERFXS_OVERSAM ( AUERFXS_INTF / AUERFXS_PORTINT ) // oversampling rate of loop closure monitoring
#define AUERFXS_RINGFREQ_MIN AUERFXS_RINGFREQ_25HZ // lowest supported ringing frequency
#define AUERFXS_RINGFREQ_25HZ       25          // ringing frequency = 25 Hz
#define AUERFXS_RINGFREQ_50HZ       50          // ringing frequency = 50 Hz
#define AUERFXS_PORT_MAXNUM          4          // max. nr. of ports per module (mainboard or plug in module)
#define AUERFXS_MIN_ONHOOK_TIME    500          // min ONHOOK time before return to FXS ONHOOK state (ms)
#define AUERFXS_TIME_DEBOUNCE       30          // debounce time until reporting ON-/OFFHOOK change to DAHDI
#define AUERFXS_MIN_OFFHOOK_TIME    40          // min OFFHOOK time before leaving FXS ONHOOK state (ms)
#define AUERFXS_TIMTIC ( 1000 / AUERFXS_PORTINT ) // reprocess period of each FXS port (ms)
#define DAHDI_OHT_TIMER           6000          // how long after RING to retain OHT (ms)

/**
 *  debounce parameters for loop closure detect
*/
#define AUERFXS_LC_LOW_PASS_INIT        AUERFXS_OVERSAM
#define AUERFXS_LC_LOW_PASS_MAX         ( 3 * AUERFXS_OVERSAM )
#define AUERFXS_LC_LOW_PASS_THRES       ( 2 * AUERFXS_OVERSAM )

/**
 * codes for loop closure changes
*/
#define AUERFXS_LC_0_TO_0            0          // loop remains open
#define AUERFXS_LC_0_TO_1            1          // loop open -> loop closed
#define AUERFXS_LC_1_TO_0            2          // loop closed -> loop open
#define AUERFXS_LC_1_TO_1            3          // loop remains closed

/**
 * average loop current detect: > 50 % denotes "loop closed"
*/
#define AUERFXS_LC50  ( AUERFXS_OVERSAM / 2 )

/**
 * codes for linefeed (and ringing voltage) generation
*/
#define AUERFXS_RINGING_OFF     0
#define AUERFXS_RINGING_ON      1
#define AUERFXS_LINEFEED_POS    2
#define AUERFXS_LINEFEED_NEG    3
#define AUERFXS_LINEFEED_ZERO   4

/**
 * register bit offset positive to negative linefeed voltage
*/
#define OFFSET_POS           0       // low nibble controls pos. linefeed voltage
#define OFFSET_NEG           4       // high nibble controls neg. linefeed voltage

/**
 *  FXS port states
*/
#define AUERFXS_PORTSTATE_INIT_ONHOOK  0   // init (short time)
#define AUERFXS_PORTSTATE_ONHOOK       1   //
#define AUERFXS_PORTSTATE_OFFHOOK      2   //

/**
 *  AD SSM2602 codec register definitions
*/
#define AD_SSM26xx_ADR_SHIFT    9
#define AD_SSM26xx_L_IV         ( 0x0000 << AD_SSM26xx_ADR_SHIFT )
#define AD_SSM26xx_L_DACV       ( 0x0002 << AD_SSM26xx_ADR_SHIFT )
#define AD_SSM26xx_AAP          ( 0x0004 << AD_SSM26xx_ADR_SHIFT )
#define AD_SSM26xx_DAP          ( 0x0005 << AD_SSM26xx_ADR_SHIFT )
#define AD_SSM26xx_PM           ( 0x0006 << AD_SSM26xx_ADR_SHIFT )
#define AD_SSM26xx_DA_IF        ( 0x0007 << AD_SSM26xx_ADR_SHIFT )
#define AD_SSM26xx_SR           ( 0x0008 << AD_SSM26xx_ADR_SHIFT )
#define AD_SSM26xx_ACTIVE       ( 0x0009 << AD_SSM26xx_ADR_SHIFT )
#define AD_SSM26xx_RESET        ( 0x000F << AD_SSM26xx_ADR_SHIFT )

#define AD_SSM26xx_IV_0DB       0x0017
#define AD_SSM26xx_IV_M3DB      0x0015      // - 3 dB
#define AD_SSM26xx_IV_M6DB      0x0013      // - 6 dB

/**
 * FXS port context
*/
struct auerfxs_port
{
    nr_slot_t slot;                 // nr. of (virtual) slot (maybe AUERMOD_CP3000_SLOT_MOD or AUERMOD_CP3000_SLOT_AB)
    nr_port_t pnum;                 // nr of port (0... )
    struct aucp3kfxs_entry *slotp;  // ptr to module context
    u8 mask;                        // bitmask for this port
    spinlock_t portlock;            //
    int hook_low_pass;              // low pass for OFFHOOK detection

    // ringing control
    u8 ring_on;                     // 1: ringing on, 0: ringing off
    u8 ring_allowed;                // ringing inhibit

    // port statemachine
    unsigned int state;             // main state
    unsigned int time_lcurr;        // elapsed time with loop closed
    unsigned int time_nolcurr;      // elapsed time with loop open
    u8 last_lcurr;                  // loop state (open/closed) of previous scan period
    int dahdi_hookstate;            // last hookstate sent to DAHDI
};

/**
 * module description
*/
typedef struct fxs_module_desc_t
{
    char *module_desc;              // module ID string
    unsigned int nr_ports;          // nr of ports
    unsigned int ports_pro_int;     // nr of ports to be handled per interrupt
    u16 port_mask;                  // bitmask
    char *location;                 // physical location
    // callback
    void ( *make_ringphase )( struct aucp3kfxs_entry *);            // generate ringing waveform
    void ( *init )( struct aucp3kfxs_entry * );                     // init
    void ( *set_linefeed )( struct auerfxs_port*, u8 val );         // switch pos/neg linefeed
    u16  ( *get_lcurr )( struct aucp3kfxs_entry * );                // get loop currents
} fxs_module_desc_t;

/**
 * module context
*/
struct aucp3kfxs_entry
{
    unsigned int moduleID;                  //
    fxs_module_desc_t * mod_desc;           // ptr to module description struct
    nr_slot_t slot;                         // nr. of (virtual) slot (AUERMOD_CP3000_SLOT_MOD or AUERMOD_CP3000_SLOT_AB)
    int usecount;
    int dead;
    int flags[ AUERFXS_PORT_MAXNUM ];
    int cardflag;                                   // Bit-map of present ports
    spinlock_t lock;
    struct dahdi_span span;
    struct dahdi_chan _chans[ AUERFXS_PORT_MAXNUM ];
    struct dahdi_chan *chans[ AUERFXS_PORT_MAXNUM ];
    unsigned int portindex;                         // port index for interrupt processing
    void __iomem * base_address;                    // for SPI access
    u8 ringfreq;                                    //
    unsigned int overs_index;                       // index for lcurr_samples access
    u8 lcurr_samples[ AUERFXS_OVERSAM ];            // loopcurrent oversampling ringbuffer
    unsigned int long_overs_index;                  // index for lcurr_long_samples access
    u8 lcurr_long_samples[ AUERFXS_INTF / AUERFXS_RINGFREQ_MIN ]; // loopcurr. oversamp. ringbuffer f. 1 ringing period
    volatile u8 sreg_linefeed;                      // shadow register for linefeed control
    spinlock_t sreg_lock;                           // lock for shadow reg
    struct auerfxs_port port[ AUERFXS_PORT_MAXNUM ];// port context structs
    struct fxs
    {
        int ohttimer;
        int idletxhookstate;                        // IDLE changing hook state
        int lasttxhook;
    } modfxs[ AUERFXS_PORT_MAXNUM ];
};

/**
*  global variables
*/
static int debug = 0;                                                   // module_param!
static struct aucp3kfxs_entry *aucp3kfxs_list[ CP3000_AUERMODANZ + 1 ]; // list of FXS modules
static int card_cnt;
static int alawoverride = 1;

/**
*  externals
*/
// base addresses for IO reads/writes
extern void __iomem * auergb_rioadr;
extern void __iomem * auergb_wioadr;

/**
 * local prototypes
*/
static void aucp3kfxs_cleanup( void );
static void auer2_4ab_resetmodule( struct aucp3kfxs_entry * sp );
static void aucp3kfxs_release( struct aucp3kfxs_entry * sp );
static void auer2_4ab_setlinefeed( struct auerfxs_port* port_p, u8 val );
static inline void treat_fxsstate_onhook( u8 lcurr, struct auerfxs_port* port );
static inline void treat_fxsstate_offhook( u8 lcurr, struct auerfxs_port* port );

///---------------------------------------------------------------------------------------------------------------------
/**
 * Write single codec register
 * @param       base_address    SPI base address
 * @param       data            write data (16 bits).
 *                              (data[15:9]: regaddr., data[8:0]: regvalue)
*/
static void write_ssm2602_reg( void __iomem * base_address, unsigned data )
{
    unsigned long flags;

    spin_lock_irqsave( &spi_lock, flags );
    spi_start( base_address );
    spi_write( base_address, data >> 8 );
    spi_write( base_address, data );
    // wait for end of CS, then stop
    ( void )spi_read_stop( base_address );
    spin_unlock_irqrestore( &spi_lock, flags );
    return;
} // write_ssm2602_reg()

///---------------------------------------------------------------------------------------------------------------------
/**
 * Calculate base address
 * @param       slot        slotnr (1... )
 * @param       offset      0 for FXS ports 1+2, 5+6; 1 for FXS ports 3+4
*/
static void __iomem * calc_baseaddress( unsigned slot, unsigned offset )
{
    unsigned base_address = SPI_CSNONE;

    switch( slot )
    {
    case AUERMOD_CP3000_SLOT_MOD:
        base_address = SPI_CSMOD;
        break;
    case AUERMOD_CP3000_SLOT_AB:
        base_address = SPI_CSTN12;
        break;
    default:
        break;
    }
    base_address |= SPI_12M;
    base_address |= SPI_CLKLH;
    base_address |= SPI_NORM;
    base_address += offset;
    return (void __iomem *) base_address;
} // calc_baseaddress()

///---------------------------------------------------------------------------------------------------------------------
/**
 * Get address of FXS channel transmit chunk
 * @param       slot        slotnr (1...)
 * @param       pnum        portnr (0...)
 *
*/
static u8 * aucp3kfxs_get_chan_tx_chunk( nr_slot_t slot, nr_port_t pnum )
{
    struct aucp3kfxs_entry *sp;

    if( slot && slot <= CP3000_AUERMODANZ )
    {
        sp = aucp3kfxs_list[ slot ];
        if( sp )
        {
            if( sp->moduleID == MODID2AB || sp->moduleID == MODID4AB )
            {
                if( sp->cardflag & 0x01 << pnum )
                {
                    return sp->chans[ pnum ]->swritechunk;
                }
            }
        }
    }
    return NULL;
} // aucp3kfxs_get_chan_tx_chunk()

///---------------------------------------------------------------------------------------------------------------------
/**
 * Get address of FXS channel receive chunk
 * @param       slot        slotnr (1...)
 * @param       pnum        portnr (0...)
 */
static u8 * aucp3kfxs_get_chan_rx_chunk( nr_slot_t slot, nr_port_t pnum )
{
    struct aucp3kfxs_entry *sp;

    if( slot && slot <= CP3000_AUERMODANZ )
    {
        sp = aucp3kfxs_list[ slot ];
        if( sp )
        {
            if( sp->moduleID == MODID2AB || sp->moduleID == MODID4AB )
            {
                if( sp->cardflag & 0x01 << pnum )
                {
                    return sp->chans[ pnum ]->sreadchunk;
                }
            }
        }
    }
    return NULL;
} // aucp3kfxs_get_chan_rx_chunk()

///---------------------------------------------------------------------------------------------------------------------
/**
 * Main fxs port routine, called with AUERFXS_PORTINT Hz for each fxs port. Loop currents are sampled with
 * AUERFXS_INTF Hz and stored in ringbuffers port->slotp->lcurr_samples[] and port->slotp->lcurr_long_samples[].
 *  @param      port        port context
*/
static inline void treat_fxsport( struct auerfxs_port* port )
{
    unsigned long flags;
    int lcurr_filtered = 0;
    u8 lcurr = 0;

    spin_lock_irqsave( &port->portlock, flags );

    // If port is ringing not all lcurr samples may be used
    if( port->ring_on && port->slotp->mod_desc->make_ringphase )
    {
        // Along every ringing voltage period only a single lcurr sample is used
        if( port->slotp->long_overs_index >= AUERFXS_OVERSAM )
        {
            spin_unlock_irqrestore( &port->portlock, flags );
            return;
        }
        if( port->slotp->lcurr_long_samples[ ( ( AUERFXS_INTF / ( 2 * port->slotp->ringfreq ) ) ) - 2 ] & port->mask )
        {
            lcurr_filtered = AUERFXS_OVERSAM + AUERFXS_OVERSAM / 2;
        }
    } else
    {
        // No ringing, the last AUERFXS_OVERSAM are evaluated
        unsigned int lcurr_index;

        for( lcurr_index = 0; lcurr_index < AUERFXS_OVERSAM; lcurr_index++ )
        {
            if( port->slotp->lcurr_samples[ lcurr_index ] & port->mask )
            {
                lcurr_filtered++;
            }
        }
        lcurr = lcurr_filtered > AUERFXS_LC50 ? 1 : 0;
    }
    // Treat ports depending on port state
    switch( port->state )
    {
    case AUERFXS_PORTSTATE_INIT_ONHOOK:
        // init ONHOOK state
        port->time_lcurr    = 0;
        port->last_lcurr    = 0;
        port->ring_allowed  = 1;
        port->hook_low_pass = AUERFXS_LC_LOW_PASS_INIT;
        port->state = AUERFXS_PORTSTATE_ONHOOK;
        //no break!
    case AUERFXS_PORTSTATE_ONHOOK:
        if( !lcurr_filtered )
        {
            port->hook_low_pass -= AUERFXS_OVERSAM;
        } else
        {
            port->hook_low_pass += lcurr_filtered;
            port->hook_low_pass -= AUERFXS_OVERSAM / 2;
        }
        // range check
        if( port->hook_low_pass > AUERFXS_LC_LOW_PASS_MAX )
        {
            port->hook_low_pass = AUERFXS_LC_LOW_PASS_MAX;
        }
        if( port->hook_low_pass < 0 )
        {
            port->hook_low_pass = 0;
        }
        lcurr = port->hook_low_pass >= AUERFXS_LC_LOW_PASS_THRES ? 1 : 0;
        treat_fxsstate_onhook( lcurr, port );
        break;
    case AUERFXS_PORTSTATE_OFFHOOK:
        treat_fxsstate_offhook( lcurr, port );
        break;
    default:
        // invalid state
        port->state = AUERFXS_PORTSTATE_INIT_ONHOOK;
        break;
    }
    spin_unlock_irqrestore (&port->portlock, flags);
    return;
} // treat_fxsport()

///---------------------------------------------------------------------------------------------------------------------
/**
 * initializes port states of a module
 *  @param      sp          module context
*/
static inline void fxs_port_init( struct aucp3kfxs_entry *sp )
{
    int k ;

    for( k = 0; k < sp->mod_desc->nr_ports; k++ )
    {
        wmb();
        sp->port[ k ].ring_on = 0;
        if( sp->mod_desc->set_linefeed )
        {
            sp->mod_desc->set_linefeed( &sp->port[ k ], AUERFXS_RINGING_OFF );
        }
        sp->port[ k ].state = AUERFXS_PORTSTATE_INIT_ONHOOK;
    }
    return;
} // fxs_port_init()

///---------------------------------------------------------------------------------------------------------------------
/**
 *  treat the fxs ONHOOK state
 *  @param      lcurr       loop current on/off
 *  @param      port        port context
*/
static inline void treat_fxsstate_onhook( u8 lcurr, struct auerfxs_port* port )
{
    // treat state depending on lcurr change (0->0, 0->1, 1->0, 1->1)
    switch( port->last_lcurr << 1 | lcurr )
    {
    // last_lcurr == 0, lcurr == 0
    case AUERFXS_LC_0_TO_0:
        // nothing to do
        break;

    // last_lcurr == 0, lcurr == 1
    case AUERFXS_LC_0_TO_1:
        // start lcurr time capture
        port->time_lcurr=AUERFXS_TIMTIC;
        // stop ringing if appropriate
        port->ring_allowed = 0;
        if( port->ring_on )
        {
            port->slotp->mod_desc->set_linefeed( port, AUERFXS_RINGING_OFF );
        }
        break;

    // last_lcurr == 1, lcurr == 0
    case AUERFXS_LC_1_TO_0:
        // lcurr was to short, ignore (spike)
        port->time_lcurr = 0;
        port->ring_allowed = 1;
        if( port->ring_on )
        {
            port->slotp->mod_desc->set_linefeed( port, AUERFXS_RINGING_ON );
        }
        break;

    // last_lcurr == 1, lcurr == 1
    case AUERFXS_LC_1_TO_1:
        // lcurr time capture, change state if appropriate
        port->time_lcurr += AUERFXS_TIMTIC;
        if( port->time_lcurr >= AUERFXS_MIN_OFFHOOK_TIME )
        {
            // stop ringing
            port->ring_on = 0;
            port->slotp->mod_desc->set_linefeed( port, AUERFXS_RINGING_OFF );
            port->time_nolcurr = 0;
            port->time_lcurr = 0;
            // change state and inform DAHDI
            port->state = AUERFXS_PORTSTATE_OFFHOOK;
            dahdi_hooksig( port->slotp->chans[ port->pnum ], DAHDI_RXSIG_OFFHOOK );
            port->dahdi_hookstate = DAHDI_RXSIG_OFFHOOK;
        }
        break;
    default:
        // ignore
        break;
    }
    port->last_lcurr = lcurr;
    return;
} // treat_fxsstate_onhook()

///---------------------------------------------------------------------------------------------------------------------
/**
 *  treat the fxs OFFHOOK state
 *  @param      lcurr       loop current on/off
 *  @param      port        port context
*/
static inline void treat_fxsstate_offhook( u8 lcurr, struct auerfxs_port* port )
{
    switch( port->last_lcurr << 1 | lcurr )
    {
    // last_lcurr == 0, lcurr == 0
    case AUERFXS_LC_0_TO_0:
        // nolcurr time capture, change state and inform DAHDI if appropriate
        port->time_nolcurr += AUERFXS_TIMTIC;
        if( port->time_nolcurr >= AUERFXS_TIME_DEBOUNCE && port->dahdi_hookstate != DAHDI_RXSIG_ONHOOK )
        {
            port->dahdi_hookstate = DAHDI_RXSIG_ONHOOK;
            dahdi_hooksig( port->slotp->chans[ port->pnum ], DAHDI_RXSIG_ONHOOK );
        }
        if( port->time_nolcurr >= AUERFXS_MIN_ONHOOK_TIME )
        {
            port->state = AUERFXS_PORTSTATE_INIT_ONHOOK;
        }
        break;

    // last_lcurr == 0, lcurr == 1
    case AUERFXS_LC_0_TO_1:
        // ignore lcurr gap if short
        if( port->time_nolcurr < ( AUERFXS_TIME_DEBOUNCE - AUERFXS_TIMTIC ) )
        {
            port->time_nolcurr = 0;
            port->time_lcurr = AUERFXS_TIMTIC;
        } else
        {
            // nolcurr time capture
            port->time_lcurr = AUERFXS_TIMTIC;
        }
        break;

    // last_lcurr == 1, lcurr == 0
    case AUERFXS_LC_1_TO_0:
        // nolcurr time capture
        port->time_nolcurr = AUERFXS_TIMTIC;
        port->time_lcurr = 0;
        break;

    // last_lcurr == 1, lcurr == 1
    case AUERFXS_LC_1_TO_1:
        // lcurr time capture, inform DAHDI if appropriate
        if( port->time_nolcurr )
        {
            port->time_lcurr += AUERFXS_TIMTIC;
            if( port->time_lcurr >= AUERFXS_TIME_DEBOUNCE )
            {
                port->time_nolcurr = 0;
                if( port->dahdi_hookstate != DAHDI_RXSIG_OFFHOOK )
                {
                    dahdi_hooksig( port->slotp->chans[ port->pnum ], DAHDI_RXSIG_OFFHOOK );
                    port->dahdi_hookstate = DAHDI_RXSIG_OFFHOOK;
                }
            }
        }
        break;
    } //switch
    port->last_lcurr = lcurr;
    return;
}// treat_fxsstate_offhook()

///---------------------------------------------------------------------------------------------------------------------
/**
 * Interrupt routine, periodically called by bfin_sport_tdm driver with 1 kHz (== 1 DAHDI chunk), synchronized 
 * to local IOM Bus. Calls dahdi_ec_span(), dahdi_receive() and dahdi_transmit() for each FXS span.
*/
static void aucp3kfxs_interrupt( void )
{
    unsigned int p;
    nr_slot_t slotnr;
    struct aucp3kfxs_entry *sp;

    for( slotnr = 1; slotnr <= CP3000_AUERMODANZ; slotnr++ )
    {
        sp = aucp3kfxs_list[ slotnr ];
        if( sp )
        {
            // scan loop current detect of all ports
            sp->lcurr_samples[ sp->overs_index ] = sp->mod_desc->get_lcurr( sp );
            sp->lcurr_long_samples[ sp->long_overs_index ] = sp->lcurr_samples[ sp->overs_index ];
           // switch ringing voltage if appropriate
            if( sp->mod_desc->make_ringphase )
            {
                sp->mod_desc->make_ringphase( sp );
            }
            // treat all ports with 100 Hz each
            for( p = 0; p < sp->mod_desc->ports_pro_int; p++ )
            {
                if( sp->portindex < sp->mod_desc->nr_ports )
                {
                    treat_fxsport( &sp->port[ sp->portindex ] );
                }
                sp->portindex = ( ++sp->portindex ) % ( sp->mod_desc->ports_pro_int * AUERFXS_OVERSAM );
            }
            sp->overs_index = ( sp->overs_index + 1 ) % AUERFXS_OVERSAM;
            // Process DAHDI ohttimer
            for( p = 0; p < AUERFXS_PORT_MAXNUM; p++ )
            {
                if( sp->cardflag & ( 1 << p ) )
                {
                    if( sp->modfxs[ p ].lasttxhook == 0x4 )
                    {
                        // RINGing, prepare for OHT
                        sp->modfxs[ p ].ohttimer = DAHDI_OHT_TIMER * DAHDI_CHUNKSIZE;
                        sp->modfxs[ p ].idletxhookstate = 0x2; // OHT mode when idle 
                    } else
                    {
                        if( sp->modfxs[ p ].ohttimer )
                        {
                            sp->modfxs[ p ].ohttimer -= DAHDI_CHUNKSIZE;
                            if( !sp->modfxs[ p ].ohttimer )
                            {
                                sp->modfxs[ p ].idletxhookstate = 0x1; // Switch to Active
                                if( sp->modfxs[ p ].lasttxhook == 0x2 )
                                {
                                    // Apply the change if appropriate
                                    sp->modfxs[ p ].lasttxhook = 0x1;
                                }
                            }
                        }
                    }
                }
            }
            // transfer of audio data from/to DAHDI buffers
            dahdi_ec_span( &sp->span );
            dahdi_receive( &sp->span );
            dahdi_transmit( &sp->span );
        } // if( sp )
    } // for( slotnr ...
    return;
} // aucp3kfxs_interrupt()

///---------------------------------------------------------------------------------------------------------------------
/**
 * Codec initialization
 *  @param      base_address    SPI baseaddress
 *  @param      wideband        0 if 8 KHz, !=0 if 16 KHz
 *  @param      is_AD_SSM26xx   codec manufacturer, 0 if ADI codec, 1 if not
*/
static void ssm2602_init( void __iomem * base_address, int wideband, uint16_t is_AD_SSM26xx )
{
    // POWER UP SEQUENCE
    // o  Switch on power supplies. By default the Codec is in Standby Mode, the DAC is
    //    digitally muted and the Audio Interface and Outputs are all OFF.
    write_ssm2602_reg( base_address, AD_SSM26xx_RESET | 0x0 ); // 000000000 chip reset

    // o  Set all required bits in the Power Down Register (0Ch) to '0'; EXCEPT the OUTPD
    //    bit, this should be set to '1' (Default).
    write_ssm2602_reg( base_address, AD_SSM26xx_PM | 0x0072 ); // 001110010 PD: power up, CLKOUT dis, OSC dis, OUT dis, DAC en, ADC en, MIC dis, LINEIN en

    // o  Set required values in all other registers except ACTIVE.
    write_ssm2602_reg( base_address, AD_SSM26xx_L_IV | 0x100 | AD_SSM26xx_IV_0DB ); // 100010111 RLI = LLI: not mute, 0 dB
    write_ssm2602_reg( base_address, AD_SSM26xx_L_DACV | 0x100 );  // 100000000 LHPO = RHPO: mute, no zero cross detect

    write_ssm2602_reg( base_address, AD_SSM26xx_AAP | 0x0012 ); // 000010010 AAPC: no Micboost2, no sidetone, select DAC, BYPASS=0, ADC=Line in, Mute Mic, No Mic Boost

    write_ssm2602_reg( base_address, AD_SSM26xx_DAP | 0x0000 ); // 000000000 DAPC: HPOR=0, DACMU=0, no deemph., enable high pass

    if( is_AD_SSM26xx )
    {
        write_ssm2602_reg( base_address, AD_SSM26xx_DA_IF | 0x0093 ); // 010010011 DAIF: BLCK inv., slave mode, LRSWAP=0, LRP=1, IWL=16bit, DSP format
    } else
    {
        write_ssm2602_reg( base_address, AD_SSM26xx_DA_IF | 0x0013 ); // 000010011 DAIF: BLCK non-inv., slave mode, LRSWAP=0, LRP=1, IWL=16bit, DSP format
    }
    if( wideband )
    {
        if( is_AD_SSM26xx )
        {
            write_ssm2602_reg( base_address, AD_SSM26xx_SR | 0x0014 ); // 000010100: CLKOUT,Clk=CLKIN,SR=16KHz,BOSR=256fs, no USB
        } else
        {
            write_ssm2602_reg( base_address, AD_SSM26xx_SR | 0x0058 ); // 001011000: CLKOUT,Clk=CLKIN/2,SR=32KHz,BOSR=256fs, no USB
        }
    } else
    {
        write_ssm2602_reg( base_address, AD_SSM26xx_SR | 0x000C );     // 000001100: CLKOUT,Clk=CLKIN,SR=8KHz,BOSR=256fs, no USB
    }
    // Here we have to WAIT at least T=C*25000/3.5, where C is the VMID capacitor.
    // This information is from AD, and it is not covered in the data sheet(!)
    // For 10uF, this is 72 ms.
    schedule_timeout( HZ / 10 );
    // o  Set the 'Active' bit.
    write_ssm2602_reg( base_address, AD_SSM26xx_ACTIVE | 0x0001 ); // 000000001
    // o  The last write of the sequence should be setting OUTPD to '0' (active) in register
    //    0Ch, enabling the DAC signal path, free of any significant power-up noise.
    write_ssm2602_reg( base_address, AD_SSM26xx_PM | 0x0062 ); // 001100010 PD: power up, CLKOUT dis, OSC dis, OUT en, DAC en, ADC en, MIC dis, LINEIN en
    return;
} // ssm2602_init()


///---------------------------------------------------------------------------------------------------------------------
/**
 * Init mainboard FXS codecs
*/
static inline void ssm2602_init_cp3000_gb( void )
{
    uint16_t is_AD_SSM26xx = 1;
    // ADI codec?
    if( read_rmodid() & CP3000_MODID_GB_CODEC )
    {
        is_AD_SSM26xx = 0;
    }
    // FXS 1+2
    ssm2602_init( calc_baseaddress( AUERMOD_CP3000_SLOT_AB, 0 ), 1, is_AD_SSM26xx );
    // FXS 3+4
    ssm2602_init( calc_baseaddress( AUERMOD_CP3000_SLOT_AB, 1 ), 1, is_AD_SSM26xx );
    return;
} // ssm2602_init_cp3000_gb()

///---------------------------------------------------------------------------------------------------------------------
/**
 * Init plug in module FXS codec
*/
static inline void ssm2602_init_cp3000_module( void )
{
    uint16_t is_AD_SSM26xx = 1;
    // ADI codec?
    if( read_rmodid() & CP3000_MODID_MOD_CODEC )
    {
        is_AD_SSM26xx = 0;
    }
    // moduleclock = 16 kHz / framesync early
    write_wclock( WCLOCK_TN14_16 | WCLOCK_TN56_16 );
    // FXS 5+6
    ssm2602_init( calc_baseaddress( AUERMOD_CP3000_SLOT_MOD, 0 ), 1, is_AD_SSM26xx );
    return;
} // ssm2602_init_cp3000_module()

///---------------------------------------------------------------------------------------------------------------------
/**
 * Set FXS ports to initial state
 *  @param      sp          module context
*/
static void auer2_4ab_resetmodule( struct aucp3kfxs_entry *sp )
{
    int k;

    // init hardware
    switch( sp->moduleID )
    {
    case MODID2AB:
        write_wtnk56( 0 );             // ringing voltage off
        ssm2602_init_cp3000_module();
        break;
    case MODID4AB:
        write_wtnk14( 0 );             // ringing voltage off
        ssm2602_init_cp3000_gb();
        break;
    default:
        break;
    }
    // init port statemachines
    fxs_port_init( sp );
    for( k = 0; k < sp->mod_desc->nr_ports; k++ )
    {
        wmb();
        auer2_4ab_setlinefeed( &sp->port[ k ], AUERFXS_RINGING_OFF );
    }
    return;
} // auer2_4ab_resetmodule()

///=====================================================================================================================
/**
* Callback funktions for DAHDI span
*/
///=====================================================================================================================
// => span.hooksig
static int aucp3kfxs_hooksig( struct dahdi_chan *chan, enum dahdi_txsig txsig )
{
    struct aucp3kfxs_entry *sp = chan->pvt;

    switch( txsig )
    {
    case DAHDI_TXSIG_ONHOOK:
        switch( chan->sig )
        {
        case DAHDI_SIG_EM:
        case DAHDI_SIG_FXOKS:
        case DAHDI_SIG_FXOLS:
            sp->modfxs[ chan->chanpos - 1 ].lasttxhook = sp->modfxs[ chan->chanpos - 1 ].idletxhookstate;
            break;
        case DAHDI_SIG_FXOGS:
            sp->modfxs[ chan->chanpos - 1 ].lasttxhook = 3;
            break;
        default:
            break;
        }
        break;
    case DAHDI_TXSIG_OFFHOOK:
        switch( chan->sig )
        {
        case DAHDI_SIG_EM:
            sp->modfxs[ chan->chanpos - 1 ].lasttxhook = 5;
            break;
        default:
            sp->modfxs[ chan->chanpos - 1 ].lasttxhook = sp->modfxs[ chan->chanpos - 1 ].idletxhookstate;
            break;
        }
        break;
    case DAHDI_TXSIG_START:
        sp->modfxs[ chan->chanpos - 1 ].lasttxhook = 4;
        break;
    case DAHDI_TXSIG_KEWL:
        sp->modfxs[ chan->chanpos - 1 ].lasttxhook = 0;
        break;
    default:
        printk( KERN_NOTICE "aucp3kfxs: Can't set tx state to %d\n", txsig );
    }
    if( debug )
    {
        printk( KERN_DEBUG "Setting FXS hook state to %d (%02x)\n", txsig, sp->modfxs[ chan->chanpos - 1 ].lasttxhook );
    }
    sp->port[ chan->chanpos - 1 ].ring_on = ( sp->modfxs[ chan->chanpos - 1 ].lasttxhook == 4 ) ? 1 : 0;
    if( !sp->port[ chan->chanpos - 1 ].ring_on && sp->mod_desc->set_linefeed )
    {
        sp->mod_desc->set_linefeed( &sp->port[ chan->chanpos - 1 ], AUERFXS_RINGING_OFF );
    }
    return 0;
} // aucp3kfxs_hooksig()

// => span.open
static int aucp3kfxs_open( struct dahdi_chan *chan )
{
    struct aucp3kfxs_entry *sp = chan->pvt;

    if( !( sp->cardflag & ( 1 << ( chan->chanpos - 1 ) ) ) )
    {
        return -ENODEV;
    }
    if( sp->dead )
    {
        return -ENODEV;
    }
    sp->usecount++;
    try_module_get( THIS_MODULE );
    return 0;
} // aucp3kfxs_open()

// => span.close
static int aucp3kfxs_close( struct dahdi_chan *chan )
{
    struct aucp3kfxs_entry *sp = chan->pvt;

    sp->usecount--;
    module_put( THIS_MODULE );
    sp->modfxs[ chan->chanpos - 1 ].idletxhookstate = 1;
    // If we're dead, release us now
    if( !sp->usecount && sp->dead )
    {
        aucp3kfxs_release( sp );
    }
    return 0;
} // aucp3kfxs_close()


// => span.ioctl
static int aucp3kfxs_ioctl( struct dahdi_chan *chan, unsigned int cmd, unsigned long data )
{
    switch( cmd )
    {
    case DAHDI_ONHOOKTRANSFER:
    // no action required on CP3000, OHT always allowed (beyond ringbursts)
        break;
    case DAHDI_SETPOLARITY:     // not possible with COMpact 3000 FXS
    case DAHDI_VMWI_CONFIG:     // not possible with COMpact 3000 FXS
    case DAHDI_VMWI:            // not possible with COMpact 3000 FXS
    case DAHDI_SET_HWGAIN:      // not possible with COMpact 3000 FXS
        return -EINVAL;
    default:
        return -ENOTTY;
    }
    return 0;
} // aucp3kfxs_ioctl()

// => span.watchdog
static int aucp3kfxs_watchdog( struct dahdi_span *span, int event )
{
    return 0;
} // aucp3kfxs_watchdog()

///=====================================================================================================================
/**
 * Switch linefeed/ringing voltage
 *  @param      sp          module context
 *  @param      val         @see AUERFXS_LINEFEED_POS
*/
static inline void switch_linefeed( struct aucp3kfxs_entry *sp, u8 val )
{
    unsigned int portnr;

    for( portnr = 0; portnr < sp->mod_desc->nr_ports; portnr++ )
    {
        if( sp->port[ portnr ].ring_on )
        {
            if( sp->port[ portnr ].ring_allowed )
            {
                sp->mod_desc->set_linefeed( &sp->port[ portnr ], val );
            }
        }
    }
    return;
} // switch_linefeed()

///=====================================================================================================================
/**
* Callback funktions for module description table
*/
///=====================================================================================================================
/**
 * Callback: Generate ringing frequency (called with AUERFXS_INTF Hz)
 *  =>sp->mod_desc.make_ringphase
 *  @param      sp          module context
*/
static void auer6_10ab_ringphase( struct aucp3kfxs_entry *sp )
{
    int half_period = ( AUERFXS_INTF / ( 2 * sp->ringfreq ) );

    if( sp->long_overs_index % half_period == 0 )
    {
        switch_linefeed( sp, ( ( sp->long_overs_index / half_period ) & 0x01 ) ? AUERFXS_LINEFEED_NEG : AUERFXS_LINEFEED_POS );
    }
    if( ( sp->long_overs_index % half_period ) == ( half_period - 2 ) )
    {
        switch_linefeed( sp, AUERFXS_LINEFEED_ZERO );
    }
    sp->long_overs_index = ( sp->long_overs_index + 1 ) % ( 2 * half_period );
    return;
}   // auer6_10ab_ringphase()

/**
 * Callback: Set linefeed in hardware
 *  =>sp->mod_desc.set_linefeed
 *  @param      port_p      a/b-Port
 *  @param      val         @see AUERFXS_LINEFEED_POS
*/
static void auer2_4ab_setlinefeed( struct auerfxs_port* port_p, u8 val )
{
    unsigned long flags;

    spin_lock_irqsave( &port_p->slotp->sreg_lock, flags );
    switch( val )
    {
    case AUERFXS_RINGING_OFF:
    case AUERFXS_LINEFEED_POS:
        port_p->slotp->sreg_linefeed |=  ( 1 << ( ( port_p->pnum % 4 ) + OFFSET_POS ) );
        port_p->slotp->sreg_linefeed &= ~( 1 << ( ( port_p->pnum % 4 ) + OFFSET_NEG ) );
        break;
    case AUERFXS_LINEFEED_NEG:
        port_p->slotp->sreg_linefeed |=  ( 1 << ( ( port_p->pnum % 4 ) + OFFSET_NEG ) );
        port_p->slotp->sreg_linefeed &= ~( 1 << ( ( port_p->pnum % 4 ) + OFFSET_POS ) );
        break;
    case AUERFXS_RINGING_ON:
        // ignore, make_ringphase will do this
        break;
    case AUERFXS_LINEFEED_ZERO:
    default:
        port_p->slotp->sreg_linefeed &= ~( 1 << ( ( port_p->pnum % 4 ) + OFFSET_NEG ) );
        port_p->slotp->sreg_linefeed &= ~( 1 << ( ( port_p->pnum % 4 ) + OFFSET_POS ) );
        break;
    }
    // write shadow regs to hw
    if( port_p->slotp->moduleID == MODID4AB )
    {   // 4 ports on mainboard
        write_wtnk14( port_p->slotp->sreg_linefeed );
    } else
    {   // 2 ports on plug in module
        write_wtnk56( port_p->slotp->sreg_linefeed );
    }
    spin_unlock_irqrestore (&port_p->slotp->sreg_lock, flags);
    return;
} // auer2_4ab_setlinefeed()

/**
 * Callback: Check loop closure of all FXS ports
 *  =>sp->mod_desc.get_lcurr
 *  @param      sp          module context
 *  @return                 loop current bits
*/
static u16 auer6_10ab_get_lcurr( struct aucp3kfxs_entry *sp )
{
    u16 lcurr;
    unsigned long flags;

    spin_lock_irqsave( &sp->sreg_lock, flags );
    switch( sp->moduleID )
    {
    case MODID2AB:
        lcurr = ( u16 )( ( read_ranrs() >> 4 ) & sp->mod_desc->port_mask );
        break;
    case MODID4AB:
        lcurr = ( u16 )read_ranrs() & sp->mod_desc->port_mask;
        break;
    default:
        lcurr = 0;
        break;
    }
    spin_unlock_irqrestore (&sp->sreg_lock, flags);
    return lcurr;
} // auer6_10ab_get_lcurr()

///=====================================================================================================================
/**
 *  module description tables
*/
fxs_module_desc_t desc_mod_4ab =
{
    module_desc:    "AUERSWALD COMpact 3000",
    port_mask:      0x000f,
    nr_ports:       AUER4AB_PORTS,
    ports_pro_int:  ( ( AUER4AB_PORTS - 1 ) / AUERFXS_OVERSAM ) + 1,
    location:       "mainboard",
    // callback
    make_ringphase: auer6_10ab_ringphase,
    init:           auer2_4ab_resetmodule,
    set_linefeed:   auer2_4ab_setlinefeed,
    get_lcurr:      auer6_10ab_get_lcurr
};

fxs_module_desc_t desc_mod_2ab =
{
    module_desc:    "AUERSWALD COMpact 2a/b-Modul",
    port_mask:      0x0003,
    nr_ports:       AUER2AB_PORTS,
    ports_pro_int:  ( ( AUER2AB_PORTS - 1 ) / AUERFXS_OVERSAM ) + 1,
    location:       "plug in module",
    // callback
    make_ringphase: auer6_10ab_ringphase,
    init:           auer2_4ab_resetmodule,
    set_linefeed:   auer2_4ab_setlinefeed,
    get_lcurr:      auer6_10ab_get_lcurr
};

///---------------------------------------------------------------------------------------------------------------------
/**
 * Get moduledescription
 *  @param      id          moduleID
*/
static inline fxs_module_desc_t * auerfxs_get_modDesc( unsigned int id )
{
    switch( id )
    {
    case MODID4AB:
        return &desc_mod_4ab;
    case MODID2AB:
        return &desc_mod_2ab;
    default:
        break;
    }
    return NULL;
} // auerfxs_get_modDesc()

///---------------------------------------------------------------------------------------------------------------------
/**
 * release module
 *  @param      sp          module context
*/
static void aucp3kfxs_release( struct aucp3kfxs_entry * sp )
{
    dahdi_unregister( &sp->span );
    if( sp->moduleID )
    {
        // reinit hardware
        sp->mod_desc->init( sp );
    }
    // remove from module table
    aucp3kfxs_list[ sp->slot ] = NULL;
    // free memory
    kfree( sp );
    printk( KERN_INFO "COMpact 3000 FXS module released\n");
    return;
} // aucp3kfxs_release()

///---------------------------------------------------------------------------------------------------------------------
/**
 * slot deinit
 *  @param      slot        slotnr (1... )
*/
static inline int aucp3kfxs_remove_one( nr_slot_t slot )
{
    struct aucp3kfxs_entry * sp = aucp3kfxs_list[ slot ];

    if( sp )
    {
        // Release span
        if( !sp->usecount )
        {
            aucp3kfxs_release( sp );
        } else
        {
            sp->dead = 1;
        }
    }
    return 0;
} // aucp3kfxs_remove_one()

///---------------------------------------------------------------------------------------------------------------------
/**
 * initializes a group of FXS ports
 *  @param      sp          module context
 *  @return     0: ok, !0: Error
 *
 *  already initialized when called:
 *      sp->moduleID
 *      sp->slot
 *      sp->chans
*/
static int __init init_slot( struct aucp3kfxs_entry * sp )
{
    unsigned int pnum; 

    if( !sp )
    {
        printk( KERN_ERR "%s called with invalid context ptr \n", __FUNCTION__ );
        return -1;
    }
    sp->mod_desc = auerfxs_get_modDesc( sp->moduleID );
    if( !sp->mod_desc )
    {
        printk( KERN_ERR "%s: no module description for module in slot %d \n", __FUNCTION__, sp->slot );
        return -1;
    }
    spin_lock_init ( &sp->sreg_lock );
    sp->ringfreq = AUERFXS_RINGFREQ_50HZ;
    // init ports
    for( pnum = 0; pnum < sp->mod_desc->nr_ports; pnum++ )
    {
        struct auerfxs_port *pp = &sp->port[ pnum ]; // port contxt ptr
        spin_lock_init( &pp->portlock );
        pp->slot = sp->slot;
        pp->slotp = sp;
        pp->pnum = pnum;
        pp->mask = 0x01 << pnum;
    }
    // DAHDI stuff
    sprintf( sp->span.name, "AUERASK_CP3K_FXS/%d", sp->slot );
    snprintf( sp->span.desc, sizeof( sp->span.desc ) - 1, "%s", sp->mod_desc->module_desc );
    snprintf( sp->span.location, sizeof( sp->span.location ) - 1, "%s", sp->mod_desc->location );
    snprintf( sp->span.devicetype, sizeof( sp->span.devicetype ) - 1, "%s", sp->mod_desc->module_desc );
    sp->span.manufacturer = "Auerswald GmbH & Co. KG";
    if( alawoverride )
    {
        printk( KERN_INFO "ALAW override parameter detected.  Device will be operating in ALAW\n" );
        sp->span.deflaw = DAHDI_LAW_ALAW;
    } else
    {
        sp->span.deflaw = DAHDI_LAW_MULAW;
    }
    for( pnum = 0; pnum < sp->mod_desc->nr_ports; pnum++ )
    {
        sprintf( sp->chans[ pnum ]->name, "%s/slot%d/port%d", sp->mod_desc->module_desc, sp->slot, pnum );
        sp->chans[ pnum ]->sigcap = DAHDI_SIG_FXOKS | DAHDI_SIG_FXOLS | DAHDI_SIG_FXOGS | DAHDI_SIG_SF | DAHDI_SIG_EM | DAHDI_SIG_CLEAR;
        sp->chans[ pnum ]->chanpos = pnum + 1;
        sp->chans[ pnum ]->pvt = sp;
        sp->cardflag |= ( 1 << pnum );
    }
    sp->span.chans = sp->chans;
    sp->span.channels = sp->mod_desc->nr_ports;
    sp->span.hooksig = aucp3kfxs_hooksig;
    sp->span.open = aucp3kfxs_open;
    sp->span.close = aucp3kfxs_close;
    sp->span.flags = DAHDI_FLAG_RBS;
    sp->span.ioctl = aucp3kfxs_ioctl;
    sp->span.watchdog = aucp3kfxs_watchdog;
    init_waitqueue_head( &sp->span.maintq );
    sp->span.pvt = sp;
    if( dahdi_register( &sp->span, 1 ) )    // preferred master!
    {
        printk( KERN_NOTICE "Unable to register span with DAHDI\n" );
        aucp3kfxs_list[ sp->slot ] = NULL;
        return -1;
    } else
    {
        printk( KERN_DEBUG "Span %s registered with DAHDI\n", sp->span.name );
    }
    // init hardware of the detected module
    sp->mod_desc->init( sp );
    return 0;
} // init_slot()

///---------------------------------------------------------------------------------------------------------------------
/**
 * checks presence of COMpact 2a/b FXS module and initializes all FXS ports
*/
static int __init auerfxs_probe( void )
{
    int i;
    nr_slot_t slotnr;
    unsigned int id;
    struct aucp3kfxs_entry * sp = NULL;

    // find FXS modules
    for( slotnr = CP3000_AUERMODANZ; slotnr > 0; slotnr-- )
    {
        id = auerask_cp3k_read_modID( slotnr );
        switch( id )
        {
        case MODID4AB:
        case MODID2AB:
            sp = ( struct aucp3kfxs_entry * ) kmalloc( sizeof( struct aucp3kfxs_entry), GFP_KERNEL);
            if( !sp )
            {
                printk( KERN_ERR "%s: No kmem for FXS module \n", __FUNCTION__ );
                return -ENOMEM;
            }
            aucp3kfxs_list[ slotnr ] = sp;
            memset( sp, 0, sizeof( struct aucp3kfxs_entry ) );
            sp->moduleID = id;
            sp->slot = slotnr;
            for( i = 0; i < sizeof( sp->chans ) / sizeof( sp->chans[ 0 ]); ++i )
            {
                sp->chans[ i ] = &sp->_chans[ i ];
            }
            if( init_slot( sp ) < 0 )
            {
                printk( KERN_ERR "%s: init_slot ERROR! virtual slot %d \n", __FUNCTION__, slotnr );
                aucp3kfxs_cleanup();
                return -1;
            }
            card_cnt++;
            break;
        default:
            break;
        }
    }
    return 0;
} // auerfxs_probe()

///---------------------------------------------------------------------------------------------------------------------
/**
 * driver initialization, called after module loading.
*/
static int __init aucp3kfxs_init( void )
{
    memset( &aucp3kfxs_list, 0, sizeof( aucp3kfxs_list ) );
    auerask_cp3k_spi_init();
    if( auerfxs_probe() )
    {
        auerask_cp3k_spi_exit();
        return -ENODEV;
    }
    return 0;
} // aucp3kfxs_init()

///---------------------------------------------------------------------------------------------------------------------
/**
 * driver deinit, called before module removal.
*/
static void aucp3kfxs_cleanup( void )
{
    nr_slot_t slotnr;

    // remove modules
    for( slotnr = 1; slotnr <= CP3000_AUERMODANZ; slotnr++ )
    {
        aucp3kfxs_remove_one( slotnr );
    }
    auerask_cp3k_spi_exit();
} // aucp3kfxs_cleanup

///---------------------------------------------------------------------------------------------------------------------
/**
 * exit section
*/
static void __exit aucp3kfxs_exit( void )
{
    aucp3kfxs_cleanup();
} // aucp3kfxs_exit

///---------------------------------------------------------------------------------------------------------------------

EXPORT_SYMBOL( aucp3kfxs_interrupt );
EXPORT_SYMBOL( aucp3kfxs_get_chan_tx_chunk );
EXPORT_SYMBOL( aucp3kfxs_get_chan_rx_chunk );

module_param(debug, int, 0600);

MODULE_DESCRIPTION("Auerswald COMpact 3000 FXS port driver");
MODULE_AUTHOR("<info@auerswald.de>");
MODULE_LICENSE("GPL");

module_init( aucp3kfxs_init );
module_exit( aucp3kfxs_exit );
