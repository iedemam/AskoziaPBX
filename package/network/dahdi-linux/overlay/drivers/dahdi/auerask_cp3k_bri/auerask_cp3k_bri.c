/* $Id: zap_xhfc_su.c,v 1.12 2007/08/30 14:15:40 martinb1 Exp $
 *
 * DAHDI driver for CologneChip AG's XHFC
 *
 * Authors : Martin Bachem, Joerg Ciesielski
 * Contact : info@colognechip.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *******************************************************************************
 *
 * MODULE PARAMETERS:
 *
 * - protocol=<p1>[,p2,p3...]
 *   Values:
 *        <bit 0>   0x0001  Net side stack (NT mode)
 *        <bit 1>   0x0002  Line Interface Mode (0=S0, 1=Up)
 *        <bit 2>   0x0004  st line polarity (1=exchanged)
 *
 *        <bit 7>   0x0080  B1 channel loop ST-RX -> XHFC PCM -> ST-TX
 *        <bit 8>   0x0100  B2 channel loop ST-RX -> XHFC PCM -> ST-TX
 *        <bit 9>   0x0200  D channel loop  ST-RX -> XHFC PCM -> ST-TX
 *
 * - debug:
 *        enable debugging (see xhfc_su.h for debug options)
 *
 */

/*******
 * Modifications made by Jens-D. Moeller for porting this driver
 * to DAHDI in order to be used with Auerswald COMpact 3000 PBXes.
 *
 * Major changes are SPI-access to the XHFC and PLL adjustment
 * due to a different clocking. The interrupt is triggered
 * by another kernel module.
 * The driver was stripped down to what we need for our CP3K
 * hardware. The XHFCs fifo setup remains the same but the
 * b-channel data is transported via PCM bus and grabbed by
 * another driver within an DMA interrupt.
 *
 * Renamed the files from zap_xhfc_su* to auerask_cp3k_bri*
 * to reflect the changes made and to indicate that this driver
 * out of the box will probably work on no other hardware than
 * the Auerswald COMpact 3000 PBXes together with the
 * Askozia distribution.
 *
 * Added code to read a hardware modules NT/TE/S0/UP0
 * configuration. Removed useless chip configuration  for
 * interfaces not connected.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <asm/timex.h>
#include <dahdi/kernel.h>
#include "auerask_cp3k_bri.h"
#include "xhfc24succ.h"

/* Mainboard driver for hardware access */
#include "auerask_cp3k_mb.h"	


static const char xhfc_rev[] = "$Revision: 1.12 $";

#define GPIO_BITMASK 0x03
#define UP0INT       0x01
#define S0EXT        0x02
#define S0INT        0x03

#define MAX_CARDS	8
static int card_cnt;
static u_int protocol[MAX_CARDS * MAX_PORT];
static int debug = 0;

// #define OLD_MODULE_PARAM

#ifdef MODULE
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");
#endif
#ifdef OLD_MODULE_PARAM
MODULE_PARM(debug, "1i");
#define MODULE_PARM_T   "1-8i"
MODULE_PARM(protocol, MODULE_PARM_T);
#else
module_param(debug, uint, S_IRUGO | S_IWUSR);
#ifdef OLD_MODULE_PARAM_ARRAY
static int num_protocol = 0, num_layermask = 0;
module_param_array(protocol, uint, num_protocol, S_IRUGO | S_IWUSR);
#else
module_param_array(protocol, uint, NULL, S_IRUGO | S_IWUSR);
#endif
#endif
#endif

#define NUM_SLOTS 2

static xhfc_hw* hw_p[NUM_SLOTS+1];

/**
 * read_xhfc: reads a single register via spi
 *
 * @param       base_address    xhfc spi base address
 * @param       reg             xhfc register address to be read
 * @return                      the read byte value
 */
static inline __u8 read_xhfc(xhfc_hw * xhfc, __u8 reg_addr)
{
	register unsigned result;
	unsigned long flags;

	// lock spi access
	spin_lock_irqsave(&spi_lock, flags);
	// start spi transfer
	spi_start(xhfc->base_address);
	// write control byte: write address broadcast
	spi_write(xhfc->base_address, SPI_WRITE_ADR_BC);
	// write address byte
	spi_write(xhfc->base_address, reg_addr);
	// write control byte: read single data byte
	spi_write(xhfc->base_address, SPI_READ_SINGLE_BC);
	// read data cycle
	spi_write(xhfc->base_address, 0);
	// read out data byte from spi register
	result = spi_read_stop(xhfc->base_address);
	// unlock 
	spin_unlock_irqrestore(&spi_lock, flags);
	// ready
	return result;
}

/**
 * write_xhfc: write a single register value into the xhfc
 *
 * @param       base_address     base spi address for the xhfc
 * @param       reg              register addresss in the xhfc
 * @param       data             data byte to be written
 */
static inline void write_xhfc(xhfc_hw * xhfc, __u8 reg_addr, __u8 value)
{
	unsigned long flags;

	// lock spi access
	spin_lock_irqsave(&spi_lock, flags);
	// start spi transfer
	spi_start(xhfc->base_address);
	// write control byte: write address broadcast
	spi_write(xhfc->base_address, SPI_WRITE_ADR_BC);
	// write address byte
	spi_write(xhfc->base_address, reg_addr);
	// write control byte: write single data byte
	spi_write(xhfc->base_address, SPI_WRITE_SINGLE_BC);
	// write data cycle
	spi_write(xhfc->base_address, value);
	// force chip select high
	spi_stop(xhfc->base_address);
	// unlock
	spin_unlock_irqrestore(&spi_lock, flags);
}

/**
 * reads_xhfc: reads a block of data from the XHFC.
 *
 * @param       base_address    base spi address for the xhfc
 * @param       reg             register addresss in the xhfc
 * @param       data            data byte to be read
 * @param       len             number of bytes to be read
 */
static void inline readn_xhfc(void __iomem * base_address, unsigned reg,
			      uint8_t * data, unsigned len)
{
	unsigned long flags;

	// no len, no work
	if (!len)
		return;
	// Absicherung gegen Interrupts
	spin_lock_irqsave(&spi_lock, flags);
	// Starte den SPI-Transfer.
	spi_start(base_address);
	// write control byte: write address broadcast
	spi_write(base_address, SPI_WRITE_ADR_BC);
	// write address byte
	spi_write(base_address, reg);
	// start first transfer
	// write control byte: read multiple or read single
	spi_write(base_address,
		  (len >= 4 ? SPI_READ_MULTIPLE : SPI_READ_SINGLE_BC));
	do {
		if (len >= 4) {
			// input invalid data
			(void)spi_read_go(base_address);
			// read first byte
			--len;
			*data++ = spi_read_go(base_address);
			// read 2nd byte
			--len;
			*data++ = spi_read_go(base_address);
			// read 3rd byte
			--len;
			*data++ = spi_read_go(base_address);
			// read 4th byte, write next command
			--len;
			if (!len)
				continue;
			// write control byte: read multiple or read single
			*data++ =
			    spi_read_write(base_address,
					   (len >=
					    4 ? SPI_READ_MULTIPLE :
					    SPI_READ_SINGLE_BC));
		} else {
			// input invalid data
			(void)spi_read_go(base_address);
			// read out data byte from SPI register
			--len;
			if (!len)
				continue;
			// write control byte: read single
			*data++ =
			    spi_read_write(base_address, SPI_READ_SINGLE_BC);
		}
	} while (len);
	// read out last data value, set CS high
	*data = spi_read_stop(base_address);
	// lock wieder freigeben
	spin_unlock_irqrestore(&spi_lock, flags);
}

