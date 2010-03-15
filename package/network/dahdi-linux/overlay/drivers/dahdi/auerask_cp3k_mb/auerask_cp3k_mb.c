/**
 * Interface implementation that is offered to other
 * Auerswald Askozia drivers for COMpact 3000 VoIP PBXes.
 *
 */
#include <linux/kernel.h>	/* printk() */
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <asm/gptimers.h>

#include "auerask_cp3k_mb.h"

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

static pll_state_t pll_state;

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
 * Read module ID from hardware
 */
unsigned int auerask_cp3k_read_modID( nr_slot_t slot )
{
    unsigned int u = MODIDLEER;

    switch( slot ) 
    {
    case AUERMOD_CP3000_SLOT_MOD:     // plug in module slot
        switch (read_rmodid() & CP3000_MODID_MASK) 
        {
        case CP3000_MODID_ISDN2:
            u = MODID1S0UP0;
            break;
        case CP3000_MODID_ISDN2 + CP3000_MODID_MOD:
            u = MODID1S0UP0 + AUERMOD_MODMASK;
            break;
        case CP3000_MODID_ISDN:
            u = MODID1S0;
            break;
        case CP3000_MODID_ISDN + CP3000_MODID_MOD:
            u = MODID1S0 + AUERMOD_MODMASK;
            break;
        case CP3000_MODID_2AB:
            u = MODID2AB;
            break;
        case CP3000_MODID_2AB + CP3000_MODID_MOD:
            u = MODID2AB + AUERMOD_MODMASK;
            break;
        default:
            break;
        }
        break;
    case AUERMOD_CP3000_SLOT_S0:    // mainboard S0 port
	u = MODID1S0UP0;
      break;
    case AUERMOD_CP3000_SLOT_AB:    // mainboard FXO ports
        u = MODID4AB;
        break;
    default:
        break;
    }
    return u;
}


unsigned int auerask_cp3k_read_hwrev( void )
{
   unsigned int u = 0;

    u = read_rhw() & 0x00FF; 
    return u;
}

/**
 * Sets up the IO memory
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

      request_mem_region(gb_wioadr, gb_wiolen, "auerask_cp3k_mb");
      
      iomem_state = REQUESTED;

    case REQUESTED:
      if (gb_wioadr) {
	auergb_wioadr = ioremap_nocache( gb_wioadr, gb_wiolen);

	// Clocks fuer den IOM
	// Codecs auf 16 KHz, Modulclock wird vom Modultreiber gesetzt
	// KW: Modulclock default == 8kHz + Framesync late == SO-Modul
	// das analog Modul stellt diesen Wert bei der Initialisierung um
	if(auergb_wioadr)
	  write_wclock(WCLOCK_TN14_16|WCLOCK_FS_LATE);

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

/*
 * "calculate" a devices spi-address
 */
void __iomem * auerask_calc_spi_addr(unsigned cs, unsigned slot, unsigned speed, unsigned polarity, unsigned invert)
{
    unsigned base_address = SPI_CSNONE;
    switch (slot) {
        case AUERMOD_CP3000_SLOT_MOD : base_address = SPI_CSMOD;  break;
        case AUERMOD_CP3000_SLOT_S0  : base_address = SPI_CSISDN; break;
        case AUERMOD_CP3000_SLOT_AB  : base_address = SPI_CSTN12; break;
    }
    base_address |= speed;
    base_address |= polarity;
    base_address |= invert;
    return (void __iomem *) base_address;
}

/*
 * Switches the systems clocking source
 * pll to another sync source. For now,
 * there are just three cases.
 */
void write_pll_source(__u8 slot)
{
  switch(slot)
    {
    case AUERMOD_CP3000_SLOT_MOD:
       write_wsync(WSYNC_PLLON | WSYNC_PLLMOD);
      break;
    case AUERMOD_CP3000_SLOT_S0:
       write_wsync(WSYNC_PLLON);
      break;
    default:
       write_wsync(WSYNC_DEFAULT);
    }
}


/*
 * Receives a syncronisation event from a driver. This
 * routine keeps a tiny per-slot sync state data structure
 * up to date and switches the on-board pll for clock sync
 * if necessary. If there are multiple ports per slot that
 * can be used as sync source extensions will have to be
 * made here.
 */
