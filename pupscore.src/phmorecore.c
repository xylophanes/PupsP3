/*-----------------------------------------------------------------------------
    Copyright (C) 1991, 1992, 1993, 1994 Free Software Foundation, Inc.
    This file is part of the GNU C Library.

    The GNU C Library is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2, or (at your option)
    any later version.

    The GNU C Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the GNU C Library; see the file COPYING.  If not, write to
    the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

    Persistent heap modifications by Mark O'Neill (mao@tumblingdice.co.uk)
    (C) 1998-2022 M.A. O'Neill, Tumbling Dice
----------------------------------------------------------------------------*/

#include <pheap.h>

#ifndef	_PHMALLOC_INTERNAL
#define	_PHMALLOC_INTERNAL
#include <phmalloc.h>
#endif /* _PHMALLOC_INTERNAL */

#ifdef __GNU_LIBRARY__


/*-------------------------------------------------------------------------*/
/* It is best not to declare this and cast its result on foreign operating */
/* systems with potentially hostile include files.                         */
/*-------------------------------------------------------------------------*/

extern void *msm_sbrk __P ((const unsigned int hdes, size_t increment));
#endif /* __GNU_LIBRARY__ */

#ifndef NULL
#define NULL 0
#endif /* NULL */


/*--------------------------------------------------------*/
/* Allocate INCREMENT more bytes of data space,           */
/* and return the start of data space, or NULL on errors. */
/* If INCREMENT is negative, shrink data space.           */
/*--------------------------------------------------------*/

#ifdef __STDC__
_PUBLIC __ptr_t __default_phmorecore (int hdes,  ptrdiff_t increment)
#else
_PUBLIC __ptr_t __default_phmorecore (int hdes,  int increment)
#endif /* __STDC__ */
{
  __ptr_t result = (__ptr_t)msm_sbrk(hdes, increment);

  if(result == (__ptr_t) -1)
    return NULL;

  return result;
}
