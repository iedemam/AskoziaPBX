/**********************************************************************************/
/* DSP Device Driver for WARP HYBRID aka Open Source WARP                         */
/* (c) 2010 Pika Technologies Inc.                                                */
/**********************************************************************************/

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
#include <linux/timer.h>
#include <linux/kfifo.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/cdev.h>
#include <linux/vmalloc.h>
#include <linux/of_gpio.h>
#include <asm/uaccess.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 18)
#include <asm/io.h>
#else
#include <linux/io.h>
#endif

#include "GpakCust.h"
#include "GpakApi.h"

#include "warp-dsp.h"
#include "warp-dsp-ioctl.h"
#include "warp-dsp-defs.h"

#define DEFAULT_DSP_ID				0
#define MAX_DSP_COUNT				1
#define MAX_TC_COUNT				15
#define MAX_EC_COUNT				17
#define MAX_CHANNEL_COUNT			42
#define MAX_MSGS_TO_PROCESS			8
#define MAX_STAT_CMD_LEN			64
#define MAX_STAT_TYPE_LEN			64
#define MAX_PKT_SIZE				160
#define MAX_PKT_COUNT 				50
#define MAX_FRAME_BLOCK_SIZE			256
#define MIN_FRAME_BLOCK_SIZE	 		64
#define STAT_CMD_COUNT				2
#define DEBUG_CMD_COUNT				3
#define CHANNEL_CMD_COUNT			3
#define DSP_DELAY_IN_US				100
#define INVALID_CHANNEL_ID			(unsigned short)-1
#define TCODER_READ_TIMEOUT			10
#define DSP_FRAME_LENGTH			10
#define ALG_CONTROL_WAIT	 		(3 * DSP_FRAME_LENGTH)
#define ALG_CONTROL_RETRY_COUNT			3
#define FRAMES_PER_INTR				6
#define MAX_PURGE_COUNT				5000
#define MAX_ISR_MSEC				10
#define MAX_RX_QUEUE_LENGTH			250
#define MAX_TX_QUEUE_LENGTH			250
#define DSP_COMMAND_TIMEOUT_JIFFIES 		2000
#define DELAY_FOR_WRITE_READY			0
#define DELAY_FOR_READ_READY			1
#define MAX_DATA_BUFSZ				32
#define MIN_INFO_COUNT				6

#define MAX_BUF_COUNT				64
#define MAX_WARN_COUNT				3

#define DSP_PROC_ENTRY				"driver/dsp"
#define	G729_FRAME_SIZE				10
#define PCMU_FRAME_SIZE				80
#define EVENT_FIFO_MAX_MSGS			16

#define PKT_TX_READY				0
#define PKT_RX_READY				1
#define CHAN_READY				2
#define CHAN_SHUTDOWN				3

#define HANDSHAKE_COUNT         		4
#define HANDSHAKE_OFFSET        		0x00000400
#define HANDSHAKE_PATTERN_WORD  		0xABCD
#define HANDSHAKE_RESPONSE_WORD 		0xDCBA
#define IMAGE_DOWNLOAD_OFFSET   		0x00050000
#define IMAGE_DOWNLOAD_FLAG     		0xDCBA
#define STAT_CHECK_COUNT			100

#define MODULE_A_FIRST_CHAN			2
#define MODULE_B_FIRST_CHAN			6

#define MIN_TCODER_B_SIDE_CHAN_NUM_RANGE1	0
#define MAX_TCODER_B_SIDE_CHAN_NUM_RANGE1	13

#define MIN_TCODER_A_SIDE_CHAN_NUM_RANGE1	32
#define MAX_TCODER_A_SIDE_CHAN_NUM_RANGE1	41

#define MIN_TCODER_A_SIDE_CHAN_NUM_RANGE2	14
#define MAX_TCODER_A_SIDE_CHAN_NUM_RANGE2	31

#define MIN_ECHOCAN_CHAN_NUM_RANGE1		14
#define MAX_ECHOCAN_CHAN_NUM_RANGE1		31

#define DSP_DEFERRED_SEND			1
#define DSP_DOWNLOAD_CHECK_MAX_TRIES 		3

#define PIK_98_00910_NO_OBFXS			0	// 910 without Onboard FXS populated
#define PIK_98_00910				1	// all population
#define PIK_98_00920_FULL			2	// all population options on 920
#define PIK_98_00920_NO_USB2			3	// USB2.0 not populated
#define PIK_98_00920_NO_RTC			4	// Real Time Clock not populated
#define PIK_98_00920_NO_RTC_USB2		5	// Real Time Clock and USB2.0 not populated
#define PIK_98_00920_NO_OBFXS			6	// FXS depopulated

#define USE_SEMAPHORE_WAIT			1	// Use semaphores for efficient busy wait

#define MODULE_TYPE_FXO         		0x01
#define MODULE_TYPE_FXS         		0x02
#define MODULE_TYPE_BRI         		0x03
#define MODULE_TYPE_GSM         		0x04
#define MODULE_TYPE_NONE        		0x0F

/* procfs parameters */

enum {  
	AVAILABLE = 0, 
	BUSY = 1
};

enum {
	UNSET = 0,
	ECHOCAN = 1,
	TRANSCODER = 2
};

enum {
	TRANSCODER_A_SIDE = 0,
	TRANSCODER_B_SIDE = 1
};

struct warp_dsp_port_stats {
	unsigned int status;		/* mcbsp stats for port1 */
	unsigned int rxints;		/* todo put them in a struct */
	unsigned int rxslips;
	unsigned int rxdmaerrs;
	unsigned int txints;
	unsigned int txslips;
	unsigned int txdmaerrs;
	unsigned int frameerrs;
	unsigned int restarts;
};

struct warp_dsp {
	unsigned int id;			/* dsp id */
	unsigned int irq;			/* dsp irq */
	int dsp_ready;				/* ready flag for the dsp */
	void *hp_address;			/* host port address */
	unsigned long flags;			/* flags to indicate status */
	wait_queue_head_t wq;			/* wait queue */
	struct semaphore access_sem;		/* access control semaphore */
	struct semaphore cmd_access_sem;	/* command access control semaphore */
	struct semaphore dsp_cmd_write_sem;	/* dsp command write semaphore */
	struct semaphore dsp_cmd_read_sem;	/* dsp command read semaphore */
	atomic_t in_use;			/* flag to check if driver is in use */
	atomic_t tcoder_count;			/* number of transcoders */
	atomic_t ecan_count;			/* number of echo cancellers */
	struct kfifo *event_fifo;		/* event fifo to keep events in */
	unsigned int stat_check_count;		/* channel status check counter */
	unsigned long average_util;		/* average utilization */
	unsigned long peak_util;		/* peak utilization */
	struct warp_dsp_port_stats port1;	/* port1 mcbsp stats */
	struct warp_dsp_port_stats port2;	/* port2 mcbsp stats */
	unsigned long max_irq_time;		/* maximum time spent in irq in msecs */
	unsigned long avg_irq_time;		/* average time spent in irq in msecs */
};

struct chan_stats {
	unsigned int pkt_sent;			/* number of sent packets for this channel */
	unsigned int pkt_recvd;			/* number of received packets for this channel */
	unsigned int pkt_dropped;		/* number of dropped/errored packets for this channel */
	unsigned int pkt_nopayload;		/* number of times when no packet was received */
	unsigned int write_count;		/* number of packets written */
	unsigned int read_count;		/* number of packets read (or to be read) */
	unsigned int pkt_queue_count;		/* number of queued packets */
	unsigned int pkt_sent_count;		/* number of sent packets */
	unsigned int pkt_int_rx_queued;		/* number of packets received from ISR */
	unsigned int pkt_int_rx_recvd;		/* number of packets received by app */
	unsigned int pkts_to_dsp;		/* number of packets sent to dsp */
	unsigned int pkts_from_dsp;		/* number of packets received from dsp */
	unsigned int pkts_bad_header;		/* number of packets with bad header */
	unsigned int pkts_underflow_cnt;	/* packet underflow count */
	unsigned int pkts_overflow_cnt;		/* packet overflow count */
	unsigned int pkts_none_sent;		/* number of times no packet was sent in ISR */
	unsigned int pkts_none_recvd;		/* number of times no packet was recvd in ISR */

};

struct chan_buf {
	unsigned short length;
	unsigned int encoding;
	unsigned char data[MAX_PKT_SIZE];
};

struct chan {
	unsigned int status;			/* status */
	unsigned int type;			/* type (tdm-to-tdm, packet-to-packet) */
	unsigned long flags;			/* flags to indicate channel status */
	unsigned int encoder;			/* whether it's encoder or decoder */
	unsigned short paired_channel;		/* paired channel id (where to read from for this channel) */
	wait_queue_head_t wq;			/* wait queue for read operations */
	spinlock_t dsp2cpu_lock;		/* lock to access the rx list */
	spinlock_t cpu2dsp_lock;		/* lock to access the tx list */
	struct chan_buf cpu2dsp_buf[MAX_PKT_COUNT];
	unsigned int cpu2dsp_wp;		/* cpu to dsp write pointer */
	unsigned int cpu2dsp_rp;		/* cpu to dsp read pointer */
	unsigned int cpu2dsp_len;		/* cpu to dsp buffer length */
	struct chan_buf dsp2cpu_buf[MAX_PKT_COUNT];
	unsigned int dsp2cpu_wp;		/* dsp to cpu write pointer */
	unsigned int dsp2cpu_rp;		/* dsp to cpu  read pointer */
	unsigned int dsp2cpu_len;		/* dsp to cpu  buffer length */
	struct chan_stats stats;		/* statistics for this channel */
	unsigned char rx_buf[MAX_PKT_SIZE];	/* temporary receive buffer */
	unsigned short rx_buf_offset;		/* receive buffer offset */
	unsigned int ecan_timeslot;		/* echo cancellation timeslot */
	unsigned int discard_frames;		/* frames to discard from output as there was no input */
	unsigned int warn_count;		/* warning count */
};

struct warp_dsp_module {
	void *fpga;
	void *dsp_host_port;
	unsigned int irq;
	spinlock_t list_spinlock;		/* list spinlock */
	struct device class_dev;		/* class device */
	struct cdev cdev;			/* cdev struct */
	struct chan chans[MAX_CHANNEL_COUNT];	/* channel array --> should this be for each dsp? yes! */
	struct warp_dsp dsp[MAX_DSP_COUNT];	/* array of DSPs (for future expansion) */
	unsigned long isr_count;		/* number of ISRs */
};

struct dsp_fifo_event {
	unsigned short channel_id;
	GpakAsyncEventCode_t event_code;
	GpakAsyncEventData_t event_data;
};

/* dsp control commands */
struct dsp_args {
	unsigned int dsp_id;
	unsigned int param1;
	unsigned int param2;
	unsigned int param3;
	unsigned int result;
};

static int dsp_open(struct inode *inode, struct file *file);
static int dsp_release(struct inode *inode, struct file *file);
static ssize_t dsp_read(struct file *file, char *buf, size_t count, loff_t *offset);
static int dsp_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);

const struct file_operations dsp_ops = {
	.owner		= THIS_MODULE,
	.open		= dsp_open,
	.release	= dsp_release,
	.read		= dsp_read,
	.ioctl		= dsp_ioctl,
};

static struct warp_dsp_module *dsp_module;
static unsigned char *warp_dsp_firmware;
static unsigned int warp_dsp_firmware_size;

static struct class dsp_class;
static char dsp_class_name[] = "dsp";

static dev_t dsp_dev;

static int dsp_debug_flags = 0;
static unsigned int file_ptr = 0;

/* echo cancellation parameters */
static int taillength = 64;
static int nlptype = 6;
static int adaptenable = 1;
static int g165detenable = 0;
static int dbltalkthreshold = 4;
static int nlpthreshold = 24;
static int nlpupperlimitthresh = 18;
static int nlpupperlimitprethresh = 12;
static int nlpmaxsupp = 10;
static int nlpcngthreshold = -43;
static int nlpadaptlimit = 50;
static int crosscorrlimit = 15;
static int numfirsegments = 3;
static int firsegmentlen = 4;
static int firtapcheckperiod = 10;
static int maxdoubletalkthreshold = 40;
static int mixedfourwiremode = 0;
static int reconvcheckenable = 1;
static int test_chan_count = 0;

static void dahdi_warp_check_event_fifo(struct warp_dsp_module *dspm);
static void dahdi_warp_recv_transcoded_frames(struct warp_dsp_module *dspm);
static void dahdi_warp_send_raw_frames(struct warp_dsp_module *dspm);
int dahdi_warp_cleanup_transcoder_channel(unsigned int dsp_id, unsigned int channel_id);
static int dahdi_warp_reset_channel_stats(unsigned int dsp_id, unsigned int channel_id);
static void dahdi_warp_reset_dsp(struct warp_dsp_module *dspm);
static int dahdi_warp_load_dsp_firmware_cmd(struct dsp_args *pargs);
static int dahdi_warp_configure_dsp_ports(struct warp_dsp_module *dspm);
static void dahdi_warp_set_dsp_system_parameters(unsigned int dspid);
static void warp_dsp_execute(void);
static void set_ebc_dsp_slow(void);
static void set_ebc_dsp_fast(void);
static void warp_dsp_write_hpic(unsigned short value);
inline void warp_dsp_read_hpid(unsigned int offset, unsigned short *value);
inline void warp_dsp_write_hpid(unsigned int offset, unsigned short value);
static int dahdi_warp_set_echocan_channel_id(unsigned int dsp_id, unsigned int in_timeslot, unsigned int chan_id);
static int dahdi_warp_get_channel(unsigned int dsp_id, unsigned int *new_channel_id, unsigned int channel_type, unsigned int channel_subtype);
static int dahdi_warp_enable_echocan(unsigned int dsp_id, unsigned int chan_id);
static int dahdi_warp_disable_echocan(unsigned int dsp_id, unsigned int chan_id);
static int dahdi_warp_alloc_echocan_channel(unsigned int dsp_id, unsigned int in_timeslot, unsigned int tap_length, unsigned int *channel_id);
static int dahdi_warp_free_echocan_channel(unsigned int dsp_id, unsigned int channel_id);
static int dahdi_warp_configure_channels(unsigned int dsp_id);
static void dahdi_warp_set_dsp_ready(int dsp_id, int flag);
static int dahdi_warp_get_dsp_ready(int dsp_id);
static int dahdi_warp_purge_channel_buffers(int dsp_id, int channel_id);
static int dahdi_warp_reset_system_stats(int dsp_id);
static int dahdi_warp_reset_cpu_stats(int dsp_id);
static int dahdi_warp_reset_packet_stats(int dsp_id, int channel_id);
static void dahdi_warp_enable_dsp_irq(void);
static void dahdi_warp_disable_dsp_irq(void);
int fpga_read_indirect(void __iomem *fpga, int channel, int reg, __u8 *value);
static int dahdi_warp_send_one_payload(unsigned short dsp_id, unsigned short channel_id, int encoding, unsigned char *payload, unsigned short payload_size);
static int dahdi_warp_check_channel_stats(struct warp_dsp_module *dspm);
static void warp_dsp_data_lock(unsigned short dsp_id);
static void warp_dsp_data_unlock(unsigned short dsp_id);
static void warp_dsp_check_dsp_command_status(struct warp_dsp_module *module);
static void get_module_info(unsigned *moda, unsigned *modb);

static int
dahdi_warp_check_capacity(int echo_count, int tc_count)
{
	const int capacity_table[MAX_EC_COUNT][MAX_TC_COUNT] = {
		// 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 
		 { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },// 0 
		 { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },// 1 
		 { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },// 2
		 { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 },// 3
		 { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 },// 4
		 { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 },// 5
		 { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 },// 6
		 { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 },// 7
		 { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 },// 8
		 { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 },// 9
		 { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 },// 10
		 { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 },// 11
		 { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 },// 12
		 { 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 },// 13
		 { 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 },// 14
		 { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },// 15
		 { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },// 16
	};
	int ret = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with (ec:%d,tc:%d)\n",
			__FUNCTION__, echo_count, tc_count);

	if ((echo_count < 0) || (echo_count >= MAX_EC_COUNT)) {
		printk(KERN_ERR "%s() : Invalid Echo Canceller Count %d\n",
			__FUNCTION__, echo_count);
		ret = -1;
	}

	if ((ret == 0) && ((tc_count < 0) || (tc_count >= MAX_EC_COUNT))) {
		printk(KERN_ERR "%s() : Invalid Transcoder Count %d\n",
			__FUNCTION__, tc_count);
		ret = -1;
	}

	if (ret == 0) {
		ret = capacity_table[echo_count][tc_count];
	} 

	dahdi_warp_log(DSP_FUNC_LOG, "%s() (ec:%d,tc:%d) capable = %d\n",
		__FUNCTION__, echo_count, tc_count, ret);

	return (ret);
}


static inline 
int limit_output(void)
{
	return 1;
}

void dahdi_warp_log(int flag, const char *format, ...)
{
	char buffer[1024];
	va_list args;

	if ((dsp_debug_flags & flag)) {
		va_start(args, format);
        	vsprintf(buffer, format, args);
	        va_end(args);
		printk(KERN_DEBUG "%s", buffer);
	}
}
EXPORT_SYMBOL(dahdi_warp_log);

static int
dsp_open(struct inode *inode, struct file *file)
{
	unsigned minor = MINOR(inode->i_rdev);
	
	dahdi_warp_log(DSP_FS_LOG, "%s() called\n", __FUNCTION__);
	
	if (atomic_inc_and_test(&dsp_module->dsp[minor].in_use)) {
		__kfifo_reset(dsp_module->dsp[minor].event_fifo);
		dahdi_warp_log(DSP_FS_LOG, "%s() resetting FIFO and returning success\n", __FUNCTION__);
		return 0;
	} else {
		printk(KERN_ERR "%s() : WARP DSP Device Busy.\n",
			__FUNCTION__);
		atomic_dec(&dsp_module->dsp[minor].in_use);
		dahdi_warp_log(DSP_FS_LOG, "%s() device busy returning -EBUSY\n", __FUNCTION__);
		return -EBUSY;
	}
}


static int
dsp_release(struct inode *inode, struct file *file)
{
	unsigned minor = MINOR(inode->i_rdev);
	
	dahdi_warp_log(DSP_FS_LOG, "%s() called\n", __FUNCTION__);
	atomic_dec(&dsp_module->dsp[minor].in_use);

	return 0;
}

