/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) 2006  Michael Iedema <mi.iedema@fh-wolfenbuettel.de>
 *	sponsored by: 
 *	Braunschweig / Wolfenbuettel School of Applied Sciences
 *	http://fh-wolfenbuettel.de
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

/*! 
 * \file
 * \author Michael Iedema <mi.iedema@fh-wolfenbuettel.de>
 *
 * \brief Records system and Asterisk statistics to a space delimited file.
 * 
 */

#include "asterisk.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "asterisk/lock.h"
#include "asterisk/channel.h"
#include "asterisk/logger.h"
#include "asterisk/file.h"
#include "asterisk/pbx.h"
#include "asterisk/module.h"
#include "asterisk/manager.h"
#include "asterisk/cli.h"
#include "asterisk/monitor.h"
#include "asterisk/app.h"
#include "asterisk/utils.h"
#include "asterisk/config.h"
#include "asterisk/build.h"


static char *qstat_version = "v0.2.0";
static char *qstat_path = "/var/asterisk/log/qstat/";


/* pthread run condition */
int res_qstat_dont_stop;
/* the actual worker thread */
static pthread_t thread = AST_PTHREADT_NULL;
/* storage place for session description given on the cli */
char log_description[512];


static void *qstat_thread(void *data);
//int get_open_file_descriptor_count(void);
int get_active_memory_count(void);
int get_cpu_ticks(unsigned long long *ticks);
//int get_pid_ticks(long target_pid, unsigned long *ticks);
//int get_pid_thread_count(long);


