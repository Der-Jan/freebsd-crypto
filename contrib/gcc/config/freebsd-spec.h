/*
  %{!ansi:								\
    %{!std=*:-D_LONGLONG}						\
    %{std=gnu*:-D_LONGLONG} }						\
  %{std=c99:-D_LONGLONG}						\

*/
/* Base configuration file for all FreeBSD targets.
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* $FreeBSD$ */

/* Common FreeBSD configuration. 
   All FreeBSD architectures should include this file, which will specify
   their commonalities.
   Adapted from gcc/config/freebsd.h by 
   David O'Brien <obrien@FreeBSD.org>
   Loren J. Rittle <ljrittle@acm.org>.  */


/* In case we need to know.  */
#define USING_CONFIG_FREEBSD_SPEC 1

/* This defines which switch letters take arguments.  On FreeBSD, most of
   the normal cases (defined in gcc.c) apply, and we also have -h* and
   -z* options (for the linker) (coming from SVR4).
   We also have -R (alias --rpath), no -z, --soname (-h), --assert etc.  */

#define FBSD_SWITCH_TAKES_ARG(CHAR)					\
  (DEFAULT_SWITCH_TAKES_ARG (CHAR)					\
    || (CHAR) == 'h'							\
    || (CHAR) == 'z' /* ignored by ld */				\
    || (CHAR) == 'R')

/* This defines which multi-letter switches take arguments.  */

#define FBSD_WORD_SWITCH_TAKES_ARG(STR)					\
  (DEFAULT_WORD_SWITCH_TAKES_ARG (STR)					\
   || !strcmp ((STR), "rpath") || !strcmp ((STR), "rpath-link")		\
   || !strcmp ((STR), "soname") || !strcmp ((STR), "defsym") 		\
   || !strcmp ((STR), "assert") || !strcmp ((STR), "dynamic-linker"))


#ifndef FREEBSD_NATIVE	/* these bits are here to reduce merge diffs, but I don't want to acutally use the bits right now */


#if FBSD_MAJOR == 6
#define FBSD_CPP_PREDEFINES \
  "-D__FreeBSD__=6 -Dunix -D__ELF__ -D__KPRINTF_ATTRIBUTE__ -Asystem=unix -Asystem=bsd -Asystem=FreeBSD"
#endif

#if FBSD_MAJOR == 5
#define FBSD_CPP_PREDEFINES \
  "-D__FreeBSD__=5 -Dunix -D__ELF__ -D__KPRINTF_ATTRIBUTE__ -Asystem=unix -Asystem=bsd -Asystem=FreeBSD"
#endif

#if FBSD_MAJOR == 4
#define FBSD_CPP_PREDEFINES \
  "-D__FreeBSD__=4 -Dunix -D__ELF__ -D__KPRINTF_ATTRIBUTE__ -Asystem=unix -Asystem=bsd -Asystem=FreeBSD"
#endif

#if FBSD_MAJOR == 3
#define FBSD_CPP_PREDEFINES \
  "-D__FreeBSD__=3 -Dunix -D__ELF__ -D__KPRINTF_ATTRIBUTE__ -Asystem=unix -Asystem=bsd -Asystem=FreeBSD"
#endif

#ifndef FBSD_CPP_PREDEFINES
#define FBSD_CPP_PREDEFINES \
  "-D__FreeBSD__   -Dunix -D__ELF__ -D__KPRINTF_ATTRIBUTE__ -Asystem=unix -Asystem=bsd -Asystem=FreeBSD"
#endif


#else	/* FREEBSD_NATIVE */
/* Place spaces around this string.  We depend on string splicing to produce
   the final CPP_PREDEFINES value.  */

#define FBSD_CPP_PREDEFINES \
  "-D__FreeBSD__=5 -D__FreeBSD_cc_version=500004 -Dunix -D__KPRINTF_ATTRIBUTE__ -Asystem=unix -Asystem=bsd -Asystem=FreeBSD"
#endif	/* ! FREEBSD_NATIVE */


