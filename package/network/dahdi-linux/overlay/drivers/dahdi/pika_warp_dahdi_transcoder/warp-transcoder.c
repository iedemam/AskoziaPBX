/* WARP Transcoder Driver 
 * Copyright (C) 2010, PIKA Technologies Inc.
 *
 * All rights reserved.
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
#include <linux/slab.h>
#include <linux/kmod.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/moduleparam.h>
#include <linux/firmware.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/etherdevice.h>
#include <linux/timer.h>
#include <linux/kfifo.h>

#include "dahdi/kernel.h"
#include "warp-dsp.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
#include <asm/io.h>
#else
#include <linux/io.h>
#endif

/* For now, we support one and only one channel format/type */
#define SUPPORTED_FORMATS	(DAHDI_FORMAT_G729A)
#define SUPPORTED_CHANNELS	12
#define SUPPORTED_FORMAT_NAME	"G.729a"

#define WARP_INVALID_CHANNEL	(u16)-1

#define WARP_TC_READY		1
#define WARP_TC_SHUTDOWN	2

/* Modify these to match with whatever 3rd party API has */
#define WARP_TC_MULAW		(0)
#define WARP_TC_G729A		(17)
#define WARP_TC_UNDEF		(127)

#define G729_FRAME_SIZE		10
#define PCMU_FRAME_SIZE		80

#define G729_PAYLOAD_SIZE	20
#define DEFAULT_DSP_ID		0

struct warp_transcoder;

struct warp_channel_pvt {
	spinlock_t lock;		/* lock to access the struct */
	struct warp_transcoder *wt;	/* pointer to transcoder struct */
	u32 channel;			/* channel number we're operating on */
	u32 encoder;			/* flag to indicate whether we're encoder or decoder */
	u16 timeslot_in_num;		/* "timeslot" to receive from */
	u16 timeslot_out_num;		/* "timeslot" to send to */
	u16 chan_num;			/* 3rd party API channel identifier */
};

struct warp_transcoder {
	/* add all elements as necessary */
	int numchannels;
	unsigned long flags;
	atomic_t open_channels;
	struct semaphore chansem;
	struct dahdi_transcoder *dencode;
	struct dahdi_transcoder *ddecode;
	struct warp_channel_pvt *encoders;
	struct warp_channel_pvt *decoders;
};

static struct warp_transcoder *pwtc = NULL;

static void
dahdi_warp_cleanup_channel_private(struct warp_transcoder *wtc, struct dahdi_transcoder_channel *dtc);

static void
dahdi_warp_init_channel_state(struct warp_channel_pvt *pvt, int encoder, 
	unsigned int channel, struct warp_transcoder *wt)
{
	dahdi_warp_log(DSP_TCODER_LOG, "%s() called with encoder = (%d) channel = (%d)\n",
			__FUNCTION__, encoder, channel);

	memset(pvt, 0, sizeof(*pvt));
	pvt->encoder = encoder;
	pvt->channel = channel;
	pvt->wt = wt;
	pvt->chan_num = WARP_INVALID_CHANNEL;
	pvt->timeslot_in_num = channel * 2;
	pvt->timeslot_out_num = channel * 2;

	if (encoder) 
		++pvt->timeslot_out_num;
	else 
		++pvt->timeslot_in_num;

	spin_lock_init(&pvt->lock);
}

static int 
dahdi_warp_initialize_channel_pvt(struct warp_transcoder *wtc, int encoder,
		struct warp_channel_pvt **cpvt)
{
	int cc;

	dahdi_warp_log(DSP_TCODER_LOG, "%s() called with encoder = (%d) channel count = (%d)\n",
			__FUNCTION__, encoder, wtc->numchannels);

	*cpvt = kmalloc(sizeof(struct warp_channel_pvt) * wtc->numchannels,
			GFP_KERNEL);

	if (!(*cpvt)) {
		printk(KERN_ERR "%s() : insufficient memory for %d channels\n",
			__FUNCTION__, wtc->numchannels);
		return -ENOMEM;
	}

	for (cc = 0; cc < wtc->numchannels; cc++) {
		dahdi_warp_init_channel_state((*cpvt) + cc, encoder, cc, wtc);
	}
		
	return 0;
}

