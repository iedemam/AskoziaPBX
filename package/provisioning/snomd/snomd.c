/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)

	Copyright (C) 2009-2010 IKT <http://itison-ikt.de>.
	All rights reserved.

	Originally authored by Elisa Martin Caro Zapardiel <elisamcz@gmail.com>

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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#define SIP_PORT	5060
#define SIP_GROUP	"224.0.1.75"
#define MSGBUFSIZE	448

struct info_phone {
	char mac_addr[16];
	char ip_addr[16];
	char port[5];
	char via[64];
	char from[64];
	char to[64];
	char call_id[64];
	char cseq[64];
	char tagFrom[64];
	char fromSinTag[64];
	char event[108];
};

int kaboom() {
	closelog();
	exit(1);
}

int main(int argc, char **argv) {
	int logging = 0;
	int verbose = 0;
	char *url = NULL;
	int ch;
	struct sockaddr_in addr;
	struct ip_mreq mreq;
	int multisocket;
	int unisocket;
	int addrlen;
	char msgbuf[MSGBUFSIZE];
	struct info_phone subscribe;
	int count;
	char *line;

	while ((ch = getopt(argc, argv, "lvu:")) != -1) {
		switch (ch) {
			case 'u':
				url = optarg;
				break;
			case 'l':
				logging = 1;
				break;
			case 'v':
				verbose = 1;
				break;
		}
	}

	if (url == NULL) {
		printf("Please provide a provisioning url, i.e:\n./snomd -u \"http://192.168.1.2/settings.xml\"\n");
		exit(1);
	}

	openlog ("snomd", 0, 0);

	/* setup multicast receive socket... */
	memset(&addr,0,sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=htonl(INADDR_ANY);
	addr.sin_port=htons(SIP_PORT);
	mreq.imr_multiaddr.s_addr=inet_addr(SIP_GROUP);
	mreq.imr_interface.s_addr=htonl(INADDR_ANY);

	/* ...create a UDP socket */
	if ((multisocket = socket(AF_INET,SOCK_DGRAM, 0)) < 0) {
		if (logging) {
			syslog(LOG_ERR, "creating socket failed!");
		}
		kaboom();
	}
	if (logging && verbose) {
		syslog(LOG_INFO, "created socket");
	}

	/* ...allow multiple sockets to use the same PORT number */
	u_int yes = 1;
	if (setsockopt(multisocket, SOL_SOCKET, SO_REUSEADDR ,&yes, sizeof(yes)) < 0) {
		if (logging) {
			syslog(LOG_ERR, "setsockopt failed!");
		}
		kaboom();
	}
	if (logging && verbose) {
		syslog(LOG_INFO, "set socket options");
	}

	/* ...bind to receive address */
	if (bind(multisocket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		if (logging) {
			syslog(LOG_ERR, "bind failed!");
		}
		kaboom();
	}
	if (logging && verbose) {
		syslog(LOG_INFO, "socket bound");
	}

	/* ...request that the kernel join a multicast group */
	if (setsockopt(multisocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
		if (logging) {
			syslog(LOG_ERR, "set multicast group failed!");
		}
		kaboom();
	}
	if (logging && verbose) {
		syslog(LOG_INFO, "set multicast group");
	}

	/* ...start listening */
	while (1) {
		addrlen = sizeof(addr);
		int err = 0;
		if ((err = recvfrom(multisocket, msgbuf, MSGBUFSIZE, 0, (struct sockaddr *) &addr, &addrlen)) < 0) {
			if (logging) {
				syslog(LOG_ERR, "recvfrom failed!");
			}
			kaboom();
		}
		if (logging && verbose) {
			syslog(LOG_INFO, "recvfrom returned");
		}

		if((memcmp(msgbuf, "SUBSCRIBE", 9))!=0) {
			if (logging && verbose) {
				syslog(LOG_INFO, "received something that was not a SUBSCRIBE message\n%s", msgbuf);
			}
			continue;
		}
		if (logging && verbose) {
			syslog(LOG_INFO, "received a SUBSCRIBE message\n%s", msgbuf);
		}

		/* respond if we were sent a SUBSCRIBE message */
		memset(&subscribe, 0, sizeof(subscribe));
		line = strtok(msgbuf, "\n\r"); 
		strncpy(subscribe.mac_addr, line+20, (32-20));
		count = 0;
		while (line!=NULL) {
			line = strtok(NULL, "\n\r");
			count++;

			if ((memcmp(line, "Via", 3))==0) {
				strncpy(subscribe.via, line, strlen(line));
				strncpy(subscribe.ip_addr, line+17, 13);
				strncpy(subscribe.port, line+31, 4);

			} else if ((memcmp(line, "From", 4))==0) {
				strncpy(subscribe.from, line, strlen(line));
				strncpy(subscribe.fromSinTag, line, strlen(line)-15);
				strncpy(subscribe.tagFrom, line+46, 14);

			} else if ((memcmp(line, "To", 2))==0) {
				strncpy(subscribe.to, line, strlen(line));

			} else if ((memcmp(line, "Call-ID", 7))==0) {
				strncpy(subscribe.call_id, line, strlen(line));

			} else if ((memcmp(line, "CSeq", 4))==0) {
				strncpy(subscribe.cseq, line+6, 1);

			} else if((memcmp(line, "Event", 5))==0) {
				strncpy(subscribe.event, line, strlen(line));
			}

			/* exit while */
			if (count == 6){
				break;
			}
		}

		/* build 200 OK */
		char my_ip_addr [255];
		gethostname(my_ip_addr, sizeof(my_ip_addr));
		char ok_response[480]="SIP/2.0 200 OK\r\n";
		strncat(ok_response, subscribe.via, strlen(subscribe.via));
		strncat(ok_response, "\r\n", 2);
		strncat(ok_response, "Contact: <sip:", 14);
		strncat(ok_response, my_ip_addr , strlen(my_ip_addr));	
		strncat(ok_response, ":5060>\r\n", 8);
		strncat(ok_response, subscribe.to, strlen(subscribe.to));
		strncat(ok_response, ";tag=12344321\r\n", 15);
		strncat(ok_response, subscribe.from, strlen(subscribe.from));
		strncat(ok_response, "\r\n", 2);
		strncat(ok_response, subscribe.call_id, strlen(subscribe.call_id));
		strncat(ok_response, "\r\n", 2);
		strncat(ok_response, "Cseq: ", 6);
		strncat(ok_response, subscribe.cseq, strlen(subscribe.cseq));
		strncat(ok_response, " SUBSCRIBE\r\nExpires: 0\r\nContent-Length: 0\r\n\r\n", 45);

		/* send 200 OK */
		memset(&addr,0,sizeof(addr));
		addr.sin_family=AF_INET;
		addr.sin_addr.s_addr=inet_addr(subscribe.ip_addr);
		addr.sin_port=htons(atoi(subscribe.port));	
		if ((unisocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
			if (logging) {
				syslog(LOG_ERR, "sending 200OK socket() failed!");
			}
			kaboom();
		}
		if (sendto(unisocket, ok_response, sizeof(ok_response), 0, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
			if (logging) {
				syslog(LOG_ERR, "sending 200OK sendto() failed!");
			}
			kaboom();
		}
		if (logging && verbose) {
			syslog(LOG_INFO, "sent 200OK\n%s", ok_response);
		}

		/* build NOTIFY */
		char notify [868] = "NOTIFY sip:";
		strncat(notify, subscribe.ip_addr, strlen(subscribe.ip_addr));
		strncat(notify, ":", 1);
		strncat(notify, subscribe.port, strlen(subscribe.port));
		strncat(notify, " SIP/2.0\r\n", 10);
		strncat(notify, "Via: SIP/2.0/UDP ", 17);
		strncat(notify, my_ip_addr, strlen(my_ip_addr));
		strncat(notify,	":5060\r\n", 7);
		strncat(notify, "Max-Forwards: 70\r\n", 19);
		strncat(notify, "Contact: <sip:", 14);
		strncat(notify, my_ip_addr, strlen(my_ip_addr));
		strncat(notify, ":", 1);
		strncat(notify, subscribe.port, strlen(subscribe.port));
		strncat(notify, ">\r\n", 3);
		strncat(notify, subscribe.fromSinTag, strlen(subscribe.from));
		strncat(notify, ";tag=12344321\r\n", 15);	
		strncat(notify, subscribe.to, strlen(subscribe.to));
		strncat(notify, ";", 4);
		strncat(notify, subscribe.tagFrom, strlen(subscribe.tagFrom));
		strncat(notify, "\r\n", 2);
		strncat(notify, subscribe.call_id, strlen(subscribe.call_id));
		strncat(notify, "\r\n", 2);
		int num = atoi(subscribe.cseq);
		int suma = num + 1;
		char seq [16];
		sprintf( seq, "%d\r\n", suma );
		strncat(notify, "CSeq: ", 6);
		strncat(notify, seq, 1);
		strncat(notify, " NOTIFY\r\n", 9);
		strncat(notify, "Content-Type: application/url\r\n", 31); 
		strncat(notify, "Subscription-State: terminated;reason=timeout\r\n", 47);
		strncat(notify,"Event: ua-profile;profile-type=\"device\";vendor=\"snom\";model=\"snom360\";version=\"7.4.19\"", 94);
		strncat(notify, "\r\n", 2);
		strncat(notify, "Content-Length:58\r\n\r\n", 21);
		strncat(notify, url, strlen(url));
		strncat(notify, "\r\n", 2);

		/* send NOTIFY */
		if (sendto(unisocket, notify, sizeof(notify), 0, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
			if (logging) {
				syslog(LOG_ERR, "sending NOTIFY sendto() failed!");
			}
			kaboom();
		}
		if (logging && verbose) {
			syslog(LOG_INFO, "sent NOTIFY\n%s", notify);
		}
	}

	closelog();
	exit(EXIT_SUCCESS);
}

