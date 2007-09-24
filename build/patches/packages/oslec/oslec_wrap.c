/*
  oslec_wrap.c
  David Rowe
  7 Feb 2007

  Wrapper for OSLEC to turn it into a kernel module compatable with Zaptel.

  The /proc/oslec interface points to the first echo canceller
  instance created. Zaptel appears to create/destroy e/c on a call by
  call basis, and with the current echo can function interface it is
  difficult to tell which channel is assigned to which e/c.  So to
  simply the /proc interface (at least in this first implementation)
  we limit it to the first echo canceller created.

  So if you only have one call up on a system, /proc/oslec will refer
  to that.  That should be sufficient for debugging the echo canceller
  algorithm, we can extend it to handle multiple simultaneous channels
  later.
*/

/*
  Copyright (C) 2007 David Rowe
 
  All rights reserved.
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2, as
  published by the Free Software Foundation.
   This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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


/*#include <asm/delay.h>*/

/*#define malloc(a) kmalloc((a), GFP_KERNEL)*/
/*#define free(a) kfree(a)*/

#include "oslec.h"
#include <echo.h>

/* constants for isr cycle averaging */
/* #define LTC   5   /* base 2 log of TC */

/* number of cycles we are using per call */
/*static int cycles_last = 0;*/
/*static int cycles_worst = 0;*/
/*static int cycles_average = 0;*/

/*static __inline__ uint64_t cycles(void) {
  uint64_t x;
  __asm__ volatile ("rdtsc\n\t" : "=A" (x));
  return x;
}*/

/* vars to help us with /proc interface */
/*static echo_can_state_t *mon_ec;*/
/*static int num_ec;*/
/*static int len_ec;*/
 
/* We need this lock as multiple threads may try to manipulate
   the globals used for diagnostics at the same time */

/*DECLARE_MUTEX(oslec_lock);*/

MODULE_VERSION(oslec, 1);

MALLOC_DEFINE(M_OSLEC, "oslec", "oslec echo canceller");


/* XXX memset missing in 6.2's libkern, should only be defined for FreeBSD < 7.0 */
#if __FreeBSD_version < 700000
void *memset(void *b, int c, size_t len) {
	char *bb;

	for (bb = (char *)b; len--; )
		*bb++ = c;
	return (b);
}
#endif



static int oslec_initialize(void) {
	/*struct proc_dir_entry *proc_oslec, *proc_mode, *proc_reset;*/

	printf("Open Source Line Echo Canceller Installed\n");

	/*num_ec = 0;*/
	/*mon_ec = NULL;*/

	/*proc_oslec = proc_mkdir("oslec", 0);*/
	/*create_proc_read_entry("oslec/info", 0, NULL, proc_read_info, NULL);*/
	/*proc_mode = create_proc_read_entry("oslec/mode", 0, NULL, proc_read_mode, NULL);*/
	/*proc_reset = create_proc_read_entry("oslec/reset", 0, NULL, NULL, NULL);*/

	/*proc_mode->write_proc = proc_write_mode;*/
	/*proc_reset->write_proc = proc_write_reset;*/

	return 0;
}


static void oslec_cleanup(void) {
	/*remove_proc_entry("oslec/reset", NULL);
	remove_proc_entry("oslec/info", NULL);
	remove_proc_entry("oslec/mode", NULL);
	remove_proc_entry("oslec", NULL);*/
	printf("Open Source Line Echo Canceller Removed\n");
}


/* Thread safety issues:

  Due to the design of zaptel an e/c instance may be created and
  destroyed at any time.  So monitoring and controlling a running e/c
  instance through /proc/oslec has some special challenges:

  1/ oslec_echo_can_create() might be interrupted part way through,
     and called again by another thread.

  2/ proc_* might be interrupted and the e/c instance pointed to
     by mon_ec destroyed by another thread.

  3/ Call 1 might be destroyed while Call 2 is still up, mon_ec will
     then point to a destroyed instance.

  4/ What happens if an e/c is destroyed while we are modifying it's
     mode?

  The goal is to allow monitoring and control of at least once
  instance while maintaining stable multithreaded operation.  This is
  tricky (at least for me) given zaptel design and the use of globals
  here to monitor status.

  Thanks Dmitry for helping point these problems out.....
*/
struct echo_can_state *oslec_echo_can_create(int len, int adaption_mode) {
  struct echo_can_state *ec;

  /*down(&oslec_lock);*/

  ec = (struct echo_can_state *)malloc(sizeof(struct echo_can_state));
  ec->ec = (void*)echo_can_create(len,   ECHO_CAN_USE_ADAPTION 
				       | ECHO_CAN_USE_NLP 
				       | ECHO_CAN_USE_CLIP
				       | ECHO_CAN_USE_TX_HPF
				       | ECHO_CAN_USE_RX_HPF);
				   
  /*num_ec++;*/

  /* We monitor the first e/c created after mon_ec is set to NULL.  If
     no other calls exist this will be the first call.  If a monitored
     call hangs up, we will monitor the next call created, ignoring
     any other current calls.  Not perfect I know, however this is
     just meant to be a development tool.  Stability is more important
     than comprehensive monitoring abilities.
  */
  /*if (mon_ec == NULL) {
      mon_ec = (echo_can_state_t*)(ec->ec);
      len_ec = len;
  }*/

  /*up(&oslec_lock);*/

  return ec;
}


void oslec_echo_can_free(struct echo_can_state *ec) {
  /*down(&oslec_lock);*/

  /* if this is the e/c being monitored, disable monitoring */
  /*if (mon_ec == ec->ec)
    mon_ec = NULL;*/

  echo_can_free((echo_can_state_t*)(ec->ec));
  /*num_ec--;*/
  free(ec);

  /*up(&oslec_lock);*/
}


/* 
   This code in re-entrant, and will run in the context of an ISR.  No
   locking is required for cycles calculation, as only one thread will have
   ec->ec == mon_ec at a given time, so there will only ever be one
   writer to the cycles_* globals. 
*/
short oslec_echo_can_update(struct echo_can_state *ec, short iref, short isig) {
    short clean;
    /*u32   start_cycles = 0;*/

    /*if (ec->ec == mon_ec) {
      start_cycles = cycles();
    }*/

    clean = echo_can_update((echo_can_state_t*)(ec->ec), iref, isig);

    /*
      Simple IIR averager:

                   -LTC           -LTC
      y(n) = (1 - 2    )y(n-1) + 2    x(n)

    */

    /*if (ec->ec == mon_ec) {
      cycles_last = cycles() - start_cycles;
      cycles_average += (cycles_last - cycles_average) >> LTC;
    
      if (cycles_last > cycles_worst)
	cycles_worst = cycles_last;
    }*/

    return clean;
}


int oslec_echo_can_traintap(struct echo_can_state *ec, int pos, short val) {
	return 0;
}


short oslec_hpf_tx(struct echo_can_state *ec, short txlin) {
	if (ec != NULL) 
		return echo_can_hpf_tx((echo_can_state_t*)(ec->ec), txlin);  
	else
		return txlin;
}


static int oslec_modevent(module_t mod, int type, void* data) {

	switch (type) {
		
		case MOD_LOAD:
		oslec_initialize();
		break;

		case MOD_UNLOAD:
		oslec_cleanup();
		break;
	
		case MOD_SHUTDOWN:
		default:
		return EOPNOTSUPP;
	}

	return 0;
}


DEV_MODULE(oslec, oslec_modevent, NULL);
