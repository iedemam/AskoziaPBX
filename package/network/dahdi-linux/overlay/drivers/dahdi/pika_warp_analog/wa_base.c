/* 
 * PIKA WARP FXS/FXO DAHDI Compliant Drivers
 * Provides support to run standard Asterisk on WARP for Analog channels
 * Copyright (C) 2009 PIKA Technologies, Canada
 * All rights reserved.
 */

#include <linux/version.h>

#ifndef VERSION_CODE
#  define VERSION_CODE(vers,rel,seq) ( ((vers)<<16) | ((rel)<<8) | (seq) )
#endif

#if LINUX_VERSION_CODE < VERSION_CODE(2,4,5)
#  error "This kernel is too old: not supported by this file"
#endif

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/moduleparam.h>
#include <linux/of_platform.h>
#include <asm/io.h>

#include <dahdi/kernel.h>
#include <dahdi/wctdm_user.h>

#include "driver.h"
#include "daytona.h"
#include "wa_parms.h"
#include "wa_devext.h"
#include "wa_dma.h"
#include "wa_common.h"
#include "fxo_modes.h"

/* undefine this to go back to dahdi 2.3.x series */
#define DAHDI_2_4_0		1

/* common defines */
#define HOOK_STATE_DEBOUNCE_TIME_DEFAULT \
				PKH_BOARD_HOOK_STATE_DEBOUNCE_TIME_DEFAULT

#define THRESHOLD_DETECT_DEBOUNCE_TIME_DEFAULT \
				PKH_BOARD_THRESHOLD_DETECT_DEBOUNCE_TIME_DEFAULT

#define POLARITY_XOR(card) ((reversepolarity!=0) ^ \
				(wa->fxs_reversepolarity[card]!=0) ^ \
				(wa->fxs_vmwi_lrev[card]!=0))

#define LINE_STATE_REVERSE_MASK	0x04


/* module variables */
static int warpalg_count_priv = 0;
static struct warp_analog *warpalg_priv[MAX_MODULE_COUNT] = { NULL, NULL };
static int _opermode = -1;
static int reversepolarity = 0;

/* default values for the kernel module parameters */
static int fxs_rxgain = 0;
static int fxs_txgain = 0;
static int fxo_rxgain = 0;
static int fxo_txgain = 0;
static int alawoverride = 0;
static char *opermode = "FCC";

static int dahdi_warp_get_line_count(int port);
static int dahdi_warp_get_module_mask(int port);
const char *dahdi_warp_event_to_text(unsigned int event_code);

int warp_pft_init(PDEVICE_EXTENSION pdx);

int daytona_phone_set_linefeed(PDEVICE_EXTENSION pdx, int phone, int linefeed);

/* functions exported from warp_shared.ko */
void do_phones(PDEVICE_EXTENSION pdx, unsigned iccr);
void do_trunks(PDEVICE_EXTENSION pdx, unsigned iccr);

/* our function to take care of timer events */
void do_timers(PDEVICE_EXTENSION pdx);



/* function headers */
static int dahdi_warp_analog_open(struct dahdi_chan *chan);
static int dahdi_warp_analog_close(struct dahdi_chan *chan);
static int dahdi_warp_analog_ioctl(struct dahdi_chan *chan, unsigned int cmd, unsigned long data);
static int dahdi_warp_analog_watchdog(struct dahdi_span *span, int event);
static int dahdi_warp_analog_hooksig(struct dahdi_chan *chan, enum dahdi_txsig txsig);

/* warp analog span ops struct for DAHDI 2.4.0 and beyond */
#if defined(DAHDI_2_4_0)
static const struct dahdi_span_ops warpalg_span_ops = {
	.owner = THIS_MODULE,
	.open = dahdi_warp_analog_open,
	.close = dahdi_warp_analog_close,
	.ioctl = dahdi_warp_analog_ioctl,
	.watchdog = dahdi_warp_analog_watchdog,
	.hooksig = dahdi_warp_analog_hooksig,
};
#endif

/* allocate a new analog device struct */
static struct warp_analog *dahdi_warp_analog_device_alloc(void)
{
	struct warp_analog *wa = NULL;

	if (warpalg_count_priv < MAX_MODULE_COUNT) {
		if ((wa = kzalloc(sizeof(struct warp_analog), GFP_KERNEL))) { 
			warpalg_priv[warpalg_count_priv] = wa;
			warpalg_count_priv++;
		}
	}

	return (wa);
}

/* return pointer to the device struct with a given device_id (index) */
static struct warp_analog *dahdi_warp_analog_devptr(int device_id)
{
	struct warp_analog *wa = NULL;

	if ((device_id < 0) || (device_id >= MAX_MODULE_COUNT)) {
		printk(KERN_ERR "%s() device_id %d is out of bounds\n",
			 __FUNCTION__, device_id);
	} else {
		wa = warpalg_priv[device_id];
	}

	return  (wa);
}

/* return the number of devices */
static int dahdi_warp_analog_devcount(void)
{
	return warpalg_count_priv;
}

/* get the first timeslot for a given port */
static int dahdi_warp_analog_get_first_timeslot(struct warp_analog *wa)
{
	int result = -1;

	if (wa->module_port & MODULE_PORT_A) {
		result = 2;
	} else if (wa->module_port &  MODULE_PORT_B) {
		result = 6;
	} else if (wa->module_port & MODULE_INTERNAL) {
		/* valid choice but handled separately */
		result = -1;
	} else {
		printk(KERN_ERR "%s() : invalid module port %d\n",
			__FUNCTION__, wa->module_port);
	}

	return (result);
}

/* prepare data from asterisk for transmission (put it out on the buffer) */
void dahdi_warp_analog_transmitprep(unsigned char *tx_addr)
{
	struct warp_analog *wa;
	unsigned int base_addr;
	int ii, dd, cc, ts_offset;
	unsigned char *wr_buffer;

	/* for all active devices, perform the work */
	for (dd = 0; dd < dahdi_warp_analog_devcount(); dd++) {

		/* go through the active audio channels */
		wa = dahdi_warp_analog_devptr(dd);

		/* quick check (just in case) */
		if (!wa) {
			printk(KERN_ERR "%s() received invalid pointer\n",
				__FUNCTION__); 
			continue;
		}

		/* compute transmission (get buffer from Asterisk) */
		dahdi_transmit(&wa->span);

		/* write data to DMA buffer */
		for (ii = 0; ii < DAHDI_CHUNKSIZE; ii++) {
			base_addr = ii * FRAMESIZE;
			for (cc = 0 ; cc < wa->num_chans; cc++) {
				wr_buffer = &wa->chans[cc]->writechunk[0];
				ts_offset = wa->chan_timeslot[cc];
				tx_addr[base_addr + ts_offset] = wr_buffer[ii];
			}
		}
	}
}

/* prepare incoming audio data to be received by asterisk */
void dahdi_warp_analog_receiveprep(unsigned char *rx_addr)
{
	struct warp_analog *wa;
	unsigned int base_addr;
	unsigned int ts_offset;
	int ii, dd, cc;
	unsigned char *rd_buffer;

	/* for all active devices, perform the work */
	for (dd = 0; dd < dahdi_warp_analog_devcount(); dd++) {

		/* go through the active audio channels */
		wa = dahdi_warp_analog_devptr(dd);

		/* quick check (just in case) */
		if (!wa) {
			printk(KERN_ERR "%s() received invalid pointer\n",
				__FUNCTION__); 
			continue;
		}

		/* read data from DMA buffer */
		for (ii = 0; ii < DAHDI_CHUNKSIZE; ii++) {
			base_addr = ii * FRAMESIZE;
			for (cc = 0 ; cc < wa->num_chans; cc++) {
				rd_buffer = &wa->chans[cc]->readchunk[0];
				ts_offset = wa->chan_timeslot[cc];
				rd_buffer[ii] = rx_addr[base_addr + ts_offset];
			}
		}

		/* this is where echo cancellation is done 
		* with the registered echo canceller 
		*/
		dahdi_ec_span(&wa->span);

		/* push data into asterisk */
		dahdi_receive(&wa->span);
	}
}


/* function to open a new span */
static int dahdi_warp_analog_open(struct dahdi_chan *chan)
{
	struct warp_analog *wa = chan->pvt;

	if (!wa) {
		return -ENODEV;
	}

	if (!(wa->chan_mask & (1 << (chan->chanpos - 1)))) {
		return -ENODEV;
	}

	if (wa->removed) {
		return -ENODEV;
	}

	wa->usecount++;
	try_module_get(THIS_MODULE);
	
	return 0;
}

