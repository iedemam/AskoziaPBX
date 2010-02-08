/**
 * Interface definition for the spi communication
 * to special chips.
 *
 */

#ifndef __AUERASK_CP3K_MB_H__
#define __AUERASK_CP3K_MB_H__

#define SPI_WRITE_SINGLE_BC     0x00
#define SPI_WRITE_ADR_BC        0x60
#define SPI_WRITE_MULTIPLE      0x20
#define SPI_READ_SINGLE_BC      0x80
#define SPI_READ_MULTIPLE       0xA0



#define SPI_CSTN12      0x00    // Codec TN 1+2
#define SPI_CSTN34      0x01    // Codec TN 3+4
#define SPI_CSPOTS      0x02    // analoges Amt auf Grundboard
#define SPI_CSISDN      0x03    // ISDN Amt auf Grundboard
#define SPI_CSMOD       0x04    // Steckmodul
#define SPI_CSNONE      0x07    // nichts
#define SPI_CS1         0x00    // dummy, weil kompatibel mit CP VoIP
                                // Bit 3 = Taktfrequenz 0 = 12 MHz, 1 = 25 MHz
#define SPI_12M         0x00    // 12 MHz Takt
#define SPI_25M         0x08    // 25 MHz Takt
                                // Bit 4 = Clock Polarit�t 0 = HIGH/LOW, 1 = LOW/HIGH
#define SPI_CLKHL       0x00    // Clock Signal: HIGH/LOW
#define SPI_CLKLH       0x10    // Clock Signal: LOW/HIGH
                                // Bit 5 = Ruhepegel des Clock-Signals
#define SPI_NORM        0x00    // normal
#define SPI_INV         0x20    // invers (fuer XHFC)
                                // Bit 6-7 unbelegt
                                // Reset-Zustand: 0

#define WSPICS_OFS      10      // chip select and mode for spi

#define write_wspics(data) writeb((data), auergb_wioadr + WSPICS_OFS)

#define WSPIOUT_OFS     12      // spi output register

#define write_wspiout(data) writeb((data), auergb_wioadr + WSPIOUT_OFS)

#define RSPIIN_OFS      28      // spi read register

#define read_rspiin() readb(auergb_rioadr + RSPIIN_OFS)

#define RMODID_OFS      18      // ID register of plug in module
                                // Bit 0: modificationbit, 0 = original, 1 = modified
#define CP3000_MODID_MOD   0x01     // module modified
                                    // Bit 1-2: module ID of plug in module
#define CP3000_MODID_NONE  0x00     // no module
#define CP3000_MODID_ISDN  0x02     // S0 NT ISDN module plugged in
#define CP3000_MODID_2AB   0x04     // FXS module plugged in
#define CP3000_MODID_ISDN2 0x06     // S0 NT/TE UP0 ISDN module plugged in
#define CP3000_MODID_MASK  0x07     // ID mask

#define CP3000_MODID_GB_CODEC   0x10     // 0 if SSM2602 codecs on mainboard, otherwise 1
#define CP3000_MODID_MOD_CODEC  0x20     // 0 if SSM2602 codecs on plug in module, otherwise 1

#define read_rmodid() readb( auergb_rioadr + RMODID_OFS )

#define CP3000_AUERMODANZ           4  // nr of virtual module slots of CP3k hardware
#define AUERMOD_CP3000_SLOT_MOD     1  // slot id of CP3000 plugin module (may be intern BRI or 2 * fxs ports)
#define AUERMOD_CP3000_SLOT_S0      2  // slot id of onboard S0 bus
#define AUERMOD_CP3000_SLOT_AB      3  // slot id of CP3000 mainboard fxs ports (4 ports)
#define AUERMOD_MODMASK          0x01  // Bit 0 = modified hardware

#define WTNK14_OFS      0       // ringing register FXS 1..4 (mainboard)
                                 // Bit 0 = +40V FXS1     0 == off, 1 == on
                                 // Bit 1 = +40V FXS2     0 == off, 1 == on
                                 // Bit 2 = +40V FXS3     0 == off, 1 == on
                                 // Bit 3 = +40V FXS4     0 == off, 1 == on
                                 // Bit 4 = -80V FXS1     0 == off, 1 == on
                                 // Bit 5 = -80V FXS2     0 == off, 1 == on
                                 // Bit 6 = -80V FXS3     0 == off, 1 == on
                                 // Bit 7 = -80V FXS4     0 == off, 1 == on
                                 // reset state: 0
