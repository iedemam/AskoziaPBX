#include "driver.h"

#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/kfifo.h>
#include <linux/poll.h>

int debug = 1; /* SAM DBG */
module_param(debug, int, 0664);

int use_fxs = 0;
module_param(use_fxs, int, 0444);

struct gsm_req {
	int type;
	unsigned param1;
	unsigned param2;
	};

/* Only one instance of the driver */
static struct gsm_card *gsm_card;
static unsigned gsm_int_mask;

/* ioctl */
#define GSM_IOC_MAGIC		'G'
#define GSM_SETLED		_IO(GSM_IOC_MAGIC, 0)
#define GSM_GETLED		_IO(GSM_IOC_MAGIC, 1)
#define GSM_SETSTATE		_IO(GSM_IOC_MAGIC, 2)
#  define STATE_CONNECTED       (1 << 0)
#  define STATE_HANGUP          (1 << 1)
#define GSM_SETNEXTSIMSLOT	_IO(GSM_IOC_MAGIC, 3)
#define GSM_KILLME		_IO(GSM_IOC_MAGIC, 4)
#define GSM_PEEK_FPGA		_IO(GSM_IOC_MAGIC, 5)
#define GSM_POKE_FPGA		_IO(GSM_IOC_MAGIC, 6)
#define GSM_POWERCYCLE_RADIO	_IO(GSM_IOC_MAGIC, 7)
#define GSM_SET_RX_GAIN		_IO(GSM_IOC_MAGIC, 8)
#define GSM_SET_TX_GAIN		_IO(GSM_IOC_MAGIC, 9)
#define GSM_IOC_MAXNR		_IOC_NR(GSM_SET_TX_GAIN)

#define CTL_RX_FIFO_SIZE 320 /* SAM CHECKME! */
#define CTL_TX_FIFO_SIZE 256 /* SAM CHECKME! */

#define PORT_RX_FIFO_SIZE 512 /* SAM CHECKME! */
#define PORT_TX_FIFO_SIZE 1024 /* SAM CHECKME! */

#define PORT_RX_THRESHOLD 160 /* SAM CHECKME! */

#define CHUNKSIZE	(2 << 4) /* SAM Checkme */

#define MAX_FPGA_RANGE		0x1000

#define GSM_1A_STAT_MASK	 0x10000000	/* GSM STATUS MASKS */
#define GSM_2A_STAT_MASK	 0x20000000
#define GSM_1B_STAT_MASK	 0x40000000
#define GSM_2B_STAT_MASK	 0x80000000

#define GSM_1A_PWR_MASK		 0x10
#define GSM_2A_PWR_MASK		 0x20
#define GSM_1B_PWR_MASK		 0x40
#define GSM_2B_PWR_MASK		 0x80

static struct class      gsm_class;
static char              gsm_class_name[] = "gsm";

static dev_t dev_first;

static int num_chans;

static inline void delay(int ms)
{
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(msecs_to_jiffies(ms));
}

static int gsm_power_up(struct gsm_card *gsm, unsigned expected);
static int gsm_power_cycle(struct gsm_card *gsm, unsigned expected);
static void gsm_set_rx_codec_gain(struct gsm_chan *chan, unsigned char value);
static void gsm_set_tx_codec_gain(struct gsm_chan *chan, unsigned char value);

/* ---- interrupt handling ---- */

static u8 call_ready[] = "Call Ready\r\n";
#define CALL_READY_STATE (sizeof(call_ready) - 1)

void look_for_call_ready(struct gsm_chan *chan, u8 *buf, u8 n)
{
	u8 i, state = chan->call_ready_state;

	for (i = 0; i < n; ++i)
		if (buf[i] == call_ready[state]) {
			++state;
			if (state == CALL_READY_STATE) {
				printk("%d: Call Ready\n", chan->id); // SAM DBG
				break;
			}
		} else
			state = 0;

	chan->call_ready_state = state;
}

/* The gsm_uart_read call is very expensive and *must* be called from
 * the ISR to ack the interupt. This version reads the entire fifo
 * once. It is a good tradeoff between time in ISR and ISR call
 * frequency. It tends to read 6-21 chars per call. */
static void gsm_rx_data(struct gsm_chan *chan)
{
	u8 data[64], n, i;

	gsm_uart_read(chan, IS752_RXLVL, &n);

	for (i = 0; i < n; ++i)
		gsm_uart_read(chan, IS752_RHR, &data[i]);

	if (unlikely(chan->call_ready_state != CALL_READY_STATE))
		look_for_call_ready(chan, data, n);

	__kfifo_put(chan->ctl.rx_fifo, data, n);
	wake_up(&chan->ctl.rx_queue);
}

static inline int gsm_interrupt(struct gsm_chan *chan)
{
	u8 iir, data;

	gsm_uart_read(chan, IS752_IIR, &iir);

	switch (iir & 0x3f) {
	case 0x06: /* Receive Line Status error */
		gsm_uart_read(chan, IS752_LSR, &data);

		if (data & 0x9e)
			printk(KERN_WARNING "%d: LSR errors %x\n",
			       chan->id, data);

		if (data & 1)
			gsm_rx_data(chan);
		break;
	case 0x0c: /* Receiver time-out */
	case 0x04: /* RHR interrupt */
		gsm_rx_data(chan);
		break;
	case 0x02: /* THR interrupt */
		chan->ctl.tx_empty = 1;
		wake_up(&chan->ctl.tx_queue);
		return 1;
	default:
		if ((iir & 1) == 0)
			printk(KERN_INFO "UNEXPECTED IIR %x\n", iir);
		/* else spurious */
		return 0;
	}

	return 1;
}

static irqreturn_t gsm_isr(int irq, void *arg)
{
	struct gsm_card *gsm = arg;
	unsigned iccr = gsm_fpga_read(gsm, FPGA_ICCR);
	int i, saw = 0;

	if (iccr & gsm_int_mask)
		for (i = 0; i < GSM_MAX_CHANS; ++i)
			if (gsm->chans[i])
				saw += gsm_interrupt(gsm->chans[i]);

	/* DMA interrupts handled in callback */

	return saw ? IRQ_HANDLED : IRQ_NONE;
}

/* ---- DMA handling ---- */

#define FRAMES_PER_BUFFER	320 /* 40ms / (125us per frame) */
#define FRAMES_PER_TRANSFER	8
#define FRAMESIZE		64

static void dma_rx_data(struct gsm_chan *chan, u8 *buf)
{
	unsigned char tmpbuf[FRAMES_PER_TRANSFER];
	int i;

	buf += chan->timeslot; /* get the right timeslot */

	for (i = 0; i < FRAMES_PER_TRANSFER; ++i) {
		tmpbuf[i] = *buf;
		buf += FRAMESIZE;
	}

	__kfifo_put(chan->port.rx_fifo, tmpbuf, sizeof(tmpbuf));
	wake_up(&chan->port.rx_queue);
}

static void dma_tx_data(struct gsm_chan *chan, u8 *buf)
{
	u8 tmpbuf[FRAMES_PER_TRANSFER];
	int i;

	buf += chan->timeslot; /* get the right timeslot */

	if (__kfifo_len(chan->port.tx_fifo) >= FRAMES_PER_TRANSFER)
		__kfifo_get(chan->port.tx_fifo, tmpbuf, FRAMES_PER_TRANSFER);
	else
		memset(tmpbuf, 0xff, FRAMES_PER_TRANSFER); /* silence */

	for (i = 0; i < FRAMES_PER_TRANSFER; ++i) {
		*buf = tmpbuf[i];
		buf += FRAMESIZE;
	}

	wake_up(&chan->port.tx_queue);
}