unsigned char
dahdifmt_to_warpfmt(unsigned int fmt)
{
	unsigned char res;

	dahdi_warp_log(DSP_TCODER_LOG, "%s() called with fmt = %d\n",
			__FUNCTION__, fmt);

	switch (fmt) {
		case DAHDI_FORMAT_ULAW:
			res = WARP_TC_MULAW;
			break;
		case DAHDI_FORMAT_G729A:
			res = WARP_TC_G729A;
			break;
		default:
			res = WARP_TC_UNDEF;
			break;
	}

	return (res);
}

static int
dahdi_warp_create_channel_pair(struct warp_transcoder *wtc, 
	struct warp_channel_pvt *pvt,
	u8 warp_srcfmt, u8 warp_dstfmt)
{
	struct warp_channel_pvt *encoder_pvt, *decoder_pvt;
	u16 encoder_timeslot;
	u16 decoder_timeslot;
	u32 enc_chan_id;
	u32 dec_chan_id;

	dahdi_warp_log(DSP_TCODER_LOG, "%s() called with encoder = %d srcfmt = %x dstfmt = %x\n",
			__FUNCTION__, pvt->encoder, warp_srcfmt, warp_dstfmt);

	BUG_ON(!wtc);
	BUG_ON(!pvt);

	if (pvt->encoder) {
		encoder_timeslot = pvt->timeslot_in_num;
		decoder_timeslot = pvt->timeslot_out_num;
	} else {
		u8 tmpval;
		encoder_timeslot = pvt->timeslot_out_num;
		decoder_timeslot = pvt->timeslot_in_num;
		tmpval = warp_srcfmt;
		warp_srcfmt = warp_dstfmt;
		warp_dstfmt = tmpval;
	}

	BUG_ON(encoder_timeslot/2 >= wtc->numchannels);
	BUG_ON(decoder_timeslot/2 >= wtc->numchannels);

	encoder_pvt = wtc->dencode->channels[encoder_timeslot/2].pvt;
	decoder_pvt = wtc->ddecode->channels[decoder_timeslot/2].pvt;

	BUG_ON(!encoder_pvt);
	BUG_ON(!decoder_pvt);

	/* Call the 3rd Party API function (via the DSP driver) to create and activate the (Encoder,Decoder) Pair. */
	if (dahdi_warp_open_transcoder_channel_pair(DEFAULT_DSP_ID, &enc_chan_id, &dec_chan_id) == 0) {
		dahdi_warp_log(DSP_TCODER_LOG, "%s() : Channel Pair (enc,dec) = (%d,%d) Created Successfully\n",
			__FUNCTION__, enc_chan_id, dec_chan_id);	
		encoder_pvt->chan_num = enc_chan_id;
		decoder_pvt->chan_num = dec_chan_id;
	} else {
		printk(KERN_ERR "%s() : Error : Could NOT create channel pair successfully.\n",
			__FUNCTION__);	
		encoder_pvt->chan_num = WARP_INVALID_CHANNEL;
		decoder_pvt->chan_num = WARP_INVALID_CHANNEL;
		return -1;
	}

	return 0;
}

static int
dahdi_warp_destroy_channel_pair(struct warp_transcoder *wtc,
	struct warp_channel_pvt *pvt)
{
	struct dahdi_transcoder_channel *dtc1, *dtc2;
	struct warp_channel_pvt *encoder_pvt, *decoder_pvt;
	int ts1, ts2;

	dahdi_warp_log(DSP_TCODER_LOG, "%s() called with encoder = %d\n",
			__FUNCTION__, pvt->encoder);

	if (pvt->encoder) {
		ts1 = pvt->timeslot_in_num;
		ts2 = pvt->timeslot_out_num;
	} else {
		ts1 = pvt->timeslot_out_num;
		ts2 = pvt->timeslot_in_num;
	}

	if (ts1/2 >= wtc->numchannels || ts2/2 >= wtc->numchannels) {
		printk(KERN_ERR "%s() : invalid channel numbers chan1:%d chan2:%d\n",
			__FUNCTION__, ts1, ts2);
		return 0;
	}

	dtc1 = &(wtc->dencode->channels[ts1/2]);
        dtc2 = &(wtc->ddecode->channels[ts2/2]);
        encoder_pvt = dtc1->pvt;
        decoder_pvt = dtc2->pvt;

