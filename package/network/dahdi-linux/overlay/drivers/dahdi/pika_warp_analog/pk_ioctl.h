// -----------------------------------------------------------------
//
//  File: pk_ioctl.h
//
//  Description: This header file defines the IOCTL codes for the AOH
//    driver for MC7.0 project.
//
//    Each IOCTL code contains a command identifier, and other device
//    information, such as the file access type and buffering type.
//
//  History:
//    Date        Name          Reason
//    2005-08-24  Q. XIE        Modified for MC7.0 KM Drivers
//
// ==================================================================
// IF YOU DO NOT AGREE WITH THE FOLLOWING STATEMENT, YOU MUST
// PROMPTLY RETURN THE SOFTWARE AND ANY ACCOMPANYING DOCUMENTATION
// ("PRODUCT") TO PIKA TECHNOLOGIES INC. ("PIKA").
//
// Pika Technologies Inc. (hereinafter "PIKA") owns all title and
// ownership rights to Software.  "Software" means any data processing
// programs (hereinafter "programs") consisting of a series of
// instructions or statements in object code form, including any
// systemized collection of data in the form of a data base, and any
// related proprietary materials such as flow charts, logic diagrams,
// manuals media and listings provided for use with the programs.
// User has no right, express or implied, to transfer, sell, provide
// access to or dispose of Software to any third party without PIKA's
// prior written consent.  User will not copy, reverse assemble,
// reverse engineer, decompile, modify, alter, translate or display
// the Software other than as expressly permitted by PIKA in writing.
//
// -----------------------------------------------------------------
#ifndef PK_IOCTL_H
#define PK_IOCTL_H

#ifdef __linux__
# include <linux/ioctl.h>
#endif // __linux__

//=====================
// PIKA INCLUDED FILES
//=====================


//===================
// MACRO DEFINITIONS
//===================

/*--------------------------------------------------------------
 * AOH IOCTL OFFSET BASE (user defined area > 0x8000)
 * AOH/HSP/HMP IOCTL Command Offset (user defined area > 0x800)
 *--------------------------------------------------------------
 */
#ifdef _WIN32

# define PK_HANDLE                      HANDLE

# define HSP_TYPE                       0xA00
# define HMP_TYPE                       0xC00
# define DEVICE_TYPE_HMP                0xA000

# define PIKA_IOCTL(type, code) \
         CTL_CODE(DEVICE_TYPE_HMP, (type)+(code), METHOD_BUFFERED, FILE_ANY_ACCESS)
# define PIKA_IOCTL_RDONLY(type, code) \
         CTL_CODE(DEVICE_TYPE_HMP, (type)+(code), METHOD_BUFFERED, FILE_READ_ACCESS)

/* // BLAW - we cannot use our own defined NTSTATUS errors if we return to UM
   // because GetLastError translates from NTSTATUS errors to win32 errors.
# define PIKA_IOCTL_ERROR_BASE  0xE0001000
 /// 0xE0000000 required to signal error (bit 29 set indicates application error
 //  1000 as to not overlap with existing OS errors.
# define PIKA_IOCTL_ERROR(code) PIKA_IOCTL_ERROR_BASE + code
*/
#elif defined(__linux__)

# define PK_HANDLE                      int

/* ioctl command encoding: 32 bits total, command in lower 16 bits, size
 * of the parameter structure in the lower 14 bits of the upper 16 bits.
 *
 * Encoding the size of the parameter structure in the ioctl request is
 * useful for catching programs compiled with old versions and to avoid
 * overwriting user space outside the user buffer area.
 *
 * The highest 2 bits are reserved for indicating the ``access mode''.
 * NOTE: This limits the max parameter size to 16kB -1 !
 */
# define HSP_TYPE                       0xEA
# define HMP_TYPE                       0xEB

# define PIKA_IOCTL(type, code)         _IOC(_IOC_READ|_IOC_WRITE, type, code, 0)
# define PIKA_IOCTL_RDONLY(type, code)  _IOC(_IOC_READ, type, code, 0)

# define PIKA_IOCTL_ERROR_BASE  135
# define PIKA_IOCTL_ERROR(code) -(PIKA_IOCTL_ERROR_BASE + code)

#else
# error OS not supported!
#endif  // _WIN32


/* HSP IOCTL Commands - General
 */
#define IOCTL_HSP_GET_INFO                      PIKA_IOCTL(HSP_TYPE, 0x00)
#define IOCTL_HSP_SET_DEBUG_TRACE               PIKA_IOCTL(HSP_TYPE, 0x01)
#define IOCTL_HSP_REPORT_REAL_TIME_PERFORMANCE  PIKA_IOCTL(HSP_TYPE, 0x02)
#define IOCTL_HSP_GET_VERSION_INFO              PIKA_IOCTL(HSP_TYPE, 0x03)

