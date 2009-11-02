/**
 * Interface definition for the spi communication
 * to special chips.
 *
 */

#ifndef __AUERASK_CP3K_SPI_H__
#define __AUERASK_CP3K_SPI_H__

#define SPI_WRITE_SINGLE_BC     0x00
#define SPI_WRITE_ADR_BC        0x60
#define SPI_READ_SINGLE_BC      0x80

#define SPI_CSNONE      0x07    // nothing

#define WSPICS_OFS      10      // chip select and mode for spi

#define write_wspics(data) writeb((data), auergb_wioadr + WSPICS_OFS)

#define WSPIOUT_OFS     12      // spi output register

#define write_wspiout(data) writeb((data), auergb_wioadr + WSPIOUT_OFS)

#define RSPIIN_OFS      28      // spi read register

#define read_rspiin() readb(auergb_rioadr + RSPIIN_OFS)

typedef enum {
  INITIAL     =  0, // Initial state - nothing happened
  REQUESTED   =  1, // IO-Mem requested but not mapped
  MAPPED      =  2, // IO-Mem is ready to use
} iomem_state_e;

/* Spinlock fuer das Locken der SPI-Zugriffe */
extern spinlock_t spi_lock;

/* Start the spi transfer. Sets clock polarity and
   forces chip select to low. */
void spi_start(void __iomem * base_address);

/* Simple read access that prepares the next access. */
u8 spi_read_go(void __iomem * base_address);

/* Read followed by a write. */
u8 spi_read_write(void __iomem * base_address, u8 data);

/* A simple read. */
u8 spi_read(void __iomem * base_address);

/* Read followed by a STOP signalling. */
u8 spi_read_stop(void __iomem * base_address);

/* Write access */
void spi_write(void __iomem * base_address, u8 data);

/* Stop after the last access. */
void spi_stop(void __iomem * base_address);

/**
 * Has to be called, if a kernel module wants
 * to use SPI communicate to a Chip
 */
iomem_state_e auerask_cp3k_spi_init(void);

/**
 * Has to be called, when a kernel module
 * does not want to use SPI anymore.
 */
iomem_state_e auerask_cp3k_spi_exit(void);

#endif // __AUERASK_CP3K_SPI_H__
