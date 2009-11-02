/*
 * Auerswald COMpact 3000 ISDN NT driver
 *
 * Copyright (C) 2009 Auerswald GmbH & Co. KG
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

#include <linux/autoconf.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/errno.h>	/* error codes */
#include <linux/module.h>
#include <linux/types.h>	/* size_t */
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/pci.h>		/* for PCI structures */
#include <linux/delay.h>
#include <asm/io.h>
#include <linux/spinlock.h>
#include <linux/device.h>	/* dev_err() */
#include <linux/interrupt.h>
#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */
#include <linux/workqueue.h>	/* work_struct */
#include <linux/timer.h>	/* timer_struct */
#include <linux/moduleparam.h>
#include <linux/proc_fs.h>

#include <dahdi/kernel.h>

#include "auerask_cp3ks0.h"
#include "auerask_cp3k_spi.h"


/**
 * read_xhfc: reads a single register via spi
 *
 * @param       base_address    xhfc spi base address
 * @param       reg             xhfc register address to be read
 * @return                      the read byte value
 */
static unsigned inline read_xhfc(void __iomem * base_address, unsigned reg)
{
        register unsigned result;
        unsigned long flags;

        // lock spi access
        spin_lock_irqsave(&spi_lock, flags);
        // start spi transfer
        spi_start(base_address);
        // write control byte: write address broadcast
        spi_write(base_address, SPI_WRITE_ADR_BC);
        // write address byte
        spi_write(base_address, reg);
        // write control byte: read single data byte
        spi_write(base_address, SPI_READ_SINGLE_BC);
        // read data cycle
        spi_write(base_address, 0);
        // read out data byte from spi register
        result = spi_read_stop(base_address);
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
static void inline write_xhfc(void __iomem * base_address, unsigned reg, unsigned data)
{
        unsigned long flags;

        // lock spi access
        spin_lock_irqsave(&spi_lock, flags);
        // start spi transfer
        spi_start(base_address);
        // write control byte: write address broadcast
        spi_write(base_address, SPI_WRITE_ADR_BC);
        // write address byte
        spi_write(base_address, reg);
        // write control byte: write single data byte
        spi_write(base_address, SPI_WRITE_SINGLE_BC);
        // write data cycle
        spi_write(base_address, data);
        // force chip select high
        spi_stop(base_address);
        // unlock
        spin_unlock_irqrestore(&spi_lock, flags);
}


static int __init cp3ks0_init(void)
{
  /* base address for spi */
  void __iomem * spi_adr; 

  auerask_cp3k_spi_init();

  spi_adr = 0x3C;

  printk(KERN_ERR "%s: ID: 0x%02x Revision: 0x%02x\n",
	 __FUNCTION__,
	 read_xhfc(spi_adr, R_CHIP_ID),
	 read_xhfc(spi_adr, R_CHIP_RV) & 0x0f );

  auerask_cp3k_spi_exit();
  
  return 0;
}

static void __exit cp3ks0_exit(void)
{
    printk(KERN_ERR "%s: run\n", __FUNCTION__);
}

MODULE_AUTHOR("Auerswald GmbH & Co. KG");
MODULE_DESCRIPTION("Auerswald COMpact 3000 ISDN NT driver");
MODULE_LICENSE("GPL");


module_init(cp3ks0_init);
module_exit(cp3ks0_exit);