/* HSP IOCTL Commands - Kernel
 */
#define IOCTL_HSP_CREATE_KERNEL                 PIKA_IOCTL(HSP_TYPE, 0x10)
#define IOCTL_HSP_DESTROY_KERNEL                PIKA_IOCTL(HSP_TYPE, 0x11)

/* HSP IOCTL Commands - Board
 */
#define IOCTL_HSP_CREATE_BOARD                  PIKA_IOCTL(HSP_TYPE, 0x12)
#define IOCTL_HSP_DESTROY_BOARD                 PIKA_IOCTL(HSP_TYPE, 0x13)
// #define IOCTL_HSP_START_BOARD                   PIKA_IOCTL(HSP_TYPE, 0x14)
// #define IOCTL_HSP_STOP_BOARD                    PIKA_IOCTL(HSP_TYPE, 0x15)

/* HSP IOCTL Commands - Memory
 */
#define IOCTL_HSP_ALLOC_MEMORY                  PIKA_IOCTL(HSP_TYPE, 0x16)
#define IOCTL_HSP_FREE_MEMORY                   PIKA_IOCTL(HSP_TYPE, 0x17)
#define IOCTL_HSP_GET_DATA                      PIKA_IOCTL(HSP_TYPE, 0x18)
#define IOCTL_HSP_SET_DATA                      PIKA_IOCTL(HSP_TYPE, 0x19)

/* HSP IOCTL Commands - Thread
 */
#define IOCTL_HSP_GET_MAILBOX                   PIKA_IOCTL(HSP_TYPE, 0x22)

/* HSP IOCTL Commands - Application
 */
#define IOCTL_HSP_CREATE_APPLICATION            PIKA_IOCTL(HSP_TYPE, 0x30)
#define IOCTL_HSP_DESTROY_APPLICATION           PIKA_IOCTL(HSP_TYPE, 0x31)

/* HSP IOCTL Commands - Mediastream
 */
#define IOCTL_HSP_CREATE_MEDIASTREAM            PIKA_IOCTL(HSP_TYPE, 0x32)
#define IOCTL_HSP_DESTROY_MEDIASTREAM           PIKA_IOCTL(HSP_TYPE, 0x33)
#define IOCTL_HSP_MEDIASTREAM_SET_FORMAT        PIKA_IOCTL(HSP_TYPE, 0x34)
#define IOCTL_HSP_MEDIASTREAM_SET_PREFILLVALUE  PIKA_IOCTL(HSP_TYPE, 0x35)

/* HSP IOCTL Commands - Process
 */
#define IOCTL_HSP_CREATE_PROCESS                PIKA_IOCTL(HSP_TYPE, 0x40)
#define IOCTL_HSP_DESTROY_PROCESS               PIKA_IOCTL(HSP_TYPE, 0x41)
#define IOCTL_HSP_START_PROCESS                 PIKA_IOCTL(HSP_TYPE, 0x42)
#define IOCTL_HSP_STOP_PROCESS                  PIKA_IOCTL(HSP_TYPE, 0x43)
#define IOCTL_HSP_SEND_MESSAGE_TO_PROCESS       PIKA_IOCTL(HSP_TYPE, 0x44)

/* HSP IOCTL Commands - Switching
 */
#define IOCTL_HSP_HALFDUPLEXCONNECT             PIKA_IOCTL(HSP_TYPE, 0x50)
#define IOCTL_HSP_HALFDUPLEXDISCONNECT          PIKA_IOCTL(HSP_TYPE, 0x51)

/* HSP IOCTL Commands - Conferencing
 */
#define IOCTL_HSP_CONFERENCE_CREATE             PIKA_IOCTL(HSP_TYPE, 0x60)
#define IOCTL_HSP_CONFERENCE_SET_CONFIG         PIKA_IOCTL(HSP_TYPE, 0x61)
#define IOCTL_HSP_CONFERENCE_DESTROY            PIKA_IOCTL(HSP_TYPE, 0x62)
#define IOCTL_HSP_CONFERENCE_ADD_OR_UPDATE      PIKA_IOCTL(HSP_TYPE, 0x63)
#define IOCTL_HSP_CONFERENCE_REMOVE             PIKA_IOCTL(HSP_TYPE, 0x64)

/* HSP IOCTL Commands - Media Bridge
 */
#define IOCTL_HSP_MEDIA_BRIDGE_CREATE           PIKA_IOCTL(HSP_TYPE, 0x70)
#define IOCTL_HSP_MEDIA_BRIDGE_DESTROY          PIKA_IOCTL(HSP_TYPE, 0x71)
#define IOCTL_HSP_MEDIA_BRIDGE_START            PIKA_IOCTL(HSP_TYPE, 0x72)
#define IOCTL_HSP_MEDIA_BRIDGE_STOP             PIKA_IOCTL(HSP_TYPE, 0x73)

