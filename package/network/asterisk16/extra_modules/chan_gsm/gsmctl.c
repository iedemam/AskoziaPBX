#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>

#include "gsmctl.h"

/* todo : move them to gsmctl.h */
#define GSM_LINE_LENGTH 	4096
#define DEFAULT_WAIT4MOD 	10
#define DEFAULT_GSM_SEND_DELAY 	0
#define GSM_CMD_QUEUE_KEY	0xBEEF0000
#define LINE_BUFFER_SIZE	4096
#define GSM_WAIT_TIMEOUT	10
#define GSM_CMD_LENGTH		255
#define INVALID_TIMER_ID	(timer_t)-1

#define GSM_TX_GAIN_MIN		-12
#define GSM_TX_GAIN_MAX		24
#define GSM_TX_GAIN_DEFAULT	0

#define GSM_RX_GAIN_MIN		-18
#define GSM_RX_GAIN_MAX		10
#define GSM_RX_GAIN_DEFAULT	0

/* when DEFAULT_GSM_SEND_DELAY is defined, all delays after sending a messsage will
 * be at this value in milliseconds. */

static int debug = 0; /* SAM DBG */
FILE *debugfp = NULL;

#if defined(__PPC__) && defined(TEST_GSM)
#define gsm_log(type, format, args...) printf(format, ## args)
#else
#include <asterisk.h>
#include <asterisk/logger.h>
#endif

enum ctlport_state {
	CTLPORT_STATE_NONE=0,
	CTLPORT_STATE_SMS,
	CTLPORT_STATE_KILLME,
	CTLPORT_STATE_KILLED,
};

enum modem_state {
	MODEM_OK=0,
	MODEM_WAIT_FOR_RESPONSE,
	MODEM_RECEIVED_RESPONSE
};

/* gsm command type */
struct gsm_cmd {
	int mtype;
	char cmd_msg[GSM_CMD_LENGTH+1];
};

#define IMEI_SZ	32
#define IMSI_SZ 32
#define FW_REV_SZ 128	
struct ctlport {
	FILE *fpw,*fpr;
	pthread_t t;
	pthread_t wt;
	int fd;
// SAM	char *initfile;
	int port;
	int in_use;
	struct gsm_config *cfg;
	enum ctlport_state state;
	enum modem_state modem_state;
	void *priv;
	int write_mq_id;
	int msg_count;
	struct gsm_cmd last_cmd_sent;
	char imei[IMEI_SZ+1];
// Only the SIM currently connected to the radio is queried.
	char imsi[IMSI_SZ+1]; 
	char fw_rev_info[FW_REV_SZ+1];
};


typedef struct gain_tbl_entry {
  int table_value;
  int codec_value;
  int radio_value;
} gain_tbl_entry_t;

const static gain_tbl_entry_t gsm_rx_gain_table[] = {
	{ .table_value =  10,  .codec_value = 0x00, .radio_value = 100 },
	{ .table_value =   9,  .codec_value = 0x00, .radio_value = 98 },
        { .table_value =   8,  .codec_value = 0x00, .radio_value = 95 },
        { .table_value =   7,  .codec_value = 0x00, .radio_value = 93 },
        { .table_value =   6,  .codec_value = 0x00, .radio_value = 90 },
        { .table_value =   5,  .codec_value = 0x01, .radio_value = 90 },
        { .table_value =   4,  .codec_value = 0x02, .radio_value = 90 },
        { .table_value =   3,  .codec_value = 0x03, .radio_value = 90 },
        { .table_value =   2,  .codec_value = 0x04, .radio_value = 90 },
        { .table_value =   1,  .codec_value = 0x05, .radio_value = 90 },
        { .table_value =   0,  .codec_value = 0x06, .radio_value = 90 },
        { .table_value =  -1,  .codec_value = 0x07, .radio_value = 90 },
        { .table_value =  -2,  .codec_value = 0x08, .radio_value = 90 },
        { .table_value =  -3,  .codec_value = 0x09, .radio_value = 90 },
        { .table_value =  -4,  .codec_value = 0x0A, .radio_value = 90 },
        { .table_value =  -5,  .codec_value = 0x0B, .radio_value = 90 },
        { .table_value =  -6,  .codec_value = 0x0C, .radio_value = 90 },
        { .table_value =  -7,  .codec_value = 0x0D, .radio_value = 90 },
        { .table_value =  -8,  .codec_value = 0x0E, .radio_value = 90 },
        { .table_value =  -9,  .codec_value = 0x0F, .radio_value = 90 },
        { .table_value = -10,  .codec_value = 0x10, .radio_value = 90 },
        { .table_value = -11,  .codec_value = 0x11, .radio_value = 90 },
        { .table_value = -12,  .codec_value = 0x12, .radio_value = 90 },
        { .table_value = -13,  .codec_value = 0x13, .radio_value = 90 },
        { .table_value = -14,  .codec_value = 0x14, .radio_value = 90 },
        { .table_value = -15,  .codec_value = 0x15, .radio_value = 90 },
        { .table_value = -16,  .codec_value = 0x16, .radio_value = 90 },
        { .table_value = -17,  .codec_value = 0x17, .radio_value = 90 },
        { .table_value = -18,  .codec_value = 0x18, .radio_value = 90 },
};

const static gain_tbl_entry_t gsm_tx_gain_table[] = {
	{ .table_value =  24, .codec_value = 0x00, .radio_value = 8 },
	{ .table_value =  23, .codec_value = 0x01, .radio_value = 8 },
	{ .table_value =  22, .codec_value = 0x02, .radio_value = 8 },
	{ .table_value =  21, .codec_value = 0x03, .radio_value = 8 },
	{ .table_value =  20, .codec_value = 0x04, .radio_value = 8 },
	{ .table_value =  19, .codec_value = 0x05, .radio_value = 8 },
	{ .table_value =  18, .codec_value = 0x06, .radio_value = 8 },
	{ .table_value =  17, .codec_value = 0x07, .radio_value = 8 },
	{ .table_value =  16, .codec_value = 0x08, .radio_value = 8 },
	{ .table_value =  15, .codec_value = 0x09, .radio_value = 8 },
	{ .table_value =  14, .codec_value = 0x0A, .radio_value = 8 },
	{ .table_value =  13, .codec_value = 0x0B, .radio_value = 8 },
	{ .table_value =  12, .codec_value = 0x0C, .radio_value = 8 },
	{ .table_value =  11, .codec_value = 0x0D, .radio_value = 8 },
	{ .table_value =  10, .codec_value = 0x0E, .radio_value = 8 },
	{ .table_value =   9, .codec_value = 0x0F, .radio_value = 8 },
	{ .table_value =   8, .codec_value = 0x10, .radio_value = 8 },
	{ .table_value =   7, .codec_value = 0x11, .radio_value = 8 },
	{ .table_value =   6, .codec_value = 0x12, .radio_value = 8 },
	{ .table_value =   5, .codec_value = 0x13, .radio_value = 8 },
	{ .table_value =   4, .codec_value = 0x14, .radio_value = 8 },
	{ .table_value =   3, .codec_value = 0x15, .radio_value = 8 },
	{ .table_value =   2, .codec_value = 0x16, .radio_value = 8 },
	{ .table_value =   1, .codec_value = 0x17, .radio_value = 8 },
	{ .table_value =   0, .codec_value = 0x18, .radio_value = 8 },
	{ .table_value =  -1, .codec_value = 0x18, .radio_value = 7 },
	{ .table_value =  -2, .codec_value = 0x18, .radio_value = 7 },
	{ .table_value =  -3, .codec_value = 0x18, .radio_value = 6 },
	{ .table_value =  -4, .codec_value = 0x18, .radio_value = 5 },
	{ .table_value =  -5, .codec_value = 0x18, .radio_value = 5 },
	{ .table_value =  -6, .codec_value = 0x18, .radio_value = 4 },
	{ .table_value =  -7, .codec_value = 0x18, .radio_value = 3 },
	{ .table_value =  -8, .codec_value = 0x18, .radio_value = 3 },
	{ .table_value =  -9, .codec_value = 0x18, .radio_value = 2 },
	{ .table_value = -10, .codec_value = 0x18, .radio_value = 1 },
	{ .table_value = -11, .codec_value = 0x18, .radio_value = 1 },
	{ .table_value = -12, .codec_value = 0x18, .radio_value = 0 },
};




/* TODO : tuck all semaphores into ctlport structure */
sem_t ready[MAX_GSM_PORTS];
sem_t write_sem[MAX_GSM_PORTS];
sem_t avail_sem[MAX_GSM_PORTS];
timer_t wdog_timer[MAX_GSM_PORTS];

struct ctlport ctlport[MAX_GSM_PORTS];

