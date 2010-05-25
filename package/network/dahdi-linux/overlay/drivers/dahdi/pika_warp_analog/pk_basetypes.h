/*DOM-IGNORE-BEGIN*/
/*
 * --------------------------------------------------------------------
 * File:     pk_basetypes.h
 * Revision: $Rev: 14072 $
 * Date:     $Date: 2008-10-09 17:37:39 -0400 (Thu, 09 Oct 2008) $
 *
 * ==================================================================
 * This software, together with any related media and documentation,
 * is subject to the terms and conditions of the PIKA MonteCarlo
 * Software License Agreement in the license.txt file which is
 * included in product installation package.
 * ==================================================================
 */
/*DOM-IGNORE-END*/

#ifndef PK_BASETYPES_H
#define PK_BASETYPES_H

#ifdef _WIN32
 #include <basetsd.h>
#elif __linux__
#include <stddef.h>
#ifdef __KERNEL__
 #include <linux/types.h>
#else
 #include <stdint.h>
#endif
#endif

#ifdef _WIN32
# ifdef _WIN64
#  define PK_API
# else
#  define PK_API __stdcall
# endif
# define PK_CALLBACK __stdcall
# define PK_INLINE _inline
#elif __linux__
# define PK_API
# define PK_CALLBACK
# define PK_INLINE inline
#endif

#ifndef IN
# define IN
#endif

#ifndef OUT
# define OUT
#endif

/*
 * --------------------------------------------------------------------
 * Standard base types
 * --------------------------------------------------------------------
 */

/*
 * Void data types
 */
typedef void PK_VOID;               /* Void data type */
typedef void *PK_PVOID;             /* Void pointer data type */
#define PK_NULL ((void *)0)         /* Null pointer value */

/*
 * Boolean data types
 */
typedef unsigned PK_BOOL;           /* Boolean data type */
typedef unsigned *PK_PBOOL;         /* Boolean pointer data type */
#define PK_TRUE  1                  /* True value for PK_BOOL type */
#define PK_FALSE 0                  /* False value for PK_BOOL type */

/*
 * Character data types
 */
typedef char PK_CHAR;               /* Character data type */
typedef char *PK_PCHAR;             /* Character pointer data type */

/*
 * Real data types
 */
typedef double PK_REAL;             /* Real data type */
typedef double *PK_PREAL;           /* Real pointer data type */

/*
 * Float data types
 */
typedef float PK_FLOAT;             /* Float data type */
typedef float *PK_PFLOAT;           /* Float pointer data type */

/*
 * Double data types
 */
typedef double PK_DOUBLE;           /* Double data type */
typedef double *PK_PDOUBLE;         /* Double pointer data type */

/*
 * Word data types -- the size depends on target OS
 */
typedef unsigned int PK_WORD;       /* <b>depreciated</b> do not use this */
typedef unsigned int *PK_PWORD;     /* <b>depreciated</b> do not use this */

/*
 * Integer data types
 */
typedef unsigned int PK_UINT;       /* Unsigned integer data type */
typedef unsigned int *PK_PUINT;     /* Unsigned integer pointer data type */
typedef signed   int PK_INT;        /* Signed integer data type */
typedef signed   int *PK_PINT;      /* Signed integer pointer data type */

/*
 * Short data types
 */
typedef unsigned short PK_USHORT;   /* Unsigned short data type */
typedef unsigned short *PK_PUSHORT; /* Unsigned short pointer data type */
typedef signed   short PK_SHORT;    /* Signed short data type */
typedef signed   short *PK_PSHORT;  /* Signed short pointer data type */

/*
 * Long data types
 */
typedef unsigned long PK_ULONG;     /* Unsigned long data type */
typedef unsigned long *PK_PULONG;   /* Unsigned long pointer data type */
typedef signed   long PK_LONG;      /* Signed long data type */
typedef signed   long *PK_PLONG;    /* Signed long pointer data type */

/*
 * Integer/Pointer union data type
 */
typedef unsigned long PK_UINTPTR;   /* Unsigned integer or pointer data type */
typedef signed   long PK_INTPTR;    /* Signed integer or pointer data type */

/*
 * --------------------------------------------------------------------
 * Sized base types
 * --------------------------------------------------------------------
 */

/*
 * 8-bit data types
 */