/* HSP IOCTL Commands - Mips Pool
 */
#define IOCTL_HSP_CREATE_MIPS_POOL              PIKA_IOCTL(HSP_TYPE, 0x80)

/* -------------------------------------------------------------------- */

/* HMP Card Commands */
#define IOCTL_HMPCARD_GET_INFO                  PIKA_IOCTL(HMP_TYPE, 0x00)
#define IOCTL_HMPCARD_HSP_INFO                  PIKA_IOCTL(HMP_TYPE, 0x01)
#define IOCTL_HMPCARD_CONFIGURE                 PIKA_IOCTL(HMP_TYPE, 0x02)
#define IOCTL_HMPCARD_RESET                     PIKA_IOCTL(HMP_TYPE, 0x03)
#define IOCTL_HMPCARD_ATTACH                    PIKA_IOCTL(HMP_TYPE, 0x04)
#define IOCTL_HMPCARD_START                     PIKA_IOCTL(HMP_TYPE, 0x05)
#define IOCTL_HMPCARD_STOP                      PIKA_IOCTL(HMP_TYPE, 0x06)
#define IOCTL_HMPCARD_DETACH                    PIKA_IOCTL(HMP_TYPE, 0x07)
#define IOCTL_HMPCARD_RESTART                   PIKA_IOCTL(HMP_TYPE, 0x08)
#define IOCTL_HMPCARD_SET_HSP                   PIKA_IOCTL(HMP_TYPE, 0x09)
#define IOCTL_HMPCARD_UNSET_HSP                 PIKA_IOCTL(HMP_TYPE, 0x0A)
#define IOCTL_HMPCARD_GET_DIAG                  PIKA_IOCTL(HMP_TYPE, 0x0B)
#define IOCTL_HMPCARD_SET_DIAG                  PIKA_IOCTL(HMP_TYPE, 0x0C)
#define IOCTL_HMPCARD_SET_DBG                   PIKA_IOCTL(HMP_TYPE, 0x0D)
#define IOCTL_HMPCARD_MEMACCESS                 PIKA_IOCTL(HMP_TYPE, 0x0E)

/* HMP Analog Trunk Commands */
#define IOCTL_TRUNK_START                       PIKA_IOCTL(HMP_TYPE, 0x10)
#define IOCTL_TRUNK_STOP                        PIKA_IOCTL(HMP_TYPE, 0x11)
#define IOCTL_TRUNK_HOOK_SWITCH                 PIKA_IOCTL(HMP_TYPE, 0x12)
#define IOCTL_TRUNK_GET_INFO                    PIKA_IOCTL(HMP_TYPE, 0x13)
#define IOCTL_TRUNK_DIAL                        PIKA_IOCTL(HMP_TYPE, 0x14)
#define IOCTL_TRUNK_SET_DETECTION               PIKA_IOCTL(HMP_TYPE, 0x15)
#define IOCTL_TRUNK_SET_THRESHOLD               PIKA_IOCTL(HMP_TYPE, 0x16)
#define IOCTL_TRUNK_SET_AUDIO                   PIKA_IOCTL(HMP_TYPE, 0x17)

#define IOCTL_LINE_MEMACCESS                    PIKA_IOCTL(HMP_TYPE, 0x18)
#define IOCTL_LINE_STATE                        PIKA_IOCTL(HMP_TYPE, 0x19)

#define IOCTL_PHONE_RING_START                  PIKA_IOCTL(HMP_TYPE, 0x20)
#define IOCTL_PHONE_RING_STOP                   PIKA_IOCTL(HMP_TYPE, 0x21)
#define IOCTL_PHONE_REVERSAL                    PIKA_IOCTL(HMP_TYPE, 0x22)
#define IOCTL_PHONE_DISCONNECT                  PIKA_IOCTL(HMP_TYPE, 0x23)
#define IOCTL_PHONE_RING_PATTERN                PIKA_IOCTL(HMP_TYPE, 0X24)
#define IOCTL_PHONE_START                       PIKA_IOCTL(HMP_TYPE, 0x25)
#define IOCTL_PHONE_STOP                        PIKA_IOCTL(HMP_TYPE, 0x26)
#define IOCTL_PHONE_ONHOOK_TX                   PIKA_IOCTL(HMP_TYPE, 0x27)
#define IOCTL_PHONE_SET_AUDIO                   PIKA_IOCTL(HMP_TYPE, 0x28)
#define IOCTL_PHONE_GET_INFO                    PIKA_IOCTL(HMP_TYPE, 0x29)
#define IOCTL_PHONE_GET_DIAG                    PIKA_IOCTL(HMP_TYPE, 0x2A)
#define IOCTL_PHONE_SET_DIAG                    PIKA_IOCTL(HMP_TYPE, 0x2B)

