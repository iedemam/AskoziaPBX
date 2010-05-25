/******************************************************************************
** File - FlashStructure.h
**
** This is the structure definition for the seeprom template. 
** It corresponds (bytewise) to the contents of the seeprom.
** 
******************************************************************************/
#ifndef FLASHSTRUCTURE_H
#define FLASHSTRUCTURE_H

// FLASH_FIELDS_TEMPLATE structure must be byte aligned
#pragma pack(1)

typedef struct FLASH_FIELDS_TEMPLATE {
/* 0x00 */ u16 DeviceID;
/* 0x02 */ u16 VendorID;
/* 0x04 */ u16 ClassCode;
/* 0x06 */ u16 ClassCodeRevision;
/* 0x08 */ u16 MaximumLatencyMinimumGrant;
/* 0x0A */ u16 InterruptPinInterruptNumber;
/* 0x0C */ u16 Mailbox0_MSW;
/* 0x0E */ u16 Mailbox0_LSW;
/* 0x10 */ u16 Mailbox1_MSW;
/* 0x12 */ u16 Mailbox1_LSW;
/* 0x14 */ u16 PCIToLocalAddressSpace0Range_MSW;
/* 0x16 */ u16 PCIToLocalAddressSpace0Range_LSW;
/* 0x18 */ u16 PCIToLocalAddressSpace0BaseAddress_MSW;
/* 0x1A */ u16 PCIToLocalAddressSpace0BaseAddress_LSW;
/* 0x1C */ u16 ModeDMAArbitrationRegister_MSW;
/* 0x1E */ u16 ModeDMAArbitrationRegister_LSW;
/* 0x20 */ u16 SeepromWriteProtectedAddress;
/* 0x22 */ u16 LocalMiscControlReg_LocalBusEndianDescReg_LSW;
/* 0x24 */ u16 PCIToLocalExpansionROMRange_MSW;
/* 0x26 */ u16 PCIToLocalExpansionROMRange_LSW;
/* 0x28 */ u16 PCIToLocalExpansionROMBaseAddress_MSW;
/* 0x2A */ u16 PCIToLocalExpansionROMBaseAddress_LSW;
/* 0x2C */ u16 PCIToLocalAccessesBusRegionDescriptors_MSW;
/* 0x2E */ u16 PCIToLocalAccessesBusRegionDescriptors_LSW;
/* 0x30 */ u16 PCIInitiatorToPCIRange_MSW;
/* 0x32 */ u16 PCIInitiatorToPCIRange_LSW;
/* 0x34 */ u16 DirectMasterToPCIMemoryBaseAddress_MSW;
/* 0x36 */ u16 DirectMasterToPCIMemoryBaseAddress_LSW;
/* 0x38 */ u16 PCIInitiatorToPCIIOConfigBusAddress_MSW;
/* 0x3A */ u16 PCIInitiatorToPCIIOConfigBusAddress_LSW;
/* 0x3C */ u16 PCIInitiatorToPCIBaseAddress_MSW;
/* 0x3E */ u16 PCIInitiatorToPCIBaseAddress_LSW;
/* 0x40 */ u16 PCIInitiatorToPCIIOConfigConfAddressReg_MSW;
/* 0x42 */ u16 PCIInitiatorToPCIIOConfigConfAddressReg_LSW;
/* 0x44 */ u16 SubsystemID;
/* 0x46 */ u16 SubsystemVendorID;
/* 0x48 */ u16 PCIToLocalAddressSpace1Range_MSW;
/* 0x4A */ u16 PCIToLocalAddressSpace1Range_LSW;
/* 0x4C */ u16 PCIToLocalAddressSpace1BaseAddress_MSW;
/* 0x4E */ u16 PCIToLocalAddressSpace1BaseAddress_LSW;
/* 0x50 */ u16 PCIToLocalAccessesBusRegionDescriptorsSpace1_MSW;
/* 0x52 */ u16 PCIToLocalAccessesBusRegionDescriptorsSpace1_LSW;
/* 0x54 */ u16 HotSwapControlReg_MSW;
/* 0x56 */ u16 HotSwapControlReg_LSW;
/* 0x58 */ u8  Unused1[0xC];
/* 0x64 */ u16 MagicNumber_MSW;
/* 0x66 */ u16 MagicNumber_LSW;
/* 0x68 */ u16 SerialNumber_LSW;
/* 0x6A */ u16 SerialNumber_MSW;
/* 0x6C */ u16 CardID_MSW;
/* 0x6E */ u16 CardID_LSW;
/* 0x70 */ u16 InterfaceConfigurationInfo0;
/* 0x72 */ u16 InterfaceConfigurationInfo4;
/* 0x74 */ u16 InterfaceConfigurationInfo8;
/* 0x76 */ u16 InterfaceConfigurationInfo12;
/* 0x78 */ u16 InterfaceConfigurationInfo16;
/* 0x7A */ u16 InterfaceConfigurationInfo20;
/* 0x7C */ u8  Reserved1[0xC];
/* 0x88 */ u16 PerBoardFeatures;
/* 0x8A */ u8  CountryOptions;
/* 0x8B */ u8  DSPTypes;
/* 0x8C */ u8  BaseboardDSPExternalRAM;
/* 0x8D */ u8  ModuleDSPExternalRAM;
/* 0x8E */ u8  ProductNumber[0xE];
/* 0x9C */ u32 ProductRev;
/* 0xA0 */ u8  TemplateRev[0x14];
/* 0xB4 */ u8  Unused2[0xE];
/* 0xC2 */ u16 InterfaceConfigurationSubdefine0;
/* 0xC4 */ u16 InterfaceConfigurationSubdefine4;
/* 0xC6 */ u16 InterfaceConfigurationSubdefine8;
/* 0xC8 */ u16 InterfaceConfigurationSubdefine12;
/* 0xCA */ u16 InterfaceConfigurationSubdefine16;
/* 0xCC */ u16 InterfaceConfigurationSubdefine20;
/* 0xCE */ u8  Unused3[0xA];
/* 0xD8 */ u16 CustomerVendorID;
/* 0xDA */ u8  HostSyncSupport;
/* 0xDB */ u8  LicensingInfo[0x20];
/* 0xFB */ u8  Unused4[0x1];
/* 0xFC */ u16 WriteCounter;
/* 0xFE */ u16 Checksum;
} FLASH_FIELDS_STRUCT;

typedef union FLASH_TEMPLATE {
    FLASH_FIELDS_STRUCT	  FIELD_ACCESS;
    u16                WORD_ARRAY[128];
    u8                 BYTE_ARRAY[256];
} FLASH_UNION;



#endif
/* EOF */
