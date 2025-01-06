/*------------------------------------------------------------------------
    Copyright (C) 1991, 1992, 1994 Free Software Foundation, Inc.

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
   (C) 1998-2024 M.A. O'Neill, Tumbling Dice
-----------------------------------------------------------------------*/

#include <errno.h>
#include <stdint.h>

#ifndef	_PHMALLOC_INTERNAL
#define	_PHMALLOC_INTERNAL
#include <phmalloc.h>
#endif /* SHMALLOC_INTERNAL */

#include <xtypes.h>


/*-----------------------------------------------------------*/
/* Allocate an array of NMEMB elements each SIZE bytes long. */
/* The entire array is initialized to zeros.                 */
/*-----------------------------------------------------------*/

_PUBLIC __ptr_t phcalloc (const uint32_t hdes, const __malloc_size_t nmemb, const __malloc_size_t size, const char *name)
{
  register __ptr_t result;


  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_lock(&htab_mutex);
  #endif /* PTHREAD_SUPPORT */


  /*--------------------------------------------------------------------------*/
  /* Does this persistent object already exits? If so, we cannot allocate it! */
  /*--------------------------------------------------------------------------*/

  if(name != (char *)NULL && msm_phobject_exists(hdes,name))
  {  errno = EEXIST;

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_unlock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */

     return((__ptr_t *)NULL);
  }

  result  = phmalloc (hdes, nmemb * size, name);

  if (result != NULL)
    (void) memset (result, 0, nmemb * size);

  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_unlock(&htab_mutex);
  #endif /* PTHREAD_SUPPORT */

  return result;
}
