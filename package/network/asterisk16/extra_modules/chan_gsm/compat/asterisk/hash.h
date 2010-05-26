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

/*! \file hash.h
 *
 * \brief A simple hash implementation.
 *
 * \author Nadi Sarrar <nadi@beronet.com>
 *
 * A set of macros to provide an easy way of working with hashes.
 * Works with either integer or character string keys out of the
 * box. If you want to use another key type, just write a hash
 * function for it and look at macros beginning with "__".
 */


#ifndef ASTERISK_HASH_H
#define ASTERISK_HASH_H

#include <sys/types.h>
#include "asterisk/lock.h"

int modolo_hash	(int key, size_t size);
int joaat_hash (char *key, size_t size);

int eq_str (char *a, char *b);
int eq_int (int a, int b);

#define __AST_HASH(name, ktype, vtype)	\
struct name {							\
	struct {							\
		char        _used;				\
		size_t 		_next;				\
		ktype       _key;				\
		vtype       _val;				\
	} *_data;							\
	size_t          _size;				\
	size_t          _length;			\
	ast_mutex_t     _lock;				\
	int     (*_hash_f)(ktype, size_t);	\
	int     (*_eq_f)  (ktype, ktype);	\
}

#define __AST_HASH_NOLOCK(name, ktype, vtype)	\
struct name {							\
	struct {							\
		char        _used;				\
		size_t      _next;				\
		ktype       _key;				\
		vtype       _val;				\
	} *_data;							\
	size_t          _size;				\
	size_t          _length;			\
	int     (*_hash_f)(ktype, size_t);	\
	int     (*_eq_f)  (ktype, ktype);	\
}

#define AST_HASH_INT(name, type)		\
	__AST_HASH(name, int, type)

#define AST_HASH_INT_NOLOCK(name, type)	\
	__AST_HASH_NOLOCK(name, int, type)

#define AST_HASH_STR(name, type)		\
	__AST_HASH(name, char*, type)

#define AST_HASH_STR_NOLOCK(name, type)	\
	__AST_HASH_NOLOCK(name, char*, type)




#define __AST_HASH_INIT_NOLOCK(hash, size, hash_f, eq_f)	\
do {														\
	(hash)->_data = calloc((size), sizeof(*(hash)->_data));	\
	(hash)->_size = (size);									\
	(hash)->_hash_f = (hash_f);								\
	(hash)->_eq_f = (eq_f);									\
	for ((hash)->_length = (size); (hash)->_length; --(hash)->_length)	\
		(hash)->_data[(hash)->_length - 1]._next = -1;		\
} while (0)

#define __AST_HASH_INIT(hash, size, hash_f, cmp)			\
do {														\
	__AST_HASH_INIT_NOLOCK((hash), (size), (hash_f), (cmp));\
	ast_mutex_init(&(hash)->_lock);							\
} while (0)

#define AST_HASH_INIT_INT_NOLOCK(hash, size)				\
	__AST_HASH_INIT_NOLOCK((hash), (size), modolo_hash, eq_int)

#define AST_HASH_INIT_INT(hash, size)						\
	__AST_HASH_INIT((hash), (size), modolo_hash, eq_int)

#define AST_HASH_INIT_STR_NOLOCK(hash, size)				\
	__AST_HASH_INIT_NOLOCK((hash), (size), joaat_hash, eq_str)

#define AST_HASH_INIT_STR(hash, size)						\
	__AST_HASH_INIT((hash), (size), joaat_hash, eq_str)




#define AST_HASH_DESTROY_NOLOCK(hash)	\
do {									\
	if ((hash)->_data)					\
		free((hash)->_data);			\
} while (0)

#define AST_HASH_DESTROY(hash)			\
do {									\
	AST_HASH_DESTROY_NOLOCK((hash));	\
	ast_mutex_destroy(&(hash)->_lock);	\
} while (0)




#define AST_HASH_LOCK(hash)				\
	ast_mutex_lock(&(hash)->_lock)

#define AST_HASH_TRYLOCK(hash)			\
	ast_mutex_trylock(&(hash)->_lock)

#define AST_HASH_UNLOCK(hash)			\
	ast_mutex_unlock(&(hash)->_lock)




#define __TRY_INSERT(hash, key, val, pos, prev, re)	\
	if (!hash->_data[pos]._used) {					\
		hash->_data[pos]._key = key;				\
		hash->_data[pos]._val = val;				\
		hash->_data[pos]._used = 1;					\
		if (prev > -1)								\
			hash->_data[prev]._next = pos;			\
		++hash->_length;							\
		re = 0;										\
		break;										\
	}