/* function to release all -- to be called at the very end */
static void dahdi_warp_analog_release(struct warp_analog *wa)
{
        dahdi_unregister(&wa->span);

	/* when this function is called, 
	 * all the spans should have been freed
	 * (zero reference count) */
        kfree(wa);
}

/* function to close a channel */
static int dahdi_warp_analog_close(struct dahdi_chan *chan)
{
	struct warp_analog *wa = (struct warp_analog*)chan->pvt;
	int direction;

	wa->usecount--;
	module_put(THIS_MODULE);

	/* set the channel to forward active or reverse active mode */
	if (wa->module_type == MODULE_TYPE_FXS) {

		if (wa->fxs_reversepolarity[chan->chanpos-1]) 
			direction = LINE_STATE_REVERSE_ACTIVE;
		else 
			direction = LINE_STATE_FORWARD_ACTIVE;

		daytona_phone_set_linefeed(wa->devext, 
					  (chan->chanpos-1),
					   direction);
	}

	/* if we're no more, it's time to go. set the "removed" flag */
	if ((wa->usecount == 0) && (wa->removed)) 
		dahdi_warp_analog_release(wa);

	return 0;
}

/* function to handle fxo hook signaling */
static int dahdi_warp_analog_fxo_hooksig(struct dahdi_chan *chan, enum dahdi_txsig txsig)
{
	struct warp_analog *wa = (struct warp_analog*)chan->pvt;
	trunk_hookswitch_t trunk_hookcmd;
	int retval = -1;

	switch (txsig) {

	case DAHDI_TXSIG_START:
	case DAHDI_TXSIG_OFFHOOK:
		trunk_hookcmd.trunk = chan->chanpos - 1;
		trunk_hookcmd.state = TRUNK_GO_OFFHOOK;
		retval = daytona_hook_switch(wa->devext, &trunk_hookcmd);
		break;

	case DAHDI_TXSIG_ONHOOK:
		trunk_hookcmd.trunk = chan->chanpos - 1;
		trunk_hookcmd.state = TRUNK_GO_ONHOOK;
		retval = daytona_hook_switch(wa->devext, &trunk_hookcmd);
		break;

	default:
		printk(KERN_DEBUG "%s() : unknown dahdi event code 0x%08x\n",
			__FUNCTION__, txsig);
		break;
	}

	printk(KERN_DEBUG "%s() : trunk %d state %d retval %d\n", 
		__FUNCTION__, trunk_hookcmd.trunk, trunk_hookcmd.state, retval);

	return (retval);
}

/* return the channel number for a given channel struct pointer */
static int dahdi_warp_analog_chan_index(struct dahdi_chan *chan)
{
	struct warp_analog *wa;
	int ii;

	if (chan) {
		wa = chan->pvt;
	} else {
		return -1;
	}

	if (wa) {
		for (ii = 0; ii < wa->num_chans; ii++) {
			if (chan == wa->chans[ii]) {
				return (ii);
			}
		}
	} 

	return -1;
}

#ifdef SAM_NOT_USED
static int dahdi_warp_analog_onhook_tx_state(int state)
{
	return ((state == LINE_STATE_RV_ONHOOK_TX) || \
		(state == LINE_STATE_FD_ONHOOK_TX));
}
#endif

static int dahdi_warp_analog_active_state(int state)
{
	return ((state == LINE_STATE_FORWARD_ACTIVE) || \
		(state == LINE_STATE_REVERSE_ACTIVE));
}


/* function to handle fxs hook signaling */
static int dahdi_warp_analog_fxs_hooksig(struct dahdi_chan *chan, 
					enum dahdi_txsig txsig)
{
	struct warp_analog *wa = chan->pvt;
	// SAM PDEVICE_EXTENSION pdx = wa->devext;
	int ch_num = dahdi_warp_analog_chan_index(chan);

	switch (txsig) {
	/* onhook tx */
	case DAHDI_TXSIG_ONHOOK:		
		switch (chan->sig) {
			case DAHDI_SIG_EM:
			case DAHDI_SIG_FXOKS:
			case DAHDI_SIG_FXOLS:
				wa->lasttxhook[ch_num] = wa->idletxhookstate[ch_num];
				daytona_phone_set_linefeed(wa->devext,
						(chan->chanpos-1),
						wa->lasttxhook[ch_num]);
				break;

			case DAHDI_SIG_FXOGS:
				/* set the line state to TIP Open for GS */
				daytona_phone_set_linefeed(wa->devext,
						(chan->chanpos-1),
						LINE_STATE_TIP_OPEN);
				break;
			}
			

		/* offhook tx */
		case DAHDI_TXSIG_OFFHOOK:
			switch (chan->sig) {
				case DAHDI_SIG_EM:
					daytona_phone_set_linefeed(wa->devext,
								(chan->chanpos-1),
								LINE_STATE_REVERSE_ACTIVE);
					break;
				default:
					wa->lasttxhook[ch_num] = wa->idletxhookstate[ch_num];
					daytona_phone_set_linefeed(wa->devext, (chan->chanpos-1), wa->lasttxhook[ch_num]);
					break;
			}
			break;

		/* start/ring the phone */
		case DAHDI_TXSIG_START:
			wa->lasttxhook[ch_num] = LINE_STATE_RINGING;
			daytona_phone_set_linefeed(wa->devext,
						(chan->chanpos-1),
						wa->lasttxhook[ch_num]);
			break;

		/* drop battery */
		case DAHDI_TXSIG_KEWL:
			daytona_phone_stop(wa->devext, (chan->chanpos-1));
			break;

		default:
			printk(KERN_ERR "%s(): cannot set txstate to %d\n",
				__FUNCTION__, txsig);
			break;

	}

	return 0;
}

/* depending on the type of the module, this function calls 
 * either the fxs or the fxo version 
 */
static int dahdi_warp_analog_hooksig(struct dahdi_chan *chan, 
					enum dahdi_txsig txsig)
{
	struct warp_analog *wa = (struct warp_analog*)chan->pvt;
	int retval = 0;

	switch (wa->module_type) {
		case MODULE_TYPE_FXS:
			retval = dahdi_warp_analog_fxs_hooksig(chan, txsig);
			break;
		case MODULE_TYPE_FXO:
			retval = dahdi_warp_analog_fxo_hooksig(chan, txsig);
			break;
		default:
			printk(KERN_ERR "%s() : unknown module type %d\n",
				__FUNCTION__, wa->module_type);
			retval = -1;
			break;
			
	}

	return (retval);
}

/* set the hw gain on the fxo module */
static int dahdi_warp_analog_fxo_set_hwgain(struct warp_analog *wa, int card, __s32 gain, __u32 tx)
{
	if (wa->module_type != MODULE_TYPE_FXO) {
		printk(KERN_ERR "Cannot adjust gain on non-FXO!\n");
		return -1;
	}

	if (tx) { /* tx */
		if (gain >=  -150 && gain <= 0) {
			write_silabs(wa->devext, card, 38, 16 + (gain/-10));
			write_silabs(wa->devext, card, 40, 16 + (-gain%10));
		} else if (gain <= 120 && gain > 0) {
			write_silabs(wa->devext, card, 38, gain/10);
			write_silabs(wa->devext, card, 40, (gain%10));
		} else {
			printk(KERN_ERR "FXO tx gain is out of range (%d)\n",
				gain);
			return -1;
		}
	} else { /* rx */
		if (gain >=  -150 && gain <= 0) {
			write_silabs(wa->devext, card, 39, 16+ (gain/-10));
			write_silabs(wa->devext, card, 41, 16 + (-gain%10));
		} else if (gain <= 120 && gain > 0) {
			write_silabs(wa->devext, card, 39, gain/10);
			write_silabs(wa->devext, card, 41, (gain%10));
		} else {
			printk(KERN_ERR "FXO rx gain is out of range (%d)\n",
				gain);
			return -1;
		}
	}

	return 0;
}

