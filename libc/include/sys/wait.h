/*
Copyright (c) 1982, 1986, 1993
The Regents of the University of California.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
3. Neither the name of the University nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.
 */
#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

#include <sys/cdefs.h>

_BEGIN_STD_C

#include <sys/types.h>
#include <sys/_wait.h>
#include <sys/resource.h>

pid_t wait(int *);
pid_t waitpid(pid_t, int *, int);
pid_t wait3(int *, int, struct rusage *);
pid_t wait4(pid_t, int *, int, struct rusage *);

/* waitid()/idtype_t need siginfo_t, which <signal.h> exposes only under
   __POSIX_VISIBLE; guard the addition the same way. A single-title console has
   no children, so waitid() fails ECHILD (posix_unsupported.c). */
#if __POSIX_VISIBLE
#include <signal.h>   /* siginfo_t */
typedef enum { P_ALL = 0, P_PID = 1, P_PGID = 2 } idtype_t;
/* waitid() option bits; guarded individually because sys/_wait.h already defines
   some (e.g. WCONTINUED) with its own value -- keep those, add the rest. */
#ifndef WEXITED
#define WEXITED    0x04
#endif
#ifndef WSTOPPED
#define WSTOPPED   0x08
#endif
#ifndef WCONTINUED
#define WCONTINUED 0x10
#endif
#ifndef WNOWAIT
#define WNOWAIT    0x01000000
#endif
int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);
#endif

_END_STD_C

#endif
