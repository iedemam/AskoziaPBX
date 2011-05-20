/*___________________________________________________________________________
 |
 |   File: adt_typedef.h
 |
 |   This file contains the Adaptive Digital Technologies, Inc. 
 |   word length assignments based on processor type.
 |   
 |   This software is the property of Adaptive Digital Technologies, Inc.
 |   Unauthorized use is prohibited.
 |
 |   Copyright (c) 2000-2006, Adaptive Digital Technologies, Inc.
 |
 |   www.adaptivedigital.com
 |   610-825-0182
 |
 |______________________________________________________________________________
*/
//.S
#ifndef _ADT_TYPEDEF
#define _ADT_TYPEDEF

#undef I64_AVAIL
#define bytesToWords(a) a
#define byteSize(a)     sizeof(a)

   #ifdef _TMS320C5XX
      typedef short int           ADT_Bool;    //16b
      typedef char                ADT_Int8;    //16b
      typedef unsigned int        ADT_UInt8;   //16b
      typedef short int           ADT_Int16;   //16b
      typedef unsigned short int  ADT_UInt16;  //16b
      typedef long int            ADT_Int32;   //32b
      typedef unsigned long int   ADT_UInt32;  //32b
      typedef long int            ADT_Int40;   //40b
      typedef unsigned long int   ADT_UInt40;  //40b
      #define DSP_TYPE 54
      #define LowBitFirst 0

      #undef bytesToWords
      #define INLINE inline
      #undef byteSize
      #define bytesToWords(a) (((a)+1)/2)
      #define byteSize(a)     (sizeof(a)*2)

   #endif

   #ifdef __TMS320C55XX__
      typedef short int           ADT_Bool;    //16b
      typedef char                ADT_Int8;    //16b
      typedef unsigned char       ADT_UInt8;   //16b
      typedef short int           ADT_Int16;   //16b
      typedef unsigned short int  ADT_UInt16;  //16b
      typedef long int            ADT_Int32;   //32b
      typedef unsigned long int   ADT_UInt32;  //32b
      typedef long long           ADT_Int40;   //40b
      typedef unsigned long long  ADT_UInt40;  //40b
      typedef long long int       ADT_Int64;   //64b
	  typedef unsigned long long  ADT_UInt64;  //unsigned 64 b
      #define I64_AVAIL TRUE

      #define DSP_TYPE 55
      #define LowBitFirst 0

      #undef bytesToWords
      #define INLINE inline
      #undef byteSize
      #define bytesToWords(a) (((a)+1)/2)
      #define byteSize(a)     (sizeof(a)*2)

   #endif

   #ifdef _TMS320C6X

      typedef int                 ADT_Bool;    //32b
      typedef char                ADT_Int8;    // 8b
      typedef unsigned char       ADT_UInt8;   // 8b
      typedef short int           ADT_Int16;   //16b
      typedef unsigned short int  ADT_UInt16;  //16b
      typedef int                 ADT_Int32;   //32b
      typedef unsigned int        ADT_UInt32;  //32b
      typedef long int            ADT_Int40;   //40b
#if (__COMPILER_VERSION__ <= 500)
      typedef double              ADT_Int64;   //64b
#else
	  typedef long long int       ADT_Int64;   //64b
	  typedef unsigned long long int       ADT_UInt64;   //64b
#endif
      typedef short               Shortword;   //16b
      #define I64_AVAIL TRUE
      #define DSP_TYPE 64
      #define INLINE inline
      #define LowBitFirst 1
   #endif
   
   #if defined(WIN32)
      typedef int                 ADT_Bool;    //32b
      typedef char                ADT_Int8;    // 8b
      typedef unsigned char       ADT_UInt8;   // 8b
      typedef short int           ADT_Int16;   //16b
      typedef unsigned short int  ADT_UInt16;  //16b
      typedef long int            ADT_Int32;   //32b
      typedef unsigned long int   ADT_UInt32;  //32b
      typedef __int64             ADT_Int40;   //64b
      typedef __int64             ADT_Int64;    // 64b
      typedef unsigned __int64    ADT_UInt64;    // 64b

      #define I64_AVAIL TRUE
      #define INLINE _inline
      #define LowBitFirst 1
   #endif 

   #if defined (LINUX) || defined (LINUX32) || defined(__arm)  || defined(__i386)
      typedef int                 ADT_Bool;    //32b
      typedef char                ADT_Int8;    // 8b
      typedef unsigned char       ADT_UInt8;   // 8b
      typedef short int           ADT_Int16;   //16b
      typedef unsigned short int  ADT_UInt16;  //16b
      typedef long int            ADT_Int32;   //32b
      typedef unsigned long int   ADT_UInt32;  //32b
      typedef long long int       __int64;     //64b
      typedef long long int       ADT_Int40;   //64b
      typedef long long int       ADT_Int64;   //64b
      typedef unsigned long long int       ADT_UInt64;   //64b

      #define INLINE inline
      #define I64_AVAIL TRUE
      #define LowBitFirst 1
   #endif 

#if 0 // moved to previous #if clause because of issues where LINUX and __arm are both defined
   #ifdef __arm 
      typedef int                 ADT_Bool;    //32b
      typedef char                ADT_Int8;    // 8b
      typedef unsigned char       ADT_UInt8;   // 8b
      typedef short int           ADT_Int16;   //16b
      typedef unsigned short int  ADT_UInt16;  //16b
      typedef long int            ADT_Int32;   //32b
      typedef unsigned long int   ADT_UInt32;  //32b
      typedef long long int       ADT_Int40;   //64b
      typedef long long int       ADT_Int64;    // 64b
      typedef unsigned long long int       ADT_UInt64;   //64b

      #define INLINE inline
      #define I64_AVAIL TRUE
      #define LowBitFirst 1
   #endif
#endif

typedef ADT_Int8   Flag;
   
typedef unsigned int ADT_Word;  // Processor word size (at least 16-bits)

typedef ADT_Int8   Word8;
typedef ADT_Int16  Word16;
typedef ADT_Int32  Word32;

#ifndef _TI_STD_TYPES
typedef ADT_Int8   Int8;
typedef ADT_Int16  Int16;
typedef ADT_Int32  Int32;
#endif 
typedef ADT_UInt8  UInt8;
typedef ADT_UInt16 UInt16;
typedef ADT_UInt32 UInt32;

#ifndef _CSL_STDINC_H_
typedef ADT_Int40  Int40;
#endif

typedef ADT_Int8   ADT_PCM8;
typedef ADT_Int16  ADT_PCM16;

#ifdef I64_AVAIL
   typedef ADT_Int64  Word64;
   typedef ADT_Int64  Int64;
#endif

typedef ADT_UInt32 ADT_TimeMS;


#define todo(a) 
#endif //_ADT_TYPEDEF
//.E
