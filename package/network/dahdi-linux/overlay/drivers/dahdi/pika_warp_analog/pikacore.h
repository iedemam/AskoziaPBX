/*
 * PIKA Core Driver
 *
 * Copyright (c) 2008 PIKA Technologies
 *   Sean MacLennan <smaclennan@pikatech.com>
 */

#ifndef PIKACORE_H
#define PIKACORE_H

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>

/* pikacore_register* returns a negative number on error.
 * On success, it returns the minor number.
 */
int pikacore_register_board(unsigned serial,
			    const struct file_operations *fops);
int pikacore_register(int minor, char *name,
		      const struct file_operations *fops);
int pikacore_unregister(int minor);

/* No good place for these */
int pika_dtm_register_shutdown(void (*func)(void *arg), void *arg);
int pika_dtm_unregister_shutdown(void (*func)(void *arg), void *arg);

/* Some compatibility changes. */

#ifndef DEFINE_MUTEX
#define DEFINE_MUTEX DECLARE_MUTEX
#define mutex_lock down
#define mutex_unlock up
#endif

#include <linux/interrupt.h>
#ifndef IRQF_SHARED
#define IRQF_SHARED (SA_SHIRQ | SA_INTERRUPT)
#define IRQF_DISABLED 0
#endif

#endif
