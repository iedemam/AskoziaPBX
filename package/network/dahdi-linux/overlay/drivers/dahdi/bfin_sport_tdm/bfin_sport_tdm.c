/*
 * File:         drivers/char/bfin_sport_tdm.c
 *
 * Based on:     drivers/char/bfin_sport.c by Roy Huang (roy.huang@analog.com)
 *
 * Created:      Dec. 03 2009
 * Description:  Simple driver to use Blackfin Sport as a TDM Interface for COMpact 3000 / Dahdi / Asterisk
 *
 * Modified:
 *               Copyright 2004-2006 Analog Devices Inc.
 *               Copyright 2009-2010 Auerswald GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* Limitations
 * - only support COMpact 3000 VoIP
 * - only support TDM interface for internal analog interfaces (FSX) on board and on module
 * - only support TDM interface for internal ISDN/BRI interface on module
 * - user interface for test purpose only
 * - payload transfer directly to Dahdi driver
 * - only for SPORT0 on BF52x
 * - only support 8kHz narrow band on interface to Dahdi/Asterisk
 *
 */


#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <asm/blackfin.h>
#include <asm/bfin_sport.h>
#include <asm/dma.h>
#include <asm/cacheflush.h>

#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/signal.h>
#include <asm/gpio.h>
#include <asm/portmux.h>

// some headers for conversion, see copyright inside or http://www.soft-switch.org/downloads/spandsp/
#include "bit_operations.h"
#include "g711.h"

#include "../auerask_cp3k_mb/auerask_cp3k_mb.h"             //$$$XHe

/*
 * option switch
 */
//#define DEBUG
#undef DEBUG

//#define NO_FXS_MOD
#undef NO_FXS_MOD

//#define NO_BRI_MOD
#undef NO_BRI_MOD

#define SPORT_ANOMALY_05000462
//#undef SPORT_ANOMALY_05000462

//#define SPORT_SEND_TEST_TONE
#undef SPORT_SEND_TEST_TONE

//#define SPORT_TEST_LOOPBACK
#undef SPORT_TEST_LOOPBACK

/*
 * Defines
 */
#define     DRV_NAME "bf52x-sport-tdm"
#define     SPORT_TDM_NR_DEVS               1               // only support TDM 0 for BF52x
#define     KF_CHANNELS_MAX                 16             // amount of usable channels (8kHz/8Bit; see TDM map)
#define     DMA_INNER_LOOP_TIME             1                   // ms
#define     DMA_FRAMES_PER_MS               8                   // buffer per ms
#define     DMA_TS_BIT_LEN                  16                  // wideband timeslot bit length
#define     DMA_TS_PER_BUF                  16                  // time slots per fram
#define     DMA_TS_DIST                     (DMA_TS_PER_BUF/2)  // time slot distance
#define     DMA_BUF_WORD_LEN                (DMA_INNER_LOOP_TIME*DMA_FRAMES_PER_MS*DMA_TS_PER_BUF)               // no. of word in buffer
#define     DMA_BUF_BYTE_LEN                (DMA_BUF_WORD_LEN*(DMA_TS_BIT_LEN/8))                                // no. of bytes in buffer
#define     DMA_BUF_COUNT                   2           // no. of buffers / double buffer
#define     TDM_DIST_WIDEBAND               8           // distance of wideband timeslots in buffer
#define     DISTANZ_BYTE                    DMA_TS_PER_BUF*2        // distance of byte timeslots in buffer
#define     DISTANZ_WORD                    TDM_DIST_WIDEBAND       // distance of wideband timeslots of the same channel (FXS)
#define     DISTANZ_DIREKT                  1                       // distance of wideband timeslots of the same channel (POTS/FXO)

int sport_major =   SPORT_MAJOR;
int sport_minor =   0;

struct sport_dev *sport_devices;	/* allocated in sport_init_module */
static struct class *sport_class;

// DMA transfer double buffer
static  int16_t *dma_rx_buf;
static  int16_t *dma_tx_buf;
static  int16_t *conv_in_buf;
static  int16_t *conv_out_buf;

#ifndef NO_BRI_MOD
extern void xhfc_int(void);
extern u8* xhfc_get_bchan_tx_chunk(nr_slot_t slot, nr_port_t pnum, __u8 bchan);
extern u8* xhfc_get_bchan_rx_chunk(nr_slot_t slot, nr_port_t pnum, __u8 bchan);
#endif
#ifndef NO_FXS_MOD
extern void aucp3kfxs_interrupt( void );
extern u8* aucp3kfxs_get_chan_tx_chunk( nr_slot_t slot, nr_port_t pnum );
extern u8* aucp3kfxs_get_chan_rx_chunk( nr_slot_t slot, nr_port_t pnum );
#endif

static unsigned short bfin_char_pin_req_sport[] =
    {P_SPORT0_DTPRI, P_SPORT0_TSCLK, P_SPORT0_RFS, P_SPORT0_DRPRI, P_SPORT0_RSCLK, 0};


#undef assert