static ssize_t
dsp_read(struct file *file, char *buf,
	size_t count, loff_t *offset)
{
	unsigned minor = iminor(file->f_dentry->d_inode);
	struct dsp_fifo_event tmp_fifo_event;
	unsigned int fifo_len;
	int event_ready = 0;
	ssize_t ret = 0;

	dahdi_warp_log(DSP_FS_LOG, "%s() called\n", __FUNCTION__);

	if (!dahdi_warp_get_dsp_ready(minor)) {
		printk(KERN_ERR "%s() : Dsp (%d) NOT Ready!\n",
			__FUNCTION__, minor);
		return (-ENODEV);
	}

	if (count < sizeof(struct dsp_fifo_event)) {
		dahdi_warp_log(DSP_FS_LOG, "%s() insufficient space returning -ENOBUFS\n",
			__FUNCTION__);
		return (-ENOBUFS);
	}

	fifo_len = __kfifo_len(dsp_module->dsp[minor].event_fifo);
	if (fifo_len > 0) {
		event_ready = 1;
		dahdi_warp_log(DSP_FIFO_LOG, "%s() FIFO event ready\n",
			__FUNCTION__);
	} else if (fifo_len == 0) {
		if (file->f_flags & O_NONBLOCK) {
			dahdi_warp_log(DSP_FS_LOG, "%s() returning -EAGAIN", __FUNCTION__);
			return (-EAGAIN);
		}
		dahdi_warp_log(DSP_FIFO_LOG, "%s() waiting for event\n",
			__FUNCTION__);
		ret = wait_event_interruptible(dsp_module->dsp[minor].wq,
				(__kfifo_len(dsp_module->dsp[minor].event_fifo) > 0));

		if (ret == -ERESTARTSYS) {
			dahdi_warp_log(DSP_FIFO_LOG, "%s() signal received.",
				__FUNCTION__);
			return (-EINTR);
		} else {
			event_ready = 1;
			dahdi_warp_log(DSP_FIFO_LOG, "%s() FIFO event ready after wait\n",
				__FUNCTION__);
		}
	} else {
		if (limit_output()) {
			printk(KERN_ERR "%s() event fifo length cannot be negative\n", __FUNCTION__);
		}
		dahdi_warp_log(DSP_FIFO_LOG, "%s() FIFO size error\n",
			__FUNCTION__);
		return -EFAULT;
	}

	if (event_ready) {
		dahdi_warp_log(DSP_FIFO_LOG, "%s() receiving data from FIFO\n",
			__FUNCTION__);
		if (__kfifo_get(dsp_module->dsp[minor].event_fifo,
			(unsigned char*)&tmp_fifo_event, 
			sizeof(struct dsp_fifo_event)) == sizeof(struct dsp_fifo_event)) {
			dahdi_warp_log(DSP_FIFO_LOG, "%s() FIFO event received\n", __FUNCTION__);
			if  (copy_to_user(buf, (void*)&tmp_fifo_event, sizeof(struct dsp_fifo_event))) {
				return -EFAULT;
			}
			ret = sizeof(struct dsp_fifo_event);
			dahdi_warp_log(DSP_FIFO_LOG, "%s() FIFO event size (%d)\n", __FUNCTION__, ret);
			printk(KERN_CRIT "%s() returning (%d) octets\n", __FUNCTION__, ret);
		} else {
			dahdi_warp_log(DSP_FIFO_LOG, "%s() Invalid FIFO state, resetting.\n",
				__FUNCTION__);
			if (limit_output()) {
				printk(KERN_ERR "%s() : Dsp (%d) Invalid FIFO element. Resetting FIFO.\n",
					__FUNCTION__, minor);
			}
			__kfifo_reset(dsp_module->dsp[minor].event_fifo);
		}
	} else {
		dahdi_warp_log(DSP_FIFO_LOG, "%s() FIFO error, we should not be here\n",
			__FUNCTION__);
	}

	return (ret);
}

static int
dahdi_warp_get_dsp_data(unsigned short *buffer)
{
	unsigned short result = 0;
	int ret;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n", __FUNCTION__);

	if ((ret = gpakGetDataFromDsp(DEFAULT_DSP_ID, 0, buffer, &result)) != 0) {
		printk(KERN_ERR "%s() : failed to retrieve license key info from DSP error code %d result = %d\n",
			__FUNCTION__, ret, result);
	}

	return (ret);
}

static int
dahdi_warp_send_dsp_data(struct dsp_args *pdsp_args)
{
	unsigned char *buffer;
	unsigned char *data;
	unsigned int channel;
	unsigned int dsp_id = DEFAULT_DSP_ID;
	unsigned short length;
	int ret = 0;

	channel = pdsp_args->param1;
	buffer = (unsigned char*)pdsp_args->param2;
	length = (unsigned short)pdsp_args->param3;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with channel %d length %d\n", __FUNCTION__, channel, length);

	if (!(data = vmalloc(length))) {
		printk(KERN_ERR "%s() : error allocating %d octets\n",
			__FUNCTION__, length);
		ret = -1;
	}

	if ((ret == 0) && (copy_from_user((void*)data, (void*)buffer, length))) {
		printk(KERN_ERR "%s() : error copying %d octets from userspace\n",
			__FUNCTION__, length);
		vfree(data);	
		ret = -1;
	}

	if (ret == 0) {
		int res;
		if ((res = gpakSendDataToDsp(dsp_id, channel, data, length)) != 0) {
			printk(KERN_ERR "%s() : error (%d) sending %d octets to DSP on channel %d\n",
				__FUNCTION__, res, length, channel); 
			ret = -1;
		} 
		vfree(data);
	}

	return (ret);
}


static int
dsp_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
	unsigned short wbuffer[MAX_DATA_BUFSZ];
	GpakSystemConfig_t dsp_system_cfg;
	struct dsp_args args;
	int ret = 0;
	unsigned int address;
	unsigned short value;
	int tries;

	dahdi_warp_log(DSP_FS_LOG, "%s() called\n", __FUNCTION__);

	switch (cmd) {
		
		case DSP_RESET:
			dahdi_warp_log(DSP_FS_LOG, "%s() calling dsp reset\n",
				__FUNCTION__);
			dahdi_warp_release_all_transcoder_channels(DEFAULT_DSP_ID);
			dahdi_warp_release_all_echocan_channels(DEFAULT_DSP_ID);
			dahdi_warp_reset_dsp(dsp_module);
			break;

		case DSP_LOAD_FIRMWARE:
			dahdi_warp_log(DSP_FS_LOG, "%s() copying firmware from userspace\n",
				__FUNCTION__);
			if (copy_from_user(&args, (struct dsp_args*)arg, sizeof(struct dsp_args))) {
				printk(KERN_ERR "%s() : Error Getting DSP arguments\n",
				__FUNCTION__);
				return (-EINVAL);
			}
			dahdi_warp_log(DSP_FS_LOG, "%s() settings changed to slow access\n",
				__FUNCTION__);
			set_ebc_dsp_slow();
			dahdi_warp_log(DSP_FS_LOG, "%s() attempting to load firmware\n",
				__FUNCTION__);
			if ((ret=dahdi_warp_load_dsp_firmware_cmd(&args))) {
				printk(KERN_ERR "%s() : Error (%d) Loading DSP Firmware\n",
					__FUNCTION__, ret);
				dahdi_warp_log(DSP_FS_LOG, "%s() firmware download failed\n",
					__FUNCTION__);
				args.result = ret;
				if (copy_to_user((void*)arg, &args, sizeof(struct dsp_args))) {
					printk(KERN_ERR "%s() : Error copying back to userspace\n",
						__FUNCTION__);
				}
				return (-EINVAL);
			}

			dahdi_warp_log(DSP_FS_LOG, "%s() successful download of firmware\n",
				__FUNCTION__);

			tries = 0;
			while (tries++ < DSP_DOWNLOAD_CHECK_MAX_TRIES) {
				if (dahdi_warp_get_dsp_ready(DEFAULT_DSP_ID)) {
					dahdi_warp_log(DSP_FS_LOG, "%s() DSP (%d) Ready after (%d) tries\n",
							__FUNCTION__, DEFAULT_DSP_ID, tries);
					break;
				} else {
					dahdi_warp_log(DSP_FS_LOG, "%s() waiting 1 second to check again\n",
							__FUNCTION__);
					set_current_state(TASK_INTERRUPTIBLE);
					schedule_timeout(1000);	
				}
			}

			break;

		case DSP_CONFIGURE_PORTS:
			/* configure the dsp ports on all DSPs to aid with debugging */
			dahdi_warp_log(DSP_FS_LOG, "%s() configuring the dsp ports\n",
				__FUNCTION__);

			if (!dahdi_warp_get_dsp_ready(DEFAULT_DSP_ID)) {
				printk(KERN_ERR "%s() : Dsp (%d) NOT Ready!\n",
					__FUNCTION__, DEFAULT_DSP_ID);
				return (-ENODEV);
			}

			if ((ret = dahdi_warp_configure_dsp_ports(dsp_module))) {
				printk(KERN_ERR "%s( : Unable to configure DSP ports. Check configuration\n",
					__FUNCTION__);
				dahdi_warp_log(DSP_FS_LOG, "%s() DSP configuration failed\n",
					__FUNCTION__);
				return (-EINVAL);
			}

			dahdi_warp_log(DSP_FS_LOG, "%s() DSP configuration successful\n",
				__FUNCTION__);

			dahdi_warp_set_dsp_system_parameters(0);

			dahdi_warp_log(DSP_FS_LOG, "%s() System Ports configuration successful\n",
				__FUNCTION__);

			dahdi_warp_configure_channels(0);

			break;
	
		case DSP_QUERY_VERSION:
			dahdi_warp_log(DSP_FS_LOG, "%s() DSP version query called (TBD)\n",
				__FUNCTION__);

			if (!dahdi_warp_get_dsp_ready(DEFAULT_DSP_ID)) {
				printk(KERN_ERR "%s() : Dsp (%d) NOT Ready!\n",
					__FUNCTION__, DEFAULT_DSP_ID);
				return (-ENODEV);
			}

			if ((ret=gpakGetSystemConfig(DEFAULT_DSP_ID, &dsp_system_cfg))== GscSuccess) {
				args.result = dsp_system_cfg.GpakVersionId;
				if (copy_to_user((void*)arg, &args, sizeof(struct dsp_args))) {
					printk(KERN_ERR "%s() : Error copying back to userspace\n",
						__FUNCTION__);
					return (-EINVAL);
				}
			} else {
				printk(KERN_ERR "%s() : Dsp (%d) Error (%d) getting system configuration\n",
					__FUNCTION__, DEFAULT_DSP_ID, ret);
				return (-EINVAL);
			}
			break;

		case DSP_ENABLE_DEBUG:
			dahdi_warp_log(DSP_FS_LOG, "%s() DSP Debug Enabled\n",
				__FUNCTION__);
			break;


		case DSP_DISABLE_DEBUG:
			dahdi_warp_log(DSP_FS_LOG, "%s() DSP Debug Disabled\n",
				__FUNCTION__);
			break;

		case DSP_PEEK_ADDRESS:
			dahdi_warp_log(DSP_FS_LOG, "%s() DSP Peek Address\n",
				__FUNCTION__);
			if (copy_from_user(&args, (struct dsp_args*)arg, sizeof(args))) {
				printk(KERN_ERR "%s() : Error Getting DSP arguments\n",
				__FUNCTION__);
				return (-EINVAL);
			}
			address = args.param1;
			warp_dsp_read_hpid(address, &value);
			args.result = value;
			dahdi_warp_log(DSP_FUNC_LOG, "%s() read value [0x%04x] from offset [0x%08x]\n",
					__FUNCTION__, value, address);
			if (copy_to_user((void*)arg, &args, sizeof(args))) {
				printk(KERN_ERR "%s() : Error copying back to userspace\n",
					__FUNCTION__);
				return (-EINVAL);
			}
			break;

		case DSP_POKE_ADDRESS:
			dahdi_warp_log(DSP_FS_LOG, "%s() DSP Poke Address\n",
				__FUNCTION__);
			if (copy_from_user(&args, (struct dsp_args*)arg, sizeof(args))) {
				printk(KERN_ERR "%s() : Error Getting DSP arguments\n",
				__FUNCTION__);
				return (-EINVAL);
			}
			address = args.param1;
			value = args.param2;
			warp_dsp_write_hpid(address, value);
			dahdi_warp_log(DSP_FUNC_LOG, "%s() wrote value [0x%04x] to address [0x%08x]\n",
					__FUNCTION__, value, address);
			break;

		case DSP_SEND_DATA:	
			dahdi_warp_log(DSP_FS_LOG, "%s() DSP Send Data\n",
				__FUNCTION__);
			if (copy_from_user(&args, (struct dsp_args*)arg, sizeof(args))) {
				printk(KERN_ERR "%s() : Error Getting DSP arguments\n",
				__FUNCTION__);
				return (-EINVAL);
			}
			return (dahdi_warp_send_dsp_data(&args));
			break;

		case DSP_GET_DATA:
			dahdi_warp_log(DSP_FS_LOG, "%s() DSP Get Data\n",
				__FUNCTION__);
			ret = dahdi_warp_get_dsp_data(wbuffer);
			if (copy_to_user((void*)arg, wbuffer, sizeof(wbuffer))) {
				printk(KERN_ERR "%s() : Error Getting DSP arguments\n",
				__FUNCTION__);
				return (-EINVAL);
			}
			return (ret);
			break;

		default:
			printk(KERN_ERR "%s() : Invalid DSP Command (0x%08x)\n",
				__FUNCTION__, cmd);
			dahdi_warp_log(DSP_FS_LOG, "%s() Invalid DSP command (0x%08x)\n",
				__FUNCTION__, cmd);
			return (-EINVAL);
			break;

	}

	return 0;
}

static inline unsigned 
fpga_read(struct warp_dsp_module *dspm, int reg)
{
	unsigned result = in_be32(dspm->fpga + reg);
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called read 0x%08x from 0x%08x\n",
		__FUNCTION__, result, reg);
        return (result);
}

static inline void 
fpga_write(struct warp_dsp_module *dspm, int reg, unsigned val)
{
        out_be32(dspm->fpga + reg, val);
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called wrote 0x%08x to 0x%08x\n",
		__FUNCTION__, val, reg);
}


void
warp_dsp_get_dsp_command_buffer_write_lock(unsigned short dsp_id)
{
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n", __FUNCTION__);
	if (down_timeout(&dsp_module->dsp[dsp_id].dsp_cmd_write_sem, DSP_COMMAND_TIMEOUT_JIFFIES) == -ETIME) {
		printk(KERN_ERR "%s() timed out waiting for semaphore\n", __FUNCTION__);
	} else {
		dahdi_warp_log(DSP_FUNC_LOG, "%s() got write semaphore\n", __FUNCTION__);
	}
}

void
warp_dsp_get_dsp_command_buffer_read_lock(unsigned short dsp_id)
{
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n", __FUNCTION__);
	if (down_timeout(&dsp_module->dsp[dsp_id].dsp_cmd_read_sem, DSP_COMMAND_TIMEOUT_JIFFIES)  == -ETIME) {
		printk(KERN_ERR "%s() timed out waiting for semaphore\n", __FUNCTION__);
	} else {
		dahdi_warp_log(DSP_FUNC_LOG, "%s() got read semaphore\n", __FUNCTION__);
	}
}

static void
warp_dsp_check_dsp_command_status(struct warp_dsp_module *module)
{
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n", __FUNCTION__);

	if (gpakDspCheckCommandBufferAvailable(DEFAULT_DSP_ID)) {
		if (module->dsp[DEFAULT_DSP_ID].dsp_cmd_write_sem.count == 0) {
			up(&module->dsp[DEFAULT_DSP_ID].dsp_cmd_write_sem);
		}
	}	

	if (gpakDspCheckCommandReplyAvailable(DEFAULT_DSP_ID)) {
		if (module->dsp[DEFAULT_DSP_ID].dsp_cmd_read_sem.count == 0) {
			up(&module->dsp[DEFAULT_DSP_ID].dsp_cmd_read_sem);
		}
	}	
}

static irqreturn_t
warp_dsp_isr(int irq, void *arg)
{
	struct warp_dsp_module *module = (struct warp_dsp_module*)arg;
	unsigned long start_time, end_time, diff_time;
	unsigned long send_time = 0, recv_time = 0, fifo_time = 0;
	static int run_count = 0;
	static int avg_time = 0;

	start_time = jiffies_to_msecs(jiffies);

	module->isr_count++;

	dahdi_warp_log(DSP_INT_LOG, "%s() called\n",
		__FUNCTION__);
	
	if (atomic_read(&module->dsp[0].tcoder_count) != 0) {
#if (DSP_DEFERRED_SEND == 1)
		dahdi_warp_send_raw_frames(module);
		send_time = jiffies_to_msecs(jiffies);
#endif
		dahdi_warp_recv_transcoded_frames(module);
		recv_time = jiffies_to_msecs(jiffies);
	} else {
		send_time = start_time;
		recv_time = start_time;
	}

	/* check the command buffer status */
	warp_dsp_check_dsp_command_status(module);

	module->dsp[DEFAULT_DSP_ID].stat_check_count++;
	if (module->dsp[DEFAULT_DSP_ID].stat_check_count % STAT_CHECK_COUNT == 0) {
		dahdi_warp_check_event_fifo(module);
		fifo_time = jiffies_to_msecs(jiffies);
	} else {
		fifo_time = recv_time;
	}

	end_time = jiffies_to_msecs(jiffies);
	diff_time = end_time - start_time;
	avg_time = (run_count * avg_time + diff_time) / (run_count + 1);
	run_count = run_count + 1;
	module->dsp[DEFAULT_DSP_ID].avg_irq_time = avg_time;

	if (diff_time > module->dsp[DEFAULT_DSP_ID].max_irq_time) {
		module->dsp[DEFAULT_DSP_ID].max_irq_time = diff_time;
	}

	if (diff_time > MAX_ISR_MSEC) {
		printk(KERN_INFO "%s() took %ld msecs send (%ld) msecs recv (%ld) msecs fifo (%ld) msecs\n",
			__FUNCTION__,
			diff_time,
			(send_time - start_time),
			(recv_time - send_time),
			(fifo_time - recv_time));
	}

	dahdi_warp_log(DSP_INT_LOG, "%s() took %ld msecs send (%ld) msecs recv (%ld) msecs fifo (%ld) msecs\n",
		__FUNCTION__,
		diff_time,
		(send_time - start_time),
		(recv_time - send_time),
		(fifo_time - recv_time));

	return IRQ_HANDLED;
}

inline void
warp_dsp_write_hpid(unsigned int offset, unsigned short value)
{
	unsigned int dsp_address = (unsigned int)dsp_module->dsp_host_port;
	dsp_address += DSP_HCNTL0_SELECT_OFFSET;
	dsp_address += offset;
	
	outw(cpu_to_le16(value), dsp_address);
	dahdi_warp_log(DSP_HPORT_LOG, "%s() wrote 0x%04x to address 0x%08x (HPID)\n",
		__FUNCTION__, value, dsp_address);
}

inline void
warp_dsp_read_hpid(unsigned int offset, unsigned short *value)
{
	unsigned int dsp_address = (unsigned int)dsp_module->dsp_host_port;
	dsp_address += DSP_HCNTL0_SELECT_OFFSET;
	dsp_address += offset;

	*value = le16_to_cpu(inw(dsp_address));
	dahdi_warp_log(DSP_HPORT_LOG, "%s() read 0x%04x from address 0x%08x (HPID)\n",
		__FUNCTION__, *value, dsp_address);
}

static void
warp_dsp_write_hpic(unsigned short value)
{
	unsigned int dsp_address = ((unsigned int)dsp_module->dsp_host_port);
	outw(cpu_to_le16(value), dsp_address);
	dahdi_warp_log(DSP_HPORT_LOG, "%s() wrote 0x%04x to 0x%08x (HPIC)\n",
		__FUNCTION__, value, dsp_address);
}

static void
warp_dsp_read_hpic(unsigned short *value)
{
	unsigned int dsp_address = ((unsigned int)dsp_module->dsp_host_port);
	*value = le16_to_cpu(inw(dsp_address));
	dahdi_warp_log(DSP_HPORT_LOG, "%s() read 0x%04x from 0x%08x (HPIC)\n",
		__FUNCTION__, *value, dsp_address);
}

void
warp_dsp_execute(void)
{
	unsigned short hpic_value;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);
	warp_dsp_read_hpic(&hpic_value);
	warp_dsp_write_hpic(hpic_value | HPIC_RESET_MASK);
}