/**
 * writes_xhfc: writes a block of data into the chip
 *
 * @param       base_address    base spi address for the xhfc
 * @param       reg             register addresss in the xhfc
 * @param       data            data byte to be written
 * @param       len             number of bytes to be written
 */
static void inline writen_xhfc(void __iomem * base_address, unsigned reg,
			       uint8_t * data, unsigned len)
{
	unsigned long flags;

	// Absicherung gegen Interrupts
	spin_lock_irqsave(&spi_lock, flags);
	// Starte den SPI-Transfer.
	spi_start(base_address);
	// write control byte: write address broadcast
	spi_write(base_address, SPI_WRITE_ADR_BC);
	// write address byte
	spi_write(base_address, reg);
	do {
		if (len >= 4) {
			// write control byte:  write multiple
			spi_write(base_address, SPI_WRITE_MULTIPLE);
			// write first byte
			--len;
			spi_write(base_address, *data++);
			// write 2nd byte
			--len;
			spi_write(base_address, *data++);
			// write 3rd byte
			--len;
			spi_write(base_address, *data++);
			// write 4th byte
			--len;
			spi_write(base_address, *data++);
		} else {
			// write control byte: write single data byte
			spi_write(base_address, SPI_WRITE_SINGLE_BC);
			// write data cycle
			--len;
			spi_write(base_address, *data++);
		}
	} while (len);
	// set CS high after the transfer
	spi_stop(base_address);
	// lock wieder freigeben
	spin_unlock_irqrestore(&spi_lock, flags);
}

/**
 * reads four bytes
 */
inline __u32 read32_xhfc(xhfc_hw * xhfc, __u8 reg_addr)
{
	__u32 data;

	readn_xhfc(xhfc->base_address, reg_addr, (__u8 *) & data, 4);

	return (data);
}

/**
 * writes four bytes
 */
inline void write32_xhfc(xhfc_hw * xhfc, __u8 reg_addr, __u32 value)
{
	writen_xhfc(xhfc->base_address, reg_addr, (__u8 *) & value, 4);
}

/**
 * TODO: short read method is not implemented yet
 * In fact it is only used for debug output so far.
 */
static inline __u8 sread_xhfc(xhfc_hw * xhfc, __u8 reg_addr)
{
  /* Can lead to erroneous read results */
  return read_xhfc(xhfc, reg_addr);
}

static inline __u8 read_xhfcregptr(xhfc_hw * xhfc)
{
	return 0;
}

static inline void write_xhfcregptr(xhfc_hw * xhfc, __u8 reg_addr)
{
}

/* End of low-level IO interface */


/* static function prototypes */
static void setup_su(xhfc_hw * hw, __u8 pt, __u8 bc, __u8 enable);
static void init_su(xhfc_hw * hw, __u8 pt);
static void enable_interrupts(xhfc_hw * hw);
static void xhfc_ph_command(xhfc_port_t * port, u_char command);
static void setup_fifo(xhfc_hw * hw, __u8 fifo, __u8 conhdlc, __u8 subcfg,
		       __u8 fifoctrl, __u8 enable);
static void xhfc_selslot(xhfc_hw * hw, __u8 slot, __u8 dir);
static void setup_tslot(xhfc_hw * hw, __u8 slot, __u8 dir, __u8 sl_cfg);


static int xhfc_spanconfig(struct dahdi_span *span,
			    struct dahdi_lineconfig *lc)
{
	return 0;
}

static int xhfc_chanconfig(struct dahdi_chan *chan, int sigtype)
{
	return 0;
}

static int xhfc_maint(struct dahdi_span *span, int cmd)
{
	return 0;
}

static int xhfc_close(struct dahdi_chan *chan)
{
	module_put(THIS_MODULE);
	return 0;
}

static int xhfc_open(struct dahdi_chan *chan)
{
	xhfc_port_t *port = chan->pvt;
	unsigned long flags;

	try_module_get(THIS_MODULE);

	spin_lock_irqsave(&port->hw->lock, flags);

	/* try to activate when L1 port channel gets opened,
	   e.g. at asterisk startup */
	if ((!(test_bit(HFC_L1_ACTIVATED, &port->l1_flags))) &&
	    (!(test_bit(HFC_L1_ACTIVATING, &port->l1_flags)))) {
		// e.g. reset dahdi alarms
		if (port->zspan.alarms != DAHDI_ALARM_NONE) {
			port->zspan.alarms = DAHDI_ALARM_NONE;
			dahdi_alarm_notify(&port->zspan);
		}

		xhfc_ph_command(port, (port->mode & PORT_MODE_TE) ?
				HFC_L1_ACTIVATE_TE : HFC_L1_ACTIVATE_NT);
	}

	spin_unlock_irqrestore(&port->hw->lock, flags);

	return 0;
}

static int xhfc_ioctl(struct dahdi_chan *chan, unsigned int cmd,
		       unsigned long data)
{
	switch (cmd) {
	default:
		return -ENOTTY;
	}
	return 0;
}


/* DAHDI calls this when it has data it wants to send to the HDLC controller */
static void xhfc_hdlc_hard_xmit(struct dahdi_chan *chan)
{
  //printk(KERN_INFO "%s:%d\n", __FUNCTION__, __LINE__);
}

static int xhfc_rbsbits(struct dahdi_chan *chan, int bits)
{
	printk(KERN_INFO "%s\n", __FUNCTION__);
	return 0;
}

static int xhfc_shutdown(struct dahdi_span *span)
{
	xhfc_port_t *port = (xhfc_port_t *) span->pvt;
	__u8 bc;
	unsigned long flags;

	if (!(span->flags & DAHDI_FLAG_RUNNING))
		return (0);

	spin_lock_irqsave(&port->hw->lock, flags);

	/* disable B-Channel Fifos */
	for (bc = 0; bc < 2; bc++) {
		setup_fifo(port->hw, (port->idx * 8) + (bc * 2), 6, 0, 0, 0);
		setup_fifo(port->hw, (port->idx * 8) + (bc * 2) + 1, 6, 0, 0,
			   0);
		setup_su(port->hw, port->idx, bc, 0);
	}

	if (port->mode & PORT_MODE_NT)
		xhfc_ph_command(port, HFC_L1_DEACTIVATE_NT);

	spin_unlock_irqrestore(&port->hw->lock, flags);
	return 0;
}

/*
 * xhfc_startup is called at 'dahdi_cfg' from userspace
 */
