#ifndef _GSMCTL_
#define _GSMCTL_


#define GSM_MAX_STR	255
#define MAX_GSM_PORTS 	32

struct gsm_config {
	int port;
	char initfile[256];
	char pin[256];
	char smsc[256];
	int sms_pdu_mode;
	int rx_gain;
	int tx_gain;
};

struct gsm_req {
	int type;
	unsigned param1;
	unsigned param2;
};

enum gsm_errors {
	EWRONGPIN=2
};

enum cme_error_code {
	CME_ERROR_PHONE_FAILURE = 0,
	CME_ERROR_NO_PHONE_CONNECTION = 1,
	CME_ERROR_PHONE_ADAPTER_LINK_RESERVED = 2,
	CME_ERROR_OPERATION_NOT_ALLOWED = 3,
	CME_ERROR_OPERATION_NOT_SUPPORTED = 4,
	CME_ERROR_PH_SIM_PIN_REQUIRED = 5,
	CME_ERROR_PH_FSIM_PIN_REQUIRED = 6,
	CME_ERROR_PH_FSIM_PUK_REQUIRED = 7,
	CME_ERROR_SIM_NOT_INSERTED = 10,
	CME_ERROR_SIM_PIN_REQUIRED = 11,
	CME_ERROR_SIM_PUK_REQUIRED = 12,
	CME_ERROR_SIM_FAILURE = 13,
	CME_ERROR_SIM_BUSY = 14,
	CME_ERROR_SIM_WRONG = 15,
	CME_ERROR_INCORRECT_PASSWORD_PIN = 16,
	CME_ERROR_SIM_PIN2_REQUIRED = 17,
	CME_ERROR_SIM_PUK2_REQUIRED = 18,
	CME_ERROR_MEMORY_FULL = 20,
	CME_ERROR_INVALID_INDEX = 21,
	CME_ERROR_NOT_FOUND = 22,
	CME_ERROR_MEMORY_FAILURE = 23,
	CME_ERROR_TEXT_STRING_TOO_LONG = 24,
	CME_ERROR_INVALID_CHARS_IN_TEXT_STRING = 25,
	CME_ERROR_DIAL_STRING_TOO_LONG = 26,
	CME_ERROR_INVALID_CHARS_IN_DIALSTRING = 27,
	CME_ERROR_NO_NETWORK_SERVICE = 30,
	CME_ERROR_NETWORK_TIMEOUT = 31,
	CME_ERROR_EMERGENCY_CALLS_ONLY = 32,
	CME_ERROR_NETWORK_PERS_PIN_REQUIRED = 40,
	CME_ERROR_NETWORK_PERS_PUK_REQUIRED = 41,
	CME_ERROR_NETWORK_SUBS_PERS_PIN_REQUIRED = 43,
	CME_ERROR_SERVICE_PROVIDER_PERS_PIN_REQUIRED = 44,
	CME_ERROR_SERVICE_PROVIDER_PERS_PUK_REQUIRED = 45,
	CME_ERROR_CORPORATE_PERS_PIN_REQUIRED = 46,
	CME_ERROR_CORPORATE_PERS_PUK_REQUIRED = 47,
	CME_ERROR_UNKNOWN = 100,
	CME_ERROR_ILLEGAL_MS = 103,
	CME_ERROR_ILLEGAL_ME = 106,
	CME_ERROR_GRPS_SERVICES_NOT_ALLOWED = 107,
	CME_ERROR_PLMN_NOT_ALLOWED = 111,
	CME_ERROR_LOCATION_AREA_NOT_ALLOWED = 112,
	CME_ERROR_ROAMING_NOT_ALLOWED = 113,
	CME_ERROR_SERVICE_OPTION_NOT_SUPPORTED = 132,
	CME_ERROR_REQUESTED_SERVICE_OPTION_NOT_SUBSCRIBED = 133,
	CME_ERROR_SERVICE_OPTION_TEMP_OUT_OF_ORDER =134,
	CME_ERROR_UNSPECIFIED_GPRS_ERROR = 148,
	CME_ERROR_PDP_AUTHENTICATION_FAILURE = 149,
	CME_ERROR_INVALID_MOBILE_CLASS = 150,
	CME_ERROR_AUDIO_MANAGER_NOT_READY = 673,
	CME_ERROR_AUDIO_FORMAT_NOT_CONFIGURED = 674,
	CME_ERROR_SIM_TOOLKIT_NOT_CONFIGURED = 705,
	CME_ERROR_SIM_TOOLKIT_ALREADY_IN_USE = 706,
	CME_ERROR_SIM_TOOLKIT_NOT_ENABLED = 707,
	CME_ERROR_CSCS_TYPE_NOT_SUPPORTED = 737,
	CME_ERROR_CSCS_TYPE_NOT_FOUND = 738,
	CME_ERROR_MUST_INCLUDE_FORMAT_OPER = 741,
	CME_ERROR_INCORRECT_OPER_FORMAT = 742,
	CME_ERROR_OPER_LENGTH_TOO_LONG = 743,
	CME_ERROR_SIM_FULL = 744,
	CME_ERROR_CANNOT_CHANGE_PLMN_LIST = 745,
	CME_ERROR_NETWORK_OP_NOT_RECOGNIZED = 746,
	CME_ERROR_INVALID_COMMAND_LENGTH = 749,
	CME_ERROR_INVALID_INPUT_STRING = 750,
	CME_ERROR_INVALID_SIM_COMMAND = 754,
	CME_ERROR_INVALID_FILE_ID = 755,
	CME_ERROR_MISSING_P123_PARAMETER = 756,
	CME_ERROR_INVALID_P123_PARAMETER = 757,
	CME_ERROR_MISSING_REQD_COMMAND_DATA = 758,
	CME_ERROR_INVALID_CHARS_IN_COMMAND_DATA = 759,
	CME_ERROR_INVALID_INPUT_VALUE = 765,
	CME_ERROR_UNSUPPORTED_VALUE_OR_MODE = 766,
	CME_ERROR_OPERATION_FAILED = 767,
	CME_ERROR_MUX_ALREADY_ACTIVE = 768,
	CME_ERROR_SIM_INVALID_NETWORK_REJECT = 770,
	CME_ERROR_CALL_SETUP_IN_PROGRESS = 771,
	CME_ERROR_SIM_POWERED_DOWN = 772,
	CME_ERROR_SIM_FILE_NOT_PRESENT = 773
};