static void
set_ebc_dsp_slow(void)
{
	unsigned fpga_config = fpga_read(dsp_module, FPGA_CONFIG_REG_OFFSET);

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);
	fpga_write(dsp_module, FPGA_CONFIG_REG_OFFSET, fpga_config & ~CONFIG_DSP_TIMING_MASK);
	mtebc(pb3ap, CFG_EBC_PB3AP);
}

static void
set_ebc_dsp_fast(void)
{
	unsigned fpga_config = fpga_read(dsp_module, FPGA_CONFIG_REG_OFFSET);

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);
	fpga_write(dsp_module, FPGA_CONFIG_REG_OFFSET, fpga_config | CONFIG_DSP_TIMING_MASK);
	mtebc(pb3ap, CFG_EBC_PB3AP_FAST);
}


void 
warp_dsp_read_memory_16(unsigned short dsp_id, unsigned int dsp_address, unsigned int num_words, unsigned short* word_values)
{
	unsigned int address;
	unsigned short value;
	int ii;

	dahdi_warp_log(DSP_HPORT_LOG, "%s() called\n",
		__FUNCTION__);

	if (dsp_id > MAX_DSP_COUNT) {
		printk(KERN_ERR "%s() : Invalid DSP Index (%d)\n",
			__FUNCTION__, dsp_id);
		dahdi_warp_log(DSP_FUNC_LOG, "%s() Invalid DSP Index (%d)\n",
			__FUNCTION__, dsp_id);
		return;
	} 
	
	warp_dsp_data_lock(dsp_id);
	for (ii = 0; ii < num_words; ii++) {
		address = (dsp_address << 1) + (ii << 1);
		warp_dsp_read_hpid(address, &value);
		word_values[ii] = value;
	}
	warp_dsp_data_unlock(dsp_id);
}

void 
warp_dsp_read_memory_16_noswap(unsigned short dsp_id, unsigned int dsp_address, unsigned int num_words, unsigned short* word_values)
{
	unsigned int address;
	unsigned short value;
	int ii;

	dahdi_warp_log(DSP_HPORT_LOG, "%s() called\n",
		__FUNCTION__);

	if (dsp_id > MAX_DSP_COUNT) {
		printk(KERN_ERR "%s() : invalid DSP index (%d)\n",
			__FUNCTION__, dsp_id);
		dahdi_warp_log(DSP_FUNC_LOG, "%s() Invalid DSP Index (%d)\n",
			__FUNCTION__, dsp_id);
		return;
	} 

	warp_dsp_data_lock(dsp_id);
	for (ii = 0; ii < num_words; ii++) {
		address = (dsp_address << 1) + (ii << 1);
		warp_dsp_read_hpid(address, &value);
		word_values[ii] = le16_to_cpu(value);
	}
	warp_dsp_data_unlock(dsp_id);
}

void 
warp_dsp_write_memory_16(unsigned short dsp_id, unsigned int dsp_address, unsigned int num_words, unsigned short *word_values)
{
	unsigned int address;
	unsigned short value;
	int ii;

	dahdi_warp_log(DSP_HPORT_LOG, "%s() called\n",
		__FUNCTION__);

	if (dsp_id > MAX_DSP_COUNT) {
		printk(KERN_ERR "%s() : invalid DSP index (%d)\n",
			__FUNCTION__, dsp_id);
		dahdi_warp_log(DSP_FUNC_LOG, "%s() Invalid DSP Index (%d)\n",
			__FUNCTION__, dsp_id);
		return;
	} 

	warp_dsp_data_lock(dsp_id);
	for (ii = 0; ii < num_words; ii++) {
		address = (dsp_address << 1) + ( ii << 1);
		value = word_values[ii];
		warp_dsp_write_hpid(address, value);
	}
	warp_dsp_data_unlock(dsp_id);
}

void 
warp_dsp_write_memory_16_noswap(unsigned short dsp_id, unsigned int dsp_address, unsigned int num_words, unsigned short *word_values)
{
	unsigned int address;
	unsigned short value;
	int ii;

	dahdi_warp_log(DSP_HPORT_LOG, "%s() called\n",
		__FUNCTION__);

	if (dsp_id > MAX_DSP_COUNT) {
		printk(KERN_ERR "%s() : invalid DSP index (%d)\n",
			__FUNCTION__, dsp_id);
		dahdi_warp_log(DSP_FUNC_LOG, "%s() Invalid DSP Index (%d)\n",
			__FUNCTION__, dsp_id);
		return;
	} 

	warp_dsp_data_lock(dsp_id);
	for (ii = 0; ii < num_words; ii++) {
		address = (dsp_address << 1) + ( ii << 1);
		value = cpu_to_le16(word_values[ii]);
		warp_dsp_write_hpid(address, value);
	}
	warp_dsp_data_unlock(dsp_id);
}

void
warp_dsp_read_memory_32(unsigned short dsp_id, unsigned int dsp_address, unsigned int num_longs, unsigned long int *long_values)
{
	unsigned int address;
	unsigned short value1;
	unsigned short value2;
	int ii;

	dahdi_warp_log(DSP_HPORT_LOG, "%s() called\n",
		__FUNCTION__);

	if (dsp_id > MAX_DSP_COUNT) {
		printk(KERN_ERR "%s() : invalid DSP index (%d)\n",
			__FUNCTION__, dsp_id);
		dahdi_warp_log(DSP_FUNC_LOG, "%s() Invalid DSP Index (%d)\n",
			__FUNCTION__, dsp_id);
		return;
	} 

	warp_dsp_data_lock(dsp_id);
	for (ii = 0; ii < num_longs; ii++) {
		address = (dsp_address << 1) + (ii << 2);
		warp_dsp_read_hpid(address, &value1);
		warp_dsp_read_hpid(address + 2, &value2);
		long_values[ii] = (value1 << 16) | value2;
	}
	warp_dsp_data_unlock(dsp_id);
}

void
warp_dsp_read_memory_32_noswap(unsigned short dsp_id, unsigned int dsp_address, unsigned int num_longs, unsigned long int *long_values)
{
	unsigned int address;
	unsigned short value1;
	unsigned short value2;
	int ii;

	dahdi_warp_log(DSP_HPORT_LOG, "%s() called\n",
		__FUNCTION__);

	if (dsp_id > MAX_DSP_COUNT) {
		printk(KERN_ERR "%s() : invalid DSP index (%d)\n",
			__FUNCTION__, dsp_id);
		dahdi_warp_log(DSP_FUNC_LOG, "%s() Invalid DSP Index (%d)\n",
			__FUNCTION__, dsp_id);
		return;
	} 

	warp_dsp_data_lock(dsp_id);
	for (ii = 0; ii < num_longs; ii++) {
		address = (dsp_address << 1) + (ii << 2);
		warp_dsp_read_hpid(address, &value1);
		warp_dsp_read_hpid(address + 2, &value2);
		long_values[ii] = le32_to_cpu((value1 << 16) | value2);
	}
	warp_dsp_data_unlock(dsp_id);
}

void 
warp_dsp_write_memory_32(unsigned short dsp_id, unsigned int dsp_address, unsigned int num_longs, unsigned long int *long_values)
{
	unsigned int address;
	unsigned short value1;
	unsigned short value2;
	int ii;

	dahdi_warp_log(DSP_HPORT_LOG, "%s() called\n",
		__FUNCTION__);

	if (dsp_id > MAX_DSP_COUNT) {
		printk(KERN_ERR "%s() : invalid DSP index (%d)\n",
			__FUNCTION__, dsp_id);
		dahdi_warp_log(DSP_FUNC_LOG, "%s() Invalid DSP Index (%d)\n",
			__FUNCTION__, dsp_id);
		return;
	} 

	warp_dsp_data_lock(dsp_id);
	for (ii = 0; ii < num_longs; ii++) {
		address = (dsp_address << 1) + ( ii << 2);
		value1 = (long_values[ii] >> 16) & 0xFFFF;
		value2 = (long_values[ii]) & 0xFFFF;
		warp_dsp_write_hpid(address, value1);
		warp_dsp_write_hpid(address + 2, value2);
	}
	warp_dsp_data_unlock(dsp_id);
}

void 
warp_dsp_write_memory_32_noswap(unsigned short dsp_id, unsigned int dsp_address, unsigned int num_longs, unsigned long int *long_values)
{
	unsigned int address;
	unsigned short value1;
	unsigned short value2;
	int ii;

	dahdi_warp_log(DSP_HPORT_LOG, "%s() called\n",
		__FUNCTION__);

	if (dsp_id > MAX_DSP_COUNT) {
		printk(KERN_ERR "%s() : invalid DSP index (%d)\n",
			__FUNCTION__, dsp_id);
		dahdi_warp_log(DSP_FUNC_LOG, "%s() Invalid DSP Index (%d)\n",
			__FUNCTION__, dsp_id);
		return;
	} 

	warp_dsp_data_lock(dsp_id);
	for (ii = 0; ii < num_longs; ii++) {
		address = (dsp_address << 1) + ( ii << 2);
		value1 = (long_values[ii] >> 16) & 0xFFFF;
		value2 = (long_values[ii]) & 0xFFFF;
		warp_dsp_write_hpid(address, cpu_to_le16(value1));
		warp_dsp_write_hpid(address + 2, cpu_to_le16(value2));
	}
	warp_dsp_data_unlock(dsp_id);
}

void 
warp_dsp_delay(unsigned int delay_type)
{
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with delay_type = %d\n",
		__FUNCTION__, delay_type);
#if defined(USE_SEMAPHORE_WAIT)
	if (delay_type == DELAY_FOR_WRITE_READY) {
		warp_dsp_get_dsp_command_buffer_write_lock(DEFAULT_DSP_ID);
	} else if (delay_type == DELAY_FOR_READ_READY) {
		warp_dsp_get_dsp_command_buffer_read_lock(DEFAULT_DSP_ID);
	} else {
		printk(KERN_ERR "%s() : Unknown event (%d) to wait for\n",
			__FUNCTION__, delay_type);
	}
#else
	udelay(DSP_DELAY_IN_US);
#endif
}


void 
warp_dsp_data_lock(unsigned short dsp_id)
{
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	if (dsp_id > MAX_DSP_COUNT) {
		printk(KERN_ERR "%s() : invalid DSP index (%d)\n",
			__FUNCTION__, dsp_id);
		dahdi_warp_log(DSP_FUNC_LOG, "%s() Invalid DSP Index (%d)\n",
			__FUNCTION__, dsp_id);
		return;
	} 

	dahdi_warp_disable_dsp_irq();
	down(&dsp_module->dsp[dsp_id].access_sem);
}

void 
warp_dsp_data_unlock(unsigned short dsp_id)
{
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	if (dsp_id > MAX_DSP_COUNT) {
		printk(KERN_ERR "%s() : invalid DSP index (%d)\n",
			__FUNCTION__, dsp_id);
		dahdi_warp_log(DSP_FUNC_LOG, "%s() Invalid DSP Index (%d)\n",
			__FUNCTION__, dsp_id);
		return;
	} 

	up(&dsp_module->dsp[dsp_id].access_sem);
	dahdi_warp_enable_dsp_irq();
}

void 
warp_dsp_cmd_lock(unsigned short dsp_id)
{
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	if (dsp_id > MAX_DSP_COUNT) {
		printk(KERN_ERR "%s() : invalid DSP index (%d)\n",
			__FUNCTION__, dsp_id);
		dahdi_warp_log(DSP_FUNC_LOG, "%s() Invalid DSP Index (%d)\n",
			__FUNCTION__, dsp_id);
		return;
	} 

	down(&dsp_module->dsp[dsp_id].cmd_access_sem);
}

void 
warp_dsp_cmd_unlock(unsigned short dsp_id)
{
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	if (dsp_id > MAX_DSP_COUNT) {
		printk(KERN_ERR "%s() : invalid DSP index (%d)\n",
			__FUNCTION__, dsp_id);
		dahdi_warp_log(DSP_FUNC_LOG, "%s() Invalid DSP Index (%d)\n",
			__FUNCTION__, dsp_id);
		return;
	} 

	up(&dsp_module->dsp[dsp_id].cmd_access_sem);
}

void
warp_dsp_lock(unsigned short dsp_id)
{
	/* dummy function */
}

void
warp_dsp_unlock(unsigned short dsp_id)
{
	/* dummy function */
}

int 
warp_dsp_read_firmware(unsigned int file_id, unsigned char *buffer, unsigned int num_bytes)
{
	int ii;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	if ((file_ptr + num_bytes) > warp_dsp_firmware_size) {
		dahdi_warp_log(DSP_FUNC_LOG, "%s() attempt to read beyond file size\n",
			__FUNCTION__);
		/* return filereaderror */
		return -1;
	} 

	for (ii = 0; ii < num_bytes; ii++) {
		buffer[ii] = warp_dsp_firmware[file_ptr++];
		if (file_ptr == warp_dsp_firmware_size) {
			file_ptr = 0;	/* replay back to beginning if need be */
			dahdi_warp_log(DSP_FUNC_LOG, "%s() wrapping pointer back to beginning\n",
				__FUNCTION__);
			break;
		}
	}

	/* return success */
	return (num_bytes);
}

int 
dahdi_warp_set_dsp_firmware_params(unsigned char *buffer, unsigned int size)
{
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	file_ptr = 0;
	warp_dsp_firmware = buffer;
	warp_dsp_firmware_size = size;

	return 0;
}

static void
dahdi_warp_reset_dsp(struct warp_dsp_module *dspm)
{
	unsigned regval;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	regval = fpga_read(dspm, FPGA_RESET);
	regval &= ~FPGA_DSP_RESET_BIT;
	fpga_write(dspm, FPGA_RESET, regval);

	udelay(2);

	regval = fpga_read(dspm, FPGA_RESET);
	regval |= FPGA_DSP_RESET_BIT;
	fpga_write(dspm, FPGA_RESET, regval);

	dahdi_warp_set_dsp_ready(DEFAULT_DSP_ID, 0);
}

static int
dahdi_warp_load_dsp_firmware(struct warp_dsp_module *dspm, int reset_dsp)
{
	int res = 0;
	int dspid;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with reset_dsp = %d\n",
		__FUNCTION__, reset_dsp);

	if (reset_dsp) {
		dahdi_warp_reset_dsp(dspm);
	}

	for (dspid = 0; dspid < MAX_DSP_COUNT; dspid++) {
		if ((res = gpakDownloadDsp(dspid, 0)) != GdlSuccess) {
			printk(KERN_ERR "%s() : Dsp(%d) Error (%d) downloading DSP \n",
				__FUNCTION__, dspid, res);
			return (res);
		}
	}

	if (reset_dsp) {
	        int handshake_retry = 0;
		int handshake_success = 0;
		unsigned short value;

		warp_dsp_execute();
		set_ebc_dsp_fast();

		dahdi_warp_log(DSP_FS_LOG, "%s() executing dsp miniboot application\n",
			__FUNCTION__);

		while (handshake_retry++ <  HANDSHAKE_COUNT) {
			warp_dsp_read_hpid(HANDSHAKE_OFFSET, &value);
			if (value == HANDSHAKE_PATTERN_WORD) {
				handshake_success = 1;
				break;
			}
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(1000);	
		}

		if (handshake_success) {
			dahdi_warp_log(DSP_FUNC_LOG, "%s() : Handshake success %d attempts\n",
					__FUNCTION__, handshake_retry);
			warp_dsp_write_hpid(HANDSHAKE_OFFSET, HANDSHAKE_RESPONSE_WORD);
		} else {
			printk(KERN_ERR "%s() : Handshake failed. Reset DSP and try again\n",
				__FUNCTION__);
			res = -1;
		}
	} else {
		dahdi_warp_log(DSP_FUNC_LOG, "%s() : Flagging the mini bootloader for success\n",
				__FUNCTION__);
		warp_dsp_write_hpid(IMAGE_DOWNLOAD_OFFSET, IMAGE_DOWNLOAD_FLAG);
	}

	return (res);
}

static int
dahdi_warp_load_dsp_firmware_cmd(struct dsp_args *pargs)
{
	unsigned char *firmware_address;
	unsigned int firmware_size;
	int res = -1;
	int final_image = 0;
	int reset_dsp = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	firmware_size = pargs->param2;
	final_image = pargs->param3;
	reset_dsp = (final_image) ? 0 : 1;

	if ((firmware_address = vmalloc(firmware_size)) == NULL) {
		printk(KERN_ERR "%s() : Error allocating memory of size (%d) bytes\n",
			__FUNCTION__, firmware_size);
		return (res);
	}
	if (copy_from_user((void*)firmware_address, (void*)pargs->param1, firmware_size)) {
		printk(KERN_ERR "%s() : Error copying (%d) firmware bytes from user\n",
			__FUNCTION__, firmware_size);
		vfree(firmware_address);
		return (res);
	}

	dahdi_warp_set_dsp_firmware_params(firmware_address, firmware_size);
	dahdi_warp_disable_dsp_irq();
	res = dahdi_warp_load_dsp_firmware(dsp_module, reset_dsp);
	dahdi_warp_enable_dsp_irq();
	vfree(firmware_address);

	return (res);
}

static GpakCompandModes 
dahdi_warp_get_companding_mode(void)
{
	unsigned moda, modb;
	
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	get_module_info(&moda, &modb);

	if ((moda == MODULE_TYPE_BRI) || (modb == MODULE_TYPE_BRI)) {
		return cmpPCMA;
	} else {
		return cmpPCMU;
	}
}

static void
dahdi_warp_get_dsp_configuration(unsigned int dspid, GpakPortConfig_t *pconfig)
{
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	memset(pconfig, 0, sizeof(GpakPortConfig_t));

	/* We configure the first two ports with the same parameters */
	/* We leave the third port disabled */

	pconfig->Port1Enable = Enabled;
	pconfig->Port1SlotMask0_31 = 0xFFFFFFFF;
	pconfig->Port1SlotMask32_63 = 0;
	pconfig->Port1SlotMask64_95 = 0;
	pconfig->Port1SlotMask96_127 = 0;

	pconfig->TxDataDelay1 = 1;
	pconfig->RxDataDelay1 = 1;
	pconfig->ClkDiv1 = 0;		/* not used? */
	pconfig->FramePeriod1 = 0;
	pconfig->FrameWidth1 = 0;
	pconfig->Compand1 = dahdi_warp_get_companding_mode();
	pconfig->SampRateGen1 = Disabled;

	pconfig->txClockPolarity1 = 0; /* clkRising */
	pconfig->rxClockPolarity1 = 0; /* clkFalling */

	pconfig->Port2Enable = Enabled;
	pconfig->Port2SlotMask0_31 = 0xFFFFFFFF;
	pconfig->Port2SlotMask32_63 = 0;
	pconfig->Port2SlotMask64_95 = 0;
	pconfig->Port2SlotMask96_127 = 0;

	pconfig->TxDataDelay2 = 1;
	pconfig->RxDataDelay2 = 1;
	pconfig->ClkDiv2 = 0;		/* not used */
	pconfig->FramePeriod2 = 0;
	pconfig->FrameWidth2 = 0;
	pconfig->Compand2 = dahdi_warp_get_companding_mode();
	pconfig->SampRateGen2 = Disabled;

	pconfig->txClockPolarity2 = 0; /* clkRising */
	pconfig->rxClockPolarity2 = 0; /* clkFalling */

	pconfig->Port3Enable = Disabled;
	pconfig->Port3SlotMask0_31 = 0;
	pconfig->Port3SlotMask32_63 = 0;
	pconfig->Port3SlotMask64_95 = 0;
	pconfig->Port3SlotMask96_127 = 0;

}