/* ioctl() implementation for the FXO */
static int dahdi_warp_analog_fxo_ioctl(struct dahdi_chan *chan, unsigned int cmd, unsigned long data)
{
	struct warp_analog *wa = (struct warp_analog*)chan->pvt;
	struct wctdm_echo_coefs echoregs;
	struct dahdi_hwgain hwgain;
	struct wctdm_stats stats;
	struct wctdm_regop regop;
	struct wctdm_regs regs;
	unsigned char regval;
	unsigned chan_pos;
	int x;

	if (wa->module_type != MODULE_TYPE_FXO) {
		printk(KERN_ERR "%s() : called for wrong module type %d\n",
			__FUNCTION__, wa->module_type);
		return -ENODEV;
	}

	switch (cmd) {

	case WCTDM_GET_STATS:

		read_silabs(wa->devext, chan->chanpos - 1, 29, &regval);
		stats.tipvolt = regval * 1000;
		stats.ringvolt = regval * 1000;
		stats.batvolt = regval * 1000;
		if (copy_to_user((__user void *)data, &stats, sizeof(stats))) {
				return -EFAULT;
		}	
		break;

	case WCTDM_GET_REGS:

		memset(&regs, 0, sizeof(regs));
		for (x=0;x<NUM_REGS;x++) {
			read_silabs(wa->devext, chan->chanpos-1, 
					x, &regs.direct[x]);
		}

		if (copy_to_user((__user void *)data, &regs, sizeof(regs))) {
			return -EFAULT;
		}
		break;

	case WCTDM_SET_REG:

		if (copy_from_user(&regop, (__user void *)data, sizeof(regop))){
			return -EFAULT;
		}

		regop.val &= 0xff;

		printk(KERN_INFO "Setting direct %d to %04x on %d\n",
			regop.reg, regop.val, chan->chanpos);

		write_silabs(wa->devext, (chan->chanpos - 1), 
				regop.reg, regop.val);
		break;

	case DAHDI_SET_HWGAIN:

		if (copy_from_user(&hwgain, 
				(__user void *)data, sizeof(hwgain))) {
				return -EFAULT;
		}

		return (dahdi_warp_analog_fxo_set_hwgain(wa,
						chan->chanpos-1,
						hwgain.newgain,
						hwgain.tx));
		break;

	case WCTDM_SET_ECHOTUNE:

		printk(KERN_INFO "Setting echo registers\n");
		if (copy_from_user(&echoregs,
				(__user void *)data, sizeof(echoregs))){
			return -EFAULT;
		}

		chan_pos = chan->chanpos - 1;

		/* Set the ACIM register */
		write_silabs(wa->devext, chan_pos, 30, echoregs.acim);

		/* Set the digital echo canceller registers */
		write_silabs(wa->devext, chan_pos, 45, echoregs.coef1);
		write_silabs(wa->devext, chan_pos, 46, echoregs.coef2);
		write_silabs(wa->devext, chan_pos, 47, echoregs.coef3);
		write_silabs(wa->devext, chan_pos, 48, echoregs.coef4);
		write_silabs(wa->devext, chan_pos, 49, echoregs.coef5);
		write_silabs(wa->devext, chan_pos, 50, echoregs.coef6);
		write_silabs(wa->devext, chan_pos, 51, echoregs.coef7);
		write_silabs(wa->devext, chan_pos, 52, echoregs.coef8);

		printk(KERN_INFO "Set echo registers successfully\n");

		break;

	default:

		return -ENOTTY;
		break;
	}

	return 0;
}

/* set visual message waiting led on/off */
static int dahdi_warp_analog_set_vmwi(struct warp_analog *wa, struct dahdi_chan *chan)
{
	PDEVICE_EXTENSION pdx = wa->devext;
	int cc;

	/* get the index to the channel */
	cc = dahdi_warp_analog_chan_index(chan);

	if (wa->fxs_vmwi_active_msgs[cc]) {
		wa->fxs_vmwi_lrev[cc] = (wa->fxs_vmwisetting[cc].vmwi_type & DAHDI_VMWI_LREV) ? 1 : 0;
		wa->fxs_vmwi_hvdc[cc] = (wa->fxs_vmwisetting[cc].vmwi_type & DAHDI_VMWI_HVDC) ? 1 : 0;
		wa->fxs_vmwi_hvac[cc] = (wa->fxs_vmwisetting[cc].vmwi_type & DAHDI_VMWI_HVAC) ? 1 : 0;
	} else {
		wa->fxs_vmwi_lrev[cc] = 0;
		wa->fxs_vmwi_hvdc[cc] = 0;
		wa->fxs_vmwi_hvac[cc] = 0;
	}

	if (POLARITY_XOR(cc)) {
		/* select reverse modes */
		wa->idletxhookstate[cc] |= LINE_STATE_REVERSE_MASK;
		/* Do not set while currently ringing or open */
		if ((wa->lasttxhook[cc] != LINE_STATE_RINGING) &&\
			wa->lasttxhook[cc] != LINE_STATE_OPEN) {
			wa->lasttxhook[cc] |= LINE_STATE_REVERSE_MASK;
			daytona_phone_set_linefeed(pdx, (chan->chanpos-1), wa->lasttxhook[cc]);
		}
        } else {
		/* select forward modes */
		wa->idletxhookstate[cc] &= ~LINE_STATE_REVERSE_MASK;
		/* Do not set while currently ringing or open */
		if (wa->lasttxhook[cc] != LINE_STATE_RINGING &&\
			wa->lasttxhook[cc] != LINE_STATE_OPEN) {
			wa->lasttxhook[cc] &= ~LINE_STATE_REVERSE_MASK;
			daytona_phone_set_linefeed(pdx, (chan->chanpos-1), wa->lasttxhook[cc]);
		}
	}

	return 0;
}


/* ioctl() function for the FXS module */
static int dahdi_warp_analog_fxs_ioctl(struct dahdi_chan *chan,
					unsigned int cmd,
					unsigned long data)
{
	struct warp_analog *wa = (struct warp_analog*)chan->pvt;
	struct wctdm_stats stats;
	struct wctdm_regs regs;
	unsigned char regval;
	PDEVICE_EXTENSION pdx = wa->devext;
	int retval = 0;
	int x;
	int cc;
	int direction;

	/* check the module type */
	if (wa->module_type != MODULE_TYPE_FXS) {
		printk(KERN_ERR "%s() : called with NON-FXS device\n",
			__FUNCTION__);
		return -ENODEV;
	}
	
	switch (cmd) {

	/* enable/disable transfer while on hook */
	case DAHDI_ONHOOKTRANSFER: 

		if (get_user(x, (__user int *) data)) {
			return -EFAULT;
		}

		/* get the index to the channel */
		cc = dahdi_warp_analog_chan_index(chan);

		if ( cc >= 0 ) {
			DELAY_SET(&wa->fxs_oht_delay[cc], x);
			if  (POLARITY_XOR(cc))
				direction = LINE_STATE_RV_ONHOOK_TX;
			else 
				direction = LINE_STATE_FD_ONHOOK_TX;
		
			wa->idletxhookstate[cc] = direction;

			if (dahdi_warp_analog_active_state(wa->lasttxhook[cc])) {

				if  (POLARITY_XOR(cc))
					direction = LINE_STATE_RV_ONHOOK_TX;
				else 
					direction = LINE_STATE_FD_ONHOOK_TX;

				wa->lasttxhook[cc] = direction;

				daytona_phone_set_linefeed(pdx, (chan->chanpos-1), wa->lasttxhook[cc]);
			} 
		} else {
			printk(KERN_ERR "%s() : DAHDI_ONHOOKTRANSFER received invalid channel\n", __FUNCTION__);
			return -ENOTTY;
		}

		break;

	/* change line polarity (not when the phone is ringing) */
	case DAHDI_SETPOLARITY: 

		if (get_user(x, (__user int *) data)) {
			return -EFAULT;
		}

		cc = dahdi_warp_analog_chan_index(chan);

		if (cc >= 0) {
			/* check if we're ringing or open */
			if ((wa->lasttxhook[cc] == LINE_STATE_RINGING) || 
			    (wa->lasttxhook[cc] == LINE_STATE_OPEN))
				return -EINVAL;

			wa->fxs_reversepolarity[cc] = x;

			if (POLARITY_XOR(cc)) 
				wa->lasttxhook[cc] |= LINE_STATE_REVERSE_MASK;
			else
				wa->lasttxhook[cc] &= ~LINE_STATE_REVERSE_MASK;
				
			daytona_phone_set_linefeed(pdx, (chan->chanpos-1), wa->lasttxhook[cc]);
		}
		break;

	/* get the LED/LAMP control configuration for VMWI */
	case DAHDI_VMWI_CONFIG:

		cc = dahdi_warp_analog_chan_index(chan);

		if (copy_from_user(&(wa->fxs_vmwisetting[cc]), (__user void *) data, sizeof(wa->fxs_vmwisetting[cc])))
			return -EFAULT;

		dahdi_warp_analog_set_vmwi(wa, chan);
		break;

	/* set the LED/LAMP control for VMWI */
	case DAHDI_VMWI:

		if (get_user(x, (__user int *) data))
			return -EFAULT;

		if (x < 0)
			return -EFAULT;

		cc = dahdi_warp_analog_chan_index(chan);
		wa->fxs_vmwi_active_msgs[cc] = x;
		dahdi_warp_analog_set_vmwi(wa, chan);

		break;


	/* voltage information (helps with debugging using dahdi tools) */
	case WCTDM_GET_STATS: 

		read_silabs(wa->devext, chan->chanpos - 1, 80, &regval);
		stats.tipvolt = regval * -376;

		read_silabs(wa->devext, chan->chanpos - 1, 81, &regval);
		stats.ringvolt = regval * -376;

		read_silabs(wa->devext, chan->chanpos - 1, 82, &regval);
		stats.batvolt = regval * -376;

		if (copy_to_user((__user void *)data, &stats, sizeof(stats))) {
			return -EFAULT;
		}

		break;

	/* only returning direct registers for now */
	case WCTDM_GET_REGS:

		for (x = 0 ; x < NUM_REGS ; x++) {
			read_silabs(wa->devext, (chan->chanpos - 1), x, &regs.direct[x]);
		}

		break;

	default:

		return -ENOTTY;	

		break;
	}
	
	return (retval);
}