	if (dahdi_warp_release_transcoder_channel_pair(DEFAULT_DSP_ID,
						decoder_pvt->chan_num,
						encoder_pvt->chan_num) == 0) {
		dahdi_warp_log(DSP_TCODER_LOG, "%s() Channel Pair (enc,dec) = (%d,%d) Destroyed Successfully\n",
				__FUNCTION__, encoder_pvt->chan_num, decoder_pvt->chan_num);
	} else {
		printk(KERN_ERR "%s() : Error : Could NOT Destroy Channel Pair (enc,dec) = (%d,%d)\n",
			__FUNCTION__, encoder_pvt->chan_num, decoder_pvt->chan_num);	
	}

	return 0;
}

static int
dahdi_warp_mark_channel_complement_built(struct warp_transcoder *wtc,
	struct dahdi_transcoder_channel *dtc)
{
	int index;
	struct warp_channel_pvt *pvt = dtc->pvt;
	struct dahdi_transcoder_channel *compl_dtc;
	struct warp_channel_pvt *compl_pvt;

	BUG_ON(!pvt);
	index = pvt->timeslot_in_num/2;
	BUG_ON(index >= wtc->numchannels);

	dahdi_warp_log(DSP_TCODER_LOG, "%s() called with channel number = %d srcfmt = %x dstfmt = %x\n",
			__FUNCTION__, pvt->chan_num, dtc->srcfmt, dtc->dstfmt);

	/* find the "complement" channel */
	if (pvt->encoder) 
		compl_dtc = &(wtc->ddecode->channels[index]);
	else
		compl_dtc = &(wtc->dencode->channels[index]);

	WARN_ON(dahdi_tc_is_built(compl_dtc));
	compl_dtc->built_fmts = dtc->dstfmt | dtc->srcfmt;
	compl_pvt = compl_dtc->pvt;
	dahdi_warp_log(DSP_TCODER_LOG, "%s() %p is the complement to %p (CN : %d)\n",
			__FUNCTION__, compl_dtc, dtc, pvt->chan_num);

	dahdi_tc_set_built(compl_dtc);
	dahdi_warp_cleanup_channel_private(wtc, dtc);

	return 0;
}

static void
dahdi_warp_cleanup_channel_private(struct warp_transcoder *wtc, struct dahdi_transcoder_channel *dtc)
{
	struct warp_channel_pvt *pvt = dtc->pvt;

	dahdi_warp_log(DSP_TCODER_LOG, "%s() called\n",
			__FUNCTION__);
	BUG_ON(!pvt);
	dahdi_warp_log(DSP_TCODER_LOG, "%s() chan_num = %d\n",
			__FUNCTION__, pvt->chan_num);

	if (pvt->chan_num != WARP_INVALID_CHANNEL) {
		dahdi_warp_cleanup_transcoder_channel(DEFAULT_DSP_ID, pvt->chan_num);
	}
}

static int
dahdi_warp_channel_allocate(struct dahdi_transcoder_channel *dtc)
{
	struct warp_channel_pvt *pvt = (struct warp_channel_pvt *)dtc->pvt;
	struct warp_transcoder *wtc = pvt->wt;
	unsigned char warp_srcfmt, warp_dstfmt; 
	int res;

	dahdi_warp_log(DSP_TCODER_LOG, "%s() called with channel pair srcfmt 0x%08x dstfmt 0x%08x\n",
			__FUNCTION__, dtc->srcfmt, dtc->dstfmt);

	/* grab the channel semaphore */
	if (down_interruptible(&wtc->chansem))
		return -EINTR;

	/* while we're waiting for the channel semaphore, a previous call may have 
	 * already built this channel as a complement to itself. 
	 */
	if (dahdi_tc_is_built(dtc)) {
		up(&wtc->chansem);
		dahdi_warp_log(DSP_TCODER_LOG, "%s() allocating channel %p (already built).\n",
			__FUNCTION__, dtc);
		return 0;
	}

	/* cleanup the rx|tx queues for this channel */
	if (pvt->chan_num != WARP_INVALID_CHANNEL) {
		dahdi_warp_cleanup_channel_private(wtc, dtc);
	}
	
	dahdi_warp_log(DSP_TCODER_LOG, "%s() : allocating a new channel: %p.\n", 
		__FUNCTION__, dtc);

	warp_srcfmt = dahdifmt_to_warpfmt(dtc->srcfmt);
	warp_dstfmt = dahdifmt_to_warpfmt(dtc->dstfmt);
	res = dahdi_warp_create_channel_pair(wtc, pvt, warp_srcfmt, warp_dstfmt);

	if (res) {
		printk(KERN_ERR "%s() : error creating channel pair for (fmt:%02x <-> fmt:%02x)\n",
			__FUNCTION__, warp_srcfmt, warp_dstfmt);
		up (&wtc->chansem);
		return (res);
	}

	/* mark this channel as built */
	dahdi_tc_set_built(dtc);
	dtc->built_fmts = dtc->dstfmt | dtc->srcfmt;
	dahdi_warp_log(DSP_TCODER_LOG, "%s() : channel %p has dstfmt = %x and srcfmt = %x\n",
			__FUNCTION__, dtc, dtc->srcfmt, dtc->dstfmt);

	/* mark the channel complement as built */
	res = dahdi_warp_mark_channel_complement_built(wtc, dtc);
	up(&wtc->chansem);

	dahdi_transcoder_alert(dtc);
	return (res);
}