static void
dahdi_warp_get_dsp_system_parameters(unsigned int dspid, GpakSystemConfig_t *psysconfig)
{
	gpakReadSysParmsStatus_t status;

	dahdi_warp_log(DSP_BIST_LOG, "%s() called\n",
		__FUNCTION__);

	if ((status=gpakGetSystemConfig(dspid, psysconfig)) == RspSuccess) {

		/* dump dsp parameters for debugging only */
		dahdi_warp_log(DSP_BIST_LOG, "G.PAK Version Id 0x%08x\n", (unsigned int)psysconfig->GpakVersionId);
		dahdi_warp_log(DSP_BIST_LOG, "G.PAK Maximum Number of Channels %d\n", psysconfig->MaxChannels);
		dahdi_warp_log(DSP_BIST_LOG, "G.PAK ActiveChannels %d\n", psysconfig->ActiveChannels);

		if (psysconfig->SelfTestStatus & 0x01) {
			dahdi_warp_log(DSP_BIST_LOG, "G.PAK Self Test Memory OK\n");
		} else {
			dahdi_warp_log(DSP_BIST_LOG, "G.PAK Self Test Memory NOT ok\n");
		}

		if (psysconfig->SelfTestStatus & 0x02) {
			dahdi_warp_log(DSP_BIST_LOG, "G.PAK Self Test Serial Ports OK\n");
		} else {
			dahdi_warp_log(DSP_BIST_LOG, "G.PAK Self Test Serial Ports NOT ok\n");
		}

		if (psysconfig->SelfTestStatus & 0x04) {
			dahdi_warp_log(DSP_BIST_LOG, "G.PAK Self Test Host Port Interface OK\n");
		} else {
			dahdi_warp_log(DSP_BIST_LOG, "G.PAK Self Test Host Port Interface NOT ok\n");
		}

	} else {
		switch (status) {

			case RspInvalidDsp:
				printk(KERN_ERR "%s() : Invalid Dsp (%d)\n",
					__FUNCTION__, dspid);
				break;

			case RspDspCommFailure:
				printk(KERN_ERR "%s() : Dsp (%d) Communications Failure\n",
					__FUNCTION__, dspid);
				break;

			default:
				printk(KERN_ERR "%s() : Invalid/Unknown Error Code (%d)\n",
					__FUNCTION__, dspid);
		}
	}
}

static void
dahdi_warp_set_dsp_system_parameters(unsigned int dspid)
{	
	GpakSystemParms_t sysparms;
	GpakECParams_t *ppcmec = &sysparms.PcmEc;
	gpakWriteSysParmsStatus_t status;
	GPAK_SysParmsStat_t ret_status;
	unsigned short agc_update = 0;
	unsigned short vad_update = 0;
	unsigned short pcm_ec_update = 1;
	unsigned short pkt_ec_update = 0;
	unsigned short conf_update = 0;
	unsigned short cid_update = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	/* some of these parameters need to be user configurable via /etc/modprobe.conf */
	ppcmec->TailLength = taillength;				/* multiple of 32 milliseconds */
	ppcmec->NlpType = nlptype;					/* NLP noise type is OFF */
	ppcmec->AdaptEnable = adaptenable;				/* Adaptation Enabled */
	ppcmec->G165DetEnable = g165detenable;				/* G.165 Detect Disabled */
	ppcmec->DblTalkThresh = dbltalkthreshold;			/* Double Talk Threshold */
	ppcmec->NlpThreshold = nlpthreshold;				/* NLP Threshold */
	ppcmec->NlpUpperLimitThreshConv = nlpupperlimitthresh;		/* NLP Convergence Threshold */
	ppcmec->NlpUpperLimitThreshPreconv = nlpupperlimitprethresh;	/* NLP Pre-Convergence Threshold */
	ppcmec->NlpMaxSupp = nlpmaxsupp;				/* NLP Maximum Suppression */
	ppcmec->CngThreshold = nlpcngthreshold;				/* CNG Noise Threshold */
	ppcmec->AdaptLimit = nlpadaptlimit;				/* Maximum Adaptations Per Frame */
	ppcmec->CrossCorrLimit = crosscorrlimit;			/* Cross Correlation Limit */
	ppcmec->NumFirSegments = numfirsegments;			/* Number of FIR Segments */
	ppcmec->FirSegmentLen = firsegmentlen;				/* Length of each independent FIR segment in # of 8 kHz samples */
	ppcmec->FirTapCheckPeriod = firtapcheckperiod;			/* Frequency to perform background updates (just a number I picked up) */
	ppcmec->MaxDoubleTalkThres = maxdoubletalkthreshold;		/* Maximum Double Talk soft Threshold */
	ppcmec->MixedFourWireMode = mixedfourwiremode;			/* Mixed Four wire mode */
	ppcmec->ReconvergenceCheckEnable = reconvcheckenable;		/* Reconvergence Check Enable */

	if ((status = gpakWriteSystemParms(dspid, 
					&sysparms, 
					agc_update, 
					vad_update,
					pcm_ec_update, 
					pkt_ec_update,
					conf_update, 
					cid_update, 
					&ret_status)) == WspSuccess) {
		dahdi_warp_log(DSP_FUNC_LOG, "G.PAK Ecan Parameters Updated Successfully.\n");
	} else {
		switch (status) {

			case WspParmError:
				printk(KERN_ERR "%s() : Parameter Error with Status Code (%d)\n",
					__FUNCTION__, ret_status);
				break;

			case WspInvalidDsp:
				printk(KERN_ERR "%s() : Invalid Dsp (%d)\n",
					__FUNCTION__, dspid);
				break;

			case WspDspCommFailure:
				printk(KERN_ERR "%s() : Dsp (%d) Communications Failure\n",
					__FUNCTION__, dspid);
				break;

			default:
				printk(KERN_ERR "%s() : Invalid/Unknown Error Code (%d)\n",
					__FUNCTION__, dspid);
				break;

		}
	}
}

static int 
dahdi_warp_configure_dsp_ports(struct warp_dsp_module *dspm)
{
	GpakPortConfig_t dsp_port_config;
	GPAK_PortConfigStat_t status;
	int res = 0;
	int dspid;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	for (dspid = 0; dspid < MAX_DSP_COUNT; dspid++) {

		dahdi_warp_get_dsp_configuration(dspid, &dsp_port_config);

		if ((res = gpakConfigurePorts(dspid, &dsp_port_config, &status)) != CpsSuccess) {
			printk(KERN_ERR "%s() : Dsp(%d) Error (%d) Status (%d) configuring DSP ports\n",
				 __FUNCTION__, dspid, res, status);
			break;
		}
	}

	return (res);
}

static int 
dahdi_warp_reset_channel_stats(unsigned int dsp_id, unsigned int channel_id)
{
	struct chan_stats *pstats;
	int res = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	if (channel_id > MAX_CHANNEL_COUNT) {
		printk(KERN_ERR "%s() : Dsp (%d) Invalid Channel Id (%d)\n",
			__FUNCTION__, dsp_id, channel_id);
		res = -1;
		return (res);
	}

	pstats=&dsp_module->chans[channel_id].stats;
	pstats->pkt_sent = 0;
	pstats->pkt_recvd = 0;
	pstats->pkt_dropped = 0;
	pstats->pkt_nopayload = 0;
		
	res = 0;

	return (res);
}

static int
dahdi_warp_get_channel(unsigned int dsp_id, unsigned int *new_channel_id, unsigned int channel_type, unsigned int channel_subtype)
{
	unsigned long flags;
	int res = 1;
	int ii;
	int range_begin, range_end;
	int found = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	if (channel_type == TRANSCODER) {
		if  (channel_subtype == TRANSCODER_B_SIDE) {
			range_begin = MIN_TCODER_B_SIDE_CHAN_NUM_RANGE1;
			range_end = MAX_TCODER_B_SIDE_CHAN_NUM_RANGE1;
		} else if (channel_subtype == TRANSCODER_A_SIDE) {
			range_begin = MIN_TCODER_A_SIDE_CHAN_NUM_RANGE1;
			range_end = MAX_TCODER_A_SIDE_CHAN_NUM_RANGE1;
		} else {
			printk(KERN_ERR "%s() : Invalid channel_subtype = %d\n",
				__FUNCTION__, channel_subtype);
			return (res);
		}
	} else if (channel_type == ECHOCAN) {
		range_begin = MIN_ECHOCAN_CHAN_NUM_RANGE1;
		range_end = MAX_ECHOCAN_CHAN_NUM_RANGE1;
	} else {
		printk(KERN_ERR "%s() : Invalid channel_type = %d\n",
			__FUNCTION__, channel_type);
		return (res);
	}

	dahdi_warp_log(DSP_FUNC_LOG, "%s() channel_type = %d range_begin = %d range_end = %d\n",
		__FUNCTION__, channel_type, range_begin, range_end);

	spin_lock_irqsave(&dsp_module->list_spinlock, flags);

	for (ii = range_begin; ii <= range_end; ii++) {
		if (dsp_module->chans[ii].status == AVAILABLE) {
			dsp_module->chans[ii].status = BUSY;
			found = 1;
			break;
		}
	}

	if ((!found) && (channel_subtype == TRANSCODER_A_SIDE)) {

		/* 2nd chance search for A side transcoder in the "TCODER_A/ECAN" channel range */
		for (ii = MIN_TCODER_A_SIDE_CHAN_NUM_RANGE2; ii <= MAX_TCODER_A_SIDE_CHAN_NUM_RANGE2; ii++) {
			if (dsp_module->chans[ii].status == AVAILABLE) {
				dsp_module->chans[ii].status = BUSY;
				found = 1;
				break;
			}
		}

		/* 3rd chance (or last chance?) search for A side transcoder in the "TCODER_A/TCODER_B" channel range */
		if (!found) {
			for (ii = MIN_TCODER_B_SIDE_CHAN_NUM_RANGE1; ii <= MAX_TCODER_B_SIDE_CHAN_NUM_RANGE1; ii++) {
				if (dsp_module->chans[ii].status == AVAILABLE) {
					dsp_module->chans[ii].status = BUSY;
					found = 1;
					break;
				}
			}
		}
	}

	spin_unlock_irqrestore(&dsp_module->list_spinlock, flags);

	if (found) {
		dsp_module->chans[ii].type = channel_type;
		dsp_module->chans[ii].flags = 0;
		dsp_module->chans[ii].rx_buf_offset = 0;
		memset(dsp_module->chans[ii].rx_buf, 0, MAX_PKT_SIZE);
		*new_channel_id = ii;
		res = 0;
		dahdi_warp_log(DSP_FUNC_LOG, "%s() new channel type (%d) id (%d)\n",
			__FUNCTION__, channel_type, ii);
		if (channel_type == TRANSCODER) {
			atomic_inc(&dsp_module->dsp[dsp_id].tcoder_count);
		} else if (channel_type == ECHOCAN) {
			atomic_inc(&dsp_module->dsp[dsp_id].ecan_count);
		} else {
			printk(KERN_ERR "%s() : Invalid channel_type = %d\n",
				__FUNCTION__, channel_type);
	 	}	
	}

	if (res) {
		dahdi_warp_log(DSP_FUNC_LOG, "%s() no available channel for type (%d)\n",
			__FUNCTION__, channel_type);
		printk(KERN_ERR "%s() no available channel for type (%d)\n",
			__FUNCTION__, channel_type);
	}


	return (res);
}

static int
dahdi_warp_free_channel(unsigned int dsp_id, unsigned int channel_id, unsigned int free_entry)
{
	unsigned long flags;
	int res = 1;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	if (channel_id > MAX_CHANNEL_COUNT) {
		dahdi_warp_log(DSP_FUNC_LOG, "%s() Invalid Channel Id %d\n",
			__FUNCTION__, channel_id);
		printk(KERN_ERR "%s(): Dsp(%d) Error Invalid Channel Id (%d)\n",
			__FUNCTION__, dsp_id, channel_id);
		return (res);
	}

	spin_lock_irqsave(&dsp_module->list_spinlock, flags);

	if (dsp_module->chans[channel_id].status == BUSY) {
		if (dsp_module->chans[channel_id].type == TRANSCODER) {
			if ((res = dahdi_warp_cleanup_transcoder_channel(dsp_id, channel_id)) == 0) {
				atomic_dec(&dsp_module->dsp[dsp_id].tcoder_count);
				dahdi_warp_reset_channel_stats(dsp_id, channel_id);
				dahdi_warp_log(DSP_FUNC_LOG, "%s() successfully cleaned up transcoder channel (%d)\n",
						__FUNCTION__, channel_id);
			} else {
				printk(KERN_ERR "%s() : Error cleaning up transcoder channel (%d)\n",
					__FUNCTION__, channel_id);
			}
		} else if (dsp_module->chans[channel_id].type == ECHOCAN) {
			dahdi_warp_set_echocan_channel_id(dsp_id, -1, channel_id);
			atomic_dec(&dsp_module->dsp[dsp_id].ecan_count);
			res = 0;
		} else {
			printk(KERN_ERR "%s() : BUG() channel (%d) of unknown type (%d)\n",
				__FUNCTION__, channel_id, dsp_module->chans[channel_id].type);
		}

		clear_bit(CHAN_READY, &dsp_module->chans[channel_id].flags);
		dsp_module->chans[channel_id].status = AVAILABLE;
		dsp_module->chans[channel_id].paired_channel = -1;
		dsp_module->chans[channel_id].discard_frames = 0;
		dsp_module->chans[channel_id].stats.write_count = 0;
		dsp_module->chans[channel_id].stats.read_count = 0;
		dsp_module->chans[channel_id].stats.pkt_queue_count = 0;
		dsp_module->chans[channel_id].stats.pkt_sent_count = 0;
		dsp_module->chans[channel_id].stats.pkt_int_rx_queued = 0;
		dsp_module->chans[channel_id].stats.pkt_int_rx_recvd = 0;
		dsp_module->chans[channel_id].stats.pkts_none_sent = 0;
		dsp_module->chans[channel_id].stats.pkts_none_recvd = 0;
		dsp_module->chans[channel_id].cpu2dsp_rp = 0;
		dsp_module->chans[channel_id].cpu2dsp_wp = 0;
		dsp_module->chans[channel_id].cpu2dsp_len = 0;
		dsp_module->chans[channel_id].dsp2cpu_rp = 0;
		dsp_module->chans[channel_id].dsp2cpu_wp = 0;
		dsp_module->chans[channel_id].dsp2cpu_len = 0;
		dsp_module->chans[channel_id].warn_count = 0;
		dahdi_warp_log(DSP_FUNC_LOG, "%s() released channel id (%d)\n",
			__FUNCTION__, channel_id);
	} else {
		dahdi_warp_log(DSP_FUNC_LOG, "%s() Channel (%d) already available/freed\n",
			__FUNCTION__, channel_id);
		printk(KERN_ERR "%s() : Dsp(%d) Freeing an already available channel (%d)\n",
			__FUNCTION__, dsp_id, channel_id);
	}

	spin_unlock_irqrestore(&dsp_module->list_spinlock, flags);

	return (res);
}

static int
dahdi_warp_allocate_ecan_channel_range(int dsp_id, int start, int end, int step)
{
	int channel, timeslot, channel_id;
	int res = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
			__FUNCTION__);

	timeslot = start * 2;
	for (channel = start; channel < end; channel++) {
		if (dahdi_warp_alloc_echocan_channel(dsp_id, timeslot, taillength, &channel_id) == 0) {
			dahdi_warp_set_echocan_channel_id(dsp_id, timeslot, channel);
			if (dahdi_warp_disable_echocan(dsp_id, channel_id) == 0) {
				dahdi_warp_log(DSP_FUNC_LOG, "%s() ECAN (Bypassed) timeslot %d channel %d channel_id %d\n",
						__FUNCTION__, timeslot, channel, channel_id);
			} else {
				printk(KERN_ERR "%s() error bypassing ecan channel = % dsp_channel_id = %d\n",
					__FUNCTION__, channel, channel_id);
			}
		} else {
			printk(KERN_ERR "%s() error allocating ecan for channel (%d)\n",
				__FUNCTION__, channel); 
			res = -1;
		}
		timeslot += step;
 	}

	return (res);
}

static void 
get_module_info(unsigned *moda, unsigned *modb)
{
	unsigned rev = fpga_read(dsp_module, FPGA_REV_OFFSET);
	
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	*moda = (rev >> 16) & 0xf;
	*modb = (rev >> 20) & 0xf;
}

static void
get_bri_module_span_count(unsigned *moda_span_count, unsigned *modb_span_count)
{
#define R_CHIP_ID	0x16
#define CHIP_ID_2S4U	0x62
#define CHIP_ID_4SU	0x63
#define BRI_MODA_CHIP	2
#define BRI_MODB_CHIP	6
	unsigned moda, modb;
	u8 id, rev;
	
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	get_module_info(&moda, &modb);

	if (moda == MODULE_TYPE_BRI) { 
		fpga_read_indirect(dsp_module->fpga, BRI_MODA_CHIP, R_CHIP_ID, &id);
		fpga_read_indirect(dsp_module->fpga, BRI_MODA_CHIP, R_CHIP_ID, &rev);
		if (id == CHIP_ID_2S4U) {
			*moda_span_count = 2;
		} else if (id == CHIP_ID_4SU) {
			*moda_span_count = 4;
		} else {
			printk(KERN_ERR "%s() : Unexpected id %x rev %x\n",
				__FUNCTION__, id, rev);
			*moda_span_count = -1;
		}
	} else {
		*moda_span_count = -1;
	}

	if (modb == MODULE_TYPE_BRI) { 
		fpga_read_indirect(dsp_module->fpga, BRI_MODB_CHIP, R_CHIP_ID, &id);
		fpga_read_indirect(dsp_module->fpga, BRI_MODB_CHIP, R_CHIP_ID, &rev);
		if (id == CHIP_ID_2S4U) {
			*modb_span_count = 2;
		} else if (id == CHIP_ID_4SU) {
			*modb_span_count = 4;
		} else {
			printk(KERN_ERR "%s() : Unexpected id %x rev %x\n",
				__FUNCTION__, id, rev);
			*modb_span_count = -1;
		}
	} else {
		*modb_span_count = -1;
	}
}

static int 
dahdi_warp_configure_channels(unsigned int dsp_id)
{
	int start_channel, end_channel;
	int mod_a_step, mod_b_step;
	unsigned mod_a_count, mod_b_count;
	unsigned moda, modb;
	int chan_count;
	int res = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
			__FUNCTION__);

	dahdi_warp_allocate_ecan_channel_range(dsp_id, 0, 2, 2);

	get_module_info(&moda, &modb);

	if ((moda == MODULE_TYPE_BRI) || (modb == MODULE_TYPE_BRI)) {
		get_bri_module_span_count(&mod_a_count, &mod_b_count);
	} else {
		mod_a_count = 0;
		mod_b_count = 0;
	}

	if (moda != MODULE_TYPE_NONE) {
		start_channel = MODULE_A_FIRST_CHAN;
		mod_a_step = (moda == MODULE_TYPE_BRI) ? 1 : 2;
		chan_count = (moda == MODULE_TYPE_BRI) ? (mod_a_count * 2) : 4;
		end_channel = start_channel + chan_count;
		res = dahdi_warp_allocate_ecan_channel_range(dsp_id, start_channel, end_channel, mod_a_step);
		dahdi_warp_log(DSP_FUNC_LOG, "%s() : module A : start_channel = %d end_channel = %d mod_a_step = %d chan_count = %d\n",
				__FUNCTION__, start_channel, end_channel, mod_a_step, chan_count);
	} 

	if ((res == 0) && (modb != MODULE_TYPE_NONE)) {
		start_channel = MODULE_B_FIRST_CHAN;
		mod_b_step = (modb == MODULE_TYPE_BRI) ? 1 : 2;
		chan_count = (modb == MODULE_TYPE_BRI) ? (mod_b_count * 2) : 4;
		end_channel = start_channel + chan_count;
		dahdi_warp_log(DSP_FUNC_LOG, "%s() : module B : start_channel = %d end_channel = %d mod_b_step = %d chan_count = %d\n",
				__FUNCTION__, start_channel, end_channel, mod_b_step, chan_count);
		res = dahdi_warp_allocate_ecan_channel_range(dsp_id, start_channel, end_channel, mod_b_step);
	}

	return  (res);
}