void gsm_dma_callback(int cardid)
{
	unsigned int bytes = FRAMES_PER_TRANSFER * FRAMESIZE;
	unsigned int bcount = FRAMES_PER_BUFFER / FRAMES_PER_TRANSFER;
	u8 *cur_rx_buf, *cur_tx_buf;
	int rx_index, tx_index, i;
	int index = gsm_fpga_read(gsm_card, FPGA_DMA_SG_IDX) & 0x1ff;

	index = index / 2;

	/* pick the RX buffer before the current one */
	rx_index = ((index - 1) >= 0) ? (index - 1) : (index - 1 + bcount);

	/* pick the TX buffer after the current one */
	tx_index = ((index + 1) < bcount) ? (index + 1) : (index + 1 - bcount);

	/* calculate the position of the rx and tx buffers */
	cur_tx_buf = gsm_card->tx_buf + tx_index * bytes;
	cur_rx_buf = gsm_card->rx_buf + rx_index * bytes;

	/* SAM we should also handle if we missed some interrupts */
	/* Yes 0 means port is in use */
	for (i = 0; i < GSM_MAX_CHANS; ++i)
		if (gsm_card->chans[i] &&
		    atomic_read(&gsm_card->chans[i]->port.in_use) == 0) {
			dma_tx_data(gsm_card->chans[i], cur_tx_buf);
			dma_rx_data(gsm_card->chans[i], cur_rx_buf);
		}
}

/* ---- file operations ---- */

static void chan_on(struct gsm_chan *chan)
{
	if (atomic_add_return(1, &chan->use_count) > 1)
		return;

	__kfifo_reset(chan->ctl.rx_fifo);
	__kfifo_reset(chan->ctl.tx_fifo);
	__kfifo_reset(chan->port.rx_fifo);
	__kfifo_reset(chan->port.tx_fifo);

	// SAM this is not right!
	up(&chan->ctl.write_sem);

#ifdef SAM_NOT_YET
	if (atomic_read(&chan->on_off_state) != SW_OFF) {
		LOG_CHAN(chan, "switching channel on (delayed)\n");
		atomic_set(&chan->req_on, 1);
	} else {
		LOG_CHAN(chan, "switching channel on\n");
		atomic_set(&chan->on_off_state, SW_OFF_TO_ON);
	}
#endif
}

static void chan_off(struct gsm_chan *chan)
{
	if (atomic_sub_return(1, &chan->use_count))
		return;

	LOG_CHAN(chan, "switching channel off\n");

#if 0
	if (!atomic_dec_and_test(&chan->req_on)) {
		atomic_inc(&chan->req_on);
		down(&chan->ctl.write_sem);
	}
	atomic_set(&chan->on_off_state, SW_ON_TO_OFF);
	atomic_set(&chan->call_state, STANDBY);
#endif
}

static int ctl_open(struct inode *inode, struct file *file)
{
	struct gsm_ctl *ctl = container_of(inode->i_cdev, struct gsm_ctl, cdev);
	struct gsm_chan *chan = container_of(ctl, struct gsm_chan, ctl);

	if (file->f_flags & O_NONBLOCK) {
		if (down_trylock(&ctl->use_sem))
			return -EAGAIN;
	} else {
		if (down_interruptible(&ctl->use_sem))
			return -ERESTARTSYS;
	}

	switch (ctl->use_count) {
	case 0:
		break;
	case 1:
		if (ctl->used_flags == O_RDWR ||
			(file->f_flags & O_RDWR) == O_RDWR ||
			ctl->used_flags == (file->f_flags & (O_RDONLY | O_WRONLY))) {
			up(&chan->ctl.use_sem);
			DEBUG_CTL(chan, "open: rejected second open attempt (%s)\n",
					  file->f_flags & O_RDWR ? "rw" : file->f_flags & O_WRONLY? "w" : "r");
			return -EBUSY;
		}
		break;
	default:
		up(&chan->ctl.use_sem);
		DEBUG_CTL(chan, "open: third open attempt rejected\n");
		return -EBUSY;
	}

	ctl->used_flags |= file->f_flags & (O_RDONLY | O_WRONLY | O_RDWR);
	++ctl->use_count;
	file->private_data = chan;
	chan_on(chan);

	/* Fake a call ready */
	if (!(file->f_flags & O_WRONLY))
		if (chan->call_ready_state == CALL_READY_STATE) {
			printk("Send call ready\n"); // SAM DBG
			__kfifo_put(chan->ctl.rx_fifo,
				    call_ready, CALL_READY_STATE);
		}

	up(&chan->ctl.use_sem);
	DEBUG_CTL(chan, "open: %s, %s\n", file->f_flags & O_NONBLOCK ? "non blocking" : "blocking allowed",
			  file->f_flags & O_RDWR ? "rw" : file->f_flags & O_WRONLY? "w" : "r");

	return 0;
}

static int ctl_release(struct inode *inode, struct file *file)
{
	struct gsm_chan *chan = file->private_data;

	if (file->f_flags & O_NONBLOCK) {
		if (down_trylock(&chan->ctl.use_sem))
			return -EAGAIN;
	} else {
		if (down_interruptible(&chan->ctl.use_sem))
			return -ERESTARTSYS;
	}

	chan_off(chan);
	chan->ctl.used_flags &= ~(file->f_flags & (O_RDONLY | O_WRONLY | O_RDWR));
	--chan->ctl.use_count;
	up(&chan->ctl.use_sem);
	DEBUG_CTL(chan, "release\n");

	return 0;
}

static unsigned int ctl_poll(struct file *file, struct poll_table_struct *wait)
{
	struct gsm_chan *chan = file->private_data;
	unsigned int mask = 0;

	poll_wait(file, &chan->ctl.rx_queue, wait);
	poll_wait(file, &chan->ctl.tx_queue, wait);
	if (!down_trylock(&chan->ctl.read_sem)) {
		if (__kfifo_len(chan->ctl.rx_fifo))
			mask |= POLLIN | POLLRDNORM;
		up(&chan->ctl.read_sem);
	}
	if (!down_trylock(&chan->ctl.write_sem)) {
		if (__kfifo_len(chan->ctl.tx_fifo) < CTL_TX_FIFO_SIZE)
			mask |= POLLOUT | POLLWRNORM;
		up(&chan->ctl.write_sem);
	}

	return mask;
}

#define RX_FIFO_HAS_DATA __kfifo_len(chan->ctl.rx_fifo)

static ssize_t ctl_read(struct file *file, char *buf,
			size_t count, loff_t *offset)
{
	struct gsm_chan *chan = file->private_data;
	unsigned char tmpbuf[CHUNKSIZE];
	int len, chunklen, readcount = 0;

	if (!count)
		return 0;
	if (count < 0)
		return -EINVAL;

	if (file->f_flags & O_NONBLOCK) {
		if (down_trylock(&chan->ctl.read_sem))
			return -EAGAIN;
	} else {
		if (down_interruptible(&chan->ctl.read_sem))
			return -ERESTARTSYS;
	}

	while (readcount < count) {
		chunklen = count - readcount;
		if (chunklen > sizeof(tmpbuf))
		    chunklen = sizeof(tmpbuf);

		len = __kfifo_get(chan->ctl.rx_fifo, tmpbuf, chunklen);

		if (len > 0) {
			if (copy_to_user(buf + readcount, tmpbuf, len)) {
				up(&chan->ctl.read_sem);
				return -EFAULT;
			}
			readcount += len;
		} else if (file->f_flags & O_NONBLOCK || readcount)
			break;
		else if (wait_event_interruptible(chan->ctl.rx_queue,
						  RX_FIFO_HAS_DATA)) {
			up(&chan->ctl.read_sem);
			return -ERESTARTSYS;
		}
	}

	up(&chan->ctl.read_sem);
	return readcount;
}

