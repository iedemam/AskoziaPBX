/*
 * $Id: app_wakeme.c 252 2007-10-30 12:38:16Z michael.iedema $
 * 
 * Asterisk Wake-Up Call Module - part of AskoziaPBX <http://askozia.com/pbx>
 * 
 * Copyright (C) 2007 tecema (a.k.a IKT) <http://www.tecema.de> 
 *
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

#include "asterisk.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/param.h>

#include "asterisk/file.h"
#include "asterisk/logger.h"
#include "asterisk/channel.h"
#include "asterisk/pbx.h"
#include "asterisk/module.h"
#include "asterisk/lock.h"
#include "asterisk/app.h"
#include "asterisk/time.h"
#include "asterisk/say.h"

static int _play_file(struct ast_channel* chan, char* filename);
static int _file_filter(struct dirent *entry);
static int _setwakeup(int time_int, char *cid);


static char *app = "WakeMe";
static char *synopsis = "Wake-up Call Manager";
static char *descrip = "WakeMe allows a user to set a wake-up call on their current extension.\n";

const char *spool_path = "/var/asterisk/spool/outgoing/";


static int wakeme_exec(struct ast_channel *chan, void *data) {

	int res = 0;					/* result 							*/
	char buffer[256];				/* general purpose buffer 			*/
	struct ast_module_user *u;		/* local user for resource control 	*/
	char time_str[5];				/* 4 digit DTMF time entry 			*/
	time_t timer;					/* init for tm structure			*/
	struct tm *time_struct = NULL;	/* time structure for ast_say_time	*/
	int time_int = 0;				/* atoi of time_str 				*/
	int max_chars = 4;				/* loop control for dtmf entry 		*/
	int cur_chars;					/* loop control for dtmf entry		*/
	char ampm;						/* am or pm char entry				*/
	struct dirent **files;			/* files in outgoing spool			*/
	int i;							/* loop control						*/
	int count;						/* file count in outgoing spool		*/
	char *existing_wakeup_file = NULL;

	u = ast_module_user_add(chan);
	
	/* load up any previous wake-up call */
	count = scandir(spool_path, &files, _file_filter, NULL);
	sprintf(buffer, "ext_%s", chan->cid.cid_num);
	for(i=0;i<count;i++) {
		if(strstr(files[i]->d_name, buffer)) {
			existing_wakeup_file = files[i]->d_name;
			/* XXX dirty... */
			sprintf(buffer, "%c%c%c%c", 
				existing_wakeup_file[7],
				existing_wakeup_file[8],
				existing_wakeup_file[9],
				existing_wakeup_file[10]);
			time_int = atoi(buffer);
			timer = time(NULL);
			time_struct = localtime(&timer);
			time_struct->tm_hour = time_int/100;
			time_struct->tm_min = time_int%100;
			timer = mktime(time_struct);
			break;
		}
	}
	
	/* XXX implement callback counts to change prompts */
	/* wakeup call playback & snooze option */
	if (!ast_strlen_zero(data)) {
		for(;;) {
			res = _play_file(chan, "this-is-yr-wakeup-call");
			if(!res) {
				res = _play_file(chan, "to-snooze-for");
			}
			if(!res) {
				res = _play_file(chan, "digits/10");
			}
			if(!res) {
				res = _play_file(chan, "minutes");
			}
			if(!res) {
				res = _play_file(chan, "press-0");
			}
			if(!res) {
				res = _play_file(chan, "silence/5");
			}
			/* XXX "or hangup" prompt needed */
			
			if ((char)res == '0') {
				if ((time_int/100 == 23 && time_int%100 >=50)) {
					time_int -= 2350;
				} else if ((time_int%100 >= 50)) {
					time_int += 50;	
				} else {
					time_int += 10;
				}
				res = _setwakeup(time_int, data);
				if (res >= 0) {
					res = _play_file(chan, "thank-you-for-calling");
					if (!res) {
						res = _play_file(chan, "goodbye");
					}
				}
				goto out;
			} else if (res < 0) {
				goto out;
			}
		}
		
	}
		
	/* allow for cancel / reset */
	if (existing_wakeup_file) {
		for(;;) {
			res = _play_file(chan, "to-cancel-wakeup");
			if (!res) {
				res = _play_file(chan, "for");
			}
			if (!res) {
				ast_say_time(chan, timer, AST_DIGIT_ANY, chan->language);
			}
			if (!res) {
				res = _play_file(chan, "press-1");
			}
			if (!res) {
				res = _play_file(chan, "silence/5");
			}

			if ((char)res == '1') {
				sprintf(buffer, "%s%s", spool_path, existing_wakeup_file);
				if(!remove(buffer)) {
					res = _play_file(chan, "wakeup-call-cancelled");
					if (!res) {
						res = _play_file(chan, "silence/1");
					}
				}
				if (res < 0) {
					goto out;
				}
				break;
			} else if (res < 0) {
				goto out;
			}
		}
	}
	
	
	/* set new wake-up call */
	for(;;) {
		time_int = 0;
		cur_chars = 0;
		ampm = 0;

		/* play back wakeup call introduction */
		res = _play_file(chan, "to-rqst-wakeup-call");
		if (res > 0) {
			time_str[cur_chars++] = (char)res;
		} else if (res == -1) {
			goto out;
		}
	
		/* if we weren't interrupted during the last playback we 
		 * continue with the hour entry instructions */
		if (!cur_chars) {
			res = _play_file(chan, "enter-a-time");
			if (res > 0) {
				time_str[cur_chars++] = (char)res;
			} else if (res == -1) {
				goto out;
			}
		}
		
		/* wait until we have 4 digits */
		/* XXX could use a reprompt here after so many loops */
		while (cur_chars < max_chars) {
			res = _play_file(chan, "silence/5");
			if (res > 0) {
				time_str[cur_chars++] = (char)res;
			} else if (res == -1) {
				goto out;
			}
		}

		/* convert the time characters into an integer */
		time_int = atoi(time_str);
		
		/* sanity check on the time */
		/* XXX better prompt as "im-sorry" needed */
		if ((strpbrk(time_str,"#*")) || (time_int%100 >= 60 ) || (time_int > 2400)) {
			res = _play_file(chan, "im-sorry");
			if (res == -1) {
				goto out;
			}
			continue;
		}
				
		/* if the time entered is less than 1300 we need to clear up am/pm */
		if (time_int < 1300 && time_int >= 100) {
			for(;;) {
				/* playback am or pm question */
				res = _play_file(chan, "1-for-am-2-for-pm");
				if ((char)res == '1' || (char)res == '2') {
					ampm = (char)res;
					break;
				} else if (res == -1) {
					goto out;
				} else if (res != 0) {
					continue;
				} else {
					res = _play_file(chan, "silence/5");
					if ((char)res == '1' || (char)res == '2') {
						ampm = (char)res;
						break;
					} else if (res == -1) {
						goto out;
					} else {
						continue;
					}
				}
			}
		}
		
		/* adjust time integer for am/pm */
		if (time_int < 1200 && ampm == '2') {
			time_int += 1200;
		} else if (time_int >= 1200 && ampm == '1') {
			time_int -= 1200;
		}
	
		for(;;) { 
			timer = time(NULL);
			time_struct = localtime(&timer);
			time_struct->tm_hour = time_int/100;
			time_struct->tm_min = time_int%100;
			timer = mktime(time_struct);

			/* playback entry to confirm */
			res = _play_file(chan, "rqsted-wakeup-for");

			if (!res) {
				res = ast_say_time(chan, timer, AST_DIGIT_ANY, chan->language);
			}
			if (!res) {
				res = _play_file(chan, "1-yes-2-no");
			}
			
			/* XXX prompting needs work
			if (!res) {
				res = _play_file(chan, "press-1");
			}
			if (!res) {
				res = _play_file(chan, "to-confirm-wakeup");
			}
			if (!res) {
				res = _play_file(chan, "press-2");
			}
			if (!res) {
				res = _play_file(chan, "for");
			}
			if (!res) {
				res = _play_file(chan, "another-time");
			}*/
			
			if (!res) {
				res = _play_file(chan, "silence/5");
			}
		
			if ((char)res == '1' || (char)res == '2' || res < 0) {
				break;
			}
		}
		
		if ((char)res == '1') {
			res = _setwakeup(time_int, chan->cid.cid_num);
			if (res >= 0) {
				res = _play_file(chan, "thank-you-for-calling");
				if (!res) {
					res = _play_file(chan, "goodbye");
				}
			}
			break;
		} else if (res == -1) {
			break;
		}
	}