typedef struct error_code {
	int code;
	const char *message;
} error_code_t;


const error_code_t cme_error_codes[] = {
	{ CME_ERROR_PHONE_FAILURE, "phone failure" },
	{ CME_ERROR_NO_PHONE_CONNECTION, "no connection to phone" },
	{ CME_ERROR_PHONE_ADAPTER_LINK_RESERVED, "phone-adaptor link reserved" },
	{ CME_ERROR_OPERATION_NOT_ALLOWED, "operation not allowed" },
	{ CME_ERROR_OPERATION_NOT_SUPPORTED, "operation not supported" },
	{ CME_ERROR_PH_SIM_PIN_REQUIRED, "ph-sim pin required" },
	{ CME_ERROR_PH_FSIM_PIN_REQUIRED, "ph-fsim pin required" },
	{ CME_ERROR_PH_FSIM_PUK_REQUIRED, "ph-fsim puk required" },
	{ CME_ERROR_SIM_NOT_INSERTED, "sim not inserted" },
	{ CME_ERROR_SIM_PIN_REQUIRED, "sim pin required" },
	{ CME_ERROR_SIM_PUK_REQUIRED, "sim puk required" },
	{ CME_ERROR_SIM_FAILURE, "sim failure" },
	{ CME_ERROR_SIM_BUSY, "sim busy" },
	{ CME_ERROR_SIM_WRONG, "sim wrong" },
	{ CME_ERROR_INCORRECT_PASSWORD_PIN, "incorrect password/pin" },
	{ CME_ERROR_SIM_PIN2_REQUIRED, "sim pin2 required" },
	{ CME_ERROR_SIM_PUK2_REQUIRED, "sim puk2 required" },
	{ CME_ERROR_MEMORY_FULL, "memory full" },
	{ CME_ERROR_INVALID_INDEX, "invalid index" },
	{ CME_ERROR_NOT_FOUND, "not found" },
	{ CME_ERROR_MEMORY_FAILURE, "memory failure" },
	{ CME_ERROR_TEXT_STRING_TOO_LONG, "text string too long" },
	{ CME_ERROR_INVALID_CHARS_IN_TEXT_STRING, "invalid characters in text string" },
	{ CME_ERROR_DIAL_STRING_TOO_LONG, "dial string too long" },
	{ CME_ERROR_INVALID_CHARS_IN_DIALSTRING, "invalid characters in dial string" },
	{ CME_ERROR_NO_NETWORK_SERVICE, "no network service" },
	{ CME_ERROR_NETWORK_TIMEOUT, "network timeout" },
	{ CME_ERROR_EMERGENCY_CALLS_ONLY, "network not allowed - emergency calls only" },
	{ CME_ERROR_NETWORK_PERS_PIN_REQUIRED, "network personalization PIN required" },
	{ CME_ERROR_NETWORK_PERS_PUK_REQUIRED, "network personalization PUK required" },
	{ CME_ERROR_NETWORK_SUBS_PERS_PIN_REQUIRED, "network subset personalization PIN required" },
	{ CME_ERROR_SERVICE_PROVIDER_PERS_PIN_REQUIRED, "service provider personalization PIN required" },
	{ CME_ERROR_SERVICE_PROVIDER_PERS_PUK_REQUIRED, "service provider personalization PUK required" },
	{ CME_ERROR_CORPORATE_PERS_PIN_REQUIRED, "corporate personalization PIN required" },
	{ CME_ERROR_CORPORATE_PERS_PUK_REQUIRED, "corporate personalization PUK required" },
	{ CME_ERROR_UNKNOWN, "unknown" },
	{ CME_ERROR_ILLEGAL_MS, "illegal MS" },
	{ CME_ERROR_ILLEGAL_ME, "illegal ME" }, 
	{ CME_ERROR_GRPS_SERVICES_NOT_ALLOWED, "GPRS services not allowed" },
	{ CME_ERROR_PLMN_NOT_ALLOWED, "PLMN not allowed" },
	{ CME_ERROR_LOCATION_AREA_NOT_ALLOWED, "location area not allowed" },
	{ CME_ERROR_ROAMING_NOT_ALLOWED, "roaming not allowed in this location area" },
	{ CME_ERROR_SERVICE_OPTION_NOT_SUPPORTED, "service option not supported" },
	{ CME_ERROR_REQUESTED_SERVICE_OPTION_NOT_SUBSCRIBED, "requested service option not subscribed" },
	{ CME_ERROR_SERVICE_OPTION_TEMP_OUT_OF_ORDER, "service option temporarily out of order" },
	{ CME_ERROR_UNSPECIFIED_GPRS_ERROR, "unspecified GPRS error" },
	{ CME_ERROR_PDP_AUTHENTICATION_FAILURE, "PDP authentication failure" },
	{ CME_ERROR_INVALID_MOBILE_CLASS, "invalid mobile class" },
	{ CME_ERROR_AUDIO_MANAGER_NOT_READY, "audio manager not ready" },
	{ CME_ERROR_AUDIO_FORMAT_NOT_CONFIGURED, "audio format cannot be configured" },
	{ CME_ERROR_SIM_TOOLKIT_NOT_CONFIGURED, "sim toolkit menu has not been configured" },
	{ CME_ERROR_SIM_TOOLKIT_ALREADY_IN_USE, "sim toolkit already in use" },
	{ CME_ERROR_SIM_TOOLKIT_NOT_ENABLED, "sim toolkit not enabled" },
	{ CME_ERROR_CSCS_TYPE_NOT_SUPPORTED, "+CSCS type not supported" },
	{ CME_ERROR_CSCS_TYPE_NOT_FOUND, "CSCS type not found" },
	{ CME_ERROR_MUST_INCLUDE_FORMAT_OPER, "must include <format> with <oper>" },
	{ CME_ERROR_INCORRECT_OPER_FORMAT, "incorrect <oper> format" },
	{ CME_ERROR_OPER_LENGTH_TOO_LONG, "<oper> length too long" },
	{ CME_ERROR_SIM_FULL, "SIM full" },
	{ CME_ERROR_CANNOT_CHANGE_PLMN_LIST, "unable to change PLMN list" },
	{ CME_ERROR_NETWORK_OP_NOT_RECOGNIZED, "network operator not recognized" },
	{ CME_ERROR_INVALID_COMMAND_LENGTH, "invalid command length" },
	{ CME_ERROR_INVALID_INPUT_STRING, "invalid input string" },
	{ CME_ERROR_INVALID_SIM_COMMAND, "invalid SIM command" },
	{ CME_ERROR_INVALID_FILE_ID, "invalid file id" },
	{ CME_ERROR_MISSING_P123_PARAMETER, "missing required P1/2/3 parameter" },
	{ CME_ERROR_INVALID_P123_PARAMETER, "invalid P1/2/3 parameter" },
	{ CME_ERROR_MISSING_REQD_COMMAND_DATA, "missing required command data" },
	{ CME_ERROR_INVALID_CHARS_IN_COMMAND_DATA, "invalid characters in command data" },
	{ CME_ERROR_INVALID_INPUT_VALUE, "invalid input value" },
	{ CME_ERROR_UNSUPPORTED_VALUE_OR_MODE, "unsupported value or mode" },
	{ CME_ERROR_OPERATION_FAILED, "operation failed" },
	{ CME_ERROR_MUX_ALREADY_ACTIVE, "multiplexer already active" },
	{ CME_ERROR_SIM_INVALID_NETWORK_REJECT, "sim invalid - network reject" },
	{ CME_ERROR_CALL_SETUP_IN_PROGRESS, "call setup in progress" },
	{ CME_ERROR_SIM_POWERED_DOWN, "sim powered down" },
	{ CME_ERROR_SIM_FILE_NOT_PRESENT, "sim file not present" }
};