/* top level ioctl() function (gain control, battery voltage, diagnostics, etc) */
static int dahdi_warp_analog_ioctl(struct dahdi_chan *chan, unsigned int cmd, unsigned long data)
{
	struct warp_analog *wa = (struct warp_analog*)chan->pvt;
	int retval = -1;

	switch (wa->module_type) {

	case MODULE_TYPE_FXS:
		retval = dahdi_warp_analog_fxs_ioctl(chan, cmd, data);
		break;

	case MODULE_TYPE_FXO:
		retval = dahdi_warp_analog_fxo_ioctl(chan, cmd, data);
		break;

	default:
		printk(KERN_ERR "%s(): unknown module type %d\n",
			__FUNCTION__, wa->module_type);	
		retval = -1;
		break;
	}

	return (retval);
}



/* do we really need this function? */
static int dahdi_warp_analog_watchdog(struct dahdi_span *span, int event)
{
	return 0;
}

/* initialization function for fxo modules */
static int dahdi_warp_analog_initialize_fxo(struct warp_analog *warpalg, unsigned int port_mask)
{
	int first_timeslot;
	int ii, ts;

	warpalg->pos = dahdi_warp_analog_devcount();
	warpalg->module_type = MODULE_TYPE_FXO; 
	warpalg->module_port = port_mask;
	warpalg->chan_mask = dahdi_warp_get_module_mask(port_mask);
	warpalg->num_chans = dahdi_warp_get_line_count(port_mask);
	first_timeslot = dahdi_warp_analog_get_first_timeslot(warpalg);
	ts = first_timeslot;

	for (ii = 0; ii < warpalg->num_chans; ii++) {
		warpalg->chan_timeslot[ii] = ((ts++) << 1);
	}

	sprintf(warpalg->span.name, "FXO/%d", warpalg->pos);
	snprintf(warpalg->span.desc, sizeof(warpalg->span.desc) - 1, 
		"%s Board %d", "Pika FXO", warpalg->pos);
	dahdi_copy_string(warpalg->span.devicetype, 
			"WARP FXO driver", sizeof(warpalg->span.devicetype));

	for (ii = 0; ii < warpalg->num_chans; ii++) {
		warpalg->chans[ii] = &warpalg->_chans[ii];
		sprintf(warpalg->chans[ii]->name, "FXO/%d/%d", warpalg->pos, ii);
		warpalg->chans[ii]->sigcap = DAHDI_SIG_FXSKS | \
					DAHDI_SIG_FXSLS | \
					DAHDI_SIG_SF;
		warpalg->chans[ii]->pvt = warpalg;
		warpalg->chans[ii]->chanpos = first_timeslot + ii + 1;
	}

	warpalg->span.chans = warpalg->chans;
	warpalg->span.manufacturer = "Pika";
	warpalg->span.channels = warpalg->num_chans;
	warpalg->span.irq = 0;	
	warpalg->span.flags = DAHDI_FLAG_RBS; 	 
#if defined(DAHDI_2_4_0)
	warpalg->span.ops = &warpalg_span_ops;
#else
	warpalg->span.owner = THIS_MODULE;
	warpalg->span.open = dahdi_warp_analog_open;
	warpalg->span.close = dahdi_warp_analog_close;
	warpalg->span.ioctl = dahdi_warp_analog_ioctl;
	warpalg->span.watchdog = dahdi_warp_analog_watchdog;
	warpalg->span.hooksig = dahdi_warp_analog_hooksig;
	warpalg->span.pvt = warpalg;
#endif
	init_waitqueue_head(&warpalg->span.maintq);	
	warpalg->span.deflaw = DAHDI_LAW_MULAW;
	
	/* analog board is =not= the preferred master (BRI would be) */
	if (dahdi_register(&warpalg->span, 0)) {
		printk(KERN_ERR "%s() : Unable to Register\n",
			__FUNCTION__);
		return -1;
	}

	printk(KERN_DEBUG "%s() : Successfully Registered.\n", __FUNCTION__);

	return 0;	
}

/* initialization function for fxs modules */
static int dahdi_warp_analog_initialize_fxs(struct warp_analog *warpalg,
					unsigned int port_mask)
{
	struct warp_analog *wa = warpalg;
	int first_timeslot;
	int ii, ts;

	/* this goes into the fxs initalization function */
	warpalg->pos = dahdi_warp_analog_devcount();
	warpalg->module_type = MODULE_TYPE_FXS; 
	warpalg->module_port = port_mask;
	warpalg->chan_mask = dahdi_warp_get_module_mask(port_mask);
	warpalg->num_chans = dahdi_warp_get_line_count(port_mask);
	first_timeslot = dahdi_warp_analog_get_first_timeslot(warpalg);
	ts = first_timeslot;

	/* add the on board FXS port */
	warpalg->chan_mask |= 0x02;
	warpalg->chan_timeslot[0] = 2;

	/* add the rest if there are modules */
	if (first_timeslot > 0) {
		for (ii = 1; ii < warpalg->num_chans; ii++) {
			warpalg->chan_timeslot[ii] = ((ts++) << 1);
		}
	}

	sprintf(warpalg->span.name, "FXS/%d", warpalg->pos);
	snprintf(warpalg->span.desc, sizeof(warpalg->span.desc) - 1,
		"%s Board %d", "Pika FXS", warpalg->pos);
	dahdi_copy_string(warpalg->span.devicetype,
			"WARP FXS driver", sizeof(warpalg->span.devicetype));

	for (ii = 0; ii < warpalg->num_chans; ii++) {
		warpalg->chans[ii] = &warpalg->_chans[ii];
		sprintf(warpalg->chans[ii]->name, "FXS/%d/%d", warpalg->pos, ii);
		warpalg->chans[ii]->sigcap = DAHDI_SIG_FXOKS | \
					DAHDI_SIG_FXOLS | \
					DAHDI_SIG_SF | \
					DAHDI_SIG_EM | \
					DAHDI_SIG_CLEAR;
		warpalg->chans[ii]->pvt = warpalg;
		if (ii > 0) {
			warpalg->chans[ii]->chanpos = first_timeslot + ii;
		} else {				
			/* 1st channel is always the on board FXS */
			warpalg->chans[0]->chanpos = 2;
		}

		/* do not reverse polarity */
		warpalg->fxs_reversepolarity[ii] = 0;

		/* check for line reversal */
		warpalg->idletxhookstate[ii] = POLARITY_XOR(ii) ? \
			LINE_STATE_REVERSE_ACTIVE : LINE_STATE_FORWARD_ACTIVE;
		DELAY_CLR(&warpalg->fxs_oht_delay[ii]);

		/* always start with default FSK with no messages in the bank */
		memset(&warpalg->fxs_vmwisetting[ii], 0, sizeof(warpalg->fxs_vmwisetting[ii]));
		warpalg->fxs_vmwi_lrev[ii] = 0;
		warpalg->fxs_vmwi_hvdc[ii] = 0;
		warpalg->fxs_vmwi_hvac[ii] = 0;
		warpalg->fxs_vmwi_active_msgs[ii] = 0;
	}

	warpalg->span.chans = warpalg->chans;
	warpalg->span.manufacturer = "Pika";
	warpalg->span.channels = warpalg->num_chans;
	warpalg->span.irq = 0;	
	warpalg->span.flags = DAHDI_FLAG_RBS;	/* (option 2) in dahdi.h */ 

#if defined(DAHDI_2_4_0)
	warpalg->span.ops = &warpalg_span_ops;
#else
	warpalg->span.owner = THIS_MODULE;
	warpalg->span.open = dahdi_warp_analog_open;
	warpalg->span.close = dahdi_warp_analog_close;
	warpalg->span.ioctl = dahdi_warp_analog_ioctl;
	warpalg->span.watchdog = dahdi_warp_analog_watchdog;
	warpalg->span.hooksig = dahdi_warp_analog_hooksig;
	warpalg->span.pvt = warpalg;
#endif

	init_waitqueue_head(&warpalg->span.maintq);	
	warpalg->span.deflaw = DAHDI_LAW_MULAW;	/* default to mulaw */
	
	/* analog board is =not= the preferred master */
	if (dahdi_register(&warpalg->span, 0)) {
		printk(KERN_ERR "%s() : Unable to Register\n",
			__FUNCTION__);
		return -1;
	}

	printk(KERN_DEBUG "%s() : Successfully Registered.\n",
		__FUNCTION__);

	return 0;	
}