static int xhfc_startup(struct dahdi_span *span)
{
	xhfc_port_t *port = (xhfc_port_t *) span->pvt;
	__u8 bc;
	unsigned long flags;

	if (debug & DEBUG_HFC_INIT)
		printk(KERN_INFO "%s %s\n", __FUNCTION__, span->name);

	spin_lock_irqsave(&port->hw->lock, flags);

	for (bc = 0; bc < 2; bc++) {

		/* setup B channel buffers (8 bytes each) */
		memset(port->brxbuf[bc], 0x0, sizeof(port->brxbuf[bc]));
		port->zchans[bc]->readchunk = port->brxbuf[bc];
		memset(port->btxbuf[bc], 0x0, sizeof(port->btxbuf[bc]));
		port->zchans[bc]->writechunk = port->btxbuf[bc];

		if (((!bc) && (!(port->mode & (PORT_MODE_LOOP_B1)))) ||
		    (bc && (!(port->mode & (PORT_MODE_LOOP_B2))))) {
		  __u8 slot, dir, sl_cfg, hfc_channel; 
		  reg_a_con_hdlc a_con_hdlc_bk;
		  
		        a_con_hdlc_bk.reg = 0;
		        a_con_hdlc_bk.bit_value.v_iff = 0;        /* 0 fill */
		        a_con_hdlc_bk.bit_value.v_hdlc_trp = 1;   /* transparent mode */
		        a_con_hdlc_bk.bit_value.v_fifo_irq = 7;   /* no interrupt, fifo active */
		        a_con_hdlc_bk.bit_value.v_data_flow = 7;  /* PCM <-> S/T interface */

		        setup_fifo(port->hw, (port->idx * 8) + (bc * 2), a_con_hdlc_bk.reg, 0, 0, 1);	// enable TX Fifo
			setup_fifo(port->hw, (port->idx * 8) + (bc * 2) + 1, a_con_hdlc_bk.reg, 0, 0, 1);	// enable RX Fifo

			/* PCM timeslot */
			slot = ((port->hw->slot-1)*4) + bc;
			/* TX=0, RX=1 */
			dir = 0;
			/* build hfc channel identifier */
			hfc_channel = (port->idx * 8 + bc) << 1;
			/* slot configuration */
			sl_cfg = M_ROUT | hfc_channel | dir;

			/* TX */
			setup_tslot(port->hw, slot, dir, sl_cfg);

			dir = 1;
			sl_cfg = M_ROUT | hfc_channel | dir;

			/* RX */
			setup_tslot(port->hw, slot, dir, sl_cfg);

			setup_su(port->hw, port->idx, bc, 1);
		} else {
			if (debug & DEBUG_HFC_INIT)
				printk(KERN_INFO
				       "%s %s skipping B%c initialization"
				       " due to Testloop at port(%i)\n",
				       __FUNCTION__, span->name,
				       (bc) ? '2' : '1', port->idx);
		}
	}

	/* dahdi chan 2 used as HDLC D-Channel */
	if (!(port->mode & PORT_MODE_LOOP_D)) {
		setup_fifo(port->hw, (port->idx * 8) + 4, 5, 2, M_MIX_IRQ, 1);	/* D TX */
		setup_fifo(port->hw, (port->idx * 8) + 5, 5, 2, M_MIX_IRQ, 1);	/* D RX */
	} else {
		if (debug & DEBUG_HFC_INIT)
			printk(KERN_INFO "%s %s skipping D initialization"
			       " due to Testloop at port(%i)\n",
			       __FUNCTION__, span->name, port->idx);
	}

	port->zchans[2]->flags &= ~DAHDI_FLAG_HDLC;
	port->zchans[2]->flags |= DAHDI_FLAG_NOSTDTXRX;

	if (debug & DEBUG_HFC_INIT)
		printk(KERN_INFO "%s %s starting port(%i)\n", __FUNCTION__,
		       span->name, port->idx);

	/* enable this port's state machine */
	write_xhfc(port->hw, R_SU_SEL, port->idx);
	write_xhfc(port->hw, A_SU_WR_STA, 0);

	if (port->mode & PORT_MODE_TE)
		xhfc_ph_command(port, HFC_L1_ACTIVATE_TE);
	else
		xhfc_ph_command(port, HFC_L1_ACTIVATE_NT);

	span->flags |= DAHDI_FLAG_RUNNING;

	spin_unlock_irqrestore(&port->hw->lock, flags);
	return (0);
}

static int init_dahdi_interface(xhfc_port_t * port)
{
	__u8 i;

	port->zspan.spanconfig = xhfc_spanconfig;
	port->zspan.chanconfig = xhfc_chanconfig;
	port->zspan.startup = xhfc_startup;
	port->zspan.shutdown = xhfc_shutdown;
	port->zspan.maint = xhfc_maint;
	port->zspan.rbsbits = xhfc_rbsbits;
	port->zspan.open = xhfc_open;
	port->zspan.close = xhfc_close;
	port->zspan.ioctl = xhfc_ioctl;
	port->zspan.hdlc_hard_xmit = xhfc_hdlc_hard_xmit;

	port->zspan.chans = port->zchans;
	port->zspan.channels = 3;
	port->zspan.deflaw = DAHDI_LAW_ALAW;
	port->zspan.linecompat = DAHDI_CONFIG_AMI | DAHDI_CONFIG_CCS;
	init_waitqueue_head(&port->zspan.maintq);
	port->zspan.pvt = port;
	port->zspan.offset = port->idx;

	sprintf(port->zspan.name, "%s%d/%d", DRIVER_NAME, port->hw->cardnum,
		port->idx);

	sprintf(port->zspan.desc, port->hw->card_name);
	sprintf(port->zspan.devicetype, port->hw->card_name);

	port->zspan.manufacturer = "Auerswald GmbH & Co. KG";
	port->zspan.spantype = (port->mode & PORT_MODE_TE) ? "TE" : "NT";

	if(port->hw->slot == AUERMOD_CP3000_SLOT_S0)
	  sprintf(port->zspan.location, "mainboard");
	else if(port->hw->slot == AUERMOD_CP3000_SLOT_MOD)
	  sprintf(port->zspan.location, "plug in module");
	else
	  sprintf(port->zspan.location, "unknown");

	for (i = 0; i < port->zspan.channels; i++) {
		port->zchans[i] = &port->_chans[i];
		memset(port->zchans[i], 0x0, sizeof(struct dahdi_chan));
		sprintf(port->zchans[i]->name, "%s/%d", port->zspan.name,
			i + 1);
		port->zchans[i]->pvt = port;

		port->zchans[i]->sigcap = DAHDI_SIG_EM | DAHDI_SIG_CLEAR | DAHDI_SIG_FXSLS |
		    DAHDI_SIG_FXSGS | DAHDI_SIG_FXSKS | DAHDI_SIG_FXOLS |
		    DAHDI_SIG_FXOGS | DAHDI_SIG_FXOKS | DAHDI_SIG_CAS |
		    DAHDI_SIG_SF | DAHDI_SIG_DACS;

		port->zchans[i]->chanpos = i + 1;
	}

	if (dahdi_register(&port->zspan, 0)) {
		printk(KERN_INFO DRIVER_NAME
		       ": unable to register dahdi span %d!\n", port->idx + 1);
		return -1;
	}

	return (0);
}

/**
 * Polls a busy flag. This is used when switching between
 * array registers of the same address.
 */
static inline void xhfc_waitbusy(xhfc_hw * hw)
{
	while (read_xhfc(hw, R_STATUS) & M_BUSY) ;
}

static inline void xhfc_selfifo(xhfc_hw * hw, __u8 fifo)
{
	write_xhfc(hw, R_FIFO, fifo);
	xhfc_waitbusy(hw);
}

/**
 * Selects a special pcm time slot for configuration.
 * hw   - points to the hw data structure
 * slot - pcm slot number
 * dir  - direction: 1 receive, 0 transmit
 */
static inline void xhfc_selslot(xhfc_hw * hw, __u8 slot, __u8 dir)
{
	write_xhfc(hw, R_SLOT, (slot << 1) | dir );
	xhfc_waitbusy(hw);
}

static inline void xhfc_inc_f(xhfc_hw * hw)
{
	write_xhfc(hw, A_INC_RES_FIFO, M_INC_F);
	xhfc_waitbusy(hw);
}

static inline void xhfc_resetfifo(xhfc_hw * hw)
{
	write_xhfc(hw, A_INC_RES_FIFO, M_RES_FIFO | M_RES_FIFO_ERR);
	xhfc_waitbusy(hw);
}

