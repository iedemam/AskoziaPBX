#ifndef PK_DBG_H
#define PK_DBG_H

#include "dbgt.h"

#ifdef _WIN32
#ifdef HSP_UM_DEBUG
#define PK_PRINT printf
#elif defined(UM_HSP)
#define PK_PRINT printf
#else
#define PK_PRINT DbgPrint
#endif
#else
#define PK_PRINT printk
#endif

/* Must be int for Linux module_param macro */
extern int debug;

/* Some handy macros */

#define dbg (debug & 0xf)
#define dbg_mask_level ((debug >> 4) & 0xf)

#define DBGT_SET(l) (debug & (1 << (l)))

#define PK_DBG(x) if(dbg) PK_PRINT x

#ifdef __linux__
#include <linux/time.h>

/* Diff end - start and put result in end. Assumes end >= start! */
static inline time_t diff_time(struct timeval *start, struct timeval *end)
{
	if (start->tv_usec > end->tv_usec) {
		end->tv_sec = end->tv_sec - 1 - start->tv_sec;
		end->tv_usec = end->tv_usec + 1000000 - start->tv_usec;
	} else {
		end->tv_sec -= start->tv_sec;
		end->tv_usec -= start->tv_usec;
	}
	return end->tv_usec;
}
#endif

#endif