static int
dahdi_warp_get_transcoder_config(GpakChannelConfig_t *pconfig, unsigned int chan_bb)
{
	PktPktCfg_t *ppkt = &(pconfig->PktPkt);

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	/* By convention, A is from PCMU->G729 and B is from G729->PCMU. */
	/* Frame sizes are fixed in the image by default (10 milliseconds), 
	 * so they are not really configurable.
	 */

	ppkt->PktInCodingA = PCMU_64;			/* Packet In A Coding */
	ppkt->PktInFrameSizeA = FrameSize_10_ms;	/* A Frame Size */
	ppkt->PktOutCodingA = PCMU_64;			/* Packet Out A Coding */
	ppkt->PktOutFrameSizeA = FrameSize_10_ms;	/* A Frame Size */
	ppkt->EcanEnableA = Disabled;			/* Pkt A Echo Cancel enable */
	ppkt->VadEnableA = Disabled;			/* VAD enable */
	ppkt->ToneTypesA = Null_tone;			/* Tone Detect Types */
	ppkt->AgcEnableA = Disabled;			/* AGC Enable */
	ppkt->ChannelIdB = chan_bb;			/* Channel Id B */
	ppkt->PktInCodingB = G729;			/* Packet In B Coding */
	ppkt->PktInFrameSizeB = FrameSize_10_ms;	/* Frame Size */
	ppkt->PktOutCodingB = G729;			/* Packet Out B Coding */
	ppkt->PktOutFrameSizeB = FrameSize_10_ms;	/* Frame Size */
	ppkt->EcanEnableB = Disabled;			/* Pkt B Echo Cancel enable */
	ppkt->VadEnableB = Disabled;			/* VAD enable */
	ppkt->ToneTypesB = Null_tone;			/* Tone Detect Types */
	ppkt->AgcEnableB = Disabled;			/* AGC Enable */

	return 0;
}

static void
dahdi_warp_enable_dsp_irq(void)
{
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
			__FUNCTION__);

	enable_irq(dsp_module->irq);
}

static void
dahdi_warp_disable_dsp_irq(void)
{
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
			__FUNCTION__);

	disable_irq_nosync(dsp_module->irq);
}

static int 
dahdi_warp_get_echocan_config(GpakChannelConfig_t *pconfig, unsigned int in_out_timeslot)
{
	PcmPcmCfg_t *pPcm = &(pconfig->PcmPcm);

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with in_out_timeslot = %d\n",
		__FUNCTION__, in_out_timeslot);

	pPcm->InPortA = McBSPPort0;
	pPcm->InSlotA = in_out_timeslot;

	pPcm->OutPortA = McBSPPort0;
	pPcm->OutSlotA = in_out_timeslot;
	
	pPcm->InPortB = McBSPPort1;
	pPcm->InSlotB = in_out_timeslot;

	pPcm->OutPortB = McBSPPort1;
	pPcm->OutSlotB = in_out_timeslot;

	pPcm->EcanEnableA = Disabled;
	pPcm->EcanEnableB = Enabled;

	pPcm->AECEcanEnableA = Disabled;
	pPcm->AECEcanEnableB = Disabled;

	pPcm->FrameSize = FrameSize_10_0ms;
	pPcm->NoiseSuppressA = Disabled;
	pPcm->NoiseSuppressB = Disabled;

	pPcm->AgcEnableA = Disabled;
	pPcm->AgcEnableB = Disabled;

	pPcm->ToneTypesA = Null_tone;
	pPcm->ToneTypesB = Null_tone;

	pPcm->CIdModeA = CidDisabled;
	pPcm->CIdModeB = CidDisabled;

	pPcm->Coding = GpakVoiceChannel;

	pPcm->ToneGenGainG1A = 0;
	pPcm->OutputGainG2A = 0;
	pPcm->InputGainG3A = 0;

	pPcm->ToneGenGainG1B = 0;
	pPcm->OutputGainG2B = 0;
	pPcm->InputGainG3B = 0;

	return 0;
}

int
dahdi_warp_open_transcoder_channel_pair(unsigned int dsp_id, unsigned int *encoder_chan_id, unsigned int *decoder_chan_id)
{
	GPAK_ChannelConfigStat_t tc_config_status;
	GpakChannelConfig_t tc_channel_config;
	unsigned long flags;
	int channel_aa = 0;
	int channel_bb = 0;
	int status;
	int res = 0;
	int tcoder_count;
	int ecan_count;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	if (!dahdi_warp_get_dsp_ready(dsp_id)) {
		printk(KERN_ERR "%s() : DSP (%d) NOT Ready!\n", __FUNCTION__, dsp_id);
		res = -1;
		return (res);
	}

	spin_lock_irqsave(&dsp_module->list_spinlock, flags);

	tcoder_count = (atomic_read(&dsp_module->dsp[dsp_id].tcoder_count) >> 1);
	ecan_count = atomic_read(&dsp_module->dsp[dsp_id].ecan_count);

	if (dahdi_warp_check_capacity(ecan_count, (tcoder_count + 1)) <= 0) {
		printk(KERN_ERR "%s() : Over DSP Capacity (ECAN:%d,TRANSCODER:%d)",
				__FUNCTION__, ecan_count, (tcoder_count + 1));
		res = -1;
        }

	if ((res == 0) && (dahdi_warp_get_channel(dsp_id, &channel_aa, TRANSCODER, TRANSCODER_A_SIDE))) {
		printk(KERN_ERR "%s(): Dsp(%d) No more free channels\n",
			__FUNCTION__, dsp_id);
		res = -1;
	}

	if ((res == 0) && (dahdi_warp_get_channel(dsp_id, &channel_bb, TRANSCODER, TRANSCODER_B_SIDE))) {
		dahdi_warp_free_channel(dsp_id, channel_aa, 1);
		printk(KERN_ERR "%s(): Dsp(%d) No more free channels\n",
			__FUNCTION__, dsp_id);
		res = -1;
	}

	spin_unlock_irqrestore(&dsp_module->list_spinlock, flags);

	if (res == 0) {
		dahdi_warp_get_transcoder_config(&tc_channel_config, channel_bb);
		if ((status=gpakConfigureChannel(dsp_id, 
					channel_aa, 
					packetToPacket, 
					&tc_channel_config,
					&tc_config_status)) == CcsSuccess) {

			spin_lock_irqsave(&dsp_module->list_spinlock, flags);

			*encoder_chan_id = channel_aa;
			*decoder_chan_id = channel_bb;

			dsp_module->chans[channel_aa].encoder = 1;
			dsp_module->chans[channel_aa].paired_channel = channel_bb;
			dsp_module->chans[channel_aa].discard_frames = 0;
			dsp_module->chans[channel_aa].stats.write_count = 0;
			dsp_module->chans[channel_aa].stats.read_count = 0;
			set_bit(CHAN_READY, &dsp_module->chans[channel_aa].flags);

			dsp_module->chans[channel_bb].encoder = 0;
			dsp_module->chans[channel_bb].paired_channel = channel_aa;
			dsp_module->chans[channel_bb].discard_frames = 0;
			dsp_module->chans[channel_bb].stats.write_count = 0;
			dsp_module->chans[channel_bb].stats.read_count = 0;
			set_bit(CHAN_READY, &dsp_module->chans[channel_bb].flags);

			spin_unlock_irqrestore(&dsp_module->list_spinlock, flags);

			dahdi_warp_purge_channel_buffers(dsp_id, channel_aa);
			dahdi_warp_purge_channel_buffers(dsp_id, channel_bb);

			dahdi_warp_log(DSP_FUNC_LOG, "%s() decoder ch.id (%d) encoder ch.id (%d)\n",
				__FUNCTION__, channel_bb, channel_aa);
		} else {
			printk(KERN_ERR "%s() : Dsp(%d) Error (%d) Status (%d) Creating Transcoder Channel Pair.\n",
				__FUNCTION__, dsp_id, status, tc_config_status);
			dahdi_warp_log(DSP_FUNC_LOG, "%s() Failed to create channel. status (%d) code (%d)\n",
				__FUNCTION__, status, tc_config_status);

			spin_lock_irqsave(&dsp_module->list_spinlock, flags);

			dahdi_warp_free_channel(dsp_id, channel_aa, 1);
			dahdi_warp_free_channel(dsp_id, channel_bb, 1);

			spin_unlock_irqrestore(&dsp_module->list_spinlock, flags);

			res = -1;
		}
	}


	return (res);
}
EXPORT_SYMBOL(dahdi_warp_open_transcoder_channel_pair);

int
dahdi_warp_cleanup_transcoder_channel(unsigned int dsp_id, unsigned int channel_id)
{
	unsigned long flags;
	struct chan *pchan;
	int res = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with channel_id = %d\n",
		__FUNCTION__, channel_id);

	if (unlikely(channel_id > MAX_CHANNEL_COUNT)) {
		printk(KERN_ERR "%s() : Invalid Channel Id (%d)\n",
			__FUNCTION__, channel_id);
		res = -1;
		return (res);
	} else {
		pchan = &dsp_module->chans[channel_id];
	}

	spin_lock_irqsave(&pchan->dsp2cpu_lock, flags);
	pchan->dsp2cpu_wp = 0;
	pchan->dsp2cpu_rp = 0;
	pchan->dsp2cpu_len = 0;
	spin_unlock_irqrestore(&pchan->dsp2cpu_lock, flags);

	dahdi_warp_log(DSP_FUNC_LOG, "%s() emptied channel (%d) RX FIFO\n",
			__FUNCTION__, channel_id);

	spin_lock_irqsave(&pchan->cpu2dsp_lock, flags);
	pchan->cpu2dsp_wp = 0;
	pchan->cpu2dsp_rp = 0;
	pchan->cpu2dsp_len = 0;
	spin_unlock_irqrestore(&pchan->cpu2dsp_lock, flags);

	pchan->warn_count = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() emptied channel (%d) TX FIFO\n",
		__FUNCTION__, channel_id);

	return (res);
}
EXPORT_SYMBOL(dahdi_warp_cleanup_transcoder_channel);

int
dahdi_warp_release_transcoder_channel_pair(unsigned int dsp_id, unsigned int decoder_chan_id, unsigned int encoder_chan_id)
{
	GPAK_TearDownChanStat_t teardown_status; 
	unsigned long flags;
	int res = 0;
	int free_entry = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with decoder_chan_id = %d encoder_chan_id = %d\n",
			__FUNCTION__, decoder_chan_id, encoder_chan_id);

	if (!dahdi_warp_get_dsp_ready(dsp_id)) {
		printk(KERN_ERR "%s() : DSP (%d) NOT Ready!\n", __FUNCTION__, dsp_id);
		res = -1;
		return (res);
	}

	if ((res=gpakTearDownChannel(dsp_id, encoder_chan_id, &teardown_status)) == TdsSuccess) {
		dahdi_warp_log(DSP_FUNC_LOG, "%s() successfully destroyed decoder channel (%d) encoder channel (%d)\n",
				__FUNCTION__, decoder_chan_id, encoder_chan_id);
		free_entry = 1;
	} else {
		printk(KERN_ERR "%s() : Dsp(%d) Error (%d) tearing down decoder channel (%d) encoder channel (%d)\n",
			__FUNCTION__, dsp_id, res, decoder_chan_id, encoder_chan_id);
		free_entry = 0;
		dahdi_warp_log(DSP_FUNC_LOG, "%s() Error tearing down decoder channel id (%d)\n",
			__FUNCTION__, decoder_chan_id);
		res = -1;
	}

	spin_lock_irqsave(&dsp_module->list_spinlock, flags);

	dahdi_warp_free_channel(dsp_id, decoder_chan_id, free_entry);	
	if (encoder_chan_id != -1) {
		dahdi_warp_free_channel(dsp_id, encoder_chan_id, free_entry);	
	}

	dahdi_warp_log(DSP_FUNC_LOG, "%s() done tearing down encoder/decoder(%d,%d) channels\n",
		__FUNCTION__, encoder_chan_id, decoder_chan_id);

	spin_unlock_irqrestore(&dsp_module->list_spinlock, flags);

	return (res);
}
EXPORT_SYMBOL(dahdi_warp_release_transcoder_channel_pair);

int dahdi_warp_release_all_transcoder_channels(unsigned int dsp_id)
{
	struct chan *pchan;
	int res = 0;
	int ii;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	for (ii = 0; ii < MAX_CHANNEL_COUNT; ii++) {
		pchan = &dsp_module->chans[ii];
		if ((pchan->type == TRANSCODER) && (pchan->status == BUSY)) {
			if (pchan->encoder) {
				if (dahdi_warp_free_channel(dsp_id, ii, 1) < 0) {
					printk(KERN_ERR "%s() : Error freeing encoder channel (%d)\n",
						__FUNCTION__, ii);
					res = -1;
					break;
				}
			}  else {
				if (dahdi_warp_release_transcoder_channel_pair(dsp_id, ii, -1) < 0) {
					printk(KERN_ERR "%s() : Error freeing decoder channel (%d)\n",
						__FUNCTION__, ii);
					res = -1;
					break;
				}
			}
		}
	}

	return (res);
}
EXPORT_SYMBOL(dahdi_warp_release_all_transcoder_channels);

int dahdi_warp_release_all_echocan_channels(unsigned int dsp_id)
{
	struct chan *pchan;
	int res = 0;
	int ii;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	for (ii = 0; ii < MAX_CHANNEL_COUNT; ii++) {
		pchan = &dsp_module->chans[ii];
		if ((pchan->type == ECHOCAN) && (pchan->status == BUSY)) {
			if (dahdi_warp_free_echocan_channel(dsp_id, ii) < 0) {
				printk(KERN_ERR "%s() : Error freeing ECHO CAN channel (%d)\n",
					__FUNCTION__, ii);
				res = -1;
				break;
			}
		}
	}

	return (res);
}
EXPORT_SYMBOL(dahdi_warp_release_all_echocan_channels);

static unsigned int 
dahdi_warp_get_echocan_channel_id(unsigned int dsp_id, unsigned int in_timeslot)
{
	int ret = -1;
	int ii;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with in_timeslot = %d\n",
			__FUNCTION__, in_timeslot);

	for (ii = MIN_ECHOCAN_CHAN_NUM_RANGE1; ii <= MAX_ECHOCAN_CHAN_NUM_RANGE1; ii++) {
		if (dsp_module->chans[ii].ecan_timeslot == in_timeslot) {
			ret = ii;
			break;
		}
	}

	return (ret);
}

static
int dahdi_warp_set_echocan_channel_id(unsigned int dsp_id, unsigned int in_timeslot, unsigned int chan_id)
{
	int ret = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with in_timeslot = %d chan_id = %d\n",
			__FUNCTION__, in_timeslot, chan_id);

	if ((chan_id >= MIN_ECHOCAN_CHAN_NUM_RANGE1) || (chan_id <= MAX_ECHOCAN_CHAN_NUM_RANGE1)) {
		dsp_module->chans[chan_id].ecan_timeslot = in_timeslot;	
	} else {
		printk(KERN_ERR "%s() : invalid chan_id = %d\n",
			__FUNCTION__, chan_id);
		ret = -1;
	}

	return (ret);
}

static 
int dahdi_warp_issue_alg_control_command(unsigned int dsp_id, unsigned int chan_id, unsigned int side, unsigned int command)
{
	GPAK_AlgControlStat_t status;
	int retries = ALG_CONTROL_RETRY_COUNT;
	int ret = 0;
	int done = 0;
	
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with dsp_id = %d chan_id = %d side = %d command = %d\n",
		__FUNCTION__, dsp_id, chan_id, side, command);

	while ((!done) && (retries-- > 0)) {

		if (gpakAlgControl(dsp_id,
				chan_id,
				command,
				Null_tone,
				Null_tone,
				side,
				0,
				0,
				&status) == Ac_Success) {
			ret = 0;
			done = 1;
		} else {
			printk(KERN_ERR "%s() Error (%d) for Command  (%d) Retry (%d) for channel (%d)\n",
				__FUNCTION__, status, command, retries, chan_id);
			ret = -1;
		}

		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(ALG_CONTROL_WAIT);	
	}

	return (ret);
}

static
int dahdi_warp_enable_echocan(unsigned int dsp_id, unsigned int chan_id)
{
	unsigned long flags;
	int tcoder_count;
	int ecan_count;
	int ret = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with dsp_id = %d chan_id = %d\n",
			__FUNCTION__, dsp_id, chan_id);

	spin_lock_irqsave(&dsp_module->list_spinlock, flags);

	tcoder_count = (atomic_read(&dsp_module->dsp[dsp_id].tcoder_count) >> 1);
	ecan_count = atomic_read(&dsp_module->dsp[dsp_id].ecan_count);

	if (dahdi_warp_check_capacity((ecan_count + 1), tcoder_count) <= 0) {
		printk(KERN_ERR "%s() : Over DSP Capacity (ECAN:%d,TRANSCODER:%d)",
				__FUNCTION__, ecan_count, (tcoder_count + 1));
		ret = -1;
        }

	spin_unlock_irqrestore(&dsp_module->list_spinlock, flags);

	if ((ret == 0) && (dahdi_warp_issue_alg_control_command(dsp_id, chan_id, BDevice, EnableEcan) != 0)) {
		ret = -1;
		dahdi_warp_log(DSP_FUNC_LOG, "%s() failed to Enable Ecan for Channel %d\n",
				__FUNCTION__, chan_id);
	}

	if ((ret == 0) && (dahdi_warp_issue_alg_control_command(dsp_id, chan_id, BDevice, EnableEcanAdapt) != 0)) {
		ret = -1;
		dahdi_warp_log(DSP_FUNC_LOG, "%s() failed to Reset Ecan for Channel %d\n",
				__FUNCTION__, chan_id);
	}

	if (ret == 0) {
		atomic_inc(&dsp_module->dsp[dsp_id].ecan_count);
	}
				
	return (ret);		
}

static
int dahdi_warp_disable_echocan(unsigned int dsp_id, unsigned int chan_id)
{
	int ret = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with dsp_id = %d chan_id = %d\n",
			__FUNCTION__, dsp_id, chan_id);

	if (dahdi_warp_issue_alg_control_command(dsp_id, chan_id, BDevice, BypassEcan) != 0) {
		ret = -1;
		dahdi_warp_log(DSP_FUNC_LOG, "%s() failed to Bypass Ecan for Channel %d\n",
				__FUNCTION__, chan_id);
	}

	if (dahdi_warp_issue_alg_control_command(dsp_id, chan_id, BDevice, BypassEcanAdapt) != 0) {
		ret = -1;
		dahdi_warp_log(DSP_FUNC_LOG, "%s() failed to Bypass Ecan Adaptation for Channel %d\n",
				__FUNCTION__, chan_id);
	}

	if (ret == 0) {
		atomic_dec(&dsp_module->dsp[dsp_id].ecan_count);
	}

	return (ret);
}

