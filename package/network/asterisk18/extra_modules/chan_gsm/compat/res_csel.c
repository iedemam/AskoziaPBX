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

/*! \file res_csel.c
 *
 * \brief Abstract Channel Selection
 *
 * \author Nadi Sarrar <nadi@beronet.com>
 */


#include "asterisk/compat-16.h"


#include "asterisk/csel.h"
#include "asterisk/module.h"
#include "asterisk/logger.h"
#include "asterisk/lock.h"
#include "asterisk/utils.h"
#include "asterisk/cli.h"
#include "asterisk/term.h"

#include <stdlib.h>
#include <string.h>

struct csel {
	ast_mutex_t       lock;
	
	occupy_func       occupy;
	free_func         free;

	struct strategy * m;
	void            * data;
};

struct strategy {
	char            * name;
	char            * desc;
	char            * desc_params;
	int            (* init) (struct csel *cs, const char *params);
	void           (* destroy) (struct csel *cs);
	int            (* add) (struct csel *cs, void *priv);
	void *         (* get_next) (struct csel *cs);
	void           (* call) (struct csel *cs, void *priv, const void *uid);
	void           (* hangup) (struct csel *cs, void *priv, const void *uid);
};

#define LOCK(cs)	ast_mutex_lock(&(cs)->lock)
#define UNLOCK(cs)	ast_mutex_unlock(&(cs)->lock)

/* STANDARD: begin */
struct standard_data {
	int size;
	void **arr;
};

static int standard_init (struct csel *cs, const char *params)
{
	cs->data = calloc(1, sizeof(struct standard_data));
	return cs->data ? 0 : 1;
}

static void standard_destroy (struct csel *cs)
{
	struct standard_data *data = cs->data;
	int i = 0;

	if (data) {
		if (data->arr) {
			if (cs->free)
				for (; i < data->size; ++i)
					cs->free(data->arr[i]);
			free(data->arr);
		}
		free(data);
	}
}

static int standard_add (struct csel *cs, void *priv)
{
	struct standard_data *data = cs->data;

	++data->size;
	data->arr = realloc(data->arr, data->size * sizeof(void *));
	data->arr[data->size - 1] = priv;

	return 0;
}

static void * standard_get_next (struct csel *cs)
{
	struct standard_data *data = cs->data;
	void *p;
	int i = 0;

	for (; i < data->size; ++i) {
		if ((p = cs->occupy(data->arr[i])))
			return p;
	}

	return NULL;
}
/* STANDARD: end */

/* RANDOM: begin */
struct rand_data {
	int size;
	void **arr;
};

static int rand_init (struct csel *cs, const char *params)
{
	cs->data = calloc(1, sizeof(struct rand_data));
	return cs->data ? 0 : 1;
}

static void rand_destroy (struct csel *cs)
{
	struct rand_data *data = cs->data;
	int i = 0;

	if (data) {
		if (data->arr) {
			if (cs->free)
				for (; i < data->size; ++i)
					cs->free(data->arr[i]);
			free(data->arr);
		}
		free(data);
	}
}

static int rand_add (struct csel *cs, void *priv)
{
	struct rand_data *data = cs->data;

	++data->size;
	data->arr = realloc(data->arr, data->size * sizeof(void *));
	data->arr[data->size - 1] = priv;

	return 0;
}

static void * rand_get_next (struct csel *cs)
{
	struct rand_data *data = cs->data;
	void *p;
	int i = data->size,
		pos;

	for (; i > 0 ; --i) {
		pos = random() % i;
		if ((p = cs->occupy(data->arr[pos])))
			return p;
		p = data->arr[i - 1];
		data->arr[i - 1] = data->arr[pos];
		data->arr[pos] = p;
	}

	return NULL;
}
/* RANDOM: end */

/* ROUND ROBIN: begin */
struct rr_data {
	int size;
	int curr;
	void **arr;
};

static int rr_init (struct csel *cs, const char *params)
{
	cs->data = calloc(1, sizeof(struct rr_data));
	return cs->data ? 0 : 1;
}

static void rr_destroy (struct csel *cs)
{
	struct rr_data *data = cs->data;
	int i = 0;

	if (data) {
		if (data->arr) {
			if (cs->free)
				for (; i < data->size; ++i)
					cs->free(data->arr[i]);
			free(data->arr);
		}
		free(data);
	}
}

static int rr_add (struct csel *cs, void *priv)
{
	struct rr_data *data = cs->data;

	++data->size;
	data->arr = realloc(data->arr, data->size * sizeof(void *));
	data->arr[data->size - 1] = priv;

	return 0;
}