static int 
dahdi_warp_operation_allocate(struct dahdi_transcoder_channel *dtc)
{
	struct warp_transcoder *wtc = ((struct warp_channel_pvt *)(dtc->pvt))->wt;

	dahdi_warp_log(DSP_TCODER_LOG, "%s() called\n",
			__FUNCTION__);

	if (unlikely(test_bit(WARP_TC_SHUTDOWN, &wtc->flags))) {
		printk(KERN_ERR "%s() : shutting down... no more new channels!\n",
			__FUNCTION__);
		return -EIO;
	}
	
	atomic_inc(&wtc->open_channels);

	if (dahdi_tc_is_built(dtc)) {
		printk(KERN_DEBUG "%s() : allocating channel %p (already built).\n",
			__FUNCTION__, dtc);
		return 0;
	}

	return dahdi_warp_channel_allocate(dtc);
}


static int
dahdi_warp_operation_release(struct dahdi_transcoder_channel *dtc)
{
	struct dahdi_transcoder_channel *compl_dtc;
	struct warp_channel_pvt *compl_pvt;
	struct warp_channel_pvt *pvt = dtc->pvt;
	struct warp_transcoder *wtc = pvt->wt;
	int res;
	int index;

	dahdi_warp_log(DSP_TCODER_LOG, "%s() called\n",
			__FUNCTION__);

	BUG_ON(!pvt);
	BUG_ON(!wtc);
	
	if (unlikely(test_bit(WARP_TC_SHUTDOWN, &wtc->flags))) {
		printk(KERN_DEBUG "%s() : shutting down... no more new channels!\n",
			__FUNCTION__);
		return -EIO;
	}

	if (down_interruptible(&wtc->chansem))
		return -EINTR;

	atomic_dec(&wtc->open_channels);

	dahdi_warp_cleanup_channel_private(wtc, dtc);
	index = pvt->timeslot_in_num/2;
	BUG_ON(index >= wtc->numchannels);
	if (pvt->encoder)
		compl_dtc = &(wtc->ddecode->channels[index]);
	else
		compl_dtc = &(wtc->dencode->channels[index]);
	BUG_ON(!compl_dtc);
	if (!dahdi_tc_is_built(compl_dtc)) {
		printk(KERN_ERR "%s() : releasing a channel that was never built.\n",
			__FUNCTION__);
		res = 0;
		goto error_exit;
	}

	if (dahdi_tc_is_busy(compl_dtc)) {
		res = 0;
		goto error_exit;
	}

	res = dahdi_warp_destroy_channel_pair(wtc, pvt);
	if (res)
		goto error_exit;

	dahdi_warp_log(DSP_TCODER_LOG, "%s() : releasing channel %p\n", 
		__FUNCTION__, dtc);

	/* mark channel as not built */
	dahdi_tc_clear_built(dtc);
	dtc->built_fmts = 0;
	pvt->chan_num = WARP_INVALID_CHANNEL; 

	/* mark the complement channel as not built */
	dahdi_tc_clear_built(compl_dtc);
	compl_dtc->built_fmts = 0;
	compl_pvt = compl_dtc->pvt;
	compl_pvt->chan_num = WARP_INVALID_CHANNEL;
	
error_exit:

	up(&wtc->chansem);		
	return res;
}