const error_code_t cms_error_codes[] = {
	{ CMS_ERROR_ME_FAILURE, "ME failure" },
	{ CMS_ERROR_SMS_ME_RESERVED, "SMS ME reserved" },
	{ CMS_ERROR_OPERATION_NOT_ALLOWED, "operation not allowed" },
	{ CMS_ERROR_OPERATION_NOT_SUPPORTED, "operation not supported" },
	{ CMS_ERROR_INVALID_PDU_MODE, "invalid PDU mode" },
	{ CMS_ERROR_INVALID_TEXT_MODE, "invalid text mode" },
	{ CMS_ERROR_SIM_NOT_INSERTED, "SIM not inserted" },
	{ CMS_ERROR_SIM_PIN_NECESSARY, "SIM PIN necessary" },
	{ CMS_ERROR_PH_SIN_PIN_NECESSARY, "PH SIM pin necessary" },
	{ CMS_ERROR_SIM_FAILURE, "SIM failure" },
	{ CMS_ERROR_SIM_BUSY, "SIM busy" },
	{ CMS_ERROR_SIM_WRONG, "SIM wrong" },
	{ CMS_ERROR_SIM_PUK_REQUIRED, "SIM PUK required" },
	{ CMS_ERROR_SIM_PIN2_REQUIRED, "SIM PIN2 required" },
	{ CMS_ERROR_SIM_PUK2_REQUIRED, "SIM PUK2 required" },
	{ CMS_ERROR_MEMORY_FAILURE, "memory failure" },
	{ CMS_ERROR_INVALID_MEMORY_INDEX, "invalid memory index" },
	{ CMS_ERROR_MEMORY_FULL, "memory full" },
	{ CMS_ERROR_SMSC_ADDRESS_UNKNOWN, "SMSC address unknown" },
	{ CMS_ERROR_NO_NETWORK, "no network" },
	{ CMS_ERROR_NETWORK_TIMEOUT, "network timeout" },
	{ CMS_ERROR_UNKNOWN, "unknown" },
	{ CMS_ERROR_SIM_NOT_READY, "SIM not ready" },
	{ CMS_ERROR_UNREAD_RECORDS_ON_SIM, "unread records on SIM" },
	{ CMS_ERROR_CB_ERROR_UNKNOWN, "CB error unknown" },
	{ CMS_ERROR_PS_BUSY, "PS busy" },
	{ CMS_ERROR_SIM_BL_NOT_READY, "SM BL not ready" },
	{ CMS_ERROR_INVALID_CHARS_IN_PDU, "Invalid (non-hex) chars in PDU" },
	{ CMS_ERROR_INCORRECT_PDU_LENGTH, "Incorrect PDU length" },
	{ CMS_ERROR_INVALID_MTI, "Invalid MTI" },
	{ CMS_ERROR_INVALID_CHAR_IN_ADDRESS, "Invalid (non-hex) chars in address" },
	{ CMS_ERROR_INVALID_ADDRESS, "Invalid address (no digits read)" },
	{ CMS_ERROR_INVALID_PDU_LENGTH, "Incorrect PDU length (UDL)" },
	{ CMS_ERROR_INCORRECT_SCA_LENGTH, "Incorrect SCA length" },
	{ CMS_ERROR_INVALID_FIRST_OCTECT, "Invalid first octet (should be 2 or 34)" },
	{ CMS_ERROR_INVALID_COMMAND_TYPE, "Invalid Command Type" },
	{ CMS_ERROR_SRR_BIT_NOT_SET, "SRR bit not set" },
	{ CMS_ERROR_SRR_BIT_SET, "SRR bit set" },
	{ CMS_ERROR_INVALID_USER_DATA_IN_IE, "Invalid User Data Header IE" }
};
	

void (*cbEvents)(int port, char *event);

void gsm_close_port(int port);

static int _gsm_send_command(int port, char *s, int delay);

/* we could merge the two functions into a common one */

const char *gsm_get_cms_text(int cms_code)
{
	const int cms_list_size = sizeof(cms_error_codes) / sizeof(error_code_t);
	int found = 0;
	int ii;

	for (ii = 0; ii < cms_list_size; ii++) {
		if (cms_error_codes[ii].code == cms_code) {
			found = 1;
			break;
		}
	}

	if (found) {
		return (&cms_error_codes[ii].message[0]);
	} else {
		return NULL;
	}
}	

const char *gsm_get_cme_text(int cme_code)
{
	const int cme_list_size = sizeof(cme_error_codes) / sizeof(error_code_t);
	int found = 0;
	int ii;

	for (ii = 0; ii < cme_list_size; ii++) {
		if (cme_error_codes[ii].code == cme_code) {
			found = 1;
			break;
		}
	}

	if (found) {
		return (&cme_error_codes[ii].message[0]);
	} else {
		return NULL;
	}
}


void gsm_set_debugfile(char *debugfile)
{

}

void gsm_set_debuglevel(int debuglevel)
{
	debug = debuglevel;
}

/**Port List helper*/
struct ctlport *getctlport(int port)
{
	int i;

	if (!port) return NULL;

	for (i=1; i<MAX_GSM_PORTS; i++) {
		if (ctlport[i].port == port) return &ctlport[i];
	}
	return NULL;
}

struct ctlport *getctlport_bypriv(void * priv)
{
	int i;

	if (!priv) return NULL;

	for (i=1; i<MAX_GSM_PORTS; i++) {
		if (ctlport[i].priv == priv) return &ctlport[i];
	}
	return NULL;
}

/* IF Functions */
int gsm_port_is_valid(int port)
{
	struct ctlport *ctl=getctlport(port);

	if (ctl)
		return 1;
	else
		return 0;
}

int gsm_port_fetch(int port)
{
	struct ctlport *ctl=getctlport(port);
	if (ctl) {
		if (!ctl->in_use) {
			ctl->in_use=1;
			return 1;
		}
	}
	return 0;
}

int gsm_port_release(int port)
{

	struct ctlport *ctl=getctlport(port);
	if (ctl) {
		ctl->in_use=0;
	}
	return 0;
}

int gsm_port_in_use_get(int port)
{
	struct ctlport *ctl=getctlport(port);
	if (ctl) {
		return ctl->in_use;
	}
	return 0;

}

int gsm_sms_enter(int port)
{
	struct ctlport *ctl=getctlport(port);

	ctl->state=CTLPORT_STATE_SMS;
	return 0;
}

int gsm_simslot_get(int port)
{
	char buf[16];
	char path[128];
	FILE *fp;

	sprintf(path,"/sys/class/gsm/ctl%d/channel_sim_slot",port);
	fp=fopen(path,"r");
	if (fp) {
		fgets(buf, sizeof(buf), fp);
		fclose(fp);
		return strtol(buf, NULL, 10);
	} else {
		printf("Could not open sysfs interface for port %d\n", port);
	}

	return 0;
}

int gsm_simslot_next_get(int port)
{
	char buf[16];
	char path[128];
	FILE *fp;

	sprintf(path,"/sys/class/gsm/ctl%d/channel_sim_slot_next",port);
	fp=fopen(path,"r");
	if (fp) {
		fgets(buf, sizeof(buf), fp);
		fclose(fp);
		return strtol(buf, NULL, 10);
	} else {
		printf("Could not open sysfs interface for port %d\n", port);
		return -1;
	}

	return 0;
}

int gsm_set_next_simslot(int port, int slot)
{
	char path[128];
	FILE *fp;

	if (gsm_simslot_get(port)==slot)
		return 0;

	sprintf(path,"/sys/class/gsm/ctl%d/channel_sim_slot_next",port);
	fp=fopen(path,"w");
	if (fp) {
		fprintf(fp,"%d", slot?1:0);
		fclose(fp);
		return 1;
	} else {
		printf("Could not open sysfs interface for port %d\n", port);
		return -1;
	}

	return 0;

}

int gsm_check_port(int port)
{
	char path[128];
	struct stat mstat;

	sprintf(path,"/dev/gsm/ctl%d",port);

	if (stat(path, &mstat) == 0) {
		return 1;
	} else {
		return 0;
	}
}

int gsm_set_led(int port,int ledstate)
{
	struct ctlport *ctl=getctlport(port);
	int fd;

      	if (!ctl || !ctl->fpw) return 0;

	fd=fileno(ctl->fpw);

	if (fd<=0) return 0;

	return ioctl( fd, GSM_SETLED, ledstate);
}

int gsm_audio_start(int port)
{
	struct ctlport *ctl=getctlport(port);
	if (!ctl) return -1;
      	return ioctl( ctl->fd, GSM_SETSTATE, STATE_CONNECTED);
}

int gsm_audio_stop(int port)
{
	struct ctlport *ctl=getctlport(port);
      	return ioctl( ctl->fd, GSM_SETSTATE, STATE_HANGUP);
}

int gsm_audio_open(int port)
{
	struct ctlport *ctl=getctlport(port);

	char devname[128];
	sprintf(devname, "/dev/gsm/port%d",port);
	ctl->fd=open(devname, O_NONBLOCK|O_RDWR);

	return ctl->fd;
}