/* translate a warp event code to dahdi/zaptel event code */
int dahdi_warp_event_code_translate(int *dahdi_event_code, int warp_event_code)
{
	int result = 0;

	switch (warp_event_code) {

	case PHONE_EVENT_ONHOOK:
		*dahdi_event_code = DAHDI_RXSIG_ONHOOK;
		break;

	case PHONE_EVENT_OFFHOOK:
		*dahdi_event_code = DAHDI_RXSIG_OFFHOOK;
		break;

	case TRUNK_EVENT_RING_ON:
		*dahdi_event_code = DAHDI_RXSIG_RING;	
		break;

	case TRUNK_EVENT_RING_OFF:
		*dahdi_event_code = DAHDI_RXSIG_OFFHOOK; /* no ringoff event */
		break;

	case TRUNK_EVENT_DROPOUT:
		*dahdi_event_code = DAHDI_RXSIG_ONHOOK;
		break;

	default:
		result = -1;
		break;
	}

	return (result);
}

/* get pointer to the parent struct via channel number */
struct warp_analog *dahdi_warp_analog_channel_to_wa(int chnum)
{
	struct warp_analog *wa;
	int ii, found;

	found = 0;	
	for (ii = 0; ii < dahdi_warp_analog_devcount(); ii++) {
		wa = dahdi_warp_analog_devptr(ii);
		if ((wa) && (wa->chan_mask & ( 1 << chnum ))) {
			found = 1;
			break;
		}
	}

	if (found) {
		return wa;
	} else {
		return NULL;
	}
}

/* convert from warp to span channel number */
int dahdi_warp_analog_warp_to_span_chan_num(int *span_chan_num, 
					int warp_chan_num)
{
	struct warp_analog *warpalg;

	/* get pointer to the warp struct using warp_channel number */
	if (!(warpalg=dahdi_warp_analog_channel_to_wa(warp_chan_num))) {
		printk(KERN_ERR "%s() : invalid warp channel number %d\n",
			__FUNCTION__, warp_chan_num);
		return (-1);
	}

	switch (warp_chan_num) {

	case 1: 	/* onboard module */
		/* On Board FXS is always the first FXS port */
		*span_chan_num = 0; 
		break;

	case 2 ... 5:	/* module A */
		/* For FXS modules, module channels are always from 1..4, 0 is for the onboard FXS */
		/* For FXO, there is no confusion as there is no onboard FXO */
		if (warpalg->module_type == MODULE_TYPE_FXS) {
			*span_chan_num = warp_chan_num - 1;
		} else if (warpalg->module_type == MODULE_TYPE_FXO) {
			*span_chan_num = warp_chan_num - 2;
		} else {
			// should NEVER happen
		}

		break;

	case 6 ... 9:	/* module B */
		/* For FXS modules, module channels are always from 1..4, 0 is for the onboard FXS */
		/* For FXO, there is no confusion as there is no onboard FXO */
		if (warpalg->module_type == MODULE_TYPE_FXS) {
			/* check for the case where we have two modules of the same kind (FXS) */
			if ((dahdi_warp_analog_devcount() == 1) && (warpalg->num_chans > 5)) {
				*span_chan_num = warp_chan_num - 1;
			} else {
				*span_chan_num = warp_chan_num - 5;
			}
		} else if (warpalg->module_type == MODULE_TYPE_FXO) {
			/* check for the case where we have two modules of the same kind (FXO) */
			if (warpalg->num_chans > 4) {
				*span_chan_num = warp_chan_num - 2;
			} else {
				*span_chan_num = warp_chan_num - 6;
			}
		} else {
			// should NEVER happen
		}
	
		break;
			
	default:
		printk(KERN_ERR "%s() : invalid channel number received %d\n",
			__FUNCTION__, warp_chan_num);
		return (-1);
	}

	return 0;
}

/* send an event up the asterisk stack for a given channel number and a given eventcode */
int dahdi_warp_analog_send_event_up(int warp_chan_num, unsigned int warp_event_code)
{
	struct warp_analog *warpalg;
	int dahdi_event_code;
	int span_chan_num = 0;
	int result = 0;

	warpalg = dahdi_warp_analog_channel_to_wa(warp_chan_num);
	if (!warpalg) {
		printk(KERN_ERR "%s() : received invalid channel struct ptr\n",
			__FUNCTION__);
		return (-1);
	}

	if (dahdi_warp_analog_warp_to_span_chan_num(&span_chan_num, warp_chan_num) < 0) {
		printk(KERN_ERR "%s() : received invalid warp channel number\n",
			__FUNCTION__);
		return (-1);
	}

	switch (warp_event_code) {

	/* polarity reversal uses the dahdi_qevent_lock() function */
	case TRUNK_EVENT_REVERSAL:
		dahdi_qevent_lock(warpalg->chans[span_chan_num], DAHDI_EVENT_POLARITY);
		break;

	/* channel alarms use the dahdi_alarm_channel() function */
	case TRUNK_EVENT_BELOW_THRESHOLD:
		dahdi_alarm_channel(warpalg->chans[span_chan_num], DAHDI_ALARM_RED);
		break;

	/* channel alarms use the dahdi_alarm_channel() function */
	case TRUNK_EVENT_ABOVE_THRESHOLD:
		dahdi_alarm_channel(warpalg->chans[span_chan_num], DAHDI_ALARM_NONE);
		break;

	/* phone and trunk signaling is covered by dahdi_hooksig() function */
	case TRUNK_EVENT_DROPOUT:
	case PHONE_EVENT_ONHOOK:
	case PHONE_EVENT_OFFHOOK:
	case TRUNK_EVENT_RING_ON:
	case TRUNK_EVENT_RING_OFF:
		if (dahdi_warp_event_code_translate(&dahdi_event_code, 
						warp_event_code) >= 0) {
			dahdi_hooksig(warpalg->chans[span_chan_num],
					dahdi_event_code);
		} 
		break;

	/* drop anything that's not handled */
	default:
		result = -1;
		break;

	}

	return (result);
}

/* function to return the number of lines for a given port mask */
static int dahdi_warp_get_line_count(int port_mask)
{
	int result = 0;

	if (port_mask & MODULE_INTERNAL) {
		result += 1;
	}

	if (port_mask & MODULE_PORT_A) {
		result += 4;
	} 

	if (port_mask & MODULE_PORT_B) {
		result += 4;
	} 

	return (result);
}