#define TX_FIFO_HAS_ROOM (__kfifo_len(chan->ctl.tx_fifo) < CTL_TX_FIFO_SIZE)

/* SAM support NONBLOCK? */
/* SAM locking! */
static ssize_t ctl_write(struct file *file, const char *buf,
			 size_t size, loff_t *offset)
{
	u8 i, avail, data;
	int wrote = size;
	struct gsm_chan *chan = file->private_data;

	gsm_uart_read(chan, IS752_TXLVL, &avail);

	while (1) {
		if (avail > size)
			avail = size;
		size -= avail;

		for (i = 0; i < avail; ++i, ++buf) {
			if (get_user(data, buf))
				return -EFAULT;
			gsm_uart_write(chan, IS752_THR, data);
		}

		if (size <= 0)
			return wrote;

		chan->ctl.tx_empty = 0;
		gsm_uart_read(chan, IS752_TXLVL, &avail);

		while (avail == 0) {
			if (wait_event_interruptible(chan->ctl.tx_queue,
						     chan->ctl.tx_empty))
				return -ERESTARTSYS;
			gsm_uart_read(chan, IS752_TXLVL, &avail);
		}
	}

	return wrote;
}

static int gsm_ioctl(struct inode *inode, struct file *file,
		     unsigned int cmd, unsigned long arg)
{
	struct gsm_chan *chan = file->private_data;
	struct gsm_req kgsm_req;

// SAM	if (!CHAN_ON(chan))
// SAM		return -EAGAIN;
	if (_IOC_TYPE(cmd) != GSM_IOC_MAGIC)
		return -ENOTTY;
	if (_IOC_NR(cmd) > GSM_IOC_MAXNR) {
		printk("INVALID IOCTL NUMBER\n"); // SAM DBG
		return -ENOTTY;
	}

	switch (cmd) {
	case GSM_SETLED:
	case GSM_GETLED:
		return 0;
	case GSM_SETSTATE:
		if (arg == STATE_CONNECTED) {
			printk("ioctl SETSTATE IOCTL STATE_CONNECTED\n"); // SAM DBG
#if 0
			if (CHAN_CONNECTED(chan)) {
				LOG_CHAN(chan, "ioctl: double STATE_CONNECTED request.\n");
				return 0;
			}
			if (CHAN_HANGUP(chan)) {
				DEBUG_CHAN(chan, "ioctl: got STATE_CONNECTED request while channel is not ready.\n");
				while (CHAN_HANGUP(chan))
					msleep(50);
			}
			DEBUG_CHAN(chan, "ioctl: set STATE_CONNECTED.\n");
			atomic_set(&chan->call_state, CONNECTED);
			up(&chan->port.read_sem);
			up(&chan->port.write_sem);
#endif
			return 0;
		} else if (arg == STATE_HANGUP) {
		printk("ioctl SETSTATE IOCTL STATE_HANGUP\n"); // SAM DBG
#if 0
			if (!CHAN_CONNECTED(chan)) {
				LOG_CHAN(chan, "ioctl: got STATE_HANGUP request while channel is not connected.\n");
				return 0;
			}
			DEBUG_CHAN(chan, "ioctl: set STATE_HANGUP.\n");
			down(&chan->port.read_sem);
			down(&chan->port.write_sem);
			atomic_set(&chan->call_state, STATE_HANGUP);
#endif
			return 0;
		} else
			printk("ioctl SETSTATE IOCTL %ld\n", arg); // SAM DBG

		break;
	case GSM_SETNEXTSIMSLOT:
		printk("ioctl GSM_SETNEXTSIMSLOT %ld\n", arg); // SAM DBG
#if 0
		if (arg == 0 || arg == 1) {
			if (atomic_read(&chan->sim_slot_curr) == arg) {
				LOG_CHAN(chan, "ioctl: next sim slot is %d which is curr already, so not setting it.\n", arg?1:0);
				return 0;
			} else {
				LOG_CHAN(chan, "ioctl: next sim slot is %d.\n", arg?1:0);
				atomic_set(&chan->sim_slot_next, arg);
				return 1;
			}
		}
#endif
		break;
	case GSM_KILLME:
	{
		char buf[] = "KILLME\r\n";
		LOG_CHAN(chan, "ioctl: KILLME.\n");
		__kfifo_put(chan->ctl.rx_fifo, (unsigned char *)buf, (sizeof(buf)-1));
		wake_up(&chan->ctl.rx_queue);
	}
		break;
	
	case GSM_POKE_FPGA:
	{
		if (copy_from_user((void*)&kgsm_req, (struct gsm_req*)arg, sizeof(struct gsm_req)))
			return -EFAULT;

		if ((kgsm_req.param1 < 0) || (kgsm_req.param1 > MAX_FPGA_RANGE))	
			return -EINVAL;

		out_be32((void*)((unsigned char*)gsm_card->fpga + kgsm_req.param1), kgsm_req.param2);

		return 0;
	}
		break;

	case GSM_PEEK_FPGA:
	{
		if (copy_from_user((void*)&kgsm_req, (struct gsm_req*)arg, sizeof(struct gsm_req)))
			return -EFAULT;

		if ((kgsm_req.param1 < 0) || (kgsm_req.param1 > MAX_FPGA_RANGE))	
			return -EINVAL;

		kgsm_req.param2 = in_be32((void*)((unsigned)gsm_card->fpga + kgsm_req.param1));
		
		if (copy_to_user((void*)arg, (void*)&kgsm_req, sizeof(struct gsm_req)))
			return -EFAULT;

		return 0;
	}
		break;

	case GSM_POWERCYCLE_RADIO:
	{
		int chan_id = chan->id;
		unsigned network_mask = 0;
		int result = 0;

		switch (chan_id) {
			case 2: /* 1A */
				network_mask = 0x10000000; 
				break;
			case 3: /* 1B */
				network_mask = 0x40000000;
				break;
			case 6: /* 2A */
				network_mask = 0x20000000;
				break;
			case 7: /* 2B */
				network_mask = 0x80000000;
				break;
			default:
				break;
		}

		if (network_mask)
			result = gsm_power_cycle(gsm_card, network_mask);
			
		printk(KERN_CRIT "%s() : chan_id = %d\n", __FUNCTION__, chan_id);

		return result;
	}
		break;

	case GSM_SET_RX_GAIN:
		gsm_set_rx_codec_gain(chan, (unsigned char)arg);
		break;

	case GSM_SET_TX_GAIN:
		gsm_set_tx_codec_gain(chan, (unsigned char)arg);
		break;

	default:
		printk("Unknown IOCTL %x type %c NR %d DIR %d SIZE %d\n",
		       cmd, _IOC_TYPE(cmd), _IOC_NR(cmd),
		       _IOC_DIR(cmd), _IOC_SIZE(cmd)); // SAM DBG
	}

	return -ENOTTY;

}

static int port_open(struct inode *inode, struct file *file)
{
	struct gsm_port *port =
		container_of(inode->i_cdev, struct gsm_port, cdev);
	struct gsm_chan *chan = container_of(port, struct gsm_chan, port);

	if (atomic_inc_and_test(&port->in_use)) {
		file->private_data = chan;
		DEBUG_PORT(chan, "open %s: %s %s (%x)\n",
			   port->name,
			   file->f_flags & O_WRONLY ? "w" :
			   file->f_flags & O_RDWR ? "rw" : "r",
			   file->f_flags & O_NONBLOCK ? "non blocking" :
			   "blocking", file->f_flags);
		__kfifo_reset(chan->port.rx_fifo);
		__kfifo_reset(chan->port.tx_fifo);
		return 0;
	} else {
		atomic_dec(&port->in_use);
		DEBUG_PORT(chan, "open: multiple open attempt rejected\n");
		return -EBUSY;
	}
}