int gsm_get_channel_mask(void)
{
	const char taco_proc_entry[] = "/proc/driver/taco";
	const char taco_fmt[] = "%*s %*s %[^/]%*1c%s";
	const int taco_param_count = 2;
	char mod1[GSM_MAX_STR];
	char mod2[GSM_MAX_STR];
	FILE *taco_fp;
	int result = 0;

	/* open the procfs entry */
	if  (!(taco_fp = fopen(taco_proc_entry, "r"))) {
		gsm_log(GSM_ERROR, "could not open %s\n", taco_proc_entry);
		result = -1;
		return (result);
	}

	/* parse the channel configuration */
	if (fscanf(taco_fp, taco_fmt, mod1, mod2) != taco_param_count) {
		fclose(taco_fp);
		result = -2;
		return (result);
	}


	/* close the file */
	fclose(taco_fp);

	/* print it to check */
	if (debug)
		gsm_log(GSM_DEBUG, "mod1=(%s) mod2=(%s)\n", mod1, mod2);

	/* we will need to extend this to single channel GSM modules */
	if (!strncmp(mod1, "GSM", strlen("GSM")))
		result |= 0x03;
	 
	if (!strncmp(mod2, "GSM", strlen("GSM")))
		result |= 0x30;

	return (result); 
}

int gsm_get_channel_offset(void)
{
	static int channel_mask = -1;
	int channel_offset = 0;

	if (channel_mask < 0) {
		channel_mask = gsm_get_channel_mask();
	}

	switch (channel_mask) {
		case 0x03:	/* module A only */
			channel_offset = 0;
			break;
		case 0x30:	/* module B only */
			channel_offset = 4;
			break;
		case 0x33:	/* modules A and B */
			channel_offset = 0;
			break;
		case 0x00:	/* no GSM modules */
			break;
		default:
			break;
	}

	return (channel_offset);
}

int gsm_translate_channel(int *result, int channel_id)
{
	const int lookup_table[] = { 1, 2, 5, 6 };
	int channel_offset = gsm_get_channel_offset();

	if ((channel_id >= 1) && (channel_id <= 4)) { 
		*result = lookup_table[channel_id - 1] + channel_offset;
		return 0;
	} else {
		return -1;
	}
}

void gsm_audio_close(int port, int fd)
{
	struct ctlport *ctl=getctlport(port);
	close(ctl->fd);
}

void *gsm_get_priv(int port)
{
	struct ctlport *ctl=getctlport(port);
	return ctl->priv;
}

void gsm_put_priv(int port, void *priv)
{
	struct ctlport *ctl=getctlport(port);
	ctl->priv=priv;
}

void gsm_post_next_msg(int port)
{
	struct ctlport *ctl=getctlport(port);
	int m_count;

	if (!gsm_port_is_valid(port)) {
		gsm_log(GSM_DEBUG, "port=(%d): %s() invalid port number\n",
			port, __FUNCTION__);
		return;
	}

	if (sem_getvalue(&avail_sem[ctl->port], &m_count)==0) {
		if (debug) { 
			struct timeval tv;
        		gettimeofday(&tv, NULL);
			gsm_log(GSM_DEBUG, "port=(%d): POSTING avail_sem(%d) secs(%ld) usecs(%ld)\n", ctl->port, m_count, (long)tv.tv_sec, (long)tv.tv_usec);
		}
		if (m_count == 0) {
			gsm_stop_check_timer(ctl->port);
			sem_post(&avail_sem[ctl->port]);
		}
	} else {
		struct timeval tv;
        	gettimeofday(&tv, NULL);
		gsm_log(GSM_ERROR, "port=(%d): gsm_post_next_msg() secs(%ld) usecs(%ld) could not read semaphore\n", ctl->port, (long)tv.tv_sec, (long)tv.tv_usec);
	}
}

void gsm_check_timer_expired(sigval_t arg)
{
	int port = arg.sival_int;
	struct timeval tv;

	if (!gsm_port_is_valid(port)) {
		gsm_log(GSM_ERROR, "port=(%d): %s() invalid port number\n",
			port, __FUNCTION__);
		return;
	}

	gettimeofday(&tv, NULL);

	gsm_log(GSM_ERROR, "port=(%d): chan_gsm port() TIMED OUT secs(%ld) usecs(%ld) last command (%s)\n", 
		port, (long)tv.tv_sec, (long)tv.tv_usec, gsm_get_last_command(port));

	gsm_post_next_msg(port);
}

void gsm_start_check_timer(int port)
{
	struct itimerspec ts;
	struct sigevent se;
	int status;

	if (!gsm_port_is_valid(port)) {
		gsm_log(GSM_ERROR, "port=(%d): %s() invalid port number\n",
			port, __FUNCTION__);
		return;
	}

	if (wdog_timer[port] == INVALID_TIMER_ID) {
		se.sigev_notify = SIGEV_THREAD;
		se.sigev_value.sival_int = port;
		se.sigev_notify_function = gsm_check_timer_expired;
		se.sigev_notify_attributes = NULL;

		if ((status = timer_create(CLOCK_REALTIME, &se, &wdog_timer[port])) < 0) {
			gsm_log(GSM_ERROR, "port=(%d) %s() failed timer_create()\n",
				port, __FUNCTION__);
			return;
		}

		if (debug)
			gsm_log(LOG_ERROR, "port=(%d) %s() created timer_id(%d)\n",
				port, __FUNCTION__, (int)wdog_timer[port]);
	}

	ts.it_value.tv_sec = GSM_WAIT_TIMEOUT;
	ts.it_value.tv_nsec = 0;
	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;

	if ((status = timer_settime(wdog_timer[port], 0, &ts, 0)) < 0) {
		gsm_log(GSM_ERROR, "port=(%d) %s() failed timer_settime()\n",
			port, __FUNCTION__);		
		return;
	}

	if (debug)
		gsm_log(GSM_DEBUG, "port=(%d) %s() started timer_id(%d) to expire in %d seconds.\n",
			port, __FUNCTION__, (int)wdog_timer[port], GSM_WAIT_TIMEOUT);
}

void gsm_stop_check_timer(int port)
{
	struct itimerspec ts;
	int status;

	if (!gsm_port_is_valid(port)) {
		gsm_log(GSM_ERROR, "port=(%d): %s() invalid port number\n",
			port, __FUNCTION__);
		return;
	}

	ts.it_value.tv_sec = 0;
	ts.it_value.tv_nsec = 0;
	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = 0;

	if (wdog_timer[port] == INVALID_TIMER_ID) {
		gsm_log(GSM_WARNING, "port=(%d) %s() already destroyed\n",
			port, __FUNCTION__);
		return;
	}

	if ((status = timer_settime(wdog_timer[port], 0, &ts, 0)) < 0) {
		gsm_log(GSM_ERROR, "port=(%d) %s() failed timer_settime()\n",
			port, __FUNCTION__);		
		return;
	}

	if (debug)
		gsm_log(GSM_DEBUG, "port=(%d) %s() stopped timer_id(%d)\n",
			port, __FUNCTION__, (int)wdog_timer[port]);
}

void gsm_destroy_check_timer(int port)
{
	if (!gsm_port_is_valid(port)) {
		gsm_log(GSM_ERROR, "port=(%d): %s() invalid port number\n",
			port, __FUNCTION__);
		return;
	}

	if (wdog_timer[port] == INVALID_TIMER_ID) {
		gsm_log(GSM_WARNING, "port=(%d) %s() already destroyed\n",
			port, __FUNCTION__);
		return;
	}

	gsm_stop_check_timer(port);

	if (debug)
		gsm_log(GSM_DEBUG, "port=(%d) %s() destroyed timer_id(%d)\n",
			port, __FUNCTION__, (int)wdog_timer[port]);

	timer_delete(wdog_timer[port]);
	wdog_timer[port] = INVALID_TIMER_ID;
}

void gsm_masq_priv(void * oldpriv,void * newpriv)
{
	struct ctlport * port = getctlport_bypriv(oldpriv);

	if (port) {
		port->priv = newpriv;
	}
}


int wait4mod(struct ctlport *ctl, fd_set *wfds, fd_set *rfds, int secs)
{
	int fd;
	struct timeval tv;
	int ret;

	if (!ctl->fpw || ! ctl->fpr) return -1;

	fd=fileno( wfds?ctl->fpw:ctl->fpr);

	tv.tv_sec= secs ; /*wait X seconds*/
	tv.tv_usec=0;

	if (fd<0) return -1;

	FD_ZERO(wfds?wfds:rfds);
	FD_SET(fd, wfds?wfds:rfds);

	ret=select(fd+1, rfds, wfds, NULL, &tv);

	if (ret==0) {
		if (debug) gsm_log(GSM_DEBUG, "port=(%d): select timed out in wait4mod\n", ctl->port);
		return -1;
	}

	if (ret<0) {
		gsm_log(GSM_ERROR, "port=(%d): select returned error in wait4mod, error(%s)\n", ctl->port, strerror(errno));
		return -2;
	}

	return 1;
}

int wait4mod_r(struct ctlport *ctl, int secs)
{
	fd_set fds;
	return wait4mod(ctl, NULL, &fds, secs);
}

int wait4mod_w(struct ctlport *ctl, int secs)
{
	fd_set fds;
	return wait4mod(ctl, &fds, NULL, secs);
}