typedef unsigned char PK_U8;        /* Unsigned 8-bit data type */
typedef unsigned char *PK_PU8;      /* Unsigned 8-bit pointer data type */
typedef signed   char PK_I8;        /* Signed 8-bit data type */
typedef signed   char *PK_PI8;      /* Signed 8-bit pointer data type */

/*
 * 16-bit data types
 */
typedef unsigned short PK_U16;      /* Unsigned 16-bit data type */
typedef unsigned short *PK_PU16;    /* Unsigned 16-bit pointer data type */
typedef signed   short PK_I16;      /* Signed 16-bit data type */
typedef signed   short *PK_PI16;     /* Signed 16-bit pointer data type */

/*
 * 32-bit data types
 */
#ifdef _WIN32
 #ifndef _WIN64
  typedef unsigned long PK_U32;     /* Unsigned 32-bit data type */
  typedef unsigned long *PK_PU32;   /* Unsigned 32-bit pointer data type */
  typedef signed   long PK_I32;     /* Signed 32-bit data type */
  typedef signed   long *PK_PI32;   /* Signed 32-bit pointer data type */
 #else
  typedef UINT32  PK_U32;           /* Unsigned 32-bit data type */
  typedef UINT32  *PK_PU32;         /* Unsigned 32-bit pointer data type */
  typedef INT32   PK_I32;           /* Signed 32-bit data type */
  typedef INT32   *PK_PI32;         /* Signed 32-bit pointer data type */
 #endif
#elif __linux__
  typedef uint32_t  PK_U32;         /* Unsigned 32-bit data type */
  typedef uint32_t  *PK_PU32;       /* Unsigned 32-bit pointer data type */
  typedef int32_t   PK_I32;         /* Signed 32-bit data type */
  typedef int32_t   *PK_PI32;       /* Signed 32-bit pointer data type */
#else
 #error Problem determing O/S, cannot set 32 bit data type
#endif

/*
 * 64-bit data types
 */
#ifdef _WIN32
 typedef unsigned __int64 PK_U64;   /* Unsigned 64-bit data type */
 typedef unsigned __int64 *PK_PU64; /* Unsigned 64-bit pointer data type */
 typedef signed   __int64 PK_I64;   /* Signed 64-bit data type */
 typedef signed   __int64 *PK_PI64; /* Signed 64-bit pointer data type */
#elif __linux__
 typedef uint64_t PK_U64;           /* Unsigned 64-bit data type */
 typedef uint64_t *PK_PU64;         /* Unsigned 64-bit pointer data type */
 typedef int64_t  PK_I64;           /* Signed 64-bit data type */
 typedef int64_t  *PK_PI64;         /* Signed 64-bit pointer data type */
#endif

/*
 * --------------------------------------------------------------------
 * Other base types
 * --------------------------------------------------------------------
 */
/*
 * The TPikaHandle type defines the handles used to refer to all user-visible
 * Pika objects in the system. All Pika objects, regardless of their type (e.g.
 * isdn group, media stream, conference), are referred to through an abstract
 * handle value, allowing the application to call functions on the object
 * without needing to manage the memory associated with it.
 */
typedef PK_U32 TPikaHandle;
#define PK_HANDLE_NONE 0 /* Invalid/Null resource handle */
#define PK_NO_HANDLE 0   /* Invalid/Null resource handle (<b>depreciated</b> use PK_HANDLE_NONE instead) */

/*
 * Device identifier types
 */
typedef PK_UINT TPikaDeviceId; /* Device identifier type */
typedef PK_UINT TDeviceId;     /* Device identifier type (<b>depreciated</b> use TPikaDeviceId instead) */

/*
 * 64-bit compliant result of the sizeof operator
 */
#ifdef _WIN32
 typedef SIZE_T  PK_SIZE_T;
#elif __linux__
 typedef size_t  PK_SIZE_T;
#endif

/*
 * Timestamp type (in milliseconds)
 */
typedef unsigned long PK_TIMESTAMP_MS;

/*
 * API function return type
 *
 * Remarks:
 *   Error status codes are defined in pkh_errors.h.
 *
 * See Also:
 *   <link PikaErrors, Error Codes>
 */
typedef int PK_STATUS;

/*
 * Success status code
 */
#define PK_SUCCESS   0

#endif /* PK_BASETYPES_H */
