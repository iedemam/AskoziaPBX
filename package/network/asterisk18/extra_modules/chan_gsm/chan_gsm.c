#include <asterisk.h>

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <asterisk/lock.h>
#include <asterisk/module.h>
#include <asterisk/channel.h>
#include <asterisk/config.h>
#include <asterisk/logger.h>
#include <asterisk/pbx.h>
#include <asterisk/options.h>
#include <asterisk/io.h>
#include <asterisk/frame.h>
#include <asterisk/translate.h>
#include <asterisk/cli.h>
#include <asterisk/musiconhold.h>
#include <asterisk/dsp.h>
#include <asterisk/translate.h>
#include <asterisk/file.h>
#include <asterisk/callerid.h>
#include <asterisk/indications.h>
#include <asterisk/app.h>
#include <asterisk/features.h>
#include <asterisk/manager.h>
#include <asterisk/sched.h>

#include <asterisk/configman.h>
#include <asterisk/csel.h>

#include "gsmctl.h"
//#include "gsm_version.h"
void* load_module_thread(void* data);

ast_mutex_t gsm_mutex;
static int gsm_debug = 0; /* SAM DBG */
static int gsm_initialized=0;
static int gsm_save_out_msgs=0;	/* FUTURE CONFIGURATION OPTION */

/* we do mu-law */
#define GSM_AUDIO_FORMAT 				AST_FORMAT_ULAW
#define REGISTRATION_STATUS_CHECK_PERIOD_SEC		10
#define LCD_UPDATE_DELAY				10

/* and check call status every second for a channel that's dialing out */
#define CALL_STATUS_CHECK_PERIOD_SEC			1
#define SMS_SEND_PERIOD_SEC				10
#define MAX_FPGA_OFFSET					0x4044
#define GSM_PROJ_ID					0x12345678
#define GSM_PATH_NAME					"/var/spool/asterisk/smsin"


/* the main schedule context for stuff like l1 watcher, overlap dial, ... */
static struct sched_context *gsm_tasks = NULL;
static pthread_t gsm_tasks_thread;
#if 0
static pthread_t _gsm_restart_threads[MAX_GSM_PORTS];
#endif

static void _init_mod_port(int port);
int _gsm_shutdown_port(int port, int powercycle);

const char tdesc[]="GSM driver";

static int gsm_text(struct ast_channel *c, const char *text);
static int gsm_call(struct ast_channel *c, char *dest, int timeout);
static int gsm_answer(struct ast_channel *c);
static int gsm_hangup(struct ast_channel *c);
static int gsm_write(struct ast_channel *c, struct ast_frame *f);
static struct ast_frame *gsm_read(struct ast_channel *c);
static int gsm_fixup(struct ast_channel *oldchan, struct ast_channel *newchan);
#ifdef ASTERISK_1_4
static int gsm_digit_begin(struct ast_channel *c, char digit);
static int gsm_indicate(struct ast_channel *c, int cond, const void *data, size_t datalen);
static int gsm_digit_end(struct ast_channel *c, char digit, unsigned int duration);
#else
static int gsm_indicate(struct ast_channel *c, int cond);
static int gsm_digit_end(struct ast_channel *c, char digit);
#endif
static struct ast_channel *gsm_request(const char *type, int format, void *data, int *cause);

static void _gsm_restart_port(int port, int postwait);
static int _gsm_send_sms(int port, const char *number, const char *sms);
static int _gsm_queue_sms(int port, const char *number, const char *sms);
static int check_and_fetch_port(int port);
static void gsm_send_channel_state(int channel, int state);

static const struct ast_channel_tech gsm_tech = {
	.type = "GSM",
	.description = tdesc,
	.capabilities = AST_FORMAT_ULAW,
	.requester = gsm_request,
#ifdef ASTERISK_1_4
	.send_digit_begin = gsm_digit_begin,
	.send_digit_end = gsm_digit_end,
#else
	.send_digit= gsm_digit_end,
#endif
	.send_text = gsm_text,
	.hangup = gsm_hangup,
	.answer = gsm_answer,
	.read = gsm_read,
	.call = gsm_call,
	.write = gsm_write,
	.indicate = gsm_indicate,
	.fixup = gsm_fixup,
};

enum gsm_state {
	GSM_STATE_ATD_OUT,
	GSM_STATE_PROCEEDING,
	GSM_STATE_ALERTING,
	GSM_STATE_ANSWERED,
	GSM_STATE_HANGUPED,
	GSM_STATE_RINGING,
	GSM_STATE_NONE,
	GSM_STATE_COLLISION
};

enum channel_state {
	CHANNEL_STATE_DOWN,
	CHANNEL_STATE_IN_USE,
	CHANNEL_STATE_READY
};

#define GSM_STATE_DOWN 		"CHANNEL_DOWN"
#define GSM_STATE_IN_USE 	"CHANNEL_IN_USE"
#define GSM_STATE_READY		"CHANNEL_READY"

static char default_context[]="default";
static char default_exten[]="s";


static struct gsm_config gsm_cfg[MAX_GSM_PORTS];

/*DON'T FORGET THE STRING ARRAY (Status2txt)!!*/
enum gsm_status {
		GSM_STATUS_NOTCONFIGURED=0,
		GSM_STATUS_UNAVAILABLE,
		GSM_STATUS_UNINITIALIZED,
		GSM_STATUS_INITIALIZED,
		GSM_STATUS_UNREGISTERED,
		GSM_STATUS_REGISTERED,
};

char *status2txt[]={
		"NOT CONFIGURED",
		"UNAVAILABLE",
		"UNINITIALIZED",
		"INITIALIZED",
		"UNREGISTERED",
		"REGISTERED"
};

enum gsm_sms_work {
	GSM_SMS_WORK_SHOW=0,
	GSM_SMS_WORK_FETCH,
	GSM_SMS_WORK_FLUSH,
};

enum gsm_sms_state {
	GSM_SMS_STATE_NONE=0,
	GSM_SMS_STATE_EXPECTING_OK,
	GSM_SMS_STATE_EXPECTING_PROMPT,
	GSM_SMS_STATE_EXPECTING_SMSLIST,
	GSM_SMS_STATE_SMS_SENT,
	GSM_SMS_STATE_SMS_OK,
	GSM_SMS_STATE_LIST,
	GSM_SMS_STATE_GET_TEXT,
	GSM_SMS_STATE_FLUSHING,
};

#define SMS_MSG_SZ	256
#define SMS_NUM_SZ	32
struct gsm_sms_message {
	int mtype;
	int idx;
	char number[SMS_NUM_SZ];
	char date[32];
	char text[SMS_MSG_SZ];
	int pdu_len;
};

struct gsm_sms_msg_item {
	long mtype;
	struct gsm_sms_message msg;
}; 

struct gsm_sms {
	enum gsm_sms_state state;
	char sms[SMS_MSG_SZ];
	char number[SMS_NUM_SZ];

	enum gsm_sms_work work;

	/*manager stuff*/
	struct mansession *s;
	struct message *m;

	struct gsm_sms_message inbound_msg; /* temporary message, where we work on*/
	struct gsm_sms_message outbound_msg; /* temporary message, where we work on*/
	int sms_mq_id;	/* message queue id */

} gsm_sms[MAX_GSM_PORTS];

char *rssi2txt(int rssi) {
	static char txt[8];

	if (rssi == 99)
		return "not measurable";
	if (rssi == 0)
		return "<= -113 dbm";
	if (rssi == 31)
		return ">= -51dbm";

	sprintf(txt, "-%ddbm", 113 - 2 * rssi);
	return txt;
}

struct gsm_status_s {
		int status;
		char provider[128];
		int homezone;
		int rssi;
} gsm_status[MAX_GSM_PORTS];

struct gsm_pvt {
	enum gsm_state state;
	int port;
	int portfd;
	char context[256];
	char exten[256];
        struct ast_channel *ast_chan;
	struct ast_frame read_f;
	char readbuf[160];
	int hidecallerid;
	struct ast_dsp *dsp;
	struct ast_trans_pvt *trans;
};

/** Config Stuff **/

/*
 * Global Variables
 */


/*see doc p.151*/
struct gsm_slcc {
	int idx,dir,stat,mode,mpty,tca;
	enum channel_state channel_state;
	char callerid[64];
	int type;
	int ringcount;
	int ringing;
} gsm_slcc[MAX_GSM_PORTS];


struct csel_groups {
	char *name;
	struct csel *cs;
} csel_groups[MAX_GSM_PORTS];


#define CONFIG_FILE     "gsm.conf"
#define GROUPNAME_MAX   128

static cm_t *config = NULL;

/*
 * Configuration
 */
static const cm_dir_t gen_dirs[] = {
	{},
	{ "debug", "0", "debug level" },
	{ "syslogdebug","no", "Say yes to debug in syslog with identy 'chan_gsm'." },
	{ "smsdir", "/var/spool/asterisk/smsin", "The directory where inbound sms are stored" },
	{ "check_homezone", "no", "whether the homezone should be checked every 10 seconds." },
	{ "skip_plus", "no", "if the number comes with a '+' prefix, we skip it." },
	{ "relaxdtmf", "no", "whether radio/gsm optimzed dtmf detection mechanism should be used" },
	{ "dtmfbegin", "yes", "whether dtmf regeneration should start at the beginning of the dtmf frame" },
};

enum {
	CFG_GEN_NAME = 0,
	CFG_GEN_DEBUG,
	CFG_GEN_SYSLOG_DEBUG,
	CFG_GEN_SMS_DIR,
	CFG_GEN_CHEKHZ,
	CFG_GEN_SKIP_PLUS,
	CFG_GEN_RELAXDTMF,
	CFG_GEN_DTMFBEGIN,
};

static const cm_dir_t port_dirs[] = {
	{ "name", 0, "name of the group" },
	{ "channel", 0, "comma separated list of channel numbers" },
	{ "slot", 0, "default slot (none given means 0)" },
	{ "pin", "", "PIN for all ports in this group" },
	{ "context", "default", "context to use for incoming calls" },
	{ "exten", "s", "extension to use for incoming calls" },
	{ "initfile", "", "filename containing an alternative module init sequence" },
	{ "hidecallerid", "no", "if set to yes hide the callerid" },
	{ "smsc", "", "The number of your SMS Service Provider" },
	{ "sms_pdu_mode", "", "If the SMS should be provided in PDU or ClearText mode." },
	{ "provider", "", "Name of your GSM Provider (Number Format, try \"gsm show ops\" to get a list of all possible values" },
	{ "method", "standard", "Strategy for choosing channels in a group." },
	{ "cid_prefix", "", "If the callerid comes without Internation Code and without '+' prefix, the cidprefix is used as prefix." },
	{ "resetinterval", "0", "If you want chan_gsm to restart a module periodically, set this to the amount of seconds which should expire.\n After this time has expired chan_gsm will check if there is a call on this channel and if not restart it, else it will wait again." },
	{ "rxgain", 0, "GSM receive gain setting, set this to a number between -10 and +10 (0 for default values)" },
	{ "txgain", 0, "GSM transmit gain setting, set this to a number between -10 and +10 (0 for default values)" },
};

enum {
	CFG_PORT_NAME = 0,
	CFG_PORT_PORTS,
	CFG_PORT_SLOT,
	CFG_PORT_PIN,
	CFG_PORT_CONTEXT,
	CFG_PORT_EXTEN,
	CFG_PORT_INITFILE,
	CFG_PORT_HIDECALLERID,
	CFG_PORT_SMSC,
	CFG_PORT_SMS_PDU_MODE,
	CFG_PORT_PROVIDER,
	CFG_PORT_METHOD,
	CFG_PORT_CID_PREFIX,
	CFG_PORT_RESET_INTERVAL,
	CFG_PORT_RX_GAIN,
	CFG_PORT_TX_GAIN
};

static const cm_section_t config_sections[] = {
	{ "general", NULL, NULL, KTYPE_NONE, sizeof(gen_dirs) / sizeof(cm_dir_t), gen_dirs},
	{ "port", "default", "channel", KTYPE_INTEGER, sizeof(port_dirs) / sizeof(cm_dir_t), port_dirs},
};

enum {
	SECT_GENERAL = 0,
	SECT_PORT
};

//static int gsm_config_init (void)
int gsm_config_init (void)
{
	if (!(config = cm_create("gsm", config_sections, sizeof(config_sections) / sizeof(cm_section_t)))) {
		gsm_log(GSM_ERROR, "failed to initialize config manager!\n");
		goto err1;
	}

	if (cm_load(config, CONFIG_FILE)) {
		gsm_log(LOG_ERROR, "failed to load config file: %s\n", CONFIG_FILE);
		goto err2;
	}

	return 0;

err2:
	cm_destroy(config);
	config = NULL;
err1:
	return -1;
}

static void gsm_config_exit (void)
{
	if (config) {
		cm_destroy(config);
		config = NULL;
	}
}


#define CLI_CHECK_PORT(bb) { \
	if ( gsm_status[bb].status == GSM_STATUS_NOTCONFIGURED) { \
		ast_cli(a->fd, "Port: %d not configured\n", bb); \
		ast_log(LOG_WARNING, "Port: %d not configured\n", bb); \
		return CLI_FAILURE; 	\
	}	\
}

/** LCD STUFF **/
static void gsm_send_channel_state(int channel, int state)
{
	const char channel_name[] = "gsm";
	char channel_state[GSM_MAX_STR];
	int channel_number;

	memset(channel_state, 0, sizeof(channel_state));

	switch (state) {
		case CHANNEL_STATE_DOWN:
			strncpy(channel_state, GSM_STATE_DOWN, GSM_MAX_STR);
			break;
		case CHANNEL_STATE_IN_USE:
			strncpy(channel_state, GSM_STATE_IN_USE, GSM_MAX_STR);
			break;
		case CHANNEL_STATE_READY:
			strncpy(channel_state, GSM_STATE_READY, GSM_MAX_STR);
			break;
		default:
			break;
	}

	channel_number = channel + gsm_get_channel_offset();

	if ((gsm_translate_channel(&channel_number, channel) >= 0) && (strlen(channel_state))) {
		if (gsm_debug)
			gsm_log(GSM_DEBUG, "channel (%d) ---> channel_number (%d) state (%d)\n",
				channel, channel_number, state);
		manager_event(EVENT_FLAG_SYSTEM,
				"ChannelStatus",
				"Channel: PIKA/%s/%d\r\nState: %s\r\n", channel_name, channel_number, channel_state);
	}
}

static void gsm_set_channel_state(int channel, int state)
{
	if ((channel >= 1) && (channel < MAX_GSM_PORTS)) {
		gsm_slcc[channel].channel_state = state;
	}
}

static int gsm_get_channel_state(int channel, enum channel_state *state)
{
	if ((gsm_port_is_valid(channel)) && ((channel >= 1) && (channel < MAX_GSM_PORTS))) {
		*state = gsm_slcc[channel].channel_state;
		return 0;
	} else {
		return -1;
	}
}

static char* handle_gsm_request_state(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	enum channel_state channel_state;
	int ii;

	switch (cmd) {
	case CLI_INIT:
		e->command = "gsm request state";
		e->usage = "Usage: for astmanproxy only\n";
		return NULL;

	case CLI_GENERATE:
		return NULL;
	}

	for (ii = 0; ii < MAX_GSM_PORTS; ii++) {
		if ((gsm_port_is_valid(ii)) && (gsm_get_channel_state(ii, &channel_state)>=0)) {	
			gsm_send_channel_state(ii, channel_state);
			ast_cli(a->fd, "sending state (%d) for channel (%d)\n", channel_state, ii);
			usleep(500 * 1000); /* TODO : check */
		}
	}

	return CLI_SUCCESS;
}