#define write_wtnk14(data) writeb((data), auergb_wioadr + WTNK14_OFS) 

#define WTNK56_OFS      2       // ringing register FXS 5..6 (plug in module
                                 // Bit 0 = +40V FXS5     0 == off, 1 == on
                                 // Bit 1 = +40V FXS6     0 == off, 1 == on
                                 // Bit 2 = nc
                                 // Bit 3 = nc
                                 // Bit 4 = -80V FXS5     0 == off, 1 == on
                                 // Bit 5 = -80V FXS6     0 == off, 1 == on
                                 // Bit 6 = nc
                                 // Bit 7 = nc
                                 // reset state: 0
#define write_wtnk56(data) writeb((data), auergb_wioadr + WTNK56_OFS)

#define WCLOCK_OFS      8       // Steuerung der FrameSyncs für Codecs und Steckmodul
                                // Bit 0 = Modul Frame Frequenz 0 = 8 KHz, 1 = 16 KHz
                                // Bit 1 = Modul Frame Position 0 = early, 1 = late
                                // Bit 2 = TN1-4 Frame Frequenz 0 = 8 KHz, 1 = 16 KHz
#define WCLOCK_TN56_16  0x01
#define WCLOCK_FS_LATE  0x02
#define WCLOCK_TN14_16  0x04

                                // Bit 3-7 unbelegt
                                // Reset-Zustand: 0
#define write_wclock(data) writeb((data), auergb_wioadr + WCLOCK_OFS)

#define RANRS_OFS       16      // Anrufsucher TN 1..6
                                // Bit 0..5: Schleifenstrom bei TN1-6 1 = aktiv
                                // Bit 6-7 unbelegt, 0
#define read_ranrs() readb(auergb_rioadr + RANRS_OFS)


#define RHW_OFS         20      // HW-Bits, Tasten, SD Card
                                // Bit 0-1: Anlagenkennung
#define CP3000_HW_ANALOG 0x00   // COMpact 3000 analog
#define CP3000_HW_ISDN   0x01   // COMpact 3000 ISDN
#define CP3000_HW_VOIP   0x02   // COMpact 3000 VoIP
#define CP3000_HW_MASK   0x03   // Maske fuer Anlagentyp
                                // Bit 2-3: Hardware-Revision   (0)

#define read_rhw() readb(auergb_rioadr + RHW_OFS)

/**
 * Types for slot- an portnumbers
 */
typedef unsigned short  nr_slot_t;
typedef unsigned short  nr_port_t;
#define AUERDB_PORT_UNGUELTIG   ( ( unsigned short ) - 1 )

/**
 *  empty slot
 */
#define MODIDLEER                       0x00    // ID of empty slot

/**
 * 4a/b module (COMpact 3000 mainboard)
 */
#define MODID4AB                        0x40
#define AUER4AB_PORTS                   4        // nr of FXS ports

/**
 * 2a/b module (COMpact 3000 plug in module)
 */
#define MODID2AB                        0x42
#define AUER2AB_PORTS                   2        // nr of FXS ports

/**
 * ISDN module (COMpact 3000 plug in module)
 */
#define MODID1S0                        0x44
#define AUER1S0_PORTS                   1        // nr of S0 ports

/**
 *
 */
#define MODID1S0UP0                     0x46
#define AUER1S0UP0_PORTS                1        // nr of S0/UP0 ports


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

/*
 * "calculate" a devices spi-address
 */
void __iomem * calc_spi_addr(unsigned int cs, unsigned int slot, unsigned int speed, unsigned int polarity, unsigned int invert);

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

/**
 * Read module ID from hardware
 */
unsigned int auerask_cp3k_read_modID( nr_slot_t slot );

/**
 * Read the systems hardware revision
 */
unsigned int auerask_cp3k_read_hwrev( void );

#endif // __AUERASK_CP3K_MB_H__