enum cms_error_code {
	CMS_ERROR_ME_FAILURE = 300,
	CMS_ERROR_SMS_ME_RESERVED = 301,
	CMS_ERROR_OPERATION_NOT_ALLOWED = 302,
	CMS_ERROR_OPERATION_NOT_SUPPORTED = 303,
	CMS_ERROR_INVALID_PDU_MODE = 304,
	CMS_ERROR_INVALID_TEXT_MODE = 305,
	CMS_ERROR_SIM_NOT_INSERTED = 310,
	CMS_ERROR_SIM_PIN_NECESSARY = 311,
	CMS_ERROR_PH_SIN_PIN_NECESSARY = 312,
	CMS_ERROR_SIM_FAILURE = 313,
	CMS_ERROR_SIM_BUSY = 314,
	CMS_ERROR_SIM_WRONG = 315,
	CMS_ERROR_SIM_PUK_REQUIRED = 316,
	CMS_ERROR_SIM_PIN2_REQUIRED = 317,
	CMS_ERROR_SIM_PUK2_REQUIRED = 318,
	CMS_ERROR_MEMORY_FAILURE = 320,
	CMS_ERROR_INVALID_MEMORY_INDEX = 321,
	CMS_ERROR_MEMORY_FULL = 322,
	CMS_ERROR_SMSC_ADDRESS_UNKNOWN = 330,
	CMS_ERROR_NO_NETWORK = 331,
	CMS_ERROR_NETWORK_TIMEOUT = 332,
	CMS_ERROR_UNKNOWN = 500,
	CMS_ERROR_SIM_NOT_READY = 512,
	CMS_ERROR_UNREAD_RECORDS_ON_SIM = 513,
	CMS_ERROR_CB_ERROR_UNKNOWN = 514,
	CMS_ERROR_PS_BUSY = 515,
	CMS_ERROR_SIM_BL_NOT_READY = 517,
	CMS_ERROR_INVALID_CHARS_IN_PDU = 528,
	CMS_ERROR_INCORRECT_PDU_LENGTH = 529,
	CMS_ERROR_INVALID_MTI = 530,
	CMS_ERROR_INVALID_CHAR_IN_ADDRESS = 531,
	CMS_ERROR_INVALID_ADDRESS = 532,
	CMS_ERROR_INVALID_PDU_LENGTH = 533,
	CMS_ERROR_INCORRECT_SCA_LENGTH = 534,
	CMS_ERROR_INVALID_FIRST_OCTECT = 536,
	CMS_ERROR_INVALID_COMMAND_TYPE = 537,
	CMS_ERROR_SRR_BIT_NOT_SET = 538,
	CMS_ERROR_SRR_BIT_SET = 539,
	CMS_ERROR_INVALID_USER_DATA_IN_IE = 540
};



