/*
	$Id: stats.c 52 2006-02-07 14:35:14Z mkasper $
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
#include <net/if.h>
#include <net/if_mib.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/dkstat.h>

void cpu_stats() {

	long cp_time1[CPUSTATES], cp_time2[CPUSTATES];
	long total1, total2;
	size_t len;
	double cpuload;

	len = sizeof(cp_time1);

	if (sysctlbyname("kern.cp_time", &cp_time1, &len, NULL, 0) < 0)
		exit(1);

	sleep(1);

	len = sizeof(cp_time2);

	if (sysctlbyname("kern.cp_time", &cp_time2, &len, NULL, 0) < 0)
		exit(1);

	total1 = cp_time1[CP_USER] + cp_time1[CP_NICE] + cp_time1[CP_SYS] + 
			 cp_time1[CP_INTR] + cp_time1[CP_IDLE];
	total2 = cp_time2[CP_USER] + cp_time2[CP_NICE] + cp_time2[CP_SYS] + 
			 cp_time2[CP_INTR] + cp_time2[CP_IDLE];

	cpuload = 1 - ((double)(cp_time2[CP_IDLE] - cp_time1[CP_IDLE]) / (double)(total2 - total1));

	printf("%.0f\n", 100.0*cpuload);
}

void if_stats(char *cl) {

	struct ifmibdata	ifmd;
	size_t				ifmd_size =	sizeof(ifmd);
	int					nr_network_devs;
	size_t				int_size = sizeof(nr_network_devs);
	int					name[6];
	int					i;
	struct timeval		tv;
	double				uusec;
	
	/* check interface name syntax */
	for (i = 0; cl[i]; i++) {
		if (!((cl[i] >= 'a' && cl[i] <= 'z') || (cl[i] >= '0' && cl[i] <= '9')))
			exit(1);	
	}

	name[0] = CTL_NET;
	name[1] = PF_LINK;
	name[2] = NETLINK_GENERIC;
	name[3] = IFMIB_IFDATA; 	name[5] = IFDATA_GENERAL;

	if (sysctlbyname("net.link.generic.system.ifcount", &nr_network_devs,
		&int_size, (void*)0, 0) == -1) {
		
		exit(1);
	
	} else {    
		
		for (i = 1; i <= nr_network_devs; i++) {
			
			name[4] = i;    /* row of the ifmib table */
			
			if (sysctl(name, 6, &ifmd, &ifmd_size, (void*)0, 0) == -1) {    
				continue;
			}
			
			if (strncmp(ifmd.ifmd_name, cl, strlen(cl)) == 0) {
				gettimeofday(&tv, NULL);
				uusec = (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
				printf("%lf|%u|%u\n", uusec,
					ifmd.ifmd_data.ifi_ibytes, ifmd.ifmd_data.ifi_obytes);
				exit(0);
			}
		}
	}
}

int main(int argc, char *argv[]) {
	
	char				*cl, *rm;
	
	printf("Content-Type: text/plain\n\n");
	
	rm = getenv("REQUEST_METHOD");
	if (rm == NULL)
		exit(1);
	if (strcmp(rm, "GET") != 0)
		exit(1);
		
	cl = getenv("QUERY_STRING");
	if (cl == NULL)
		exit(1);
	
	if ((strlen(cl) < 3) || (strlen(cl) > 16))
		exit(1);
	
	if (strcmp(cl, "cpu") == 0)
		cpu_stats();
	else
		if_stats(cl);
	
	return 0;
}
