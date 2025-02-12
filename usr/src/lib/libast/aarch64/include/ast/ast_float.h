/***********************************************************************
*                                                                      *
*               This software is part of the ast package               *
*          Copyright (c) 1985-2010 AT&T Intellectual Property          *
*                      and is licensed under the                       *
*                  Common Public License, Version 1.0                  *
*                    by AT&T Intellectual Property                     *
*                                                                      *
*                A copy of the License is available at                 *
*            http://www.opensource.org/licenses/cpl1.0.txt             *
*         (with md5 checksum 059e8cd6165cb4c31e351f2b69388fd9)         *
*                                                                      *
*              Information and Software Systems Research               *
*                            AT&T Research                             *
*                           Florham Park NJ                            *
*                                                                      *
*                 Glenn Fowler <gsf@research.att.com>                  *
*                  David Korn <dgk@research.att.com>                   *
*                   Phong Vo <kpv@research.att.com>                    *
*                                                                      *
***********************************************************************/

/* : : generated by proto : : */
/* : : generated from /home/gisburn/ksh93/ast_ksh_20100309/build_i386_64bit/src/lib/libast/features/float by iffe version 2009-12-04 : : */
                  
#ifndef _def_float_ast
#if !defined(__PROTO__)
#  if defined(__STDC__) || defined(__cplusplus) || defined(_proto) || defined(c_plusplus)
#    if defined(__cplusplus)
#      define __LINKAGE__	"C"
#    else
#      define __LINKAGE__
#    endif
#    define __STDARG__
#    define __PROTO__(x)	x
#    define __OTORP__(x)
#    define __PARAM__(n,o)	n
#    if !defined(__STDC__) && !defined(__cplusplus)
#      if !defined(c_plusplus)
#      	define const
#      endif
#      define signed
#      define void		int
#      define volatile
#      define __V_		char
#    else
#      define __V_		void
#    endif
#  else
#    define __PROTO__(x)	()
#    define __OTORP__(x)	x
#    define __PARAM__(n,o)	o
#    define __LINKAGE__
#    define __V_		char
#    define const
#    define signed
#    define void		int
#    define volatile
#  endif
#  define __MANGLE__	__LINKAGE__
#  if defined(__cplusplus) || defined(c_plusplus)
#    define __VARARG__	...
#  else
#    define __VARARG__
#  endif
#  if defined(__STDARG__)
#    define __VA_START__(p,a)	va_start(p,a)
#  else
#    define __VA_START__(p,a)	va_start(p)
#  endif
#  if !defined(__INLINE__)
#    if defined(__cplusplus)
#      define __INLINE__	extern __MANGLE__ inline
#    else
#      if defined(_WIN32) && !defined(__GNUC__)
#      	define __INLINE__	__inline
#      endif
#    endif
#  endif
#endif
#if !defined(__LINKAGE__)
#define __LINKAGE__		/* 2004-08-11 transition */
#endif

#define _def_float_ast	1
#define _sys_types	1	/* #include <sys/types.h> ok */
#define _hdr_float	1	/* #include <float.h> ok */
#define _hdr_limits	1	/* #include <limits.h> ok */
#define _hdr_math	1	/* #include <math.h> ok */
#define _hdr_values	1	/* #include <values.h> ok */
#define _LIB_m	1	/* -lm is a library */
#define _lib_fpclassify	1	/* fpclassify() in default lib(s) */
#define _lib_frexp	1	/* frexp() in default lib(s) */
#define _lib_frexpl	1	/* frexpl() in default lib(s) */
#define _lib_ldexp	1	/* ldexp() in default lib(s) */
#define _lib_ldexpl	1	/* ldexpl() in default lib(s) */
#define _lib_finite	1	/* finite() in default lib(s) */
#define _lib_isinf	1	/* isinf() in default lib(s) */
#define _lib_isnan	1	/* isnan() in default lib(s) */
#define _lib_isnanl	1	/* isnanl() in default lib(s) */
#define _lib_signbit	1	/* signbit() in default lib(s) */
#define _lib_copysign	1	/* copysign() in default lib(s) */
#define _lib_copysignl	1	/* copysignl() in default lib(s) */
#include <ast_common.h>
#include <float.h>
#include <math.h>
#ifndef FLT_DIG
#define FLT_DIG		 6
#endif
#ifndef FLT_MAX
#define FLT_MAX		 3.4028234663852885981170E+38F
#endif
#ifndef FLT_MAX_10_EXP
#define FLT_MAX_10_EXP	 ( + 38 )
#endif
#ifndef FLT_MAX_EXP
#define FLT_MAX_EXP	 ( + 128 )
#endif
#ifndef FLT_MIN
#define FLT_MIN		 1.1754943508222875079688E-38F
#endif
#ifndef FLT_MIN_10_EXP
#define FLT_MIN_10_EXP	 ( - 37 )
#endif
#ifndef FLT_MIN_EXP
#define FLT_MIN_EXP	 ( - 125 )
#endif
#ifndef DBL_DIG
#define DBL_DIG		 15
#endif
#ifndef DBL_MAX
#define DBL_MAX		 1.7976931348623157081452E+308
#endif
#ifndef DBL_MAX_10_EXP
#define DBL_MAX_10_EXP	 ( + 308 )
#endif
#ifndef DBL_MAX_EXP
#define DBL_MAX_EXP	 ( + 1024 )
#endif
#ifndef DBL_MIN
#define DBL_MIN		 2.2250738585072013830903E-308
#endif
#ifndef DBL_MIN_10_EXP
#define DBL_MIN_10_EXP	 ( - 307 )
#endif
#ifndef DBL_MIN_EXP
#define DBL_MIN_EXP	 ( - 1021 )
#endif
#ifndef LDBL_DIG
#define LDBL_DIG		 33
#endif
#ifndef LDBL_MAX
#define LDBL_MAX		 1.189731495357231765085759326628007016E+4932L
#endif
#ifndef LDBL_MAX_10_EXP
#define LDBL_MAX_10_EXP	 ( + 4932 )
#endif
#ifndef LDBL_MAX_EXP
#define LDBL_MAX_EXP	 ( + 16384 )
#endif
#ifndef LDBL_MIN
#define LDBL_MIN		 3.362103143112093506262677817321752603E-4932L
#endif
#ifndef LDBL_MIN_10_EXP
#define LDBL_MIN_10_EXP	 ( - 4931 )
#endif
#ifndef LDBL_MIN_EXP
#define LDBL_MIN_EXP	 ( - 16381 )
#endif