/* qstat worker thread */
static void *qstat_thread(void *data) {
	time_t timer;
	struct tm *time_struct;
	char filename[15];
	char buffer[256];
	FILE *statfile;
	double load_average_values[1];
	unsigned int second_count = 0;
	char description[512];
	int res, i;
	//long my_pid;

	unsigned long long ticks_sum;
	unsigned long long old_ticks[8], new_ticks[8];
	//unsigned long old_pid_ticks[2], new_pid_ticks[2];
	float cpu_stats[10];

	/* set the pid */
	//my_pid = (long)getpid();

	/* grab the time */
	timer = time(NULL);
	time_struct = localtime(&timer);

	/* create missing directories */
	if (chdir(qstat_path) != 0) {
		sprintf(buffer, "mkdir -p %s", qstat_path);
		system(buffer);
		if (chdir(qstat_path) != 0) {
			return (void*)-1;
		}
	}

	/* create file in qstat_path */
	strftime(filename, 15, "%Y%m%d%H%M%S", time_struct);	
	sprintf(buffer, "%s%s.qstat", qstat_path, filename);

	/* fail on creation failure */
	if ((statfile = fopen(buffer, "w")) == NULL) {
		return (void*)-1;
	}

	/* if we have an additional argument save it as our log description */
	sprintf(description,"%s",(char*)data);

	/* print out file header */
	fprintf(statfile, 
		"# qstat statistics file\n"
		"#\n"
		"# qstat version: %s\n"
		"# timestamp: %s\n"
		"#\n"
		"# description: %s\n"
		"#\n"
		"# asterisk build info:\n"
		"#   hostname: %s\n"
		"#   kernel:   %s\n"
		"#   machine:  %s\n"
		"#   os:       %s\n"
		"#   date:     %s\n"
		"#   user:     %s\n"
		"#\n"
		"# fields:\n"
		//"# seconds | load avg (1 min) | call count | file desc. count "
		//	"| active ram (kB) | cpu user | cpu nice | cpu system "
		//	"| cpu idle | cpu iowait | cpu irq | cpu softirq "
		//	"| cpu asterisk user | cpu asterisk system "
		//	"| asterisk thread count\n",
		"# seconds | processed calls | active calls | active channels | active ram (kB) | load avg (1 min) | cpu user | cpu nice | cpu system | cpu idle | cpu iowait | cpu irq | cpu softirq | cpu steal\n",
		qstat_version,
		filename,
		description,
		BUILD_HOSTNAME,
		BUILD_KERNEL,
		BUILD_MACHINE,
		BUILD_OS,
		BUILD_DATE,
		BUILD_USER);
	fflush(statfile);

	/* grab new ticks */
	res = get_cpu_ticks(old_ticks);
	//res = get_pid_ticks(my_pid, old_pid_ticks);
	sleep(1);

	/* while() until we get a stop signal */
	while(res_qstat_dont_stop) {

		/* grab new ticks */
		res = get_cpu_ticks(new_ticks);
		//res = get_pid_ticks(my_pid, new_pid_ticks);

		/* calculate difference & sum */
		ticks_sum = 0;
		for(i = 0; i < 8; i++) {
			old_ticks[i] = new_ticks[i] - old_ticks[i];
			ticks_sum += old_ticks[i];
		}

		/* calculate field percentages */
		for(i = 0; i < 8; i++) {
			cpu_stats[i] = (float)old_ticks[i]/(float)ticks_sum*100;
		}

		/* calculate pid difference & field percentages */
		//old_pid_ticks[0] = new_pid_ticks[0] - old_pid_ticks[0];
		//old_pid_ticks[1] = new_pid_ticks[1] - old_pid_ticks[1];
		//cpu_stats[8] = (float)old_pid_ticks[0]/(float)ticks_sum*100;
		//cpu_stats[9] = (float)old_pid_ticks[1]/(float)ticks_sum*100;

		getloadavg(load_average_values, 1);

		//fprintf(statfile, "%d %d %f %d %d %f %f %f %f %f %f %f %f %f %f %d\n", 
		//	second_count++,						/* second count */
		//	ast_active_calls(), 				/* active call count */
		//	load_average_values[0], 			/* 1 min load average */
		//	get_open_file_descriptor_count(),	/* open file descriptor count */
		//	get_active_memory_count(),			/* active memory (kB) */
		//	cpu_stats[0],						/* cpu user */
		//	cpu_stats[1],						/* cpu nice */
		//	cpu_stats[2],						/* cpu system */
		//	cpu_stats[3],						/* cpu idle */
		//	cpu_stats[4],						/* cpu iowait */
		//	cpu_stats[5],						/* cpu irq */
		//	cpu_stats[6],						/* cpu softirq */
		//	cpu_stats[7],						/* cpu steal */
		//	cpu_stats[8],						/* cpu asterisk user */
		//	cpu_stats[9],						/* cpu asterisk system */
		//	get_pid_thread_count(my_pid));		/* asterisk thread count */

		// seconds | processed calls | active calls | active channels | active ram (kB) | load avg (1 min) | cpu user | cpu nice | cpu system | cpu idle | cpu iowait | cpu irq | cpu softirq | cpu steal
		fprintf(statfile, "%d %d %d %d %d %f %f %f %f %f %f %f %f %f\n", 
			second_count++,						/* second count */
			ast_processed_calls(),				/* processed call count */
			ast_active_calls(), 				/* active call count */
			ast_active_channels(), 				/* active channel count */
			get_active_memory_count(),			/* active memory (kB) */
			load_average_values[0], 			/* 1 min load average */
			cpu_stats[0],						/* cpu user */
			cpu_stats[1],						/* cpu nice */
			cpu_stats[2],						/* cpu system */
			cpu_stats[3],						/* cpu idle */
			cpu_stats[4],						/* cpu iowait */
			cpu_stats[5],						/* cpu irq */
			cpu_stats[6],						/* cpu softirq */
			cpu_stats[7]);						/* cpu steal */
		fflush(statfile);

		for(i = 0; i < 8; i++) {
			old_ticks[i] = new_ticks[i];
		}

		//old_pid_ticks[0] = new_pid_ticks[0];
		//old_pid_ticks[1] = new_pid_ticks[1];

		sleep(1);
	}

	fclose(statfile);

	return (void*)second_count;
}

/* returns number of open file descriptors */
//int get_open_file_descriptor_count(void) {
//	int res = -1;
//	FILE *f = fopen("/proc/sys/fs/file-nr", "r");
//	if(!f) {
//		return res;
//	}
//	fscanf(f,"%d",&res);
//	fclose(f);
//	return res;
//}

/* returns the number of kilobytes of active memory */
int get_active_memory_count(void) {
	int res = -1;
	FILE *f = fopen("/proc/meminfo", "r");
	if(!f) {
		return res;
	}
	fscanf(f,"%*[^A]");
	fscanf(f,"%*s\t%d",&res);
	fclose(f);
	return res;
}