unsigned int dahdi_warp_get_trunk_configuration(void)
{
	unsigned int trunk_configuration;
	unsigned char reg_16 = 0;
	unsigned char reg_26 = 0;
	unsigned char reg_30 = 0;
	unsigned char reg_31 = 0;

	/* check we've got a valid configuration and default to North American */
	/* to be on safety's side */
	if (_opermode == -1) {
                printk(KERN_ERR "Unknown/Invalid operating mode. Defaulting to North American [%s].\n", opermode);
		_opermode = 0;
	}

	/* Set On-hook speed, Ringer impedence, and ringer threshold */
	reg_16 |= (fxo_modes[_opermode].ohs << 6);
	reg_16 |= (fxo_modes[_opermode].rz << 1);
	reg_16 |= (fxo_modes[_opermode].rt);

	/* Set DC Termination:
	Tip/Ring voltage adjust, minimum operational current, current limitation */
	reg_26 |= (fxo_modes[_opermode].dcv << 6);
	reg_26 |= (fxo_modes[_opermode].mini << 4);
	reg_26 |= (fxo_modes[_opermode].ilim << 1);

	/* Set AC Impedance */
	reg_30 = (fxo_modes[_opermode].acim);

	/* Misc. DAA parameters */
	reg_31 = 0xa3;
	reg_31 |= (fxo_modes[_opermode].ohs2 << 3);

	trunk_configuration = (	reg_16 << 24 ) | \
                              ( reg_26 << 16 ) | \
                              ( reg_30 << 8 ) | \
                              ( reg_31 );

	return (trunk_configuration);
}

/* function to configure an FXO on the WARP */
static void dahdi_warp_configure_fxo_hw(struct warp_analog *waptr)
{
	trunk_start_t trunk_start_cfg;
	unsigned int detection_mask;
	PDEVICE_EXTENSION pdx = waptr->devext;
	PKH_TTrunkInfo info;
	int span_chan_num;
	int ii;

	/* configure hw through the daytona() functions */
	daytona_configure_fxo(waptr->devext, waptr->chan_mask);

	/* change the detection mask */
	detection_mask = PKH_TRUNK_REVERSAL_DETECTION | \
			PKH_TRUNK_LCSO | \
			PKH_TRUNK_DOD | \
			PKH_TRUNK_LOF | \
			PKH_TRUNK_RX_OVERLOAD | \
			PKH_TRUNK_RING_DETECTION | \
			PKH_TRUNK_THRESHOLD_DETECTION;

	/* get the trunk start configuration word */
	trunk_start_cfg.internationalControl = dahdi_warp_get_trunk_configuration();

	/* and if we're overriding alaw */
	trunk_start_cfg.compandMode = (alawoverride) ? PKH_TRUNK_AUDIO_ALAW : PKH_TRUNK_AUDIO_MULAW;

	/* set up each trunk */
	trunk_for_each(ii) {
		trunk_start_cfg.trunk = ii;
	 	daytona_trunk_set_config(pdx, &trunk_start_cfg);	
		daytona_set_detect(pdx, ii, detection_mask);
		daytona_trunk_start(pdx, ii);
	}

	/* read the voltage from each trunk and see if we have connection or not */
	/* FPGA only monitors transitions, so the initial state is still on us to determine */
	trunk_for_each(ii) {
		if (dahdi_warp_analog_warp_to_span_chan_num(&span_chan_num, ii) < 0) {
			printk(KERN_ERR "%s() : invalid trunk number %d\n", __FUNCTION__, ii);
			return;
		}
		daytona_trunk_info(pdx, ii, &info);

		if (abs(info.lineVoltage) < PKH_TRUNK_THRESHOLD_NORMAL) {
			dahdi_alarm_channel(waptr->chans[span_chan_num], DAHDI_ALARM_RED);
		} else {
			dahdi_alarm_channel(waptr->chans[span_chan_num], DAHDI_ALARM_NONE);
		}
	}

	/* set the hw gain values via the module parameters */
	for (ii = 0; ii < waptr->num_chans; ii++) {
		dahdi_warp_analog_fxo_set_hwgain(waptr, ii, fxo_rxgain, 0);
		dahdi_warp_analog_fxo_set_hwgain(waptr, ii, fxo_txgain, 1);
	}
}

/* function to get the module mask (which lines?) */
static int dahdi_warp_get_module_mask(int port)
{
	int result = 0;

	if (port & MODULE_PORT_A) {
		result |= 0x003C;
	}

	if (port & MODULE_PORT_B) {
		result |= 0x03C0;
	}

	if (port & MODULE_INTERNAL) {
		result |= 0x0002;
	}

	return (result);
}

/* function to configure an FXS on the WARP */
static void dahdi_warp_configure_fxs_hw(struct warp_analog *waptr)
{
	PDEVICE_EXTENSION pdx = waptr->devext;
	int operating_mode;
	int compand_mode;
	int phone;

	/* check the companding mode */	
	compand_mode = (alawoverride) ? PKH_PHONE_AUDIO_ALAW : PKH_PHONE_AUDIO_MULAW;

	/* check for EU|NA and default to NA if it's not recognized */
	if (!strcmp(opermode, "TBR21")) {
		operating_mode = PKH_PHONE_INTERNATIONAL_CONTROL_EU;
	} else if (!strcmp(opermode, "FCC")) {
		operating_mode = PKH_PHONE_INTERNATIONAL_CONTROL_NA;
	} else {
		printk(KERN_ERR "Unknown Operating Mode [%s]. ", opermode);
		printk(KERN_ERR "Defaulting to North American\n");
		printk(KERN_ERR "Call/e-mail PIKA for custom settings.\n");
		operating_mode = PKH_PHONE_INTERNATIONAL_CONTROL_NA;
	} 

	/* configure hw through the daytona() functions */
	daytona_configure_fxs(waptr->devext, waptr->chan_mask);

        /* start each phone with the correct settings */
	phone_for_each(phone) {
		/* we do 8khz only */
		daytona_phone_config(pdx, phone, operating_mode, compand_mode);

#if 0		/* this is still to be tested -- sets the HW gain for rx/tx for the FXS lines */
		dahdi_warp_analog_fxs_set_hwgain(pdx, phone, fxs_rxgain, fxo_rxgain);
#endif

		/* start the phone */
		daytona_phone_start(pdx, phone);
	}

}

#ifdef SAM_NOT_USED
/* set the fxs receive or transmit gain */
static void dahdi_warp_analog_fxs_set_hwgain(struct warp_analog *wa, int chan, int fxs_rx_gain, int fxs_tx_gain)
{
	unsigned char reg9;

	read_silabs(wa->devext, chan, 29, &reg9);

	switch (fxs_tx_gain) {

		case 35:
			reg9 += 8;
			break;
		case -35:
			reg9 += 4;
			break;
		case 0:
			break;
	}

	switch (fxs_rx_gain) {

		case 35:
			reg9 += 2;
			break;
		case -35:
			reg9 += 1;
			break;
		case 0:
			break;
	}

	write_silabs(wa->devext, chan, 9, reg9);
}
#endif

/* add a module to the list of modules */
static void dahdi_warp_add_module(PDEVICE_EXTENSION pdevext, int type, int port)
{
	struct warp_analog *waptr;

	if ((waptr = dahdi_warp_analog_device_alloc()) == NULL) {
		printk(KERN_ERR "%s() : cannot exceed the max module count\n",
			__FUNCTION__);
		return;
	}

	/* do some common setup */
	waptr->devext = pdevext;

	switch (type) {

	case MODULE_TYPE_FXO:
		printk(KERN_DEBUG "%s() : add one FXO to WARP 0x%08x\n",
			__FUNCTION__, (unsigned int)waptr);
		
		dahdi_warp_analog_initialize_fxo(waptr, port); 
		dahdi_warp_configure_fxo_hw(waptr);
		break;

	case MODULE_TYPE_FXS:
		printk(KERN_DEBUG "%s() : add one FXS to WARP 0x%08x\n",
			__FUNCTION__, (unsigned int)waptr);

		dahdi_warp_analog_initialize_fxs(waptr, port); 
		dahdi_warp_configure_fxs_hw(waptr);
		break;

	default:
		printk(KERN_ERR "%s() : unknown module type %d\n",
			__FUNCTION__, type);
		break;

	}
}