static int port_release(struct inode *inode, struct file *file)
{
	struct gsm_chan *chan = file->private_data;

	DEBUG_PORT(chan, "release\n");
	chan_off(chan);
	atomic_dec(&chan->port.in_use);

	return 0;
}

static unsigned int port_poll(struct file *file, struct poll_table_struct *wait)
{
	struct gsm_chan *chan = file->private_data;
	unsigned int mask = 0;

	poll_wait(file, &chan->port.rx_queue, wait);
	poll_wait(file, &chan->port.tx_queue, wait);
	if (!down_trylock(&chan->port.read_sem)) {
		if (__kfifo_len(chan->port.rx_fifo) >= PORT_RX_THRESHOLD)
			mask |= POLLIN | POLLRDNORM;
		up(&chan->port.read_sem);
	}
	if (!down_trylock(&chan->port.write_sem)) {
		if (__kfifo_len(chan->port.tx_fifo) < PORT_TX_FIFO_SIZE)
			mask |= POLLOUT | POLLWRNORM;
		up(&chan->port.write_sem);
	}

	return mask;
}

static inline int block_on(struct gsm_port *port)
{
	return wait_event_interruptible(
		port->rx_queue,
		(__kfifo_len(port->rx_fifo) >= PORT_RX_THRESHOLD));
}

/* port character device operations */
static ssize_t port_read(struct file *file, char __user *buf,
			 size_t count, loff_t *f_pos)
{
	struct gsm_chan *chan = file->private_data;
	unsigned char tmpbuf[CHUNKSIZE];
	int len, chunklen, readcount = 0;

	if (!count)
		return 0;
	if (count < 0)
		return -EINVAL;

	if (file->f_flags & O_NONBLOCK) {
		if (down_trylock(&chan->port.read_sem))
			return -EAGAIN;
	} else if (down_interruptible(&chan->port.read_sem))
		return -ERESTARTSYS;

	while (readcount < count) {
		chunklen = count - readcount;
		if (chunklen > sizeof(tmpbuf))
			chunklen = sizeof(tmpbuf);

		len = __kfifo_get(chan->port.rx_fifo, tmpbuf, chunklen);
		if (len > 0) {
			if (copy_to_user(buf + readcount, tmpbuf, len)) {
				up(&chan->port.read_sem);
				return -EFAULT;
			}
			readcount += len;
		} else if (file->f_flags & O_NONBLOCK || readcount >= PORT_RX_THRESHOLD)
			break;
		else if (block_on(&chan->port)) {
			up(&chan->port.read_sem);
			return -ERESTARTSYS;
		}
	}

	up(&chan->port.read_sem);
	return readcount;
}

static ssize_t port_write(struct file *file, const char __user *buf,
			  size_t count, loff_t *f_pos)
{
	struct gsm_chan *chan = file->private_data;
	unsigned char tmpbuf[CHUNKSIZE];
	int len, chunklen, writecount = 0;

	if (!count)
		return 0;
	if (count < 0)
		return -EINVAL;

	if (file->f_flags & O_NONBLOCK) {
		if (down_trylock(&chan->port.write_sem))
			return -EAGAIN;
	} else {
		if (down_interruptible(&chan->port.write_sem))
			return -ERESTARTSYS;
	}

	while (writecount < count) {
		chunklen = count - writecount;
		if (chunklen > sizeof(tmpbuf))
			chunklen = sizeof(tmpbuf);
		if (copy_from_user(tmpbuf, buf + writecount, chunklen)) {
			up(&chan->port.write_sem);
			return -EFAULT;
		}
		len = __kfifo_put(chan->port.tx_fifo, tmpbuf, chunklen);
		if (len > 0)
			writecount += len;
		else if (file->f_flags & O_NONBLOCK /* SAM || writecount */)
			break;
		else {
			if (wait_event_interruptible(chan->port.tx_queue,
						  __kfifo_len(chan->port.tx_fifo) < PORT_TX_FIFO_SIZE - CHUNKSIZE)) {
			up(&chan->port.write_sem);
			return -ERESTARTSYS;
			}
		}
	}

	up(&chan->port.write_sem);
	return writecount;
}

/* Performs mapping between the GSM modules and the external SIM slots */
/* Radio 1 --> External SIM Bank 1 on Module (A|B) */
/* Radio 2 --> External SIM Bank 2 on Module (A|B) */ 
static void gsm_configure_sim_switches(struct gsm_card *gsm)
{
	unsigned mod_a_reset_mask = ( 1 << 16 ) | ( 1 << 18 );
	unsigned mod_a_set_mask =  ( 1 << 17 ) | ( 1 << 19 );
	unsigned mod_b_reset_mask = ( 1 << 20 ) | ( 1 << 22 );
	unsigned mod_b_set_mask =  ( 1 << 21 ) | ( 1 << 23 );
	unsigned config;	

	config = gsm_fpga_read(gsm, FPGA_CONFIG);

	/* do for module a */	
	config &= ~mod_a_reset_mask;
	config |= mod_a_set_mask;

	/* do for module b */
	config &= ~mod_b_reset_mask;
	config |= mod_b_set_mask;

	gsm_fpga_write(gsm, FPGA_CONFIG, config);
}

/* --- Dave Code ;) --- */

static int gsm_power_up_down(struct gsm_card *gsm, unsigned char power_up_down_mask)
{
	unsigned config;
	int ms_timer, network_status_lines = 0;

	printk(KERN_DEBUG "entering %s() power_up_down_mask 0x%02x\n", __FUNCTION__, power_up_down_mask);

	/* assert PWRKEY signals low for at least 2 seconds for all those that need it */
	config = gsm_fpga_read(gsm, FPGA_CONFIG);
	gsm_fpga_write(gsm, FPGA_CONFIG, config & ~(power_up_down_mask << 8));
	delay(3000);
	gsm_fpga_write(gsm, FPGA_CONFIG, config | (power_up_down_mask << 8));

	/* This test of the network status bits also serves as a delay
	 * before accessing the radios (delay not documented, trial
	 * and error determined 4s to be enough)
	 *
	 * worst case duty cycle for network status line is 64ms on /
	 * 3000ms off
	 */
	for (ms_timer = 0; ms_timer < 4000; ms_timer++) {
		network_status_lines |= gsm_fpga_read(gsm, FPGA_CONFIG);
		delay(1);
	}
	
	printk(KERN_DEBUG "leaving %s() network_status_lines = 0x%08x\n",
		__FUNCTION__, network_status_lines);

	return network_status_lines & 0xf0000000;
}

static int gsm_power_cycle(struct gsm_card *gsm, unsigned expected)
{
	/* powers up GSM radio and that in turn powers the UART */
	unsigned char power_up_down_mask = 0;

	/* check the expected mask instead of the status mask as we know
	 * the state of radios when we call this function.
	 */
   
	/* check for radio 1A */
	if  (expected & GSM_1A_STAT_MASK) {
		power_up_down_mask |= GSM_1A_PWR_MASK;
	}

	/* check for radio 2A */
	if  (expected & GSM_2A_STAT_MASK) {
		power_up_down_mask |= GSM_2A_PWR_MASK;
	}

	/* check for radio 1B */
	if  (expected & GSM_1B_STAT_MASK) {
		power_up_down_mask |= GSM_1B_PWR_MASK;
	}

	/* check for radio 2B */
	if  (expected & GSM_2B_STAT_MASK) {
		power_up_down_mask |= GSM_2B_PWR_MASK;
	}

	printk(KERN_DEBUG "%s(): expected = 0x%08x\n", __FUNCTION__, expected);

	/* power down the radio, wait for 8 seconds and power up again */
	gsm_power_up_down(gsm, power_up_down_mask);
	printk(KERN_CRIT "Waiting 8 seconds to log out from the network\n");
	delay(8000);
	/* perform sim switching configuration before power up */
	gsm_configure_sim_switches(gsm);

	return gsm_power_up_down(gsm, power_up_down_mask);
}