static char* handle_gsm_show_info(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	int gsm_channel_mask = gsm_get_channel_mask();

	switch (cmd) {
	case CLI_INIT:
		e->command = "gsm show info";
		e->usage = "Usage: gsm show info\n";
		return NULL;

	case CLI_GENERATE:
		return NULL;
	}

	ast_cli(a->fd, "Gsm Mask:          0x%04x\n", gsm_channel_mask);

	return CLI_SUCCESS;
}

/** CLI STUFF **/
static char* handle_gsm_send_at(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	char *portstr, *command, *p;
	int port;

	switch (cmd) {
	case CLI_INIT:
		e->command = "gsm send at";
		e->usage = "Usage: gsm send at <port> \"<command>\"\n";
		return NULL;

	case CLI_GENERATE:
		return NULL;
	}

        if (a->argc != 5)
                return CLI_SHOWUSAGE;

        portstr = a->argv[3];
        command = strdup(a->argv[4]);

	port=atoi(portstr);

	CLI_CHECK_PORT(port);

	while( (p=strchr(command,'/')) )
		*p='?';

	gsm_send(port, command, 100);

	free (command);

	return 0;
}


/*************** Helpers END *************/
static void sighandler(int sig)
{}

static void* gsm_tasks_thread_func (void *data)
{
	int wait;
	struct sigaction sa;

	sa.sa_handler = sighandler;
	sa.sa_flags = SA_NODEFER;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGUSR1);
	sigaction(SIGUSR1, &sa, NULL);

	sem_post((sem_t *)data);

	while (1) {
		wait = ast_sched_wait(gsm_tasks);
		if (wait < 0)
			wait = 8000;
		if (poll(NULL, 0, wait) < 0)
			ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Waking up gsm_tasks thread\n");
		ast_sched_runq(gsm_tasks);
	}
	return NULL;
}

static void gsm_tasks_init (void)
{
	sem_t blocker;
	int i = 5;

	if (sem_init(&blocker, 0, 0)) {
		perror("chan_gsm: Failed to initialize semaphore!");
		exit(1);
	}

	ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Starting gsm_tasks thread\n");

	gsm_tasks = sched_context_create();
	pthread_create(&gsm_tasks_thread, NULL, gsm_tasks_thread_func, &blocker);

	while (sem_wait(&blocker) && --i);
	sem_destroy(&blocker);
}

static void gsm_tasks_destroy (void)
{
	if (gsm_tasks) {
		ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Killing gsm_tasks thread\n");
		if ( pthread_cancel(gsm_tasks_thread) == 0 ) {
			ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Joining gsm_tasks thread\n");
			pthread_join(gsm_tasks_thread, NULL);
		}
		sched_context_destroy(gsm_tasks);
		gsm_tasks = NULL;
	}
}

static inline void gsm_tasks_wakeup (void)
{
	pthread_kill(gsm_tasks_thread, SIGUSR1);
}

static inline int _gsm_tasks_add_variable (int timeout, ast_sched_cb callback, void *data, int variable)
{
	int task_id;

	if (!gsm_tasks) {
		gsm_tasks_init();
	}
	task_id = ast_sched_add_variable(gsm_tasks, timeout, callback, data, variable);
	gsm_tasks_wakeup();

	return task_id;
}

static int gsm_tasks_add (int timeout, ast_sched_cb callback, void *data)
{
	return _gsm_tasks_add_variable(timeout, callback, data, 0);
}
#if 0
static int gsm_check_homezone(const void *data)
{
	int port;

	for (port=0; port < MAX_GSM_PORTS; port++) {
		if (gsm_status[port].status == GSM_STATUS_REGISTERED) {
			gsm_send(port, "AT+CSQ", 0); /*query the signal strength */
			if (gsm_debug)
				gsm_log(GSM_DEBUG, "Check the homezone for port %d\n", port);
		}
	}

	return 1;
}
#endif
/* function to send sms messages */
static int gsm_sms_sender_func(const void *data)
{
	int msglen = sizeof(struct gsm_sms_message) - sizeof(long);
	struct ast_channel *ast;
	struct gsm_pvt *gsm=NULL;
	int port, retval;

	for (port=0; port < MAX_GSM_PORTS; port++) {
		/* the port must be registered, so that we can send SMS messages */
		/* the port must not be busy doing call setups, and we must have a message to send */
		/* note that we cannot and we do not block on this function */
		if ((gsm_status[port].status == GSM_STATUS_REGISTERED) && \
		    (gsm_sms[port].state == GSM_SMS_STATE_NONE)) {
			/* we don't send SMS messages during call establishment */
		 	if (gsm_slcc[port].ringing) {
				if (gsm_debug)
					gsm_log(GSM_DEBUG, "skipping port(%d) sms send as we have an incoming call\n", port);
				continue;
			}	
			/* we do this when we're idle or already in a call */
			if ((ast=gsm_get_priv(port))) {
				if ((gsm = ast->tech_pvt) && \
				    ((gsm->state == GSM_STATE_ATD_OUT) || \
				     (gsm->state == GSM_STATE_ALERTING) || \
				     (gsm->state == GSM_STATE_PROCEEDING))) {
					if (gsm_debug)
						gsm_log(GSM_DEBUG, "skipping port(%d) sms send as we are now setting up a call\n", port);
					continue;
				}
			}
			/* get the message and send it out */
			if ((retval = msgrcv(gsm_sms[port].sms_mq_id, &gsm_sms[port].outbound_msg, msglen, 1, IPC_NOWAIT)) > 0) {
				/* send the message */
				_gsm_send_sms(port, gsm_sms[port].outbound_msg.number, gsm_sms[port].outbound_msg.text);
				/* print more debug output here */
				if (gsm_debug) 
					gsm_log(GSM_WARNING, "sending SMS message (%s) to (%s) on port (%d)\n",
						gsm_sms[port].outbound_msg.text,
						gsm_sms[port].outbound_msg.number,
						port);
			} else {
				//if (gsm_debug)
				//	gsm_log(GSM_WARNING, "nothing to send from port (%d) got (%s)\n", port, strerror(errno));
			}
		}
	}

	return 1;
}

static int gsm_handle_cme_error(int port, int cme_error_code)
{
	struct ast_channel *ast=gsm_get_priv(port);
	struct gsm_pvt *gsm=NULL;

	if (ast)
		gsm=ast->tech_pvt;

	if (gsm_debug)
		gsm_log(GSM_DEBUG, "%s() received error code %d\n",
			__FUNCTION__, cme_error_code);
	
	switch (cme_error_code) {

		case CME_ERROR_PHONE_FAILURE ... CME_ERROR_EMERGENCY_CALLS_ONLY:
		case CME_ERROR_AUDIO_MANAGER_NOT_READY:
		case CME_ERROR_AUDIO_FORMAT_NOT_CONFIGURED:
		case CME_ERROR_CALL_SETUP_IN_PROGRESS:
			if (ast && gsm && gsm->state != GSM_STATE_HANGUPED) {
				gsm->state=GSM_STATE_HANGUPED;
				ast->hangupcause=3; /* check corresponding codes for each cme error code */
				ast_queue_hangup(ast);
			}
			break;

		default:
			break;
	}

	return 0;
}

static int gsm_handle_cms_error(int port, int cms_error_code)
{
	struct ast_channel *ast=gsm_get_priv(port);
	struct gsm_pvt *gsm=NULL;

	if (ast)
		gsm=ast->tech_pvt;

	if (gsm_debug)
		gsm_log(GSM_DEBUG, "%s() received error code %d\n",
			__FUNCTION__, cms_error_code);
	
	switch (cms_error_code) {

		case CMS_ERROR_ME_FAILURE ... CMS_ERROR_OPERATION_NOT_SUPPORTED:
			if (ast && gsm->state != GSM_STATE_HANGUPED) {
				gsm->state=GSM_STATE_HANGUPED;
				ast->hangupcause=3; /* check corresponding codes for each cms error code */
				ast_queue_hangup(ast);
			}
			break;

		default:
			break;
	}

	return 0;
}


static int gsm_config_get_bool(int sec, int parameter)
{
        char t[16];
        cm_get(config, t, sizeof(t), sec, parameter);
        return ast_true(t)==-1?1:ast_true(t);
}

/* query the call state for the instance of the channel driver */
static int gsm_query_call_state(const void *data)
{
	struct ast_channel *ast=NULL;
	struct gsm_pvt *gsm=NULL;
	int port;

	for (port=0; port < MAX_GSM_PORTS; port++) {
		if (gsm_port_is_valid(port) && \
		    (gsm_status[port].status == GSM_STATUS_REGISTERED))  {
			/* send query only if you're in one of dialing out/proceeding/alerting states for a given port */
			if ((ast=gsm_get_priv(port))) {
				if ((gsm = ast->tech_pvt) && \
				    ((gsm->state == GSM_STATE_ATD_OUT) || \
				     (gsm->state == GSM_STATE_ALERTING) || \
				     (gsm->state == GSM_STATE_PROCEEDING))) {	
					gsm_send(port, "AT+CLCC", 0);
				}
			}
		}
	}

	return 1;
}

static int gsm_update_lcd_status(const void *data)
{
       enum channel_state channel_state;
       int ii;

       for (ii = 0; ii < MAX_GSM_PORTS; ii++) {
               if ((gsm_port_is_valid(ii)) && (gsm_get_channel_state(ii, &channel_state) >= 0)) {
                       gsm_send_channel_state(ii, channel_state);
                       if (gsm_debug) 
				gsm_log(GSM_DEBUG, "sending state (%d) for channel (%d)\n", channel_state, ii);
                       usleep(500 * 1000);
               }
       }

       /* run this just once */
       return 0;
}

static int gsm_resetinterval(const void *data)
{
	int port=(int)data;

	if (port>0 && port < MAX_GSM_PORTS)  {
		if (!gsm_port_in_use_get(port)) {
			ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Restarting port: %d after resetinterval\n", port);
			_gsm_restart_port(port, 1);
		}
	}
	return 1;
}

static int _gsm_send_sms(int port, const char *number, const char *sms)
{
	strcpy(gsm_sms[port].number, number);
	strcpy(gsm_sms[port].sms, sms);

	if (gsm_sms[port].state != GSM_SMS_STATE_NONE) {
		ast_verbose(CHAN_GSM_VERBOSE_PREFIX "SMS Statemachine busy, try later\n");
		gsm_log(GSM_WARNING, "SMS Statemachine busy, try later\n");
		return -1;
	}

	gsm_sms[port].state = GSM_SMS_STATE_EXPECTING_OK;
	gsm_send(port, "AT+CMGF=1", 0); // SAM -- always sending text mode SMS

	return 0;
}

static int _gsm_queue_sms(int port, const char *number, const char *sms)
{
	struct gsm_sms_message outbound_msg; /* temporary message, where we work on*/
	int msglen = sizeof(outbound_msg) - sizeof(long);

	memset(&outbound_msg, 0, sizeof(outbound_msg));

	/* queue the gsm message */
	outbound_msg.mtype = 1;
	strcpy(outbound_msg.number, number);
	strcpy(outbound_msg.text, sms);

	if (msgsnd(gsm_sms[port].sms_mq_id, (void*)&outbound_msg, msglen, IPC_NOWAIT) < 0) {
		gsm_log(GSM_ERROR, "error queuing message (%s) to number (%s) err(%s)\n",
			sms, number, strerror(errno));
		return -1;
	}

	return 0;
}