/* function to process the onhook transfer timer */
void do_timers(PDEVICE_EXTENSION pdx)
{
	struct warp_analog *wa;
	int direction;
	int ii, jj, cc;

	/* OHT timer for FXS (used for CID) */
	for (ii = 0; ii < dahdi_warp_analog_devcount(); ii++) {

		/* we only do FXS in this loop */
		wa = dahdi_warp_analog_devptr(ii);
		if ((!wa) || (wa->module_type != MODULE_TYPE_FXS)) {
			continue;
		}

		/* loop over existing channels */
		for (jj = 0; jj < wa->num_chans; jj++) {
			cc = dahdi_warp_analog_chan_index(wa->chans[jj]);
			if (wa->lasttxhook[cc] == LINE_STATE_RINGING) {
				DELAY_SET(&wa->fxs_oht_delay[cc], 
					OHT_TIMER_DELAY);

				if (wa->fxs_reversepolarity[cc])
					direction = LINE_STATE_RV_ONHOOK_TX;
				else
					direction = LINE_STATE_FD_ONHOOK_TX;

				wa->idletxhookstate[cc] = direction;
			} else {
				if  (DELAY_ISSET(&wa->fxs_oht_delay[cc]) && \
					(DELAY_UP(&wa->fxs_oht_delay[cc]))) {
					DELAY_CLR(&wa->fxs_oht_delay[cc]);

					if (wa->fxs_reversepolarity[cc])
						direction = LINE_STATE_REVERSE_ACTIVE;
					else
						direction = LINE_STATE_FORWARD_ACTIVE;

					wa->idletxhookstate[cc] = direction;

					if ((wa->lasttxhook[cc] == LINE_STATE_FD_ONHOOK_TX) || \
						(wa->lasttxhook[cc] == LINE_STATE_RV_ONHOOK_TX)) {
						wa->lasttxhook[cc] = direction; 
						daytona_phone_set_linefeed(pdx, (wa->chans[jj]->chanpos-1), wa->lasttxhook[cc]);
					}
				}
			}
		}
	}
}



/* interrupt service routine */
irqreturn_t dahdi_warp_isr(int irq, void *context)
{
   	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)context;
        unsigned iccr, stat_mask;

        iccr = BAR0_READ(pdx, BAR0_ICCR);

        stat_mask = iccr & ~(IMR_FPGA_MASK | IMR_MASK);
        if (stat_mask) {
                BAR0_WRITE(pdx, BAR0_ICCR, stat_mask);
	}

        iccr &= pdx->imr; /* mask out ignored ints */

        /* We need to update tics to make timers work */
        pdx->tics = jiffies_to_msecs(jiffies);

	/* Work the phones */
	if (pdx->phone_mask) {
		do_phones(pdx, iccr);
	}

	/* Work the trunks */
	if (pdx->trunk_mask) {
		do_trunks(pdx, iccr);
	}

	/* Work timers */
	do_timers(pdx);

	/* Do one last read to flush the acks. */
        BAR0_READ(pdx, BAR0_ICCR);

	return IRQ_HANDLED;
}

/* find the installed modules (if any) on the WARP */
int __init dahdi_warp_scan_modules(PDEVICE_EXTENSION pdx)
{
	unsigned int fpga_rev = BAR0_READ(pdx, BAR0_FPGA_REV);
	unsigned int module_a_type = (fpga_rev >> 16) & MODULE_TYPE_MASK;
	unsigned int module_b_type = (fpga_rev >> 20) & MODULE_TYPE_MASK;
	unsigned int fxo_module_mask = 0;
	unsigned int fxs_module_mask = 0;

	/* configure FXS modules first */

	/* the internal FXS is always implicitly added (always there) */
	fxs_module_mask |= MODULE_INTERNAL;

	/* then, module A (if any) */
	if (module_a_type == MODULE_TYPE_FXS) {
		fxs_module_mask |= MODULE_PORT_A;
	}
	
	/* then, module B */
	if (module_b_type == MODULE_TYPE_FXS) {
		fxs_module_mask |= MODULE_PORT_B;
	}
	
	/* add any/all fxs modules */
	if (fxs_module_mask) {
		dahdi_warp_add_module(pdx, MODULE_TYPE_FXS, fxs_module_mask);
	}

	/* configure FXO modules next */

	if (module_a_type == MODULE_TYPE_FXO) {
		fxo_module_mask |= MODULE_PORT_A;
	}

	if (module_b_type == MODULE_TYPE_FXO) {
		fxo_module_mask |= MODULE_PORT_B;
	}

	/* add any/all fxo modules */
	if (fxo_module_mask) {
		dahdi_warp_add_module(pdx, MODULE_TYPE_FXO, fxo_module_mask);
	}

	/* return result (always success)*/
	return 0;
}

/* utility function to print out debug info in human readable form */
const char *dahdi_warp_event_to_text(unsigned int event_code)
{
	struct event_tbl {
		unsigned int code;
		const char *text;
	} static event_table[] = {
		{ 
			.code = PHONE_EVENT_OFFHOOK,
			.text = "PHONE_EVENT_OFFHOOK", 		
		},
		{
		 	.code = PHONE_EVENT_ONHOOK,
			.text = "PHONE_EVENT_ONHOOK",
		},
		{ 
			.code = PHONE_EVENT_HOOKFLASH,
			.text = "PHONE_EVENT_HOOKFLASH",
		},
		{ 	
			.code = PHONE_EVENT_RING_ON,
			.text = "PHONE_EVENT_RING_ON",
		},
		{ 	
			.code = PHONE_EVENT_RING_OFF,
			.text = "PHONE_EVENT_RING_OFF",
		},
		{ 
			.code = PHONE_EVENT_RING_TIMEOUT,
			.text = "PHONE_EVENT_RING_TIMEOUT",
		},
		{ 
			.code = PHONE_EVENT_POWER_ALARM,
			.text = "PHONE_EVENT_POWER_ALARM",	
		},
		{ 	
			.code = TRUNK_EVENT_HOOKFLASH,
			.text = "TRUNK_EVENT_HOOKFLASH",	
		},
		{ 
			.code = TRUNK_EVENT_DIALED,
			.text = "TRUNK_EVENT_DIALED",
		},
		{ 
			.code = TRUNK_EVENT_REVERSAL,
			.text = "TRUNK_EVENT_REVERSAL",
		},
		{ 
			.code = TRUNK_EVENT_LCSO,
			.text = "TRUNK_EVENT_LCSO",
		},
		{ 	
			.code = TRUNK_EVENT_DROPOUT,
			.text = "TRUNK_EVENT_DROPOUT",
		},
		{ 	
			.code = TRUNK_EVENT_LOF,
			.text = "TRUNK_EVENT_LOF",
		},
		{ 	
			.code = TRUNK_EVENT_RX_OVERLOAD,
			.text = "TRUNK_EVENT_RX_OVERLOAD",
		},
		{ 	
			.code = TRUNK_EVENT_RING_OFF,
			.text = "TRUNK_EVENT_RING_OFF",
		},
		{ 
			.code = TRUNK_EVENT_RING_ON,
			.text = "TRUNK_EVENT_RING_ON",
		},
		{ 	
			.code = TRUNK_EVENT_BELOW_THRESHOLD,
			.text = "TRUNK_EVENT_BELOW_THRESHOLD",
		},
		{ 	
			.code = TRUNK_EVENT_ABOVE_THRESHOLD,
			.text = "TRUNK_EVENT_ABOVE_THRESHOLD",
		},
		{  	
			.code = -1,
			.text = "INVALID",
		}
	};

	int found = 0;
	int ii = 0;

	for  (ii = 0; event_table[ii].code != -1; ii++) {
		if (event_table[ii].code == event_code) {
			found = 1;
			break;
		}
	}

	if (found) {
		return event_table[ii].text;
	} else	{
		return "UNKNOWN EVENT";
	}
}

/* Function to send upstream messages to asterisk
 * (phone onhook, offhook, trunk events, etc) 
 */
int dahdi_add_event(PDEVICE_EXTENSION pdx, unsigned type,
			unsigned line, unsigned mask) 
{
	return dahdi_warp_analog_send_event_up(line, mask);
}

EXPORT_SYMBOL(dahdi_add_event);

