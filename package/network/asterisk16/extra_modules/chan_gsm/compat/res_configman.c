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

/*! \file res_configman.c
 *
 * \brief Configuration Management
 *
 * \author Nadi Sarrar <nadi@beronet.com>
 *
 * Configuration Management written to be used by
 * Asterisk channel modules, but may fit fine for
 * other cases too.
 */

/* TODO:
 *  - handle return values for *alloc, strdup, ast_cli_register
 */





#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


#include "asterisk/configman.h"
#include "asterisk/config.h"
#include "asterisk/cli.h"
#include "asterisk/hash.h"
#include "asterisk/logger.h"
#include "asterisk/lock.h"
#include "asterisk/module.h"
#include "asterisk/strings.h"
#include "asterisk/term.h"
#include "asterisk/utils.h"

static inline char * notdir (char *str)
{
	char *p = strrchr(str, '/');
	return (p && *(p + 1)) ? p + 1 : str;
}

#define WARNING_PARSE_NV(cm, name, value, section) \
	ast_log(LOG_WARNING, "%s [%s]: invalid expression in section \"%s\": \"%s = %s\". " \
			"Please edit your %s and then reload.\n", \
			(cm)->modname, notdir((cm)->filename), (section), (name), (value), notdir((cm)->filename))

#define WARNING_PARSE_SEC(cm, section) \
	ast_log(LOG_WARNING, "%s [%s]: invalid section \"%s\". " \
			"Please edit your %s and then reload.\n", \
			(cm)->modname, notdir((cm)->filename), (section), notdir((cm)->filename))

