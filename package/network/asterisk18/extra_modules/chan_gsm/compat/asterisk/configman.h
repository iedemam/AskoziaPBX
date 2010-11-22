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

/*! \file configman.h
 * \brief Configuration Management
 * \author Nadi Sarrar <nadi@beronet.com>
 */

#ifndef _ASTERISK_CONFIGMAN_H
#define _ASTERISK_CONFIGMAN_H

#include <stdlib.h>
#include <sys/types.h>

#include "asterisk/strings.h" 

typedef struct cm    cm_t;

typedef struct {
	char           * name;
	char           * def;
	char           * desc;
} cm_dir_t;

enum ktype {
	KTYPE_NONE = 0,
	KTYPE_INTEGER,
	KTYPE_STRING,
};

typedef struct {
	char           * section_name;      /* Name of the section. */
	char           * default_name;      /* If you want this section to collect config categories with
										   any name and have a special category for default values,
										   name it here. */
	char           * key_field;         /* If you specify a key field, then this will be the key to
										   access values, specify the key_type below. */
	enum ktype       key_type;          /* The type of the key field. */
    int              num_directives;    /* Number of the directives array elements. */
	const cm_dir_t * directives;        /* The first directive targets the string between the '[ ]'.
										   If key_type == KEY_NONE, let the first one be empty, i.e.
										   { 0, 0, 0 } or { } or { 0 } or whatever you like most. */
} cm_section_t;

/*! \brief Create a new config manager object
 * \param modname Name to identify the owner, used in log messages and for registering cli commands like
 *                {modname} show config [...]
 * \param sections Array of sections, must be static or allocated until after cm_destroy
 * \param num Number of elemenets in sections
 * Creates a cm_t structure from given cm_section_t's.
 *
 * Returns NULL on error, or a cm_t data structure on success.
 */
cm_t *   cm_create (const char *modname, const cm_section_t *sections, int num);

/*! \brief Load and parse a configuration file
 * \param cm_t which cm_t to use
 * \param filename path of file to open. If no preceding '/' character, path is considered relative to AST_CONFIG_DIR
 * Fills the cm_t structure with data from a given configuration file.
 *
 * Returns non-zero on error.
 */
int      cm_load (cm_t *cm, const char *filename);

/*! \brief Destroy a cm_t
 * \param cm_t which cm_t to destroy
 * Destroys and frees a given cm_t data structure.
 */
void     cm_destroy (cm_t *cm);

/*! \brief Get the subsequent id
 * \param cm_t which cm_t to use
 * \param sec_id section id to use
 * \param prev pointer to the previous id, if you want to retrieve the first id, give NULL
 * \param next output pointer to the id after prev
 * Gets the subsequent id of a given id.
 *
 * Returns non-zero if next points to the id after prev, or zero if there
 * is no subsequent id.
 * Example for KTYPE_INT:
 *   int next, *prev = NULL;
 *   for (; cm_get_next_id(mycm, mysec_id, prev, &next); prev = &next) {
 *     .. <next> is your traversing id ..
 *   }
 */
int      cm_get_next_id (cm_t *cm, int sec_id, void *prev, void *next);

/*! \brief Get the subsequent id and rewinds if the end was reached
 * \param cm_t which cm_t to use
 * \param sec_id section id to use
 * \param prev pointer to the previous id, if you want to retrieve the first id, give NULL
 * \param next output pointer to the id after prev
 * Gets the subsequent id of a given id. This variant succeeds and returns the first
 * id if the last one was given as prev.
 *
 * Returns non-zero if next points to the id after prev, or zero if there
 * is no subsequent id.
 */
int      cm_get_next_id_spin (cm_t *cm, int sec_id, void *prev, void *next);

/*! \brief Validate an id
 * \param cm_t which cm_t to use
 * \param sec_id section id to use
 * \param ... integer or string id
 * Validates a given id.
 *
 * Returns true on success, zero if either id or sec_id is not valid.
 */
int      cm_id_valid (cm_t *cm, int sec_id, ...);

/*! \brief Get a configuration value
 * \param cm_t which cm_t to use
 * \param buf where to put the result
 * \param size size of buf
 * \param sec_id section id to use
 * \param elem_id element id to use
 * \param ... integer or string id. omit this parameter, if the underlying cm_section_t is a single section type
 * Reads a value from the cm_t data structure and copies it into buf.
 *
 * Returns non-zero on error.
 */
int      cm_get (cm_t *cm, char *buf, size_t size, int sec_id, int elem_id, ...);

#define  cm_get_int(cm,dest,sec_id,elem_id,...)	\
({	\
	char __buf[32];	\
	int __retval = 0, __tmp;	\
	if (cm_get((cm), __buf, sizeof(__buf), (sec_id), (elem_id), ##__VA_ARGS__) ||	\
		sscanf(__buf, "%d", &__tmp) != 1)	\
		__retval = -1;	\
	else 	\
		dest = __tmp;	\
	__retval;	\
})

#define  cm_get_boolint(cm,dest,val_true,val_false,sec_id,elem_id,...)	\
({	\
	char __buf[32];	\
	int __retval = 0;	\
	if (cm_get((cm), __buf, sizeof(__buf), (sec_id), (elem_id), ##__VA_ARGS__))	\
		__retval = -1;	\
	else 	\
		dest = ast_true(__buf) ? val_true : val_false; \
	__retval;	\
})

#define  cm_get_bool(cm,dest,sec_id,elem_id,...)	\
	cm_get_boolint(cm, dest, 1, 0, sec_id, elem_id, ##__VA_ARGS__)

#define  cm_get_strcasecmp(cm,str,sec_id,elem_id,...)	\
({	\
	char __buf[128];	\
	int __retval = -1;	\
	if (!cm_get((cm), __buf, sizeof(__buf), (sec_id), (elem_id), ##__VA_ARGS__))	\
		__retval = strcasecmp((str), __buf);	\
	__retval;	\
})

#endif
