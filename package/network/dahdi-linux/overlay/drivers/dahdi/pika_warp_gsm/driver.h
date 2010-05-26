#ifndef _DRIVER_H_
#define _DRIVER_H_

#include <linux/module.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

#include "gsm.h"
#include "pikataco.h"

#define GSM_N_CHANS	2
#define GSM_MAX_CHANS	8

struct gsm_chan;

struct gsm_card {
	char name[8];
	struct device class_dev;

	int irq;
	unsigned rev;

	void __iomem *fpga;

	u8 *rx_buf, *tx_buf;

	struct gsm_chan *chans[GSM_MAX_CHANS];
};

struct gsm_ctl {
	char                 name[8];
	struct device  class_dev;
	struct cdev          cdev;

	/* read / write */
	struct semaphore     use_sem;
	unsigned int         used_flags;
	int			use_count;

	struct semaphore     write_sem;
	struct semaphore     read_sem;

	struct kfifo       * rx_fifo;
	wait_queue_head_t    rx_queue;

	struct kfifo       * tx_fifo; /* SAM not used */
	wait_queue_head_t    tx_queue;
	int tx_empty;
};

struct gsm_port {
	char                 name[8];
	struct device  class_dev;
	struct cdev          cdev;

	/* read / write */
	atomic_t             in_use;

	struct semaphore     read_sem;
	struct kfifo       * rx_fifo;
	wait_queue_head_t    rx_queue;

	struct semaphore     write_sem;
	struct kfifo       * tx_fifo;
	wait_queue_head_t    tx_queue;
};

struct gsm_chan {
	unsigned int	id;
	void __iomem	*fpga;
	struct gsm_card	*card;
	struct gsm_ctl	ctl;
	struct gsm_port	port;
	int timeslot;

	atomic_t	sim_slot_curr;
	atomic_t	sim_slot_next;

	atomic_t	use_count;
// SAM	atomic_t	on_off_state;
// SAM	unsigned int	on_off_timer;
// SAM	atomic_t	req_on;
// SAM	atomic_t	call_state;

	int		call_ready_state;
};


static inline unsigned gsm_fpga_read(struct gsm_card *gsm, int reg)
{
	return in_be32(gsm->fpga + reg);
}

static inline void gsm_fpga_write(struct gsm_card *gsm, int reg, unsigned val)
{
	out_be32(gsm->fpga + reg, val);
}

static inline int gsm_codec_write(struct gsm_chan *chan, int reg, u8 val)
{
	return fpga_write_indirect(chan->fpga, chan->id | 0x20, reg, val);
}

static inline int gsm_codec_read(struct gsm_chan *chan, int reg, u8 *val)
{
	return fpga_read_indirect(chan->fpga, chan->id | 0x20, reg, val);
}

static inline int gsm_uart_write(struct gsm_chan *chan, int reg, u8 val)
{
	return fpga_write_indirect(chan->fpga, chan->id, reg, val);
}

static inline int gsm_uart_read(struct gsm_chan *chan, int reg, u8 *val)
{
	return fpga_read_indirect(chan->fpga, chan->id, reg, val);
}

static inline unsigned chan_fpga_read(struct gsm_chan *chan, int reg)
{
	return in_be32(chan->fpga + reg);
}

static inline void chan_fpga_write(struct gsm_chan *chan, int reg, unsigned val)
{
	out_be32(chan->fpga + reg, val);
}

/* helper macros */
#define _DEBUG
#ifdef _DEBUG
# define DEBUG(...)                        printk(KERN_INFO "bnxgsm: <debug> " __VA_ARGS__)
# define DEBUG_CARD(card,fmt,...)          printk(KERN_INFO "bnxgsm: [%s] <debug> " fmt, \
												  (card)->name, ##__VA_ARGS__)
# define DEBUG_CHAN(chan,fmt,...)          printk(KERN_INFO "bnxgsm: [%s:%d] <debug> " fmt, \
												  (chan)->card->name, (chan)->id, ##__VA_ARGS__)
# define DEBUG_CTL(chan,fmt,...)           printk(KERN_INFO "bnxgsm: [%s:%s] <debug> " fmt, \
												  (chan)->card->name, (chan)->ctl.name, ##__VA_ARGS__)
# define DEBUG_PORT(chan,fmt,...)          printk(KERN_INFO "bnxgsm: [%s:%s] <debug> " fmt, \
												  (chan)->card->name, (chan)->port.name, ##__VA_ARGS__)
#else
# define DEBUG(...)
# define DEBUG_CARD(...)
# define DEBUG_CHAN(...)
# define DEBUG_CTL(...)
# define DEBUG_PORT(...)
#endif

#define LOG(...)                           printk(KERN_INFO "bnxgsm: " __VA_ARGS__)
#define LOG_CARD(card,fmt,...)             printk(KERN_INFO "bnxgsm: [%s] " fmt, \
												  (card)->name, ##__VA_ARGS__)
#define LOG_CHAN(chan,fmt,...)             printk(KERN_INFO "bnxgsm: [chan%d] " fmt, \
												  (chan)->id, ##__VA_ARGS__)
#define LOG_CTL(chan,fmt,...)              printk(KERN_INFO "bnxgsm: [%s:%s] " fmt, \
												  (chan)->card->name, (chan)->ctl.name, ##__VA_ARGS__)
#define LOG_PORT(chan,fmt,...)             printk(KERN_INFO "bnxgsm: [%s:%s] " fmt, \
												  (chan)->card->name, (chan)->port.name, ##__VA_ARGS__)

#define ERR(...)                           printk(KERN_ERR "bnxgsm: <error> " __VA_ARGS__)
#define ERR_CARD(card,fmt,...)             printk(KERN_ERR "bnxgsm: [%s] <error> " fmt, \
												  (card)->name, ##__VA_ARGS__)
#define ERR_CHAN(chan,fmt,...)             printk(KERN_ERR "bnxgsm: [%s:%d] <error> " fmt, \
												  (chan)->card->name, (chan)->id, ##__VA_ARGS__)
#define ERR_CTL(chan,fmt,...)              printk(KERN_ERR "bnxgsm: [%s] <error> " fmt, \
												  (chan)->ctl.name, ##__VA_ARGS__)
#define ERR_PORT(chan,fmt,...)             printk(KERN_ERR "bnxgsm: [%s] <error> " fmt, \
												  (chan)->port.name, ##__VA_ARGS__)

#endif