#ifdef DEBUG
#define sport_tdm_debug(fmt, arg...)  printk("%s:%d: " fmt, __FUNCTION__, __LINE__, ##arg)
#define assert(expr) \
	if (!(expr)) { \
		printk("Assertion failed! %s, %s, %s, line=%d \n", \
			#expr, __FILE__, __FUNCTION__, __LINE__); \
	}
#else
#define assert(expr)
#define sport_tdm_debug(fmt, arg...)  ({ if (0) printk(fmt, ##arg); 0; })
#endif

// mode/bit rate on timeslot
typedef enum
{
    e_kf_mode_unused = 0,
    e_kf_mode_8kHz,                     // 16 Bit / 8kHz Narrowband
    e_kf_mode_16kHz,                    // 16 Bit / 16kHz Wideband
    e_kf_mode_max
} e_kf_mode;

// timeslot description
typedef struct {
    e_kf_mode   mode;
    int16_t     ts_distance;            // dist. between both 16 bit timeslot in a frame
    int16_t     *rx_delay_line;         // Delay line for 16<>8 kHz Filter rx / upstream
    int16_t     *tx_delay_line;         // Delay line for 8<>16 kHz Filter tx / downstream
    u8          *dahdi_rx_chunk;        // dahdi receive chunk
    u8          *dahdi_tx_chunk;        // dahdi transmit chunk
} t_tdm;

/* COMpact 3000 TDM Map
 *
 * Usable hardware timeslots ( 16 * 8 bit with 8 kHz).
 *
 * FXS and FXO ports are wideband and need 4 normal timeslots (2 * 16 Bit for 16kHz with 16 Bit linear).
 * If a wideband timeslot is used the following odd timeslot is also used.
 *
 * First half (no. 0 - 15; usable)
 *                                      _________________________________________________________________________________
 * TDM channel number                   | 00 | 01 | 02 | 03 | 04 | 05 | 06 | 07 | 08 | 09 | 10 | 11 | 12 | 13 | 14 | 15 |
 *                                      |-------------------------------------------------------------------------------|
 * COMpact 3000 VoIP                    |                                       |  FXS1_1 |  FXS2_1 |  FXS3_1 |  FXS4_1 |
 * COMpact 3000 analog                  |                   | FXO_1   | FX0_2   |  FXS1_1 |  FXS2_1 |  FXS3_1 |  FXS4_1 |
 * COMpact 3000 ISDN                    |                   | B1 | B2 |         |  FXS1_1 |  FXS2_1 |  FXS3_1 |  FXS4_1 |
 * COMpact 3000 2 * FXS module          |  FXS5_1 |  FXS6_1 |                                                           |
 * COMpact 3000 1 * BRI/ISDN module     | B1 | B2 |                                                                     |
 *                                      |--------------------------------------------------------------------------------
 *
 * Second half (no. 16 - 31); only used for secondary wideband timeslots for FXS
 *
 * TDM channel number                   | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 | 24 | 25 | 26 | 27 | 28 | 29 | 30 | 31 |
 *                                      |-------------------------------------------------------------------------------|
 * COMpact 3000 VoIP                    |                                       |  FXS1_2 |  FXS2_2 |  FXS3_2 |  FXS4_2 |
 * COMpact 3000 analog                  |                                       |  FXS1_2 |  FXS2_2 |  FXS3_2 |  FXS4_2 |
 * COMpact 3000 ISDN                    |                                       |  FXS1_2 |  FXS2_2 |  FXS3_2 |  FXS4_2 |
 * COMpact 3000 2 * FXS module          |  FXS5_2 |  FXS6_2 |                                                           |
 * COMpact 3000 1 * BRI/ISDN module     |_______________________________________________________________________________|
 */

// Initial TDM configuration
static t_tdm phys_tdm[KF_CHANNELS_MAX] = {
    {e_kf_mode_unused,           0, NULL, NULL, NULL, NULL},   // 00: module BRI/ISDN: B1  | module 2*FXS: port 5
    {e_kf_mode_unused,           0, NULL, NULL, NULL, NULL},   // 01: module BRI/ISDN: B2
    {e_kf_mode_unused,           0, NULL, NULL, NULL, NULL},   // 02:                        module 2*FXS: port 6
    {e_kf_mode_unused,           0, NULL, NULL, NULL, NULL},   // 03:
    {e_kf_mode_unused,           0, NULL, NULL, NULL, NULL},   // 04: CP3000 analog  : FXO | CP3000 ISDN : B1
    {e_kf_mode_unused,           0, NULL, NULL, NULL, NULL},   // 05:                      | CP3000 ISDN : B2
    {e_kf_mode_unused,           0, NULL, NULL, NULL, NULL},   // 06: CP3000 analog  : FXO
    {e_kf_mode_unused,           0, NULL, NULL, NULL, NULL},   // 07:
    {e_kf_mode_16kHz, DISTANZ_WORD, NULL, NULL, NULL, NULL},   // 08: FSX port 1
    {e_kf_mode_unused,           0, NULL, NULL, NULL, NULL},   // 09:
    {e_kf_mode_16kHz, DISTANZ_WORD, NULL, NULL, NULL, NULL},   // 10: FSX port 2
    {e_kf_mode_unused,           0, NULL, NULL, NULL, NULL},   // 11:
    {e_kf_mode_16kHz, DISTANZ_WORD, NULL, NULL, NULL, NULL},   // 12: FSX port 3
    {e_kf_mode_unused,           0, NULL, NULL, NULL, NULL},   // 13:
    {e_kf_mode_16kHz, DISTANZ_WORD, NULL, NULL, NULL, NULL},   // 14: FSX port 4
    {e_kf_mode_unused,           0, NULL, NULL, NULL, NULL}    // 15:
};


/* TODO: Hier sind nur einige einfache Testbuffer definiert. Die sollten durch die Dahdi _DOPPEL_Buffer ersetzt werden */
static int8_t     test_io_rx_buf[KF_CHANNELS_MAX][DMA_FRAMES_PER_MS] __attribute__ ((l1_data));
static int8_t     test_io_tx_buf[KF_CHANNELS_MAX][DMA_FRAMES_PER_MS] __attribute__ ((l1_data));

// filter 8 <> 16 kHz
#define SWDSP_SAMPLING_NTaps 24
static int Saturate=0;

static int16_t SWDSP_LOWFOUR_FTaps[SWDSP_SAMPLING_NTaps] __attribute__ ((l1_data)) = {
        0xFDEF,
        0x00A7,
        0x02F7,
        0x00AD,
        0xFC6C,
        0xFEF7,
        0x0615,
        0x0378,
        0xF69A,
        0xF771,
        0x153A,
        0x3745,
        0x3745,
        0x153A,
        0xF771,
        0xF69A,
        0x0378,
        0x0615,
        0xFEF7,
        0xFC6C,
        0x00AD,
        0x02F7,
        0x00A7,
        0xFDEF,
};


static inline int Interpolate(int TapsPhase, const int16_t *const Koeff,
       int16_t *Delay, int Len, int16_t *In, int16_t *Out);

/* Original aus multirate_algs von DSP-Guru, auf Fixedpoint adaptiert */
static inline int Decimate(int Taps, const int16_t *const Koeff,
      int16_t *Delay, int Len, int16_t *In, int16_t *Out);


static int swdsp_Decimate(int16_t *Delay, int16_t *InBuffer, int16_t *OutBuffer, int Len) {
    return Decimate(SWDSP_SAMPLING_NTaps, SWDSP_LOWFOUR_FTaps, Delay, Len, InBuffer, OutBuffer);
}
static int swdsp_Interpolate(int16_t *Delay, int16_t *InBuffer, int16_t *OutBuffer, int Len) {
    return Interpolate(SWDSP_SAMPLING_NTaps/2, SWDSP_LOWFOUR_FTaps, Delay, Len, InBuffer, OutBuffer);
}

#ifdef SPORT_ANOMALY_05000462

#define SPORT_TEST_MUSTER               0x55AA
#define SPORT_TEST_MUSTER_OFFSET        10          // 16 bit unused timeslot
#define SPORT_TEST_MUSTER_BIT_OFFSET    ((SPORT_TEST_MUSTER_OFFSET*DMA_TS_BIT_LEN)-1)
#define SPORT_GPIO_TX         (1<<6)
#define SPORT_GPIO_RX         (1<<7)
#define SPORT_GPIO_FS         (1<<8)
#define SPORT_GPIO_BCLK       (1<<9)
#define SPORT_GPIO_MASK       (SPORT_GPIO_TX | SPORT_GPIO_RX | SPORT_GPIO_FS | SPORT_GPIO_BCLK)

static inline void sport_check_anomaly(void);
static inline void send_testmuster_single_slot(int16_t *dest, int16_t data);
#endif // SPORT_ANOMALY_05000462



static inline void send_buffer_byte_raw(uint8_t *dest, int8_t *data){
    int count;
    for(count=0;count<DMA_FRAMES_PER_MS;++count){
        dest[count*DISTANZ_BYTE] = data[count];
    }
}

static inline void receive_buffer_byte_raw(uint8_t *src, int8_t *data){
    int count;
    for(count=0;count<DMA_FRAMES_PER_MS;++count)
           data[count] = src[count*DISTANZ_BYTE];
}

static inline void send_buffer_word(int16_t *dest, int16_t *data, uint16_t odd_dist){
    int count;
    for(count=0;count<DMA_FRAMES_PER_MS*2;++count){
        if(count%2){
            dest[((count-1)*DISTANZ_WORD)+odd_dist] = data[count];
        }else{
            dest[count*DISTANZ_WORD] = data[count];
        }
    }
}

static inline void receive_buffer_word(int16_t *src, int16_t *data, uint16_t odd_dist){
    int count;
    for(count=0;count<DMA_FRAMES_PER_MS*2;++count){
        if(count%2){
            data[count] = src[((count-1)*DISTANZ_WORD)+odd_dist];
        }else{
            data[count] = src[count*DISTANZ_WORD];
        }
    }
}

static ssize_t sport_status_show(struct class *sport_class, char *buf)
{
    char *p;
    unsigned short i;
    p = buf;

    if(sport_devices) {
        for (i = 0; i < SPORT_TDM_NR_DEVS; i++)
            p += sprintf(p,
                "sport%d:\nrx_irq=%d, rx_received=%d, tx_irq=%d, tx_sent=%d,\n"
                "mode=%d, channels=%d, data_format=%d, word_len=%d.\n",
                i, sport_devices[i].rx_irq, sport_devices[i].rx_received,
                sport_devices[i].tx_irq, sport_devices[i].tx_sent,
                sport_devices[i].config.mode, sport_devices[i].config.channels,
                sport_devices[i].config.data_format, sport_devices[i].config.word_len);
    }
    return p - buf;
}

static CLASS_ATTR(status, S_IRUGO, &sport_status_show, NULL);

#define DMA_COUNT_NO_OVERFLOW      10000 // 10s

#ifdef SPORT_SEND_TEST_TONE
static const char test_ton_400Hz_alaw[] = {
        0xD5, 0x86, 0xB7, 0xBC, 0xBB, 0xA5, 0xBB, 0xBC,
        0xB7, 0x86, 0xD5, 0x06, 0x37, 0x3C, 0x3B, 0x25,
        0x3B, 0x3C, 0x37, 0x06, 0x55, 0x86, 0xB7, 0xBC,
        0xBB, 0xA5, 0xBB, 0xBC, 0xB7, 0x86, 0xD5, 0x06,
        0x37, 0x3C, 0x3B, 0x25, 0x3B, 0x3C, 0x37, 0x06
};
static int test_ton_counter=0;
#endif // SPORT_SEND_TEST_TONE

// DMA IRQ Service Routine; Inner Loop
static irqreturn_t dma_irq_handler(int irq, void *dev_id)
{
    struct sport_dev *dev = dev_id;
    int16_t     rx_chan = dev->dma_rx_chan;                             // DMA channel no
    int16_t     *rx_buf;
    int16_t     *tx_buf;
    int16_t     *curr_rx_addr = (uint16_t *)get_dma_curr_addr(rx_chan);
    int16_t     count;
    int16_t     chan_count;
    static uint16_t     dma_count __attribute__ ((l1_data)) = 1;

#ifdef  SPORT_ANOMALY_05000462
    // check anomaly every 1000 interrupts (1000*1ms=1s)
    if(!(dma_count%1000))
        sport_check_anomaly();
#endif  // SPORT_ANOMALY_05000462

    /* Calculate current DMA-Buffer address */
    /* TODO: Hier wird der aktive Buffer des Doppelbuffers berechnet und es muss die entsprechende Funktion aufgerufen
     * werden die den aktiven Dahdi Buffer umschaltet
     */
    if( (((unsigned long)curr_rx_addr - (unsigned long)dma_rx_buf) / DMA_BUF_BYTE_LEN) % 2  ){
        // DMA == Buffer 1 => Transfer == Buffer 0
        rx_buf = dma_rx_buf;
        tx_buf = dma_tx_buf;
        // TODO: Aktiver Dahdi Doppel-Buffer == 1
    }else{
        // DMA == Buffer 0 => Transfer == Buffer 1
        rx_buf = dma_rx_buf+DMA_BUF_WORD_LEN;
        tx_buf = dma_tx_buf+DMA_BUF_WORD_LEN;
        // TODO: Aktiver Dahdi Doppel-Buffer == 0
    }

    clear_dma_irqstat(rx_chan);

    ++dma_count;

    if(dma_count > DMA_COUNT_NO_OVERFLOW){
        dma_count -= DMA_COUNT_NO_OVERFLOW;
    }


#ifdef SPORT_ANOMALY_05000462
    send_testmuster_single_slot(&tx_buf[SPORT_TEST_MUSTER_OFFSET], SPORT_TEST_MUSTER);
#endif

    // for all time slots: 
    for(chan_count=0; chan_count<KF_CHANNELS_MAX; ++chan_count){
        t_tdm    *dest = &phys_tdm[chan_count];

        // only used channels
        if(dest->mode != e_kf_mode_unused){
            if(dest->mode == e_kf_mode_8kHz){
                /*
                 * Active 8 kHz timeslot
                 */
                // copy samples from DMA to rx buffer
#ifndef NO_BRI_MOD
	      receive_buffer_byte_raw((uint8_t *)rx_buf + (chan_count^1), dest->dahdi_rx_chunk);
#endif

#ifdef SPORT_TEST_LOOPBACK
                for(count=0;count<DMA_FRAMES_PER_MS;++count){
                    test_io_tx_buf[chan_count][count] = test_io_rx_buf[chan_count][count];
                }
#endif

#ifdef SPORT_SEND_TEST_TONE
                if(chan_count == 1){    // only one channel of 0 or 1 (BRI module), 4 or 5 (CP3000 ISDN)
                    for(count=0;count<DMA_FRAMES_PER_MS;++count){
                        test_io_tx_buf[chan_count][count] = test_ton_400Hz_alaw[(test_ton_counter+count)%sizeof(test_ton_400Hz_alaw)];
                    }
                }
#endif
            }else{

                /*
                 *  Active 16 kHz timeslot
                 */
                int16_t phys_chan = chan_count >> 1;  // 8 * 16 Bit channels in DMA Buffer

                // TODO: reduce copy loops

                // copy samples from DMA to rx buffer
                receive_buffer_word(&rx_buf[phys_chan], conv_in_buf, dest->ts_distance);
                // 16 kHz linear > 8 kHz a-Law
                if(dest->rx_delay_line){
                    swdsp_Decimate(dest->rx_delay_line, conv_in_buf, conv_out_buf, DMA_FRAMES_PER_MS*2);
#ifndef NO_FXS_MOD
		    for(count=0;count<DMA_FRAMES_PER_MS;++count){
		      dest->dahdi_rx_chunk[count] = linear_to_alaw(conv_out_buf[count]);
		    }		    
#endif
                }

#ifdef SPORT_TEST_LOOPBACK
                for(count=0;count<DMA_FRAMES_PER_MS;++count){
                    test_io_tx_buf[chan_count][count] = test_io_rx_buf[chan_count][count];
                }
#endif

#ifdef SPORT_SEND_TEST_TONE
                if(chan_count == 14){    // only one channel of 0 or 2 (FXS 5/6 module) or 8,10,12 or 14 (FXS 1-4)
                    for(count=0;count<DMA_FRAMES_PER_MS;++count){
                        test_io_tx_buf[chan_count][count] = test_ton_400Hz_alaw[(test_ton_counter+count)%sizeof(test_ton_400Hz_alaw)];
                    }
                }
#endif
            }
        }
    }

    /*
     * perform data transmissions between dahdi and xhfc driver
     * for all spans
     * this routine also does all the d-channel signalling
     */
#ifndef NO_BRI_MOD
    xhfc_int();
#endif
    /*
     * perform voice data interchange between dahdi and FXS driver
     */
#ifndef NO_FXS_MOD
    aucp3kfxs_interrupt();
#endif

    // for all time slots
    for(chan_count=0; chan_count<KF_CHANNELS_MAX; ++chan_count){
        t_tdm    *dest = &phys_tdm[chan_count];

        // only used channels
        if(dest->mode != e_kf_mode_unused){
            if(dest->mode == e_kf_mode_8kHz){
                /*
                 * Active 8 kHz timeslot
                 */
                // copy samples from tx buffer to DMA
                /*
                 * TODO: z.Z. nur LOOP BACK und Testtoene
                 * die test_io_xx_buf muessen durch die entsprechenden _AKTIVEN_ Dahdi Buffer ersetzt werden
                 */
                send_buffer_byte_raw((uint8_t *)tx_buf + (chan_count^1), dest->dahdi_tx_chunk);
            }else{
                /*
                 *  Active 16 kHz timeslot
                 */
                int16_t phys_chan = chan_count >> 1;  // 8 * 16 Bit channels in DMA Buffer

                // 8 kHz a-Law > 16 kHz linear
                if(dest->tx_delay_line){
                    for(count=0;count<DMA_FRAMES_PER_MS;++count){
                        conv_in_buf[count] = alaw_to_linear(dest->dahdi_tx_chunk[count]);
                    }
                    swdsp_Interpolate(dest->tx_delay_line, conv_in_buf, conv_out_buf, DMA_FRAMES_PER_MS);
                }

                // copy samples from tx buffer to DMA
                send_buffer_word(&tx_buf[phys_chan], conv_out_buf, dest->ts_distance);
            }
        }
    }

#ifdef SPORT_SEND_TEST_TONE
    test_ton_counter = (test_ton_counter+DMA_FRAMES_PER_MS) % sizeof(test_ton_400Hz_alaw);
#endif

#if 0
    /* TEST TEST TEST
     * direct connect FXS 1 <> FXS 2 and FXS 3 <> FXS 4
     */
    for(count=0;count<DMA_FRAMES_PER_MS;++count){
        test_io_tx_buf[8][count] = test_io_rx_buf[10][count];
        test_io_tx_buf[10][count] = test_io_rx_buf[8][count];
        test_io_tx_buf[12][count] = test_io_rx_buf[14][count];
        test_io_tx_buf[14][count] = test_io_rx_buf[12][count];
    }
#endif

    return IRQ_HANDLED;
}


// COMpact 3000 HW/CPLD access
#define AU_CPLD_BASE   0x20200000
#define AU_CPLD_WTNK14 0
#define AU_CPLD_WTNK56 2
#define AU_CPLD_RMODID 18

#define read_moduleID  ((uint16_t volatile *)(AU_CPLD_BASE+AU_CPLD_RMODID))

static void sport_init_matrix(void){
    // check HW
    int chan_count, result;

#ifndef NO_FXS_MOD
    // init dahdi buffer pointer
    phys_tdm[8].dahdi_rx_chunk = aucp3kfxs_get_chan_rx_chunk( AUERMOD_CP3000_SLOT_AB, 0 );
    phys_tdm[8].dahdi_tx_chunk = aucp3kfxs_get_chan_tx_chunk( AUERMOD_CP3000_SLOT_AB, 0 );
    phys_tdm[10].dahdi_rx_chunk = aucp3kfxs_get_chan_rx_chunk( AUERMOD_CP3000_SLOT_AB, 1 );
    phys_tdm[10].dahdi_tx_chunk = aucp3kfxs_get_chan_tx_chunk( AUERMOD_CP3000_SLOT_AB, 1 );
    phys_tdm[12].dahdi_rx_chunk = aucp3kfxs_get_chan_rx_chunk( AUERMOD_CP3000_SLOT_AB, 2 );
    phys_tdm[12].dahdi_tx_chunk = aucp3kfxs_get_chan_tx_chunk( AUERMOD_CP3000_SLOT_AB, 2 );
    phys_tdm[14].dahdi_rx_chunk = aucp3kfxs_get_chan_rx_chunk( AUERMOD_CP3000_SLOT_AB, 3 );
    phys_tdm[14].dahdi_tx_chunk = aucp3kfxs_get_chan_tx_chunk( AUERMOD_CP3000_SLOT_AB, 3 );
#else
    phys_tdm[8].mode = e_kf_mode_unused;
    phys_tdm[8].ts_distance = 0;
    phys_tdm[10].mode = e_kf_mode_unused;
    phys_tdm[10].ts_distance = 0;
    phys_tdm[12].mode = e_kf_mode_unused;
    phys_tdm[12].ts_distance = 0;
    phys_tdm[14].mode = e_kf_mode_unused;
    phys_tdm[14].ts_distance = 0;
#endif

    // check for board type
    result = auerask_cp3k_read_hwrev();
    printk(KERN_INFO "HWID : 0x%02X\n", result);

    if((result & CP3000_HW_MASK) == CP3000_HW_ANALOG){
        printk(KERN_INFO "       Analog\n");
        // init FXO channel 4 wideband; use 4-7
        phys_tdm[4].mode = e_kf_mode_16kHz;
        phys_tdm[4].ts_distance = DISTANZ_DIREKT;
        phys_tdm[5].mode = e_kf_mode_unused;
        phys_tdm[5].ts_distance = 0;
        phys_tdm[6].mode = e_kf_mode_unused;
        phys_tdm[6].ts_distance = 0;
        phys_tdm[7].mode = e_kf_mode_unused;
        phys_tdm[7].ts_distance = 0;
    }else if((result & CP3000_HW_MASK) == CP3000_HW_ISDN){
        printk(KERN_INFO "       ISDN\n");
        // init ISDN/BRI 8 kHz channel 4  + 5
#ifndef NO_BRI_MOD
        phys_tdm[4].mode = e_kf_mode_8kHz;
        phys_tdm[4].ts_distance = 0;
        phys_tdm[4].dahdi_rx_chunk = xhfc_get_bchan_rx_chunk( AUERMOD_CP3000_SLOT_S0, 0, 0 );
        phys_tdm[4].dahdi_tx_chunk = xhfc_get_bchan_tx_chunk( AUERMOD_CP3000_SLOT_S0, 0, 0 );
        phys_tdm[5].mode = e_kf_mode_8kHz;
        phys_tdm[5].ts_distance = 0;
        phys_tdm[5].dahdi_rx_chunk = xhfc_get_bchan_rx_chunk( AUERMOD_CP3000_SLOT_S0, 0, 1 );
        phys_tdm[5].dahdi_tx_chunk = xhfc_get_bchan_tx_chunk( AUERMOD_CP3000_SLOT_S0, 0, 1 );
#endif
    }else{
        printk(KERN_INFO "       VoIP\n");
        // no additional interfaces vor CP3000 VoIP
    }

    // check for modules
    result = *read_moduleID & CP3000_MODID_MASK;
    printk(KERN_INFO "MODID: 0x%02X\n", result);

    if(result == CP3000_MODID_ISDN2){     // isdn/bri module ?
#ifndef NO_BRI_MOD
        // init B1/B2 for BRI / narrow band
        printk(KERN_INFO "       1 * BRI detected\n");
        phys_tdm[0].mode = e_kf_mode_8kHz;
        phys_tdm[0].ts_distance = 0;
        phys_tdm[0].dahdi_rx_chunk = xhfc_get_bchan_rx_chunk( AUERMOD_CP3000_SLOT_MOD, 0, 0 );
        phys_tdm[0].dahdi_tx_chunk = xhfc_get_bchan_tx_chunk( AUERMOD_CP3000_SLOT_MOD, 0, 0 );
        phys_tdm[1].mode = e_kf_mode_8kHz;
        phys_tdm[1].ts_distance = 0;
        phys_tdm[1].dahdi_rx_chunk = xhfc_get_bchan_rx_chunk( AUERMOD_CP3000_SLOT_MOD, 0, 1 );
        phys_tdm[1].dahdi_tx_chunk = xhfc_get_bchan_tx_chunk( AUERMOD_CP3000_SLOT_MOD, 0, 1 );
        phys_tdm[2].mode = e_kf_mode_unused;
        phys_tdm[2].ts_distance = 0;
        phys_tdm[3].mode = e_kf_mode_unused;
        phys_tdm[3].ts_distance = 0;
#endif
    } else if( result == CP3000_MODID_2AB){       // analog/FXS module
        printk(KERN_INFO "       2 * FXS detected (%s)\n", (result&CP3000_MODID_MOD_CODEC)?"none SSM2602":"SSM2602");

#ifndef NO_FXS_MOD
        // init FXS 5 + 6 / wideband
        phys_tdm[0].mode = e_kf_mode_16kHz;
        phys_tdm[0].ts_distance = DISTANZ_WORD;
        phys_tdm[0].dahdi_rx_chunk = aucp3kfxs_get_chan_rx_chunk( AUERMOD_CP3000_SLOT_MOD, 0 );
        phys_tdm[0].dahdi_tx_chunk = aucp3kfxs_get_chan_tx_chunk( AUERMOD_CP3000_SLOT_MOD, 0 );
        phys_tdm[1].mode = e_kf_mode_unused;
        phys_tdm[1].ts_distance = 0;
        phys_tdm[2].mode = e_kf_mode_16kHz;
        phys_tdm[2].ts_distance = DISTANZ_WORD;
        phys_tdm[2].dahdi_rx_chunk = aucp3kfxs_get_chan_rx_chunk( AUERMOD_CP3000_SLOT_MOD, 1 );
        phys_tdm[2].dahdi_tx_chunk = aucp3kfxs_get_chan_tx_chunk( AUERMOD_CP3000_SLOT_MOD, 1 );
        phys_tdm[3].mode = e_kf_mode_unused;
        phys_tdm[3].ts_distance = 0;
#endif
    }

    // get memory for 8<>16 kHz filter for all used timeslots
    for(chan_count=0; chan_count<KF_CHANNELS_MAX; ++chan_count){
        if(phys_tdm[chan_count].mode != e_kf_mode_unused){
            phys_tdm[chan_count].tx_delay_line = l1_data_sram_alloc(SWDSP_SAMPLING_NTaps*sizeof(uint16_t));
            phys_tdm[chan_count].rx_delay_line = l1_data_sram_alloc(SWDSP_SAMPLING_NTaps*sizeof(uint16_t));
            sport_tdm_debug("%02d (%s): 0x%08X 0x%08X\n", chan_count,
                                                 phys_tdm[chan_count].mode == e_kf_mode_8kHz?" 8kHz":"16kHz",
                                                 (unsigned int)phys_tdm[chan_count].tx_delay_line,
                                                 (unsigned int)phys_tdm[chan_count].rx_delay_line);
        }
    }
}

/* note: multichannel is in units of 8 channels, tdm_count is # channels NOT / 8 ! */
static int sport_set_multichannel(struct sport_register *regs,
                    int tdm_count, int packed, int frame_delay)
{
    if (tdm_count) {

        int shift = 32 - tdm_count;
        unsigned int mask = (0xffffffff >> shift);

        regs->mcmc1 = ((tdm_count>>3)-1) << 12;  /* set WSIZE bits */

        regs->mcmc2 = (frame_delay << 12)| MCMEN | \
                    (packed ? (MCDTXPE|MCDRXPE) : 0);

        regs->mtcs0 = mask;
        regs->mrcs0 = mask;

    } else {

        regs->mcmc1 = 0;
        regs->mcmc2 = 0;

        regs->mtcs0 = 0;
        regs->mrcs0 = 0;
    }

    regs->mtcs1 = 0; regs->mtcs2 = 0; regs->mtcs3 = 0;
    regs->mrcs1 = 0; regs->mrcs2 = 0; regs->mrcs3 = 0;

    SSYNC();

    return 0;
}

static int sport_configure(struct sport_dev *dev, struct sport_config *config, bool dma_request)
{
    // support only DMA

    unsigned int tcr1,tcr2,rcr1,rcr2;
    unsigned int clkdiv, fsdiv;

    tcr1=tcr2=rcr1=rcr2=0;
    clkdiv = fsdiv =0;

    // setup DMA only if requested
    if(dma_request){
        /* request RX dma */
        if (request_dma(dev->dma_rx_chan, "sport_rx_dma_chan") < 0) {
            printk(KERN_WARNING "Unable to request sport rx dma channel\n");
            goto fail;
        }

        // DMA IRQ on RX only; TX done at same IRQ
        set_dma_callback(dev->dma_rx_chan, dma_irq_handler, dev);

        /* Request TX dma */
        if (request_dma(dev->dma_tx_chan, "sport_tx_dma_chan") < 0) {
            printk(KERN_WARNING "Unable to request sport tx dma channel\n");
            goto fail;
        }
    }

    // save config
    memcpy(&dev->config, config, sizeof(*config));

    if ((dev->regs->tcr1 & TSPEN) || (dev->regs->rcr1 & RSPEN))
        return -EBUSY;

    if (config->mode == TDM_MODE) {
        if(config->channels & 0x7 || config->channels>32)
            return -EINVAL;

        sport_set_multichannel(dev->regs, config->channels, 1, config->frame_delay);

        tcr1 |= (config->lsb_first << 4);
        rcr1 |= (config->lsb_first << 4);

    } else if (config->mode == I2S_MODE) {
        tcr1 |= (TCKFE | TFSR);
        tcr2 |= TSFSE ;

        rcr1 |= (RCKFE | RFSR);
        rcr2 |= RSFSE;
    } else {
        tcr1 |= (config->lsb_first << 4) | (config->fsync << 10) | \
            (config->data_indep << 11) | (config->act_low << 12) | \
            (config->late_fsync << 13) | (config->tckfe << 14) ;
        if (config->sec_en)
            tcr2 |= TXSE;

        rcr1 |= (config->lsb_first << 4) | (config->fsync << 10) | \
            (config->data_indep << 11) | (config->act_low << 12) | \
            (config->late_fsync << 13) | (config->tckfe << 14) ;
        if (config->sec_en)
            rcr2 |= RXSE;
    }

    /* Using internal clock*/
    if (config->int_clk) {
        u_long sclk=get_sclk();

        if (config->serial_clk < 0 || config->serial_clk > sclk/2)
            return -EINVAL;
        clkdiv = sclk/(2*config->serial_clk) - 1;
        fsdiv = config->serial_clk / config->fsync_clk - 1;

        tcr1 |= (ITCLK | ITFS);
        rcr1 |= (IRCLK | IRFS);
    }

    /* Setting data format */
    tcr1 |= (config->data_format << 2); /* Bit TDTYPE */
    rcr1 |= (config->data_format << 2); /* Bit TDTYPE */
    if (config->word_len >= 3 && config->word_len <= 32) {
        tcr2 |= config->word_len - 1;
        rcr2 |= config->word_len - 1;
    } else
        return -EINVAL;

    dev->regs->rcr1 = rcr1;
    dev->regs->rcr2 = rcr2;
    dev->regs->rclkdiv = clkdiv;
    dev->regs->rfsdiv = fsdiv;
    dev->regs->tcr1 = tcr1;
    dev->regs->tcr2 = tcr2;
    dev->regs->tclkdiv = clkdiv;
    dev->regs->tfsdiv = fsdiv;
    SSYNC();

    return 0;

fail:
    return -1;
}

// no. of bytes per word
static inline uint16_t sport_wordbytes(int16_t word_len)
{
    // nur 2 / 4 Byte Uebertragung in den Buffer moeglich
    if ((word_len > 3) && (word_len <= 16)) {
        return 2;
    } else if (word_len <= 32) {
        return 4;
    }
    return 0;
}

// DMA transfer size Config.Register
static inline uint16_t sport_wordsize(int16_t word_len)
{
    if (word_len <= 16) {
        return WDSIZE_16;
    } else if (word_len <=32) {
        return WDSIZE_32;
    }
    return 0;
}


static void sport_stop_dma(struct sport_dev *dev){
    // DMA off
    disable_dma(dev->dma_rx_chan);
    disable_dma(dev->dma_tx_chan);
}

static void sport_start_dma(struct sport_dev *dev, int16_t cfg_word_len){
    uint16_t    dma_config;
    uint16_t    dma_word_size = sport_wordsize(cfg_word_len);
    uint16_t    dma_word_bytes = sport_wordbytes(cfg_word_len);
    uint16_t    xcount = DMA_BUF_WORD_LEN;
    uint16_t    ycount = DMA_BUF_COUNT;

    // DMA off
    sport_stop_dma(dev);

    // TX
    set_dma_start_addr(dev->dma_tx_chan, (unsigned long)dma_tx_buf);
    set_dma_x_count(dev->dma_tx_chan, xcount);
    set_dma_x_modify(dev->dma_tx_chan, dma_word_bytes);
    set_dma_y_count(dev->dma_tx_chan, ycount);
    set_dma_y_modify(dev->dma_tx_chan, dma_word_bytes);
    //auto buffer + two dimensional + row interrupt + 16 Bit
    dma_config = (DMAFLOW_AUTO | dma_word_size | DMA2D);
    set_dma_config(dev->dma_tx_chan, dma_config);

    // RX
    set_dma_start_addr(dev->dma_rx_chan, (unsigned long)dma_rx_buf);
    set_dma_x_count(dev->dma_rx_chan, xcount);
    set_dma_x_modify(dev->dma_rx_chan, dma_word_bytes);
    set_dma_y_count(dev->dma_rx_chan, ycount);
    set_dma_y_modify(dev->dma_rx_chan, dma_word_bytes);
    //auto buffer + two dimensional + row interrupt + 16 Bit
    dma_config = (DMAFLOW_AUTO | dma_word_size | DMA2D | DI_EN  | DI_SEL | WNR);
    set_dma_config(dev->dma_rx_chan, dma_config);

    enable_dma(dev->dma_tx_chan);
    enable_dma(dev->dma_rx_chan);
}

static void sport_cleanup_module(void)
{
    int     chan_count;
    dev_t devno = MKDEV(sport_major, sport_minor);

    sport_tdm_debug("\n");

    if (sport_devices) {
        // sport Rx/Tx off
        sport_tdm_debug("SPORT RX/TX off\n");
        sport_devices[0].regs->tcr1 &= ~TSPEN;
        sport_devices[0].regs->rcr1 &= ~RSPEN;

        // DMA off
        sport_tdm_debug("DMA off\n");
        sport_stop_dma(&sport_devices[0]);

        // DMA free
        sport_tdm_debug("DMA free\n");
        free_dma(sport_devices[0].dma_rx_chan);
        free_dma(sport_devices[0].dma_tx_chan);

        /* free DMA Buffer */
        if(dma_tx_buf){
            sport_tdm_debug("DMA TX buffer free\n");
            l1_data_sram_free(dma_tx_buf);
        }
        if(dma_rx_buf){
            sport_tdm_debug("DMA RX buffer free\n");
            l1_data_sram_free(dma_rx_buf);
        }
        /* free conv buffer */
        if(conv_in_buf){
            sport_tdm_debug("conv_in_buf free\n");
            l1_data_sram_free(conv_in_buf);
        }
        if(conv_out_buf){
            sport_tdm_debug("conv_out_buf free\n");
            l1_data_sram_free(conv_out_buf);
        }

        // GPIO free
        sport_tdm_debug("GPIO free\n");
        peripheral_free_list(bfin_char_pin_req_sport);

        // cleanup device and class
        sport_tdm_debug("cdev_del\n");
        cdev_del(&sport_devices[0].cdev);

        sport_tdm_debug("device_destroy\n");
        device_destroy(sport_class, MKDEV(sport_major, 0));

        sport_tdm_debug("class_destroy\n");
        class_destroy(sport_class);

        // free device
        sport_tdm_debug("sport_devices free\n");
        kfree(sport_devices);

        sport_tdm_debug("unregister_chrdev_region\n");
        unregister_chrdev_region(devno, SPORT_TDM_NR_DEVS);

        // free memory for 8<>16 kHz filter for all used timeslots
        for(chan_count=0; chan_count<KF_CHANNELS_MAX; ++chan_count){
            if(phys_tdm[chan_count].mode != e_kf_mode_unused){

                sport_tdm_debug("%02d (%s): 0x%08X 0x%08X free\n", chan_count,
                                                     phys_tdm[chan_count].mode == e_kf_mode_8kHz?" 8kHz":"16kHz",
                                                     (unsigned int)phys_tdm[chan_count].tx_delay_line,
                                                     (unsigned int)phys_tdm[chan_count].rx_delay_line);

                if(phys_tdm[chan_count].rx_delay_line){
                    l1_data_sram_free(phys_tdm[chan_count].rx_delay_line);
                    phys_tdm[chan_count].rx_delay_line = NULL;

                }
                if(phys_tdm[chan_count].tx_delay_line){
                    l1_data_sram_free(phys_tdm[chan_count].tx_delay_line);
                    phys_tdm[chan_count].tx_delay_line = NULL;
                }
            }
        }
    }

    auerask_cp3k_spi_exit();
}


static void sport_register_init(struct sport_dev *dev, bool dma_request){

    struct sport_config config;

    // Default Config 16 TS with 16 Bit; linear; FSync == TS0/Bit0
    config.mode = TDM_MODE;
    config.channels = 16;               /* no. channels */
    config.word_len = DMA_TS_BIT_LEN;    /* 16 Bit */
    config.frame_delay = 0;             /* Delay between frame sync pulse and first bit */
    config.right_first = 0;             /* I2S mode only */
    config.lsb_first = 0;               /* order of transmit or receive data */
    config.fsync = 1;                   /* Frame sync required */
    config.data_indep = 0;              /* data independent frame sync generated */
    config.act_low = 0;                 /* Active low TFS */
    config.late_fsync = 0;              /* Late frame sync */
    config.tckfe = 0;
    config.sec_en = 0;                  /* Secondary side enabled */
    config.int_clk = 0;                 /* External clock */
    config.serial_clk = 0;              /* If external clock is used, the following fields are ignored */
    config.fsync_clk = 0;
    config.data_format = NORM_FORMAT;   /* lineare Samples*/
    config.dma_enabled = 1;             /* DMA on == IRQ off */

    // base init.
    sport_configure(dev, &config, dma_request);

}


static struct file_operations sport_fops = {
    .owner =    THIS_MODULE,
    .read =     NULL,
    .write =    NULL,
    .ioctl =    NULL,
    .open =     NULL,
    .release =  NULL,
};


static void sport_setup_cdev(struct sport_dev *dev, int index)
{
    int err, devno = MKDEV(sport_major, sport_minor + index);

    cdev_init(&dev->cdev, &sport_fops);
    dev->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
        printk(KERN_NOTICE "Error %d adding sport%d", err, index);
}


/*****************************************************************************
 * INIT
 ****************************************************************************/
static int __init sport_init_module(void)
{
    int     result;
    char    minor_name[32];
    dev_t   dev = 0;
    int     minor = 0;


    auerask_cp3k_spi_init();
    
    sport_tdm_debug("\n");

    sport_tdm_debug("register_chrdev_region\n");
    dev = MKDEV(sport_major, sport_minor);
    result = register_chrdev_region(dev, SPORT_TDM_NR_DEVS, "sport_tdm");

    if (result < 0) {
        printk(KERN_WARNING "sport: can't get major %d\n", sport_major);
        return result;
    }

    // SPORT Devices Handle
    sport_devices = kmalloc(SPORT_TDM_NR_DEVS * sizeof(struct sport_dev), GFP_KERNEL);
    if (!sport_devices) {
        result = -ENOMEM;
        goto fail;
    }
    memset(sport_devices, 0, SPORT_TDM_NR_DEVS * sizeof(struct sport_dev));

    sport_class = class_create(THIS_MODULE, "sport_tdm");
    if ((result=class_create_file(sport_class, &class_attr_status)) != 0) {
        sport_cleanup_module();
        return result;
    }

    sprintf(minor_name, "sport_tdm%d", minor);
    device_create(sport_class, NULL, MKDEV(sport_major, minor), minor_name);

    sport_setup_cdev(&sport_devices[0], 0);
    sport_devices[0].sport_num = 0;

    // register GPIO
    if (peripheral_request_list(bfin_char_pin_req_sport, DRV_NAME)) {
            goto fail;
    }


#ifdef SPORT_ANOMALY_05000462
    // add. GPIO
    bfin_write_PORTGIO_INEN(bfin_read_PORTGIO_INEN()|SPORT_GPIO_MASK);
#endif

    sport_devices[0].regs = (struct sport_register *) SPORT0_TCR1;
    sport_devices[0].dma_rx_chan = CH_SPORT0_RX;
    sport_devices[0].dma_tx_chan = CH_SPORT0_TX;
    sport_devices[0].sport_err_irq = IRQ_SPORT0_ERROR;

    // base register init.
    sport_register_init(&sport_devices[0], true);

    // Allocate L1 buffers
    // DMA double buffer
    dma_rx_buf = l1_data_sram_alloc(DMA_BUF_BYTE_LEN*DMA_BUF_COUNT);
    dma_tx_buf = l1_data_sram_alloc(DMA_BUF_BYTE_LEN*DMA_BUF_COUNT);

    // 8<>16 kHz conversion buffer
    conv_in_buf = l1_data_sram_alloc(DMA_FRAMES_PER_MS*2*sizeof(int16_t));  // 16 kHz * int16_t
    conv_out_buf = l1_data_sram_alloc(DMA_FRAMES_PER_MS*2*sizeof(int16_t));  // 16 kHz * int16_t

    if (!dma_tx_buf || !dma_rx_buf || !conv_in_buf || !conv_out_buf){
        result = -ENOMEM;

        goto fail; /*No room in L1*/
    }

    // init. for send silence on all channels
    #define SPORT_TDM_SILENCE       -1
    memset(dma_tx_buf, SPORT_TDM_SILENCE, DMA_BUF_BYTE_LEN*DMA_BUF_COUNT);
    memset(test_io_rx_buf, SPORT_TDM_SILENCE, KF_CHANNELS_MAX*DMA_FRAMES_PER_MS);
    memset(test_io_tx_buf, SPORT_TDM_SILENCE, KF_CHANNELS_MAX*DMA_FRAMES_PER_MS);

    // bitrate matrix init.
    sport_init_matrix();

    // DMA start
    sport_start_dma(&sport_devices[0], DMA_TS_BIT_LEN);

    /* SPORT start RX+TX */
    sport_devices[0].regs->tcr1 |= TSPEN;
    sport_devices[0].regs->rcr1 |= RSPEN;

    return 0; /* succeed */

fail:
    sport_cleanup_module();
    return result;

}

/* Anomaly 05000462
 *
 * like described in the anomaly we recognize a time shift of the TDM transmit path in some situations on the running interface
 * after some minutes/hours. The shift can not be recognized on the sport interface registers.
 *
 * Workaround:
 * 1) Sending a static test signal on an unused channel.
 * 2) Reading this signal periodical / every x seconds.
 * 3) There are no direct way to read back sport transmit signal. The sport transmit pin is also used as GPIO input.
 * 4) We read this input bit by bit and reinit. SPORT/DMA if signal is not the test signal.
*/

#ifdef SPORT_ANOMALY_05000462
// Workaround Anomaly
static int sport_reinit_counter = 0;

static void sport_reinit(void){

    ++sport_reinit_counter;

    sport_tdm_debug(":%d\n", sport_reinit_counter);

    printk( KERN_WARNING "bfin_sport_tdm 0 reinit %d times\n", sport_reinit_counter);

    if (sport_devices) {
        // Sport Rx/Tx off
        sport_devices[0].regs->tcr1 &= ~TSPEN;
        sport_devices[0].regs->rcr1 &= ~RSPEN;

        // DMA off
        sport_stop_dma(&sport_devices[0]);

        // SPORT Register reinit.; no DMA Request
        sport_register_init(&sport_devices[0], false);

        // DMA start
        sport_start_dma(&sport_devices[0], DMA_TS_BIT_LEN);

        // Sport Rx/Tx on
        sport_devices[0].regs->tcr1 |= TSPEN;
        sport_devices[0].regs->rcr1 |= RSPEN;
    }
}


static inline void sport_check_anomaly(void){
    uint16_t bit_count = SPORT_TEST_MUSTER_BIT_OFFSET;
    uint16_t test_muster = 0;
    unsigned long     flags;

    local_irq_save(flags);

    // await FS off
    while(!(bfin_read_PORTGIO()&SPORT_GPIO_FS));
    // await FS on; start FS
    while(bfin_read_PORTGIO()&SPORT_GPIO_FS);

    // Count bits to test time slot
    while(bit_count){
        // await BCLK off
        while(bfin_read_PORTGIO()&SPORT_GPIO_BCLK);
        --bit_count;
        // awati BCLK on; start of bit
        while(!(bfin_read_PORTGIO()&SPORT_GPIO_BCLK));
    }

    // Start of test time slot reached
    bit_count = DMA_TS_BIT_LEN;

    while(bit_count){
        // await falling edge of BCLK
        while(bfin_read_PORTGIO()&SPORT_GPIO_BCLK);
        --bit_count;
        test_muster <<= 1;

        // detect bit value
        if(bfin_read_PORTGIO()&SPORT_GPIO_TX){
            test_muster |= 1;
        }
        //  await rising edge of BCLK
        while(!(bfin_read_PORTGIO()&SPORT_GPIO_BCLK));
    }

    // time shift occured???
    if(test_muster != SPORT_TEST_MUSTER){
        sport_reinit();
    }

    local_irq_restore(flags);

}


static inline void send_testmuster_single_slot(int16_t *dest, int16_t data){
    int count;
    for(count=0;count<DMA_FRAMES_PER_MS;++count){
        dest[count*DISTANZ_WORD*2] = data;
    }
}

#endif // SPORT_ANOMALY_05000462




// filter 8 <> 16 kHz
static inline int Interpolate(int TapsPhase, const int16_t *const Koeff,
       int16_t *Delay, int Len, int16_t *In, int16_t *Out)
{
    int tap, num_out, phase_num;
    const int16_t *p_coeff;
    long sum;

    num_out = 0;
    while (--Len >= 0) {
        /* shift delay line up to make room for next sample */
        for (tap = TapsPhase - 1; tap > 0; tap--) {
            Delay[tap] = Delay[tap - 1];
        }

        /* copy next sample from input buffer to bottom of delay line */
        Delay[0] = *In++;

        /* calculate outputs */
        for (phase_num = 0; phase_num < 2; phase_num++) {
            /* point to the current polyphase filter */
            p_coeff = Koeff + phase_num;

            /* calculate FIR sum */
            sum = 0;
            for (tap = 0; tap < TapsPhase; tap++) {
                sum += *p_coeff * Delay[tap] *2;
                p_coeff += 2;          /* point to next coefficient */
            }
        sum = sum >>15;
        /* Saturate accumulated filter result */
        if (sum>0x7fff) {
        sum = 0x7fff;
        Saturate++;
        }
        else if (sum<-0x8000) {
        sum = -0x8000;
        Saturate++;
        }
            *Out++ = sum;     /* store sum and point to next output */
            num_out++;
        }
    }
    return(num_out);   /* pass number of outputs back to caller */
}

static inline int Decimate(int Taps, const int16_t *const Koeff,
      int16_t *Delay, int Len, int16_t *In, int16_t *Out)
{
    int tap, num_out;
    long sum;

    num_out = 0;
    while (Len >= 2) {
        /* shift delay line up to make room for next samples */
        for (tap = Taps - 1; tap >= 2; tap--) {
            Delay[tap] = Delay[tap - 2];
        }

        /* copy next samples from input buffer to bottom of delay line */
        for (tap = 1; tap >= 0; tap--) {
            Delay[tap] = *In++;
        }
        Len -= 2;

        /* calculate FIR sum */
        sum = 0.0;
        for (tap = 0; tap < Taps; tap++) {
            sum += Koeff[tap] * Delay[tap];
        }
    sum = sum >>15;
    /* Saturate accumulated filter result */
    if (sum>0x7fff) {
        sum = 0x7fff;
        Saturate++;
        }
    else if (sum<-0x8000) {
        sum = -0x8000;
        Saturate++;
        }
        *Out++ = sum;     /* store sum and point to next output */
        num_out++;
    }

    return num_out;   /* pass number of outputs back to caller */
}


module_init(sport_init_module);
module_exit(sport_cleanup_module);

MODULE_AUTHOR("Roy Huang <roy.huang@analog.com> / <info@auerswald.de>");
MODULE_DESCRIPTION("Special sport tdm driver for blackfin");
MODULE_LICENSE("GPL");