static int gsm_power_up(struct gsm_card *gsm, unsigned expected)
{	/* powers up GSM radio and that in turn powers the UART */
	unsigned status = 0;
	int ms_timer;
	int gsm_1a_up = 0;
	int gsm_1b_up = 0;
	int gsm_2a_up = 0;
	int gsm_2b_up = 0;
	unsigned char power_up_down_mask = 0;

	/* We do not know the status of the radios, so check if they
	 * are already up by sampling all status lines for 4000 milliseconds . */
	for (ms_timer = 0; ms_timer < 4000; ms_timer++) {
		status = gsm_fpga_read(gsm, FPGA_CONFIG);
		if (status & GSM_1A_STAT_MASK) gsm_1a_up = 1;
		if (status & GSM_1B_STAT_MASK) gsm_1b_up = 1;
		if (status & GSM_2A_STAT_MASK) gsm_2a_up = 1;
		if (status & GSM_2B_STAT_MASK) gsm_2b_up = 1;
		delay(1);
	}

	/* we could do all these in a big for loop */

	/* check for radio 1A */
	if  (expected & GSM_1A_STAT_MASK) {
		power_up_down_mask |= GSM_1A_PWR_MASK;
	}

	/* check for radio 2A */
	if  (expected & GSM_2A_STAT_MASK) {
		power_up_down_mask |= GSM_2A_PWR_MASK;
	}

	/* check for radio 1B */
	if  (expected & GSM_1B_STAT_MASK) {
		power_up_down_mask |= GSM_1B_PWR_MASK;
	}

	/* check for radio 2B */
	if  (expected & GSM_2B_STAT_MASK) {
		power_up_down_mask |= GSM_2B_PWR_MASK;
	}

	/* put all GSM status bits in place */
	status = (gsm_1a_up ? GSM_1A_STAT_MASK : 0) | \
		 (gsm_1b_up ? GSM_1B_STAT_MASK : 0) | \
		 (gsm_2a_up ? GSM_2A_STAT_MASK : 0) | \
		 (gsm_2b_up ? GSM_2B_STAT_MASK : 0);

	if ((status & expected) == expected) {
		printk(KERN_DEBUG "%s() : status and expected are in sync status = 0x%08x expected = 0x%08x\n",
			__FUNCTION__, status, expected);
		/* power down */
		gsm_power_up_down(gsm, power_up_down_mask);
		printk(KERN_CRIT "Waiting 8 seconds to log out from the network\n");
		delay(8000);
		/* power up */

		/* before powering up radios again, perform sim switching configuration */
		gsm_configure_sim_switches(gsm);
		return gsm_power_up_down(gsm, power_up_down_mask);
	}

	printk(KERN_DEBUG "%s() : calling gsm_power_up_down with status 0x%08x expected 0x%08x power_up_down_mask 0x%02x\n", 
		__FUNCTION__, status, expected, power_up_down_mask);

	/* before powering up radios, perform sim switching configuration */
	gsm_configure_sim_switches(gsm);
	return gsm_power_up_down(gsm, power_up_down_mask);
}

static int gsm_power_down(struct gsm_card *gsm)
{
	return gsm_power_up_down(gsm, 0xF0);
}

static int __init gsm_verify_spi(struct gsm_card *gsm, int module)
{	/* Only one channel is necessary since both channels share the
	 * same interface. */
	struct gsm_chan *chan = gsm->chans[module == 0 ? 2 : 6];
	int failed = 0;
	u8 reg;

	/* Verify default value of CODEC register. We cannot trust the
	 * default since we can (and do!) change it. This value is
	 * only cleared on a power cycle. */
	gsm_codec_write(chan, 0, 6);
	if (gsm_codec_read(chan, 0, &reg) || reg != 6) {
		printk(KERN_ERR "%d: codec read failed %x != 6\n",
		       chan->id, reg);
		++failed;
	}

	/* Verify UART register access */
	gsm_uart_write(chan, IS752_LCR, 3);
	gsm_uart_read(chan, IS752_LCR, &reg);
	if (reg != 3) {
		printk(KERN_ERR "%d: uart access failed %x != 3\n",
		       chan->id, reg);
		++failed;
	}

	return failed ? -ENODEV : 0;
}

static void __init gsm_configure_uart(struct gsm_chan *chan)
{
	u8 reg;

	/* enable programming of clock divisor */
	gsm_uart_write(chan, IS752_LCR, 0x80);
	/* divide by 2 for xtal= 3.6864MHz */
	gsm_uart_write(chan, IS752_DLL, 0x02);
	gsm_uart_write(chan, IS752_DLH, 0x00);
	/* access EFR register */
	gsm_uart_write(chan, IS752_LCR, 0xBF);
	/* enable enhanced features registers */
	gsm_uart_write(chan, IS752_EFR, 0x10);
	/* general register access */
	gsm_uart_write(chan, IS752_LCR, 0x03);
	/* enable TCR and TLR registers */
	gsm_uart_write(chan, IS752_MCR, 0x04);
	/* halt reception at 32 characters,
	 * resume at 8 characters */
	gsm_uart_write(chan, IS752_TCR, 0x28);
	/* access EFR register */
	gsm_uart_write(chan, IS752_LCR, 0xBF);
	/* enable enhanced registers and hardware flow control */
	gsm_uart_write(chan, IS752_EFR, 0xD0);
	/* 8 bit data, 1 stop bit, no parity */
	gsm_uart_write(chan, IS752_LCR, 0x03);
	/* reset TXFIFO, reset RXFIFO, non FIFO mode */
	gsm_uart_write(chan, IS752_FCR, 0x06);
	/* set FIFO mode */
	gsm_uart_write(chan, IS752_FCR, 0x01);
	/* disable all interrupts */
	gsm_uart_write(chan, IS752_IER, 0x00);
	/* set GPIO pins as modem pins */
	gsm_uart_read(chan, IS752_IOControl, &reg);
	gsm_uart_write(chan, IS752_IOControl, reg | 6);
	/* enable TCR and TLR registers and set /DTR low */
	gsm_uart_write(chan, IS752_MCR, 0x05);

	/* enable interrupts */
	gsm_uart_write(chan, IS752_IER, 7);
}

static void gsm_set_rx_codec_gain(struct gsm_chan *chan, unsigned char value)
{

	if (!chan) {
		printk(KERN_ERR "%s() : invalid chan struct pointer\n", __FUNCTION__);
		return;
	}

	switch (chan->id) {

		case 2:
		case 6:
			gsm_codec_write(chan, AUDIO_CH0_RX_GAIN, value);
			break;

		case 3:
		case 7:
			gsm_codec_write(chan, AUDIO_CH1_RX_GAIN, value);
			break;

		default:
			/* invalid channel number */	
			break;
	}
}

static void gsm_set_tx_codec_gain(struct gsm_chan *chan, unsigned char value)
{

	if (!chan) {
		printk(KERN_ERR "%s() : invalid chan struct pointer\n", __FUNCTION__);
		return;
	}

	switch (chan->id) {

		case 2:
		case 6:
			gsm_codec_write(chan, AUDIO_CH0_TX_GAIN, value);
			break;

		case 3:
		case 7:
			gsm_codec_write(chan, AUDIO_CH1_TX_GAIN, value);
			break;

		default:
			/* invalid channel number */	
			break;
	}
}