/* Provide a CPP_SPEC appropriate for FreeBSD.  We just deal with the GCC 
   option `-posix', and PIC issues.  Try to detect support for the
   `long long' type.  Unfortunately the GCC spec parser will not allow us
   to properly detect the "iso9899:1990" and "iso9899:199409" forms of
   -std=c89.  Because of the ':' in the -std argument. :-(  I have left
   them in the spec as a place holder in hopes someone knows a way to make
   the detection of them work.  */

#define FBSD_CPP_SPEC "							\
  %(cpp_cpu)								\
  %{!maout: -D__ELF__}							\
  %{munderscores: -D__UNDERSCORES__}					\
  %{maout: %{!mno-underscores: -D__UNDERSCORES__}}			\
  %{fPIC:-D__PIC__ -D__pic__} %{fpic:-D__PIC__ -D__pic__}		\
  %{!ansi:%{!std=c89:%{!std=iso9899.1990:%{!std=iso9899.199409:-D_LONGLONG}}}} \
  %{posix:-D_POSIX_SOURCE}"

/* Provide a STARTFILE_SPEC appropriate for FreeBSD.  Here we add
   the magical crtbegin.o file (see crtstuff.c) which provides part 
	of the support for getting C++ file-scope static object constructed 
	before entering `main'. */
   
#define FBSD_STARTFILE_SPEC \
  "%{!shared: \
     %{pg:gcrt1.o%s} %{!pg:%{p:gcrt1.o%s} \
		       %{!p:%{profile:gcrt1.o%s} \
			 %{!profile:crt1.o%s}}}} \
   crti.o%s %{!shared:crtbegin.o%s} %{shared:crtbeginS.o%s}"

/* Provide a ENDFILE_SPEC appropriate for FreeBSD.  Here we tack on
   the magical crtend.o file (see crtstuff.c) which provides part of 
	the support for getting C++ file-scope static object constructed 
	before entering `main', followed by a normal "finalizer" file, 
	`crtn.o'.  */

#define FBSD_ENDFILE_SPEC \
  "%{!shared:crtend.o%s} %{shared:crtendS.o%s} crtn.o%s"

/* Provide a LIB_SPEC appropriate for FreeBSD as configured and as
   required by the user-land thread model.  Before __FreeBSD_version
   500016, select the appropriate libc, depending on whether we're
   doing profiling or need threads support.  At __FreeBSD_version
   500016 and later, when threads support is requested include both
   -lc and -lc_r instead of only -lc_r.  To make matters interesting,
   we can't actually use __FreeBSD_version provided by <osreldate.h>
   directly since it breaks cross-compiling.  As a final twist, make
   it a hard error if -pthread is provided on the command line and gcc
   was configured with --disable-threads (this will help avoid bug
   reports from users complaining about threading when they
   misconfigured the gcc bootstrap but are later consulting FreeBSD
   manual pages that refer to the mythical -pthread option).  */

/* Provide a LIB_SPEC appropriate for FreeBSD.  Just select the appropriate
   libc, depending on whether we're doing profiling or need threads support.
   (simular to the default, except no -lg, and no -p).  */

#ifdef FBSD_NO_THREADS
#define FBSD_LIB_SPEC "							\
  %{pthread: %eThe -pthread option is only supported on FreeBSD when gcc \
is built with the --enable-threads configure-time option.}		\
  %{!shared:								\
    %{!pg: -lc}								\
    %{pg:  -lc_p}							\
  }"
#else
#include <sys/param.h>
#if __FreeBSD_version >= 500016
#define FBSD_LIB_SPEC "							\
  %{!shared:								\
    %{!pg: %{pthread:-lc_r} -lc}					\
    %{pg:  %{pthread:-lc_r_p} -lc_p}					\
  }"
#else
#define FBSD_LIB_SPEC "							\
  %{!shared:								\
    %{!pg:								\
      %{!pthread:-lc}							\
      %{pthread:-lc_r}}							\
    %{pg:								\
      %{!pthread:-lc_p}							\
      %{pthread:-lc_r_p}}						\
  }"
#endif
#endif