static char* handle_gsm_send_sms(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{	
	int result;

	switch (cmd) {
	case CLI_INIT:
		e->command = "gsm send sms";
		e->usage = "Usage: gsm send sms <port> <number> \"<message>\"\n";
		return NULL;

	case CLI_GENERATE:
		return NULL;
	}

        if (a->argc != 6)
                return CLI_SHOWUSAGE;

	int port=strtol(a->argv[3], NULL, 10);

	CLI_CHECK_PORT(port);

	if (!gsm_port_is_valid(port)) {
		gsm_log(GSM_WARNING, "Port:'%d' is not configured, please review your gsm.conf\n",port);
		return CLI_FAILURE;
	}

	result = _gsm_queue_sms(port, a->argv[4], a->argv[5]);

	/* we print out ACK when we have successful receipt of the message */
	if (result < 0)
		gsm_log(GSM_ERROR, "error queuing SMS message\n");

	return (result >= 0) ? CLI_SUCCESS : CLI_FAILURE;
}


static int _gsm_list_sms(int port)
{
	if (gsm_cfg[port].sms_pdu_mode) {
		/* the mode is already set at initialization */
		/* ask a list of all messages, so change the sms state to something other than NONE */
		gsm_sms[port].state=GSM_SMS_STATE_EXPECTING_SMSLIST;
		gsm_send(port, "AT+CMGL=4",100);
	} else {
		/* the mode is already set at initialization */
		/* ask a list of all messages, so change the sms state to something other than NONE */
		gsm_sms[port].state=GSM_SMS_STATE_EXPECTING_SMSLIST;
		gsm_send(port, "AT+CMGL=\"ALL\"",100);
	}

	return 0;
}

static char* handle_gsm_fetch_sms(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	switch (cmd) {
	case CLI_INIT:
		e->command = "gsm fetch sms";
		e->usage = "Usage: gsm fetch sms <port>\n";
		return NULL;

	case CLI_GENERATE:
		return NULL;
	}

	if (a->argc != 4)
		return CLI_SHOWUSAGE;

	int port=strtol(a->argv[3], NULL, 10);

	CLI_CHECK_PORT(port);

	if (!gsm_port_is_valid(port)) {
		gsm_log(GSM_WARNING, "Port:'%d' is not configured, please review your gsm.conf\n",port);
		return CLI_FAILURE;
	}

	gsm_sms[port].work=GSM_SMS_WORK_FETCH;
	_gsm_list_sms(port);
	return CLI_SUCCESS;
}

static char* handle_gsm_show_sms(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	switch (cmd) {
	case CLI_INIT:
		e->command = "gsm show sms";
		e->usage = "Usage: gsm show sms <port>\n";
		return NULL;

	case CLI_GENERATE:
		return NULL;
	}

	if (a->argc != 4)
		return CLI_SHOWUSAGE;

	int port=strtol(a->argv[3], NULL, 10);

	CLI_CHECK_PORT(port);

	if (!gsm_port_is_valid(port)) {
		gsm_log(GSM_WARNING, "Port:'%d' is not configured, please review your gsm.conf\n",port);
		return CLI_FAILURE;
	}

	gsm_sms[port].work=GSM_SMS_WORK_SHOW;
	_gsm_list_sms(port);
	return CLI_SUCCESS;
}

static char* handle_gsm_set_debug(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	switch (cmd) {
	case CLI_INIT:
		e->command = "gsm set debug";
		e->usage = "Usage: gsm set debug <flag>\n";
		return NULL;

	case CLI_GENERATE:
		return NULL;
	}

        if (a->argc != 4)
                return CLI_SHOWUSAGE;

	gsm_debug=atoi(a->argv[3]);

	gsm_set_debuglevel(gsm_debug);

	return CLI_SUCCESS;
}


static char* handle_gsm_debug_info(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	int port;

	switch (cmd) {
	case CLI_INIT:
		e->command = "gsm debug info";
		e->usage = "Usage: gsm debug info\n";
		return NULL;

	case CLI_GENERATE:
		return NULL;
	}


	for (port = 0; port < MAX_GSM_PORTS; port++) {
		if (gsm_port_is_valid(port)) 
			ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Sms_State (%d) sms_work (%d)\n", gsm_sms[port].state, gsm_sms[port].work);
	}
		
	for (port = 0; port < MAX_GSM_PORTS; port++) {
		if (gsm_port_is_valid(port)) 
			ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Port %d IMEI (%-15s) Firmware (%s)\n", port, gsm_get_imei_number(port), gsm_get_firmware_info(port));
	}

	for (port = 0; port < MAX_GSM_PORTS; port++) {
		if (gsm_port_is_valid(port)) 
			ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Port %d IMSI (%-15s)\n", port, gsm_get_imsi_number(port));
	}

	return 0;
}


static char* handle_gsm_shutdown_port(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{

	switch (cmd) {
	case CLI_INIT:
		e->command = "gsm shutdown port";
		e->usage = "Usage: gsm shutdown port <port>\n";
		return NULL;

	case CLI_GENERATE:
		return NULL;
	}

	if (a->argc != 4)
		return CLI_SHOWUSAGE;

	int port=strtol(a->argv[3], NULL, 10);

	CLI_CHECK_PORT(port);

	if (!gsm_port_is_valid(port)) {
		gsm_log(GSM_WARNING, "Port:'%d' is not configured, please review your gsm.conf\n",port);
		return CLI_FAILURE;
	}

	_gsm_shutdown_port(port, 0);

	return CLI_SUCCESS;
}


int _gsm_shutdown_port(int port, int powercycle)
{
	ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Shutting down port %d\n",port);
	gsm_status[port].status=GSM_STATUS_UNINITIALIZED;
	gsm_shutdown_port(port, powercycle);

	return 0;
}

int _gsm_init_port(int port)
{
#if 0
	struct gsm_config cfg;

	if (!gsm_port_is_valid(port)) {
		ast_log(LOG_WARNING, "Port:'%d' is not configured, please review your gsm.conf\n",port);
		return -1;
	}
#endif
	if (gsm_debug)
		gsm_log(GSM_DEBUG, "%s() : Initializing gsm port %d\n",
			__FUNCTION__, port);

	if (!gsm_check_port(port)) {
		gsm_log(GSM_ERROR , "port=(%d) device doesnot exist\n", port);
		return -1;
	}

	ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Initializing port %d\n",port);

	_init_mod_port(port);

	gsm_init_port(port, &gsm_cfg[port]);
	if (gsm_wait_ready_with_timeout(port, GSM_READY_TIMEOUT) == 0) {
		ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Started gsm port %d\n",port);
	} else {
		ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Failed starting gsm port %d\n", port);
		gsm_log(GSM_ERROR, "Error starting gsm port %d\n", port);
	}

	if (gsm_debug)
		gsm_log(GSM_DEBUG, "%s() : Started gsm port %d\n",
			__FUNCTION__, port);

	return 0;
}

static char* handle_gsm_init_port(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	
	switch (cmd) {
	case CLI_INIT:
		e->command = "gsm init port";
		e->usage = "Usage: gsm init port <port>\n";
		return NULL;

	case CLI_GENERATE:
		return NULL;
	}

        if (a->argc != 4)
                return CLI_SHOWUSAGE;

	int port = atoi(a->argv[3]);

	CLI_CHECK_PORT(port);

	if (gsm_status[port].status != GSM_STATUS_UNINITIALIZED) {
		ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Port (%d) already initialized.\n", port);
		return CLI_FAILURE;
	} else {
		_gsm_init_port(port);
		return CLI_SUCCESS;
	}
}


static char* handle_gsm_set_next_simslot(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	int port, ns;

	switch (cmd) {
	case CLI_INIT:
		e->command = "gsm set next simslot";
		e->usage = "Usage: gsm set next simslot <port> <nextslot>\n";
		return NULL;

	case CLI_GENERATE:
		return NULL;
	}

	if (a->argc != 6)
    		return CLI_SHOWUSAGE;

	port=strtol(a->argv[4],NULL,10);
	ns=strtol(a->argv[5],NULL,10);

	CLI_CHECK_PORT(port);

	if (!gsm_port_is_valid(port)) {
		ast_log(LOG_WARNING, "Port:'%d' is not configured, please review your gsm.conf\n",port);
		return CLI_FAILURE;
	}

	int i=gsm_set_next_simslot(port, ns);

	if (i<0)
		ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Setting simslot failed (%s)\n", strerror(errno));

	return CLI_SUCCESS;
}

static void _gsm_restart_port_sync(int port, int postwait)
{
	unsigned long then = time(NULL);

	if (!gsm_port_is_valid(port)) {
		gsm_log(GSM_WARNING, "Port:'%d' is not configured, please review your gsm.conf (not restarting)\n",port);
		return ;
	}

	/* if we're doing a normal (sync) restart, =do not= power cycle the module */
	/* as there is no need and it takes longer to reinitialize that way.       */
	_gsm_shutdown_port(port, 0);
	sleep(3);
	_gsm_init_port(port);

	//Wait for REGISTER EVENT
	sleep(5);
	unsigned long now = time(NULL);
	ast_verbose(CHAN_GSM_VERBOSE_PREFIX "It took us %ld seconds\n", now-then);

	if (gsm_debug)
		gsm_log(GSM_DEBUG, "It took us %ld seconds\n", now-then);
}
#if 0
/* we could just use the _sync() version of the function */
static void* _gsm_restart_thread(void *data)
{
	int port = (int)data;

	unsigned long then = time(NULL);

	if (!gsm_port_is_valid(port)) {
		gsm_log(GSM_WARNING, "Port:'%d' is not configured, please review your gsm.conf (not restarting)\n",port);
		return NULL;
	}

	_gsm_shutdown_port(port, 0);
	sleep(3);
	_gsm_init_port(port);

	//Wait for REGISTER EVENT
	sleep(5);
	unsigned long now = time(NULL);
	ast_verbose(CHAN_GSM_VERBOSE_PREFIX "It took us %ld seconds\n", now-then);

	if (gsm_debug)
		gsm_log(GSM_DEBUG, "It took us %ld seconds\n", now-then);

	return NULL;
}
#endif
#if 0
static void _gsm_restart_port_async(int port, int postwait)
{
	pthread_create(&_gsm_restart_threads[port], NULL, _gsm_restart_thread, (void*)port); 
}
#endif
static void _gsm_restart_port(int port, int postwait)
{
	_gsm_restart_port_sync(port, postwait);
}

static char* handle_gsm_restart_port(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	int port;

	switch (cmd) {
	case CLI_INIT:
		e->command = "gsm restart port";
		e->usage = "Usage: gsm restart port <port>\n";
		return NULL;

	case CLI_GENERATE:
		return NULL;
	}


        if (a->argc != 4)
                return CLI_SHOWUSAGE;

	port=strtol(a->argv[3], NULL, 10);

	CLI_CHECK_PORT(port);

	_gsm_restart_port(port, 0);

	return CLI_SUCCESS;
}

struct gsm_op {
	char name[128];
	char origname[128];
	char number[32];
} ;

static char* handle_gsm_show_ops(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	int port;

	switch (cmd) {
	case CLI_INIT:
		e->command = "gsm show ops";
		e->usage = "Usage: gsm show ops <port>\n";
		return NULL;

	case CLI_GENERATE:
		return NULL;
	}

        if (a->argc != 4)
                return CLI_SHOWUSAGE;

	port=strtol(a->argv[3], NULL, 10);

	CLI_CHECK_PORT(port);

	gsm_send(port, "AT+COPN", 100);
	ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Getting Operator List, please stand by\n");

	return CLI_SUCCESS;
}

static char* handle_gsm_set_op(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{

	switch (cmd) {
	case CLI_INIT:
		e->command = "gsm set op";
		e->usage = "Usage: gsm set op <port> \"<operator>\"\n";
		return NULL;

	case CLI_GENERATE:
		return NULL;
	}

        if (a->argc != 5)
                return CLI_SHOWUSAGE;

	char buf[64];
	int port;

	port=strtol(a->argv[3], NULL, 10);

	CLI_CHECK_PORT(port);

	sprintf(buf,"AT+COPS=1,2,%s",a->argv[4]);
	gsm_send(port, buf, 100);

	ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Setting Operator, please stand by\n");
	return CLI_SUCCESS;
}

static char* handle_gsm_show_status(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	int i;

	switch (cmd) {
	case CLI_INIT:
		e->command = "gsm show status";
		e->usage = "Usage: gsm show status\n";
		return NULL;

	case CLI_GENERATE:
		return NULL;
	}


	/* perform the signal strength query for all channels */
	for (i=1; i<MAX_GSM_PORTS; i++) {
		if ((gsm_status[i].status == GSM_STATUS_INITIALIZED) && \
		(gsm_status[i].status == GSM_STATUS_UNREGISTERED) && \
		(gsm_status[i].status == GSM_STATUS_REGISTERED) && \
		(gsm_port_is_valid(i)))
		gsm_send(i, "AT+CSQ", 200);
	}
 
	ast_cli(a->fd, "\nPort\tStatus\t\tInUse\tProvider\tHome Zone\tSignal Quality\n\n");
	for (i=1; i<MAX_GSM_PORTS; i++) {
		/* query the signal strength for each channel */
		if (    gsm_status[i].status != GSM_STATUS_UNAVAILABLE &&
			gsm_status[i].status != GSM_STATUS_NOTCONFIGURED ) {
				ast_cli(a->fd,"%d (%d)\t%s\t%c\t%s\t%s\t\t%s\n",
							i, gsm_simslot_get(i),
							status2txt[gsm_status[i].status],
							gsm_port_in_use_get(i)?'y':'n',
							gsm_status[i].provider,
							gsm_status[i].homezone?"y":"n",
							rssi2txt(gsm_status[i].rssi)
							);
		}
	}
	ast_cli(a->fd, "\n");
	return CLI_SUCCESS;
}


static char* handle_gsm_show_version(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{

	switch (cmd) {
	case CLI_INIT:
		e->command = "gsm show version";
		e->usage = "Usage: gsm show version\n";
		return NULL;

	case CLI_GENERATE:
		return NULL;
	}

	//ast_cli(a->fd,"\nchan_gsm version (" CHAN_GSM_VERSION ")\n");
	ast_cli(a->fd,"\nchan_gsm version (trunk)\n");
	return CLI_SUCCESS;
}

static char* handle_gsm_peek(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	unsigned reg;
	char *reg_p;
	struct gsm_req my_gsm_req;

	switch (cmd) {
	case CLI_INIT:
		e->command = "gsm peek";
		e->usage = "Usage: gsm peek <offset>\n";
		return NULL;

	case CLI_GENERATE:
		return NULL;
	}


	if (a->argc != 3) 
                return CLI_SHOWUSAGE;

	reg = strtoul(a->argv[2], &reg_p, 16); 

	if (*reg_p != '\0') {
		ast_cli(a->fd,"Invalid offset [%s]\n", a->argv[2]);
		return CLI_SHOWUSAGE;
	}

	if ((reg < 0) || (reg > MAX_FPGA_OFFSET)) {
		ast_cli(a->fd,"Offset [%s] exceeds valid range [0..%x]\n", a->argv[2], MAX_FPGA_OFFSET);	
		return CLI_SHOWUSAGE;
	}

	memset(&my_gsm_req, 0, sizeof(my_gsm_req));
	my_gsm_req.type = GSM_PEEK_FPGA;
	my_gsm_req.param1 = reg;

	if (!gsm_do_ioctl(1, GSM_PEEK_FPGA, (int)&my_gsm_req))
		ast_cli(a->fd,"register:0x%08x value:0x%08x\n", reg, my_gsm_req.param2);
	else
		ast_cli(a->fd,"Invalid register access/error\n");

	return CLI_SUCCESS;
}

static char* handle_gsm_poke(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	unsigned reg;
	unsigned value;
	char *reg_p, *value_p;
	struct gsm_req my_gsm_req;

	switch (cmd) {
	case CLI_INIT:
		e->command = "gsm poke";
		e->usage = "Usage: gsm poke <offset> <value>\n";
		return NULL;

	case CLI_GENERATE:
		return NULL;
	}


	if (a->argc != 4) 
                return CLI_SHOWUSAGE;

	reg = strtoul(a->argv[2], &reg_p, 16); 

	if (*reg_p != '\0') {
		ast_cli(a->fd,"Invalid offset [%s]\n", a->argv[2]);
		return CLI_SHOWUSAGE;
	}

	if ((reg < 0) || (reg > MAX_FPGA_OFFSET)) {
		ast_cli(a->fd,"Offset [%s] exceeds valid range [0..%x]\n", a->argv[2], MAX_FPGA_OFFSET);	
		return CLI_SHOWUSAGE;
	}

	value = strtoul(a->argv[3], &value_p, 16); 

	if (*value_p != '\0') {
		ast_cli(a->fd,"Invalid value [%s]\n", a->argv[3]);
		return CLI_SHOWUSAGE;
	}

	memset(&my_gsm_req, 0, sizeof(my_gsm_req));
	my_gsm_req.type = GSM_POKE_FPGA;
	my_gsm_req.param1 = reg;
	my_gsm_req.param2 = value;

	if (!gsm_do_ioctl(1, GSM_POKE_FPGA, (int)&my_gsm_req))
		ast_cli(a->fd,"register:0x%08x value:0x%08x\n", reg, my_gsm_req.param2);
	else
		ast_cli(a->fd,"Invalid register access/error\n");

	return CLI_SUCCESS;
}


static struct ast_cli_entry chan_gsm_clis[] = {
	AST_CLI_DEFINE(handle_gsm_send_at, "send AT command to port <port>"),
	AST_CLI_DEFINE(handle_gsm_send_sms, "send SMS to port <port> with number <number>"),
	AST_CLI_DEFINE(handle_gsm_fetch_sms, "fetch all the stored SMS from port <port> and store them in smsdir\n"),
	AST_CLI_DEFINE(handle_gsm_show_sms, "show all the stored SMS rom port <port>\n"),
	AST_CLI_DEFINE(handle_gsm_set_debug, "set the debuglevel to <level>\n"),
	AST_CLI_DEFINE(handle_gsm_debug_info, "print debug information\n"),
	AST_CLI_DEFINE(handle_gsm_shutdown_port, "shut down port <port>\n"),
	AST_CLI_DEFINE(handle_gsm_init_port, "initialize given <port>\n"),
	AST_CLI_DEFINE(handle_gsm_set_next_simslot, "set next sim slot to <slot>\n"),
	AST_CLI_DEFINE(handle_gsm_restart_port, "restart gsm port <port>\n"),
	AST_CLI_DEFINE(handle_gsm_show_ops, "show list of operators\n"),
	AST_CLI_DEFINE(handle_gsm_show_status, "show status of gsm modules\n"),
	AST_CLI_DEFINE(handle_gsm_show_version, "show version of the gsm channel driver\n"),
	AST_CLI_DEFINE(handle_gsm_peek, "read value of a register in FPGA\n"),
	AST_CLI_DEFINE(handle_gsm_poke, "write value to a given register in FPGA\n"),
	AST_CLI_DEFINE(handle_gsm_set_op, "set an operator\n"),
	AST_CLI_DEFINE(handle_gsm_request_state, "only used by astmanproxy\n"),
	AST_CLI_DEFINE(handle_gsm_show_info, "show GSM information\n"),
};

/*manager commands*/
#ifdef ASTERISK_1_4
static int action_send_sms(struct mansession *s, const struct message *m)
#else
static int action_send_sms(struct mansession *s, struct message *m)
#endif
{
        const char *port= astman_get_header(m, "Port");
        const char *number= astman_get_header(m, "Number");
        const char *text= astman_get_header(m, "Text");

        if (ast_strlen_zero(port)) {
                astman_send_error(s, m, "No port specified");
                return 0;
        }
        if (ast_strlen_zero(number)) {
                astman_send_error(s, m, "No number specified");
                return 0;
        }
        if (ast_strlen_zero(text)) {
                astman_send_error(s, m, "No text specified");
                return 0;
        }

	gsm_sms[atoi(port)].s=s;
	gsm_sms[atoi(port)].m=(struct message*)m;

	_gsm_queue_sms(atoi(port), number, text);


	astman_send_ack(s, m, "GsmSendSms: Success");
        return 0;
}

/*manager commands*/
#ifdef ASTERISK_1_4
static int action_check_sms_state(struct mansession *s, const struct message *m)
#else
static int action_check_sms_state(struct mansession *s, struct message *m)
#endif
{
        const char *port= astman_get_header(m, "Port");

        if (ast_strlen_zero(port)) {
                astman_send_error(s, m, "No port specified");
                return 0;
        }

	char ack[256];
	if (gsm_sms[atoi(port)].state != GSM_SMS_STATE_NONE)
		sprintf(ack, "GsmCheckSMSState: State-Machine Busy (%d)", gsm_sms[atoi(port)].state);
	else
		sprintf(ack, "GsmCheckSMSState: State-Machine IDLE");

	astman_send_ack(s, m, ack);

        return 0;
}


#ifdef ASTERISK_1_4
/*
 *  * some of the standard methods supported by channels.
 *   */
static int gsm_digit_begin(struct ast_channel *c, char digit)
{
	if (gsm_config_get_bool(SECT_GENERAL, CFG_GEN_DTMFBEGIN)) {
		struct gsm_pvt *gsm=c->tech_pvt;
        	char buf[16];

        	if (!gsm) return -1;

	        ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Sending DTMF %c\n", digit);
		if (gsm_debug)
			gsm_log(GSM_DEBUG, "Sending DTMF %c\n", digit);
	        sprintf(buf,"at+vts=%c", digit);
        	gsm_send(gsm->port,buf,100);
	}	

	return 0;
}
#endif

#ifdef ASTERISK_1_4
static int gsm_digit_end(struct ast_channel *c, char digit, unsigned int duration)
#else
static int gsm_digit_end(struct ast_channel *c, char digit)
#endif
{
	if (!gsm_config_get_bool(SECT_GENERAL, CFG_GEN_DTMFBEGIN)) {
		struct gsm_pvt *gsm=c->tech_pvt;
	        char buf[16];

	        if (!gsm) return -1;
		
        	ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Sending DTMF %c\n", digit);
		if (gsm_debug)
			gsm_log(GSM_DEBUG, "Sending DTMF %c\n", digit);
        	sprintf(buf,"at+vts=%c", digit);
	        gsm_send(gsm->port,buf,100);
	}

	return 0;
}


static int gsm_text(struct ast_channel *c, const char *text)
{
        /* print received messages */
        ast_verbose(CHAN_GSM_VERBOSE_PREFIX " << GSM Received text %s >> \n", text);
        return 0;
}

static int gsm_call(struct ast_channel *c, char *dest, int timeout)
{
	struct gsm_pvt *gsm=c->tech_pvt;
	int hidecallerid=0;

	if (!gsm) return -1;

	char buf[128];
	char *port_str, *dad, *p;

	ast_copy_string(buf, dest, sizeof(buf)-1);
	p=buf;
	port_str=strsep(&p, "/");
	dad=strsep(&p, "/");

	if (gsm_debug)
		ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Call: ext:%s port:%d dest:(%s) -> dad(%s) \n", c->exten, gsm->port, dest, dad);

	if (gsm_debug)
		gsm_log(GSM_DEBUG, "Call: ext:%s port:%d dest:(%s) -> dad(%s) \n", c->exten, gsm->port, dest, dad);

	char dialstring[128];

	if (gsm->hidecallerid) {
		hidecallerid=1;
	}

	if (hidecallerid)
		sprintf(dialstring,"ATD#31#%s;",dad);
	else
		sprintf(dialstring,"ATD%s;",dad);

	if (gsm_debug)
		gsm_log(GSM_DEBUG, "Dialstring is '%s'\n",dialstring);
	/* cannot use fetch_port() here as it has already been done */
	if (!gsm_slcc[gsm->port].ringing) {
		gsm_send(gsm->port,dialstring,300);
		gsm->state=GSM_STATE_ATD_OUT;
	} else {
		gsm_log(GSM_WARNING, "Call: ext:%s port:%d is busy (has an incoming call).\n", c->exten, gsm->port);
		return -1;
	}

	return 0;
}

static int gsm_answer(struct ast_channel *c)
{
	struct gsm_pvt *gsm=c->tech_pvt;

	if (gsm_debug)
		gsm_log(GSM_DEBUG, "Answering call on port:%d\n", gsm->port);

	gsm_send(gsm->port,"ATA",100);
	gsm_slcc[gsm->port].ringing = 0;

	if (gsm_debug) 
		gsm_log(GSM_DEBUG, "Updating LCD for port:%d to CHANNEL_STATE_IN_USE\n", gsm->port);

	gsm_set_channel_state(gsm->port, CHANNEL_STATE_IN_USE);
	gsm_send_channel_state(gsm->port, CHANNEL_STATE_IN_USE);
	return 0;
}

static int gsm_hangup(struct ast_channel *c)
{
	struct gsm_pvt *gsm=c->tech_pvt;
	int port=gsm->port;

	gsm_put_priv(gsm->port,NULL);

	if (gsm_debug)
		gsm_log(GSM_DEBUG, "Hanging up call on port:%d state:%d\n", gsm->port, gsm->state);

	if (gsm->state != GSM_STATE_COLLISION)
		gsm_send(gsm->port,"ATH",100);

	gsm_audio_stop(gsm->port);
	gsm_audio_close(gsm->port, gsm->portfd);

	if (gsm->dsp)
			ast_dsp_free(gsm->dsp);
	if (gsm->trans)
			ast_translator_free_path(gsm->trans);

	free(gsm);
	c->tech_pvt=NULL;

	/* ringing cleared here with everything else */
	memset(&gsm_slcc[port],0,sizeof(gsm_slcc[port]));

	gsm_port_release(port);

	if (gsm_debug)
		gsm_log(GSM_DEBUG, "Updating LCD for port:%d to CHANNEL_STATE_READY\n", gsm->port);

	/* update the LCD */
	gsm_set_channel_state(gsm->port, CHANNEL_STATE_READY);
	gsm_send_channel_state(port, CHANNEL_STATE_READY);

	return 0;
}

static int gsm_write(struct ast_channel *c, struct ast_frame *f)
{
        struct gsm_pvt*gsm= c->tech_pvt;
        int count = 0,
                again = 1,
                len;

        switch (f->frametype) {
        case AST_FRAME_VOICE:
                do {
                        len = write(gsm->portfd, (char*)f->data.ptr, f->datalen);

                        if (len < 0) {

				if (errno == EAGAIN) {
                                	gsm_log(GSM_DEBUG, "droping frame, EAGAIN\n");
					return 0;
				}

                                gsm_log(GSM_WARNING, "%d: error writing to port fd(%d) error(%d,%s)\n", gsm->port, gsm->portfd, errno, strerror(errno));
                                ast_queue_hangup(c);
                                return 0;
                        }
                        count += len;
                } while (again-- && count < f->datalen);
                if (count < f->datalen) {
                      //  gsm_log(GSM_WARNING, "%d: port congestion, dropping %d of %d frames\n", gsm->port, f->datalen - count, f->datalen);
					}
                break;
        default:
                gsm_log(GSM_WARNING, "%d unhandled frame type: %d\n",gsm->port, f->frametype);
                return -1;
        }


	return 0;
}

static struct ast_frame *gsm_read(struct ast_channel *c)
{
	struct gsm_pvt *gsm = c->tech_pvt;
	int count;

	count = read(gsm->portfd, gsm->readbuf, sizeof(gsm->readbuf));

	gsm->read_f.samples = count;
	gsm->read_f.datalen = count;
	gsm->read_f.delivery = ast_tv(0,0);
	gsm->read_f.data.ptr = gsm->readbuf;
	gsm->read_f.mallocd = 0;
	gsm->read_f.frametype= AST_FRAME_VOICE;
	gsm->read_f.subclass= GSM_AUDIO_FORMAT;

	if (gsm->trans) {

		struct ast_frame *f, *f2;
		f2=ast_translate(gsm->trans, &gsm->read_f,0);

		if (gsm->dsp) {
			f=ast_dsp_process(c, gsm->dsp, f2);
			if (f && f->frametype == AST_FRAME_DTMF) {
				gsm_log(GSM_NOTICE,"P(%d) Detected DTMF '%c'\n",gsm->port, f->subclass);
				ast_verbose(CHAN_GSM_VERBOSE_PREFIX "P(%d) Detected DTMF '%c'\n",gsm->port, f->subclass);
				return f;
			}
		}
	}

	return &gsm->read_f;
}

static int gsm_fixup(struct ast_channel *oldchan, struct ast_channel *newchan)
{
	struct gsm_pvt *p = newchan->tech_pvt;

	if (p->ast_chan == oldchan) {
		p->ast_chan = newchan;
	}

	gsm_masq_priv(oldchan,newchan);

        return 0;
}

#ifdef ASTERISK_1_4
static int gsm_indicate(struct ast_channel *c, int cond, const void *data, size_t datalen)
#else
static int gsm_indicate(struct ast_channel *c, int cond)
#endif
{
        int res = -1;

        switch (cond) {
                case AST_CONTROL_BUSY:
                case AST_CONTROL_CONGESTION:
                case AST_CONTROL_RINGING:
			return -1;
                case -1:
                        return 0;

                case AST_CONTROL_VIDUPDATE:
                        res = -1;
                        break;
                case AST_CONTROL_HOLD:
                        //ast_moh_start(c, data, g->mohinterpret);
                        break;
                case AST_CONTROL_UNHOLD:
                        //ast_moh_stop(c);
                        break;
                default:
                        //gsm_log(GSM_NOTICE, "Don't know how to display condition %d on %s\n", cond, c->name);
                        return -1;
        }

        return 0;
}

static struct ast_channel * gsm_new (struct gsm_pvt *gsm, const char *exten, const char *cidnum, char *cidname, const int state)
{
        struct ast_channel *tmp;
	char gsmname[128];
	sprintf(gsmname,"GSM/%d", gsm->port);

	if (gsm_debug)
		gsm_log(GSM_DEBUG, "Allocating new channel for port:%d\n", gsm->port);

#ifdef ASTERISK_1_4
        //tmp = ast_channel_alloc(1, state, cidnum , cidname, gsmname);
	tmp = ast_channel_alloc(1, state, cidnum, cidname, "", exten, "", 0, gsmname);
#else
        tmp = ast_channel_alloc(1);

#endif

        if (tmp) {
#ifdef ASTERISK_1_4
                ast_string_field_build(tmp, name, "%s", gsmname);
#else
                sprintf(tmp->name, "%s", gsmname);
				ast_setstate(tmp, state);

		tmp->type=gsm_tech.type;
#endif
                tmp->tech = &gsm_tech;
                tmp->tech_pvt = gsm;

		gsm->portfd=gsm_audio_open(gsm->port);

		if (gsm->portfd>=0) {
             		tmp->fds[0] = gsm->portfd;
		} else {
			gsm_log(GSM_WARNING, "Could not open audio fd (%s)\n", strerror(errno));
		}

                tmp->nativeformats = GSM_AUDIO_FORMAT;
                tmp->readformat = GSM_AUDIO_FORMAT;
                tmp->writeformat = GSM_AUDIO_FORMAT;
			
                if (exten)
                        ast_copy_string(tmp->exten, exten, AST_MAX_EXTENSION);

                ast_copy_string(tmp->context, gsm->context, AST_MAX_CONTEXT);

#ifdef ASTERISK_1_4
		if (cidnum) {
                        tmp->cid.cid_num = ast_strdup(cidnum);
                        tmp->cid.cid_ani = ast_strdup(cidnum);
		}

		if (cidname)
                        tmp->cid.cid_name = ast_strdup(cidname);
#else
		if (cidnum) {
                        tmp->cid.cid_num = strdup(cidnum);
                        tmp->cid.cid_ani = strdup(cidnum);
		}

		if (cidname)
                        tmp->cid.cid_name = strdup(cidname);
#endif


#if 0
		ast_mutex_lock(&bnxgsm_usecnt_lock);
                ++bnxgsm_usecnt;
                ast_mutex_unlock(&bnxgsm_usecnt_lock);
#endif
                ast_update_use_count();
        }

        gsm->ast_chan = tmp;

	gsm->dsp=ast_dsp_new();
	
	if (gsm->dsp) {
		ast_dsp_set_features(gsm->dsp, DSP_FEATURE_DIGIT_DETECT);
		if (gsm_config_get_bool(SECT_GENERAL, CFG_GEN_RELAXDTMF))
			ast_dsp_set_digitmode(gsm->dsp,
					DSP_DIGITMODE_DTMF |
					DSP_DIGITMODE_RELAXDTMF); // SAM
	} else { 
		gsm_log(GSM_ERROR, " NO DSP!!!!!\n"); // SAM DBG
	}

	gsm->trans=ast_translator_build_path(AST_FORMAT_SLINEAR, GSM_AUDIO_FORMAT);

	gsm_put_priv(gsm->port, tmp);
	gsm->state=GSM_STATE_NONE;

	return tmp;
}

#define gsm_cfg_get_array(name, buf, target, slot)  {char *p1, *p2, *s; \
		s=buf;\
		/*ast_verbose("CFG: element '%s' splitting '%s'\n",name, buf);*/\
		p1=strsep(&s, ","); p2=strsep(&s, ",");\
			if (slot && p2 ) {\
				strcpy(target,p2);\
			} else if (p1)  {\
				strcpy(target,p1);\
			} else {\
				strcpy(target,buf);\
			}\
			/*ast_verbose("CFG: using '%s'\n",target);*/\
		}


static void configure_gsm(struct gsm_pvt *gsm)
{
	int simslot=gsm_simslot_get(gsm->port);
	char context[sizeof(gsm->context)];
	char exten[sizeof(gsm->exten)];
	char hc[32];

	if (gsm_debug)
		gsm_log(GSM_DEBUG, "Configuring gsm channel for port:%d\n", gsm->port);

	if (cm_get(config, context, sizeof(context), SECT_PORT, CFG_PORT_CONTEXT, gsm->port)) {
		strcpy(context,default_context);
	}

	gsm_cfg_get_array("context", context, gsm->context, simslot);

	if (cm_get(config, exten, sizeof(exten), SECT_PORT, CFG_PORT_EXTEN, gsm->port)) {
		strcpy(exten,default_exten);
	}

	gsm_cfg_get_array("exten", exten, gsm->exten, simslot);

	if (cm_get(config, hc, sizeof(hc), SECT_PORT, CFG_PORT_HIDECALLERID, gsm->port)) {
		strcpy(hc,"no");
	}

	gsm_cfg_get_array("hidecallerid",hc, hc, simslot);
	gsm->hidecallerid=ast_true(hc);

}

static int check_and_fetch_port(int port)
{
	if (!gsm_port_is_valid(port)) {
		gsm_log(GSM_WARNING, "Port:'%d' is not configured, please review your gsm.conf\n",port);
		return -1;
	}

	if (gsm_status[port].status != GSM_STATUS_REGISTERED) {
		gsm_log(GSM_WARNING, "There is no Registration for port :%d\n",port);
		return -1;
	}

	/* gsm_port_fetch is the last Test which might return NULL, else we need to
	 * gsm_port_release, because the in_use flag would stick otherwise */
	if (!gsm_port_fetch(port)) {
		gsm_log(GSM_WARNING, "There is already a call on port :%d\n",port);
		return -1;
	}

	return 0;
}


void *csel_occupy (void *p)
{
	int port = (int)p;

	if (!check_and_fetch_port(port))
		return p;

	return NULL;
}

static struct ast_channel *gsm_request(const char *type, int format, void *data, int *cause)
{

	char buf[128];
	char *port_str, *ext, *p;
	int port;

	ast_copy_string(buf, data, sizeof(buf)-1);
	p=buf;
	port_str=strsep(&p, "/");
	ext=strsep(&p, "/");
	ast_verbose(CHAN_GSM_VERBOSE_PREFIX "portstr:%s ext:%s\n",port_str, ext);

	if (gsm_debug)
		gsm_log(GSM_DEBUG, "portstr:%s ext:%s\n",port_str, ext);

	if (port_str[0] == 'g' && port_str[1] == ':') {
		char *group = &port_str[2];
		struct csel *cs=NULL;
		int i;
		//Group Call, so let's find a free port
		ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Group call on group: %s\n", group);

		for (i=0;i<MAX_GSM_PORTS; i++) {
			if (csel_groups[i].name &&
				!strcasecmp(csel_groups[i].name, group)) {
					cs=csel_groups[i].cs;
					break;
				}
		}

		if (cs) {
			void *p=csel_get_next(cs);
			if (!p) {
				gsm_log(GSM_WARNING, "No free port in group: %s\n",group);
				return NULL;
			}

			port=(int)p;
		} else {
			gsm_log(GSM_WARNING,"group: %s not configured\n",group);
			return NULL;
		}

	} else {
		port=strtol(port_str, NULL, 10);

		if (check_and_fetch_port(port)<0)
			return NULL;
	}

	struct gsm_pvt *gsm=malloc(sizeof(struct gsm_pvt));

	if (!gsm) 
		return NULL;

	memset(gsm,0,sizeof(struct gsm_pvt));
	
	gsm->port=port;

	configure_gsm(gsm);

	if (gsm_debug)
		gsm_log(GSM_DEBUG, "Updating LCD for port:%d to CHANNEL_STATE_IN_USE\n", gsm->port);

	gsm_set_channel_state(gsm->port, CHANNEL_STATE_IN_USE);
	gsm_send_channel_state(gsm->port, CHANNEL_STATE_IN_USE);

	return gsm_new(gsm, ext, NULL, NULL, AST_STATE_RINGING);
}


/*sms functions*/
static void output_sms(int port, struct gsm_sms_message *msg)
{
	ast_verbose(CHAN_GSM_VERBOSE_PREFIX "P(%d) SMS RECEIVED: Idx(%d) Number(%s) Date(%s) Text(%s)\n", port,
			msg->idx,
			msg->number,
			msg->date,
			msg->text
			);
}

static void save_sms(int port, struct gsm_sms_message *msg)
{
	char filename[128];
	cm_get(config, filename, sizeof(filename)-32, SECT_GENERAL, CFG_GEN_SMS_DIR );

	if (!strlen(filename)) {
		gsm_log(GSM_WARNING,"cannot save sms, no directory specified (see gsm.conf)\n");
		return;
	}

	struct timeval tv;
	gettimeofday(&tv,NULL);

	char name[64];
	sprintf(name,"/%d-%d-port-%d.sms",(int)tv.tv_sec, (int)tv.tv_usec, port);
	strcat(filename, name);

	ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Saving SMS in %s\n",filename);

	FILE *f=fopen(filename, "w+");

	if (!f) {
		gsm_log(GSM_WARNING,"cannot save sms at (%s) error (%s)\n",filename, strerror(errno));
		return;
	}

	fprintf(f,"port=%d\n", port);

	if (gsm_cfg[port].sms_pdu_mode) {
		fprintf(f,"pdu_len=%d\n", msg->pdu_len);
	} else {
		fprintf(f,"callerid=%s\n", msg->number);
		fprintf(f,"date=%s\n", msg->date);
	}
	fprintf(f,"text=%s\n", msg->text);

	fclose(f);

	/*generate a Manager Event*/
	if (gsm_cfg[port].sms_pdu_mode)
		manager_event(EVENT_FLAG_CALL, "NewSMS_PDU", "Port: %d\r\nPDU_LEN: %d\r\nText: %s\r\n", port, msg->pdu_len, msg->text);
	else
		manager_event(EVENT_FLAG_CALL, "NewSMS", "Port: %d\r\nCallerID: %s\r\nDate: %s\r\nText: %s\r\n", port, msg->number, msg->date, msg->text);

}

static int sms_idx[256];


static void sms_idx_add(int idx)
{
	int i;
	for (i=0; i<256; i++) {
		if (!sms_idx[i]) {
			sms_idx[i]=idx;
			//ast_verbose("adding i%d idx %d\n", i,idx);
			break;
		}
	}
}

static int sms_idx_get_next(void)
{
	int i;
	for (i=0; i<256; i++) {
		if (sms_idx[i]) {
			int idx=sms_idx[i];
			sms_idx[i]=0;
			//ast_verbose("returnind i%d idx %d\n", i,idx);
			return idx;
		}
	}
	return 0;
}

enum gsm_events {
	GSM_EVENT_NONE=0,
	GSM_EVENT_RING,				/* ringing */
	GSM_EVENT_BUSY,				/* busy */
	GSM_EVENT_NO_CARRIER,			/* no carrier */
	GSM_EVENT_OK,				/* ok */
	GSM_EVENT_CLCC_CALL_0,			/* active call */
	GSM_EVENT_CLCC_CALL_1,			/* incoming call */
	GSM_EVENT_CLCC_CALL_2,			/* dialing call */
	GSM_EVENT_CLCC_CALL_3,			/* alerting call */
	GSM_EVENT_CSQ,				/* signal strength response */
	GSM_EVENT_CREG_SERVICE_0,		/* not registered, and not looking for a new operator to register to */
	GSM_EVENT_CREG_SERVICE_1,		/* registered, home network */
	GSM_EVENT_CREG_SERVICE_2,		/* not registered, and looking for a new operator to register to */
	GSM_EVENT_CREG_SERVICE_3,		/* registration denied */
	GSM_EVENT_CREG_SERVICE_4,		/* unknown */
	GSM_EVENT_CREG_SERVICE_5,		/* registered and roaming (not on home network) */
	GSM_EVENT_ENTER_SMS,			/* sms entry prompt */
	GSM_EVENT_SMS_ECHO,
	GSM_EVENT_SMS_CMGL_ECHO,		/* list sms messages */
	GSM_EVENT_SMS_TRANSMITTED,		/* sms transmission indication */
	GSM_EVENT_SMS_NEW_MESSAGE,		/* sms message list item */
	GSM_EVENT_SMS_INCOMING_MESSAGE,		/* new incoming message */
	GSM_EVENT_ERROR,			/* error */
	GSM_EVENT_COPS,				/* provider query */
	GSM_EVENT_INIT,				/* initialization */
	GSM_EVENT_CUSD,				/* CUSD */
	GSM_EVENT_MODEM_CHECK_ERROR,		/* keep it for now */
	GSM_EVENT_CALL_READY,			/* call ready message */
	GSM_EVENT_COPN,				/* read operator names */
	GSM_EVENT_CLIP,				/* callerid */
	GSM_EVENT_CME_ERROR,			/* CME error  */
	GSM_EVENT_CMS_ERROR,			/* CMS error  */
	GSM_EVENT_SIM_INSERTED,			/* SIM insertion event */
	GSM_EVENT_SIM_REMOVED			/* SIM insertion event */
};

/* parse the call info structure and return the state of the call */
/* note that we do one active call at a time */
static enum gsm_events parse_current_call_state(const char *event)
{
	if (gsm_debug)
		gsm_log(GSM_DEBUG, "%s() : event = %s\n", __FUNCTION__, event);

	if (strstr(event, "+CLCC: 1,0,0")) 				
		return GSM_EVENT_CLCC_CALL_0;	/* active -- connected */
	else if (strstr(event, "+CLCC: 1,0,3"))
		return GSM_EVENT_CLCC_CALL_3;	/* alerting */
	else if (strstr(event, "+CLCC: 1,0,2")) 
		return GSM_EVENT_CLCC_CALL_2;	/* dialing */
	else if (strstr(event, "+CLCC: 1,1")) {
		return GSM_EVENT_CLCC_CALL_1;   /* incoming (held) call */
	} else
		return GSM_EVENT_NONE;		/* none that we know of */
}

static enum gsm_events parse_events (char *event) {

	if (gsm_debug) 
		gsm_log(GSM_DEBUG, "%s() : event = %s\n", __FUNCTION__, event);

	if (strstr(event, "RING"))
		return GSM_EVENT_RING;
	else if (strstr(event, "BUSY"))
		return GSM_EVENT_BUSY;
	else if (strstr(event, "OK"))
		return GSM_EVENT_OK;
	else if (strstr(event, "NO CARRIER"))
		return GSM_EVENT_NO_CARRIER;
	else if (strstr(event, "+CLCC: 1")) 
		return parse_current_call_state(event);
	else if (strstr(event, "+CSQ: "))
		return GSM_EVENT_CSQ;
	else if (strstr(event, "+CREG: 1,0") || strstr(event, "+CREG: 0,0") || strstr(event, "+CREG: 0"))		
		/* not registered and not trying to register */
		return GSM_EVENT_CREG_SERVICE_0;
	else if (strstr(event, "+CREG: 1,1") || strstr(event, "+CREG: 0,1") || strstr(event, "+CREG: 1"))		
		/* registered in the homezone */
		return GSM_EVENT_CREG_SERVICE_1;
	else if (strstr(event, "+CREG: 1,2") || strstr(event, "+CREG: 0,2") || strstr(event, "+CREG: 2"))		
		/* not registered but trying to register */
		return GSM_EVENT_CREG_SERVICE_2;
	else if (strstr(event, "+CREG: 1,3") || strstr(event, "+CREG: 0,3") || strstr(event, "+CREG: 3"))		
		/* registration denied */
		return GSM_EVENT_CREG_SERVICE_3;
	else if (strstr(event, "+CREG: 1,4") || strstr(event, "+CREG: 0,4") || strstr(event, "+CREG: 4"))		
		/* unknown */
		return GSM_EVENT_CREG_SERVICE_4;
	else if (strstr(event, "+CREG: 1,5") || strstr(event, "+CREG: 0,5") || strstr(event, "+CREG: 5"))		
		/* registered and roaming */
		return GSM_EVENT_CREG_SERVICE_5;
	else if (strstr(event, "+CREG: 1"))
		return GSM_EVENT_CREG_SERVICE_1;
	else if (strstr(event, "+CREG:"))
		return GSM_EVENT_CREG_SERVICE_0;
	else if (strstr(event, "Call Ready"))
		return GSM_EVENT_CALL_READY;
	else if (strstr(event, "> "))
		return GSM_EVENT_ENTER_SMS;
	else if (strstr(event, "+CMGW: "))
		return GSM_EVENT_SMS_ECHO;
	else if (strstr(event, "+CMGS: ") || strstr(event, "+CMSS: "))
		return GSM_EVENT_SMS_TRANSMITTED;
	else if (strstr(event, "AT+CMGL=\"ALL\""))
		return GSM_EVENT_SMS_CMGL_ECHO;
	else if (strstr(event, "+CMGL: "))
		return GSM_EVENT_SMS_NEW_MESSAGE;
	else if (strstr(event, "+CMTI: "))
		return GSM_EVENT_SMS_INCOMING_MESSAGE;
	else if (strstr(event, "+COPS:"))
		return GSM_EVENT_COPS;
	else if (strstr(event, "GSMINIT:"))
		return GSM_EVENT_INIT;
	else if (strstr(event, "MODEM_CHECK_ERROR"))
		return GSM_EVENT_MODEM_CHECK_ERROR;
	else if (strstr(event, "+CUSD:"))
		return GSM_EVENT_CUSD;
	else if (strstr(event, "+COPN:"))
		return GSM_EVENT_COPN;
	else if (strstr(event, "+CLIP:"))
		return GSM_EVENT_CLIP;
	else if (strstr(event, "+CME ERROR:"))
		return GSM_EVENT_CME_ERROR;
	else if (strstr(event, "+CMS ERROR:"))
		return GSM_EVENT_CMS_ERROR;
	else if (strstr(event, "ERROR"))
		return GSM_EVENT_ERROR;
	else if (strstr(event, "+CSMINS: 1,0"))
		return GSM_EVENT_SIM_REMOVED;
	else if (strstr(event, "+CSMINS: 1,1"))
		return GSM_EVENT_SIM_INSERTED;
	else 
		return GSM_EVENT_NONE;
}

void cb_events(int port, char *event)
{
	struct ast_channel *ast=gsm_get_priv(port);
	struct gsm_pvt *gsm=NULL;
	enum gsm_events ev=parse_events(event);

	if (ast)
		gsm=ast->tech_pvt;

	if (!strlen(event)) return ; /*skip empty lines*/

	if (gsm_debug) {
		if (ev != GSM_EVENT_NONE)
			ast_verbose(CHAN_GSM_VERBOSE_PREFIX "P(%d)> ev (%d) state (%d) event (%s) smsstate (%d)\n", port, ev, gsm?gsm->state:-1, event, gsm_sms[port].state);
		else
			ast_verbose(CHAN_GSM_VERBOSE_PREFIX "P(%d)> unknown event state (%d) event (%s) smsstate (%d)\n",port, gsm?gsm->state:-1, event, gsm_sms[port].state);
	}

	/* handle "unknown events" which are data (e.g. unknown at compile time) radio sends in response our commands */
	if (ev == GSM_EVENT_NONE)
		gsm_handle_unknown_event(port, event);

	/*note that the OK event might be handled under different switches*/
	/*sms handling*/
	//ast_verbose("SMS_STATE: ev(%d)  sms_state(%d)\n", gsm_sms[port].state);

	if (ev == GSM_EVENT_MODEM_CHECK_ERROR) {
                if (ast && gsm && gsm->state == GSM_STATE_ATD_OUT) {
                        gsm->state=GSM_STATE_HANGUPED;
                        ast->hangupcause=22;
                        ast_queue_hangup(ast);
		} else {
			ast_verbose(CHAN_GSM_VERBOSE_PREFIX "P(%d)> MODEM RESPONSE DELAYED\n",port);
			gsm_log(GSM_WARNING, "P(%d)> MODEM RESPONSE DELAYED(%s)\n", port, event);
		}
		/* restart the radio with full power cycle */
		_gsm_restart_port(port, 1);
		return;
	}

	if (gsm_sms[port].state != GSM_SMS_STATE_NONE) {
		struct gsm_sms_message *msg=&gsm_sms[port].inbound_msg;
		char buf[1048];
		//ast_verbose("SMS_STATE != NONE\n");

		switch (ev) {
			/*RECEIVING SMS*/
			case GSM_EVENT_SMS_CMGL_ECHO:
				/*enter list mode*/
				gsm_sms[port].state=GSM_SMS_STATE_LIST;
				if (gsm_debug) 
					ast_verbose(CHAN_GSM_VERBOSE_PREFIX "CMGL_ECHO -> STATE_LIST\n");
			break;
			case GSM_EVENT_SMS_NEW_MESSAGE:
				/*new sms*/
				/*are we the first new message ?*/
				if (gsm_debug) 
					ast_verbose(CHAN_GSM_VERBOSE_PREFIX "NEW_MESSAGE->GET_TEXT\n");
				if (gsm_sms[port].state==GSM_SMS_STATE_GET_TEXT) {
					if(gsm_sms[port].work==GSM_SMS_WORK_SHOW) {
						output_sms(port, &gsm_sms[port].inbound_msg);
						memset(&gsm_sms[port].inbound_msg, 0, sizeof(struct gsm_sms_message));
					}
					if(gsm_sms[port].work==GSM_SMS_WORK_FETCH) {
						save_sms(port, &gsm_sms[port].inbound_msg);
						sms_idx_add(gsm_sms[port].inbound_msg.idx);
					}
				}

				if (!gsm_cfg[port].sms_pdu_mode) { /*parse the sms header*/
					char *p=event, *r;
					r=strsep(&p,":"); if (!r) break; /*CMGL*/
					r=strsep(&p,","); if (!r) break; /*idx*/
					msg->idx=atoi(r);
					r=strsep(&p,","); if (!r) break;/*REC READ*/
					r=strsep(&p,"\""); if (!r) break;/*skip*/
					r=strsep(&p,"\""); if (!r) break;/*Number*/
					strcpy(msg->number,r);
					r=strsep(&p,"\""); if (!r) break;/*skip*/
					r=strsep(&p,"\""); if (!r) break;/*date*/
					strcpy(msg->date,r);
					gsm_sms[port].state=GSM_SMS_STATE_GET_TEXT;
					msg->text[0]=0;
				} else {
						//received a PDU:
					char *p=event, *r;
					r=strsep(&p,":"); if (!r) break; /*CMGL*/
					r=strsep(&p,","); if (!r) break; /*idx*/
					msg->idx=atoi(r);
					r=strsep(&p,","); if (!r) break;/* 0*/
					r=strsep(&p,","); if (!r) break;/* skip ,*/
					r=strsep(&p,",)"); if (!r) break;/* get length*/
					msg->pdu_len=atoi(r);
					msg->text[0]=0;
					gsm_sms[port].state=GSM_SMS_STATE_GET_TEXT;

					msg->number[0]=0;
					msg->date[0]=0;
					if (gsm_debug) 
						ast_verbose(CHAN_GSM_VERBOSE_PREFIX "RECEIVING PDU VERSION\n");
				}
			break;

			case GSM_EVENT_NONE:
				if (gsm_sms[port].state==GSM_SMS_STATE_GET_TEXT) {
					/*its the Text*/
					strcat(msg->text, event);
				}
			break;

			case GSM_EVENT_OK:

				if (gsm_debug)
					ast_verbose(CHAN_GSM_VERBOSE_PREFIX "EVENT_OK\n");

				if (gsm_sms[port].state == GSM_SMS_STATE_EXPECTING_OK) {
					char buf[40];

					gsm_sms[port].state = GSM_SMS_STATE_EXPECTING_PROMPT;
					if (gsm_save_out_msgs) {
						sprintf(buf,"AT+CMGW=\"%s\"", gsm_sms[port].number);
					} else {
						sprintf(buf,"AT+CMGS=\"%s\"", gsm_sms[port].number);
					}
					gsm_send(port, buf, 0);
					gsm_sms_enter(port);
					break;
				}

				if (gsm_sms[port].state==GSM_SMS_STATE_GET_TEXT) {
					if (gsm_sms[port].work==GSM_SMS_WORK_SHOW) {
						output_sms(port, &gsm_sms[port].inbound_msg);
						memset(&gsm_sms[port].inbound_msg, 0, sizeof(struct gsm_sms_message));
					}
					if (gsm_sms[port].work==GSM_SMS_WORK_FETCH) {
						save_sms(port, &gsm_sms[port].inbound_msg);
						sms_idx_add(gsm_sms[port].inbound_msg.idx);
					}

					/*finish sms list mode*/
					int idx = sms_idx_get_next();
					if (idx) {
						char buf[32];
						sprintf(buf, "AT+CMGD=%d",idx);
						gsm_sms[port].state=GSM_SMS_STATE_FLUSHING;
						gsm_send(port,buf,100);
						if (gsm_debug)
							ast_verbose(CHAN_GSM_VERBOSE_PREFIX "sms del %d\n",idx);
						break;
					}
				}

				if (gsm_sms[port].state==GSM_SMS_STATE_FLUSHING) {
					/*FIXME*/
					int idx = sms_idx_get_next();
					if (idx) {
						char buf[32];
						sprintf(buf, "AT+CMGD=%d",idx);
						gsm_sms[port].state=GSM_SMS_STATE_FLUSHING;
						gsm_send(port,buf,100);
						if (gsm_debug)
							ast_verbose(CHAN_GSM_VERBOSE_PREFIX "sms del %d\n",idx);
						break;
					}
				}

				gsm_sms[port].state=GSM_SMS_STATE_NONE;
				gsm_sms[port].work=GSM_SMS_WORK_SHOW;
			break;

			/*TRANSMITTING SMS*/
			case GSM_EVENT_SMS_TRANSMITTED:
				/*if (gsm_sms[port].s && gsm_sms[port].m)
					astman_send_ack(gsm_sms[port].s, gsm_sms[port].m,
							"GsmSendSms: Success");
				*/
				gsm_sms[port].s=NULL;
				gsm_sms[port].m=NULL;
				gsm_sms[port].state=GSM_SMS_STATE_NONE;
				memset(&gsm_sms[port].outbound_msg, 0, sizeof(struct gsm_sms_message));
				gsm_log(GSM_EVENT, "SMS successfully transmitted on port %d.\n", port);
				ast_verbose(CHAN_GSM_VERBOSE_PREFIX "SMS successfully transmitted on port %d.\n", port);
				/* get back to PDU mode if that was the mode we were in */
				/* note that this will kick in after we get an OK */
				if (gsm_cfg[port].sms_pdu_mode) {
					gsm_send(port, "AT+CMGF=0", 0);
				}
			break;

			case GSM_EVENT_SMS_ECHO:
				if (gsm_debug)
					ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Enter SMS Mode\n");

				if (gsm_save_out_msgs) {
					char *p = strchr(event, ':');
					if (p) {
						char txt[100];
						int msg = strtol(p + 1, 0, 10);

						sprintf(txt, "AT+CMSS=%d", msg);
						gsm_send(port, txt, 10);
					}
				}
			break;

			case GSM_EVENT_ENTER_SMS:
				gsm_sms[port].state=GSM_SMS_STATE_SMS_SENT;

				if (gsm_debug)
					ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Sending sms (%s)\n",gsm_sms[port].sms);
				/* We have not posted the lock here for another message
				   since SMS needs to be handled as a "transaction". This is the only
				   place we use gsm_send_raw() which does not use the locking mechanism.
                                   We'll post the lock once we get an OK or an CME_ERROR/CMS_ERROR response. 
                                 */
				sprintf(buf,"%s\x1A",gsm_sms[port].sms);
				gsm_send_raw(port, buf, 100);
			break;

			default:
			break;
		}
	}

	/*pre call handling*/
	if (!gsm) {
		switch(ev) {
			case GSM_EVENT_CALL_READY:
				/* this message is received when the radio is ready to go. */
				// init_mod_port(port);
				// gsm_send(port, "AT+CREG?", 0);
				break;

			case GSM_EVENT_NO_CARRIER:
				/* caller hung up before we could answer */
				gsm_slcc[port].ringing = 0;
				gsm_slcc[port].ringcount = 0;
				break;
	
			case GSM_EVENT_CREG_SERVICE_0:
				gsm_status[port].status=GSM_STATUS_UNREGISTERED;
				gsm_status[port].rssi = 0;
				strcpy(gsm_status[port].provider, "\"none\"   ");
				gsm_status[port].homezone=0;
				ast_verbose(CHAN_GSM_VERBOSE_PREFIX "P(%d)> Module NOT Registered @ Network\n", port);
				gsm_set_led(port,LED_RED);
				gsm_send(port, "AT+CSQ", 0);

				if (gsm_debug) {
					gsm_log(GSM_DEBUG, "P(%d)> Module NOT Registered @ Network\n", port);
					gsm_log(GSM_DEBUG, "Updating LCD for port:%d to CHANNEL_STATE_DOWN\n", port);
				}
				gsm_set_channel_state(port, CHANNEL_STATE_DOWN);
				gsm_send_channel_state(port, CHANNEL_STATE_DOWN);
				break;

			case GSM_EVENT_CREG_SERVICE_1:
				gsm_status[port].status=GSM_STATUS_REGISTERED;

				ast_verbose(CHAN_GSM_VERBOSE_PREFIX "P(%d)> Module Registered @ Network\n", port);
				gsm_send(port, "AT+COPS?", 100);

				ast_verbose(CHAN_GSM_VERBOSE_PREFIX "P(%d)> We're in the homezone\n", port);
				gsm_status[port].homezone=1;
				gsm_send(port, "AT+CSQ", 0);
				gsm_send(port, "AT+CCWA=0,0", 0);

				if (gsm_debug) {
					gsm_log(GSM_DEBUG,  "P(%d)> Module Registered @ Network\n", port);
					gsm_log(GSM_DEBUG, "Updating LCD for port:%d to CHANNEL_STATE_READY\n", port);
				}
				gsm_set_channel_state(port, CHANNEL_STATE_READY);
				gsm_send_channel_state(port, CHANNEL_STATE_READY);
				break;

			case GSM_EVENT_CREG_SERVICE_2:
				gsm_status[port].status=GSM_STATUS_UNREGISTERED;
				gsm_status[port].rssi = 0;
				strcpy(gsm_status[port].provider, "\"none\"   ");
				gsm_status[port].homezone=0;

				ast_verbose(CHAN_GSM_VERBOSE_PREFIX "P(%d)> Module Searching for Network\n", port);
				gsm_send(port, "AT+CSQ", 0);

				if (gsm_debug) { 
					ast_log(LOG_DEBUG, "P(%d)> Module Searching for Network\n", port);
					ast_log(LOG_DEBUG, "Updating LCD for port:%d to CHANNEL_STATE_DOWN\n", port);
				}
				gsm_set_channel_state(port, CHANNEL_STATE_DOWN);
				gsm_send_channel_state(port, CHANNEL_STATE_DOWN);

				break;

			case GSM_EVENT_CREG_SERVICE_3:
				gsm_status[port].status=GSM_STATUS_UNREGISTERED;
				strcpy(gsm_status[port].provider, "\"none\"   ");
				gsm_status[port].homezone=0;
				ast_verbose(CHAN_GSM_VERBOSE_PREFIX "P(%d)> Module Network Registration DENIED.\n", port);
				gsm_send(port, "AT+CSQ", 0);

				if (gsm_debug) {
					gsm_log(GSM_DEBUG, "P(%d)> Module Network Registration DENIED.\n", port);
					gsm_log(GSM_DEBUG, "Updating LCD for port:%d to CHANNEL_STATE_DOWN\n", port);
				}
				gsm_set_channel_state(port, CHANNEL_STATE_DOWN);
				gsm_send_channel_state(port, CHANNEL_STATE_DOWN);
				break;

			case GSM_EVENT_CREG_SERVICE_4:
				ast_verbose(CHAN_GSM_VERBOSE_PREFIX "P(%d)> Unknown Event.\n", port);
				if (gsm_debug)
					gsm_log(GSM_DEBUG, "P(%d)> Unknown Event.\n", port);
				break;

			case GSM_EVENT_CREG_SERVICE_5:
				gsm_status[port].status=GSM_STATUS_REGISTERED;

				ast_verbose(CHAN_GSM_VERBOSE_PREFIX "P(%d)> Module Registered @ Network\n", port);
				gsm_send(port, "AT+COPS?", 100);

				ast_verbose(CHAN_GSM_VERBOSE_PREFIX "P(%d)> We're not in the homezone\n", port);
				gsm_send(port, "AT+CSQ", 0);
				gsm_send(port, "AT+CCWA=0,0", 0);
				gsm_status[port].homezone=0;

				if (gsm_debug) {
					ast_log(LOG_DEBUG, "P(%d)> Module Registered @ Network\n", port);
					ast_log(LOG_DEBUG, "Updating LCD for port:%d to CHANNEL_STATE_READY\n", port);
			 	}	
				gsm_set_channel_state(port, CHANNEL_STATE_READY);
				gsm_send_channel_state(port, CHANNEL_STATE_READY);
				break;

			case GSM_EVENT_CSQ:
				{
					char *p = strchr(event, ':');
					if (p)
						gsm_status[port].rssi = atoi(++p);
				}
				break;

			case GSM_EVENT_INIT:
				if (strstr(event, "GSMINIT: PORT NOT INITIALIZED:")) {
						gsm_status[port].status=GSM_STATUS_UNINITIALIZED;
						gsm_set_led(port,LED_RED);
						gsm_log(GSM_WARNING, "P(%d)> ! Module not Initialized: %s\n", port, event);
				} else {
						gsm_set_led(port,LED_GREEN|LED_BLINK);
						gsm_status[port].status=GSM_STATUS_INITIALIZED;

						if (gsm_debug)
							ast_verbose(CHAN_GSM_VERBOSE_PREFIX "P(%d)> Module Initialized, requesting Registration\n", port);
				}
				break;

			case GSM_EVENT_COPS:
				if (!strcasecmp(event, "+COPS: 1")) {
					ast_verbose(CHAN_GSM_VERBOSE_PREFIX "P(%d)> ! Module has no Provider\n", port);
					strcpy(gsm_status[port].provider, "\"none\"   ");
				} else {
					char *p=event, *prov;
					strsep(&p,",");
					strsep(&p,",");
					prov=strsep(&p,",");

					if (gsm_debug)
						ast_verbose(CHAN_GSM_VERBOSE_PREFIX "P(%d)> Module Provider (%s)\n", port, prov);
					if (prov)
						strcpy(gsm_status[port].provider, prov);
					else
						strcpy(gsm_status[port].provider, "\"none\"   ");
				}
				gsm_send(port, "AT+CSQ", 0);
				break;
	
			case GSM_EVENT_CLIP:
				sscanf(event,"+CLIP: \"%[+0-9]\",%d",
					gsm_slcc[port].callerid,
					&gsm_slcc[port].type);
				
				if (gsm_debug)
					ast_verbose(CHAN_GSM_VERBOSE_PREFIX "mode: %s cid: %s\n",
						gsm_slcc[port].type==145?"INT":"OTHER",
						gsm_slcc[port].callerid);

				break;

			case GSM_EVENT_RING:

				/* mark this line as ringing, we'll clear this once the line is answered or hung up */
				gsm_slcc[port].ringing = 1;

				/* pick up the phone on the second ring message */
				/* so, we can get the +CLIP message in between */
				if (gsm_slcc[port].ringcount == 0) {
					gsm_slcc[port].ringcount++;
					return;
				}

				/* reset the ring count here */			
				gsm_slcc[port].ringcount = 0;

				if (!gsm_port_fetch(port)) {
						ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Port %d already in use\n",port);
						gsm_send(port,"ATH",100);
						break;
				}
		
				/* The GSM CID and the Analog CID are curiously similar
				 * Just like the Analog CID, the GSM CID receives the CID between
                                 * two RING messages in the form of a +CLIP message. So, we pick up
                                 * the incoming call not on the first ring but on the second one :)
                                 */

				gsm=malloc(sizeof(struct gsm_pvt));
				if (!gsm) {
					gsm_log(GSM_ERROR, "Error allocating memory for new GSM channel.");
					return;
				}

				memset(gsm,0,sizeof(struct gsm_pvt));

				gsm->port=port;

				configure_gsm(gsm);
				if (gsm_debug)
					ast_verbose(CHAN_GSM_VERBOSE_PREFIX " --> exten:%s callerid:%s\n",gsm->exten,
						gsm_slcc[port].callerid);

				char calleridbuf[64];
				char *cid=gsm_slcc[port].callerid;

				if (    gsm_config_get_bool(SECT_GENERAL, CFG_GEN_SKIP_PLUS) &&
					strlen(cid) &&
					gsm_slcc[port].type==145) {
					cid++; /*skip the '+'*/
				}

				if ( gsm_slcc[port].type==129) {
					char prefix[16];
					cm_get(config, prefix, sizeof(prefix), SECT_PORT, CFG_PORT_CID_PREFIX, port);

					if (gsm_debug) 
						ast_verbose(CHAN_GSM_VERBOSE_PREFIX "type==129, prefix=%s\n", prefix);
					if (!ast_strlen_zero(prefix)) {
						strcpy(calleridbuf, prefix);
						strcat(calleridbuf, gsm_slcc[port].callerid);
						cid=calleridbuf;
					}
				}

				struct ast_channel *chan=gsm_new(gsm, gsm->exten, cid, NULL, AST_STATE_RINGING);

				ast_pbx_start(chan);
				break;

			case GSM_EVENT_CUSD:
				{
						char *p=strstr(event, "+CUSD: ");
						if (p) {
							p+=strlen("+CUSD: ");
							ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Received CUSD: %s\n",p);
							manager_event(EVENT_FLAG_CALL, "GSM_USSD", "Port: %d\r\nMessage: %s\r\n", port, p);
						}
				}
				break;

			case GSM_EVENT_COPN:
				{
						char *p=strstr(event, "+COPN: ");
						if (p) {
							p+=strlen("+COPN: ");
							ast_verbose(CHAN_GSM_VERBOSE_PREFIX "%s\n", p);
						}
				}
				break;

			case GSM_EVENT_SMS_INCOMING_MESSAGE:
				{
					char *p = strchr(event, ',');
					int index;

					if (strstr(event, "+CMTI:")) {
						if (p) {
							index = atoi(++p);
							if (gsm_cfg[port].sms_pdu_mode) {
								manager_event(EVENT_FLAG_CALL, "IncomingSMS_PDU", "Port: %d\r\nIndex: %d\r\n", port, index);
							} else {
								manager_event(EVENT_FLAG_CALL, "IncomingSMS", "Port: %d\r\nIndex: %d\r\n", port, index);
							}
						}
					}
				}
				break;

			case GSM_EVENT_ERROR:
				gsm_log(GSM_ERROR, "port=(%d) GSM_EVENT_ERROR : last command  (%s)\n",
					port, gsm_get_last_command(port));
				/* end of "transaction", so we allow the next message to be posted */
				gsm_post_next_msg(port);
				break;

			case GSM_EVENT_CME_ERROR:
				{
					char *p = strchr(event, ':');
					char *cme_text_msg;
					int cme_error = -1;

					if (p)
						cme_error = atoi(++p);

					if ((cme_text_msg = (char*)gsm_get_cme_text(cme_error))) {
						gsm_log(GSM_ERROR, "port=(%d) GSM_EVENT_CME_ERROR : %d:%s last command  (%s)\n",
							port, cme_error, cme_text_msg, gsm_get_last_command(port));
					} else {
						gsm_log(GSM_ERROR, "port=(%d) GSM_EVENT_CME_ERROR : %d last command  (%s)\n", 
							port, cme_error, gsm_get_last_command(port));
					}

					gsm_handle_cme_error(port, cme_error);
				}
				/* end of "transaction", so we allow the next message to be posted */
				gsm_post_next_msg(port);
				break;

			case GSM_EVENT_CMS_ERROR:
				{
					char *p = strchr(event, ':');
					char *cms_text_msg;
					int cms_error = -1;

					if (p)
						cms_error = atoi(++p);

					if ((cms_text_msg = (char*)gsm_get_cms_text(cms_error))) {
						gsm_log(GSM_ERROR, "port=(%d) GSM_EVENT_CMS_ERROR : %d:%s last command (%s)\n",
							 port, cms_error, cms_text_msg, gsm_get_last_command(port));
					} else {
						gsm_log(GSM_ERROR, "port=(%d) GSM_EVENT_CMS_ERROR : %d last command (%s)\n",
							port, cms_error, gsm_get_last_command(port));
					}

					gsm_handle_cms_error(port, cms_error);
				}
				/* go back to the original PDU mode if we changed it */
				if (gsm_cfg[port].sms_pdu_mode) {
					gsm_send(port, "AT+CMGF=0", 0);
				}

				/* requeue sms message if there was any */
				if ((gsm_sms[port].state == GSM_SMS_STATE_SMS_SENT) && \
				    (strlen(gsm_sms[port].outbound_msg.number)) && \
                                    (strlen(gsm_sms[port].outbound_msg.text))) {
					gsm_log(GSM_WARNING, " retrying message(%s) to number(%s)\n", gsm_sms[port].outbound_msg.text, gsm_sms[port].outbound_msg.number);
					_gsm_queue_sms(port, gsm_sms[port].outbound_msg.number, gsm_sms[port].outbound_msg.text);
				}

				/* reset the SMS state to none (we just failed) */
				gsm_sms[port].state=GSM_SMS_STATE_NONE;

				/* end of "transaction", so we allow the next message to be posted */
				gsm_post_next_msg(port);
				break;

			case GSM_EVENT_OK:
				/* end of "transaction", so we allow the next message to be posted */
				gsm_post_next_msg(port);
				break;

			case GSM_EVENT_SIM_INSERTED:
				if (gsm_debug)
					gsm_log(GSM_DEBUG, "port=(%d) SIM insertion detected.\n", port);

				/* Query the IMSI number */
				gsm_send(port, "AT+CIMI", 0);
				break;

			case GSM_EVENT_SIM_REMOVED:
				if (gsm_debug)
					gsm_log(GSM_DEBUG, "port=(%d) SIM removal detected.\n", port);
				/* Toggle the radio functionality to force an update on SIM status */
				gsm_send(port, "AT+CFUN=0", 0);
				gsm_send(port, "AT+CFUN=1", 0);
				break;

			default:
			break;
		}
	}

	if (!gsm) return;

	/*during call handling*/
	switch (ev) {
		case GSM_EVENT_RING:
			if (gsm_debug)
				ast_verbose(CHAN_GSM_VERBOSE_PREFIX " -> RING\n");
			break;

		case GSM_EVENT_NO_CARRIER:
		case GSM_EVENT_BUSY:
			if (gsm_debug)
				ast_verbose(CHAN_GSM_VERBOSE_PREFIX " -> BUSY/NO CARRIER/CALL_0\n");
			if (ast && gsm->state != GSM_STATE_HANGUPED) {
				gsm->state=GSM_STATE_HANGUPED;
				ast->hangupcause=17;
				ast_queue_hangup(ast);
			}
			/* line no longer ringing, so it's safe to send SMS */
			gsm_slcc[gsm->port].ringing = 0;
			/* allow the next message to be posted */
			gsm_post_next_msg(port);
			break;

		case GSM_EVENT_OK:
			if (gsm_debug)
				ast_verbose(CHAN_GSM_VERBOSE_PREFIX " -> OK\n");
			/* allow the next message to be posted */
			gsm_post_next_msg(port);
			if (gsm->state != GSM_STATE_ATD_OUT) break;
			break;

		case GSM_EVENT_CLCC_CALL_0:
			/* only send answer if you have not done so yet */
			if (gsm->state != GSM_STATE_ANSWERED) {
				if (gsm_debug)
					ast_verbose(CHAN_GSM_VERBOSE_PREFIX " -> ANSWER\n");
				ast_queue_control(ast, AST_CONTROL_ANSWER);
				int res=gsm_audio_start(gsm->port);
				if (res<0) {
					gsm_log(GSM_WARNING, " couldn't start audio error (%d,%s)\n", errno, strerror(errno));
				}
				gsm->state = GSM_STATE_ANSWERED;
			}
			break;

		case GSM_EVENT_CLCC_CALL_2:
			if (gsm->state == GSM_STATE_ATD_OUT) {
				if (gsm_debug)
					ast_verbose(CHAN_GSM_VERBOSE_PREFIX " -> PROCEEDING\n");
				ast_queue_control(ast, AST_CONTROL_PROCEEDING);
				gsm->state = GSM_STATE_PROCEEDING;
			}
			break;

		case GSM_EVENT_CLCC_CALL_3:
			{ 	//inband info available
				int res=gsm_audio_start(gsm->port);
				if (res<0) {
					gsm_log(GSM_WARNING, " couldn't start audio error (%d,%s)\n", errno, strerror(errno));
				}
			}
			if (gsm->state == GSM_STATE_PROCEEDING) {
				if (gsm_debug)
					ast_verbose(CHAN_GSM_VERBOSE_PREFIX " -> ALERTING\n");
				ast_queue_control(ast, AST_CONTROL_RINGING);
				gsm->state = GSM_STATE_ALERTING;
			}	
			break;

	/*	case GSM_EVENT_OK:
			ast_verbose(" -> OK\n");
			break;*/

		case GSM_EVENT_NONE:
			if (gsm_debug)
				ast_verbose(CHAN_GSM_VERBOSE_PREFIX " -> NONE\n");
			break;

		case GSM_EVENT_CME_ERROR:
			{
				char *p = strchr(event, ':');
				char *cme_text_msg;
				int cme_error = -1;

				if (p)
					cme_error = atoi(++p);

				if ((cme_text_msg = (char*)gsm_get_cme_text(cme_error))) {
					gsm_log(GSM_ERROR, "port=(%d) GSM_EVENT_CME_ERROR : %d:%s last command  (%s)\n",
						port, cme_error, cme_text_msg, gsm_get_last_command(port));
				} else {
					gsm_log(GSM_ERROR, "port=(%d) GSM_EVENT_CME_ERROR : %d last command (%s)\n",
						port, cme_error, gsm_get_last_command(port));
				}

				gsm_handle_cme_error(port, cme_error);
			}

			/* end of "transaction", so we allow the next message to be posted */
			gsm_post_next_msg(port);
			break;

		case GSM_EVENT_CMS_ERROR:
			{
				char *p = strchr(event, ':');
				char *cms_text_msg;
				int cms_error = -1;

				if (p)
					cms_error = atoi(++p);

				if ((cms_text_msg = (char*)gsm_get_cms_text(cms_error))) {
					gsm_log(GSM_ERROR, "port=(%d) GSM_EVENT_CMS_ERROR : %d:%s last command (%s)\n",
						port, cms_error, cms_text_msg, gsm_get_last_command(port));
				} else {
					gsm_log(GSM_ERROR, "port=(%d) GSM_EVENT_CMS_ERROR : %d last command (%s)\n",
						port, cms_error, gsm_get_last_command(port));
				}

				gsm_handle_cms_error(port, cms_error);
			}
			/* go back to the original PDU mode if we changed it */
			if (gsm_cfg[port].sms_pdu_mode) {
				gsm_send(port, "AT+CMGF=0", 0);
			}
			/* reset the SMS state to none (we just failed) */
			gsm_sms[port].state=GSM_SMS_STATE_NONE;

			/* end of "transaction", so we allow the next message to be posted */
			gsm_post_next_msg(port);
			break;

		case GSM_EVENT_ERROR:
			gsm_log(GSM_ERROR, "GSM EVENT ERROR wo. gsm obj\n");
			/* end of "transaction", so we allow the next message to be posted */
			gsm_post_next_msg(port);
			break;

		case GSM_EVENT_SIM_INSERTED:
			if (gsm_debug)
				gsm_log(GSM_DEBUG, "port=(%d) SIM insertion detected during call(?).\n", port);
			break;

		case GSM_EVENT_SIM_REMOVED:
			if (gsm_debug)
				gsm_log(GSM_DEBUG, "port=(%d) SIM removal detected during call.\n", port);
			break;

		default:
			break;
	}
}


static int gsm_send_sms_exec(struct ast_channel *chan, void *data)
{
	int port;
	AST_DECLARE_APP_ARGS(args,
				AST_APP_ARG(port);
				AST_APP_ARG(number);
				AST_APP_ARG(sms);
				);

	if (ast_strlen_zero(data)) {
		gsm_log(GSM_WARNING, "This application requires arguments.\n");
		return -1;
	}

	AST_STANDARD_APP_ARGS(args, data);

	if (args.argc != 3) {
		gsm_log(GSM_WARNING, "Wrong argument count\n");
		return 0;
	}


	port = strtol(args.port, NULL, 10);

	_gsm_queue_sms(port, args.number, args.sms);

	return 0;
}

static int gsm_set_simslot_exec(struct ast_channel *chan, void *data)
{
	int port, slot, wait;
	AST_DECLARE_APP_ARGS(args,
		AST_APP_ARG(port);
		AST_APP_ARG(slot);
		AST_APP_ARG(wait);
	);

 	if (ast_strlen_zero(data)) {
		gsm_log(GSM_WARNING, "This application requires arguments.\n");
		return -1;
	}

	AST_STANDARD_APP_ARGS(args, data);

	if (args.argc < 2) {
		gsm_log(GSM_WARNING, "Wrong argument count\n");
		return 0;
	}

	port = strtol(args.port, NULL, 10);
	slot = strtol(args.slot, NULL, 10);

	if (args.argc == 3)
		wait = strtol(args.wait, NULL, 10);
	else
		wait = 5;

	//ast_verbose("Setting Simslot:%d on port:%d\n", slot, port);

	int i=gsm_set_next_simslot(port, slot);
	if (i<0) {
		ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Setting simslot failed (%s)\n", strerror(errno));
		return -1;
	}

	if (i)	
		_gsm_restart_port(port, wait) ;
	else 
		ast_verbose(CHAN_GSM_VERBOSE_PREFIX "not setting Simslot, we have it already\n");

	return 0;
}


static void _init_mod_port(int port)
{
	char buf[256];
	char pin[sizeof(gsm_cfg[port].pin)];
	char smsc[sizeof(gsm_cfg[port].smsc)];
	char initfile[sizeof(gsm_cfg[port].initfile)];
	char sms_pdu_mode[256];
	int simslot=0;

	if (gsm_debug)
		ast_verbose(CHAN_GSM_VERBOSE_PREFIX "Initializing port %d(%d)\n",port, simslot);

	simslot=gsm_simslot_next_get(port);

	/* this is a port that is not available */
	if (simslot<0) {
		gsm_log(GSM_ERROR, "Error accessing port %d\n",port);
		return;
	}

	gsm_status[port].status=GSM_STATUS_UNAVAILABLE;

	/*not measurable*/
	gsm_status[port].rssi=99;
	strcpy(gsm_status[port].provider,"\"none\"   ");

	gsm_cfg[port].port=1;

	if (cm_get(config, pin, sizeof(pin), SECT_PORT, CFG_PORT_PIN, port)) {
			strcpy(pin,"");
	}
	gsm_cfg_get_array("pin", pin, gsm_cfg[port].pin, simslot);

	if (gsm_debug)
		ast_verbose(CHAN_GSM_VERBOSE_PREFIX " cfg port (%d) pin (%s)\n",port,gsm_cfg[port].pin);


	if (cm_get(config, initfile, sizeof(buf), SECT_PORT, CFG_PORT_INITFILE, port)) {
		strcpy(initfile, "");
	}
	gsm_cfg_get_array("initfile",initfile, gsm_cfg[port].initfile, simslot);

	if (cm_get(config, smsc, sizeof(smsc), SECT_PORT, CFG_PORT_SMSC, port)) {
		strcpy(smsc, "");
	}

	if (cm_get(config, sms_pdu_mode, sizeof(sms_pdu_mode), SECT_PORT, CFG_PORT_SMS_PDU_MODE, port)) {
		strcpy(sms_pdu_mode, "no");
	}

	if (ast_true(sms_pdu_mode))
		gsm_cfg[port].sms_pdu_mode=1;
	else
		gsm_cfg[port].sms_pdu_mode=0;

	gsm_cfg_get_array("smsc", smsc, gsm_cfg[port].smsc, simslot);

	char rxgain[32];
	if (cm_get(config, rxgain, sizeof(rxgain), SECT_PORT, CFG_PORT_RX_GAIN, port)) {
		strcpy(rxgain, "0");
	}

	gsm_cfg[port].rx_gain = atoi(rxgain);

	char txgain[32];
	if (cm_get(config, txgain, sizeof(txgain), SECT_PORT, CFG_PORT_TX_GAIN, port)) {
		strcpy(txgain, "0");
	}

	gsm_cfg[port].tx_gain = atoi(txgain);

}

pthread_t gsm_load_thread;

#ifdef ASTERISK_1_4
static int load_module(void)
#else
int load_module(void)
#endif

{
	ast_mutex_init(&gsm_mutex);
	pthread_create(&gsm_load_thread,NULL,load_module_thread,NULL);
	//load_module_thread(NULL);
#ifdef ASTERISK_1_4
        return AST_MODULE_LOAD_SUCCESS;
#else
	return 0;
#endif
}

void gsm_cleanup_func(void)
{
	gsm_shutdown();
}

void* load_module_thread(void* data)
{
	int i;


	ast_mutex_lock(&gsm_mutex);

	memset(gsm_slcc, 0, sizeof(gsm_slcc));

	gsm_config_init();

	char filename[128];
	cm_get(config, filename, sizeof(filename)-32, SECT_GENERAL, CFG_GEN_SMS_DIR );
	
	if ((mkdir(filename,0777) < 0) && (errno != EEXIST))
	{
		gsm_log(GSM_WARNING, "Couldn't create sms save dir (%s) error(%s)\n",filename, strerror(errno));
	}

	char debug[16];
	cm_get(config, debug, sizeof(debug), SECT_GENERAL, CFG_GEN_DEBUG);
	gsm_debug=strtol(debug,NULL,10);

	memset(csel_groups,0,sizeof(csel_groups));
	memset(gsm_cfg,0,sizeof(gsm_cfg));
	memset(gsm_status,0,sizeof(gsm_status));

	/* create or get access to the outgoing message queues */
	for (i = 0; i < MAX_GSM_PORTS; i++) {
		int key = ftok(GSM_PATH_NAME, GSM_PROJ_ID + i);
		if ((gsm_sms[i].sms_mq_id = msgget(key, IPC_CREAT | 0660)) < 0) {
			gsm_log(GSM_ERROR, "msgget() returned error (%s)\n", strerror(errno));
		}
	}

	/*initialize the gsm_config*/
	int  next;
	int *prev = NULL;
	for (; cm_get_next_id(config, SECT_PORT, prev, &next); prev = &next) {
		char name[128], method[128];
		int i;
		char slot[32];

		if (next <=0 || next > MAX_GSM_PORTS) {
			gsm_log(GSM_WARNING, "ignoring port %d: invalid port id\n", next);
			continue;
		}

		if (!gsm_check_port(next)) {
			gsm_log(GSM_WARNING, "ignoring port %d: missing module\n", next);
			continue;
		}

		cm_get(config, name, sizeof(name), SECT_PORT, CFG_PORT_NAME, next);
		cm_get(config, method, sizeof(method), SECT_PORT, CFG_PORT_METHOD, next);
		for (i=0;i<MAX_GSM_PORTS;i++) {
			if (!csel_groups[i].name) {
				//we're at the end of the list, so
				//we add a new CSEL object
				csel_groups[i].name=strdup(name);
				csel_groups[i].cs = csel_create(
					method,
					NULL, /*parm*/
					csel_occupy, /*occupy*/
					NULL /*free*/
				);
			}

			if (!strcmp(csel_groups[i].name, name)) {
				//FIXME: add port to csel
				csel_add(csel_groups[i].cs, (void*)next);
				break;
			}
		}

		if (cm_get(config, slot, sizeof(slot), SECT_PORT, CFG_PORT_SLOT, next)) {
			strcpy(slot,"0");
		}

		gsm_set_next_simslot(next, slot[0]=='1'?1:0);

		_init_mod_port(next);

		char resetiv[32];
		if (cm_get(config, resetiv, sizeof(resetiv), SECT_PORT, CFG_PORT_RESET_INTERVAL, next)) {
			strcpy(resetiv, "0");
		}

		if (atoi(resetiv)>0) {
			gsm_tasks_add( atoi(resetiv) * 1000, gsm_resetinterval, (void*)next);
		}

	}

	for (i=0; i<256; i++)
		sms_idx[i]=0;

	char sysdebug[16];
	if (cm_get(config, sysdebug, sizeof(sysdebug), SECT_GENERAL, CFG_GEN_SYSLOG_DEBUG, next)) {
		strcpy(sysdebug, "no");
	}

	/* change gsm_debug for the time being */
	gsm_debug = ast_true(sysdebug);

	if (gsm_init( cb_events , gsm_cfg, ast_true(sysdebug)) <0) {
		gsm_log(GSM_ERROR, "Unable to initialize GSM\n");
		ast_mutex_unlock(&gsm_mutex);
		return NULL;
	}

        if (ast_channel_register(&gsm_tech)) {
                gsm_log(GSM_ERROR, "Unable to register channel type 'GSM'\n");
		ast_mutex_unlock(&gsm_mutex);
		return NULL;
        }

	ast_cli_register_multiple(chan_gsm_clis, sizeof(chan_gsm_clis) / sizeof(struct ast_cli_entry));
	ast_manager_register( "GsmSendSms", 0, action_send_sms, "Send SMS on GSM" );
	ast_manager_register( "GsmCheckSMSState", 0, action_check_sms_state, "Check the SMS statemachine state" );

	ast_register_application("gsm_set_simslot", gsm_set_simslot_exec, "gsm_set_simslot",
				 "gsm_set_simslot(<port>,<0|1>[,<waitseconds>])n"
				 "changes the to be used simslot for port <port> to either\n"
				 "0 or 1 (0 is the external accessible)\n"
				 "Waits for <waitseconds> seconds, to receive\n"
				 "a Network Register Event (5 seconds should be enough and is default)\n"
		);

	ast_register_application("gsm_send_sms", gsm_send_sms_exec, "gsm_send_sms",
				 "gsm_send_sms <port> <number> <smstext>\n"
				 "sends the smstext to the given port and number\n"
		);

	ast_register_atexit(gsm_cleanup_func);

#if 0
	char hz[32];
	if (cm_get(config, hz, sizeof(hz), SECT_GENERAL, CFG_GEN_CHEKHZ, next)) {
		strcpy(hz, "0");
	}

	if (ast_true(hz))
		gsm_tasks_add( 10 * 1000, gsm_check_homezone, NULL);

#endif

	gsm_tasks_add(CALL_STATUS_CHECK_PERIOD_SEC * 1000, gsm_query_call_state, NULL);

	gsm_tasks_add(SMS_SEND_PERIOD_SEC * 1000, gsm_sms_sender_func, NULL);

	gsm_tasks_add(LCD_UPDATE_DELAY * 1000, gsm_update_lcd_status, NULL);

	gsm_initialized=1;

	ast_mutex_unlock(&gsm_mutex);

	return NULL;
}

pthread_t gsm_unload_thread;


#ifdef ASTERISK_1_4
static int unload_module(void)
#else
int unload_module(void)
#endif
{
	ast_mutex_lock(&gsm_mutex);
	ast_cli_unregister_multiple(chan_gsm_clis, sizeof(chan_gsm_clis) / sizeof(struct ast_cli_entry));
	ast_manager_unregister( "GsmSendSms" );
	ast_manager_unregister( "GsmCheckSMSState" );
	ast_unregister_application("gsm_set_simslot");
	ast_unregister_application("gsm_send_sms");
 	ast_channel_unregister(&gsm_tech);
	gsm_config_exit();
	if (gsm_initialized) {
		gsm_tasks_destroy();
		gsm_shutdown();
		gsm_initialized = 0;
	}
	ast_unregister_atexit(gsm_cleanup_func);
	ast_mutex_unlock(&gsm_mutex);
	return 0;

}

pthread_t gsm_reload_thread;

void* reload_module_thread(void* parm)
{
	struct gsm_config gsm_prev_cfg[MAX_GSM_PORTS];
	char shutdown_port[MAX_GSM_PORTS];
	char start_port[MAX_GSM_PORTS];
	char busy_port[MAX_GSM_PORTS];
	int *prev = NULL;
	int next;
	int i;
	unsigned long then = time(NULL);

	ast_mutex_lock(&gsm_mutex);

	memset(shutdown_port, 0, sizeof(shutdown_port));
	memset(start_port, 0, sizeof(start_port));
	memset(busy_port, 0, sizeof(busy_port));

	memcpy(gsm_prev_cfg, gsm_cfg, sizeof(gsm_prev_cfg));

	for (i=1; i < MAX_GSM_PORTS;i++) {
		busy_port[i] = gsm_port_in_use_get(i);
		if (!busy_port[i]) {
			memset(&csel_groups[i],0,sizeof(struct csel_groups));
			memset(&gsm_cfg[i],0,sizeof(struct gsm_config));
			memset(&gsm_status[i],0,sizeof(struct gsm_status_s));
		}
	}

	gsm_config_exit();
	gsm_config_init();

	char filename[128];
	cm_get(config, filename, sizeof(filename)-32, SECT_GENERAL, CFG_GEN_SMS_DIR );
	
	if ((mkdir(filename,0777) < 0) && (errno != EEXIST))
	{
		gsm_log(GSM_WARNING, "Couldn't create sms save dir (%s) error(%s)\n",filename, strerror(errno));
	}

	char debug[16];
	cm_get(config, debug, sizeof(debug), SECT_GENERAL, CFG_GEN_DEBUG);
	gsm_debug = strtol(debug,NULL,10);

	char sysdebug[16];
	if (cm_get(config, sysdebug, sizeof(sysdebug), SECT_GENERAL, CFG_GEN_SYSLOG_DEBUG, next)) {
		strcpy(sysdebug, "no");
	}

	/* re-initialize the gsm_config*/
	for (; cm_get_next_id(config, SECT_PORT, prev, &next); prev = &next) {
		char name[128], method[128];
		char slot[32];

		if (next <=0 || next > MAX_GSM_PORTS) {
			gsm_log(GSM_WARNING, "ignoring port %d: invalid port id\n", next);
			continue;
		}

		if (!gsm_check_port(next)) {
			gsm_log(GSM_WARNING, "ignoring port %d: missing module\n", next);
			continue;
		}

		if (!busy_port[next]) {
			cm_get(config, name, sizeof(name), SECT_PORT, CFG_PORT_NAME, next);
			cm_get(config, method, sizeof(method), SECT_PORT, CFG_PORT_METHOD, next);
			for (i=0;i<MAX_GSM_PORTS;i++) {
				if (!csel_groups[i].name) {
					//we're at the end of the list, so
					//we add a new CSEL object
					csel_groups[i].name=strdup(name);
					csel_groups[i].cs = csel_create(
						method,
						NULL, /*parm*/
						csel_occupy, /*occupy*/
						NULL /*free*/
					);
				}

				if (!strcmp(csel_groups[i].name, name)) {
					//FIXME: add port to csel
					csel_add(csel_groups[i].cs, (void*)next);
					break;
				}
			}

			if (cm_get(config, slot, sizeof(slot), SECT_PORT, CFG_PORT_SLOT, next)) {
				strcpy(slot,"0");
			}

			gsm_set_next_simslot(next, slot[0]=='1'?1:0);

			_init_mod_port(next);
		}

	}

	for (i = 1; i < MAX_GSM_PORTS; i++) {
		
		if (!gsm_check_port(i)) continue;

		if ((gsm_prev_cfg[i].port == 0) && (gsm_cfg[i].port == 1)) {
			/* starting a port from fresh */
			ast_verbose("port=(%d) Starting\n", i);
			shutdown_port[i] = 0;
			start_port[i] = 1;
		} else if ((gsm_prev_cfg[i].port == 1) && (gsm_cfg[i].port == 1)) {
			/* restarting an existing port after checking if there is an active call on that port */
			ast_verbose("port=(%d) %s\n", i, (busy_port[i] ? "NOT Restarting (busy)." : "Restarting."));
			shutdown_port[i] = 1;
			start_port[i] = 1;
		} else if ((gsm_prev_cfg[i].port == 1) && (gsm_cfg[i].port == 0)) {
			/* shutting down an existing port, need to check if there's an active call on that port */
			ast_verbose("port=(%d) %s\n", i, (busy_port[i] ? "NOT Shutting Down (busy)." : "Shutting down."));
			shutdown_port[i] = 1;
			start_port[i] = 0;
		} else {
			/* do nothing case --> port was down and will be down */
		}
	}

	for (i=0; i<256; i++)
		sms_idx[i]=0;

	/* process the shutdowns first */
	for (i=1; i < MAX_GSM_PORTS;i++) {
		if (busy_port[i]) {
			gsm_log(LOG_DEBUG, "port=(%d) Busy. Skipping\n", i);
			continue;
		}
		if (shutdown_port[i]) {	/* before shutdown, check to be sure there are no calls on this port */
			gsm_log(LOG_DEBUG, "port=(%d) Shutting Down.\n", i);
			gsm_shutdown_port(i, 0);
			gsm_log(LOG_DEBUG, "port=(%d) Shutdown OK.\n", i);
		}
	}

	/* wait for 3 seconds */
	sleep(3);

	/* process the starts next */
	for (i=1; i < MAX_GSM_PORTS;i++) {
		if (busy_port[i]) {
			gsm_log(LOG_DEBUG, "port=(%d) Busy. Skipping\n", i);
			continue;
		}
		if (start_port[i]) {
			gsm_log(LOG_DEBUG, "port=(%d) Initializing.\n", i);
			gsm_init_port(i, &gsm_cfg[i]);
			gsm_log(LOG_DEBUG, "port=(%d) Initialization OK.\n", i);
		}
	}

	/* wait for all ports to be ready (last step) */
	for (i=1; i < MAX_GSM_PORTS;i++) {
		if (busy_port[i]) {
			gsm_log(LOG_DEBUG, "port=(%d) Busy. Skipping\n", i);
			continue;
		}
		if (start_port[i]) { /* ONLY if it's been restarted */
			gsm_log(LOG_DEBUG, "port=(%d) Waiting for ready signal.\n", i);
			if (gsm_wait_ready_with_timeout(i, GSM_READY_TIMEOUT) == 0)
				gsm_log(LOG_DEBUG, "port=(%d) Waiting for ready signal OK.\n", i);
			else
				gsm_log(LOG_ERROR, "port=(%d) Timed out waiting for ready signal.\n", i);
		}
	}

	ast_mutex_unlock(&gsm_mutex);

	unsigned long now = time(NULL);

	gsm_log(GSM_DEBUG, "It took us %ld seconds to reload/restart configuration\n", now-then);

	return 0;
}


#ifdef ASTERISK_1_4
static int reload_module(void)
#else
int reload_module(void)
#endif
{
	reload_module_thread(NULL);
	return 0;
}

#define AST_MODULE "chan_gsm"
#ifdef ASTERISK_1_4
AST_MODULE_INFO(ASTERISK_GPL_KEY,
                                AST_MODFLAG_DEFAULT,
                                "Channel driver for gsm cards",
                                .load = load_module,
                                .unload = unload_module,
                                .reload = reload_module,
                           );
#else

char *desc="chan_gsm";

char *description(void)
{
        return desc;
}

char *key(void)
{
        return ASTERISK_GPL_KEY;
}

int usecount(void)
{
	return 0;
}

#endif
