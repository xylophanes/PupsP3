/*----------------------------------------------------------------------------
    Copyright (C) 1991, 1992, 1993, 1994, 1995 Free Software Foundation, Inc.

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

    Persistent heap modification by Mark O'Neill (mao@tumblingdice.co.uk)
    (C) 1998-2022 M.A. O'Neill, Tumbling Dice
----------------------------------------------------------------------------*/

#ifndef	_PHMALLOC_INTERNAL
#define _PHMALLOC_INTERNAL
#include <phmalloc.h>
#endif /* _PHMALLOC_INTERNAL */

#include <xtypes.h>

/*--------------------------------------------------*/
/* Add a persistent object to persistent object map */
/*--------------------------------------------------*/

_PROTOTYPE _EXTERN int msm_map_object(const unsigned int, const unsigned int, const void *, const char *);


/*-------------------------------------------------------*/
/* Remove a persistent object from persistent object map */
/*-------------------------------------------------------*/

_PROTOTYPE _EXTERN int msm_unmap_object(const unsigned int, const unsigned int);


/*-------------------------*/
/* Get free object mapslot */
/*-------------------------*/

_PROTOTYPE _EXTERN int msm_get_free_mapslot(const unsigned int);


_PUBLIC __ptr_t (*__phmemalign_hook) __P ((const unsigned int, const __malloc_size_t __alignment, size_t __size, const char *));
_PUBLIC __ptr_t phmemalign (const unsigned int hdes, const __malloc_size_t alignment, __malloc_size_t size, const char *name)
{
  int               h_index;
  __ptr_t           result;
  unsigned long int adj;

  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_lock(&htab_mutex);
  #endif /* PTHREAD_SUPPORT */

  if(__phmemalign_hook)
  {
     result = (*__phmemalign_hook) (hdes, alignment, size, name);

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_unlock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */

     return result;
  }

  size = ((size + alignment - 1) / alignment) * alignment;

  result = phmalloc (hdes, size, (char *)NULL);
  if(result == NULL)
  {

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_unlock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */

     return NULL;
  }

  adj = (unsigned long int) ((unsigned long int) ((char *) result - (char *) NULL)) % alignment;

  if (adj != 0)
  {
      struct alignlist *l = (struct alignlist *)NULL;
      for(l = _aligned_blocks; l != NULL; l = l->next)
      {  if(l->aligned == NULL)


            /*-----------------------------*/
	    /* This slot is free.  Use it. */
            /*-----------------------------*/

	    break;
      }

      if (l == NULL)
      {
	  l = (struct alignlist *) phmalloc (hdes, sizeof (struct alignlist), (char *)NULL);
	  if (l == NULL)
          {
	      phfree (hdes, result);
	      return NULL;
	  }
	  l->next         = _aligned_blocks;
	  _aligned_blocks = l;
      }
      l->exact = result;
      result   = l->aligned = (char *) result + alignment - adj;
  }

  if(name != (char *)NULL)
  {  h_index = msm_get_free_mapslot(hdes);
     msm_map_object(hdes,h_index,result,name);
  }

  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_unlock(&htab_mutex);
  #endif /* PTHREAD_SUPPORT */
 
  return result;
}