int gsm_send_raw(int port, char *s, int delay)
{
	struct ctlport *ctl=getctlport(port);
	struct timeval tv;

	char send[strlen(s)+1];

	if (!ctl) return -1;

	if (wait4mod_w(ctl, DEFAULT_WAIT4MOD) <0) {
		gsm_close_port(ctl->port);
		return -1;
	}

	strcpy(send,s);

	char *p=strchr(send,'\n');
	if (p) *p='\0';

	if ((debug) && (strcmp("\r", send))) { 
        	gettimeofday(&tv, NULL);
		gsm_log(GSM_DEBUG, "port=(%d): seconds (%ld) microseconds (%ld) sending (%s)\n",ctl->port, (long)tv.tv_sec, (long)tv.tv_usec, send);
	}

	if (ctl->fpw) {
		fputs(send, ctl->fpw);
		fflush(ctl->fpw);
	} else {
		gsm_log(GSM_ERROR, "port=(%d): invalid write fp\n", port);
	}

	usleep(1000*delay);
	return 0;
}

/* put the message into the message queue to be sent by the writer thread at the right time */
int gsm_send(int port, char *s, int delay)
{
	struct gsm_cmd gsm_message;
	int msglen = sizeof(struct gsm_cmd) - sizeof(long);
	struct ctlport *p=getctlport(port);

	/* put the message into the outgoing message queue */
	memset(&gsm_message, 0, sizeof(gsm_message));
	gsm_message.mtype = 1;
	strcpy(gsm_message.cmd_msg, s);

	if (msgsnd(p->write_mq_id, (void*)&gsm_message, msglen, IPC_NOWAIT) < 0) {
		gsm_log(GSM_ERROR, "port=(%d) error queuing gsm command (%s) err(%s)\n",
			port, gsm_message.cmd_msg, strerror(errno));
		return -1;
	} else {
		p->modem_state = MODEM_WAIT_FOR_RESPONSE;
		if (debug)
			gsm_log(GSM_DEBUG, "port=(%d) queued command (%s) on port (%d)\n",
				port, gsm_message.cmd_msg, port);
	}

	/* post the semaphore to wake up the write task */
	sem_post(&write_sem[port]);

	return 0;
}

static int _gsm_send_command(int port, char *s, int delay)
{
	struct ctlport *p=getctlport(port);
	char msg[GSM_CMD_LENGTH+3];
	
	strncpy(p->last_cmd_sent.cmd_msg, s, GSM_CMD_LENGTH);

	memset(msg, 0, sizeof(msg));
	sprintf(msg, "%s\r\n", s);

#if defined(DEFAULT_GSM_SEND_DELAY)
	delay = DEFAULT_GSM_SEND_DELAY;
#endif
	gsm_send_raw(port,msg,delay);

	return 0;
}

int send_expect(struct ctlport *ctl, char *s, char *exp, int wait, char **resp, int delay, int waitsecs)
{
	char line[256];
	char *p=NULL;
	char send[strlen(s)+1];
	char *e;
	int ret;

	if (delay<=100) delay=100;

	if (strlen(s)) {

		strcpy(send,s);

		if (debug) gsm_log(GSM_DEBUG, "port=(%d): send_expect (%s) (%s) (%s)\n",ctl->port, s, send, exp);

		if (wait4mod_w(ctl, waitsecs) <0) {
			if (debug) gsm_log(GSM_DEBUG, "port=(%d): not writeable\n",ctl->port);
			gsm_close_port(ctl->port);
			return -1;
		}

		fputs(send, ctl->fpw);
		fputs("\r\n", ctl->fpw);
		fflush(ctl->fpw);

	}

	if ((ret=wait4mod_r(ctl, waitsecs)) <0) {
		if (debug) gsm_log(GSM_DEBUG, "port=(%d): not readable\n",ctl->port);
		if (ret == -2) gsm_close_port(ctl->port);
		return -1;
	}

	while(1) {
		line[0]=0;

		e=fgets(line, 255, ctl->fpr);

		if (!e) break;

		if (debug) gsm_log(GSM_DEBUG, "port=(%d): getline:%s\n",ctl->port, line);

		p=strchr(line, '\r');
		if (p)
			*p='\0';

		if (!strcasecmp(line, send)) {
			if (debug) gsm_log(GSM_DEBUG, "port=(%d): Ignoring Echo: %s\n",ctl->port, line);
		}
		else if (!strcasecmp(line, exp)) {
			if (debug) gsm_log(GSM_DEBUG, "port=(%d): send_expect got (%s)\n",ctl->port, exp);
			usleep(1000*delay);
			return 1;
		} else if (wait) {
			if (debug) gsm_log(GSM_DEBUG, "port=(%d): send_expect got (%s) waiting 4 (%s)\n",ctl->port, line,exp);
			if (strstr(line, "KILLME")) {
				if (debug) gsm_log(GSM_DEBUG, "port=(%d): send_expect got (KILLME)\n", ctl->port);
				return 0;
			} else 
				continue;

		} else {
			if (resp) {
				*resp=strdup(line);
			}

			if (debug) gsm_log(GSM_DEBUG, "port=(%d): expected (%s) got (%s)\n",ctl->port, exp,line);
			usleep(1000*delay);
			return 0;
		}
	}

	if (!e) {
		if (debug) gsm_log(GSM_DEBUG, "port=(%d): Connection closed\n", ctl->port);
		return -1;
	}

	return 0;
}

static void gsm_set_rx_gain(int port, struct gsm_config *gsm_cfg)
{
	struct ctlport *ctl = getctlport(port);
	char clvl_command[64];
	int fdw = (ctlport[port].fpw) ? fileno(ctlport[port].fpw) : -1;
	int rx_gain_idx;
	int ii;

	if (fdw < 0) {
		gsm_log(GSM_ERROR, "port=(%d): invalid port number.\n", port);
		return;
	}

	rx_gain_idx = (gsm_cfg) ? (gsm_cfg->rx_gain) : 0;

	if ((rx_gain_idx < GSM_RX_GAIN_MIN) || (rx_gain_idx > GSM_RX_GAIN_MAX)) {
		gsm_log(GSM_ERROR, "port=(%d) rx_gain %d is out of range, falling back to default.\n", port, rx_gain_idx);
		rx_gain_idx = 0;
	}

	for (ii = 0; ii < sizeof(gsm_rx_gain_table) / sizeof(gain_tbl_entry_t); ii++) {
		if (gsm_rx_gain_table[ii].table_value == rx_gain_idx) {
			snprintf(clvl_command, sizeof(clvl_command)-1, "AT+CLVL=%d",
				gsm_rx_gain_table[ii].radio_value);
			send_expect(ctl, clvl_command, "OK", 1, NULL, 0, DEFAULT_WAIT4MOD);
			/* rx|tx is other way around for codec */
			ioctl(fdw, GSM_SET_TX_GAIN, gsm_rx_gain_table[ii].codec_value);
			break;
		}
	}
}

static void gsm_set_tx_gain(int port, struct gsm_config *gsm_cfg)
{
	struct ctlport *ctl = getctlport(port);
	char cmic_command[64];
	int fdw = (ctlport[port].fpw) ? fileno(ctlport[port].fpw) : -1;
	int tx_gain_idx;
	int ii;

	if (fdw < 0) {
		gsm_log(GSM_ERROR, "port=(%d): invalid port number.\n", port);
		return;
	}

	tx_gain_idx = (gsm_cfg) ? (gsm_cfg->tx_gain) : 0;

	if ((tx_gain_idx < GSM_TX_GAIN_MIN) || (tx_gain_idx > GSM_TX_GAIN_MAX)) {
		gsm_log(GSM_ERROR, "port=(%d) tx_gain %d is out of range, falling back to default.\n", port, tx_gain_idx);
		tx_gain_idx = 0;
	}

	for (ii = 0; ii < sizeof(gsm_tx_gain_table) / sizeof(gain_tbl_entry_t); ii++) {
		if (gsm_tx_gain_table[ii].table_value == tx_gain_idx) {
			snprintf(cmic_command, sizeof(cmic_command)-1, "AT+CMIC=0,%d",
				gsm_tx_gain_table[ii].radio_value);
			send_expect(ctl, cmic_command, "OK", 1, NULL, 0, DEFAULT_WAIT4MOD);
			/* rx|tx is other way around for codec */
			ioctl(fdw, GSM_SET_RX_GAIN, gsm_tx_gain_table[ii].codec_value);
			break;
		}
	}
}