static void * rr_get_next (struct csel *cs)
{
	struct rr_data *data = cs->data;
	void *p;
	int begin = (data->curr + 1) % data->size,
		i = begin;

	do {
		if ((p = cs->occupy(data->arr[i]))) {
			data->curr = i;
			return p;
		}
		i = (i + 1) % data->size;
	} while (i != begin);

	return NULL;
}
/* ROUND ROBIN: end */

static struct strategy strategies[] = {
	{ "standard", "Select the first free channel. This is the default.", 0,
		standard_init, standard_destroy, standard_add, standard_get_next, 0, 0 },
	{ "random", "Select a random free channel.", 0,
		rand_init, rand_destroy, rand_add, rand_get_next, 0, 0 },
	{ "round_robin", "Use the round robin algorithm to select a free channel.", 0,
		rr_init, rr_destroy, rr_add, rr_get_next, 0, 0 },
};

struct csel * csel_create (const char *strategy,
						   const char *params,
						   occupy_func occupy,
						   free_func   free)
{
	struct csel *cs;
	int i = 0;

	if (strategy && *strategy) {
		for (; i < (sizeof(strategies) / sizeof(struct strategy)); ++i) {
			if (!strcasecmp(strategies[i].name, strategy))
				break;
		}
		if (i == (sizeof(strategies) / sizeof(struct strategy))) {
			ast_log(LOG_WARNING, "csel: unknown strategy: %s, falling back to: %s\n", strategy, strategies[0].name);
			i = 0;
		}
	}

	cs = calloc(1, sizeof(struct csel));

	cs->occupy = occupy;
	cs->m = &strategies[i];

	if (cs->m->init(cs, params)) {
		free(cs);
		return NULL;
	}
	
	ast_mutex_init(&cs->lock);

	ast_module_ref(ast_module_info->self);

	return cs;
}

void csel_destroy (struct csel *cs)
{
	ast_mutex_destroy(&cs->lock);
	cs->m->destroy(cs);
	free(cs);
	
	ast_module_unref(ast_module_info->self);
}

int csel_add (struct csel *cs,
			  void *priv)
{
	int retval;
	
	LOCK(cs);
	retval = cs->m->add(cs, priv);
	UNLOCK(cs);

	return retval;
}

void * csel_get_next (struct csel *cs)
{
	void *retval;
	
	LOCK(cs);
	retval = cs->m->get_next(cs);
	UNLOCK(cs);

	return retval;
}

void csel_call (struct csel *cs,
				void *priv,
				const char *uid)
{
	LOCK(cs);
	if (cs->m->call)
		cs->m->call(cs, priv, uid);
	UNLOCK(cs);
}

void csel_hangup (struct csel *cs,
				  void *priv,
				  const char *uid)
{
	LOCK(cs);
	if (cs->m->hangup)
		cs->m->hangup(cs, priv, uid);
	UNLOCK(cs);
}

static char* csel_list_strategies (struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	int i = 0;
	char strategy[128],
		 desc[128],
		 params[128];

	switch (cmd) {
	case CLI_INIT:
		e->command = "csel list strategies";
		e->usage = "Usage: csel list strategies\n";
		return NULL;

	case CLI_GENERATE:
		return NULL;
	}


	term_color(strategy, "Strategy", COLOR_YELLOW, 0, sizeof(strategy));
	term_color(desc, "Description", COLOR_BRWHITE, 0, sizeof(desc));
	term_color(params, "Parameters", COLOR_BRWHITE, 0, sizeof(params));

	for (; i < (sizeof(strategies) / sizeof(struct strategy)); ++i)
		ast_cli(a->fd, "%s: %s\n%s: %s\n%s: %s\n%s", strategy, strategies[i].name,
				desc, strategies[i].desc, params, strategies[i].desc_params ? strategies[i].desc_params : "(none)",
				(i + 1) < (sizeof(strategies) / sizeof(struct strategy)) ? "\n" : "");

	return CLI_SUCCESS;
}

static struct ast_cli_entry csel_clis[] = {
	AST_CLI_DEFINE(csel_list_strategies, "List channel selection strategies")
};

__static__ int load_module (void)
{
	ast_cli_register_multiple(csel_clis, sizeof(csel_clis) / sizeof(struct ast_cli_entry));

	return 0;
}

__static__ int unload_module (void)
{
	ast_cli_unregister_multiple(csel_clis, sizeof(csel_clis) / sizeof(struct ast_cli_entry));
	
	return 0;
}

__static__ int reload (void)
{
	return -1;
}

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_GLOBAL_SYMBOLS, "Channel Selection Resource",
				.load = load_module,
				.unload = unload_module,
				.reload = reload,
			   );