int 
dahdi_warp_get_echocan_channel(unsigned int dsp_id, unsigned int in_timeslot, unsigned int tap_length, unsigned int *channel_id)
{
	int cur_chan_id;
	int res = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with dsp_id = %d in_timeslot = %d tap_length = %d\n",
		__FUNCTION__, dsp_id, in_timeslot, tap_length);

	cur_chan_id = dahdi_warp_get_echocan_channel_id(dsp_id, in_timeslot);

	if (cur_chan_id != -1) {
		dahdi_warp_enable_echocan(dsp_id, cur_chan_id);
		*channel_id = cur_chan_id;
	} else {
		printk(KERN_ERR "%s() Error getting ECAN channel for timeslot (%d)\n",
			__FUNCTION__, in_timeslot);
		res = -1;
	}

	return (res);
}
EXPORT_SYMBOL(dahdi_warp_get_echocan_channel);

static int 
dahdi_warp_alloc_echocan_channel(unsigned int dsp_id, unsigned int in_timeslot, unsigned int tap_length, unsigned int *channel_id)
{
	GPAK_ChannelConfigStat_t ecan_config_status;
	GpakChannelConfig_t ecan_channel_config;
	gpakConfigChanStatus_t status;
	s32 new_channel_id = 0;
	s32 res = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with dsp_id = %d in_timeslot = %d tap_length = %d\n",
		__FUNCTION__, dsp_id, in_timeslot, tap_length);

	if (dahdi_warp_get_channel(dsp_id, &new_channel_id, ECHOCAN, 0)) {
		printk(KERN_ERR "%s(): Dsp(%d) No more free channels\n",
			__FUNCTION__, dsp_id);
		res = -1;
	}

	if (res == 0) {
		dahdi_warp_get_echocan_config(&ecan_channel_config, in_timeslot);
	}

	if (res == 0) {
		if ((status=gpakConfigureChannel(dsp_id, new_channel_id,
						pcmToPcm, &ecan_channel_config,
						&ecan_config_status)) == CcsSuccess) {
			*channel_id = new_channel_id;
			dahdi_warp_set_echocan_channel_id(dsp_id, in_timeslot, new_channel_id);
			dahdi_warp_log(DSP_FUNC_LOG, "%s() Created ECAN channel with id (%d) timeslot = %d\n",
				__FUNCTION__, new_channel_id, in_timeslot);
		} else {
			printk(KERN_ERR "%s() : Dsp(%d) Error (%d) Status (%d) creating ECAN channel\n",
				__FUNCTION__, dsp_id, status, ecan_config_status);
			dahdi_warp_log(DSP_FUNC_LOG, "%s() Error (%d) creating ECAN channel Status (%d)\n",
				__FUNCTION__, status, ecan_config_status);
			dahdi_warp_free_channel(dsp_id, new_channel_id, 1);
			res = -1;
		}
	}

	return (res);
}

int
dahdi_warp_release_echocan_channel(unsigned int dsp_id, unsigned int channel_id)
{
	int res = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with dsp_id = %d channel_id = %d\n",
		__FUNCTION__, dsp_id, channel_id);

	if (dahdi_warp_disable_echocan(dsp_id, channel_id) == 0) {
		res = 0;
	} else {
		printk(KERN_ERR "%s() Error releasing ECAN for channel (%d)\n",
			__FUNCTION__, channel_id);
		res = -1;
	}

	return (res);

}
EXPORT_SYMBOL(dahdi_warp_release_echocan_channel);

static int
dahdi_warp_free_echocan_channel(unsigned int dsp_id, unsigned int channel_id)
{
	GPAK_TearDownChanStat_t teardown_status; 
	int res = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with dsp_id = %d channel_id = %d\n",
		__FUNCTION__, dsp_id, channel_id);

	/* release the transcoder channel */
	if ((res=gpakTearDownChannel(dsp_id, channel_id, &teardown_status)) == TdsSuccess) {
		dahdi_warp_log(DSP_FUNC_LOG, "%s() Freed ECAN channel with id (%d)\n",
				__FUNCTION__, channel_id);
		dahdi_warp_free_channel(dsp_id, channel_id, 0);	
	} else {
		printk(KERN_ERR "%s() : Dsp(%d) Error (%d) tearing down channel (%d)\n",
			__FUNCTION__, dsp_id, res, channel_id);
		dahdi_warp_log(DSP_FUNC_LOG, "%s() Error (%d) Freeing ECAN channel Status (%d)\n",
			__FUNCTION__, res, teardown_status);
	
	}

	return (res);
}

static GpakCodecs
dahdi_warp_get_target_payload_type(unsigned int encoding)
{
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	return (encoding == 1) ? PCMU_64 : G729;
}	

int 
dahdi_warp_send_packet_payload(unsigned int dsp_id, unsigned int channel_id, unsigned int encoding, unsigned char *payload, unsigned short payload_size, int queue)
{
	unsigned long flags;
	struct chan *pchan;
	int res = -1;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with dsp_id = %d channel_id = %d encoding = %d payload_size = %d queue = %d\n",
		__FUNCTION__, dsp_id, channel_id, encoding, payload_size, queue);

	if (channel_id > MAX_CHANNEL_COUNT) {
		printk(KERN_ERR "%s() : Invalid Channel Id (%d)\n",
			__FUNCTION__, channel_id);
		return (res);
	}

	pchan = &dsp_module->chans[channel_id];

	if (unlikely(!test_bit(CHAN_READY, &pchan->flags))) {
		return -EIO;
	}

	if (unlikely(pchan->type != TRANSCODER)) {
		printk(KERN_ERR "%s() : Channel Id (%d) is not a transcoder.\n",
			__FUNCTION__, channel_id);
		dahdi_warp_log(DSP_FIFO_LOG, "%s() Channel Id (%d) is not a transcoder\n",
			__FUNCTION__, channel_id);
		return (res);
	}

	if (unlikely(pchan->status != BUSY)) {
		printk(KERN_ERR "%s() : Channel Id (%d) is not allocated.\n",
			__FUNCTION__, channel_id);
		dahdi_warp_log(DSP_FIFO_LOG, "%s() Channel Id (%d) is not allocated\n",
			__FUNCTION__, channel_id);
		return (res);
	}

	pchan->stats.write_count++;

	if (queue) {

		spin_lock_irqsave(&pchan->cpu2dsp_lock, flags);

		if (pchan->cpu2dsp_len < MAX_PKT_COUNT) {
			memcpy(&pchan->cpu2dsp_buf[pchan->cpu2dsp_wp].data[0], payload, payload_size);
			pchan->cpu2dsp_buf[pchan->cpu2dsp_wp].length = payload_size;
			pchan->cpu2dsp_buf[pchan->cpu2dsp_wp].encoding = encoding;

			pchan->cpu2dsp_wp++;
			if (pchan->cpu2dsp_wp == MAX_PKT_COUNT) {
				pchan->cpu2dsp_wp = 0;
			}

			pchan->cpu2dsp_len++;
			pchan->stats.pkt_queue_count++;
			res = 0;
		} else {
			if (printk_ratelimit())
				printk(KERN_DEBUG "%s() : cpu2dsp buffer full for channel %d len %d\n",
					__FUNCTION__, channel_id, pchan->cpu2dsp_len);
		}

		spin_unlock_irqrestore(&pchan->cpu2dsp_lock, flags);

	} else {
		res = dahdi_warp_send_one_payload(dsp_id, channel_id, encoding, payload, payload_size);
		pchan->stats.pkt_queue_count++;
	}

#if (DSP_DEFERRED_SEND == 0)
	dahdi_warp_send_raw_frames(dsp_module);
#else
	/* isr will send the frames */
#endif

	return (res);
}
EXPORT_SYMBOL(dahdi_warp_send_packet_payload); 

static int
dahdi_warp_get_first_rx_block(struct chan_buf *rx_block, unsigned int dsp_id, unsigned int channel_id)
{
	struct chan *pchan;
	unsigned long flags;
	int ret = -1;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with dsp_id = %d channel_id = %d\n",
		__FUNCTION__, dsp_id, channel_id);

	if (unlikely(channel_id > MAX_CHANNEL_COUNT)) {
		printk(KERN_ERR "%s() : Dsp (%d) Invalid Channel Id (%d)\n",
			__FUNCTION__, dsp_id, channel_id);
		dahdi_warp_log(DSP_FIFO_LOG, "%s() Invalid Channel Id (%d)\n",
			__FUNCTION__, channel_id);
		return (ret);
	}

	pchan = &dsp_module->chans[channel_id];

	if (unlikely(pchan->type != TRANSCODER)) {
		printk(KERN_ERR "%s() : Channel Id (%d) is not a transcoder.\n",
			__FUNCTION__, channel_id);
		dahdi_warp_log(DSP_FIFO_LOG, "%s() Channel Id (%d) is not a transcoder\n",
			__FUNCTION__, channel_id);
		return (ret);
	}

	if (unlikely(pchan->status != BUSY)) {
		printk(KERN_ERR "%s() : Channel Id (%d) is not allocated.\n",
			__FUNCTION__, channel_id);
		dahdi_warp_log(DSP_FIFO_LOG, "%s() Channel Id (%d) is not allocated\n",
			__FUNCTION__, channel_id);
		return (ret);
	}

	spin_lock_irqsave(&pchan->dsp2cpu_lock, flags);

	if (pchan->dsp2cpu_len) {
		memcpy(rx_block, &pchan->dsp2cpu_buf[pchan->dsp2cpu_rp], sizeof(struct chan_buf));
		pchan->dsp2cpu_rp++;
		if (pchan->dsp2cpu_rp == MAX_PKT_COUNT) {
			pchan->dsp2cpu_rp = 0;
		}
		ret = 0;
		pchan->dsp2cpu_len--;
	} else {
		ret = -1;
	}

	if (pchan->dsp2cpu_len == 0)
		clear_bit(PKT_RX_READY, &pchan->flags);

	spin_unlock_irqrestore(&pchan->dsp2cpu_lock, flags);

	dahdi_warp_log(DSP_FIFO_LOG, "%s() : res = %d\n", __FUNCTION__, ret);

	return (ret);
}

int
dahdi_warp_receive_packet_payload(unsigned int dsp_id, unsigned int channel_id, unsigned int encoding, unsigned char *buffer, unsigned short buffer_size, unsigned int noblock)
{
	struct chan_buf rx_block;
	struct chan *pchan;
	ssize_t ret = 0;
	int res = -1;
	int pp = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with dsp_id = %d channel_id = %d encoding = %d buffer_size = %d noblock = %d\n",
		__FUNCTION__, dsp_id, channel_id, encoding, buffer_size, noblock);

	if (channel_id > MAX_CHANNEL_COUNT) {
		printk(KERN_ERR "%s() : Dsp (%d) Invalid Channel Id (%d)\n",
			__FUNCTION__, dsp_id, channel_id);
		return (-EINVAL);
	}

	pchan = &dsp_module->chans[channel_id];

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with channel_id = %d encoding = 0x%08x buffer = 0x%08x buffer_size = 0x%04x noblock = %d read_count = %d\n",
		__FUNCTION__, channel_id, encoding, buffer, buffer_size, noblock, (pchan) ? pchan->stats.read_count : -1);

	if (unlikely(!test_bit(CHAN_READY, &pchan->flags))) {
		return -EIO;
	}

	pchan->stats.read_count++;
	if (unlikely(pchan->stats.read_count == 1)) {
		if (dahdi_warp_reset_packet_stats(dsp_id, channel_id) < 0) {
			dahdi_warp_log(DSP_FUNC_LOG, "%s() failed to reset channel stats for channel (%d)\n",
					__FUNCTION__, channel_id);
		}
		dahdi_warp_purge_channel_buffers(dsp_id, pchan->paired_channel);
		dahdi_warp_purge_channel_buffers(dsp_id, channel_id);
	}

	if (!noblock) {
		dahdi_warp_log(DSP_FIFO_LOG, "%s() waiting for event\n",
			__FUNCTION__);
		ret = wait_event_interruptible(pchan->wq, test_bit(PKT_RX_READY, &pchan->flags));
		if (ret == -ERESTARTSYS) {
			dahdi_warp_log(DSP_FIFO_LOG, "%s() signal received.",
				__FUNCTION__);
			printk(KERN_ERR "%s() signal received for channel %d\n", __FUNCTION__, channel_id);
			return (-EINTR);
		} else if (ret == 0) {
			res = dahdi_warp_get_first_rx_block(&rx_block, dsp_id, channel_id);
			dahdi_warp_log(DSP_FIFO_LOG, "%s() payload packed for channel (%d) ready after wait\n",
				__FUNCTION__, channel_id);
		} else {
			printk(KERN_ERR "%s() : wait_event_interruptible() returned (%d)\n",
				__FUNCTION__, ret);
		}
	} else {
		if (test_bit(PKT_RX_READY, &pchan->flags)) {
			res = dahdi_warp_get_first_rx_block(&rx_block, dsp_id, channel_id);
			dahdi_warp_log(DSP_FIFO_LOG, "%s() received block of size (%d) for channel (%d)\n",
					__FUNCTION__, (res == 0) ? rx_block.length : 0, channel_id);
		} else {
			dahdi_warp_log(DSP_FIFO_LOG, "%s() nonblocking read returns (no packet for channel %d)\n",
				__FUNCTION__, channel_id);
			return (-EAGAIN);
		}
	}

	if (res == 0) {
		pchan->stats.pkt_int_rx_recvd++;
		if (rx_block.length <= buffer_size) {
			memcpy(buffer, &rx_block.data[0], rx_block.length);
			dahdi_warp_log(DSP_FIFO_LOG, "%s() returning (%d) octets to channel (%d)\n",
				__FUNCTION__, rx_block.length, channel_id);
		} else {
			printk(KERN_ERR "%s() : Dsp (%d) Channel (%d) Insufficient buffer size (%d) rx block size (%d)\n",
				__FUNCTION__, dsp_id, channel_id, buffer_size, rx_block.length);
			dahdi_warp_log(DSP_FIFO_LOG, "%s() Insufficient buffer for channel (%d) payload (%d) buffer (%d)\n",
				__FUNCTION__, channel_id, rx_block.length, buffer_size);
			return (-EINVAL);

		}
	} else {
		printk(KERN_ERR "%s() : Dsp (%d) nothing received for RX frame block for channel (%d) ret %d res %d pp %d rp %d wp %d len %d\n",
				__FUNCTION__, dsp_id, channel_id, ret, res, pp, pchan->dsp2cpu_wp, pchan->dsp2cpu_rp, pchan->dsp2cpu_len);
		dahdi_warp_log(DSP_FIFO_LOG, "%s() Nothing received for RX frame block for channel (%d)\n",
			__FUNCTION__, channel_id);
	}

	return (0);
}
EXPORT_SYMBOL(dahdi_warp_receive_packet_payload);

static void
dahdi_warp_set_dsp_ready(int dsp_id, int flag)
{
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with dsp_id = %d flag = %d\n",
		__FUNCTION__, dsp_id, flag);

	dsp_module->dsp[dsp_id].dsp_ready = flag;
}

static int 
dahdi_warp_get_dsp_ready(int dsp_id)
{
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with dsp_id = %d\n",
		__FUNCTION__, dsp_id);

	return (dsp_module->dsp[dsp_id].dsp_ready);
}

static void
dahdi_warp_queue_fifo_event(unsigned int dsp_id, int channel_id, GpakAsyncEventCode_t event_code, GpakAsyncEventData_t *event_data)
{
	struct dsp_fifo_event event_fifo;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with dsp_id = %d channel_id = %d event_code = %d\n",
		__FUNCTION__, dsp_id, channel_id, (int)event_code);

	if (atomic_read(&dsp_module->dsp[dsp_id].in_use) == 0) {
		event_fifo.channel_id = channel_id;
		event_fifo.event_code = event_code;
		memcpy(&event_fifo.event_data, event_data, sizeof(GpakAsyncEventData_t));
		__kfifo_put(dsp_module->dsp[dsp_id].event_fifo, (char*)&event_fifo, sizeof(struct dsp_fifo_event));
		wake_up_interruptible(&dsp_module->dsp[dsp_id].wq);
		dahdi_warp_log(DSP_FIFO_LOG, "%s() Queued Event Code (%d) to Channel (%d)\n",
			__FUNCTION__, (int)event_code, channel_id);
	} else {
		printk(KERN_DEBUG "%s() : Dsp (%d) Channel (%d) Event Code (%d) Discarded.\n",
			__FUNCTION__, dsp_id, channel_id, (int)event_code);
		dahdi_warp_log(DSP_FIFO_LOG, "%s() Discarding Event (%d) for Channel (%d)\n",
			__FUNCTION__, (int)event_code, channel_id);
	}

	if (event_code == EventDspReady) {
		dahdi_warp_set_dsp_ready(dsp_id, 1);
	}
}

static void 
dahdi_warp_check_event_fifo(struct warp_dsp_module *dspm)
{
	unsigned int max_messages = MAX_MSGS_TO_PROCESS;
	GpakAsyncEventCode_t event_code;
	GpakAsyncEventData_t event_data;
	unsigned int res, dsp_id;
	unsigned short channel_id;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);
	
	for (dsp_id = 0; dsp_id < MAX_DSP_COUNT; dsp_id++) {
		while (max_messages--) {
			if ((res = gpakReadEventFIFOMessage(dsp_id, &channel_id, &event_code, &event_data)) == RefEventAvail) {
				dahdi_warp_queue_fifo_event(dsp_id, channel_id, event_code, &event_data);
			} else {
				break;
			}
		}
	}
}