int init_mod(struct ctlport *ctl)
{
	char *resp=NULL;
	char buf[256];


	FILE *fp=NULL;

	if (ctl->cfg->initfile && strlen(ctl->cfg->initfile) ) {
		fp=fopen(ctl->cfg->initfile,"r");
		if (!fp)
			perror(ctl->cfg->initfile);
	}

	if (wait4mod_w(ctl, DEFAULT_WAIT4MOD)<0) {
		if (debug) 
			gsm_log(GSM_DEBUG, "wait4mod_w failed\n");
	}

	/* get the "call ready" message (unsolicited) */
	if (send_expect(ctl, "", "Call Ready", 0, NULL, 0, DEFAULT_WAIT4MOD) < 0) {
		if (debug) 
			gsm_log(GSM_DEBUG, "expect Call Ready failed\n");
	}

	/* check the device */
	if (send_expect(ctl, "AT", "OK", 0, NULL, 0, DEFAULT_WAIT4MOD) <0) {
		if (debug) 
			gsm_log(GSM_DEBUG, "Send_Expect AT failed\n");
	}

	/* this is the case where we have a valid initialization file */
	if (fp) {
		char line[256];

		while (fgets(line,255,fp)) {
			if ( ! strncasecmp(line,"AT", 2)) {
				char cmd[256], expect[256];
				int delay, wait;
				sscanf(line,"%s\t%s\t%d\t%d",cmd,expect,&delay,&wait);

				if (debug) 
					gsm_log(GSM_DEBUG, "port=(%d): Sending: cmd(%s) expect(%s) delay(%d) wait(%d)\n",ctl->port,
						cmd, expect, delay, wait);

				if (send_expect(ctl, cmd, expect, wait, &resp,delay, DEFAULT_WAIT4MOD)<=0) {
					if (debug) 
						gsm_log(GSM_DEBUG, "port=(%d): cmd(%s) expect(%s) error: %s\n",ctl->port, cmd,expect, resp);
					free(resp);
				}
			} else {
				if (debug) 
					gsm_log(GSM_DEBUG, "port=(%d): ignoring line: %s\n",ctl->port, line);
			}
		}

		/* authenticate us to the SIM : PIN is a parameter we need to get from configuration */
		if ( strcasecmp(ctl->cfg->pin, "none") ) {
			sprintf(buf,"AT+CPIN=\"%s\"",ctl->cfg->pin);
			if (!send_expect(ctl, buf, "OK", 0, NULL, 0, DEFAULT_WAIT4MOD)) {
				if (debug) 
					gsm_log(GSM_DEBUG, "port=(%d): CPIN error: %s\n", ctl->port, ((resp) ? resp : ""));
			}
		} else {
				printf("%d NO PIN AUTH\n", ctl->port);
		}

		/* set sms text mode (text vs. pdu) */
		if (ctl->cfg->sms_pdu_mode) {
			send_expect(ctl, "AT+CMGF=0", "OK", 1, NULL, 0, DEFAULT_WAIT4MOD);
		} else {
			send_expect(ctl, "AT+CMGF=1", "OK", 1, NULL, 0, DEFAULT_WAIT4MOD);
		}

		/* set the short message service center if it's there and not set to "none" */
		if (strlen(ctl->cfg->smsc) && (strcasecmp(ctl->cfg->smsc, "none"))) {
			sprintf(buf,"AT+CSCA=\"%s\"", ctl->cfg->smsc);
			if (!send_expect(ctl, buf, "OK", 1, NULL,0, DEFAULT_WAIT4MOD)) {
				if (debug) 
					gsm_log(GSM_DEBUG, "port=(%d): SMSC error\n", ctl->port);

			}
		} else {
			if (debug) 
				gsm_log(GSM_DEBUG, "port=(%d): SMSC left blank, not setting it.\n", ctl->port);
		}
	} else {
		/* this is when we have no valid configuration file */
		if (debug) 
			gsm_log(GSM_DEBUG, "port=(%d): falling back to default init\n", ctl->port);

		/*Back to default settings*/

		/* Set DCD to be always ON */
		send_expect(ctl, "AT&C0", "OK", 1, NULL, 0, DEFAULT_WAIT4MOD);

		/* do not echo back received chars to us */
		send_expect(ctl, "ATE0", "OK", 1, NULL, 0, DEFAULT_WAIT4MOD);

		/* turn on unsolicited registration status messages */
		send_expect(ctl, "AT+CREG=1", "OK", 1, NULL, 0, DEFAULT_WAIT4MOD);

		/* query the registration status now */
		send_expect(ctl, "AT+CREG?", "OK", 1, NULL, 0, DEFAULT_WAIT4MOD);

		/* set the sidetone gain level */
		send_expect(ctl, "AT+SIDET=0", "OK", 1, NULL, 0, DEFAULT_WAIT4MOD);
	
		/* we now set the rx/tx gain with values read from configuration */	

		/* switch on SIM card detection */
		send_expect(ctl, "AT+CSDT=1", "OK", 1, NULL, 0, DEFAULT_WAIT4MOD);

		/* enable SIM insertion status reporting */
		send_expect(ctl, "AT+CSMINS=1", "OK", 1, NULL, 0, DEFAULT_WAIT4MOD);

		/* enable calling line identification presentation */
		send_expect(ctl, "AT+CLIP=1", "OK", 1, NULL, 0, DEFAULT_WAIT4MOD);
		
		/* set sms text mode (text vs. pdu) */
		if (ctl->cfg->sms_pdu_mode) {
			send_expect(ctl, "AT+CMGF=0", "OK", 1, NULL, 0, DEFAULT_WAIT4MOD);
		} else {
			send_expect(ctl, "AT+CMGF=1", "OK", 1, NULL, 0, DEFAULT_WAIT4MOD);
		}

		/* set the short message service center if it's there */
		if (strlen(ctl->cfg->smsc) && (strcasecmp(ctl->cfg->smsc, "none"))) {
			sprintf(buf,"AT+CSCA=\"%s\"", ctl->cfg->smsc);
			if (!send_expect(ctl, buf, "OK", 1, NULL,0, DEFAULT_WAIT4MOD)) {
				gsm_log(GSM_ERROR, "port=(%d): SMSC error: %s\n", ctl->port, resp);
			}
		} else {
			if (debug)
				gsm_log(GSM_DEBUG, "port=(%d): SMSC left blank, not setting it.\n", ctl->port);
		}

		/* authenticate us to the SIM */
		if ( strcasecmp(ctl->cfg->pin, "none") ) {
			sprintf(buf,"AT+CPIN=\"%s\"",ctl->cfg->pin);
			if (!send_expect(ctl, buf, "OK", 0, NULL,0, DEFAULT_WAIT4MOD)) {
				if (debug) 
					gsm_log(GSM_ERROR, "port=(%d): CPIN error: %s\n", ctl->port, resp);
					return -EWRONGPIN;
			}
		} else {
			if (debug)
				gsm_log(GSM_DEBUG, "%d NO PIN AUTH\n",ctl->port);
		}
	}

	/* set RX gain */
	gsm_set_rx_gain(ctl->port, ctl->cfg);

	/* set TX gain */
	gsm_set_tx_gain(ctl->port, ctl->cfg);

	return 0;
}

int init_mod_port(int port)
{
	struct ctlport *ctl = getctlport(port);

	if (ctl)
		return init_mod(ctl);
	else
		return -ENODEV;
}


char *errtostr[]={
		"Module initialization error 0",
		"Module initialization error",
		"WRONG PIN/ SIM Not Present"
};

/* writer thread to write to a port */
void *write_thread(void *p)
{
	struct ctlport *ctl=(struct ctlport*)p;
	struct gsm_cmd gsm_command;
	int msglen = sizeof(struct gsm_cmd) - sizeof(long);
	int retval;

	/* loop until we're killed */
	while (ctl->state != CTLPORT_STATE_KILLME) {
		
		if (debug) 
			gsm_log(GSM_DEBUG, "port=(%d): waiting on write_sem\n", ctl->port);

		/* wait on semaphore to continue */
		sem_wait(&write_sem[ctl->port]);

		if (debug) 
			gsm_log(GSM_DEBUG, "port=(%d): acquired write_sem\n", ctl->port);

		/* check if we need to quit */
		if (ctl->state == CTLPORT_STATE_KILLME) {
			if (debug)
				gsm_log(GSM_DEBUG, "port=(%d): leaving the write_thread()\n", ctl->port);
			break;
		}

		/* get a message from the outgoing message queue */
		if ((retval = msgrcv(ctl->write_mq_id, (void*)&gsm_command, msglen, 1, IPC_NOWAIT)) > 0) {
			/* acquire the semaphore to let you write to the port and write the message */
			if (debug)
				gsm_log(GSM_DEBUG, "port=(%d): waiting on avail_sem\n", ctl->port);
			sem_wait(&avail_sem[ctl->port]);

			/* check if we need to quit */
			if (ctl->state == CTLPORT_STATE_KILLME) {
				if (debug)
					gsm_log(GSM_DEBUG, "port=(%d): leaving the write_thread()\n", ctl->port);
				break;
			}

			if (debug) 
				gsm_log(GSM_DEBUG, "port=(%d): acquired avail_sem SENDING message (%s)\n", ctl->port, gsm_command.cmd_msg);
			_gsm_send_command(ctl->port, gsm_command.cmd_msg, 0);
			/* increment the number of msgs sent */
			ctl->msg_count++;
			if (debug) 
				gsm_log(GSM_DEBUG, "port=(%d): acquired avail_sem SENT message (%s)\n", ctl->port, gsm_command.cmd_msg);
			/* kick the watchdog timer here */
			gsm_start_check_timer(ctl->port);
		} else {
			/* we should not be here */
			if (debug) 
				gsm_log(GSM_DEBUG, "port=(%d): did not get message from queue\n", ctl->port);
		}
	}

	return NULL;
}

