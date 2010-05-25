/*DOM-IGNORE-BEGIN*/
/*
 * ==================================================================
 * This software, together with any related media and documentation,
 * is subject to the terms and conditions of the PIKA MonteCarlo
 * Software License Agreement in the license.txt file which is
 * included in product installation package.
 * ==================================================================
 */
/*DOM-IGNORE-END*/

#ifndef PKH_BOARD_H
#define PKH_BOARD_H

#define PKH_BOARD_EEPROM_PRODUCTNUM_ARRAY_SIZE     	14
#define PKH_BOARD_DRIVER_VERSION_STRING_LENGTH     	50
#define PKH_BOARD_EEPROM_LICENSEINFO_ARRAY_SIZE     	4

#define PKH_BOARD_MAX_RING_PATTERNS          		16
#define PKH_BOARD_MAX_NUMBER_OF_TRUNKS       		24
#define PKH_BOARD_MAX_NUMBER_OF_PHONES       		24

#define PKH_BOARD_THRESHOLD_DETECT_DEBOUNCE_TIME_MIN        0  /* Minimum threshold detection debounce time in ms. */
#define PKH_BOARD_THRESHOLD_DETECT_DEBOUNCE_TIME_MAX     2040  /* Maximum threshold detection debounce time in ms. */
#define PKH_BOARD_THRESHOLD_DETECT_DEBOUNCE_TIME_DEFAULT  296  /* Default threshold detection debounce time in ms. */

#define PKH_BOARD_HOOK_STATE_DEBOUNCE_TIME_MIN           0  /* Minimum hookstate debounce time in ms. */
#define PKH_BOARD_HOOK_STATE_DEBOUNCE_TIME_MAX         159  /* Maximum hookstate debounce time in ms. */
#define PKH_BOARD_HOOK_STATE_DEBOUNCE_TIME_DEFAULT     150  /* Default hookstate debounce time in ms. */

#define PKH_BOARD_HOOKFLASH_DEBOUNCE_TIME_MIN            0  /* Minimum hookflash debounce time in ms. */
#define PKH_BOARD_HOOKFLASH_DEBOUNCE_TIME_DEFAULT      800  /* Default hookflash debounce time in ms. */

#define PKH_BOARD_DISCONNECT_DURATION_MIN                1  /* Minimum disconnect duration in ms. */
#define PKH_BOARD_DISCONNECT_DURATION_DEFAULT          500  /* Default disconnect duration in ms. */

typedef struct
{
  u32 featureCode;         /* Type of licensed feature */
  u32 quantity;            /* Number of available licenses */
} PKH_TLicenseInfo;

typedef struct
{
  u32           serialNumber;       /* Board serial number in decimal format. */
  u32           cardID;             /* Board type; use PKH_TBoardType to determine the meaning of this field. */
  u32           realCardID;         /* Detailed board type; use PKH_TBoardRealType to determine the meaning of this field. */
  u32           productRevision;    /* Board revision. */
  u8            productNumber[PKH_BOARD_EEPROM_PRODUCTNUM_ARRAY_SIZE]; /* Board product id. */
  PKH_TLicenseInfo licenseInfo[PKH_BOARD_EEPROM_LICENSEINFO_ARRAY_SIZE];  /* Board based licenses */
} PKH_TBoardEEPROM;

/*
 * The PKH_TBoardInfo structure is used by PKH_BOARD_GetInfo function to retrieve read-only
 * information describing the board location and configuration in the system. This
 * information is primarily used for verifying the board type, serial number, and
 * slot position in the computer.
 *
 * Remarks:
 * The first 16 bits of the trunk mask map to the 16 trunks on the Analog Gateway board.
 * A bit set to 1 indicates the interface associated with that trunk is operational.
 * It does not indicate the trunk is present. Use the PKH_TRUNK_GetInfo function
 * to determine if a trunk is connected to the interface.
 *
 * Similarly, the first 12 bits of the phone mask map to the 12 phones on the
 * Analog Gateway board, if they are present. A bit set to 1 indicates the interface
 * associated with that phone is operational.
 */
typedef struct
{
  u32          busId;  /* Id of the PCI bus the where the card is located. */
  u32          slotId; /* Id of the PCI slot the card is plugged into on the PCI bus. */
  u32          irql;   /* Interrupt level assigned to the card */
  PKH_TBoardEEPROM eeprom; /* Information stored in non-volatile ram on the board. */
  s8          driverVersion[PKH_BOARD_DRIVER_VERSION_STRING_LENGTH]; /* Version of the driver used with this board. */
  u32           FPGA_version;   /* FPGA Version. */
  u32          numberOfSpans;  /* Number of spans on the Digital Gateway card. */
  u32          spanMask;       /* Mask of operational span interfaces. */
  u32          numberOfTrunks; /* Number of trunks on the Analog Gateway card. */
  u32          trunkMask;      /* Mask of operational trunk interfaces. */
  u32          numberOfPhones; /* Number of phones on the Analog Gateway card. */
  u32          phoneMask;      /* Mask of operational phone interfaces. */
  u32          disabledMask;   /* Mask of lines that are currently disabled. */
  u32          numberOfGSMs;   /* Number of GSM interfaces on the Analog Gateway card. */
  u32          gsmMask;        /* Mask of operational GSM interfaces. */
} PKH_TBoardInfo;

#endif	// PKH_BOARD_H
