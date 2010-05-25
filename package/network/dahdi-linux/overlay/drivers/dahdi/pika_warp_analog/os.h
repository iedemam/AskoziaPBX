#ifndef HMPOS_H
#define HMPOS_H

#include <asm/io.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

static inline void delay(int ms)
{
  set_current_state(TASK_INTERRUPTIBLE);
  schedule_timeout(msecs_to_jiffies(ms));
}

#define PK_KSTATUS      s32
#define PK_KERROR       -1
#define PK_KOK          PK_SUCCESS

typedef u32 tAddress;

#endif // HMPOS_H