#define WARNING(fmt,...) \
	ast_log(LOG_WARNING, "confman: " fmt, ##__VA_ARGS__)
#define WARNING_CM(cm,fmt,...) \
	ast_log(LOG_WARNING, "%s: " fmt, (cm)->modname, ##__VA_ARGS__)
#define WARNING_CMF(cm,fmt,...) \
	ast_log(LOG_WARNING, "%s [%s]: " fmt, (cm)->modname, (cm)->filename, ##__VA_ARGS__)

#define ERROR(fmt,...) \
	ast_log(LOG_ERROR, "confman: " fmt, ##__VA_ARGS__)
#define ERROR_CM(cm,fmt,...) \
	ast_log(LOG_ERROR, "%s: " fmt, (cm)->modname, ##__VA_ARGS__)
#define ERROR_CMF(cm,fmt,...) \
	ast_log(LOG_ERROR, "%s [%s]: " fmt, (cm)->modname, (cm)->filename, ##__VA_ARGS__)

#define LOCK(cm)         ast_mutex_lock(&((cm)->lock))
#define UNLOCK(cm)       ast_mutex_unlock(&((cm)->lock))

#define HASHSIZE         512

static char              SHOW[] = "show";
static char              CONFIG[] = "config";
static char              DESCRIPTION[] = "description";
static char              DESCRIPTIONS[] = "descriptions";
static char              VALUES[] = "values";

typedef union {
	AST_HASH_INT_NOLOCK(, int) i;
	AST_HASH_STR_NOLOCK(, int) c;
} cm_hash_t;

typedef union {
	int    i;
	char * c;
} cm_key_t;

static AST_HASH_STR(cm_obj_hash_t, cm_t *) cm_obj_hash;

enum cm_state {
	CM_NULL = 0,
	CM_CREATED,
	CM_LOADED,
};

typedef char           * cm_val_t;
typedef cm_val_t       * cm_row_t;
typedef struct {
	int                  num_rows;
	cm_row_t           * rows;
} cm_matrix_t;

struct cm {
	ast_mutex_t          lock;
	enum cm_state        state;
	char               * modname;
	char               * filename;
	unsigned int         num_secs;
	const cm_section_t * secs;
	cm_matrix_t        * vals;
	cm_hash_t         ** hashes;
	cm_key_t          ** key_arrays;
	cm_cli_entry_t clis[3];
};

static int __get_col (const cm_section_t *sec, char *name)
{
	int i;

	if (!strcasecmp(sec->section_name, name))
		return 0;

	for (i = 0; i < sec->num_directives; ++i)
		if (sec->directives[i].name &&
			!strcasecmp(sec->directives[i].name, name))
			return i;

	return -1;
}

int cm_id_valid (cm_t *cm, int sec_id, ...)
{
	va_list           ap;
	int               id_int;
	char            * id_string;
	int               val;

	if (!cm)
		goto err1;

	LOCK(cm);

	if (cm->state != CM_LOADED || sec_id >= cm->num_secs || sec_id < 0)
		goto err2;

	switch (cm->secs[sec_id].key_type) {
	case KTYPE_NONE:
		goto err2;
	case KTYPE_INTEGER:
		va_start(ap, sec_id);
		id_int = va_arg(ap, int);
		va_end(ap);
		if (!cm->hashes[sec_id])
			goto err2;
		if (AST_HASH_LOOKUP_NOLOCK(&cm->hashes[sec_id]->i, id_int, val))
			goto err2;
		break;
	case KTYPE_STRING:
		va_start(ap, sec_id);
		id_string = va_arg(ap, char *);
		va_end(ap);
		if (!cm->hashes[sec_id])
			goto err2;
		if (AST_HASH_LOOKUP_NOLOCK(&cm->hashes[sec_id]->c, id_string, val))
			goto err2;
		break;
	}

	UNLOCK(cm);

	return 1;

err2:
	UNLOCK(cm);
err1:
	return 0;
}

int cm_get (cm_t *cm, char *buf, size_t size, int sec_id, int elem_id, ...)
{
	va_list           ap;
	int               id_int,
					  row = 0;
	char            * val,
		            * id_string;

	if (!cm)
		goto err1;

	LOCK(cm);

	if (cm->state != CM_LOADED)
		goto err2;

	if (!buf || !size || sec_id >= cm->num_secs || sec_id < 0 ||
		elem_id < 0 || elem_id >= cm->secs[sec_id].num_directives)
		goto err2;

	switch (cm->secs[sec_id].key_type) {
	case KTYPE_NONE:
		val = cm->vals[sec_id].rows[0][elem_id];
		if (!val || !memccpy(buf, val, 0, size)) {
			*buf = 0;
			goto err2;
		}
		goto out;
	case KTYPE_INTEGER:
		va_start(ap, elem_id);
		id_int = va_arg(ap, int);
		va_end(ap);
		if (!cm->hashes[sec_id])
			goto err2;
		if (AST_HASH_LOOKUP_NOLOCK(&cm->hashes[sec_id]->i, id_int, row))
			goto err2;
		break;
	case KTYPE_STRING:
		va_start(ap, elem_id);
		id_string = va_arg(ap, char *);
		va_end(ap);
		if (!cm->hashes[sec_id])
			goto err2;
		if (AST_HASH_LOOKUP_NOLOCK(&cm->hashes[sec_id]->c, id_string, row))
			goto err2;
		break;
	}
	
	val = cm->vals[sec_id].rows[row][elem_id];
	if (!val)
		val = cm->vals[sec_id].rows[0][elem_id];
	if (!val || !memccpy(buf, val, 0, size)) {
		*buf = 0;
		goto err2;
	}

out:
	UNLOCK(cm);

	return 0;

err2:
	UNLOCK(cm);
err1:
	return -1;
}

static void __read_row (cm_t *cm, const cm_section_t *sec, cm_row_t *row_p, char *cat, struct ast_variable *v)
{
	cm_row_t  row;
	int       pos;

	if (!*row_p)
		*row_p = calloc(sec->num_directives, sizeof(cm_val_t));
	row = *row_p;

	if (!row[0])
		row[0] = strdup(cat);
	for (; v; v = v->next) {
		pos = __get_col(sec, v->name);
		if (pos >= 0) {
			if (row[pos]) {
				row[pos] = realloc(row[pos], strlen(row[pos]) + strlen(v->value) + 2);
				sprintf(row[pos] + strlen(row[pos]), ",%s", v->value);
			} else
				row[pos] = strdup(v->value);
		} else
			WARNING_PARSE_NV(cm, v->name, v->value, cat);
	}
}

static void __read_category_1 (cm_t *cm, const cm_section_t *sec, cm_matrix_t *matrix, char *cat, struct ast_variable *v)
{
	if (!matrix->num_rows) {
		matrix->num_rows = 1;
		matrix->rows = calloc(1, sizeof(cm_row_t));
	}
	__read_row(cm, sec, &matrix->rows[0], cat, v);
}

static void __read_category_n (cm_t *cm, const cm_section_t *sec, cm_matrix_t *matrix, char *cat, struct ast_variable *v)
{
	int row;

	for (row = 1; row < matrix->num_rows; ++row) {
		if (!strcasecmp(matrix->rows[row][0], cat)) {
			__read_row(cm, sec, &matrix->rows[row], cat, v);
			return;
		}
	}

	if (!matrix->num_rows) {
		matrix->num_rows = 2;
		matrix->rows = calloc(2, sizeof(cm_row_t));
	}
	else {
		++matrix->num_rows;
		matrix->rows = realloc(matrix->rows, matrix->num_rows * sizeof(cm_row_t));
		matrix->rows[matrix->num_rows - 1] = NULL;
	}
	__read_row(cm, sec, &matrix->rows[matrix->num_rows - 1], cat, v);
}

static void __hash_categories (cm_t *cm, const cm_section_t *sec, cm_matrix_t *matrix, cm_hash_t **hash)
{
	int    pos,
		   start,
		   end,
		   i;
	char * token,
		 * tmp;

	switch (sec->key_type) {
	case KTYPE_INTEGER:
		if (!*hash) {
			*hash = calloc(1, sizeof(cm_hash_t));
			AST_HASH_INIT_INT_NOLOCK(&(*hash)->i, HASHSIZE);
		}
		pos = __get_col(sec, sec->key_field);
		if (pos < 0)
			ERROR_CM(cm, "missing key field \"%s\" in section structure!\n", sec->key_field);
		else
			for (i = 1; i < matrix->num_rows; ++i) {
				if (!matrix->rows[i][pos])
					WARNING_CMF(cm, "missing value for key field \"%s\" in section \"%s\"!\n",
								sec->key_field,
								matrix->rows[i][0] ? matrix->rows[i][0] : sec->section_name);
				else {
					tmp = strdup(matrix->rows[i][pos]);
					for (token = strsep(&tmp, ","); token; token = strsep(&tmp, ",")) { 
						if (!*token)
							continue;
						if (sscanf(token, "%d-%d", &start, &end) >= 2) {
							for (; start <= end; start++) {
								if (AST_HASH_INSERT_NOLOCK(&(*hash)->i, start, i))
									WARNING_CMF(cm, "failed to hash integer key: %d\n", start);
							}
						} else if (sscanf(token, "%d", &start)) {
							if (AST_HASH_INSERT_NOLOCK(&(*hash)->i, start, i))
								WARNING_CMF(cm, "failed to hash integer key: %d\n", start);
						} else
							WARNING_PARSE_NV(cm, sec->directives[pos].name, matrix->rows[i][pos], sec->section_name);
					}
					free(tmp);
				}
			}
		break;
	case KTYPE_STRING:
		if (!*hash) {
			*hash = calloc(1, sizeof(cm_hash_t));
			AST_HASH_INIT_STR_NOLOCK(&(*hash)->c, HASHSIZE);
		}
		pos = __get_col(sec, sec->key_field);
		if (pos < 0)
			ERROR_CM(cm, "missing key field \"%s\" in section structure!\n", sec->key_field);
		else
			for (i = 1; i < matrix->num_rows; ++i) {
				if (!matrix->rows[i][pos])
					WARNING_CMF(cm, "missing value for key field \"%s\" in section \"%s\"!\n",
								sec->key_field,
								matrix->rows[i][0] ? matrix->rows[i][0] : sec->section_name);
				else {
					if (AST_HASH_INSERT_NOLOCK(&(*hash)->c, matrix->rows[i][pos], i))
						WARNING_CMF(cm, "failed to hash string key: %s\n", matrix->rows[i][pos]);
				}
			}
		break;
	case KTYPE_NONE:
		break;
	}
}

static void __fill_defaults (cm_t *cm, const cm_section_t *sec, cm_matrix_t *matrix)
{
	int i;

	if (!matrix->num_rows) {
		matrix->num_rows = 1;
		matrix->rows = calloc(1,  sizeof(cm_row_t));
	}

	if (!matrix->rows[0])
		matrix->rows[0] = calloc(sec->num_directives, sizeof(cm_val_t));

	if (!matrix->rows[0][0])
		matrix->rows[0][0] = sec->default_name ? strdup(sec->default_name) : strdup(sec->section_name);
	
	for (i = 1; i < sec->num_directives; ++i)
		if (!matrix->rows[0][i] && sec->directives[i].def)
			matrix->rows[0][i] = strdup(sec->directives[i].def);
}

static inline void __keys_insert_sorted_int (cm_key_t *keys, int key)
{
	int i,
		a,
		b;

	for (i = 0; keys[i].i != -1 && keys[i].i <= key; ++i);

	b = keys[i].i;
	keys[i].i = key;

	for (++i; b != -1; ++i) {
		a = keys[i].i;
		keys[i].i = b;
		b = a;
	}
}

static inline void __keys_insert_sorted_string (cm_key_t *keys, char *key)
{
	int    i;
	char * a,
		 * b;

	for (i = 0; keys[i].c && strcasecmp(keys[i].c, key) <= 0; ++i);

	b = keys[i].c;
	keys[i].c = key;

	for (++i; b; ++i) {
		a = keys[i].c;
		keys[i].c = b;
		b = a;
	}
}

static void __init_keys (cm_t *cm, cm_hash_t *hash, cm_key_t **keys, enum ktype type)
{
	int    i,
		   id_int,
		   val;
	char * id_string;

	if (type == KTYPE_INTEGER) {
		*keys = calloc(AST_HASH_LENGTH_NOLOCK(&hash->i), sizeof(cm_key_t));
		for (i = 0; i < AST_HASH_LENGTH_NOLOCK(&hash->i); ++i)
			(*keys)[i].i = -1;
		AST_HASH_TRAVERSE_NOLOCK(&hash->i, id_int, val, i)
			__keys_insert_sorted_int(*keys, id_int);
	}

	if (type == KTYPE_STRING) {
		*keys = calloc(AST_HASH_LENGTH_NOLOCK(&hash->c), sizeof(cm_key_t));
		AST_HASH_TRAVERSE_NOLOCK(&hash->c, id_string, val, i)
			__keys_insert_sorted_string(*keys, id_string);
	}
}

int cm_get_next_id (cm_t *cm, int sec_id, void *prev, void *next)
{
	int             i,
					retval = 0;
	cm_key_t      * keys;
	cm_hash_t     * hash;

	if (!cm)
		goto err1;

	LOCK(cm);

	if (cm->state != CM_LOADED || sec_id >= cm->num_secs || sec_id < 0 ||
		cm->secs[sec_id].key_type == KTYPE_NONE)
		goto err2;

	hash = cm->hashes[sec_id];
	keys = cm->key_arrays[sec_id];
	
	if (!AST_HASH_LENGTH_NOLOCK(&hash->i))
		goto err2;

	switch (cm->secs[sec_id].key_type) {
	case KTYPE_INTEGER:
		if (!prev || *(int *)prev < 0) {
			*(int *)next = keys[0].i;
		} else {
			for (i = 0; i < AST_HASH_LENGTH_NOLOCK(&hash->i) && keys[i].i != *(int *)prev; ++i);
			if (i >= (AST_HASH_LENGTH_NOLOCK(&hash->i) - 1))
				goto err2;
			*(int *)next = keys[i + 1].i;
		}
		retval = 1;
		break;
	case KTYPE_STRING:
		if (!prev || !*(char **)prev) {
			*(char **)next = keys[0].c;
		} else {
			for (i = 0; i < AST_HASH_LENGTH_NOLOCK(&hash->c) && strcasecmp(keys[i].c, *(char **)prev); ++i);
			if (i >= (AST_HASH_LENGTH_NOLOCK(&hash->c) - 1))
				goto err2;
			*(char **)next = keys[i + 1].c;
		}
		retval = 1;
		break;
	case KTYPE_NONE:
		goto err2;
	}

err2:
	UNLOCK(cm);
err1:
	return retval;
}

int cm_get_next_id_spin (cm_t *cm, int sec_id, void *prev, void *next)
{
	return cm_get_next_id(cm, sec_id, prev, next) || cm_get_next_id(cm, sec_id, NULL, next);
}

/* cli commands for modules */
static void __show_config_description (int fd, const cm_section_t *sec, int col)
{
	char name[128];
	char section[128];

	term_color(name, sec->directives[col].name, COLOR_BRWHITE, 0, sizeof(name));
	term_color(section, sec->section_name, COLOR_YELLOW, 0, sizeof(section));

	if (sec->directives[col].def && *sec->directives[col].def)
		ast_cli(fd, "[%s] %s   (Default: %s)\n\t%s\n",
				section,
				name,
				sec->directives[col].def,
				sec->directives[col].desc);
	else
		ast_cli(fd, "[%s] %s\n\t%s\n",
				section,
				name,
				sec->directives[col].desc);
}

static char * __complete_section (cm_t *cm, const char *word, int state)
{
	int which = 0,
		wordlen = strlen(word),
		i;

	for (i = 0; i < cm->num_secs; ++i) {
		if (!wordlen || !strncmp(word, cm->secs[i].section_name, wordlen)) {
			if (++which > state) {
				return strdup(cm->secs[i].section_name);
			}
		}
	}

	return NULL;
}

static char * __complete_directive (cm_t *cm, const char *word, int state)
{
	int which = 0,
		wordlen = strlen(word),
		i,
		j;

	for (i = 0; i < cm->num_secs; ++i) {
		for (j = 1; j < cm->secs[i].num_directives; ++j) {
			if (!wordlen || !strncmp(word, cm->secs[i].directives[j].name, wordlen)) {
				if (++which > state) {
					return strdup(cm->secs[i].directives[j].name);
				}
			}
		}
	}

	return NULL;
}

static char * __complete_keys (cm_t *cm, const cm_section_t *sec, const char *word, int state)
{
	cm_key_t * keys;
	int        which = 0,
			   wordlen = strlen(word),
			   len,
			   i;
	char       buf[32];

	for (i = 0; i < cm->num_secs; ++i)
		if (&cm->secs[i] == sec)
			break;
	if (i == cm->num_secs)
		return NULL;

	len = AST_HASH_LENGTH_NOLOCK(&cm->hashes[i]->i);
	keys = cm->key_arrays[i];

	if (sec->key_type == KTYPE_STRING) {
		for (i = 0; i < len; ++i) {
			if (!wordlen || !strncmp(word, keys[i].c, wordlen)) {
				if (++which > state) {
					return strdup(keys[i].c);
				}
			}
		}
	} else if (sec->key_type == KTYPE_INTEGER) {
		if (!wordlen) {
			if (state >= len)
				return NULL;
			snprintf(buf, sizeof(buf), "%d", keys[state].i);
			return strdup(buf);
		}
		for (i = 0; i < len; ++i) {
			snprintf(buf, sizeof(buf), "%d", keys[i].i);
			if (!strncmp(word, buf, wordlen)) {
				if (++which > state) {
					return strdup(buf);
				}
			}
		}
	}

	return NULL;
}

static inline cm_t * __get_cm_from_hash (char *modname)
{
	cm_t * cm = NULL;
	AST_HASH_LOOKUP(&cm_obj_hash, modname, cm);
	return cm;
}

static cm_t * __get_cm_from_line (const char *line)
{
	char modname[128];

	if (sscanf(line, "%128s", modname) == 1)
		return __get_cm_from_hash(modname);
	else
		return NULL;
}

static const cm_section_t * __get_sec_from_line (cm_t *cm, const char *line)
{
	char section_name[128],
		 buf1[128],
		 buf2[128];
	int  i;

	if (sscanf(line, "%127s show config %128s %128s", buf1, buf2, section_name) != 3)
		return NULL;

	for (i = 0; i < cm->num_secs; ++i)
		if (!strcasecmp(cm->secs[i].section_name, section_name))
			return &cm->secs[i];

	return NULL;
}

static char * cli_show_config_description_completer (const char *line, const char *word, int pos, int state)
{
	cm_t * cm;
	char * retval = NULL;

	if (pos != 4)
		return NULL;

	cm = __get_cm_from_line(line);
	if (!cm)
		return NULL;

	LOCK(cm);
	retval = __complete_directive(cm, word, state);
	UNLOCK(cm);

	return retval;
}

static char* cli_show_config_description (struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	cm_t * cm;
	int i, col;
	char *err = CLI_SHOWUSAGE;
	const int buf_size = 256;
	char *buf;

	switch (cmd) {
		case CLI_INIT:
			if (buf = (char*)malloc(buf_size)) {
				snprintf(buf, (buf_size - 1), "%s show config description", e->cmda[0]);
				e->command = buf;
			}
		return NULL;

		case CLI_GENERATE:
			return cli_show_config_description_completer(a->line, a->word, a->pos, a->n);
		return NULL;
	}


	if (a->argc != 5)
		return err;

	cm = __get_cm_from_hash(a->argv[0]);
	if (!cm)
		return err;

	LOCK(cm);
	for (i = 0; i < cm->num_secs; ++i) {
		col = __get_col(&cm->secs[i], a->argv[4]);
		if (col > 0) {
			__show_config_description(a->fd, &cm->secs[i], col);
			err = CLI_SUCCESS;
		}
	}
	UNLOCK(cm);

	if (err != CLI_SUCCESS)
		ast_cli(a->fd, "No such directive: %s\n", a->argv[4]);

	return err;
}

static char * cli_show_config_descriptions_completer (const char *line, const char *word, int pos, int state)
{
	cm_t * cm;
	char * retval = NULL;

	if (pos != 4)
		return NULL;

	cm = __get_cm_from_line(line);
	if (!cm)
		return NULL;

	LOCK(cm);
	retval = __complete_section(cm, word, state);
	UNLOCK(cm);

	return retval;
}

static char* cli_show_config_descriptions (struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	cm_t            * cm;
	int				  i,
					  j;
	char *err = CLI_SHOWUSAGE;
	const int buf_size = 256;
	char *buf;

	switch (cmd) {
		case CLI_INIT:
			if (buf = (char*)malloc(buf_size)) {
				snprintf(buf, (buf_size - 1), "%s show config descriptions", e->cmda[0]);
				e->command = buf;
			}
		return NULL;

		case CLI_GENERATE:
			return cli_show_config_descriptions_completer(a->line, a->word, a->pos, a->n);
		return NULL;
	}

	if (a->argc - 4 && a->argc - 5)
		return err;

	cm = __get_cm_from_hash(a->argv[0]);
	if (!cm)
		return err;

	LOCK(cm);
	for (i = 0; i < cm->num_secs; ++i) {
		if (a->argc == 4 || (a->argc == 5 && !strcasecmp(cm->secs[i].section_name, a->argv[4]))) {
			for (j = 1; j < cm->secs[i].num_directives; ++j)
				__show_config_description(a->fd, &cm->secs[i], j);
			err = CLI_SUCCESS;
			if (a->argc == 5)
				break;
		}
	}
	UNLOCK(cm);

	if ((err != CLI_SUCCESS) && a->argc == 5)
		ast_cli(a->fd, "No such section: %s\n", a->argv[4]);

	return err;
}

static char * cli_show_config_values_completer (const char *line, const char *word, int pos, int state)
{
	cm_t               * cm;
	const cm_section_t * sec;
	char               * retval = NULL;

	cm = __get_cm_from_line(line);
	if (!cm)
		return NULL;

	if (pos == 4) {
		LOCK(cm);
		retval = __complete_section(cm, word, state);
		UNLOCK(cm);
	} else if (pos == 5) {
		LOCK(cm);
		sec = __get_sec_from_line(cm, line);
		if (sec && sec->key_type != KTYPE_NONE)
			retval = __complete_keys(cm, sec, word, state);
		UNLOCK(cm);
	}

	return retval;
}

static inline void __show_config_values_header (int fd, char *name)
{
	char yname[128];

	term_color(yname, name, COLOR_YELLOW, 0, sizeof(yname));
	ast_cli(fd, "[%s]\n", yname);
}

static void __show_config_values (int fd, cm_t *cm, int sec_id, int row)
{
	char   wval[128],
		 * val;
	int    i,
		   col;

	col = cm->secs[sec_id].key_type == KTYPE_NONE ? 0 : 
		__get_col(&cm->secs[sec_id], cm->secs[sec_id].key_field);

	for (i = 0; i < cm->secs[sec_id].num_directives; ++i) {
		if (i == col)
			continue;
		val = cm->vals[sec_id].rows[row][i];
		if (!val)
			val = cm->vals[sec_id].rows[0][i];
		if (!val)
			val = "";
		term_color(wval, val, COLOR_BRWHITE, 0, sizeof(wval));
		ast_cli(fd, " -> %s: %s\n", cm->secs[sec_id].directives[i].name, wval);
	}
}

static void __show_config_values_list (int fd, cm_t *cm, int sec_id)
{
	cm_key_t * keys;
	int        len,
			   row = 0,
			   i;
	char       id[128];

	switch (cm->secs[sec_id].key_type) {
	case KTYPE_NONE:
		__show_config_values_header(fd, cm->secs[sec_id].section_name);
		__show_config_values(fd, cm, sec_id, 0);
		break;
	case KTYPE_INTEGER:
		len = AST_HASH_LENGTH_NOLOCK(&cm->hashes[sec_id]->i);
		keys = cm->key_arrays[sec_id];
		for (i = 0; i < len; ++i) {
			if (!AST_HASH_LOOKUP_NOLOCK(&cm->hashes[sec_id]->i, keys[i].i, row)) {
				snprintf(id, sizeof(id), "%d", keys[i].i);
				__show_config_values_header(fd, id);
				__show_config_values(fd, cm, sec_id, row);
			}
		}
		break;
	case KTYPE_STRING:
		len = AST_HASH_LENGTH_NOLOCK(&cm->hashes[sec_id]->c);
		keys = cm->key_arrays[sec_id];
		for (i = 0; i < len; ++i) {
			if (!AST_HASH_LOOKUP_NOLOCK(&cm->hashes[sec_id]->c, keys[i].c, row)) {
				__show_config_values_header(fd, keys[i].c);
				__show_config_values(fd, cm, sec_id, row);
			}
		}
		break;
	}
}

static char* cli_show_config_values (struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	cm_t * cm;
	int   i,
		   sec_id = -1,
		   id_int,
		   row = 0;
	const int buf_size = 256;
	char *buf;
	

	char *err = CLI_SHOWUSAGE;

	switch (cmd) {
		case CLI_INIT:
			if (buf = (char*)malloc(buf_size)) {
				snprintf(buf, (buf_size - 1), "%s show config values", e->cmda[0]);
				e->command = buf;
			}
		return NULL;

		case CLI_GENERATE:
			return cli_show_config_values_completer(a->line, a->word, a->pos, a->n);
		return NULL;
	}

	if (a->argc < 4 || a->argc > 6)
		return err;

	cm = __get_cm_from_hash(a->argv[0]);
	if (!cm)
		return err;

	LOCK(cm);

	if (a->argc > 4) {
		for (i = 0; i < cm->num_secs; ++i)
			if (!strcmp(a->argv[4], cm->secs[i].section_name)) {
				sec_id = i;
				break;
			}
		if (sec_id < 0) {
			ast_cli(a->fd, "No such section: %s\n", a->argv[4]);
			goto out;
		}
	}

	if (a->argc > 5) {
		switch (cm->secs[sec_id].key_type) {
		case KTYPE_NONE:
			ast_cli(a->fd, "Section \"%s\" has no key field!\n", a->argv[4]);
			goto out;
		case KTYPE_INTEGER:
			if (sscanf(a->argv[5], "%d", &id_int) != 1 ||
				AST_HASH_LOOKUP_NOLOCK(&cm->hashes[sec_id]->i, id_int, row)) {
				ast_cli(a->fd, "Unknown ID: %s\n", a->argv[5]);
				goto out;
			}
			break;
		case KTYPE_STRING:
			if (AST_HASH_LOOKUP_NOLOCK(&cm->hashes[sec_id]->c, a->argv[5], row)) {
				ast_cli(a->fd, "Unknown ID: %s\n", a->argv[5]);
				goto out;
			}
			break;
		}
		__show_config_values_header(a->fd, a->argv[5]);
		__show_config_values(a->fd, cm, sec_id, row);
	}

	if (a->argc == 5)
		__show_config_values_list(a->fd, cm, sec_id);

	if (a->argc == 4)
		for (i = 0; i < cm->num_secs; ++i)
			__show_config_values_list(a->fd, cm, i);

	err = CLI_SUCCESS;

out:
	UNLOCK(cm);

	return err;
}

int cm_load (cm_t *cm, const char *filename)
{
	struct ast_config    * cfg;
	struct ast_variable  * v;
	char                 * cat = NULL,
						   buf[128];
	int                    freesec = -1,
						   i;
	const cm_section_t   * sec;

	if (!cm) {
		WARNING("cm_load called with NULL parameter!\n");
		goto err1;
	}

	LOCK(cm);

	if (cm->state != CM_CREATED) {
		WARNING("cm_load called with %s cm!\n",
				cm->state == CM_LOADED ? "already loaded" : "non-created");
		goto err2;
	}

	cm->filename = strdup(filename);
	if (!cm->filename) {
		ERROR_CM(cm, "not enough memory!\n");
		goto err2;
	}

#ifdef ASTERISK_TRUNK
	struct ast_flags config_flags = { 0 } ;
	cfg = ast_config_load(filename, config_flags);
#else
	cfg = ast_config_load(filename);
#endif
	if (!cfg) {
		ERROR_CM(cm, "failed to load file: %s\n", filename);
		goto err3;
	}

	for (i = 0; i < cm->num_secs; ++i) {
		if (cm->secs[i].key_type != KTYPE_NONE) {
			freesec = i;
			break;
		}
	}

	while ((cat = ast_category_browse(cfg, cat))) {
		v = ast_variable_browse(cfg, cat);
		for (i = 0; i < cm->num_secs; ++i) {
			sec = &cm->secs[i];
			if ((sec->default_name && !strcasecmp(sec->default_name, cat)) ||
				(!sec->default_name && !strcasecmp(sec->section_name, cat))) {
				__read_category_1(cm, sec, &cm->vals[i], cat, v);
				break;
			}
		}
		if (i == cm->num_secs) {
			if (freesec > -1)
				__read_category_n(cm, &cm->secs[freesec], &cm->vals[freesec], cat, v);
			else
				WARNING_PARSE_SEC(cm, cat);
		}
	}

	ast_config_destroy(cfg);

	for (i = 0; i < cm->num_secs; ++i) {
		__fill_defaults(cm, &cm->secs[i], &cm->vals[i]);
		if (cm->secs[i].key_type == KTYPE_INTEGER || cm->secs[i].key_type == KTYPE_STRING) {
			__hash_categories(cm, &cm->secs[i], &cm->vals[i], &cm->hashes[i]);
			__init_keys(cm, cm->hashes[i], &cm->key_arrays[i], cm->secs[i].key_type);
		}
	}

	cm->clis[2].cmda[0] = cm->modname;
	cm->clis[2].cmda[1] = SHOW;
	cm->clis[2].cmda[2] = CONFIG;
	cm->clis[2].cmda[3] = VALUES;
	snprintf(buf, sizeof(buf), "Print the configuration values read from file");
	cm->clis[2].summary = strdup(buf);
	snprintf(buf, sizeof(buf), "Usage: %s show config values [<section name> [<key>]]\n", cm->modname);
	cm->clis[2].usage = strdup(buf);
	cm->clis[2].handler = cli_show_config_values;
	ast_cli_register((struct ast_cli_entry *)&cm->clis[2]);

	cm->state = CM_LOADED;

	UNLOCK(cm);

	return 0;

err3:
	free(cm->filename);
err2:
	UNLOCK(cm);
err1:
	return -1;
}

static inline void __destroy_row (cm_row_t row, size_t cols)
{
	int i;

	for (i = 0; i < cols; ++i)
		if (row[i])
			free(row[i]);
	
	free(row);
}

static inline void __destroy_matrix (cm_matrix_t *matrix, size_t cols)
{
	int i;

	for (i = 0; i < matrix->num_rows; ++i)
		if (matrix->rows[i])
			__destroy_row(matrix->rows[i], cols);
}

void cm_destroy (cm_t *cm)
{
	int i;

	if (!cm) {
		WARNING("cm_destroy called with NULL parameter!\n");
		return;
	}

	if (cm->state != CM_LOADED && cm->state != CM_CREATED) {
		WARNING("cm_destroy called with non-initialized cm!\n");
		return;
	}

	LOCK(cm);

	ast_cli_unregister_multiple((struct ast_cli_entry *)cm->clis, cm->state == CM_LOADED ? 3 : 2);
	for (i = 0; i < (cm->state == CM_LOADED ? 3 : 2); ++i) {
		free(cm->clis[i].summary);
		free(cm->clis[i].usage);
		free(cm->clis[i].command);
	}

	AST_HASH_REMOVE(&cm_obj_hash, cm->modname);

	if (cm->state == CM_LOADED) {
		free(cm->filename);
		for (i = 0; i < cm->num_secs; ++i) {
			__destroy_matrix(&cm->vals[i], cm->secs[i].num_directives);
			if (cm->hashes[i]) {
				AST_HASH_DESTROY_NOLOCK(&cm->hashes[i]->i);
				free(cm->hashes[i]);
			}
			if (cm->key_arrays[i])
				free(cm->key_arrays[i]);
		}
	}

	free(cm->key_arrays);
	free(cm->hashes);
	free(cm->vals);
	free(cm->modname);

	ast_mutex_destroy(&cm->lock);

	free(cm);

	ast_module_unref(ast_module_info->self);
}

cm_t * cm_create (const char *modname, const cm_section_t *sections, int num)
{
	cm_t * cm;
	char   buf[128];

	cm = calloc(1, sizeof(cm_t));
	if (!cm)
		goto err1;

	ast_mutex_init(&cm->lock);

	cm->modname = strdup(modname);
	if (!cm->modname)
		goto err2;

	cm->num_secs = num;
	cm->secs = sections;

	if (AST_HASH_INSERT(&cm_obj_hash, cm->modname, cm))
		goto err3;

	cm->vals = calloc(cm->num_secs, sizeof(cm_matrix_t));
	if (!cm->vals)
		goto err4;

	cm->hashes = calloc(cm->num_secs, sizeof(cm_hash_t *));
	if (!cm->hashes)
		goto err5;

	cm->key_arrays = calloc(cm->num_secs, sizeof(cm_hash_t *));
	if (!cm->key_arrays)
		goto err6;

	cm->clis[0].cmda[0] = cm->modname;
	cm->clis[0].cmda[1] = SHOW;
	cm->clis[0].cmda[2] = CONFIG;
	cm->clis[0].cmda[3] = DESCRIPTION;
	snprintf(buf, sizeof(buf), "Display description for the given configuration directive");
	cm->clis[0].summary = strdup(buf);
	snprintf(buf, sizeof(buf), "Usage: %s show config description <directive>\n", cm->modname);
	cm->clis[0].usage = strdup(buf);
	cm->clis[0].handler = cli_show_config_description;
//	cm->clis[0].generator = (generator_t)cli_show_config_description_completer;
	ast_cli_register((struct ast_cli_entry *)&cm->clis[0]);

	cm->clis[1].cmda[0] = cm->modname;
	cm->clis[1].cmda[1] = SHOW;
	cm->clis[1].cmda[2] = CONFIG;
	cm->clis[1].cmda[3] = DESCRIPTIONS;
	snprintf(buf, sizeof(buf), "List descriptions for all configuration directives");
	cm->clis[1].summary = strdup(buf);
	snprintf(buf, sizeof(buf), "Usage: %s show config descriptions [<section name>]\n", cm->modname);
	cm->clis[1].usage = strdup(buf);
	cm->clis[1].handler = cli_show_config_descriptions;
//	cm->clis[1].generator = (generator_t)cli_show_config_descriptions_completer;
	ast_cli_register((struct ast_cli_entry *)&cm->clis[1]);

	ast_module_ref(ast_module_info->self);
	cm->state = CM_CREATED;

	return cm;

err6:
	free(cm->hashes);
err5:
	free(cm->vals);
err4:
	AST_HASH_REMOVE(&cm_obj_hash, cm->modname);
err3:
	free(cm->modname);
err2:
	ast_mutex_destroy(&cm->lock);
	free(cm);
err1:
	return NULL;
}

/* configman cli commands */
static char* configman_list_modules(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	cm_t * cm;
	char * key;
	int    i;

	switch (cmd) {
	case CLI_INIT:
		e->command = "configman list modules";
		e->usage = "Usage: configman list modules\n";
		return NULL;

	case CLI_GENERATE:
		return NULL;
	}


	AST_HASH_LOCK(&cm_obj_hash);

	ast_cli(a->fd, "Modules using configman: %d\n", AST_HASH_LENGTH_NOLOCK(&cm_obj_hash));
	AST_HASH_TRAVERSE_NOLOCK(&cm_obj_hash, key, cm, i)
		ast_cli(a->fd, " >> %s\n", key);

	AST_HASH_UNLOCK(&cm_obj_hash);

	return CLI_SUCCESS;
}

static struct ast_cli_entry configman_clis[] = {
	AST_CLI_DEFINE(configman_list_modules, "List modules using configman")
};

__static__ int load_module (void)
{
	AST_HASH_INIT_STR(&cm_obj_hash, HASHSIZE);
	ast_cli_register_multiple((struct ast_cli_entry *)configman_clis, sizeof(configman_clis) / sizeof(struct ast_cli_entry));

	return 0;
}

__static__ int unload_module (void)
{
	ast_cli_unregister_multiple((struct ast_cli_entry *)configman_clis, sizeof(configman_clis) / sizeof(struct ast_cli_entry));
	AST_HASH_DESTROY(&cm_obj_hash);

	return 0;
}

__static__ int reload (void)
{
	return -1;
}

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_GLOBAL_SYMBOLS, "Configuration Management Resource",
				.load = load_module,
				.unload = unload_module,
				.reload = reload,
			   );
