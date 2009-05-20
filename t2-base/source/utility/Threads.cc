/*
 * --- GSMP-COPYRIGHT-NOTE-BEGIN ---
 * 
 * This copyright note is auto-generated by ./scripts/Create-CopyPatch.
 * Please add additional copyright information _after_ the line containing
 * the GSMP-COPYRIGHT-NOTE-END tag. Otherwise it might get removed by
 * the ./scripts/Create-CopyPatch script. Do not edit this copyright text!
 * 
 * GSMP: utility/src/Threads.cc
 * General Sound Manipulation Program is Copyright (C) 2000 - 2004
 *   Valentin Ziegler and Ren� Rebe
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2. A copy of the GNU General
 * Public License can be found in the file LICENSE.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANT-
 * ABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 * 
 * --- GSMP-COPYRIGHT-NOTE-END ---
 */

#include <unistd.h>

#include <signal.h>
#include <sched.h>        // Linux/POSIX scheduler / realtime scheduling ...
#include <sys/resource.h> // set_priority

#include "Threads.hh"

#include "Logging.hh"

using namespace Utility::Threads;

MutexAttr Mutex::Default = {0};
CondAttr Condition::Default = {0};
ThreadAttr Thread::Default = {0};

void Semaphore::Up()
{
  access.Lock();
  ++ value;
  access.Unlock();
  sig.Signal();
}

void Semaphore::Down()
{
  access.Lock();
  while (value < 1)
    sig.Wait (access);
  -- value;
  access.Unlock();
}

Thread::Thread (const ThreadAttr &i_attr)
  : attr(i_attr)
{
}

Thread::~Thread ()
{
}

int Thread::Create (void* arg)
{
  arg_ = arg; // copy the arg for the static trampoline

  Thread *t = this;
  return pthread_create (&thread, attr.impl, call_main_static_, t);
}

int Thread::Detach ()
{
  return pthread_detach (thread);
}

void* Thread::Join ()
{
  void* thread_return;
  if (pthread_join (thread, &thread_return) == 0)
    return thread_return;
  else
    return 0;
}

void Thread::StopInDebugger ()
{
  pid_t m_pid = getpid ();
  kill (m_pid, SIGTRAP);
}

bool Thread::EnableRealtimeScheduling ()
{
  int t_pri = sched_get_priority_min (SCHED_FIFO);
  Q_LOG (UtilityLog) << "priority_min: " << t_pri << std::endl;
  
  if (t_pri >= 0) {
    sched_param t_params;
    t_params.sched_priority = t_pri;
    
    int error = sched_setscheduler (0, SCHED_FIFO, &t_params);
    
    if (error < 0) {
      Q_WARN (UtilityLog) << "WARNING POSIX realtime-scheduling "
			  << "not available (not root?)!" << std::endl;
    }
    return error >= 0;
  }
  Q_WARN (UtilityLog) << "WARNING minimal priority is positive!"
		      << std::endl;
  return false;
}

void Thread::USleep (int delay, bool high_precission)
{
  /* From the nanosleep man-page:
   * 
   * If  the process is scheduled under a real-time policy like
   * SCHED_FIFO or SCHED_RR, then pauses of up to 2 ms will be
   * performed as busy waits with microsecond precision.
   */
  
  if (!high_precission || delay > 2000)
    usleep (delay);
  else
    sched_yield ();
}

void Thread::NSleep (int delay, bool high_precission)
{
  /* From the nanosleep man-page:
   * 
   * If  the process is scheduled under a real-time policy like
   * SCHED_FIFO or SCHED_RR, then pauses of up to 2 ms will be
   * performed as busy waits with microsecond precision.
   */
  
  if (!high_precission || delay > 2000000)
    usleep (delay);
  else
    sched_yield ();
}

void Thread::Yield ()
{
  sched_yield ();
}

bool Thread::SetPriority (int priority)
{
  return setpriority (PRIO_PROCESS, 0, priority) >= 0;
}

void* Thread::call_main_ (void* arg)
{
  void* return_;
  return_ = main (arg);
  pthread_exit (return_);
}

void* Thread::call_main_static_ (void* obj)
{
  Thread *thread_ = (Thread*)obj;
  return thread_->call_main_ (thread_->arg_);
}