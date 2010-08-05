/*
	$Id: stats.c 52 2006-02-07 14:35:14Z mkasper $
	part of AskoziaPBX (http://askozia.com/pbx)
    
	Copyright (C) 2007-2008 tecema (a.k.a IKT) <http://www.tecema.de>.
	All rights reserved.

	get_stat_cpu and get_stat_network originally 
	part of m0n0wall (http://m0n0.ch/wall)
	Copyright (C) 2004-2005 Manuel Kasper <mk@neon1.net>.
	All rights reserved.
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	
	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.
	
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	
	THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
	AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
	OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
//#include <net/if_mib.h>
#include <sys/time.h>
//#include <sys/dkstat.h>

char * request_method;
char * query_string;

void print_debug(char * message) {
	printf("<debug>%s</debug>\n", message);
}

void get_request_method_and_query_string() {

	request_method = getenv("REQUEST_METHOD");
	if (request_method == NULL) {
		print_debug("get_request_method_and_query_string: no request method defined!");
		exit(1);
	}
	if (strcmp(request_method, "GET") != 0) {
		print_debug("get_request_method_and_query_string: must be a GET request!");
		exit(1);
	}
	
	query_string = getenv("QUERY_STRING");
	if (query_string == NULL) {
		print_debug("get_request_method_and_query_string: no query string defined!");
		exit(1);
	}
	// XXX : length checks
}

int char_count(char * str, int ch) {

	int i;
	int len = strlen(str);
	int found = 0;

	for (i = 0; i < len; i++) {
		if (str[i] == ch) {
			found++;
		}
	}

	return found;
}

void decode_url(char * string) {

	int i = 0;
	int ii = 0;
	int length = strlen(string);

	char code[5], *end;
	code[0] = '0';
	code[1] = 'x';
	code[4] = 0;

	for (i = 0; i < length; i++) {
		if (string[i] == '+') {
			string[i] = ' ';

		} else if (string[i] == '%') {
			code[2] = string[i+1];
			code[3] = string[i+2];
			string[i] = strtol(code, &end, 16);
			for (ii = i+1; ii < length; ii++) {
				string[ii] = string[ii+2];
			}
			length -= 2;
		}
	}
}

void exec_shell(char * command) {

	FILE * shell_pipe;
	char output_buffer[81];
	char * postfix = " 2>&1";
	char total_command[strlen(command) + strlen(postfix) + 1];

	strcpy(total_command, command);
	strcat(total_command, postfix);

	shell_pipe = popen(total_command, "r");
	if (shell_pipe == NULL) {
		print_debug("exec_shell: process pipe creation failed!");
		exit(1);
	}

	while (fgets(output_buffer, 80, shell_pipe) != NULL) {
		printf("%s", output_buffer);
	}

	pclose(shell_pipe);
}

void exec_ami(char * command) {

	FILE * shell_pipe;
	char output_buffer[81];
	char * prefix = "/usr/sbin/asterisk -n -rx ";
	char total_command[strlen(prefix) + strlen(command) + 1];

	strcpy(total_command, prefix);
	strcat(total_command, command);

	shell_pipe = popen(total_command, "r");
	if (shell_pipe == NULL) {
		print_debug("exec_ami: process pipe creation failed!");
		exit(1);
	}

	while (fgets(output_buffer, 80, shell_pipe) != NULL) {
		printf("%s", output_buffer);
	}

	pclose(shell_pipe);
}

void get_stat_cpu() {

	FILE *f1, *f2;
	char line1[256], line2[256];
	unsigned long long user1, nice1, system1, idle1, iowait1, irq1, softirq1;
	unsigned long long user2, nice2, system2, idle2, iowait2, irq2, softirq2;
	unsigned long long total1, total2;
	double cpuload;

	f1 = fopen("/proc/stat", "r");
	if (f1) {
		fgets(line1, 256, f1);
		fclose(f1);
		sscanf(line1, "cpu  %llu %llu %llu %llu %llu %llu %llu",
			&user1, &nice1, &system1, &idle1, &iowait1, &irq1, &softirq1);

		sleep(1);

		f2 = fopen("/proc/stat", "r");
		if (f2) {
			fgets(line2, 256, f2);
			fclose(f2);
			sscanf(line2, "cpu  %llu %llu %llu %llu %llu %llu %llu",
				&user2, &nice2, &system2, &idle2, &iowait2, &irq2, &softirq2);

			total1 = user1 + nice1 + system1 + idle1 + iowait1 + irq1 + softirq1;
			total2 = user2 + nice2 + system2 + idle2 + iowait2 + irq2 + softirq2;
			//printf("user-diff    : %llu\n", user2-user1);
			//printf("nice-diff    : %llu\n", nice2-nice1);
			//printf("system-diff  : %llu\n", system2-system1);
			//printf("idle-diff    : %llu\n", idle2-idle1);
			//printf("iowait-diff  : %llu\n", iowait2-iowait1);
			//printf("irq-diff     : %llu\n", irq2-irq1);
			//printf("softirq-diff : %llu\n", softirq2-softirq1);
			//printf("total-diff   : %llu\n", total2-total1);

			cpuload = 1 - ((double)(idle2 - idle1) / (double)(total2 - total1));

			printf("%.0f\n", 100.0*cpuload);
		}
	}
}

void get_stat_network() {
	FILE *f;
	char line[256];
	unsigned long long c1, c2, c3, c4, c5, c6, c7, c8, c9;
	unsigned long long tx, rx;
	struct timeval tv;
	double uusec;

	f = fopen("/proc/net/dev", "r");
	if (f) {
		do {
			fgets(line, 256, f);
		} while (strstr(line, "eth0") == NULL);
		fclose(f);

		if (strstr(line, "eth0: ")) {
			sscanf(line, "%*[ ]eth0:%*[ ]%llu%*[ ]%llu%*[ ]%llu%*[ ]%llu%*[ ]%llu%*[ ]%llu%*[ ]%llu%*[ ]%llu%*[ ]%llu",
				&c1, &c2, &c3, &c4, &c5, &c6, &c7, &c8, &c9);
		} else if (strstr(line, "eth0:")) {
			sscanf(line, "%*[ ]eth0:%llu%*[ ]%llu%*[ ]%llu%*[ ]%llu%*[ ]%llu%*[ ]%llu%*[ ]%llu%*[ ]%llu%*[ ]%llu",
				&c1, &c2, &c3, &c4, &c5, &c6, &c7, &c8, &c9);
		}
		rx = c1;
		tx = c9;

		gettimeofday(&tv, NULL);
		uusec = (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0; 
		printf("%lf|%llu|%llu\n", uusec, rx*8, tx*8);
	}
}

int main(int argc, char *argv[]) {

	get_request_method_and_query_string();

	char * raw_commands[char_count(query_string,'&')+1];
	char * next_command_name;
	char * next_command_string;

	printf("Content-Type: text/plain\n\n");

	/* separate raw command strings and decode string */
	int i = 0;
	raw_commands[i] = strtok(query_string, "&");
	while (raw_commands[i] != NULL) {
		decode_url(raw_commands[i]);
		raw_commands[++i] = strtok(NULL, "&");
	}

	/* separate and execute command pairs */
	int max = i;
	for (i = 0; i < max; i++) {
		next_command_name = strtok(raw_commands[i], "=");
		next_command_string = strtok(NULL, "=");

		/* execute shell command */
		if (strcmp(next_command_name, "exec_shell") == 0) {
			exec_shell(next_command_string);

		/* execute asterisk manager interface */
		} else if (strcmp(next_command_name, "exec_ami") == 0) {
			exec_ami(next_command_string);

		/* get statistic cpu load or current usage */
		} else if (strcmp(next_command_name, "get_stat_cpu") == 0) {
			get_stat_cpu();

		/* get network interface statistics */
		} else if (strcmp(next_command_name, "get_stat_network") == 0) {
			//get_stat_network(next_command_string); LINUX TODO : defaults to eth0
			get_stat_network();

		/* uh-oh! */
		} else {
			printf("ERROR: unknown command name (%s)!", next_command_name);
			exit(1);
		}
	}

	return 0;
}
