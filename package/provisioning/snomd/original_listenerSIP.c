/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
    
	Copyright (C) 2009 tecema (a.k.a IKT) <http://www.tecema.de>.
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

/*************cdaemon**************/
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#define DEFAULT_INTERVAL 3
#define DEFAULT_LOGFLAG 0

/*********************************/

#define HELLO_PORT 5060				// SIP PORT 5060
#define HELLO_GROUP "224.0.1.75"	// boradcast SIP: 224.0.1.75
#define MSGBUFSIZE 448

struct sockaddr_in addr;

struct ip_mreq mreq;
struct info_phone phone;

int fd, nbytes,addrlen;    
char msgbuf[MSGBUFSIZE];

// UDP CONNECTION
struct sockaddr_in createSocket(void) {

	u_int yes=1;           

	/* create what looks like an ordinary UDP socket */
	if ((fd=socket(AF_INET,SOCK_DGRAM,0)) < 0) {
		perror("socket");
		exit(1);
	}

	/* allow multiple sockets to use the same PORT number */
	if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) < 0) {
		perror("Reusing ADDR failed");
		exit(1);
	}

	
     /* set up destination address */
     memset(&addr,0,sizeof(addr));
	// inicializamos la estructura
     addr.sin_family=AF_INET;
     addr.sin_addr.s_addr=htonl(INADDR_ANY);	/* N.B.: differs from sender */ //cualquier direccion
     addr.sin_port=htons(HELLO_PORT);			//numero de puerto

	/* bind to receive address */ //conectamos con el socket
	if (bind(fd,(struct sockaddr *) &addr,sizeof(addr)) < 0) {
		perror("bind");
		exit(1);
	}

	/* use setsockopt() to request that the kernel join a multicast group */
	mreq.imr_multiaddr.s_addr=inet_addr(HELLO_GROUP);
	mreq.imr_interface.s_addr=htonl(INADDR_ANY);
	if (setsockopt(fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0) {
		perror("setsockopt");
		exit(1);
	}

	return addr;
}

// INFO PHONE
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

struct info_phone infoPhone(char datos[]) {

	int count=0;
    
	memset(&phone, 0, sizeof(phone));
    
	char *linea;
	//first line @MAC
	linea = strtok(datos, "\n\r"); 
	strncpy(phone.mac_addr, linea+20, (32-20));
    
	while (linea!=NULL) {
		linea = strtok(NULL, "\n\r");
		count++;
		char aux[64];
    
		if ((memcmp(linea, "Via", 3))==0) {
			strncpy(phone.via, linea, strlen(linea));
			strncpy(phone.ip_addr, linea+17, 13);
			strncpy(phone.port, linea+31, 4);
    
		} else if ((memcmp(linea, "From", 4))==0) {
			strncpy(phone.from, linea, strlen(linea));
			strncpy(phone.fromSinTag, linea, strlen(linea)-15);
			strncpy(phone.tagFrom, linea+46, 14);
    
		} else if ((memcmp(linea, "To", 2))==0) {
			strncpy(phone.to, linea, strlen(linea));
    
		} else if ((memcmp(linea, "Call-ID", 7))==0) {
			strncpy(phone.call_id, linea, strlen(linea));
    
		} else if ((memcmp(linea, "CSeq", 4))==0) {
			strncpy(phone.cseq, linea+6, 1);
    
		} else if((memcmp(linea, "Event", 5))==0) {
			strncpy(phone.event, linea, strlen(linea));
		}
    
		// Out while
		if (count == 6){
			linea = NULL;
		}
	}

	return phone;
}

