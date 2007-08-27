/*
 * Dummy Zaptel Driver for Zapata Telephony interface
 *
 * Required:  kernel compiled with "options HZ=1000" 
 * Written by Chris Stenton <jacs@gnome.co.uk>
 * 
 * Copyright (C) 2004, Digium, Inc.
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Rewritten to use the time of day clock (which should be ntp synced
 * for this to work perfectly) by David G. Lawrence <dg@dglawrence.com>.
 * July 27th, 2007.
 *
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/module.h>

#include <sys/conf.h>
#include <sys/errno.h>

#include <zaptel.h>

#include "ztdummy.h"

MODULE_VERSION(ztdummy, 1);

MALLOC_DEFINE(M_ZTD, "ztdummy", "ztdummy interface data structures");

#ifndef timersub
#define timersub(tvp, uvp, vvp)                                         \
        do {                                                            \
                (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;          \
                (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;       \
                if ((vvp)->tv_usec < 0) {                               \
                        (vvp)->tv_sec--;                                \
                        (vvp)->tv_usec += 1000000;                      \
                }                                                       \
        } while (0)
#endif

static struct callout_handle ztdummy_timer_handle = CALLOUT_HANDLE_INITIALIZER(&ztdummy_timer_handle);

static struct ztdummy *ztd;

static int debug = 0;
static struct timeval basetime, curtime, sleeptime;

static __inline void ztdummy_timer(void* arg )
{
	int i, ticks;

loop:
	for (i = 0; i < hz / 100; i++) {
		zt_receive(&ztd->span);
		zt_transmit(&ztd->span);
	}

fixtime:
	microtime(&curtime);

	/*
	 * Sleep until the next 10ms boundry.
	 */
	basetime.tv_usec += 10000;
	if (basetime.tv_usec >= 1000000) {
		basetime.tv_sec++;
		basetime.tv_usec -= 1000000;
	}
	timersub(&basetime, &curtime, &sleeptime);

	/*
	 * Detect if we've gotten behind and need to start our processing
	 * immediately.
	 */
	if (sleeptime.tv_sec < 0 || sleeptime.tv_usec == 0) {
		/*
		 * Limit how far we can get behind to something reasonable (1 sec)
		 * so that we don't go nuts when something (ntp or admin) sets the
		 * clock forward by a large amount.
		 */
		if (sleeptime.tv_sec < -1) {
			basetime.tv_sec = curtime.tv_sec;
			basetime.tv_usec = curtime.tv_usec;
			goto fixtime;
		}
		goto loop;
	}
	/*
	 * Detect if something is messing with the system clock by
	 * checking that the sleep time is no more than 20ms and
	 * resetting our base time if it is. This case will occur if
	 * the system clock has been reset to an earlier time.
	 */
	if (sleeptime.tv_sec > 0 || sleeptime.tv_usec > 20000) {
		basetime.tv_sec = curtime.tv_sec;
		basetime.tv_usec = curtime.tv_usec;
		goto fixtime;
	}

	ticks = sleeptime.tv_usec * hz / 1000000;
	if (ticks == 0)
		goto loop;

	ztdummy_timer_handle = timeout(ztdummy_timer, NULL, ticks);
}

static int ztdummy_initialize(struct ztdummy *ztd)
{
	/* Zapata stuff */
	sprintf(ztd->span.name, "ZTDUMMY/1");
	sprintf(ztd->span.desc, "%s %d", ztd->span.name, 1);
	sprintf(ztd->chan.name, "ZTDUMMY/%d/%d", 1, 0);
	ztd->chan.chanpos = 1;
	ztd->span.chans = &ztd->chan;
	ztd->span.channels = 0;		/* no channels on our span */
	ztd->span.deflaw = ZT_LAW_MULAW;
	ztd->span.pvt = ztd;
	ztd->chan.pvt = ztd;
	if (zt_register(&ztd->span, 0)) {
		return -1;
	}
	return 0;
}

static int ztdummy_attach(void )
{

	ztd = malloc(sizeof(struct ztdummy), M_ZTD, M_NOWAIT);
	if (ztd == NULL) {
		printf("ztdummy: Unable to allocate memory\n");
		return -ENOMEM;
	}

	memset(ztd, 0x0, sizeof(struct ztdummy));

	if (ztdummy_initialize(ztd)) {
		printf("ztdummy: Unable to intialize zaptel driver\n");
		free(ztd, M_ZTD);
		return -ENODEV;
	}

	microtime(&basetime);
	ztdummy_timer_handle = timeout(ztdummy_timer, NULL, 1);
    
	if (debug)
		printf("ztdummy: init() finished\n");
	return 0;
}


static void cleanup_module(void)
{
	untimeout(ztdummy_timer, NULL, ztdummy_timer_handle);
	zt_unregister(&ztd->span);
	free(ztd, M_ZTD);

	if (debug)
		printf("ztdummy: cleanup() finished\n");
}


static int ztdummy_modevent(module_t mod,int  type, void*  data)
{

static int attached = 0;
int  ret;

 switch (type) {
 case MOD_LOAD:
	 if (attached)
		 return EEXIST;
	 
	 ret = ztdummy_attach();
	 if (ret)
		 return (ret);
	 attached = 1;
	 printf("ztdummy: loaded\n");
	 if (hz < 1000) {
		 printf ("ztdummy: WARNING Ticker rate only %d. Timer will not work well!!\nRecompile kernel with \"options HZ=1000\" \n", hz);
	 }
	 
	 break;

 case MOD_UNLOAD:
	 attached = 0;
	 cleanup_module ();
	 
	 printf("ztdummy: unloaded\n");
	 break;
	 
 case MOD_SHUTDOWN:
 default:
	 return EOPNOTSUPP;
 }
 return 0;
}


MODULE_DEPEND(ztdummy, zaptel, 1, 1, 1);
DEV_MODULE(ztdummy, ztdummy_modevent, NULL);
