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

/*! \file statemachine.h
 * \brief Simple Statemachine
 * \author Nadi Sarrar <nadi@beronet.com>
 */

#ifndef _ASTERISK_STATEMACHINE_H
#define _ASTERISK_STATEMACHINE_H

#define STATE_DEFAULT   -1 /*!< return value of handle(): the next state is the default one specified in the transition table */
#define STATE_CONTINUE  -2 /*!< use this in state_next if there exists a following transition */
#define STATE_KEEP      -3 /*!< use this in state_next if you want to keep the current state */
#define STATE_ANY       -4 /*!< wildcard for the state field in struct statemachine_transision */

#define EVENT_DEFAULT   -1 /*!< return value of handle(): statemachine will send the event as read from the transition table */
#define EVENT_BREAK     -2 /*!< return value of handle(): no event will be sent out, statemachine_run returns success */
#define EVENT_NONE      -3 /*!< return value of handle(): no event will be sent out, statemachine_run continues */
#define EVENT_ANY       -4 /*!< wildcard for the event field in struct statemachine_transision */

#define LOG_RECEIVE      0 /*!< passed to log callback when receiving events */
#define LOG_TRANSMIT     1 /*!< passed to log callback when transmitting events */

#define RE_STATE_EVENT(state,event) ({ struct statemachine_handle_retval rv = { (state), (event) }; rv; })
#define RE_DEFAULT                  RE_STATE_EVENT(STATE_DEFAULT, EVENT_DEFAULT)
#define RE_EVENT(event)             RE_STATE_EVENT(STATE_DEFAULT, (event))
#define RE_STATE_DEFEVENT(state)    RE_STATE_EVENT((state), EVENT_DEFAULT)
#define RE_STATE_NOEVENT(state)     RE_STATE_EVENT((state), EVENT_NONE)
#define RE_BREAK                    RE_STATE_EVENT(STATE_DEFAULT, EVENT_BREAK)
#define RE_NONE                     RE_STATE_EVENT(STATE_DEFAULT, EVENT_NONE)

struct statemachine;

struct statemachine_handle_retval {
	int      state;
	int      event;
};

struct statemachine_transition {
	int      state;
	int      event;
	int      state_next;
	int      event_out;
	struct statemachine_handle_retval (*handle)(void *p, int state, int event);
};

struct statemachine * statemachine_create (void *p,
										   int state,
										   struct statemachine_transition *table,
										   int num_rows,
										   int (*send_event)(void *p, int event),
										   void (*log_event)(void *p, int direction, int state, int event));

void statemachine_destroy (struct statemachine *sm);

int statemachine_run (struct statemachine *sm, int event);

int statemachine_get_state (struct statemachine *sm);

void statemachine_set_state (struct statemachine *sm, int state);

#endif