/****************************************************/
/* Physical S/U commands to control Line Interface  */
/****************************************************/
static void xhfc_ph_command(xhfc_port_t * port, u_char command)
{
	xhfc_hw *hw = port->hw;

	switch (command) {
	case HFC_L1_ACTIVATE_TE:
		if (debug & DEBUG_HFC_S0_STATES)
			printk(KERN_INFO "HFC_L1_ACTIVATE_TE port(%i)\n",
			       port->idx);

		set_bit(HFC_L1_ACTIVATING, &port->l1_flags);
		write_xhfc(hw, R_SU_SEL, port->idx);
		write_xhfc(hw, A_SU_WR_STA, STA_ACTIVATE);
		port->t3 = HFC_TIMER_T3;
		break;

	case HFC_L1_FORCE_DEACTIVATE_TE:
		if (debug & DEBUG_HFC_S0_STATES)
			printk(KERN_INFO
			       "HFC_L1_FORCE_DEACTIVATE_TE port(%i)\n",
			       port->idx);

		write_xhfc(hw, R_SU_SEL, port->idx);
		write_xhfc(hw, A_SU_WR_STA, STA_DEACTIVATE);
		break;

	case HFC_L1_ACTIVATE_NT:
		if (debug & DEBUG_HFC_S0_STATES)
			printk(KERN_INFO "HFC_L1_ACTIVATE_NT port(%i)\n",
			       port->idx);

		set_bit(HFC_L1_ACTIVATING, &port->l1_flags);
		write_xhfc(hw, R_SU_SEL, port->idx);
		write_xhfc(hw, A_SU_WR_STA, STA_ACTIVATE | M_SU_SET_G2_G3);
		break;

	case HFC_L1_DEACTIVATE_NT:
		if (debug & DEBUG_HFC_S0_STATES)
			printk(KERN_INFO "HFC_L1_DEACTIVATE_NT port(%i)\n",
			       port->idx);

		write_xhfc(hw, R_SU_SEL, port->idx);
		write_xhfc(hw, A_SU_WR_STA, STA_DEACTIVATE);
		break;

	case HFC_L1_TESTLOOP_B1:
		if (debug & DEBUG_HFC_S0_STATES)
			printk(KERN_INFO "HFC_L1_TESTLOOP_B1 port(%i)\n",
			       port->idx);
		setup_fifo(hw, port->idx * 8, 0xC6, 0, 0, 0);	/* connect B1-SU RX with PCM TX */
		setup_fifo(hw, port->idx * 8 + 1, 0xC6, 0, 0, 0);	/* connect B1-SU TX with PCM RX */

		write_xhfc(hw, R_SLOT, port->idx * 8);	/* PCM timeslot B1 TX */
		write_xhfc(hw, A_SL_CFG, port->idx * 8 + 0x80);	/* enable B1 TX timeslot on STIO1 */

		write_xhfc(hw, R_SLOT, port->idx * 8 + 1);	/* PCM timeslot B1 RX */
		write_xhfc(hw, A_SL_CFG, port->idx * 8 + 1 + 0xC0);	/* enable B1 RX timeslot on STIO1 */

		setup_su(hw, port->idx, 0, 1);
		break;

	case HFC_L1_TESTLOOP_B2:
		if (debug & DEBUG_HFC_S0_STATES)
			printk(KERN_INFO "HFC_L1_TESTLOOP_B2 port(%i)\n",
			       port->idx);

		setup_fifo(hw, port->idx * 8 + 2, 0xC6, 0, 0, 0);	/* connect B2-SU RX with PCM TX */
		setup_fifo(hw, port->idx * 8 + 3, 0xC6, 0, 0, 0);	/* connect B2-SU TX with PCM RX */

		write_xhfc(hw, R_SLOT, port->idx * 8 + 2);	/* PCM timeslot B2 TX */
		write_xhfc(hw, A_SL_CFG, port->idx * 8 + 2 + 0x80);	/* enable B2 TX timeslot on STIO1 */

		write_xhfc(hw, R_SLOT, port->idx * 8 + 3);	/* PCM timeslot B2 RX */
		write_xhfc(hw, A_SL_CFG, port->idx * 8 + 3 + 0xC0);	/* enable B2 RX timeslot on STIO1 */

		setup_su(hw, port->idx, 1, 1);
		break;

	case HFC_L1_TESTLOOP_D:
		if (debug & DEBUG_HFC_S0_STATES)
			printk(KERN_INFO "HFC_L1_TESTLOOP_D port(%i)\n",
			       port->idx);
		setup_fifo(hw, port->idx * 8 + 4, 0xC4, 2, M_FR_ABO, 1);	/* connect D-SU RX with PCM TX */
		setup_fifo(hw, port->idx * 8 + 5, 0xC4, 2, M_FR_ABO | M_FIFO_IRQMSK, 1);	/* connect D-SU TX with PCM RX */

		write_xhfc(hw, R_SLOT, port->idx * 8 + 4);	/* PCM timeslot D TX */
		write_xhfc(hw, A_SL_CFG, port->idx * 8 + 4 + 0x80);	/* enable D TX timeslot on STIO1 */

		write_xhfc(hw, R_SLOT, port->idx * 8 + 5);	/* PCM timeslot D RX */
		write_xhfc(hw, A_SL_CFG, port->idx * 8 + 5 + 0xC0);	/* enable D RX timeslot on STIO1 */
		break;
	}
}


/*
 * Handles ISDN layer-1 state changes. The state machine itself is
 * built into the hardware. But there are some timers to implement
 * that should be handled by software and the upper layers have to
 * be informed about certain state change events.
 *
 * If in TE-mode, the ISDN driver sends an event upon port sync
 * and lost port sync. The mainboard driver will take care about
 * it and eventually switch the systems pll to that external
 * clock source. This is done to prevent frequncy slips that lead
 * to disruptions in signalling.
 */
static void su_new_state(xhfc_port_t * port, __u8 su_state)
{
	reg_a_su_rd_sta new_state;

	new_state.reg = su_state;

	if (port->su_state.bit_value.v_su_sta != new_state.bit_value.v_su_sta) {
	        if (debug & DEBUG_HFC_S0_STATES)
			printk(KERN_INFO "%s %s %s%i\n", __FUNCTION__,
			       port->zspan.name,
			       (port->mode & PORT_MODE_NT) ? "G" : "F",
			       new_state.bit_value.v_su_sta);

		if (port->mode & PORT_MODE_TE) {

			/* disable T3 ? */
			if ((new_state.bit_value.v_su_sta <= 3)
			    || (new_state.bit_value.v_su_sta >= 7))
				port->t3 = -1;

			switch (new_state.bit_value.v_su_sta) {
			case (3):
				auerask_cp3k_sync_ev( AUERASK_UNSYNCED, port->hw->slot);
				if (test_and_clear_bit
				    (HFC_L1_ACTIVATED, &port->l1_flags))
					port->t4 = HFC_TIMER_T4;
				break;

			case (7):
				if (port->t4 > HFC_TIMER_OFF)
					port->t4 = HFC_TIMER_OFF;

				clear_bit(HFC_L1_ACTIVATING, &port->l1_flags);
				set_bit(HFC_L1_ACTIVATED, &port->l1_flags);

				auerask_cp3k_sync_ev( AUERASK_SYNCED, port->hw->slot);

				port->zspan.alarms = DAHDI_ALARM_NONE;
				dahdi_alarm_notify(&port->zspan);
				break;

			case (8):
				port->t4 = HFC_TIMER_OFF;
				auerask_cp3k_sync_ev( AUERASK_UNSYNCED, port->hw->slot);
				break;
			default:
				auerask_cp3k_sync_ev( AUERASK_UNSYNCED, port->hw->slot);
				break;
			}

		} else if (port->mode & PORT_MODE_NT) {

			switch (new_state.bit_value.v_su_sta) {
			case (1):
				clear_bit(HFC_L1_ACTIVATED, &port->l1_flags);
				port->nt_timer = 0;
				port->mode &= ~NT_TIMER;
				break;
			case (2):
				if (port->nt_timer < 0) {
					port->nt_timer = 0;
					port->mode &= ~NT_TIMER;
					clear_bit(HFC_L1_ACTIVATING,
						  &port->l1_flags);

					xhfc_ph_command(port,
							HFC_L1_DEACTIVATE_NT);
				} else {
					port->nt_timer = NT_T1_COUNT;
					port->mode |= NT_TIMER;

					write_xhfc(port->hw, R_SU_SEL,
						   port->idx);
					write_xhfc(port->hw, A_SU_WR_STA,
						   M_SU_SET_G2_G3);
				}
				break;
			case (3):
				clear_bit(HFC_L1_ACTIVATING, &port->l1_flags);
				set_bit(HFC_L1_ACTIVATED, &port->l1_flags);
				port->nt_timer = 0;
				port->mode &= ~NT_TIMER;
				break;
			case (4):
				port->nt_timer = 0;
				port->mode &= ~NT_TIMER;
				break;
			default:
				break;
			}
		}
		port->su_state.bit_value.v_su_sta =
		    new_state.bit_value.v_su_sta;
	}
}

