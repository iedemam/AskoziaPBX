/* These should be in pk_debug.h but have to be split out because of
 * um_hsp.
 */

/* Debug Trace Levels */

/* Do not change these without changing g_LevelStr in the hsp
 * driver! */
#define DBGT_ERROR_LEVEL         0x00
#define DBGT_WARNING_LEVEL       0x10
#define DBGT_TRACE_MAJOR_LEVEL   0x20
#define DBGT_TRACE_MINOR_LEVEL   0x30
#define DBGT_TRACE_NORMAL_LEVEL  0x40
#define DBGT_INFO_MAJOR_LEVEL    0x50
#define DBGT_INFO_MINOR_LEVEL    0x60
#define DBGT_INFO_NORMAL_LEVEL   0x70

/* Debug Bitmasks */

/* Do not change these without changing the module string array in
 * hspkernel.c */
#define DBGT_DRIVER_ID           8
#define DBGT_HSPDEV_ID           9
#define DBGT_HSPCFG_ID          10
#define DBGT_HSP_ID             11
#define DBGT_HSP_CLKSYNC_ID     12
#define DBGT_HSP_PROCESS_ID     13
#define DBGT_HSP_MEDIASTREAM_ID 14
#define DBGT_HSP_CONF_SWITCH_ID 15
#define DBGT_HSP_SCHEDULER_ID   16
#define DBGT_HSP_HW_SIM_ID      17

/* Daytona bitmasks */
#define DBGT_BRD_CLOCKING       24

/* Digital bitmasks */
#define DBGT_FRAMER             25
#define DBGT_HDLC               26

/* GSM bit masks */
#define DBGT_GSM_DATA		28