#define AST_HASH_INSERT_NOLOCK(hash, key, val)		\
({													\
	int __pos = (hash)->_hash_f(key, (hash)->_size);\
 	int __end;										\
	int __prev = -1;								\
 	int __re = -1;									\
	do {											\
		__TRY_INSERT((hash), (key), (val), __pos, __prev, __re);\
		if ((hash)->_eq_f((hash)->_data[__pos]._key, (key)))	\
 			break;									\
 		__prev = __pos;								\
		if ((hash)->_data[__pos]._next > -1)		\
			__pos = (hash)->_data[__pos]._next;		\
		else {										\
			__end = __pos;							\
			__pos = (__pos + 1) % (hash)->_size;	\
			do {									\
				__TRY_INSERT((hash), (key), (val), __pos, __prev, __re);	\
				__pos = (__pos + 1) % (hash)->_size;\
			} while (__pos != __end);				\
 			break;									\
		}											\
	} while (1);									\
	__re;											\
})

#define AST_HASH_INSERT(hash, key, val)			\
({												\
 	int _re;									\
	AST_HASH_LOCK((hash));						\
	_re = AST_HASH_INSERT_NOLOCK(hash, key, val);\
	AST_HASH_UNLOCK((hash));					\
	_re;										\
})




#define AST_HASH_LOOKUP_NOLOCK(hash, key, val)	\
({												\
	int _pos = (hash)->_hash_f(key, (hash)->_size);	\
 	int _re = -1;								\
	do {										\
		if ((hash)->_data[_pos]._used &&		\
			(hash)->_eq_f((hash)->_data[_pos]._key, key)) {	\
			val = (hash)->_data[_pos]._val;		\
			_re = 0;							\
 			break;								\
 		}										\
		_pos = (hash)->_data[_pos]._next;		\
	} while (_pos > -1);						\
	_re;										\
})

#define AST_HASH_LOOKUP(hash, key, val)			\
({												\
 	int _re;									\
	AST_HASH_LOCK((hash));						\
	_re = AST_HASH_LOOKUP_NOLOCK(hash, key, val);\
	AST_HASH_UNLOCK((hash));					\
	_re;										\
})




#define AST_HASH_REMOVE_NOLOCK(hash, key)		\
do {											\
	int _pos = (hash)->_hash_f((key), (hash)->_size);	\
	int _prev = -1;								\
	do {										\
		if ((hash)->_data[_pos]._used &&		\
			(hash)->_eq_f((hash)->_data[_pos]._key, (key))) {	\
			(hash)->_data[_pos]._used = 0;		\
			if (_prev > -1)						\
				(hash)->_data[_prev]._next = (hash)->_data[_pos]._next;	\
			--(hash)->_length;					\
 			break;								\
 		}										\
		_prev = _pos;							\
		_pos = (hash)->_data[_pos]._next;		\
	} while (_pos > -1);						\
} while (0)										\

#define AST_HASH_REMOVE(hash, key)				\
do {											\
	AST_HASH_LOCK((hash));						\
	AST_HASH_REMOVE_NOLOCK((hash), (key));		\
	AST_HASH_UNLOCK((hash));					\
} while (0)




#define __AST_HASH_NEXT(hash, i)				\
({												\
	for (++i;									\
		 i < (hash)->_size &&					\
		 !(hash)->_data[i]._used;				\
		 ++i);									\
})

#define AST_HASH_TRAVERSE_NOLOCK(hash, key, val, i)	\
	for (i = -1,									\
		 __AST_HASH_NEXT((hash), i);				\
		 i < (hash)->_size &&						\
		 (((key = (hash)->_data[i]._key)) || 1) &&	\
		 (((val = (hash)->_data[i]._val)) || 1);	\
		 __AST_HASH_NEXT((hash), i))




#define AST_HASH_LENGTH_NOLOCK(hash)			\
	(hash)->_length

#define AST_HASH_LENGTH(hash)					\
({												\
 	size_t length;								\
	AST_HASH_LOCK((hash));						\
 	length = AST_HASH_LENGTH_NOLOCK((hash));	\
 	AST_HASH_UNLOCK((hash));					\
 	length;										\
})




#define AST_HASH_SIZE_NOLOCK(hash)				\
	(hash)->_size

#define AST_HASH_SIZE(hash)						\
({												\
 	size_t size;								\
	AST_HASH_LOCK((hash));						\
 	size = AST_HASH_SIZE_NOLOCK((hash));		\
 	AST_HASH_UNLOCK((hash));					\
 	size;										\
})

#endif