#define IOCTL_AUDIO_START                       PIKA_IOCTL(HMP_TYPE, 0x30)
#define IOCTL_AUDIO_STOP                        PIKA_IOCTL(HMP_TYPE, 0x31)
#define IOCTL_AUDIO_GAIN                        PIKA_IOCTL(HMP_TYPE, 0x32)

#define IOCTL_SPAN_CONFIGURE                    PIKA_IOCTL(HMP_TYPE, 0x40)
#define IOCTL_SPAN_RESET                        PIKA_IOCTL(HMP_TYPE, 0x41)
#define IOCTL_SPAN_START                        PIKA_IOCTL(HMP_TYPE, 0x42)
#define IOCTL_SPAN_STOP                         PIKA_IOCTL(HMP_TYPE, 0x43)
#define IOCTL_SPAN_GET_ABCD                     PIKA_IOCTL(HMP_TYPE, 0x44)
#define IOCTL_SPAN_SET_ABCD                     PIKA_IOCTL(HMP_TYPE, 0x45)
#define IOCTL_SPAN_GET_ALARMS                   PIKA_IOCTL(HMP_TYPE, 0x46)
#define IOCTL_SPAN_ACTIVATE_CHANNEL             PIKA_IOCTL(HMP_TYPE, 0x47)
#define IOCTL_SPAN_WAKEUP                       PIKA_IOCTL(HMP_TYPE, 0x48)
#define IOCTL_SPAN_SLEEP                        PIKA_IOCTL(HMP_TYPE, 0x49)

#define IOCTL_HDLC_CONFIGURE                    PIKA_IOCTL(HMP_TYPE, 0x50)
#define IOCTL_HDLC_START                        PIKA_IOCTL(HMP_TYPE, 0x52)
#define IOCTL_HDLC_STOP                         PIKA_IOCTL(HMP_TYPE, 0x53)
#define IOCTL_HDLC_SEND_ABORT                   PIKA_IOCTL(HMP_TYPE, 0x54)
#define IOCTL_HDLC_STOP_REPEAT                  PIKA_IOCTL(HMP_TYPE, 0x55)
#define IOCTL_HDLC_RECEIVE_NEXT                 PIKA_IOCTL(HMP_TYPE, 0x56)

#define IOCTL_GSM_SET_SIM                       PIKA_IOCTL(HMP_TYPE, 0x57)

/* This is reserved for SAM */
#define IOCTL_SAM                               PIKA_IOCTL(HMP_TYPE, 0x72)

/* -------------------------------------------------------------------- */
// IOCTL errors - Analog Gateway Card
// 0 is Success

// This is unfortunately.  There is no easy way to translate custome NT_STATUS
// errors from KM to UM in Windows, so we must use existing OS errors
// and interpret them in our way, hoping to pick ones that won't actually come up.
//
// In addition, groups of NT_STATUS errors map to the same Win32 error, so watch out!
#ifdef _WIN32
// KM error codes
#define IOCTL_ERROR_INVALID_PARAM               STATUS_INVALID_PARAMETER
#define IOCTL_ERROR_RINGGEN_BUSY                STATUS_DEVICE_BUSY
#define IOCTL_ERROR_INVALID_LINE_STATE          STATUS_INVALID_SERVER_STATE

// UM error codes
#define IOCTL_UM_ERROR_INVALID_PARAM            ERROR_INVALID_PARAMETER
#define IOCTL_UM_ERROR_RINGGEN_BUSY             ERROR_BUSY
#define IOCTL_UM_ERROR_INVALID_LINE_STATE       ERROR_INVALID_SERVER_STATE

#else
#define IOCTL_ERROR_INVALID_PARAM               PIKA_IOCTL_ERROR(1)
#define IOCTL_ERROR_RINGGEN_BUSY                PIKA_IOCTL_ERROR(2)
#define IOCTL_ERROR_INVALID_LINE_STATE          PIKA_IOCTL_ERROR(3)

#define IOCTL_UM_ERROR_INVALID_PARAM            IOCTL_ERROR_INVALID_PARAM
#define IOCTL_UM_ERROR_RINGGEN_BUSY             IOCTL_ERROR_RINGGEN_BUSY
#define IOCTL_UM_ERROR_INVALID_LINE_STATE       IOCTL_ERROR_INVALID_LINE_STATE

#endif

/* Used for peek/poke. For debugging only. It needs to be here since
 * this file is "clean" of pika includes */

struct memaccess {
  unsigned type :  1;
#define MEMACCESS_PEEK 0
#define MEMACCESS_POKE 1
  unsigned reg  : 31;
  unsigned val;
};

#endif  /* PK_IOCTL_H */
