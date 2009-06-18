/*
	$Id: stats.c 52 2006-02-07 14:35:14Z mkasper $
	part of AskoziaPBX (http://askozia.com/pbx)
    
	Copyright (C) 2007-2008 IKT <http://itison-ikt.de>.
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
	char * prefix = "/usr/sbin/asterisk -rx ";
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

//void get_stat_cpu() {
//
//	long cp_time1[CPUSTATES], cp_time2[CPUSTATES];
//	long total1, total2;
//	size_t len;
//	double cpuload;
//
//	len = sizeof(cp_time1);
//
//	if (sysctlbyname("kern.cp_time", &cp_time1, &len, NULL, 0) < 0)
//		exit(1);
//
//	sleep(1);
//
//	len = sizeof(cp_time2);
//
//	if (sysctlbyname("kern.cp_time", &cp_time2, &len, NULL, 0) < 0)
//		exit(1);
//
//	total1 = cp_time1[CP_USER] + cp_time1[CP_NICE] + cp_time1[CP_SYS] + 
//			 cp_time1[CP_INTR] + cp_time1[CP_IDLE];
//	total2 = cp_time2[CP_USER] + cp_time2[CP_NICE] + cp_time2[CP_SYS] + 
//			 cp_time2[CP_INTR] + cp_time2[CP_IDLE];
//
//	cpuload = 1 - ((double)(cp_time2[CP_IDLE] - cp_time1[CP_IDLE]) / (double)(total2 - total1));
//
//	printf("%.0f\n", 100.0*cpuload);
//}
//
//void get_stat_network(char *cl) {
//
//	struct ifmibdata	ifmd;
//	size_t				ifmd_size =	sizeof(ifmd);
//	int					nr_network_devs;
//	size_t				int_size = sizeof(nr_network_devs);
//	int					name[6];
//	int					i;
//	struct timeval		tv;
//	double				uusec;
//	
//	/* check interface name syntax */
//	for (i = 0; cl[i]; i++) {
//		if (!((cl[i] >= 'a' && cl[i] <= 'z') || (cl[i] >= '0' && cl[i] <= '9')))
//			exit(1);	
//	}
//
//	name[0] = CTL_NET;
//	name[1] = PF_LINK;
//	name[2] = NETLINK_GENERIC;
//	name[3] = IFMIB_IFDATA; 	name[5] = IFDATA_GENERAL;
//
//	if (sysctlbyname("net.link.generic.system.ifcount", &nr_network_devs,
//		&int_size, (void*)0, 0) == -1) {
//		
//		exit(1);
//	
//	} else {    
//		
//		for (i = 1; i <= nr_network_devs; i++) {
//			
//			name[4] = i;    /* row of the ifmib table */
//			
//			if (sysctl(name, 6, &ifmd, &ifmd_size, (void*)0, 0) == -1) {    
//				continue;
//			}
//			
//			if (strncmp(ifmd.ifmd_name, cl, strlen(cl)) == 0) {
//				gettimeofday(&tv, NULL);
//				uusec = (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
//				printf("%lf|%u|%u\n", uusec,
//					ifmd.ifmd_data.ifi_ibytes, ifmd.ifmd_data.ifi_obytes);
//				exit(0);
//			}
//		}
//	}
//}

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

		///* get statistic cpu load or current usage */
		//} else if (strcmp(next_command_name, "get_stat_cpu") == 0) {
		//	get_stat_cpu(next_command_string);

		///* get network interface statistics */
		//} else if (strcmp(next_command_name, "get_stat_network") == 0) {
		//	get_stat_network(next_command_string);

		/* uh-oh! */
		} else {
			printf("ERROR: unknown command name (%s)!", next_command_name);
			exit(1);
		}
	}

	return 0;
}