static void __init gsm_configure_codec(struct gsm_card *gsm, int module)
{
	unsigned config;
	struct gsm_chan *chan = gsm->chans[module == 0 ? 2 : 6];

	/* 6 = 0dB, each number up removes 1dB */
	gsm_codec_write(chan, AUDIO_CH0_RX_GAIN, 0x18);
	gsm_codec_write(chan, AUDIO_CH1_RX_GAIN, 0x18);
	gsm_codec_write(chan, AUDIO_CH0_TX_GAIN, 0x06);
	gsm_codec_write(chan, AUDIO_CH1_TX_GAIN, 0x06);

	gsm_codec_write(chan, AUDIO_POWER_DOWN,  0x04);

#if 1
	gsm_codec_write(chan, AUDIO_PCM_CONTROL, 0x04); /* mulaw */
#else
	gsm_codec_write(chan, AUDIO_PCM_CONTROL, 0x00); /* alaw */
#endif

	/* Turn on PCM clocks to the GSM codecs */
	config = gsm_fpga_read(gsm, FPGA_CONFIG);
	config |= (0x80 << module) | 1;
	gsm_fpga_write(gsm, FPGA_CONFIG, config);

	delay(130);
}

/* --- EODC --- */

const struct file_operations ctl_ops = {
	.owner		= THIS_MODULE,
	.open		= ctl_open,
	.release	= ctl_release,
	.poll		= ctl_poll,
	.read		= ctl_read,
	.write		= ctl_write,
	.ioctl		= gsm_ioctl,
};

const struct file_operations port_ops = {
	.owner		= THIS_MODULE,
	.open		= port_open,
	.release	= port_release,
	.poll		= port_poll,
	.read		= port_read,
	.write		= port_write,
	.ioctl		= gsm_ioctl,
};

/* sysfs gsm class attributes */
static ssize_t attr_class_show_num_cards(struct class *cl, char *buf)
{
	return sprintf(buf, "1\n");
}

static ssize_t attr_class_show_num_chans(struct class *cl, char *buf)
{
	return sprintf(buf, "[gsm_channels]\nchans=%d\n", num_chans);
}

static struct class_attribute class_attributes[] = {
	__ATTR(num_cards, 0444, attr_class_show_num_cards, NULL),
	__ATTR(num_chans, 0444, attr_class_show_num_chans, NULL),
};
#define N_CLASS_ATTR (sizeof(class_attributes) / sizeof(struct class_attribute))

/* sysfs card device attributes */
static ssize_t attr_card_show_frame_count(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return sprintf(buf, "0\n"); /* SAM HACK */
}

static ssize_t attr_card_show_num_chans(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d\n", num_chans);
}

static ssize_t attr_card_show_version(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct gsm_card *card = dev_get_drvdata(dev);
	return sprintf(buf, "%x\n", card->rev & 0xffff);
}

static struct device_attribute card_attributes[] = {
	__ATTR(frame_count, 0444, attr_card_show_frame_count, NULL),
	__ATTR(num_chans, 0444, attr_card_show_num_chans, NULL),
	__ATTR(version, 0444, attr_card_show_version, NULL),
};
#define N_CARD_ATTR (sizeof(card_attributes) / sizeof(struct device_attribute))

/* sysfs channel attributes, available for both port and ctl */
static ssize_t
attr_chan_show_channel_use_count(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct gsm_chan *chan = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", atomic_read(&chan->use_count));
}

static ssize_t attr_chan_show_channel_sim_slot(struct device *dev,
					       struct device_attribute *attr,
					       char *buf)
{
	struct gsm_chan *chan = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", atomic_read(&chan->sim_slot_curr));
}

static ssize_t
attr_chan_show_channel_sim_slot_next(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct gsm_chan *chan = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", atomic_read(&chan->sim_slot_next));
}

static ssize_t
attr_chan_store_channel_sim_slot_next(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct gsm_chan *chan = dev_get_drvdata(dev);
	int value;

	if (sscanf(buf, "%d", &value) == 1 && (value == 0 || value == 1)) {
		atomic_set(&chan->sim_slot_next, value);
		return count;
	}

	return -ENOTTY;
}

/* sysfs ctl device attributes */
static ssize_t attr_ctl_show_use_count(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	struct gsm_chan *chan = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", chan->ctl.use_count);
}

#ifdef _DEBUG_FIFO
static ssize_t attr_ctl_show_fifo_fails(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct gsm_chan *chan = dev_get_drvdata(dev);
	return sprintf(buf, "rx_lost: %d\nrx_overrun: %d\n",
				   atomic_read(&chan->ctl.rx_lost),
				   atomic_read(&chan->ctl.rx_overrun));
}
#endif

static struct device_attribute ctl_attributes[] = {
	__ATTR(use_count, 0444, attr_ctl_show_use_count, NULL),
#ifdef _DEBUG_FIFO
	__ATTR(fifo_fails, 0444, attr_ctl_show_fifo_fails, NULL),
#endif
	__ATTR(channel_use_count, 0444, attr_chan_show_channel_use_count, NULL),
	__ATTR(channel_sim_slot, 0444, attr_chan_show_channel_sim_slot, NULL),
	__ATTR(channel_sim_slot_next, 0664,
	       attr_chan_show_channel_sim_slot_next,
	       attr_chan_store_channel_sim_slot_next),
};
#define N_CTL_ATTR (sizeof(ctl_attributes) / sizeof(struct device_attribute))

/* sysfs port device attributes */
static ssize_t attr_port_show_in_use(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct gsm_chan *chan = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", (atomic_read(&chan->port.in_use) == 0));
}

#ifdef _DEBUG_FIFO
static ssize_t attr_port_show_fifo_fails(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct gsm_chan *chan = dev_get_drvdata(dev);
	return sprintf(buf, "rx_lost: %d\ntx_underrun: %d\n",
				   atomic_read(&chan->port.rx_lost),
				   atomic_read(&chan->port.tx_underrun));
}
#endif

static struct device_attribute port_attributes[] = {
	__ATTR(in_use, 0444, attr_port_show_in_use, NULL),
#ifdef _DEBUG_FIFO
	__ATTR(fifo_fails, 0444, attr_port_show_fifo_fails, NULL),
#endif
	__ATTR(channel_use_count, 0444, attr_chan_show_channel_use_count, NULL),
	__ATTR(channel_sim_slot, 0444, attr_chan_show_channel_sim_slot, NULL),
	__ATTR(channel_sim_slot_next, 0664,
	       attr_chan_show_channel_sim_slot_next,
	       attr_chan_store_channel_sim_slot_next),
};
#define N_PORT_ATTR (sizeof(port_attributes) / sizeof(struct device_attribute))

