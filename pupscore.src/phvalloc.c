/*---------------------------------------------------------------------------
     Allocate memory on a page boundary.
     Copyright (C) 1991, 1992, 1993, 1994 Free Software Foundation, Inc.

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public License as
     published by the Free Software Foundation; either version 2 of the
     License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.

     You should have received a copy of the GNU Library General Public
     License along with this library; see the file COPYING.LIB.  If
     not, write to the Free Software Foundation, Inc., 675 Mass Ave,
     Cambridge, MA 02139, USA.

     The author may be reached (Email) at the address mike@ai.mit.edu,
     or (US mail) as Mike Haertel c/o Free Software Foundation.

     Shared heap modifications by Mark O'Neill (mao@tumblingdice.co.uk)
     (C) 1998-2022 M.A. O'Neill, Tumbling Dice
---------------------------------------------------------------------------*/

#ifndef GLIBC
#if defined (__GNU_LIBRARY__) || defined (_LIBC)
#include <stddef.h>
#include <sys/cdefs.h>
extern size_t __getpagesize __P ((void));
#else
#define	 __getpagesize()	getpagesize()
#endif /* defined (__GNU_LIBRARY__) || defined (_LIBC) */
#endif /* GLIBC                                        */


#ifndef	_PHMALLOC_INTERNAL
#define	_PHMALLOC_INTERNAL
#include <phmalloc.h>
#endif /* _PHMALLOC_INTERNAL */
#include <xtypes.h>


_PRIVATE __malloc_size_t pagesize;

_PUBLIC __ptr_t phvalloc (const unsigned int hdes, const __malloc_size_t size, const char *name)
{

  if(pagesize == 0)
    pagesize = __getpagesize();

  return phmemalign (hdes, pagesize, size, name);
}