/* this function is to read a frame that's already been "transcoded" */
/* note that encoding and decoding are both transcoding ops. */
static ssize_t
dahdi_warp_transcoder_read(struct file *file, char __user *frame, size_t count, loff_t *ppos)
{	
	struct dahdi_transcoder_channel *dtc = file->private_data;
	struct warp_channel_pvt *pvt = dtc->pvt;
	struct warp_transcoder *wt = pvt->wt;
	u8 audio_frame[PCMU_FRAME_SIZE * 2];
	ssize_t ret;
	u16 buffer_size;
	u32 noblock = 0;

	dahdi_warp_log(DSP_TCODER_LOG, "%s() called\n",
			__FUNCTION__);

	BUG_ON(!dtc);
	BUG_ON(!pvt);
	BUG_ON(!wt);

	buffer_size = (pvt->encoder) ? G729_FRAME_SIZE : PCMU_FRAME_SIZE;
	buffer_size <<= 1;

	dahdi_warp_log(DSP_TCODER_LOG, "%s()channel number = (%d) encoder = (%d) buffer size = (%d)\n",
			__FUNCTION__, pvt->chan_num, pvt->encoder, buffer_size);

	if (likely(dahdi_warp_receive_packet_payload(DEFAULT_DSP_ID,
					pvt->chan_num, 
					pvt->encoder,
					audio_frame,
					buffer_size,
					noblock) == 0)) {
		if (unlikely(copy_to_user(frame, audio_frame, buffer_size))) {
			if (printk_ratelimit()) {
				printk(KERN_ERR "%s() : Error Copying %d Octets from Channel (%d)\n",
						__FUNCTION__, count, pvt->chan_num);
			}
			ret = -EFBIG;
		} else {
			ret = buffer_size;
			dahdi_transcoder_alert(dtc);
		}
	} else {
		if (printk_ratelimit()) {
			printk(KERN_ERR "%s() : Error Reading Packet Payload for Channel (%d)\n",
				__FUNCTION__, pvt->chan_num);
		}
		ret = -EFBIG;
	}

	return (ret);
}

static ssize_t
dahdi_warp_transcoder_write(struct file *file, const char __user *frame, 
	size_t count, loff_t *ppos)
{
	struct dahdi_transcoder_channel *dtc = file->private_data;
	struct warp_channel_pvt *pvt = dtc->pvt;
	struct warp_transcoder *wt = pvt->wt;
	char audio_frame[PCMU_FRAME_SIZE * 2];
	int frame_size;
	int queue, ii;

	dahdi_warp_log(DSP_TCODER_LOG, "%s() called\n",
			__FUNCTION__);

	/* do the usual checks */
	BUG_ON(!dtc);
	BUG_ON(!pvt);
	BUG_ON(!wt);

	if ((count > sizeof(audio_frame)) || (copy_from_user(audio_frame, frame, count))) {
		if (printk_ratelimit()) {
			printk(KERN_ERR "%s() : Error copying (%d) octets to channel (%d) audio_frame_size = %d\n",
				__FUNCTION__, count, pvt->chan_num, sizeof(audio_frame));
		}
		return -E2BIG;
	}

	frame_size = (pvt->encoder) ? PCMU_FRAME_SIZE : G729_FRAME_SIZE;

	dahdi_warp_log(DSP_TCODER_LOG, "%s()channel number = (%d) encoder = (%d) frame size = (%d) count = (%d)\n",
			__FUNCTION__, pvt->chan_num, pvt->encoder, frame_size, count);

	queue = 1;
	for (ii = 0; ii < count; ii += frame_size) {
		if (dahdi_warp_send_packet_payload(DEFAULT_DSP_ID, pvt->chan_num, pvt->encoder, &audio_frame[ii], frame_size, queue) != 0) {
			if (printk_ratelimit()) {
				printk(KERN_ERR "%s() : Error Sending Frame to Channel (%d)\n",
					__FUNCTION__, pvt->chan_num);
			}
		return -EIO;
		}
	}

	return (count);
}

static void
dahdi_warp_setup_file_operations(struct file_operations *fops)
{
	dahdi_warp_log(DSP_TCODER_LOG, "%s() called\n",
			__FUNCTION__);

	fops->owner = THIS_MODULE;
	fops->read = dahdi_warp_transcoder_read;
	fops->write = dahdi_warp_transcoder_write;
}