/* We don't need to cleanup here: gsm_destroy_board will */
static int __init gsm_create_chan(struct gsm_card *gsm, int id)
{
	static int portno = 1;
	int rc, i;
	struct gsm_chan *chan = kzalloc(sizeof(struct gsm_chan), GFP_KERNEL);
	if (!chan)
		return -ENOMEM;

	chan->id = id;
	chan->card = gsm;
	chan->fpga = gsm->fpga;
	chan->timeslot = id * 2 - (chan->id & 1);

	/* SAM bnxgsm */
	snprintf(chan->ctl.name, sizeof(chan->ctl.name), "ctl%d", portno);
	snprintf(chan->port.name, sizeof(chan->port.name), "port%d", portno);
	atomic_set(&chan->sim_slot_curr, 0);
	atomic_set(&chan->sim_slot_next, 0);
	atomic_set(&chan->port.in_use, -1);
	atomic_set(&chan->use_count, 0);
// SAM	atomic_set(&chan->on_off_state, SW_OFF);
// SAM	atomic_set(&chan->req_on, 0);
// SAM	atomic_set(&chan->call_state, STANDBY);
	init_waitqueue_head(&chan->ctl.rx_queue);
	init_waitqueue_head(&chan->ctl.tx_queue);
	init_waitqueue_head(&chan->port.rx_queue);
	init_waitqueue_head(&chan->port.tx_queue);
	init_MUTEX(&chan->ctl.use_sem);
	init_MUTEX_LOCKED(&chan->ctl.write_sem);
	init_MUTEX(&chan->ctl.read_sem);
// SAM	init_MUTEX_LOCKED(&chan->port.read_sem);
	init_MUTEX(&chan->port.read_sem);
// SAM	init_MUTEX_LOCKED(&chan->port.write_sem);
	init_MUTEX(&chan->port.write_sem);

	chan->ctl.rx_fifo = kfifo_alloc(CTL_RX_FIFO_SIZE, GFP_KERNEL, NULL);
	if (IS_ERR(chan->ctl.rx_fifo)) {
		ERR_CTL(chan, "Could not allocate fifo ctl.rx_fifo\n");
		chan->ctl.rx_fifo = NULL;
		return -ENOMEM;
	}
	chan->ctl.tx_fifo = kfifo_alloc(CTL_TX_FIFO_SIZE, GFP_KERNEL, NULL);
	if (IS_ERR(chan->ctl.tx_fifo)) {
		ERR_CTL(chan, "Could not allocate ctl.tx_fifo\n");
		chan->ctl.tx_fifo = NULL;
		return -ENOMEM;
	}
	chan->port.rx_fifo = kfifo_alloc(PORT_RX_FIFO_SIZE, GFP_KERNEL, NULL);
	if (IS_ERR(chan->port.rx_fifo)) {
		ERR_PORT(chan, "Could not allocate port.rx_fifo\n");
		chan->port.rx_fifo = NULL;
		return -ENOMEM;
	}
	chan->port.tx_fifo = kfifo_alloc(PORT_TX_FIFO_SIZE, GFP_KERNEL, NULL);
	if (IS_ERR(chan->port.tx_fifo)) {
		ERR_PORT(chan, "Could not allocate port.tx_fifo\n");
		chan->port.tx_fifo = NULL;
		return -ENOMEM;
	}

	chan->ctl.class_dev.devt = MKDEV(MAJOR(dev_first), 2 * chan->id - 1);
	cdev_init(&chan->ctl.cdev, &ctl_ops);
	rc = cdev_add(&chan->ctl.cdev, chan->ctl.class_dev.devt, 1);
	if (rc)
		return rc;

	chan->port.class_dev.devt = MKDEV(MAJOR(dev_first), 2 * chan->id);
	cdev_init(&chan->port.cdev, &port_ops);
	rc = cdev_add(&chan->port.cdev, chan->port.class_dev.devt, 1);
	if (rc)
		return rc;

	chan->ctl.class_dev.class = &gsm_class;
	dev_set_drvdata(&chan->ctl.class_dev, chan);
	dev_set_name(&chan->ctl.class_dev, "ctl%d", portno);
	rc = device_register(&chan->ctl.class_dev);
	if (rc) {
		ERR_CTL(chan, "Could not register class device.\n");
		return rc;
	}

	for (i = 0; i < N_CTL_ATTR; ++i) {
		rc = device_create_file(&chan->ctl.class_dev,
					&ctl_attributes[i]);
		if (rc) {
			ERR_CTL(chan, "Could not create device attribute.\n");
			return rc;
		}
	}

	chan->port.class_dev.class = &gsm_class;
	dev_set_drvdata(&chan->port.class_dev, chan);
	dev_set_name(&chan->port.class_dev, "port%d", portno);
	rc = device_register(&chan->port.class_dev);
	if (rc) {
		ERR_PORT(chan, "Could not register class device.\n");
		return rc;
	}

	for (i = 0; i < N_PORT_ATTR; ++i) {
		rc = device_create_file(&chan->port.class_dev,
					&port_attributes[i]);
		if (rc) {
			ERR_PORT(chan, "Could not create device attribute.\n");
			return rc;
		}
	}

	if (sysfs_create_link(&chan->card->class_dev.kobj,
			      &chan->ctl.class_dev.kobj, chan->ctl.name) ||
	    sysfs_create_link(&chan->card->class_dev.kobj,
			      &chan->port.class_dev.kobj, chan->port.name) ||
	    sysfs_create_link(&chan->ctl.class_dev.kobj,
			      &chan->card->class_dev.kobj, chan->card->name) ||
	    sysfs_create_link(&chan->port.class_dev.kobj,
			      &chan->card->class_dev.kobj, chan->card->name)) {
		ERR_CHAN(chan, "Could not create symbolic link in sysfs.\n");
		return rc;
	}

	/* SAM bnxgsm */

	gsm->chans[chan->id] = chan;
	++portno;

	return 0;
}

static int __init gsm_create_chans(struct gsm_card *gsm, int module)
{
	int i;
	int id = module == 0 ?  2 : 6;

	for (i = 0; i < GSM_N_CHANS; ++i, ++id)
		if (gsm_create_chan(gsm, id))
			return -ENOMEM;

	return 0;
}

static int __init gsm_create_module(struct gsm_card *gsm, int module)
{
	int i, err;
	int chan = module == 0 ? 2 : 6;

	/* Must create the channels first */
	err = gsm_create_chans(gsm, module);
	if (err)
		return err;

	err = gsm_verify_spi(gsm, module);
	if (err)
		return err;

	for (i = 0; i < GSM_N_CHANS; ++i, ++chan)
		gsm_configure_uart(gsm->chans[chan]);

	gsm_configure_codec(gsm, module);

	return 0;
}

static void gsm_destroy_board(struct gsm_card *gsm)
{
	int i;

	for (i = 0; i < GSM_MAX_CHANS; ++i)
		if (gsm->chans[i]) {
			struct gsm_chan *chan = gsm->chans[i];
			gsm->chans[i] = NULL;

			if (chan->port.tx_fifo)
				kfifo_free(chan->port.tx_fifo);
			if (chan->port.rx_fifo)
				kfifo_free(chan->port.rx_fifo);
			if (chan->ctl.tx_fifo)
				kfifo_free(chan->ctl.tx_fifo);
			if (chan->ctl.rx_fifo)
				kfifo_free(chan->ctl.rx_fifo);

			device_unregister(&chan->ctl.class_dev);
			device_unregister(&chan->port.class_dev);

			kfree(chan);
		}

	if (gsm->fpga)
		iounmap(gsm->fpga);

	device_unregister(&gsm->class_dev);

	kfree(gsm);
}

/* This is required even if it does nothing */
static void gsm_device_release (struct device *dev) {}

