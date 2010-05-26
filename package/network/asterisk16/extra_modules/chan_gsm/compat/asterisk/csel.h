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

/*! \file csel.h
 * \brief Abstract Channel Selection
 * \author Nadi Sarrar <nadi@beronet.com>
 */

#ifndef _ASTERISK_CSEL_H
#define _ASTERISK_CSEL_H

/* !\brief Entry for Configman: strategy
 */
#define CM_CSEL_STRATEGY \
{ "strategy", "standard",	\
	"Sets the strategy to use for channel selection.\n" \
	"\tEnter 'csel list strategies' to list the possible values." }

/* !\brief Entry for Configman: strategy_parms
 */
#define CM_CSEL_STRATEGY_PARMS \
{ "strategy_parms", "", \
	"Optional parameters passed to the channel selection algorithm.\n" \
	"\tEnter 'csel list strategies' to see which strategies take which\n" \
	"\tparameters." }

/* !\brief Private csel structure
 */
struct csel;

/* !\brief Try to occupy a channel
 * \param priv Private pointer to a structure representing a channel
 * Tries to occupy the channel represented by priv.
 *
 * Returns NULL on error (i.e. channel is busy), or a pointer which
 * will be the return value of csel_get_next.
 */
typedef void * (*occupy_func) (void *priv);

/* !\brief Free priv
 * \param priv Private pointer to a structure representing a channel
 * Cleans up the data pointer to by priv.
 */
typedef void   (*free_func)   (void *priv);


/* !\brief Create a channel selection structure
 * \param strategy Name of the strategy to be used, usually configured in the channel drivers config file
 * \param params Parameters passed to the init function of the strategy
 * \param occupy Function pointer to the channel drivers occupy function
 * \param free Function used by csel_destroy on the added priv's. NULL if you don't want that to happen
 * Allocates and initializes a csel structure.
 *
 * Returns NULL on error.
 */
struct csel * csel_create   (const char *strategy,
							 const char *params,
							 occupy_func occupy,
							 free_func   free);

/* !\brief Destroy a channel selection structure
 * \param cs The channel selection structure to be destroyed
 * Destroys a channel selection structure.
 */
void          csel_destroy  (struct csel *cs);

/* !\brief Add a channel
 * \param cs The channel selection structure
 * \param priv Private pointer to a structure representing a channel
 * Adds a channel to the pool of this cs.
 *
 * Returns non-zero on error.
 */
int           csel_add      (struct csel *cs,
							 void        *priv);

/* !\brief Get the next channel
 * \param cs The channel selection structure
 * Gets the next free channel.
 *
 * Returns NULL if there is no free channel available.
 */
void *        csel_get_next (struct csel *cs);

/* !\brief Register a starting call
 * \param cs The channel selection structure
 * \param priv Private pointer to a structure representing a channel
 * \param uid Unique identifier for that call
 * Registers a starting call, may be useful for some strategies.
 */
void          csel_call     (struct csel *cs,
							 void        *priv,
							 const char  *uid);

/* !\brief Register a hangup
 * \param cs The channel selection structure
 * \param priv Private pointer to a structure representing a channel
 * \param uid Unique identifier for that call
 * Registers a hangup, may be useful for some strategies.
 */
void          csel_hangup   (struct csel *cs,
							 void        *priv,
							 const char  *uid);

#endif