void sender(struct info_phone phone) {

	char my_ip_addr [255];	// "141.41.40.184";
	gethostname(my_ip_addr, sizeof(my_ip_addr));

	// Create a socket to send data
	char ok_response[480]="SIP/2.0 200 OK\r\n";

	// If a phone has been recognized first send 200 OK
	if ((struct info_phone *) &phone!=NULL) {

		strncat(ok_response, phone.via, strlen(phone.via));
		strncat(ok_response, "\r\n", 2);

		strncat(ok_response, "Contact: <sip:", 14);
		strncat(ok_response, my_ip_addr , strlen(my_ip_addr));	
		strncat(ok_response, ":5060>\r\n", 8);

		strncat(ok_response, phone.to, strlen(phone.to));
		strncat(ok_response, ";tag=12344321\r\n", 15);

		strncat(ok_response, phone.from, strlen(phone.from));
		strncat(ok_response, "\r\n", 2);

		strncat(ok_response, phone.call_id, strlen(phone.call_id));
		strncat(ok_response, "\r\n", 2);

		strncat(ok_response, "Cseq: ", 6);
		strncat(ok_response, phone.cseq, strlen(phone.cseq));
		strncat(ok_response, " SUBSCRIBE\r\nExpires: 0\r\nContent-Length: 0\r\n\r\n", 45);
	}

	/* set up destination address */
	memset(&addr,0,sizeof(addr));
	// inicializamos la estructura
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=inet_addr(phone.ip_addr);
	addr.sin_port=htons(atoi(phone.port));	

	if ((fd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0) {
		perror("socket sever");
		exit(1);
	}

	// send 200 OK
	if (sendto(fd,ok_response,sizeof(ok_response),0, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("sendto");
		exit(1);
	}

char notify [868] = "NOTIFY sip:";

	if ((struct info_phone *) &phone!=NULL) {
		strncat(notify, phone.ip_addr, strlen(phone.ip_addr));
		strncat(notify, ":", 1);
		strncat(notify, phone.port, strlen(phone.port));
		strncat(notify, " SIP/2.0\r\n", 10);

		strncat(notify, "Via: SIP/2.0/UDP ", 17);
		strncat(notify, my_ip_addr, strlen(my_ip_addr));
		strncat(notify,	":5060\r\n", 7);

		strncat(notify, "Max-Forwards: 70\r\n", 19);

		strncat(notify, "Contact: <sip:", 14);
		strncat(notify, my_ip_addr, strlen(my_ip_addr));
		strncat(notify, ":", 1);
		strncat(notify, phone.port, strlen(phone.port));
		strncat(notify, ">\r\n", 3);

		strncat(notify, phone.fromSinTag, strlen(phone.from));
		strncat(notify, ";tag=12344321\r\n", 15);	

		strncat(notify, phone.to, strlen(phone.to));
		strncat(notify, ";", 4);
		strncat(notify, phone.tagFrom, strlen(phone.tagFrom));
		strncat(notify, "\r\n", 2);

		strncat(notify, phone.call_id, strlen(phone.call_id));
		strncat(notify, "\r\n", 2);

		// SUMAR UNO A CSEQ

		int num = atoi(phone.cseq);
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

		// Proviosioning URI
		strncat(notify, "Content-Length:58\r\n\r\nhttp://141.41.40.77/~elisa/sipphone/settings.xml?mac={mac}\r\n", 81);

	}

	//send NOTIFY
	if (sendto(fd,notify,sizeof(notify),0, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("sendto");
		exit(1);
	}
}

void listener (struct sockaddr_in addr) {

	/* now just enter a read-print loop */
	// while (1) {
	nbytes=0;		

	// paqute udp broadcast	

	addrlen=sizeof(addr);

	if ((nbytes=recvfrom(fd,msgbuf,MSGBUFSIZE,0, (struct sockaddr *) &addr,&addrlen)) < 0) {
		perror("recvfrom");
		exit(1);
	}
}


int main(int argc, char **argv) {
	static int ch, interval, logflag;
	pid_t pid, sid;

	interval = DEFAULT_INTERVAL;
	logflag  = DEFAULT_LOGFLAG;

	/* getopt se llama desde un bucle. Cuando getopt devuelve
	-1, indica no hay mas opciones presentes. El bucle acaba */
	while ((ch = getopt(argc, argv, "lp:")) != -1) {
		switch (ch) {
			case 'l':
				logflag = 1;
				break;
			case 'p':
				interval = atoi(optarg);
				break;
		}
	}

	pid = fork();

	/*
	First the fork() system call will be used to create a
	copy ofour process(child), then let parent exit.
	Orphaned child will become a child of init process (this
	is the initial system process, in other words the parent
	of all 	processes). As a result our process will be
	completely detached from its parent and start operating in
	background.
	*/

	if (pid < 0) {
		exit(EXIT_FAILURE);
	} else if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	umask(0);

	/*
	Most servers runs as super-user, for security reasons they
	should protect files that they create. Setting user mask
	will pre vent unsecure file priviliges that may occur on file
	creation.
	umask(027);
	This will restrict file creation mode to 750 (complement of 027).
	*/

	/*
	obtain a new process group; setsid detaches and puts the
	daemon into a new session:
	*/
	sid = setsid();

	/*
	A process receives signals from the terminal that it is connected
	to, and each process inherits its parent's 		controlling tty.
	A server should not receive signals from the process that started
	it, so it must detach itself from its controlling tty. In Unix
	systems, processes operates within a process group, so that all
	processes within a group is treated as a single entity. Process
	group or session is also inherited. A server should operate
	independently from other processes. This call will place the
	server in a new process group and session and detach its controlling
	terminal.
	(setpgrp() is an alternative for this)
	*/

	if (sid < 0) {
		exit(EXIT_FAILURE);
	}

	if ((chdir("/")) < 0) {
		exit(EXIT_FAILURE);
	}

	/*
	A server should run in a known directory. There are many advantages,
	in fact the opposite has many disadvantages: suppose that our server
	is started in a user's home directory, it will not be able to find
	some input and output files.
	chdir("/servers/");
	The root "/" directory may not be appropriate for every server, it
	should be choosen carefully depending on the 	type of the server.
	*/

	if (logflag == 1) {
		syslog (LOG_NOTICE, " started by User %d", getuid ());
	}

	//while (1) {
	sleep(interval);

	//conecto con el socket
	struct sockaddr_in addr;
	addr = createSocket();
	listener(addr);

	// Is it a SUBSCRIBE?
	if((memcmp(msgbuf, "SUBSCRIBE", 9))==0) {
		struct info_phone info;		// mac , IP ...
		info = infoPhone(msgbuf);
		sender(info);
	}

	/* set up destination address */
	memset(&addr,0,sizeof(addr));
	// inicializamos la estructura
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=htonl(INADDR_ANY);		// N.B.: differs from sender 
	addr.sin_port=htons(HELLO_PORT);			//Port

	if ((fd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0) {
		perror("socket sever");
		exit(1);
	}

	exit(EXIT_SUCCESS);
}