void auerask_cp3k_sync_ev( __u8 sync, __u8 slot)
{
  if((pll_state.sync_source == 0) && (sync == AUERASK_SYNCED))
    {
      /* We don't have a sync source 
	 and this one can be used. */
      pll_state.sync_source = slot;
      write_pll_source(slot);
    }
  else if((pll_state.sync_source == slot) && (sync == AUERASK_UNSYNCED))
    {
      /* Our currently used sync source lost sync. Search for a new one. */
      __u8 i;

      /* This gives a higher priority to slots with low numbers.
       * Perhaps we will have to change this in future.
       */
      pll_state.sync_source = 0;
      for(i=0; i<CP3000_AUERMODANZ; i++)
	{
	  if(pll_state.slot_state[i] == SYNCED)
	    {
	      /* switch to new one */
	      pll_state.sync_source = i;
	      write_pll_source(slot);
	      break;
	    }
	}

      if(!pll_state.sync_source)
	write_pll_source(0); /* None found */
    }

  /* update our per-slot data-structure in any case */
  if(sync == AUERASK_SYNCED)
    pll_state.slot_state[slot] = SYNCED;  
  else if(sync == AUERASK_UNSYNCED)
    pll_state.slot_state[slot] = UNSYNCED;  

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


/**
 * Initialize some hardware
 * @param       exit        0 for module init, != 0 for module exit
 */
static void gb_io_setup( int exit )
{
    unsigned int au_pwm_period;

    write_wtnk14( 0 );
    write_wtnk56( 0 );
    write_wled( WLED_LANAKT );
    write_wsync( WSYNC_DEFAULT );
    write_wclock( WCLOCK_TN14_16 | WCLOCK_FS_LATE );
    write_wspics( SPI_CSNONE );
    // Setup timer for green power led
    au_pwm_period = get_sclk() / 128;
    set_gptimer_config( TIMER0_id, TIMER_MODE_PWM | TIMER_PERIOD_CNT | TIMER_PULSE_HI );
    set_gptimer_period( TIMER0_id, au_pwm_period );
    if( exit ) 
    {
        set_gptimer_pwidth( TIMER0_id, 0 );
        disable_gptimers( TIMER0bit );
    } else
    {
        set_gptimer_pwidth( TIMER0_id, au_pwm_period-1 );
        enable_gptimers( TIMER0bit );
    }
    return;
} // gb_io_setup()


static int __init auerask_cp3k_mb_init( void )
{
    __u8 i;

    iomem_alloc();
    gb_io_setup( 0 );
    pll_state.sync_source = 0;
    for( i = 0; i < CP3000_AUERMODANZ; i++ )
    {
        pll_state.slot_state[i] = UNSYNCED;
    }
    return 0;
}


static void __exit auerask_cp3k_mb_cleanup( void )
{
    gb_io_setup( 1 );
    iomem_free();
    return;
}


MODULE_AUTHOR("Auerswald GmbH & Co. KG");
MODULE_DESCRIPTION("Auerswald COMpact 3000 mainboard driver");
MODULE_LICENSE("GPL");

module_init(auerask_cp3k_mb_init);
module_exit(auerask_cp3k_mb_cleanup);

EXPORT_SYMBOL(auergb_wioadr);
EXPORT_SYMBOL(auergb_rioadr);
EXPORT_SYMBOL(auerask_cp3k_spi_init);
EXPORT_SYMBOL(auerask_cp3k_spi_exit);
EXPORT_SYMBOL(auerask_cp3k_read_modID);
EXPORT_SYMBOL(auerask_cp3k_read_hwrev);
EXPORT_SYMBOL(auerask_calc_spi_addr);
EXPORT_SYMBOL(auerask_cp3k_sync_ev);
EXPORT_SYMBOL(spi_write);
EXPORT_SYMBOL(spi_read);
EXPORT_SYMBOL(spi_read_stop);
EXPORT_SYMBOL(spi_read_go);
EXPORT_SYMBOL(spi_read_write);
EXPORT_SYMBOL(spi_stop);
EXPORT_SYMBOL(spi_start);



