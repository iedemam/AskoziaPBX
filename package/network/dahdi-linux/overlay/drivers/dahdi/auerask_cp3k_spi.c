/**
 * Interface implementation that is offered to other
 * Auerswald Askozia drivers for COMpact 3000 VoIP PBXes.
 *
 */
#include <linux/kernel.h>	/* printk() */
#include <linux/ioport.h>
#include <linux/spinlock.h>
#include <asm/io.h>

#include "auerask_cp3k_spi.h"

const unsigned long gb_wioadr = 0x20200000; // AMS2: Start of area
const unsigned long gb_wiolen = 0x1000;     // One Page: works everywhere
const unsigned long gb_rioadr = 0;          // see wioadr
const unsigned long gb_riolen = 0;          // see wiolen

/* base address for IO writes */
void __iomem * auergb_wioadr = NULL;

/* base address for IO reads */
void __iomem * auergb_rioadr = NULL;

/* holds the number of IO-Mem "users" */
static int iomem_ref_count = 0;

/* for proper IO memory allocation and free */
static iomem_state_e iomem_state = INITIAL;

void spi_start(void __iomem * base_address)
{
    /* first access configures the clock polarity and sets
       the chip-select line to high */
    write_wspics((u8)((unsigned)base_address | SPI_CSNONE));

    /* "activate" that chip */
    write_wspics((u8)(unsigned)base_address);
}

u8 spi_read_go(void __iomem * base_address)
{
    u8 data = read_rspiin();
    write_wspiout(0);
    return data;
}

u8 spi_read_write(void __iomem * base_address, u8 data)
{
    u8 rdata = read_rspiin();
    write_wspiout(data);
    return rdata;
}

u8 spi_read(void __iomem * base_address)
{
    return read_rspiin();
}


u8 spi_read_stop(void __iomem * base_address)
{
    u8 data = read_rspiin();
    write_wspics((u8)((unsigned)base_address | SPI_CSNONE));
    return data;
}


void spi_write(void __iomem * base_address, u8 data)
{
    write_wspiout(data);
}

void spi_stop(void __iomem * base_address)
{
    write_wspics((u8)((unsigned)base_address | SPI_CSNONE));
}


/**
 * Sets up the IO memory for SPI IO use.
 */
static iomem_state_e iomem_alloc(void)
{
  switch(iomem_state)
    {
    case INITIAL:

      if (check_mem_region (gb_wioadr, gb_wiolen)) {
	printk(KERN_ERR "check_mem_region failed\n");
	/* state remains the same */
	return iomem_state;
      }

      request_mem_region(gb_wioadr, gb_wiolen, "auerswald_some_module_name");
      
      iomem_state = REQUESTED;

    case REQUESTED:
      if (gb_wioadr) {
	auergb_wioadr = ioremap_nocache( gb_wioadr, gb_wiolen);
	/* read-address and write-address are the same by default */
	auergb_rioadr = auergb_wioadr;
      }
      if (gb_rioadr) {
	auergb_rioadr = ioremap_nocache( gb_rioadr, gb_riolen);
      }

      iomem_state = MAPPED;

    case MAPPED:
      /* Nothing to do */
      break;
    }

  return iomem_state;
}


/**
 * Deconfigure unused IO-Mem so we do
 * leave a "clean" system
 */
static iomem_state_e iomem_free(void)
{
  switch(iomem_state)
    {
    case MAPPED:
      iounmap((void *)auergb_wioadr);
      auergb_wioadr = 0;
      
      iounmap((void *)auergb_rioadr);
      auergb_rioadr = 0;

      iomem_state = REQUESTED;

    case REQUESTED:
      release_mem_region(gb_wioadr, gb_wiolen);

      iomem_state = INITIAL;

    case INITIAL:
      /* no thing to do */
      break;
    }

  return iomem_state;
}


/**
 * Has to be called, if a kernel module wants
 * to use SPI communication to a chip
 */
iomem_state_e auerask_cp3k_spi_init(void)
{
  if(iomem_ref_count == 0)
    iomem_alloc();

  iomem_ref_count++;

  return iomem_state;
}


/**
 * Has to be called, when a kernel module
 * does not want to use SPI anymore.
 */
iomem_state_e auerask_cp3k_spi_exit(void)
{
  if(iomem_ref_count > 0)
    {
      iomem_ref_count--;

      if(iomem_ref_count == 0)
	iomem_free();
    }

  return iomem_state;
}