static int 
dahdi_warp_check_channel_stats(struct warp_dsp_module *dspm)
{
	GPAK_ChannelStatusStat_t stat_result;
	ADT_UInt32 average_cpu_usage[7];
	ADT_UInt32 peak_cpu_usage[7];
	GpakChannelStatus_t chan_status;
	gpakMcBspStats_t mc_bsp_stats;
	unsigned short channel_id;
	unsigned short dsp_id;
	int ret = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	for (dsp_id = 0; dsp_id < MAX_DSP_COUNT; dsp_id++) {

		if (!dahdi_warp_get_dsp_ready(dsp_id)) {
			continue;
		}

		for (channel_id = 0; channel_id < MAX_CHANNEL_COUNT; channel_id++) {
			if ((dspm->chans[channel_id].type != TRANSCODER) || (dspm->chans[channel_id].status != BUSY))
				continue;

			if (((ret=gpakGetChannelStatus(dsp_id, channel_id, &chan_status, &stat_result)) == CssSuccess) && (stat_result == Cs_Success)) {
				dspm->chans[channel_id].stats.pkts_to_dsp = chan_status.NumPktsToDsp;
				dspm->chans[channel_id].stats.pkts_from_dsp = chan_status.NumPktsFromDsp;
				dspm->chans[channel_id].stats.pkts_bad_header = chan_status.BadPktHeaderCnt;
				dspm->chans[channel_id].stats.pkts_underflow_cnt = chan_status.PktUnderflowCnt;
				dspm->chans[channel_id].stats.pkts_overflow_cnt = chan_status.PktOverflowCnt;
			} else {
				dspm->chans[channel_id].stats.pkts_to_dsp = -1;
				dspm->chans[channel_id].stats.pkts_from_dsp = -1;
				dspm->chans[channel_id].stats.pkts_bad_header = -1;
				dspm->chans[channel_id].stats.pkts_underflow_cnt = -1;
				dspm->chans[channel_id].stats.pkts_overflow_cnt = -1;
				printk(KERN_ERR "%s() Dsp (%d) Error (%d) retrieving stats for channel %d\n",
					__FUNCTION__, dsp_id, ret, channel_id);
				goto exit_path;
			}
		}

		if ((ret=gpakReadCpuUsage(dsp_id, average_cpu_usage, peak_cpu_usage)) == GcuSuccess) { 
			dspm->dsp[dsp_id].average_util = average_cpu_usage[0];
			dspm->dsp[dsp_id].peak_util = peak_cpu_usage[0];
		} else {
			dspm->dsp[dsp_id].average_util = -1;
			dspm->dsp[dsp_id].peak_util = -1;
		}

		if ((ret=gpakReadMcBspStats(dsp_id, &mc_bsp_stats)) == RmsSuccess) {
			dspm->dsp[dsp_id].port1.status = (unsigned int)mc_bsp_stats.Port1Status;
			dspm->dsp[dsp_id].port1.rxints = (unsigned int)mc_bsp_stats.Port1RxIntCount;
			dspm->dsp[dsp_id].port1.rxslips = (unsigned int)mc_bsp_stats.Port1RxSlips;
			dspm->dsp[dsp_id].port1.rxdmaerrs = (unsigned int)mc_bsp_stats.Port1RxDmaErrors;
			dspm->dsp[dsp_id].port1.txints = (unsigned int)mc_bsp_stats.Port1TxIntCount;
			dspm->dsp[dsp_id].port1.txslips = (unsigned int)mc_bsp_stats.Port1TxSlips;
			dspm->dsp[dsp_id].port1.txdmaerrs = (unsigned int)mc_bsp_stats.Port1TxDmaErrors;
			dspm->dsp[dsp_id].port1.frameerrs = (unsigned int)mc_bsp_stats.Port1FrameSyncErrors;
			dspm->dsp[dsp_id].port1.restarts = (unsigned int)mc_bsp_stats.Port1RestartCount;

			dspm->dsp[dsp_id].port2.status = (unsigned int)mc_bsp_stats.Port2Status;
			dspm->dsp[dsp_id].port2.rxints = (unsigned int)mc_bsp_stats.Port2RxIntCount;
			dspm->dsp[dsp_id].port2.rxslips = (unsigned int)mc_bsp_stats.Port2RxSlips;
			dspm->dsp[dsp_id].port2.rxdmaerrs = (unsigned int)mc_bsp_stats.Port2RxDmaErrors;
			dspm->dsp[dsp_id].port2.txints = (unsigned int)mc_bsp_stats.Port2TxIntCount;
			dspm->dsp[dsp_id].port2.txslips = (unsigned int)mc_bsp_stats.Port2TxSlips;
			dspm->dsp[dsp_id].port2.txdmaerrs = (unsigned int)mc_bsp_stats.Port2TxDmaErrors;
			dspm->dsp[dsp_id].port2.frameerrs = (unsigned int)mc_bsp_stats.Port2FrameSyncErrors;
			dspm->dsp[dsp_id].port2.restarts = (unsigned int)mc_bsp_stats.Port2RestartCount;
		}
	}

exit_path:

	return (ret);
}

static void
dahdi_warp_queue_dsp_payload(unsigned int dsp_id, unsigned int channel_id, unsigned char *payload, unsigned short payload_size)
{
	struct chan *pchan, *cchan;
	unsigned short rx_offset;
	unsigned int frame_size;
	int payload_complete = 0;
	unsigned long flags;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with dsp_id = %d channel_id = %d payload_size = %d\n",
		__FUNCTION__, dsp_id, channel_id, payload_size);
	
	if (channel_id > MAX_CHANNEL_COUNT) {
		printk(KERN_ERR "%s() : Dsp (%d) Invalid Channel Id (%d)\n",
			__FUNCTION__, dsp_id, channel_id);
		return;
	}

	pchan = &dsp_module->chans[channel_id];
	cchan = &dsp_module->chans[pchan->paired_channel];

	if (unlikely(pchan->type != TRANSCODER)) {
		printk(KERN_ERR "%s() : Channel Id (%d) is not a transcoder.\n",
			__FUNCTION__, channel_id);
		dahdi_warp_log(DSP_FIFO_LOG, "%s() Channel Id (%d) is not a transcoder\n",
			__FUNCTION__, channel_id);
		return;
	}

	if (unlikely(pchan->status != BUSY)) {
		printk(KERN_ERR "%s() : Channel Id (%d) is not allocated.\n",
			__FUNCTION__, channel_id);
		dahdi_warp_log(DSP_FIFO_LOG, "%s() Channel Id (%d) is not allocated\n",
			__FUNCTION__, channel_id);
		return;
	}

	if (unlikely(pchan->stats.read_count == 0)) {
		dahdi_warp_log(DSP_FIFO_LOG, "%s() Channel Id (%d) Paired Channel Id (%d) Discarding invalid frame\n",
				__FUNCTION__, channel_id, pchan->paired_channel);
		return;
	}

	if (pchan->encoder) {
		frame_size = G729_FRAME_SIZE;
	} else {
		frame_size = PCMU_FRAME_SIZE;
	}

	if (frame_size != payload_size) {
		printk(KERN_ERR "%s() : Frame size is (%d) while payload size is (%d) for channel (%d)\n",
			__FUNCTION__, frame_size, payload_size, channel_id);
	}

	rx_offset = pchan->rx_buf_offset;
	memcpy(&pchan->rx_buf[rx_offset], payload, payload_size);

	if (pchan->rx_buf_offset == 0) {
		pchan->rx_buf_offset += payload_size;
		dahdi_warp_log(DSP_FUNC_LOG, "%s() For Channel (%d) First Half payload_complete = %d rx_buf_offset = %d\n",
			__FUNCTION__, channel_id, payload_complete, pchan->rx_buf_offset);
	} else if (pchan->rx_buf_offset == frame_size) {
		pchan->rx_buf_offset = 0;
		payload_complete = 1;
		dahdi_warp_log(DSP_FUNC_LOG, "%s() For Channel (%d) Second Half payload_complete = %d rx_buf_offset = %d\n",
			__FUNCTION__, channel_id, payload_complete, pchan->rx_buf_offset);
	} else {
		if (limit_output()) {
			printk(KERN_ERR "%s() : Dsp (%d) Error in RX buffer offset for Channel (%d) with rx_buf_offset (%d) payload_size (%d)\n",
				__FUNCTION__, dsp_id, channel_id, pchan->rx_buf_offset, payload_size);
		}
		pchan->rx_buf_offset = 0;
		return;
	}

	if (payload_complete) {

		spin_lock_irqsave(&pchan->dsp2cpu_lock, flags);

		if (pchan->dsp2cpu_len < MAX_PKT_COUNT) {
			payload_size = frame_size * 2;
			memcpy(&pchan->dsp2cpu_buf[pchan->dsp2cpu_wp].data[0], pchan->rx_buf, payload_size);
			pchan->dsp2cpu_buf[pchan->dsp2cpu_wp].length = payload_size;
			pchan->dsp2cpu_buf[pchan->dsp2cpu_wp].encoding = 0;

			pchan->dsp2cpu_wp++;
			if (pchan->dsp2cpu_wp == MAX_PKT_COUNT) {
				pchan->dsp2cpu_wp = 0;
			}

			pchan->dsp2cpu_len++;
			set_bit(PKT_RX_READY, &pchan->flags);
			pchan->stats.pkt_int_rx_queued++;

			dahdi_warp_log(DSP_FIFO_LOG, "%s() : (not full) channel number = (%d) rp = %d wp = %d len = %d flags=0x%08x\n",
					__FUNCTION__, channel_id, pchan->dsp2cpu_rp, pchan->dsp2cpu_wp, pchan->dsp2cpu_len, pchan->flags);
		} else {
			if ((pchan->warn_count++ < MAX_WARN_COUNT) && (printk_ratelimit())) {
				printk(KERN_DEBUG "%s() : dsp2cpu buffer full for channel %d\n", __FUNCTION__, channel_id); 
				printk(KERN_DEBUG "%s() : (full) channel number = (%d) rp = %d wp = %d len = %d flags=0x%08x\n",
					__FUNCTION__, channel_id, pchan->dsp2cpu_rp, pchan->dsp2cpu_wp, pchan->dsp2cpu_len, (unsigned int)pchan->flags);
			}
		}

		spin_unlock_irqrestore(&pchan->dsp2cpu_lock, flags);

		wake_up_interruptible(&pchan->wq);

	} else {
		/* do nothing */
	}
}

static int
dahdi_warp_reset_system_stats(int dsp_id)
{
	resetSsMask select_mask;
	int ret = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with dsp_id = %d\n",
		__FUNCTION__, dsp_id);

	select_mask.FramingStats = 1;
	if (gpakResetSystemStats(dsp_id, select_mask) != RssSuccess) {
		printk(KERN_ERR "%s() : Error resetting DSP (%d) Framing Stats\n",
			__FUNCTION__, dsp_id);
		ret = -1;
	}

	return (ret);
}

static int
dahdi_warp_reset_cpu_stats(int dsp_id)
{
	resetSsMask select_mask;
	int ret = 0;
	
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with dsp_id = %d\n",
		__FUNCTION__, dsp_id);

	select_mask.CpuUsageStats = 1;
	if (gpakResetSystemStats(dsp_id, select_mask) != RssSuccess) {
		printk(KERN_ERR "%s() : Error resetting DSP (%d) Framing Stats\n",
			__FUNCTION__, dsp_id);
		ret = -1;
	}

	return (ret);
}

static int
dahdi_warp_reset_packet_stats(int dsp_id, int channel_id)
{
	resetCsMask select_mask;
	int ret = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with dsp_id = %d\n",
		__FUNCTION__, dsp_id);

	select_mask.PacketStats = 1;
	if (gpakResetChannelStats(dsp_id, channel_id, select_mask) != RcsSuccess) {
		printk(KERN_ERR "%s() : Error resetting DSP (%d) Packet Stats\n",
			__FUNCTION__, dsp_id);
		ret = -1;
	}

	return (ret);
}

static int
dahdi_warp_write_dsp_params(struct file *filp, const char *buffer,
			unsigned long count, void *data)
{
	struct warp_dsp_module *dspm = (struct warp_dsp_module*)data;
	char stat_cmd[MAX_STAT_CMD_LEN+1];
	int dsp_id, chan_id;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	if (strstr(buffer, "reset_framing_stats")) {

		if (sscanf(buffer, "%s %d", stat_cmd, &dsp_id) != STAT_CMD_COUNT) {
			printk(KERN_ERR "%s() : Error Invalid DSP command (%s)\n",
				__FUNCTION__, buffer);
			return -EINVAL;	
		}	

		if ((dsp_id < 0) || (dsp_id > MAX_DSP_COUNT)) {
			printk(KERN_ERR "%s() : Error Invalid DSP id (%d)\n",
				__FUNCTION__, dsp_id);
			return -EINVAL;
		}

		if (dahdi_warp_reset_system_stats(dsp_id) < 0) {
			return -EINVAL;
		}

	} else if (strstr(buffer, "reset_cpu_stats")) {

		if (sscanf(buffer, "%s %d", stat_cmd, &dsp_id) != STAT_CMD_COUNT) {
			printk(KERN_ERR "%s() : Error Invalid DSP command (%s)\n",
				__FUNCTION__, buffer);
			return -EINVAL;
		}	

		if ((dsp_id < 0) || (dsp_id > MAX_DSP_COUNT)) {
			printk(KERN_ERR "%s() : Error Invalid DSP id (%d)\n",
				__FUNCTION__, dsp_id);
			return -EINVAL;
		}

		if (dahdi_warp_reset_cpu_stats(dsp_id) < 0) {
			return -EINVAL;
		}

	} else if (strstr(buffer, "reset_packet_stats")) {

		if (sscanf(buffer, "%s %d %d", stat_cmd, &dsp_id, &chan_id) != CHANNEL_CMD_COUNT) {
			printk(KERN_ERR "%s() : Error Invalid DSP command (%s)\n",
				__FUNCTION__, buffer);
			return -EINVAL;
		}	

		if ((dsp_id < 0) || (dsp_id > MAX_DSP_COUNT)) {
			printk(KERN_ERR "%s() : Error Invalid DSP id (%d)\n",
				__FUNCTION__, dsp_id);
			return -EINVAL;
		}

		if (dspm->chans[chan_id].status != BUSY) {
			printk(KERN_ERR "%s() : Error Channel (%d) not allocated on DSP (%d)\n",
				__FUNCTION__, chan_id, dsp_id);
			return -EINVAL;
		} 

		if (dahdi_warp_reset_packet_stats(dsp_id, chan_id) < 0) {
			return -EINVAL;
		}

	} else if (strstr(buffer, "set_debug")) {
		int debug_flags;
		if (sscanf(buffer, "%s %d %x", stat_cmd, &dsp_id, &debug_flags) != DEBUG_CMD_COUNT) {
			printk(KERN_ERR "%s() : Error Invalid DSP command\n",
				__FUNCTION__);
			return -EINVAL;
		}	
		dsp_debug_flags = debug_flags;
		printk(KERN_CRIT "%s() : WarpDSP debug_flags <= 0x%08x\n", __FUNCTION__, dsp_debug_flags);
	} else if (strstr(buffer, "clear_ecan_chans")) {
		dahdi_warp_release_all_echocan_channels(DEFAULT_DSP_ID);
		printk(KERN_CRIT "%s() : Releasing all echocan channels\n", __FUNCTION__);
	} else if (strstr(buffer, "setup_ecan_chans")) {
		dahdi_warp_configure_channels(0);
		printk(KERN_CRIT "%s() : Setting up all echocan channels\n", __FUNCTION__);
	} else {
		printk(KERN_CRIT "%s() : Error Unknown/Invalid DSP Command (%s)\n",
			__FUNCTION__, buffer);
		return -EINVAL;
	}

	return count;
}

static int
dahdi_warp_read_dsp_params(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
	struct warp_dsp_module *dspm = (struct warp_dsp_module*)data;
	unsigned short channel_id;
	unsigned short dsp_id;
	int len = 0;
	int ret;

	*eof = 1;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	ret = dahdi_warp_check_channel_stats(dspm);
	if (ret) {
		printk(KERN_ERR "%s() got error (%d) from check_channel_stats\n",
			__FUNCTION__, ret);
	}

	for (dsp_id = 0; dsp_id < MAX_DSP_COUNT; dsp_id++) {

		if (!dahdi_warp_get_dsp_ready(dsp_id)) {
			len += sprintf (page + len, "Dsp (%d) NOT Ready (yet)\n", dsp_id);
			break;
		}

		len += sprintf(page + len, "%d %d %d %lu.%lu %lu.%lu 0x%08x %lu %lu %lu\n",
				dsp_id,
				atomic_read(&dspm->dsp[dsp_id].tcoder_count),
				atomic_read(&dspm->dsp[dsp_id].ecan_count),
				dspm->dsp[dsp_id].average_util / 10,
				dspm->dsp[dsp_id].average_util % 10,
				dspm->dsp[dsp_id].peak_util / 10,
				dspm->dsp[dsp_id].peak_util % 10,
				dsp_debug_flags,
				dspm->dsp[dsp_id].max_irq_time,
				dspm->dsp[dsp_id].avg_irq_time,
				dspm->isr_count);

		len += sprintf(page + len, "%d %d %d %d %d %d %d %d %d %d\n",
				dsp_id,
				dspm->dsp[dsp_id].port1.status,
				dspm->dsp[dsp_id].port1.rxints,
				dspm->dsp[dsp_id].port1.rxslips,
				dspm->dsp[dsp_id].port1.rxdmaerrs,

				dspm->dsp[dsp_id].port1.txints,
				dspm->dsp[dsp_id].port1.txslips,
				dspm->dsp[dsp_id].port1.txdmaerrs,
				dspm->dsp[dsp_id].port1.frameerrs,
				dspm->dsp[dsp_id].port1.restarts);

		len += sprintf(page + len, "%d %d %d %d %d %d %d %d %d %d\n",
				dsp_id,
				dspm->dsp[dsp_id].port2.status,
				dspm->dsp[dsp_id].port2.rxints,
				dspm->dsp[dsp_id].port2.rxslips,
				dspm->dsp[dsp_id].port2.rxdmaerrs,
				dspm->dsp[dsp_id].port2.txints,
				dspm->dsp[dsp_id].port2.txslips,
				dspm->dsp[dsp_id].port2.txdmaerrs,
				dspm->dsp[dsp_id].port2.frameerrs,
				dspm->dsp[dsp_id].port2.restarts);

	}

	for (dsp_id = 0; dsp_id < MAX_DSP_COUNT; dsp_id++) {

		if (!dahdi_warp_get_dsp_ready(dsp_id)) {
			len += sprintf (page + len, "Dsp (%d) NOT Ready (yet)\n", dsp_id);
			break;
		}

		for (channel_id = 0; channel_id < MAX_CHANNEL_COUNT; channel_id++) {
			if ((dspm->chans[channel_id].type == TRANSCODER) && (dspm->chans[channel_id].status == BUSY)) {
				len += sprintf(page + len, "%d %d %d %d %d %d %d %d %d %d %d %d %d\n", 
						channel_id,
						dspm->chans[channel_id].paired_channel,
						dspm->chans[channel_id].stats.pkts_to_dsp,
						dspm->chans[channel_id].stats.pkts_from_dsp,
						dspm->chans[channel_id].stats.pkts_bad_header,
						dspm->chans[channel_id].stats.pkts_underflow_cnt,
						dspm->chans[channel_id].stats.pkts_overflow_cnt,
						dspm->chans[channel_id].stats.pkt_queue_count,
						dspm->chans[channel_id].stats.pkt_sent_count,
						dspm->chans[channel_id].stats.pkts_none_sent,
						dspm->chans[channel_id].stats.pkt_int_rx_queued,
						dspm->chans[channel_id].stats.pkt_int_rx_recvd,
						dspm->chans[channel_id].stats.pkts_none_recvd);
			}
		}
	}

	return (len);
}

static int
dahdi_warp_proc_init(void *data)
{
	struct proc_dir_entry *entry;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	entry = create_proc_entry(DSP_PROC_ENTRY, 0, NULL);
	if (!entry)
		return -ENOMEM;

	entry->read_proc = dahdi_warp_read_dsp_params;
	entry->write_proc = dahdi_warp_write_dsp_params;
	entry->data = data; 

	return 0;
}

static void
dahdi_warp_proc_remove(void)
{
	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	remove_proc_entry(DSP_PROC_ENTRY, NULL);
}

static int
dahdi_warp_send_one_payload(unsigned short dsp_id, unsigned short channel_id, int encoding, unsigned char *payload, unsigned short payload_size)
{
	int res;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with dsp_id = %d channel_id = %d encoding = %d payload_size = %d\n",
			__FUNCTION__, dsp_id, channel_id, encoding, payload_size);

	if ((res = gpakSendPayloadToDsp(dsp_id,
					channel_id, 
					PayClassAudio,
					dahdi_warp_get_target_payload_type(encoding),	
					payload,
					payload_size)) == SpsSuccess) {
		if (limit_output()) {
			dahdi_warp_log(DSP_FIFO_LOG, "%s() sent payload (%d) octets to channel (%d) encoding (%d)\n",
					__FUNCTION__, payload_size, channel_id, encoding);
		}
	} else {
		if (limit_output()) {
			printk(KERN_DEBUG "%s() : Dsp(%d) Error (%d) sending payload type (%d) of (%d) octets to channel (%d)\n",
				__FUNCTION__, dsp_id, res, dahdi_warp_get_target_payload_type(encoding),
				payload_size, channel_id);
		}
	}

	return (res == SpsSuccess) ? 0 : -1;

}

