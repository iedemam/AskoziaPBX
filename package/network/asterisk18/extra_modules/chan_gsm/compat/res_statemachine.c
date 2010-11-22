/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) 2006, Nadi Sarrar
 *
 * Nadi Sarrar <nadi@beronet.com>
 *
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*! \file res_statemachine.c
 *
 * \brief Simple Statemachine
 *
 * \author Nadi Sarrar <nadi@beronet.com>
 */


#include "asterisk/compat-16.h"


#include "asterisk/statemachine.h"
#include "asterisk/module.h"
#include "asterisk/logger.h"
#include "asterisk/lock.h"
#include "asterisk/utils.h"
#include "asterisk/cli.h"
#include "asterisk/term.h"

#include <stdlib.h>
#include <string.h>

#define LOCK(sm)	ast_mutex_lock(&(sm)->lock)
#define UNLOCK(sm)	ast_mutex_unlock(&(sm)->lock)

struct statemachine {
	ast_mutex_t                     lock;
	void                            *p;
	volatile int                    state;
	struct statemachine_transition  *table;
	int                             num_rows;
	int                             (*send_event)(void *p, int event);
	void                            (*log_event)(void *p, int direction, int state, int event);
};

/* State Machine: Generic */
struct statemachine * statemachine_create (void *p,
										   int state,
										   struct statemachine_transition *table,
										   int num_rows,
										   int (*send_event)(void *p, int event),
										   void (*log_event)(void *p, int direction, int state, int event))
{
	struct statemachine *sm = malloc(sizeof(struct statemachine));

	if (sm) {
		ast_module_ref(ast_module_info->self);
		ast_log(LOG_VERBOSE, "Creating statemachine with state %d ...\n", state);
		sm->p = p;
		sm->state = state;
		sm->table = table;
		sm->num_rows = num_rows;
		sm->send_event = send_event;
		sm->log_event = log_event;
		ast_mutex_init(&sm->lock);
	}

	return sm;
}

void statemachine_destroy (struct statemachine *sm)
{
	if (sm) {
		ast_log(LOG_VERBOSE, "Destroying statemachine at state %d ...\n", sm->state);
		ast_mutex_destroy(&sm->lock);
		free(sm);
		ast_module_unref(ast_module_info->self);
	}
}

int statemachine_run (struct statemachine *sm, int event)
{
	struct statemachine_transition *t;
	struct statemachine_handle_retval handle_retval;
	int i = 0,
		event_out = EVENT_DEFAULT,
		err,
		state,
		retval = -1,
		leave = 0;

	LOCK(sm);

	state = sm->state;

	if (sm->log_event)
		sm->log_event(sm->p, LOG_RECEIVE, state, event);

	for (; i < sm->num_rows; ++i) {
		t = &sm->table[i];
		if ((t->state == state || t->state == STATE_ANY) &&
			(t->event == event || t->event == EVENT_ANY)) {
			if (t->handle) {
				handle_retval = t->handle(sm->p, sm->state, event);
				event_out = handle_retval.event == EVENT_DEFAULT ? t->event_out : handle_retval.event;
				if (handle_retval.state != STATE_DEFAULT) {
					if (handle_retval.state != STATE_KEEP)
						sm->state = handle_retval.state;
					retval = 0;
					leave = 1;
				}
			} else
				event_out = t->event_out;
			switch (event_out) {
			case EVENT_BREAK:
				retval = 0;
				goto out;
			case EVENT_NONE:
				break;
			default:
				if (sm->log_event)
					sm->log_event(sm->p, LOG_TRANSMIT, sm->state, event_out);
				err = sm->send_event(sm->p, event_out);
				if (err) {
					retval = err;
					goto out;
				}
			}
			if (leave)
				goto out;
			event_out = EVENT_DEFAULT;
			if (t->state_next == STATE_CONTINUE)
				continue;
			if (t->state_next != STATE_KEEP)
				sm->state = t->state_next;
			retval = 0;
			goto out;
		}
	}

out:
	UNLOCK(sm);

	return retval;
}

int statemachine_get_state (struct statemachine *sm)
{
	return sm->state;
}

void statemachine_set_state (struct statemachine *sm, int state)
{
	LOCK(sm);
	sm->state = state;
	UNLOCK(sm);
}

__static__ int load_module (void)
{
	return 0;
}

__static__ int unload_module (void)
{
	return 0;
}

__static__ int reload (void)
{
	return -1;
}

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_GLOBAL_SYMBOLS, "Statemachine Resource",
				.load = load_module,
				.unload = unload_module,
				.reload = reload,
			   );