/* check the response */
int gsm_check_response(const char *cmd)
{
	return (strstr(cmd, "OK") || \
		strstr(cmd, "BUSY") || \
		strstr(cmd, "NO CARRIER") || \
		strstr(cmd, "ERROR") || \
		strstr(cmd, "CMS_ERROR") || \
		strstr(cmd, "CME_ERROR"));
}

void *read_thread(void *p)
{
	struct ctlport *ctl=(struct ctlport*)p;
	char *line = NULL;
	int ret;
	struct timeval tv;

	ret=init_mod(ctl);

	if (ret<0 || ! ctl->fpr ) {
		sem_post(&ready[ctl->port]);
		gsm_log(GSM_ERROR, "port=(%d): Error on module init, stopping read thread\n",ctl->port);

		char buf[256];
		sprintf(buf,"GSMINIT: PORT NOT INITIALIZED: %s", errtostr[-ret]);
		if (cbEvents) {
			if (debug) 
				gsm_log(GSM_DEBUG, "port=(%d): event: %s\n",ctl->port, buf);
			cbEvents(ctl->port, buf);
		}

		return NULL;
	}

	/* allocate buffer memory */
	if (!(line=malloc(LINE_BUFFER_SIZE))) {
		gsm_log(GSM_ERROR, "port=(%d) error allocating buffer memory\n", ctl->port);
		return NULL;
	}

	/* post so that the write thread can begin operation */
	sem_post(&ready[ctl->port]);

	/* Query the IMEI number */
	gsm_send(ctl->port, "AT+GSN", 0);

	/* Query the SIM insertion status */
	gsm_send(ctl->port, "AT+CSMINS?", 0);

	/* Query the registration status */
	gsm_send(ctl->port, "AT+CREG?", 0);

	/* Query the firmware rev info */
	gsm_send(ctl->port, "AT+GMR", 0);

	if (cbEvents)
		cbEvents(ctl->port, "GSMINIT: PORT INITIALIZED");

	if (debug) 
		gsm_log(GSM_ERROR, "port=(%d): Readthread running\n",ctl->port);

	while (ctl->state != CTLPORT_STATE_KILLME) {
		char *p;

		if (ctl->state == CTLPORT_STATE_SMS) {
			/*read "> " */
			if (debug) 
				ast_log(LOG_DEBUG, "port=(%d): Doing SMS Read\n", ctl->port);
			line[0]=fgetc(ctl->fpr);
			while (line[0] == '\n' || line[0] == '\r')
				line[0]=fgetc(ctl->fpr);
			line[1]=fgetc(ctl->fpr);
			line[2]='\0';
			ctl->state=CTLPORT_STATE_NONE;
		} else {
			memset(line, 0, LINE_BUFFER_SIZE);
			if (!fgets(line, LINE_BUFFER_SIZE-1, ctl->fpr)) {
				gsm_log(GSM_ERROR, "port=(%d): fgets returned NULL, exiting\n", ctl->port);
				return NULL;
			}
			p=strchr(line,'\r');
			if (p) *p='\0';
		}

		if ((debug) && (strlen(line))) { 
        		gettimeofday(&tv,NULL);
			gsm_log(GSM_DEBUG, "port=(%d): seconds (%ld) microseconds (%ld) read-event: %s\n",ctl->port, (long)tv.tv_sec, (long)tv.tv_usec, line);
		}
		if ((ctl->modem_state == MODEM_WAIT_FOR_RESPONSE) && (gsm_check_response(line))) {
			if (debug) 
				gsm_log(GSM_DEBUG, "port=(%d): MODEM CHECK OK\n",ctl->port);
			ctl->modem_state = MODEM_RECEIVED_RESPONSE;
		}

		if (cbEvents)
			cbEvents(ctl->port, line);
	}

	if  (line) 
		free(line);

	if (debug) 
		gsm_log(GSM_DEBUG, "Leaving the %s() main loop\n", __FUNCTION__);

	return NULL;
}

char *gsm_get_imei_number(int port)
{
	struct ctlport *ctl=getctlport(port);
	return (ctl) ? (ctl->imei) : "INVALID";
}

char *gsm_get_imsi_number(int port)
{
	struct ctlport *ctl=getctlport(port);
	return (ctl) ? (ctl->imsi) : "INVALID";
}

char *gsm_get_firmware_info(int port)
{
	struct ctlport *ctl=getctlport(port);
	return (ctl) ? (ctl->fw_rev_info) : "INVALID";
}

char *gsm_get_last_command(int port)
{
	struct ctlport *ctl=getctlport(port);
	return (ctl) ? (ctl->last_cmd_sent.cmd_msg) : "INVALID";
}

void gsm_handle_unknown_event(int port, char *event)
{
	struct ctlport *ctl=getctlport(port);

	if (debug) 
		gsm_log(GSM_DEBUG, "%s() : event (%s) last command (%s)\n",
			__FUNCTION__, event, ctl->last_cmd_sent.cmd_msg);

	/* decide what to do with the "event" depending on the last command sent */
	if (!strcmp("AT+GSN", ctl->last_cmd_sent.cmd_msg)) {		// IMEI query
		strncpy(ctl->imei, event, IMEI_SZ);
	} else if (!strcmp("AT+CIMI", ctl->last_cmd_sent.cmd_msg)) { 	// IMSI query
		strncpy(ctl->imsi, event, IMSI_SZ);
	} else if (!strcmp("AT+GMR", ctl->last_cmd_sent.cmd_msg)) {	// FW query
		strncpy(ctl->fw_rev_info, event, FW_REV_SZ);
	} else if (strstr(event, "*TENGMODE")) {
		ast_verbose(CHAN_GSM_VERBOSE_PREFIX "%s\n", event);	// ENG query
		if (debug) 
			gsm_log(GSM_DEBUG, "Port (%d) Diag Info (%s)\n", port, event);
	} else {
		/* handle others as they arise */
	}
}

 
int gsm_init_port(int port, struct gsm_config *gsm_cfg)
{
	char name[64];

	sprintf(name,"/dev/gsm/ctl%d",port);

	memset(&ctlport[port], 0, sizeof(struct ctlport));

	ctlport[port].port=port;
	ctlport[port].fpw=fopen(name, "w");
	ctlport[port].fpr=fopen(name, "r");
	ctlport[port].cfg=gsm_cfg;

	sem_init(&ready[port], 0, 0);

	if(!ctlport[port].fpw || !ctlport[port].fpr) {
		gsm_log(GSM_ERROR, "port=(%d): Could not open name:%s FP\n errror:%s \n", port, name, strerror(errno));
		return -1;
	} else {
		pthread_create(&ctlport[port].t, NULL, read_thread, &ctlport[port]);
		pthread_create(&ctlport[port].wt, NULL, write_thread, &ctlport[port]);
		sem_init(&write_sem[port], 0, 0);
		sem_init(&avail_sem[port], 0, 1);
		wdog_timer[port] = INVALID_TIMER_ID;
		int key = GSM_CMD_QUEUE_KEY; 
		if ((ctlport[port].write_mq_id = msgget(key + port, IPC_CREAT | 0660)) < 0) {
			ast_log(LOG_ERROR, "port=(%d) error creating message queue %s\n", port, strerror(errno));
		}
		/* reset the number of sent msgs to zero */
		ctlport[port].msg_count=0;
	}

	return 0;
}


void gsm_close_port(port)
{
	if (ctlport[port].fpw) {
		if (fclose(ctlport[port].fpw) < 0) {
			gsm_log(GSM_ERROR, "port=(%d): could not close ctlport(w) error:%s\n",
				port, strerror(errno));
		} else {
			ctlport[port].fpw=NULL;
		}
	}

	if (ctlport[port].fpr) {
		if (fclose(ctlport[port].fpr) < 0) {
			gsm_log(GSM_ERROR, "port=(%d): could not close ctlport(r) error:%s\n",
				port, strerror(errno));
		} else {
			ctlport[port].fpr=NULL;
		}
	}
}

