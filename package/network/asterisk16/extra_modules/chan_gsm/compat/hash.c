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

/*! \file hash.c
 *
 * \brief Some default hash functions.
 *
 * \author Nadi Sarrar <nadi@beronet.com>
 *
 * These are the default functions used by hash.h.
 */


#include "asterisk/hash.h"

#include <string.h>

int modolo_hash (int key, size_t size)
{
	return key % size;
}

/* Derived from http://en.wikipedia.org/wiki/Hash_table
 * Original creator: Bob Jenkins */
int joaat_hash (char *key, size_t size)
{
	int hash = 0;
	int len = strlen(key);
	size_t i;

	for (i = 0; i < len; ++i) {
		hash += key[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	hash %= size;

	if (hash < 0)
		hash *= -1;

	return hash;
}

int eq_str (char *a, char *b)
{
	return !strcasecmp(a, b);
}

int eq_int (int a, int b)
{
	return a == b;
}
