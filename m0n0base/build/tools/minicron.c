/*
	$Id: minicron.c 52 2006-02-07 14:35:14Z mkasper $
	part of m0n0wall (http://m0n0.ch/wall)
	
	Copyright (C) 2003-2004 Manuel Kasper <mk@neon1.net>.
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

#include <stdio.h>
#include <stdlib.h>

/* usage: minicron interval pidfile cmd */

int main(int argc, char *argv[]) {
	
	int interval;
	FILE *pidfd;
	
	if (argc < 4)
		exit(1);
	
	interval = atoi(argv[1]);
	if (interval == 0)
		exit(1);
	
	/* unset loads of CGI environment variables */
	unsetenv("CONTENT_TYPE"); unsetenv("GATEWAY_INTERFACE");
	unsetenv("REMOTE_USER"); unsetenv("REMOTE_ADDR");
	unsetenv("AUTH_TYPE"); unsetenv("SCRIPT_FILENAME");
	unsetenv("CONTENT_LENGTH"); unsetenv("HTTP_USER_AGENT");
	unsetenv("HTTP_HOST"); unsetenv("SERVER_SOFTWARE");
	unsetenv("HTTP_REFERER"); unsetenv("SERVER_PROTOCOL");
	unsetenv("REQUEST_METHOD"); unsetenv("SERVER_PORT");
	unsetenv("SCRIPT_NAME"); unsetenv("SERVER_NAME");
	
	/* go into background */
	if (daemon(0, 0) == -1)
		exit(1);
	
	/* write PID to file */
	pidfd = fopen(argv[2], "w");
	if (pidfd) {
		fprintf(pidfd, "%d\n", getpid());
		fclose(pidfd);
	}
		
	while (1) {
		sleep(interval);
		
		system(argv[3]);
	}
}