int gsm_shutdown_port(int port, int powercycle)
{
	void *r;
	int fd = -1;
	struct ctlport *ctl=getctlport(port);

	if (!ctl) return -1;

	if (ctlport[port].state==CTLPORT_STATE_KILLED) return 0;

	if (ctlport[port].fpr) 
		fd = fileno(ctlport[port].fpr);

	/* set the port state to termination */
	ctlport[port].state=CTLPORT_STATE_KILLME;
	
	/* destroy timer for this port */
	gsm_destroy_check_timer(port);

	/* post the avail/write semaphores so the write thread can quit */
	gsm_post_next_msg(port);
	sem_post(&write_sem[port]);

	if (debug) 
		gsm_log(GSM_DEBUG, "port=(%d): calling join for writethread\n", port);
	pthread_join(ctlport[port].wt, &r);
	if (debug) 
		gsm_log(GSM_DEBUG, "port=(%d): writethread from port killed\n", port);
	/* terminate the read thread if it's still around */
	if (fd >= 0) {
		/* to guarantee that the read thread will get its event */
		ioctl(fd, GSM_KILLME, 0);

		/* hangup lingering calls (if any) */
		gsm_send_raw(port, "ATH\r\n", 100);
		if (debug) 
			gsm_log(GSM_DEBUG, "port=(%d): calling join for readthread\n", port);
		pthread_join(ctlport[port].t, &r);
		if (debug) 
			gsm_log(GSM_DEBUG, "port=(%d): readthread from port killed\n", port);
	}

	/* stop the audio */
	gsm_audio_stop(port);

	/* close the audio port */
	gsm_audio_close(port, 0);

	if (ctlport[port].fpw) {
		/* power cycle the radio */
		if (powercycle) {
			int fdw = (ctlport[port].fpw) ? fileno(ctlport[port].fpw) : -1;
			if (fdw >= 0) {
				if (debug) 
					gsm_log(GSM_DEBUG, "port=(%d): begin powercycle\n", port);
				ioctl(fdw, GSM_POWERCYCLE_RADIO, 1);
				if (debug) 
					gsm_log(GSM_DEBUG, "port=(%d): end powercycle\n", port);
			}
		}
	}

	/* close all control ports (read,write) */
	gsm_close_port(port);

	/* destroy all semaphores, so we init them "fresh" next time */
	sem_destroy(&ready[port]);
	sem_destroy(&write_sem[port]);
	sem_destroy(&avail_sem[port]);

	/* destroy the message queue for writes */
	msgctl(ctlport[port].write_mq_id, IPC_RMID, NULL);

	ctlport[port].state=CTLPORT_STATE_KILLED;

	return 0;
}

int gsm_wait_ready(int port)
{
	sem_wait(&ready[port]);
	if (debug) 
		gsm_log(GSM_DEBUG, "port=(%d): Port is ready\n",port);
	return 0;
}

int gsm_wait_ready_with_timeout(int port, int seconds)
{
	struct timespec expiry_tv;
	int retval;
	
	if ((retval = clock_gettime(CLOCK_REALTIME, &expiry_tv)) == 0) {
		expiry_tv.tv_sec += seconds;
		if ((retval = sem_timedwait(&ready[port], &expiry_tv)) == 0) {
			if (debug) gsm_log(GSM_DEBUG, "port=(%d): Port is ready\n",port);
		} else  {
			gsm_log(GSM_ERROR, "port=(%d): Port not ready. sem_timedwait error=(%s)", port, strerror(errno));
		}
	} else {
		gsm_log(GSM_ERROR, "port=(%d): Port not ready. clock_gettime error=(%s)", port, strerror(errno));
	}

	return (retval);
}

int gsm_do_ioctl(int port, int cmd, int arg)
{
	struct ctlport *ctl=getctlport(port);
	int fd;

	if (!ctl || !ctl->fpw) return 0;

	fd=fileno(ctl->fpw);

	if (fd<=0) return -1;

	return ioctl(fd, cmd, arg);
}

int gsm_init( void (*cbEvent)(int port, char *event), struct gsm_config gsm_cfg[MAX_GSM_PORTS], int sysdebug)
{
	struct ctlport *ctl;
	int i;

	cbEvents=cbEvent;

	if (sysdebug) {
		debug=1;
		openlog("chan_gsm", LOG_PID, LOG_INFO);
	}


	debugfp=stderr;

	for (i=1; i<MAX_GSM_PORTS; i++) {
		if (debug) 
			gsm_log(GSM_DEBUG, "gsm_cfg: port %d pin %s\n", i, gsm_cfg[i].pin);
		if (gsm_cfg[i].port) {
			if (gsm_init_port(i, &gsm_cfg[i])<0) {
				gsm_log(GSM_ERROR, "gsm_cfg: error initializing port %d\n", i);
				return -1;
			}
		}
	}

	for (i=1; i<MAX_GSM_PORTS; i++) {
		if (gsm_cfg[i].port) {
			if (gsm_wait_ready_with_timeout(i, GSM_READY_TIMEOUT) < 0) {
				gsm_log(GSM_ERROR, "gsm_cfg: error timed out initializing port %d\n", i);
				ctl = getctlport(i);
				if ((ctl) && (ctl->fpr)) {
					int fd = fileno(ctl->fpr);
					ioctl(fd, GSM_KILLME, 0);
				}
				gsm_close_port(i);
				gsm_cfg[i].port = 0;
			}
		}
	}

	/* query registration status for each port */
	for (i=1; i < MAX_GSM_PORTS; i++) 
		if (gsm_cfg[i].port) 
			gsm_send(i, "AT+CREG?", 0);

	fflush(debugfp);

	return 0;
}

void gsm_shutdown(void)
{
	int i;
	for (i=1;i<MAX_GSM_PORTS;i++) {
		if (ctlport[i].port) {
			if (debug) 
				gsm_log(GSM_DEBUG, "port=(%d): Shutting down port\n",i);
			gsm_shutdown_port(i, 0);
		}
	}

	if (debug && debugfp && debugfp!=stdout && debugfp!=stderr) {
			fclose(debugfp);
	}
}

#ifndef TEST_GSM
void gsm_log(int loglevel, const char *format, ...)
{
        char buffer[512];
        va_list args;

        va_start(args, format);
        vsprintf(buffer, format, args);
        va_end(args);

        /* we do this as LOG_XXX are all macros and not real enums */
        switch (loglevel) {
                case GSM_DEBUG:
                        ast_log(LOG_DEBUG, "%s", buffer);
                        break;
                case GSM_ERROR:
                        ast_log(LOG_ERROR, "%s", buffer);
                        break;
                case GSM_WARNING:
                        ast_log(LOG_WARNING, "%s", buffer);
                        break;
                case GSM_NOTICE:
                        ast_log(LOG_NOTICE, "%s", buffer);
                        break;
		case GSM_EVENT:
                        ast_log(LOG_EVENT, "%s", buffer);
                        break;
                default:
                        ast_log(LOG_NOTICE, "%s", buffer);
                        break;
        }

        syslog(LOG_INFO, buffer);
}
#endif /* TEST_GSM */

#ifdef TEST_GSM
void test_event(int port, char* event )
{
	if (debug) 
		gsm_log(GSM_DEBUG, "Port (%d) fired Event (%s)\n",port, event);
}

int main(int argc, char **argv)
{
	size_t n;
	char line[256];
	struct gsm_config configs[MAX_GSM_PORTS];
	int i;

	if (argc<2 || argc>3) {
		if (debug) gsm_log(GSM_DEBUG, "Please provide at least a Port as argument, addionally provied an initscript.\n");
		exit(1);
	}

	printf("Starting %d....\n", argc);

	memset(configs, 0, sizeof(configs));

	configs[atoi(argv[1])].port = 1;
	if (argc > 2)
		strcpy(configs[atoi(argv[1])].initfile, argv[2]);

	gsm_init(test_event, configs, 1);

	while (fgets(line, 255, stdin)) {
		if (line) {
			char *p=strchr(line,'\n');
			if (p)
				*p = '\0';

			if (strncmp(line, "EOM", 3) == 0) {
				gsm_send(1, "\032", 1);
				if (debug)
					printf("ctrl-z submitted\n");

			} else {
				gsm_send(atoi(argv[1]), line, 0);
				if (debug)
					printf("command submitted\n");
			}
		}
	}

	if (debug) gsm_log(GSM_DEBUG, "Exiting main\n");

}

#endif
