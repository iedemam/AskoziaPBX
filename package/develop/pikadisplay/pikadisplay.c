#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>

typedef struct
{
  unsigned       eventType;
  unsigned       Para[3];
}TLCDEvent;

#define PK_LCD_BITMAP_WIDTH              160
#define PK_LCD_BITMAP_HEIGHT              32

#define DISPLAY_TEXT_SIZE				21

#define LCD_WIDTH (PK_LCD_BITMAP_WIDTH / 8) /* in chars */

#define NUMBER_OF_LCD_MESSAGE_PARAMETERS      3

#define LCD_EVENT_BUTTON                      1
#define LCD_EVENT_BUTTON_UP					  0
#define LCD_EVENT_BUTTON_DOWN				  1

typedef struct TLCDMessage {
	unsigned opcode;
#define LCD_SET_CONFIG                        1
#define LCD_DISPLAY_STRING                    2
#define LCD_DISPLAY_BITMAP                    3
#define LCD_SET_BLINK                         4
#define LCD_UNSET_BLINK                       5
#define LCD_CLEAR_LCD                         6
#define LCD_SET_FONTLIB                       7
#define LCD_GET_STATS                         8
#define LCD_FLUSH_EVENTS                      9
	unsigned Para[NUMBER_OF_LCD_MESSAGE_PARAMETERS];
} TLCDMessage;

typedef struct TLCDRegion {
	unsigned x;
	unsigned y;
	unsigned width;
	unsigned height;
} TLCDRegion;

static struct TLCDConfig {
	unsigned orientation;
	unsigned mode;
	unsigned shiftTime;
	unsigned blinkTime;
	unsigned brightness; /* 0 to 4 */
} config = { 0, 0, 500, 500, 4 };

#include "askozialogo.xbm"
#define BITMAP askozialogo_bits
#define WIDTH askozialogo_width
#define HEIGHT askozialogo_height

char* get_ipv4_addr(char *ifc) {
	int sd=socket(AF_INET,SOCK_STREAM,0);
	struct ifreq ifr;
	strncpy (ifr.ifr_name,ifc,sizeof(ifr.ifr_name));
	struct sockaddr_in *sin = (struct sockaddr_in*) &ifr.ifr_addr;
	ioctl(sd,SIOCGIFNAME,&ifr);
	ioctl(sd,SIOCGIFADDR,&ifr);
	if (!sin->sin_addr.s_addr) return NULL;
	else return inet_ntoa(sin->sin_addr);
}

char* get_ip(void)
{
	return get_ipv4_addr("eth0");
}

char version_string[21];

void init_version(void)
{
	static const char filename[] = "/etc/version";
	FILE *file = fopen ( filename, "r" );
	if ( file != NULL )
	{
		fgets ( version_string, sizeof version_string, file );
		fclose ( file );
	}
}

void set_mode(int fd, int mode)
{
	TLCDMessage msg;
	config.mode = mode;
	msg.opcode = LCD_SET_CONFIG;
	msg.Para[0] = (unsigned)&config;
	write(fd, &msg, sizeof(msg));
}

void clear_display(int fd)
{
	TLCDMessage msg = { .opcode = LCD_CLEAR_LCD };
	write(fd, &msg, sizeof(msg));
}

void lcd_flush(int fd)
{
	TLCDMessage msg = { .opcode = LCD_FLUSH_EVENTS };
	write(fd, &msg, sizeof(msg));
}

void init_bitmap(void)
{
	// convert to big endian
	int n;
	for (n = 0; n < sizeof(BITMAP); ++n) {
		int i;
		unsigned char c = BITMAP[n];
		unsigned char tmp = 0;
		if (c != 0xff) {
			for (i = 0; c; ++i, c >>= 1)
				if (c & 1)
					tmp |= 1 << (7 - i);
			BITMAP[n] = tmp;
		}
	}
}

void test_bitmap(int fd)
{
	TLCDMessage msg;
	TLCDRegion region = { 0, 0, WIDTH, HEIGHT };
	msg.opcode = LCD_DISPLAY_BITMAP;
	msg.Para[0] = (unsigned)&region;
	msg.Para[1] = (unsigned)BITMAP;
	set_mode(fd, 0);
	write(fd, &msg, sizeof(msg));
}

void display_logo(int fd)
{
	clear_display(fd);
	lcd_flush(fd);
	test_bitmap(fd);
}

void display_ip(int fd)
{
	set_mode(fd, 1);
	clear_display(fd);
	lcd_flush(fd);
	
	TLCDMessage msg;
	
	char display_text[21];
	char display_ver[21];
	
	strcpy(display_text,"IP: ");
	strcat(display_text,get_ip());
	
	strcpy(display_ver,"AskoziaPBX ");
	strcat(display_ver,version_string);

	msg.opcode = LCD_DISPLAY_STRING;
	msg.Para[0] = (unsigned)&display_text;
	msg.Para[1] = strlen(&display_text);
	msg.Para[2] = 0;

	write(fd, &msg, sizeof(msg));
	
	msg.opcode = LCD_DISPLAY_STRING;
	msg.Para[0] = (unsigned)&display_ver;
	msg.Para[1] = strlen(&display_ver);
	msg.Para[2] = 1;

	write(fd, &msg, sizeof(msg));
}

void read_button(int fd) {
	TLCDEvent event;
	unsigned upOrDown;
	
	while(1) {
		read(fd, &event, sizeof(TLCDEvent));
		if (event.eventType == LCD_EVENT_BUTTON) {
	        upOrDown = event.Para[0];
			if(upOrDown == LCD_EVENT_BUTTON_DOWN) {
	        	display_ip(fd);
				sleep(30);
				display_logo(fd);
			}
	     }
	}
}

int main(int argc, char *argv[])
{
	int fd = open("/dev/pikalcd", O_RDWR);
	if (fd < 0) {
		perror("/dev/pikalcd");
		exit(1);
	}

	init_bitmap();
	init_version();
	display_logo(fd);
	read_button(fd);
	close(fd);

	return 0;
}
