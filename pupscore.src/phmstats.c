/*----------------------------------------------------------------------------
    Access the statistics pups_maintained by `malloc'.
    Copyright 1990, 1991, 1992 Free Software Foundation, Inc.
		  Written May 1989 by Mike Haertel.

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

    Persistent heap modification by Mark O'Neill (mao@tumblingdice.co.uk)
    (C) 1998-2022 M.A. O'Neill, Tumbling Dice
---------------------------------------------------------------------------*/

#ifndef	_PHMALLOC_INTERNAL
#define _PHMALLOC_INTERNAL
#include <phmalloc.h>
#endif /* _PHMALLOC_INTERNAL */
#include <xtypes.h>

_PUBLIC struct mstats phmstats (int hdes)
{
  struct mstats result;

  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_lock(&htab_mutex);
  #endif /* PTHREAD_SUPPORT */

  result.bytes_total  = (char *) (*__phmorecore) (hdes, 0) - _pheapbase[hdes];
  result.chunks_used  = _pheap_chunks_used[hdes];
  result.bytes_used   = _pheap_bytes_used[hdes];
  result.chunks_free  = _pheap_chunks_free[hdes];
  result.bytes_free   = _pheap_bytes_free[hdes];

  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_unlock(&htab_mutex);
  #endif /* PTHREAD_SUPPORT */

  return result;
}