static __init int setup_sysfs(struct gsm_card *card)
{
	int rc, i;

	memset(&gsm_class, 0, sizeof(gsm_class));
	gsm_class.name = gsm_class_name;
	gsm_class.owner = THIS_MODULE;
	gsm_class.dev_release = gsm_device_release;

	rc = class_register(&gsm_class);
	if (rc)
		return rc;

	for (i = 0; i < N_CLASS_ATTR; ++i) {
		rc = class_create_file(&gsm_class, &class_attributes[i]);
		if (rc) {
			ERR("Could not create class attribute\n");
			goto err;
		}
	}

	card->class_dev.class = &gsm_class;
	dev_set_drvdata(&card->class_dev, card);
	dev_set_name(&card->class_dev, "%s", card->name);
	rc = device_register(&card->class_dev);
	if (rc) {
		ERR_CARD(card, "Could not register class device.\n");
		goto err;
	}

	for (i = 0; i < N_CARD_ATTR; ++i) {
		rc = device_create_file(&card->class_dev, &card_attributes[i]);
		if (rc) {
			ERR_CARD(card, "Could not create device attribute.\n");
			goto err;
		}
	}

	return 0;

err:
	class_unregister(&gsm_class);
	return rc;
}

static void release_sysfs(void)
{
	class_unregister(&gsm_class);
}

static __init int setup_char_devices(void)
{
	int re;

	/* we only ever have max 4 *2 devices */
	re = alloc_chrdev_region(&dev_first, 1, 8, "pika-gsm");
	if (re)
		return re;

	DEBUG("major device number: %d, minor range: %d-%d\n", MAJOR(dev_first),
		  MINOR(dev_first), MINOR(dev_first) + 8 - 1);

	return 0;
}

static void release_char_devices(void)
{
	unregister_chrdev_region(dev_first, 8);
}

void __init gsm_enable_ints(struct gsm_card *gsm)
{
	unsigned imr = gsm_fpga_read(gsm, FPGA_IMR);
	imr |= gsm_int_mask;
	imr |= 0xc0000; /* DMA */
	gsm_fpga_write(gsm, FPGA_IMR, imr);
}

void __exit gsm_disable_ints(struct gsm_card *gsm)
{
	unsigned imr = gsm_fpga_read(gsm, FPGA_IMR);
	imr &= ~gsm_int_mask;
	imr &= ~0xc0000; /* DMA */
	gsm_fpga_write(gsm, FPGA_IMR, imr);
}

#if 1 /* SAM for debugging */
/* Diag Register Bits */
#define FPGA_DIAG               0x0018
#define GSM_FXS_CONN_BITS	0x00000700
#define GSM0_TO_FXS		0x00000100
#define GSM1_TO_FXS		0x00000300
#define GSM2_TO_FXS		0x00000500
#define GSM3_TO_FXS		0x00000700
#define FPGA_PCM_LOOPBACK_BIT	2

static int __init gsm_to_fxs(struct gsm_card *card, int channel)
{
	unsigned diag = gsm_fpga_read(card, FPGA_DIAG);
	diag &= ~(GSM_FXS_CONN_BITS | FPGA_PCM_LOOPBACK_BIT);

	switch (channel) {
	case 0: /* disconnect */
		break;
	case 2:
		diag |= GSM0_TO_FXS;
		break;
	case 3:
		diag |= GSM1_TO_FXS;
		break;
	case 6:
		diag |= GSM2_TO_FXS;
		break;
	case 7:
		diag |= GSM3_TO_FXS;
		break;
	default:
		printk("Invalid channel %d\n", channel);
		return -EINVAL;
	}

	gsm_fpga_write(card, FPGA_DIAG, diag);

	/* Wait for PCM interfaces to settle after change */
	delay(300);

	return 0;
}
#endif

int __init gsm_init_module(void)
{
	int err = -EINVAL;
	struct device_node *np;
	unsigned rev;
	int network_status;
	int moda = 0, modb = 0;

	/* Create the private data */
	gsm_card = kzalloc(sizeof(struct gsm_card), GFP_KERNEL);
	if (!gsm_card) {
		printk(KERN_ERR __FILE__ " NO MEMORY\n");
		return -ENOMEM;
	}

	strcpy(gsm_card->name, "gsm0");

	np = of_find_compatible_node(NULL, NULL, "pika,fpga");
	if (!np) {
		printk(KERN_ERR __FILE__ ": Unable to find fpga\n");
		goto error_cleanup;
	}

	gsm_card->irq = irq_of_parse_and_map(np, 0);
	if (gsm_card->irq  == NO_IRQ) {
		printk(KERN_ERR __FILE__ ": irq_of_parse_and_map failed\n");
		goto error_cleanup;
	}

	gsm_card->fpga = of_iomap(np, 0);
	if (!gsm_card->fpga) {
		printk(KERN_ERR __FILE__ ": Unable to get FPGA address\n");
		err = -ENOMEM;
		goto error_cleanup;
	}

	of_node_put(np);
	np = NULL;

	rev = gsm_fpga_read(gsm_card, FPGA_REV);
	moda = (rev & 0x0f0000) == 0x040000;
	modb = (rev & 0xf00000) == 0x400000;
	if (!moda && !modb) {
		err = -ENODEV;
		goto error_cleanup;
	}
	gsm_card->rev = rev;

	if ((rev & 0xffff) < 0x2028) {
		printk(KERN_ERR "FPGA rev %x is too old. You need at least 2028\n",
		       rev & 0xffff);
		err = -EINVAL;
		goto error_cleanup;
	}

	if (use_fxs)
		gsm_to_fxs(gsm_card, use_fxs);

	if (setup_char_devices()) {
		err = -ENODEV;
		goto error_cleanup;
	}

	if (setup_sysfs(gsm_card)) {
		err = -ENODEV;
		goto error_cleanup;
	}

	printk(KERN_INFO "pika-gsm %x starting...\n", rev);


	network_status = 0;
	if (moda)
		network_status |= 0x30000000;
	if (modb)
		network_status |= 0xc0000000;

	network_status = gsm_power_up(gsm_card, network_status);

	if (moda) {
		if ((network_status & 0x30000000) != 0x30000000) {
			printk(KERN_ERR "Network A did not come up\n");
			goto error_cleanup;
		}
		gsm_int_mask |= 1 << 20;
		num_chans += 2;
		if (gsm_create_module(gsm_card, 0))
			goto error_cleanup;
	}
	if (modb) {
		if ((network_status & 0xc0000000) != 0xc0000000) {
			printk(KERN_ERR "Network B did not come up\n");
			goto error_cleanup;
		}
		gsm_int_mask |= 1 << 21;
		num_chans += 2;
		if (gsm_create_module(gsm_card, 1))
			goto error_cleanup;
	}

	err = request_irq(gsm_card->irq, gsm_isr, IRQF_SHARED,
			  "pikagsm", gsm_card);
	if (err) {
		printk(KERN_ERR __FILE__ ": Unable to request irq %d\n", err);
		goto error_cleanup;
	}

	/* Register DMA */
	pikadma_get_buffers((void *)&gsm_card->rx_buf, (void *)&gsm_card->tx_buf);
	pikadma_register_cb(99, gsm_dma_callback);

	gsm_enable_ints(gsm_card);

	printk("pika-gsm running.\n");

	return 0;

error_cleanup:
	if (np)
		of_node_put(np);

	if (moda || modb) {
		gsm_destroy_board(gsm_card);
		release_sysfs();
		release_char_devices();
	}

	return err;
}
module_init(gsm_init_module);

void __exit gsm_exit_module(void)
{
	printk("pika-gsm stopping.....\n");

	pikadma_unregister(99);

	gsm_disable_ints(gsm_card);

	gsm_power_down(gsm_card);

	free_irq(gsm_card->irq, gsm_card);

	gsm_destroy_board(gsm_card);

	release_sysfs();
	release_char_devices();

	printk("pika-gsm says \"Have a nice day\".\n");
}
module_exit(gsm_exit_module);

void gsm_module(void) {}
EXPORT_SYMBOL(gsm_module);

MODULE_DESCRIPTION("PIKA chan_gsm driver");
MODULE_AUTHOR("Sean MacLennan");
MODULE_LICENSE("GPL");