int get_cpu_ticks(unsigned long long *ticks) {
	FILE* f;
	f = fopen("/proc/stat", "r");
	if(!f) {
		return -1;
	}
	fscanf(f,"cpu  %llu %llu %llu %llu %llu %llu %llu %llu\n", 
		&ticks[0],&ticks[1],&ticks[2],&ticks[3],
		&ticks[4],&ticks[5],&ticks[6],&ticks[7]);
	fclose(f);

	return 1;
}


//int get_pid_ticks(long target_pid, unsigned long *ticks) {
//	FILE* f;
//	char _buffer[16];
//
//	sprintf(_buffer, "/proc/%ld/stat", target_pid);
//
//	f = fopen(_buffer, "r");
//	if(!f) {
//		return -1;
//	}
//
//	fscanf(f,"%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s "
//		"%*s %*s %*s %lu %lu",
//		&ticks[0],
//		&ticks[1]);
//	fclose(f);
//
//	return 1;
//}


//int get_pid_thread_count(long target_pid) {
//	FILE* f;
//	char _buffer[16];
//	int count;
//
//	sprintf(_buffer, "/proc/%ld/stat", target_pid);
//
//	f = fopen(_buffer, "r");
//	if(!f) {
//		return -1;
//	}
//
//	fscanf(f,"%*s %*s %*s %*s %*s %*s %*s %*s %*s %*s "
//		"%*s %*s %*s %*s %*s %*s %*s %*s %*s %d", 
//		&count);
//	fclose(f);
//
//	return count;
//}


static char *handle_qstat(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	void *record_count;
	static char *choices[] = { "start", "stop", NULL };
	char *res;
	switch (cmd) {
		case CLI_INIT:
			e->command = "qstat";
			e->usage = "qstat start (description)\n"
				"qstat stop";
			return NULL;

		case CLI_GENERATE:
			if (a->pos != 1) {
				return NULL;
			}
			/* ugly, can be removed when CLI entries have ast_module pointers */
			ast_module_ref(ast_module_info->self);
			res = ast_cli_complete(a->word, choices, a->n);
			ast_module_unref(ast_module_info->self);

			return res;
	}

	if (ast_strlen_zero(a->argv[1])) {
		return CLI_SHOWUSAGE;
	}

	/* ugly, can be removed when CLI entries have ast_module pointers */
	ast_module_ref(ast_module_info->self);

	if (!strcasecmp("start", a->argv[1])) {
		if (thread != AST_PTHREADT_NULL) {
			ast_cli(a->fd, "[qstat] qstat is already running, "
				"stop first\n");
		} else {
			res_qstat_dont_stop = 1;
			if (a->argc == 3) {
				sprintf(log_description,"%s", a->argv[2]);
			} else {
				sprintf(log_description," ");
			}
			res = ast_pthread_create(&thread, NULL, qstat_thread, log_description);
			ast_cli(a->fd, "[qstat] started\n");
		}
		return CLI_SUCCESS;

	} else if (!strcasecmp("stop", a->argv[1])) {
		if (thread != AST_PTHREADT_NULL) {
			res_qstat_dont_stop = 0;
			pthread_join(thread, &record_count);
			thread = AST_PTHREADT_NULL;
			ast_cli(a->fd, "[qstat] stopped (%d records written)\n", (unsigned int)record_count);
		} else {
			ast_cli(a->fd, "[qstat] qstat is not running.\n");
		}
		return CLI_SUCCESS;

	} else {
		return CLI_SHOWUSAGE;
	}

	ast_module_unref(ast_module_info->self);

	return res;
}

static struct ast_cli_entry cli_cliqstat[] = {
	AST_CLI_DEFINE(handle_qstat, "Gather system load statistics"),
};

/*! \brief Unload qstat module */
static int unload_module(void)
{
	return ast_cli_unregister_multiple(cli_cliqstat, ARRAY_LEN(cli_cliqstat));
}

/*! \brief Load qstat module */
static int load_module(void)
{
	int res;
	res = ast_cli_register_multiple(cli_cliqstat, ARRAY_LEN(cli_cliqstat));
	return res ? AST_MODULE_LOAD_DECLINE : AST_MODULE_LOAD_SUCCESS;
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "Gather system load statistics");