int gsm_send(int port, char *s, int delay);
int gsm_send_raw(int port, char *s, int delay);

int gsm_init( void (*cbEvent)(int port, char *event), struct gsm_config gsm_cfg[MAX_GSM_PORTS], int sysdebug);
void gsm_shutdown(void);

int gsm_init_port(int port, struct gsm_config *gsm_cfg);
int gsm_wait_ready(int port);
int gsm_wait_ready_with_timeout(int port, int seconds);
int gsm_shutdown_port(int port, int powercycle);

void *gsm_get_priv(int port);
void gsm_put_priv(int port, void *priv);

int gsm_audio_open(int port);
void gsm_audio_close(int port, int fd);

int gsm_audio_start(int port);
int gsm_audio_stop(int port);

int gsm_set_next_simslot(int port, int slot);

int gsm_sms_enter(int port);

int gsm_port_is_valid(int port);

int gsm_port_fetch(int port);
int gsm_port_release(int port);
int gsm_port_in_use_get(int port);

int gsm_simslot_get(int port);
int gsm_simslot_next_get(int port);

int gsm_check_port(int port);
int gsm_set_led(int port, int ledstate);

int init_mod_port(int port);

int gsm_do_ioctl(int port, int cmd, int arg);
void gsm_set_debuglevel(int debuglevel);
const char *gsm_get_cme_text(int cme_code);
const char *gsm_get_cms_text(int cme_code);
void gsm_post_next_msg(int port);
void gsm_start_check_timer(int port);
void gsm_stop_check_timer(int port);
int gsm_get_channel_mask(void);
int gsm_get_channel_offset(void);
void gsm_handle_unknown_event(int port, char *event);
char *gsm_get_imei_number(int port);
char *gsm_get_imsi_number(int port);
char *gsm_get_firmware_info(int port);
int gsm_translate_channel(int *result, int channel_id);
char *gsm_get_last_command(int port);
void gsm_masq_priv(void * oldpriv,void * newpriv);
void gsm_log(int loglevel, const char *format, ...);
void gsm_close_port(int port);

#define GSM_IOC_MAGIC                      	'G'
#define GSM_SETLED                         	_IO(GSM_IOC_MAGIC, 0)
#define GSM_GETLED                         	_IO(GSM_IOC_MAGIC, 1)
#define GSM_SETSTATE                       	_IO(GSM_IOC_MAGIC, 2)
#define GSM_SETNEXTSIMSLOT                 	_IO(GSM_IOC_MAGIC, 3)
#define GSM_KILLME				_IO(GSM_IOC_MAGIC, 4)
#define GSM_PEEK_FPGA				_IO(GSM_IOC_MAGIC, 5)
#define GSM_POKE_FPGA				_IO(GSM_IOC_MAGIC, 6)
#define GSM_POWERCYCLE_RADIO			_IO(GSM_IOC_MAGIC, 7)
#define GSM_SET_RX_GAIN				_IO(GSM_IOC_MAGIC, 8)
#define GSM_SET_TX_GAIN				_IO(GSM_IOC_MAGIC, 9)



#define LED_GREEN                          (0x1 << 0)
#define LED_RED                            (0x1 << 1)
#define LED_OFF                            (0x1 << 2)
#define LED_ON                             (0x1 << 3)
#define LED_BLINK                          (0x1 << 4)

#define STATE_CONNECTED                    (0x1 << 0)
#define STATE_HANGUP                       (0x1 << 1)

#define CHAN_GSM_VERBOSE_PREFIX			VERBOSE_PREFIX_1

#define GSM_DEBUG	__LOG_DEBUG
#define GSM_ERROR	__LOG_ERROR
#define GSM_WARNING	__LOG_WARNING
#define GSM_NOTICE	__LOG_NOTICE
#define GSM_EVENT	__LOG_EVENT

#define GSM_READY_TIMEOUT			15

#endif