/* DMA ISR routine for the WARP FXS/FXO boards */ 
//static void dahdi_warp_dma_isr(struct dma_ctx *dma, int dma_index)
static void dahdi_warp_dma_isr(int cardid)
{
	const unsigned int bytes = FRAMES_PER_TRANSFER * FRAMESIZE;
	const unsigned int bcount = FRAMES_PER_BUFFER / FRAMES_PER_TRANSFER;
	u8 *cur_rx_buf, *cur_tx_buf;
	int rx_index, tx_index, index;
	struct warp_analog *wa = dahdi_warp_analog_devptr(0);
	PDEVICE_EXTENSION pdx = wa->devext;

	/* get the dma index */
	index = fpga_read(pdx->bar0, FPGA_DMA_SG_IDX) & 0x1ff;
	index = index >> 1;

	/* pick the RX buffer before the current one */
	rx_index = ((index - 1) >= 0) ? (index - 1) : (index - 1 + bcount); 
	
	/* pick the TX buffer after the current one */
	tx_index = ((index + 1) < bcount) ? (index + 1) : (index + 1 - bcount); 

	/* calculate the position of the rx and tx buffers */
	cur_tx_buf = pdx->tx_buf + tx_index * bytes;
	cur_rx_buf = pdx->rx_buf + rx_index * bytes;

	/* do your receive work */
	dahdi_warp_analog_receiveprep(cur_rx_buf);

	/* do your transmit work */
	dahdi_warp_analog_transmitprep(cur_tx_buf);
}

/* find out the operation mode (if it's valid) */
static int dahdi_warp_get_opermode(void)
{
	int x;

	for (x = 0; x < (sizeof(fxo_modes) / sizeof(fxo_modes[0])); x++) {
		if (!strcmp(fxo_modes[x].name, opermode))
		break;
	}

	if (x < sizeof(fxo_modes) / sizeof(fxo_modes[0])) {
		_opermode = x;
	} else {
		printk(KERN_INFO "Note this option is CASE SENSITIVE!\n");
		printk(KERN_NOTICE "Invali operating mode '%s'.", opermode);
		printk(KERN_NOTICE "Please choose one of:\n");
		for (x = 0; x < sizeof(fxo_modes) / sizeof(fxo_modes[0]); x++) {
			printk(KERN_INFO "  %s\n", fxo_modes[x].name);
		}
		return (-1);
	}

	return 0;
}

/* module initialization function */
int __init dahdi_warp_init_module(void)
{
	PDEVICE_EXTENSION pdx;
	struct device_node *np;
	struct resource res;
	void __iomem *fpga;
	int irq;
	int rc;

	/* check operating mode parameters first */
	if (dahdi_warp_get_opermode() < 0) {
		return (-1);
	}

	if ((pdx = kzalloc(sizeof(DEVICE_EXTENSION), GFP_KERNEL)) == NULL) {
		printk(KERN_ERR "%s:%s() error allocating memory\n",
			__FILE__, __FUNCTION__);
		return -ENOMEM;
	}

	atomic_set(&pdx->open_count, 0);

	init_waitqueue_head(&pdx->read_queue);

	/* initialize all tasklets */
	tasklet_init(&pdx->phone_bh, phone_tasklet, (unsigned long)pdx);
	tasklet_init(&pdx->ring_bh, ring_tasklet, (unsigned long)pdx);
	tasklet_init(&pdx->line_bh, line_tasklet, (unsigned long)pdx);

	/* initialize the spinlock for ... */
	spin_lock_init(&pdx->stat_lock);

	/* extract all fpga parameters from open firmware */
	if ((np = of_find_compatible_node(NULL, NULL, "pika,fpga")) == NULL) {
		printk(KERN_ERR "%s:%s() Unable to find fpga\n",
			__FILE__, __FUNCTION__);
		goto error_cleanup;
	}

	if ((irq = irq_of_parse_and_map(np, 0)) == NO_IRQ) {
		printk(KERN_ERR "%s:%s() irq_of_parse_and_map failed\n",
			__FILE__, __FUNCTION__);
		goto error_cleanup;
	}

	if (of_address_to_resource(np, 0, &res)) {
		printk(KERN_ERR "%s:%s() Unable to get FPGA address\n",
			__FILE__, __FUNCTION__);
		goto error_cleanup;
	}

	/* map everything except SD. */
	if ((fpga = ioremap(res.start, 0x2200)) == NULL) {
		printk(KERN_ERR "%s:%s() Unable to map FPGA\n",
			__FILE__, __FUNCTION__);
		goto error_cleanup;
	}	

   	of_node_put(np);

        pdx->info.irql = irq;
        pdx->bar0 = fpga;

#ifdef STANDALONE_ANALOG_DRIVER
	/* you need a device for DMA or nothing works with 2.6.31 */
	pdx->dev = platform_device_register_simple("warp-dev", 0, NULL, 0);
	pdx->dev->dev.coherent_dma_mask = ~0ULL;

	/* initialize the dma library 
	 * note that this should be done only once per boot 
	 */

	if ((rc = dma_init_module(pdx))) {
		goto error_cleanup;
	}
#endif

	/* initialize the locks */
	spin_lock_init(&pdx->ringgen_lock);
        spin_lock_init(&pdx->event_lock);

        pdx->cardid = 0; /* How should we mark the card id? */

	/* put our default detection values in place */
        pdx->offhook_debounce   = msecs_to_tics(OFFHOOK_DEBOUNCE_MS);
        pdx->hookflash_debounce = msecs_to_tics(HOOKFLASH_DEBOUNCE_MS);
        pdx->onhook_debounce    = msecs_to_tics(ONHOOK_DEBOUNCE_MS);
	pdx->threshold_debounce = THRESHOLD_DETECT_DEBOUNCE_TIME_DEFAULT;
        pdx->reversal_debounce  = msecs_to_tics(REVERSAL_DEBOUNCE_MS);
        pdx->disconnect_debounce = msecs_to_tics(DISCONNECT_DEBOUNCE_MS);
	pdx->disconnect_hookstate_debounce = HOOK_STATE_DEBOUNCE_TIME_DEFAULT;

	/* get the addresses of the DMA buffers from taco */
	if ((rc = warp_dma_init(pdx))) {
		goto error_cleanup;
	}

	pdx->id = 0;
	pdx->info.irql = irq;

	/* disable interrupts */
	imr_clr(pdx, ~IMR_MASK); 

	/* add the callback function for DAHDI events */
	pdx->add_event_callback = dahdi_add_event;

	/* install the interrupt handler */
	if ((rc = request_irq(irq, dahdi_warp_isr, 
				IRQF_SHARED, "pikadahdi", pdx))) {
		printk(KERN_ERR "%s:%s() Unable to request irq %d\n", 
			__FILE__, __FUNCTION__, rc);
		goto error_cleanup;
	}

	/* scan all modules available on the warp */
	dahdi_warp_scan_modules(pdx);
	
	/* initialize pft */
	warp_pft_init(pdx);

	/* enable the dma for this card */
	if ((rc = warp_dma_enable(dahdi_warp_dma_isr))) {
		/* release the interrupt handler */
		free_irq(pdx->info.irql, pdx);
		/* do more cleanup */
		goto error_cleanup;
	}

	/* enable audio on the warp */
	warp_audio_enable(pdx);

        return 0;

error_cleanup:

	if (pdx->bar0)
		iounmap(pdx->bar0);

	kfree(pdx);

	if (np)
		of_node_put(np);

	return -ENOENT;
}

module_init(dahdi_warp_init_module);

/* shutdown/cleanup function for the module */
void __exit dahdi_warp_cleanup_module(void)
{
	struct warp_analog *wa = dahdi_warp_analog_devptr(0);
	PDEVICE_EXTENSION pdx;
	int ii;

	if (!wa) {
		printk(KERN_ERR "%s():there must always be one module (FXS)\n",
			__FUNCTION__);
		return;
	}

	/* get pointer to the top struct */	
	pdx = wa->devext;

	/* remove yourself from dma callback list */
	warp_dma_disable(dahdi_warp_dma_isr);

	/* unregister all spans from dahdi */
	for (ii = 0; ii < dahdi_warp_analog_devcount(); ii++) {
		wa = dahdi_warp_analog_devptr(ii);
		if ((wa) && (wa->usecount == 0)) {
			dahdi_warp_analog_release(wa);
		}
	}

	free_irq(pdx->info.irql, pdx);
}

module_exit(dahdi_warp_cleanup_module);

/* add debug module parameter here */
module_param(fxo_txgain, int, 0600);
module_param(fxo_rxgain, int, 0600);
module_param(fxs_txgain, int, 0600);
module_param(fxs_rxgain, int, 0600);
module_param(alawoverride, int, 0600);
module_param(opermode, charp, 0600);
module_param(reversepolarity, int, 0600);

MODULE_DESCRIPTION("WARP Analog FXS/FXO Driver");
MODULE_AUTHOR("Utku Karaaslan <utku.karaaslan@pikatech.com>");
MODULE_LICENSE("GPL v2");

/*
 * Force kernel coding standard on PIKA code
 * Local Variables:
 * tab-width: 8
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