out:
	ast_module_user_remove(u);
	return res;
}

static int unload_module(void) {

	int res;

	res = ast_unregister_application(app);

	ast_module_user_hangup_all();

	return res;	
}

static int load_module(void) {
	return ast_register_application(app, wakeme_exec, synopsis, descrip);
}

static int _play_file(struct ast_channel* chan, char* filename) {
	int res;

	ast_stopstream(chan);
	res = ast_streamfile(chan, filename, chan->language);

	if(!res)
		res = ast_waitstream(chan, AST_DIGIT_ANY);
	else
		res = 0;

	ast_stopstream(chan);

	return res;
}


static int _file_filter(struct dirent *entry) {
	return((int)strstr(entry->d_name,"wakeme"));
}

static int _setwakeup(int time_int, char *cid) {
	
	time_t timer;					/* init for tm structure			*/
	struct tm *time_struct = NULL;	/* time structure for ast_say_time	*/
	int cur_time_int = 0;			/* time converted from tm struct	*/
	char touch_string[13];			/* buffer for touch -t string		*/
	char buffer[256];				/* general purpose buffer 			*/
	FILE *call_file;				/* call file handle					*/
	char command[128];				/* sys command string buffer		*/
	int res = 0;					/* result 							*/
	unsigned char month_days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
	
	/* get our machine time */
	timer = time(NULL);
	time_struct = localtime(&timer);
	cur_time_int += time_struct->tm_hour * 100 + time_struct->tm_min;

	/* setup initial touch_time for today at wake time */
	time_struct->tm_hour = time_int/100;
	time_struct->tm_min = time_int%100;

	/* if the wake time is earlier than our present time we need to
	 * adjust the day that the wake-up-call will occur */
	if (time_int < cur_time_int) {

		/* year change */
		if (time_struct->tm_mon == 11 && time_struct->tm_mday == 31) {
			time_struct->tm_year++;
			time_struct->tm_mon = 0;
			time_struct->tm_mday = 1;
	
		/* leap year */
		} else if (time_struct->tm_mon == 2 && time_struct->tm_mday == 28 && time_struct->tm_year % 4 == 0 ) {
			time_struct->tm_mday++;
			
		/* month change */
		} else if (month_days[time_struct->tm_mon] <= time_struct->tm_mday) {
			time_struct->tm_mon++;
			time_struct->tm_mday = 1;
		
		/* day change */
		} else {
			time_struct->tm_mday++;
		}		
	}

	strftime(touch_string, 13, "%Y%m%d%H%M", time_struct);
	
	/* apply touch -t string and write out file */ 
	sprintf(buffer, "/tmp/wakeme_%04d_ext_%s.call", time_int, cid);
	
	if ((call_file = fopen(buffer, "w")) == NULL) {
		fprintf(stderr, "Cannot open %s\n", buffer);
		res = -1;
	} else {
		char buff[256];
		sprintf(buff, 
			"Channel: Local/%s@internal\n"
			"Callerid: Wake UP!\n"
			"MaxRetries: 5\n"
			"RetryTime: 60\n"
			"WaitTime: 15\n"
			"Application: WakeMe\n"
			"Data: %s", cid, cid);
		fprintf(call_file, buff);
		fflush(call_file);
		fclose(call_file);
		sprintf(command, "touch -t %s %s", touch_string, buffer);
		system(command);
		sprintf(command, "mv %s %s", buffer, spool_path);
		system(command);
		res = 0;
	}

	return res;
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "Wake-up Call Manager");