static void xhfc_read_fifo_dchan(xhfc_port_t * port)
{
	xhfc_hw *hw = port->hw;
	__u8 *buf = port->drxbuf;
	int *idx = &port->drx_indx;
	__u8 *data;		/* pointer for new data */
	__u8 rcnt, i;
	__u8 f1 = 0, f2 = 0, z1 = 0, z2 = 0;

	/* select D-RX fifo */
	xhfc_selfifo(hw, (port->idx * 8) + 5);

	/* hdlc rcnt */
	f1 = read_xhfc(hw, A_F1);
	f2 = read_xhfc(hw, A_F2);
	z1 = read_xhfc(hw, A_Z1);
	z2 = read_xhfc(hw, A_Z2);

	rcnt = (z1 - z2) & hw->max_z;
	if (f1 != f2)
	  rcnt++; // read additional crc result

	if (rcnt > 0) {
		data = buf + *idx;
		*idx += rcnt;

		/* read data from FIFO */
		readn_xhfc(hw->base_address, A_FIFO_DATA, data, rcnt);

		/* hdlc frame termination */
		if (f1 != f2) {
			xhfc_inc_f(hw);

			/* check minimum frame size */
			if (*idx < 4) {
			  if (debug & DEBUG_HFC_FIFO_ERR)
			    printk(KERN_INFO
				   "%s: frame in port(%i) < minimum size\n",
				   __FUNCTION__, port->idx);
			  dahdi_hdlc_abort(port->zchans[2],
					   DAHDI_EVENT_ABORT);
			  goto read_exit;
			}
			
			/* check crc */
			if (buf[(*idx) - 1]) {
			  if (debug & DEBUG_HFC_FIFO_ERR)
			    printk(KERN_INFO
				   "%s: port(%i) CRC-error\n",
				   __FUNCTION__, port->idx);
			  dahdi_hdlc_abort(port->zchans[2],
					   DAHDI_EVENT_BADFCS);
			  goto read_exit;
			}

			/* D-Channel debug to syslog */
		        if (debug & DEBUG_HFC_DTRACE) 
			{
				printk(KERN_INFO "%s D-RX len(%02i): ",
				       port->zspan.name, (*idx));
				i = 0;
				while (i < (*idx)) {
					printk("%02x ", buf[i++]);
					if (i == (*idx - 3))
						printk("- ");
				}
				printk("\n");
			}
			
			/* Cut off the xhfc's status byte. Otherwise libpri will
			   try to interpret the first checksum byte as the
			   beginning of an information element. **/
			dahdi_hdlc_putbuf(port->zchans[2], data, rcnt-1);			
			dahdi_hdlc_finish(port->zchans[2]);

read_exit:
			*idx = 0;
		}
		else
		  dahdi_hdlc_putbuf(port->zchans[2], data, rcnt);

	}
}

static void xhfc_write_fifo_dchan(xhfc_port_t * port)
{
	xhfc_hw *hw = port->hw;
	__u8 fcnt, tcnt;
	__u8 free;
	__u8 f1, f2;
	int i;
	//	__u8 fstat;
	int result;

	unsigned char buf[MAX_DFRAME_LEN_L1];
	unsigned int size = MAX_DFRAME_LEN_L1;

	/* ask dahdi for the data buffer and size */
	result = dahdi_hdlc_getbuf(port->zchans[2], buf, &size); 	

	if(size <= MAX_DFRAME_LEN_L1 && size > 0)
	  { 
	    /* there is data to be transmitted */
	    xhfc_selfifo(hw, (port->idx * 8 + 4));

	    free = (hw->max_z - (read_xhfc(hw, A_USAGE))); /* the number of free bytes in the fifo */
	    tcnt = ((free >= size) ? size : free); /* bytes to be send */

	    f1 = read_xhfc(hw, A_F1);
	    f2 = read_xhfc(hw, A_F2);
	    fcnt = 0x07 - ((f1 - f2) & 0x07);	/* free frame count in tx fifo */

	    if (free && fcnt && tcnt) {
	      /* Alright, we can stuff tcnt bytes into the hardware. */

	      if (debug & DEBUG_HFC_DTRACE) {
		printk (KERN_INFO "%s D-TX tcnt(%02i)\n",
			port->zspan.name, tcnt);
	      }

	      if (debug & DEBUG_HFC_DTRACE) 
		{
		  printk(KERN_INFO "%s D-TX len(%02i): ",
			 port->zspan.name, tcnt);
		  i = 0;
		  while (i < tcnt) {
		    printk("%02x ", buf[i]);
		    i++;
		  }
		  printk("\n");
		}
	      
	      writen_xhfc(hw->base_address, A_FIFO_DATA, (__u8 *)&buf, tcnt);

	    }	

	    /* no data is left to send */
	    xhfc_selfifo(hw, (port->idx * 8 + 4));
	    
	    /* tell the hardware we are finished with this frame */
	    xhfc_inc_f(hw);
	  }

}

/*
 * Takes care about layer-1 state changes and transports the
 * d-channel data. dahdi_transmit and dahdi_receive have to
 * be called to trigger the data transport to/from the dahdi
 * upper layer implementation.
 */
