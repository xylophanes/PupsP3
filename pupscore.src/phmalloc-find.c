/*-----------------------------------------------------------------------------
   Find the starting address of a malloc'd block, from anywhere inside it.
   Copyright (C) 1995 Free Software Foundation, Inc.

   This file is part of the GNU C Library.  Its master source is NOT part of
   the C library, however.  The master source lives in /gd/gnu/lib.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If
   not, write to the Free Software Foundation, Inc., 675 Mass Ave,
   Cambridge, MA 02139, USA.

   Persistent heap modifications by Mark O'Neill (mao@tumblingdice.co.uk)
   (C) 1998-2019 M.A. O'Neill, Tumbling Dice
----------------------------------------------------------------------------*/

#ifndef	_PHMALLOC_INTERNAL
#define _PHMALLOC_INTERNAL
#include <phmalloc.h>
#endif /* _PHMALLOC_INTERNAL */
#include <xtypes.h>


/*------------------------------------------------------*/
/* Given an address in the middle of a malloc'd object, */
/* return the address of the beginning of the object.   */
/*------------------------------------------------------*/

_PUBLIC __ptr_t malloc_find_object_address(int hdes, __ptr_t ptr)
{

  __ptr_t         result;
  __malloc_size_t block = BLOCK (hdes, ptr);
  int             type  = _pheapinfo[hdes][block].busy.type;

  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_lock(&htab_mutex);
  #endif /* PTHREAD_SUPPORT */

  if (type == 0)
  {

      /*------------------------------------------*/
      /* The object is one or more entire blocks  */
      /*------------------------------------------*/

      __malloc_ptrdiff_t sizevalue = _pheapinfo[hdes][block].busy.info.size;

      if(sizevalue < 0)
      {

        /*-------------------------------------------------------*/
	/* This is one of the blocks after the first.  SIZEVALUE */
	/* says how many blocks to go back to find the first.    */
        /*-------------------------------------------------------*/

	block += sizevalue;
      }


      /*---------------------------------------------*/
      /* BLOCK is now the first block of the object. */
      /*  Its start is the start of the object.      */
      /*---------------------------------------------*/

      result = ADDRESS (hdes, block);

      #ifdef PTHREAD_SUPPORT
      (void)pthread_mutex_unlock(&htab_mutex);
      #endif /* PTHREAD_SUPPORT */

      return result;
  }
  else
  {


      /*-----------------------------------------*/
      /* Get the size of fragments in this block */
      /*-----------------------------------------*/

      __malloc_size_t size = 1 << type;


      /*-----------------------------------------------------------------*/
      /* Turn off the low bits to find the start address of the fragment */
      /*-----------------------------------------------------------------*/

      result = _pheapbase[hdes] + (((char *) ptr - _pheapbase[hdes]) & ~(size - 1));


      #ifdef PTHREAD_SUPPORT
      (void)pthread_mutex_unlock(&htab_mutex);
      #endif /* PTHREAD_SUPPORT */

      return result;
    }
}
