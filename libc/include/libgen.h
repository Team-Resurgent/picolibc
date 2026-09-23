/* Copyright (c) 2005 Jeff Johnston  <jjohnstn@redhat.com> */
/*
 * libgen.h - defined by XPG4
 */

#ifndef _LIBGEN_H_
#define _LIBGEN_H_

#include <sys/cdefs.h>

_BEGIN_STD_C

/* There are two common basename variants.  If you do NOT #include <libgen.h>
   and you do

     #define _GNU_SOURCE
     #include <string.h>

   you get the GNU version.  Otherwise you get the POSIX versionfor which you
   should #include <libgen.h>i for the function prototype.  POSIX requires that
   #undef basename will still let you invoke the underlying function.  However,
   this also implies that the POSIX version is used in this case.  That's made
   sure here. */
/* RXDK: <string.h> now declares the GNU basename under __GNU_VISIBLE as the C
   identifier `basename` (bound by asm label to __gnu_basename). Declaring the XPG
   basename here as another plain `basename` is then a conflicting-types error, so
   bind the XPG variant to the real `basename` symbol under a private name and
   redirect calls with a macro -- the way glibc separates __xpg_basename from the
   GNU one. #undef first so any `basename` macro <string.h> installed is dropped
   and the XPG variant wins; this also makes string.h skip its GNU block when it is
   included after us (its guard is `__GNU_VISIBLE && !defined(basename)`). */
#undef basename
char *__xpg_basename(char *) __asm__("basename");
#define basename __xpg_basename
char *dirname(char *);

_END_STD_C

#endif /* _LIBGEN_H_ */