#define USHRT_DIG		4
#define UINT_DIG		9
#define ULONG_DIG		19
#define UINTMAX_DIG		ULONG_DIG

#define FLT_ULONG_MAX		18446744073709551615.0F
#define FLT_ULLONG_MAX		FLT_ULONG_MAX
#define FLT_UINTMAX_MAX		FLT_ULONG_MAX
#define FLT_LONG_MAX		9223372036854775807.0F
#define FLT_LLONG_MAX		FLT_LONG_MAX
#define FLT_INTMAX_MAX		FLT_LONG_MAX
#define FLT_LONG_MIN		(-9223372036854775808.0F)
#define FLT_LLONG_MIN		FLT_LONG_MIN
#define FLT_INTMAX_MIN		FLT_LONG_MIN

#define DBL_ULONG_MAX		18446744073709551615.0
#define DBL_ULLONG_MAX		DBL_ULONG_MAX
#define DBL_UINTMAX_MAX		DBL_ULONG_MAX
#define DBL_LONG_MAX		9223372036854775807.0
#define DBL_LLONG_MAX		DBL_LONG_MAX
#define DBL_INTMAX_MAX		DBL_LONG_MAX
#define DBL_LONG_MIN		(-9223372036854775808.0)
#define DBL_LLONG_MIN		DBL_LONG_MIN
#define DBL_INTMAX_MIN		DBL_LONG_MIN

#define LDBL_ULONG_MAX		18446744073709551615.0L
#define LDBL_ULLONG_MAX		LDBL_ULONG_MAX
#define LDBL_UINTMAX_MAX	LDBL_ULONG_MAX
#define LDBL_LONG_MAX		9223372036854775807.0L
#define LDBL_LLONG_MAX		LDBL_LONG_MAX
#define LDBL_INTMAX_MAX		LDBL_LONG_MAX
#define LDBL_LONG_MIN		(-9223372036854775808.0L)
#define LDBL_LLONG_MIN		LDBL_LONG_MIN
#define LDBL_INTMAX_MIN		LDBL_LONG_MIN

#define FLTMAX_UINTMAX_MAX	LDBL_UINTMAX_MAX
#define FLTMAX_INTMAX_MAX	LDBL_INTMAX_MAX
#define FLTMAX_INTMAX_MIN	LDBL_INTMAX_MIN

typedef union _ast_dbl_exp_u
{
	uint32_t		e[sizeof(double)/4];
	double			f;
} _ast_dbl_exp_t;

#define _ast_dbl_exp_index	1
#define _ast_dbl_exp_shift	20

typedef union _fltmax_exp_u
{
	uint32_t		e[sizeof(_ast_fltmax_t)/4];
	_ast_fltmax_t		f;
} _ast_fltmax_exp_t;

#define _ast_fltmax_exp_index	2
#define _ast_fltmax_exp_shift	0

#define _ast_flt_unsigned_max_t	unsigned long long
#define _ast_flt_nan_init	0xff,0xff,0xff,0x7f
#define _ast_flt_inf_init	0x00,0x00,0x80,0x7f
#define _ast_dbl_nan_init	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x7f
#define _ast_dbl_inf_init	0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x7f
#define _ast_ldbl_nan_init	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x7f
#define _ast_ldbl_inf_init	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x7f
#endif
