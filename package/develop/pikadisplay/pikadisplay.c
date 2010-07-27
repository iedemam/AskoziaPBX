#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define PK_LCD_BITMAP_WIDTH              160
#define PK_LCD_BITMAP_HEIGHT              32

#define LCD_WIDTH (PK_LCD_BITMAP_WIDTH / 8) /* in chars */

#define NUMBER_OF_LCD_MESSAGE_PARAMETERS      3

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

void test_bitmap(int fd)
{
	TLCDMessage msg;
	TLCDRegion region = { 0, 0, WIDTH, HEIGHT };
	int n;

	msg.opcode = LCD_DISPLAY_BITMAP;
	msg.Para[0] = (unsigned)&region;
	msg.Para[1] = (unsigned)BITMAP;

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

	set_mode(fd, 0);

	/* non-standard write return */
	n = write(fd, &msg, sizeof(msg));
}

int main(int argc, char *argv[])
{
	int fd = open("/dev/pikalcd", O_RDWR);
	if (fd < 0) {
		perror("/dev/pikalcd");
		exit(1);
	}

	lcd_flush(fd);
	clear_display(fd);
	test_bitmap(fd);

	sleep(10);

	close(fd);

	return 0;
}

/*
 * Local Variables:
 * compile-command: "${CROSS_COMPILE}gcc -O3 -Wall lcdtest.c -o lcdtest"
 * End:
 */