static void xhfc_irq_work(xhfc_hw * hw)
{
      __u8 pt;

	hw->su_irq.reg |= read_xhfc(hw, R_SU_IRQ);

	/* get fifo IRQ states in bundle */
	for (pt = 0; pt < hw->num_ports; pt++) {
		hw->fifo_irq |=
		    (read_xhfc(hw, R_FIFO_BL0_IRQ + pt) << (pt * 8));
	}

	for (pt = 0; pt < hw->num_ports; pt++) {
		if (hw->port[pt].zspan.flags & DAHDI_FLAG_RUNNING) {
			/* handle S/U statechage */
			if (hw->su_irq.reg & (1 << pt)) {
				write_xhfc(hw, R_SU_SEL, pt);
				su_new_state(&hw->port[pt],
					     read_xhfc(hw, A_SU_RD_STA));
				hw->su_irq.reg &= ~(1 << pt);
			}
			
			/* Do echo cancellation if wanted */
			dahdi_ec_span(&hw->port[pt].zspan);

			/* get tx buffer filled by dahdi */
			dahdi_transmit(&hw->port[pt].zspan);
		      
			/* transmit D-Channel Data */
			if (test_bit
			    (HFC_L1_ACTIVATED,
			     &hw->port[pt].l1_flags)) {
			  xhfc_write_fifo_dchan(&hw->port[pt]);
			} else {
			  if (!
			      (test_bit
			       (HFC_L1_ACTIVATING,
				&hw->port[pt].l1_flags))) {
			    if (hw->port[pt].mode &
				PORT_MODE_TE)
			      xhfc_ph_command
				(&hw->port[pt],
				 HFC_L1_ACTIVATE_TE);
			    else
			      xhfc_ph_command
				(&hw->port[pt],
				 HFC_L1_ACTIVATE_NT);
			  }
			}
			
			/* receive D-Channel Data */
			xhfc_read_fifo_dchan(&hw->port[pt]);

			/* signal reveive buffers to dahdi */
			dahdi_receive(&hw->port[pt].zspan);

			/* e.g. reset DAHDI_ALARM_RED, if T3 and T4 off */
			if ((hw->port[pt].t3 == HFC_TIMER_OFF) &&
			    (hw->port[pt].t4 == HFC_TIMER_OFF) &&
			    hw->port[pt].zspan.alarms == DAHDI_ALARM_RED) {
				hw->port[pt].zspan.alarms = DAHDI_ALARM_NONE;
				dahdi_alarm_notify(&hw->port[pt].zspan);
			}

			/* handle t3 */
			if (hw->port[pt].t3 > HFC_TIMER_OFF) {
				/* timer expired ? */
				if (--hw->port[pt].t3 == 0) {
					if (debug & DEBUG_HFC_S0_STATES)
					  printk(KERN_INFO
						 "%s T3 expired port(%i) state(F%i)\n",
						 hw->port[pt].zspan.name,
						 hw->port[pt].idx,
						 hw->port[pt].su_state.
						 bit_value.v_su_sta);
					
					hw->port[pt].t3 = HFC_TIMER_OFF;
					clear_bit(HFC_L1_ACTIVATING,
						  &hw->port[pt].l1_flags),
					    xhfc_ph_command(&hw->port[pt],
							    HFC_L1_FORCE_DEACTIVATE_TE);

					hw->port[pt].zspan.alarms =
					    DAHDI_ALARM_RED;
					dahdi_alarm_notify(&hw->port[pt].zspan);
				}
			}

			/* handle t4 */
			if (hw->port[pt].t4 > HFC_TIMER_OFF) {
				/* timer expired ? */
				if (--hw->port[pt].t4 == 0) {
					if (debug & DEBUG_HFC_S0_STATES)
						printk(KERN_INFO
						       "%s T4 expired port(%i) state(F%i)\n",
						       hw->port[pt].zspan.name,
						       hw->port[pt].idx,
						       hw->port[pt].su_state.
						       bit_value.v_su_sta);

					hw->port[pt].t4 = HFC_TIMER_OFF;

					hw->port[pt].zspan.alarms =
					    DAHDI_ALARM_RED;
					dahdi_alarm_notify(&hw->port[pt].zspan);
				}
			}
			
		}
	}
}


/*
 * Lets the bfin_sport_tdm driver know where the
 * transmit buffer is. So the buffer can be filled by that driver.
 */
static u8* xhfc_get_bchan_tx_chunk(nr_slot_t slot, nr_port_t pnum, __u8 bchan) 
{
  u8* result;

  if(hw_p[slot])
      result = hw_p[slot]->port[pnum].btxbuf[bchan];
  else
    result = NULL;

  return result;
}


/*
 * Points to our receive buffer.
 */
static u8* xhfc_get_bchan_rx_chunk(nr_slot_t slot, nr_port_t pnum, __u8 bchan) 
{
  u8* result;

  if(hw_p[slot])
    {
      result = hw_p[slot]->port[pnum].brxbuf[bchan];
    }
  else
    result = NULL;

  return result;
}


/*
 * Handles all that ISDN signalling stuff.
 */
void xhfc_int(void)
{
  unsigned long flags;
  xhfc_hw *hw;
  nr_slot_t slot;


  for( slot=1; slot <= NUM_SLOTS; slot++)
    {  
      hw = hw_p[slot];
  
      if(!hw) /* ...this is not ISDN hardware to take care about */
	continue;
      
      if(!hw->irq_ctrl.bit_value.v_glob_irq_en)
	continue;

      spin_lock_irqsave(&hw->lock, flags);

      xhfc_irq_work(hw);
       
      spin_unlock_irqrestore(&hw->lock, flags);
    }
 
}

/*****************************************************/
/* disable all interrupts by disabling M_GLOB_IRQ_EN */
/*****************************************************/
static void disable_interrupts(xhfc_hw * hw)
{
  
  if (debug & DEBUG_HFC_IRQ)
    printk(KERN_INFO "%s %s\n", hw->card_name, __FUNCTION__);
  
  hw->irq_ctrl.bit_value.v_glob_irq_en = 0;
}

/******************************************/
/* start interrupt and set interrupt mask */
/******************************************/
static void enable_interrupts(xhfc_hw * hw)
{
  if (debug & DEBUG_HFC_IRQ)
    printk(KERN_INFO "%s %s\n", hw->card_name, __FUNCTION__);
  
  /* enable global interrupts */
  hw->irq_ctrl.bit_value.v_glob_irq_en = 1;
}

/***********************************/
/* initialise the XHFC ISDN Chip   */
/* return 0 on success.            */
/***********************************/
static int init_xhfc(xhfc_hw * hw)
{
	int err = 0;
	int timeout = 0x2000;
	//	unsigned long flags;
	reg_r_pcm_md0 reg_pcm_md0;

	if (debug & DEBUG_HFC_INIT)
		printk(KERN_INFO "%s %s ChipID: 0x%x\n", hw->card_name,
		       __FUNCTION__, hw->chip_id);

	switch (hw->chip_id) {
	case CHIP_ID_1SU:
		hw->num_ports = 1;
		hw->max_fifo = 4;
		hw->max_z = 0xFF;
		write_xhfc(hw, R_FIFO_MD, M1_FIFO_MD * 2);
		break;
	default:
		err = -ENODEV;
	}

	if (err) {
		if (debug & DEBUG_HFC_INIT)
			printk(KERN_ERR "%s %s: unkown Chip ID 0x%x\n",
			       hw->card_name, __FUNCTION__, hw->chip_id);
		return (err);
	}

	/* software reset to enable R_FIFO_MD setting */
	write_xhfc(hw, R_CIRM, M_SRES);
	udelay(5);
	write_xhfc(hw, R_CIRM, 0);

	write_xhfc(hw, R_FIFO_THRES, 0x11);

	while ((read_xhfc(hw, R_STATUS) & (M_BUSY | M_PCM_INIT))
	       && (timeout))
		timeout--;

	if (!(timeout)) {
		if (debug & DEBUG_HFC_INIT)
			printk(KERN_ERR
			       "%s %s: initialization sequence could not finish\n",
			       hw->card_name, __FUNCTION__);
		return (-ENODEV);
	}

	/* In contrast to PCI cards the XHFC is PCM slave here. */
	reg_pcm_md0.bit_value.v_pcm_md = 0;	// slave mode
	reg_pcm_md0.bit_value.v_c4_pol = 0;	// dont care
	reg_pcm_md0.bit_value.v_f0_neg = 0;	// polariy of F0IO clock
	reg_pcm_md0.bit_value.v_f0_len = 0;	// short F0IO signal in slave mode

	/* select R_PCM_MD2 */
	reg_pcm_md0.bit_value.v_pcm_idx = 0x0A;
	write_xhfc(hw, R_PCM_MD0, reg_pcm_md0.reg);
	/* wait until that register is selected */
	udelay(5);

	/*
	 * V_SYNC_OUT1=0,
	 * V_SYNC_OUT2=0,
	 * V_C2I_EN=1 and V_C2O_EN=0 for C2IO
	 * to be available as clock in
	 */
	write_xhfc(hw, R_PCM_MD2, M_C2I_EN);

	/* M=1 and use PLL clock output */
	write_xhfc(hw, R_CLK_CFG, M_CLKO_PLL);

	/* PLL Setup values taken from XHFC1SU datasheet
	   version Oct. 2007 page 275. */
	write_xhfc(hw, R_PLL_P, 1);
	write_xhfc(hw, R_PLL_N, 6);
	write_xhfc(hw, R_PLL_S, 3);

	/* start PLL */
	write_xhfc(hw, R_PLL_CTRL, M_PLL_NRES);

	/* PLL-out is chip system clock */
	write_xhfc(hw, R_CLK_CFG, M_CLK_PLL | M_CLKO_PLL | M_CLKO_OFF);

	{			
	        /* wait for the PLL to lock **swing** */
		uint8_t temp = read_xhfc(hw, R_PLL_STA);
		uint16_t loop_count = 0;

		while (!(temp & M_PLL_LOCK)) {
			temp = read_xhfc(hw, R_PLL_STA);
			loop_count++;

			if (loop_count >= LOOP_TIMEOUT) {
				printk(KERN_ERR "%s --- PLL-Lock TIMEOUT ---\n",
				       __FUNCTION__);
				break;
			}
		}	
	}

	return(0);
}