static void
dahdi_warp_send_raw_frames(struct warp_dsp_module *dspm)
{
	unsigned short dsp_id, channel_id;
	struct chan *pchan;
	unsigned short payload_size;
	unsigned char *payload;
	unsigned int encoding;
	int count[MAX_CHANNEL_COUNT];
	struct chan_buf *pbuf;
	int pcount = 0;
	int res = -1;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	memset(count, 0, sizeof(count));

	for (dsp_id = 0; dsp_id < MAX_DSP_COUNT; dsp_id++) {
		for (channel_id = 0; channel_id < MAX_CHANNEL_COUNT; channel_id++) {
			pchan = &dspm->chans[channel_id];
			pcount = 0;
			count[channel_id] = 0;
			if ((pchan->type == TRANSCODER) && (pchan->status == BUSY) && (test_bit(CHAN_READY, &pchan->flags))) {
				while ((pchan->cpu2dsp_len > 0) && (count[channel_id]++ < 1)) { 
					pbuf = &pchan->cpu2dsp_buf[pchan->cpu2dsp_rp];
					payload = &pbuf->data[0];
					payload_size = pbuf->length;
					encoding = pbuf->encoding;

					dahdi_warp_log(DSP_FIFO_LOG, "%s() channel = %d encoding = %d size = %d\n",
							__FUNCTION__, channel_id, encoding, payload_size);

					pchan->stats.pkt_sent_count++;
					res = dahdi_warp_send_one_payload(dsp_id, channel_id, encoding, payload, payload_size);
					pcount++;
					if (res == 0) {
						if (limit_output()) {
							dahdi_warp_log(DSP_FIFO_LOG, "%s() sent payload to channel (%d) PC Sent (%d) Queued (%d) Loop (%d)\n",
									__FUNCTION__,
									channel_id,
									pchan->stats.pkt_sent_count,
									pchan->stats.pkt_queue_count,
									count[channel_id]);
						}
					} else {
						if (limit_output()) {
							printk(KERN_DEBUG "%s() : Error Sending on Channel (%d) PC Sent (%d) Queued (%d) Loop (%d)\n",
								__FUNCTION__, 
								channel_id,
								pchan->stats.pkt_sent_count,
								pchan->stats.pkt_queue_count,
								count[channel_id]); 		
						}
						break;
					}

					pchan->cpu2dsp_rp++;
					if (pchan->cpu2dsp_rp == MAX_PKT_COUNT) {
						pchan->cpu2dsp_rp = 0;
					}
					
					if (pchan->cpu2dsp_len > 0) {
						pchan->cpu2dsp_len--;
					}
				}

				if (count[channel_id] == 0) {
					dahdi_warp_log(DSP_FIFO_LOG, "%s() channel = %d nothing to send (res = %d).\n",
							__FUNCTION__, channel_id, res);
					pchan->discard_frames++;
					pchan->stats.pkts_none_sent++;
				}
			}
			dahdi_warp_log(DSP_FUNC_LOG, "%s() channel = %d pcount = %d\n",
					__FUNCTION__, channel_id, pcount);
		}
	}
}

static int
dahdi_warp_purge_channel_buffers(int dsp_id, int channel_id)
{
	unsigned char payload_buffer[MAX_PKT_SIZE];
	unsigned short payload_size;
	unsigned short payload_type;
	unsigned short pair_channel_id;
	int max_buffer_count = MAX_PURGE_COUNT;
	ADT_UInt32 payload_timestamp;
	GpakPayloadClass payload_class;
	int purge_count = 0;
	int ret = -1;
	int res;
	struct chan *pchan;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called with dsp_id = %d channel_id = %d\n",
		__FUNCTION__, dsp_id, channel_id);

	if (!dahdi_warp_get_dsp_ready(dsp_id)) {
		printk(KERN_ERR "%s() : DSP (%d) NOT Ready!\n", __FUNCTION__, dsp_id);
		return (ret);
	}

	pchan = &dsp_module->chans[channel_id];
	if ((pair_channel_id = pchan->paired_channel) == INVALID_CHANNEL_ID) {
		printk(KERN_ERR "%s() : invalid paired channel for channel %d\n",
			__FUNCTION__, pair_channel_id);
		return (ret);
	}

	if (unlikely(pchan->type != TRANSCODER)) {
		printk(KERN_ERR "%s() : Channel Id (%d) is not a transcoder.\n",
			__FUNCTION__, channel_id);
		dahdi_warp_log(DSP_FIFO_LOG, "%s() Channel Id (%d) is not a transcoder\n",
			__FUNCTION__, channel_id);
		return (ret);
	}

	if (unlikely(pchan->status != BUSY)) {
		printk(KERN_ERR "%s() : Channel Id (%d) is not allocated.\n",
			__FUNCTION__, channel_id);
		dahdi_warp_log(DSP_FIFO_LOG, "%s() Channel Id (%d) is not allocated\n",
			__FUNCTION__, channel_id);
		return (ret);
	}

	while  (max_buffer_count-- > 0) {
		payload_size = MAX_PKT_SIZE;
		if ((res = gpakGetPayloadFromDsp(dsp_id,
						pair_channel_id,
						&payload_class,
						&payload_type,
						payload_buffer,
						&payload_size,
						&payload_timestamp)) == GpsSuccess) {
			purge_count++;
		} else {
			dahdi_warp_log(DSP_FIFO_LOG, "%s() Channel  Id (%d) max_buffer_count = %d res = %d\n",
					__FUNCTION__, channel_id, max_buffer_count, res);
			break;
		}
	}

	dahdi_warp_log(DSP_FIFO_LOG, "%s() Channel Id (%d) purged (%d) packets\n",
			__FUNCTION__, channel_id, purge_count);

	ret = 0;
	return (ret);
}

static void 
dahdi_warp_recv_transcoded_frames(struct warp_dsp_module *dspm)
{
	unsigned short dsp_id, channel_id, res, payload_size;
	unsigned short pair_channel_id;
	unsigned char payload_buffer[MAX_PKT_SIZE];
	GpakPayloadClass payload_class;
	unsigned short payload_type;
	ADT_UInt32 payload_timestamp;
	struct chan *pchan;
	int pkt_count = 0;

	dahdi_warp_log(DSP_FUNC_LOG, "%s() called\n",
		__FUNCTION__);

	for (dsp_id = 0; dsp_id < MAX_DSP_COUNT; dsp_id++) {
		for (channel_id = 0; channel_id < MAX_CHANNEL_COUNT; channel_id++) {
			pchan = &dspm->chans[channel_id];
			pkt_count = 0;
			if ((pchan->type == TRANSCODER) && (pchan->status == BUSY) && (test_bit(CHAN_READY, &pchan->flags))) {
				payload_size = MAX_PKT_SIZE;
				if ((pair_channel_id = pchan->paired_channel) == INVALID_CHANNEL_ID) {
					printk(KERN_ERR "%s() : invalid paired channel for channel %d\n",
						__FUNCTION__, pair_channel_id);
					continue;
				}
				do {
					if ((res = gpakGetPayloadFromDsp(dsp_id,
									pair_channel_id,
									&payload_class,
									&payload_type,
									payload_buffer,
									&payload_size,
									&payload_timestamp)) == GpsSuccess) {
						dahdi_warp_log(DSP_FIFO_LOG, "%s() got payload (%d) octets of type (%d) for channel (%d) timestamp (%d)\n",
							__FUNCTION__, payload_size, payload_type, channel_id, payload_timestamp);
						if (payload_size == 0) {
							break;
						}
						if (pkt_count++ < FRAMES_PER_INTR) {
							dahdi_warp_queue_dsp_payload(dsp_id, channel_id, payload_buffer, payload_size);
						}
					} else {
						switch  (res) {
							case GpsNoPayload:
								dahdi_warp_log(DSP_FIFO_LOG, "%s() no payload for channel (%d) encoder(%d)\n",
									__FUNCTION__, channel_id, pchan->encoder);
								break;

							case GpsInvalidChannel:
								if (limit_output()) {
									printk(KERN_ERR "%s() : DSP (%d) Invalid Channel (%d)\n",
										__FUNCTION__, dsp_id, channel_id);
								}
								break;

							case GpsBufferTooSmall:
								if (limit_output()) {
									printk(KERN_ERR "%s() : DSP (%d) Buffer Too Small (%d)\n",
										__FUNCTION__, dsp_id, channel_id);
								}
								break;

							case GpsInvalidDsp:
								if (limit_output()) {
									printk(KERN_ERR "%s() : DSP (%d) Invalid Dsp ID\n",
										__FUNCTION__, dsp_id);
								}
								break;
							
							case GpsDspCommFailure:
								if (limit_output()) {
									printk(KERN_ERR "%s() : DSP (%d) Communications  Failure\n",
										__FUNCTION__, dsp_id);
								}
								break;
			
							default:
								if (limit_output()) {
									printk(KERN_ERR "%s() : DSP (%d) Unknown Error (%d)\n",
										__FUNCTION__, dsp_id, res);
								}
								break;
						}
					}
				} while (res == GpsSuccess);

				if (pkt_count == 0) 
					pchan->stats.pkts_none_recvd++;
			}
		}
	}
}

static void dsp_device_release(struct device *dev) {}

static int
__init dahdi_warp_dsp_init(void)
{
	struct device_node *np = NULL;
	struct device_node *child = NULL;
	int event_fifo_size = EVENT_FIFO_MAX_MSGS * sizeof(struct dsp_fifo_event);
	int chr_region_alloc = 0;
	int sysfs_class_alloc = 0;
	int cdev_alloc = 0;
	int err = -EINVAL;
	int ii, jj;
	unsigned board_id0 = 0;
	unsigned board_id1 = 0;
	unsigned board_id2 = 0;
	unsigned value;

	np = of_find_compatible_node(NULL, NULL, "gpio-boardid");
	if (!np) {
		printk(KERN_ERR "%s() : Unable to find gpio-boardid\n",
			__FUNCTION__);
		goto error_cleanup;
	}

	for_each_child_of_node(np, child) {
		if (strcmp(child->name, "board_id0") == 0) {
			board_id0 = of_get_gpio(child, 0);	
		} else if (strcmp(child->name, "board_id1") == 0) {
			board_id1 = of_get_gpio(child, 0);
		} else if (strcmp(child->name, "board_id2") == 0) {
			board_id2 = of_get_gpio(child, 0);
		}
	}
	of_node_put(np);

	value = 0;
	if (gpio_get_value(board_id0)) {
		value |= 0x0004;
	} 

	if (gpio_get_value(board_id1)) {
		value |= 0x0002;
	} 

	if (gpio_get_value(board_id2)) {
		value |= 0x0001;
	} 

	if ((value == PIK_98_00910_NO_OBFXS) || (value == PIK_98_00910)) {
		printk(KERN_ERR "DSP not Detected (boardid=0x%04x). Driver not loading.\n", value);
		return (-ENODEV);
	} else {
		printk(KERN_INFO "DSP Detected (boardid=0x%04x). Driver loading.\n", value);
	}

	dsp_module = kzalloc(sizeof(struct warp_dsp_module), GFP_KERNEL);
	if (!dsp_module) {
		printk(KERN_ERR "%s() : Insufficient memory\n",
			__FUNCTION__);
		return (-ENOMEM);
	}

	spin_lock_init(&dsp_module->list_spinlock);
	for (ii = 0; ii < MAX_CHANNEL_COUNT; ii++) {
		clear_bit(CHAN_READY, &dsp_module->chans[ii].flags);
		dsp_module->chans[ii].status = AVAILABLE;
		dsp_module->chans[ii].type= UNSET;
		spin_lock_init(&dsp_module->chans[ii].dsp2cpu_lock);
		spin_lock_init(&dsp_module->chans[ii].cpu2dsp_lock);
		dahdi_warp_reset_channel_stats(DEFAULT_DSP_ID, ii);
	}

	np = of_find_compatible_node(NULL, NULL, "pika,fpga");
        if (!np) {
                printk(KERN_ERR "%s() : Unable to find fpga\n",
			__FUNCTION__);
                goto error_cleanup;
        }

	dsp_module->fpga = of_iomap(np, 0);
	if (!dsp_module->fpga) {
		printk(KERN_ERR "%s() : Unable to get FPGA address\n",
			__FUNCTION__);
		err = -ENOMEM;
		goto error_cleanup;
	}

	of_node_put(np);
	np = NULL;

	np = of_find_compatible_node(NULL, NULL, "pika,dsp");
	if (!np) {
		printk(KERN_ERR "%s() : Unable to find dsp\n",
			__FUNCTION__);
		goto error_cleanup;
	}

	dsp_module->dsp_host_port = of_iomap(np, 0);
	if (!dsp_module->dsp_host_port) {
		printk(KERN_ERR "%s() : Unable to get DSP address\n",
			__FUNCTION__);
		err = -ENOMEM;
		goto error_cleanup;
	}

	dsp_module->irq = irq_of_parse_and_map(np, 0);
	if (dsp_module->irq == NO_IRQ) {
		printk(KERN_ERR "%s() : irq_of_parse_and_map failure\n",
			__FUNCTION__);
		goto error_cleanup;
	}

	of_node_put(np);
	np = NULL;

	for (ii = 0; ii < MAX_DSP_COUNT; ii++) {
		dsp_module->dsp[ii].id = ii;
		dsp_module->dsp[ii].hp_address = dsp_module->dsp_host_port;
		init_MUTEX(&dsp_module->dsp[ii].access_sem);
		init_MUTEX(&dsp_module->dsp[ii].cmd_access_sem);
		init_MUTEX(&dsp_module->dsp[ii].dsp_cmd_read_sem);
		init_MUTEX(&dsp_module->dsp[ii].dsp_cmd_write_sem);
		init_waitqueue_head(&dsp_module->dsp[ii].wq);
		for (jj = 0; jj < MAX_CHANNEL_COUNT; jj++) {
			init_waitqueue_head(&dsp_module->chans[jj].wq);
		}	
		atomic_set(&dsp_module->dsp[ii].in_use, -1);
		atomic_set(&dsp_module->dsp[ii].tcoder_count, 0);
		atomic_set(&dsp_module->dsp[ii].ecan_count, 0);
		dsp_module->dsp[ii].event_fifo = kfifo_alloc(event_fifo_size, GFP_KERNEL, NULL);
		if (IS_ERR(dsp_module->dsp[ii].event_fifo)) {
			printk(KERN_ERR "%s() : Could not allocate fifo event_fifo\n",
				__FUNCTION__);
			iounmap(dsp_module->fpga);
			iounmap(dsp_module->dsp_host_port);
			return (-ENOMEM);
		}
	}

	err = dahdi_warp_proc_init(dsp_module);

	if (err) {
		printk(KERN_ERR "%s() : Unable to install proc entry for DSP stats\n",
			__FUNCTION__);
		goto error_cleanup;
	}

	err = alloc_chrdev_region(&dsp_dev, 0, 1, "warp-dsp");
	if (err) {
		printk(KERN_ERR "%s() : Unable to allocate chrdev region\n",
			__FUNCTION__);
		goto error_cleanup;
	} else {
		chr_region_alloc = 1;
	}

	dsp_module->class_dev.devt = MKDEV(MAJOR(dsp_dev), 0);
	cdev_init(&dsp_module->cdev, &dsp_ops);
	err = cdev_add(&dsp_module->cdev, dsp_module->class_dev.devt, 1);
	if (err) {
		printk(KERN_ERR "%s() : Unable to add char device\n",
			__FUNCTION__);
		goto error_cleanup;
	} else {
		cdev_alloc = 1;
	}

	memset(&dsp_class, 0, sizeof(dsp_class));
	dsp_class.name = dsp_class_name;
	dsp_class.owner = THIS_MODULE;
	dsp_class.dev_release = dsp_device_release;

	err = class_register(&dsp_class);
	if (err) {
		printk(KERN_ERR "%s() : Error registering device class\n",
			__FUNCTION__);
	} else {
		sysfs_class_alloc = 1;
	}

	dsp_module->class_dev.class  = &dsp_class;
	dev_set_drvdata(&dsp_module->class_dev, dsp_module);
	dev_set_name(&dsp_module->class_dev, "dsp");
	err = device_register(&dsp_module->class_dev);
	if (err) {
		printk(KERN_ERR "%s() : Could not register class device.\n",
			__FUNCTION__);
		goto error_cleanup;
	}

	err = request_irq(dsp_module->irq, warp_dsp_isr, IRQF_SHARED,
				"warpdsp", dsp_module);
	if (err) {
		printk(KERN_ERR "%s() : Unable to request IRQ\n",
			__FUNCTION__);
		goto error_cleanup;
	}

	printk(KERN_INFO "WARP DSP initialization complete.\n");
	return 0;

error_cleanup:

	if (chr_region_alloc) {
		unregister_chrdev_region(dsp_dev, 1);
	}

	if (cdev_alloc) {
		cdev_del(&dsp_module->cdev);
	}

	if (sysfs_class_alloc) {
		class_unregister(&dsp_class);
	}

	if (np)
		of_node_put(np);

	dahdi_warp_proc_remove();

	return (err);
}

static void
__exit dahdi_warp_dsp_cleanup(void)
{
	dahdi_warp_proc_remove();

	device_unregister(&dsp_module->class_dev);

	class_unregister(&dsp_class);

	unregister_chrdev_region(dsp_dev, 1);

	free_irq(dsp_module->irq, dsp_module);

	if (dsp_module->fpga)
		iounmap(dsp_module->fpga);

	if (dsp_module->dsp_host_port)
		iounmap(dsp_module->dsp_host_port);

	printk(KERN_INFO "DSP cleanup complete.\n");
}

MODULE_LICENSE("GPL");
module_init(dahdi_warp_dsp_init);
module_exit(dahdi_warp_dsp_cleanup);

module_param(taillength, int, S_IRUGO | S_IWUSR);
module_param(nlptype, int, S_IRUGO | S_IWUSR);
module_param(adaptenable, int, S_IRUGO | S_IWUSR);
module_param(g165detenable, int, S_IRUGO | S_IWUSR);
module_param(dbltalkthreshold, int, S_IRUGO | S_IWUSR);
module_param(nlpthreshold, int, S_IRUGO | S_IWUSR);
module_param(nlpupperlimitthresh, int, S_IRUGO | S_IWUSR);
module_param(nlpupperlimitprethresh, int, S_IRUGO | S_IWUSR);
module_param(nlpmaxsupp, int, S_IRUGO | S_IWUSR);
module_param(nlpcngthreshold, int, S_IRUGO | S_IWUSR);
module_param(nlpadaptlimit, int, S_IRUGO | S_IWUSR);
module_param(crosscorrlimit, int, S_IRUGO | S_IWUSR);
module_param(numfirsegments, int, S_IRUGO | S_IWUSR);
module_param(firsegmentlen, int, S_IRUGO | S_IWUSR);
module_param(firtapcheckperiod, int, S_IRUGO | S_IWUSR);
module_param(maxdoubletalkthreshold, int, S_IRUGO | S_IWUSR);
module_param(mixedfourwiremode, int, S_IRUGO | S_IWUSR);
module_param(reconvcheckenable, int, S_IRUGO | S_IWUSR);
module_param(dsp_debug_flags, int, S_IRUGO | S_IWUSR);
module_param(test_chan_count, int, S_IRUGO | S_IWUSR);