static int
dahdi_warp_initialize_transcoder(struct warp_transcoder *wtc, unsigned int sourcefmts, 
		unsigned int destfmts, struct warp_channel_pvt *pvts,
		struct dahdi_transcoder **zt)
{
	int chan;

	dahdi_warp_log(DSP_TCODER_LOG, "%s() called\n",
			__FUNCTION__);

	*zt = dahdi_transcoder_alloc(wtc->numchannels);
	if (!(*zt))
		return -ENOMEM;

	(*zt)->srcfmts = sourcefmts;
	(*zt)->dstfmts = destfmts;
	(*zt)->allocate = dahdi_warp_operation_allocate;
	(*zt)->release = dahdi_warp_operation_release;
	dahdi_warp_setup_file_operations(&((*zt)->fops));
	for (chan = 0; chan < wtc->numchannels; ++chan)
		(*zt)->channels[chan].pvt = &pvts[chan];
	return 0;
}

static int 
dahdi_warp_initialize_encoders(struct warp_transcoder *wtc, 
			unsigned int complexfmts)
{
	int res;

	dahdi_warp_log(DSP_TCODER_LOG, "%s() called\n",
			__FUNCTION__);

	res = dahdi_warp_initialize_channel_pvt(wtc, 1, &wtc->encoders);
	if (res)
		return res;

	res = dahdi_warp_initialize_transcoder(wtc, 
		DAHDI_FORMAT_ULAW,
		complexfmts, wtc->encoders, &wtc->dencode);
	if (res)
		return res;

	sprintf(wtc->dencode->name, "DTE Encoder");

	return res;
}

static int
dahdi_warp_initialize_decoders(struct warp_transcoder *wtc,
		unsigned int complexfmts)
{
	int res;

	dahdi_warp_log(DSP_TCODER_LOG, "%s() called\n",
			__FUNCTION__);

	res = dahdi_warp_initialize_channel_pvt(wtc, 0, &wtc->decoders);
	if (res)
		return res;

	res = dahdi_warp_initialize_transcoder(wtc,
		complexfmts,
		DAHDI_FORMAT_ULAW, wtc->decoders, &wtc->ddecode);
	if (res)
		return res;

	sprintf(wtc->ddecode->name, "DTE Decoder");

	return res;
}


static int 
__init dahdi_warp_transcoder_init(void)
{
	unsigned int complexformats = SUPPORTED_FORMATS;
	struct warp_transcoder *wtc = NULL;
	int res;

	wtc = kzalloc(sizeof(*wtc), GFP_KERNEL);
	if (!wtc)
		return -ENOMEM;

	wtc->numchannels = SUPPORTED_CHANNELS;
	init_MUTEX(&wtc->chansem);
	res = dahdi_warp_initialize_encoders(wtc, complexformats);
	if (res)
		goto sw_error_cleanup;
	res = dahdi_warp_initialize_decoders(wtc, complexformats);
	if (res)
		goto sw_error_cleanup;

	dahdi_transcoder_register(wtc->dencode);
	dahdi_transcoder_register(wtc->ddecode);

	pwtc = wtc;

	return 0;

sw_error_cleanup:	

	kfree(wtc->encoders);
	kfree(wtc->decoders);
	dahdi_transcoder_free(wtc->dencode);
	dahdi_transcoder_free(wtc->ddecode);
	kfree(wtc);

	return (res);
}

static void __exit dahdi_warp_transcoder_cleanup(void)
{
	if (dahdi_warp_release_all_transcoder_channels(DEFAULT_DSP_ID)) {
		printk(KERN_ERR "%s() failed to release all transcoder channels\n",
			__FUNCTION__);
	}

	if (pwtc) {

		dahdi_transcoder_unregister(pwtc->ddecode);
		dahdi_transcoder_unregister(pwtc->dencode);

		dahdi_transcoder_free(pwtc->dencode);
		dahdi_transcoder_free(pwtc->ddecode);

		kfree(pwtc->encoders);
		kfree(pwtc->decoders);
		kfree(pwtc);
	}
}

MODULE_LICENSE("GPL");
module_init(dahdi_warp_transcoder_init);
module_exit(dahdi_warp_transcoder_cleanup);