/*********************************************/
/* init port (line interface) with SU_CRTLx  */
/*********************************************/
static void init_su(xhfc_hw * hw, __u8 pt)
{
	xhfc_port_t *port = &hw->port[pt];

	write_xhfc(hw, R_SU_SEL, pt);

	if (port->mode & PORT_MODE_NT)
		port->su_ctrl0.bit_value.v_su_md = 1;

	if (port->mode & PORT_MODE_EXCH_POL)
		port->su_ctrl2.reg = M_SU_EXCHG;

	if (port->mode & PORT_MODE_UP) {
		port->st_ctrl3.bit_value.v_st_sel = 1;
		write_xhfc(hw, A_MS_TX, 0x0F);
		port->su_ctrl0.bit_value.v_st_sq_en = 1;
	}

	/* configure end of pulse control for ST mode (TE & NT) */
	if (port->mode & PORT_MODE_S0) {
		port->su_ctrl0.bit_value.v_st_pu_ctrl = 1;
		port->st_ctrl3.reg = 0xf8;
	}

	if (debug & DEBUG_HFC_MODE)
		printk(KERN_INFO "%s %s port(%i) "
		       "su_ctrl0(0x%02x) "
		       "su_ctrl1(0x%02x) "
		       "su_ctrl2(0x%02x) "
		       "st_ctrl3(0x%02x)\n",
		       hw->card_name, __FUNCTION__, pt,
		       port->su_ctrl0.reg,
		       port->su_ctrl1.reg,
		       port->su_ctrl2.reg, port->st_ctrl3.reg);

	write_xhfc(hw, A_ST_CTRL3, port->st_ctrl3.reg);
	write_xhfc(hw, A_SU_CTRL0, port->su_ctrl0.reg);
	write_xhfc(hw, A_SU_CTRL1, port->su_ctrl1.reg);
	write_xhfc(hw, A_SU_CTRL2, port->su_ctrl2.reg);

	if (port->mode & PORT_MODE_TE)
		write_xhfc(hw, A_SU_CLK_DLY, CLK_DLY_TE);
	else
		write_xhfc(hw, A_SU_CLK_DLY, CLK_DLY_NT);
}

/*
 * Setup the systems pcm timeslot.
 *
 */
static void
setup_tslot(xhfc_hw * hw, __u8 slot, __u8 dir, __u8 sl_cfg)
{
  xhfc_selslot(hw, slot, dir);
  write_xhfc(hw, A_SL_CFG, sl_cfg);
}

/*********************************************************/
/* Setup Fifo using A_CON_HDLC, A_SUBCH_CFG, A_FIFO_CTRL */
/*********************************************************/
static inline void
setup_fifo(xhfc_hw * hw, __u8 fifo, __u8 conhdlc, __u8 subcfg,
	   __u8 fifoctrl, __u8 enable)
{
	xhfc_selfifo(hw, fifo);
	write_xhfc(hw, A_CON_HDLC, conhdlc);
	write_xhfc(hw, A_SUBCH_CFG, subcfg);
	write_xhfc(hw, A_FIFO_CTRL, fifoctrl);

	if (enable)
		hw->fifo_irqmsk |= (1 << fifo);
	else
		hw->fifo_irqmsk &= ~(1 << fifo);

	xhfc_resetfifo(hw);
	xhfc_selfifo(hw, fifo);

      	if (debug & DEBUG_HFC_MODE) {
		printk(KERN_INFO
		       "%s %s: fifo(%i) conhdlc(0x%02x) subcfg(0x%02x) fifoctrl(0x%02x)\n",
		       hw->card_name, __FUNCTION__, fifo,
		       sread_xhfc(hw, A_CON_HDLC),
		       sread_xhfc(hw, A_SUBCH_CFG), sread_xhfc(hw, A_FIFO_CTRL)
		    );
	}
}

/**************************************************/
/* Setup S/U interface, enable/disable B-Channels */
/**************************************************/
static void setup_su(xhfc_hw * hw, __u8 pt, __u8 bc, __u8 enable)
{
	xhfc_port_t *port = &hw->port[pt];

	if (!((bc == 0) || (bc == 1))) {
		printk(KERN_INFO "%s %s: pt(%i) ERROR: bc(%i) unvalid!\n",
		       hw->card_name, __FUNCTION__, pt, bc);
		return;
	}

	if (debug & DEBUG_HFC_MODE)
		printk(KERN_INFO "%s %s %s pt(%i) bc(%i)\n",
		       hw->card_name, __FUNCTION__,
		       (enable) ? ("enable") : ("disable"), pt, bc);

	if (bc) {
		port->su_ctrl2.bit_value.v_b2_rx_en = (enable ? 1 : 0);
		port->su_ctrl0.bit_value.v_b2_tx_en = (enable ? 1 : 0);
	} else {
		port->su_ctrl2.bit_value.v_b1_rx_en = (enable ? 1 : 0);
		port->su_ctrl0.bit_value.v_b1_tx_en = (enable ? 1 : 0);
	}

	if (hw->port[pt].mode & PORT_MODE_NT)
		hw->port[pt].su_ctrl0.bit_value.v_su_md = 1;

	write_xhfc(hw, R_SU_SEL, pt);
	write_xhfc(hw, A_SU_CTRL0, hw->port[pt].su_ctrl0.reg);
	write_xhfc(hw, A_SU_CTRL2, hw->port[pt].su_ctrl2.reg);
}

/********************************/
/* parse module paramaters like */
/* NE/TE and S0/Up port mode    */
/********************************/
static void parse_module_params(xhfc_hw * hw)
{
  __u8 pt, gpio_reg;

	/*
	 * Prepare the chips GPIOs to read the 
	 * hardware confguration.
	 */
	write_xhfc(hw, R_GPIO_SEL, M_GPIO_SEL0 | M_GPIO_SEL1 );

	xhfc_waitbusy(hw);

	gpio_reg = read_xhfc(hw, R_GPIO_IN0);

	/* read the hardware configuration */
	for (pt = 0; pt < hw->num_ports; pt++) {

	  switch((gpio_reg >> (2*pt)) & GPIO_BITMASK )
	    {
	    case UP0INT:
	      hw->port[pt].mode |= PORT_MODE_NT;
	      hw->port[pt].mode |= PORT_MODE_UP;
	      break;
	    case S0EXT:
	      hw->port[pt].mode |= PORT_MODE_TE;
	      hw->port[pt].mode |= PORT_MODE_S0;
	      break;
	    case S0INT:
	      hw->port[pt].mode |= PORT_MODE_NT;
	      hw->port[pt].mode |= PORT_MODE_S0;
	      break;
	    default:
	      /* fall-back */
	      hw->port[pt].mode |= PORT_MODE_TE;
	      hw->port[pt].mode |= PORT_MODE_S0;
	      printk(KERN_ERR "Fatal Error: Your hardware should send smoke signals!\n");
	    }
	  
	  /* st line polarity */
	  if (protocol[hw->param_idx + pt] & 0x04)
	    hw->port[pt].mode |= PORT_MODE_EXCH_POL;
	  
	  /* get layer1 loop-config */
	  if (protocol[hw->param_idx + pt] & 0x80)
	    hw->port[pt].mode |= PORT_MODE_LOOP_B1;

	  if (protocol[hw->param_idx + pt] & 0x0100)
	    hw->port[pt].mode |= PORT_MODE_LOOP_B2;
	  
	  if (protocol[hw->param_idx + pt] & 0x0200)
	    hw->port[pt].mode |= PORT_MODE_LOOP_D;
	  
	  if (debug & DEBUG_HFC_INIT)
	    printk("%s %s: protocol[%i]=0x%02x, mode:%s,%s %s\n",
		   hw->card_name, __FUNCTION__, hw->param_idx + pt,
		   protocol[hw->param_idx + pt],
		   (hw->port[pt].mode & PORT_MODE_TE) ? "TE" : "NT",
		   (hw->port[pt].mode & PORT_MODE_S0) ? "S0" : "Up",
		   (hw->port[pt].
		    mode & PORT_MODE_EXCH_POL) ? "SU_EXCH" : "");
	}
}

/********************************/
/* initialise the XHFC hardware */
/* return 0 on success.         */
/********************************/
static int __devinit setup_instance(xhfc_hw * hw)
{
	int err;
	int pt;

	if (debug & DEBUG_HFC_INIT)
		printk(KERN_WARNING "%s %s: requesting IRQ %d\n",
		       hw->card_name, __FUNCTION__, hw->irq);

	spin_lock_init(&hw->lock);

	err = init_xhfc(hw);
	if (err)
		goto out;

	parse_module_params(hw);

	/* init line interfaces */
	for (pt = 0; pt < hw->num_ports; pt++) {
		/* init back pointers */
		hw->port[pt].idx = pt;
		hw->port[pt].hw = hw;

		/* init port as TE/NT */
		init_su(hw, pt);

		hw->port[pt].t3 = -1;	/* init t3 timer : Timer OFF */
		hw->port[pt].t4 = -1;	/* init t4 timer : Timer OFF */

		err = init_dahdi_interface(&hw->port[pt]);
		if (err)
			goto out;
	}


	enable_interrupts(hw);

	/* init loops if desired */
	for (pt = 0; pt < hw->num_ports; pt++) {
		if (hw->port[pt].mode & PORT_MODE_LOOP_B1)
			xhfc_ph_command(&hw->port[pt], HFC_L1_TESTLOOP_B1);
		if (hw->port[pt].mode & PORT_MODE_LOOP_B2)
			xhfc_ph_command(&hw->port[pt], HFC_L1_TESTLOOP_B2);
		if (hw->port[pt].mode & PORT_MODE_LOOP_D)
			xhfc_ph_command(&hw->port[pt], HFC_L1_TESTLOOP_D);
	}

	return (0);

out:
	return (err);
}

/************************/
/* release single card  */
/************************/
static void release_card(xhfc_hw * hw)
{
	__u8 pt;

	disable_interrupts(hw);

	for (pt = 0; pt < hw->num_ports; pt++)
		if (hw->port[pt].zspan.flags & DAHDI_FLAG_REGISTERED) {
			if (debug & DEBUG_HFC_INIT)
				printk(KERN_INFO "%s dahdi_unregister %s\n",
				       __FUNCTION__, hw->port[pt].zspan.name);
			dahdi_unregister(&hw->port[pt].zspan);
		}

	mdelay(100);
	kfree(hw);
}

static int __init xhfc_spi_probe(void)
{
	xhfc_hw *hw;
	int err = -ENOMEM;
	int mod_id = 0;
	nr_slot_t slot;

	hw_p[0] = NULL; /* by definition */

	for( slot=1; slot <= NUM_SLOTS; slot++)
	  {
	    /* Take a short look if the ISDN plug-in module is present. */
	    mod_id = auerask_cp3k_read_modID(slot);
	    
	    if(slot == AUERMOD_CP3000_SLOT_MOD) {
	      if(mod_id != MODID1S0UP0 && mod_id != MODID1S0){
		hw_p[slot] = NULL;
		continue;
	      }
	    } else if(slot == AUERMOD_CP3000_SLOT_S0) {
	      if(mod_id != MODID1S0UP0){
		hw_p[slot] = NULL;
		continue;
	      }
	    }

	    if (!(hw = kmalloc(sizeof(xhfc_hw), GFP_ATOMIC))) {
	      printk(KERN_ERR "%s %s: No kmem for XHFC card\n",
		     hw->card_name, __FUNCTION__);
	      return (err);
	    }

	    memset(hw, 0, sizeof(xhfc_hw));

	    /* Safe this pointer for shutdown purpose in module exit */
	    hw_p[slot] = hw;

	    hw->base_address = (void __iomem *) auerask_calc_spi_addr(SPI_CS1, slot, SPI_25M, SPI_CLKLH, SPI_INV);

	    hw->slot = slot;
	    hw->cardnum = card_cnt;
	    hw->irq = IRQ_PF11;
	    hw->chip_id = read_xhfc(hw, R_CHIP_ID);
	    hw->chip_rev = read_xhfc(hw, R_CHIP_RV) & 0x0f;

	    if(slot == AUERMOD_CP3000_SLOT_MOD) {
	      sprintf(hw->card_name, "AUERSWALD COMpact ISDN Modul");
	    } else if(slot == AUERMOD_CP3000_SLOT_S0) {
	      sprintf(hw->card_name, "AUERSWALD COMpact 3000 ISDN");
	    }	    

	    /* Some kind of probing is done here anyway... */
	    err = setup_instance(hw);
	    
	    if (!err) {
	      card_cnt++;
	      printk(KERN_INFO "%s XHFC 0x%02x:0x%02x interface found\n",
		     hw->card_name, hw->chip_id, hw->chip_rev);
	    } else {
	      kfree(hw);
	      hw_p[slot] = NULL;
	      /* continue probing the other slots */
	    }	    
	  }

	return (0);
};

/***************/
/* Module init */
/***************/
static int __init xhfc_init(void)
{
	int err=0;

       	auerask_cp3k_spi_init();
       	xhfc_spi_probe();

	printk(KERN_INFO "XHFC: %d cards installed\n", card_cnt);

	return(err);
}

static void __exit xhfc_cleanup(void)
{
	nr_slot_t slot;

	for( slot=1; slot <= NUM_SLOTS; slot++)
	  {
	    if(hw_p[slot]) {
	      release_card(hw_p[slot]);
	    }
	  }

	auerask_cp3k_spi_exit();

	printk(KERN_INFO "%s: driver removed\n", __FUNCTION__);
}

module_init(xhfc_init);
module_exit(xhfc_cleanup);

EXPORT_SYMBOL(xhfc_int);
EXPORT_SYMBOL(xhfc_get_bchan_rx_chunk);
EXPORT_SYMBOL(xhfc_get_bchan_tx_chunk);
