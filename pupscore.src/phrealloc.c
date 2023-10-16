/*--------------------------------------------------------------------------
    Change the size of a block allocated by `malloc'.
    Copyright 1990, 1991, 1992, 1993, 1994 Free Software Foundation, Inc.
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

    Persistent heap modifications by Mark O'Neill (mao@tumblingdice.co.uk)
    (C) 1998-2022 M.A. O'Neill, Tumbling Dice
-------------------------------------------------------------------------*/

#ifndef	_PHMALLOC_INTERNAL
#define _PHMALLOC_INTERNAL
#include <phmalloc.h>
#endif /* _PHMALLOC_INTERNAL */

#include <xtypes.h>
#include <unistd.h>
#include <errno.h>


#if (defined (MEMMOVE_MISSING) || !defined(_LIBC) && !defined(STDC_HEADERS) && !defined(USG))


/*----------------------------------------------------*/
/* Snarfed directly from Emacs src/dispnew.c:         */
/* XXX Should use system bcopy if it handles overlap. */
/*----------------------------------------------------*/

#ifndef emacs


/*---------------------------------------------------------*/
/* Used to signify an internal heaplib call (which should  */
/* not use object or heap locking)                         */
/*---------------------------------------------------------*/

_IMPORT int _pheap_internal;


/*-------------------------------------------------------------*/
/* Used to signify that we should not update persistent object */
/* table (as this is an internal operation)                    */
/*-------------------------------------------------------------*/

_IMPORT int _no_phobject_mapping;


/*--------------------------------------------------*/
/* Add a persistent object to persistent object map */
/*--------------------------------------------------*/

_PROTOTYPE _EXTERN int msm_map_object(const unsigned int, const unsigned int, const void *, const char *);


/*-----------------------------------------------------*/
/* Remove persistent object from persistent object map */
/*-----------------------------------------------------*/

_PROTOTYPE _EXTERN int msm_unmap_object(const unsigned int, const unsigned int);


/*--------------------------------------------------*/
/* Like bcopy except never gets confused by overlap */
/*--------------------------------------------------*/

_PRIVATE void safe_bcopy (char *from, char *to, int size)
{
  if (size <= 0 || from == to)
    return;


    /*------------------------------------------------------------------*/
    /* If the source and destination don't overlap, then bcopy can      */
    /* handle it.  If they do overlap, but the destination is lower in  */
    /* memory than the source, we'll assume bcopy can handle that.      */
    /*------------------------------------------------------------------*/

  if(to < from || from + size <= to)
     (void)bcopy (from, to, size);
  else
  {

      /*------------------------------------*/
      /* Otherwise, we'll copy from the end */
      /*------------------------------------*/

      register char *endf = from + size;
      register char *endt = to + size;


      /*--------------------------------------------------------------*/
      /* If TO - FROM is large, then we should break the copy into    */
      /* nonoverlapping chunks of TO - FROM bytes each.  However, if  */
      /* TO - FROM is small, then the bcopy function call overhead    */
      /* makes this not worth it.  The crossover point could be about */
      /* anywhere.  Since I don't think the obvious copy loop is too  */
      /* bad, I'm trying to err in its favor.                         */
      /*--------------------------------------------------------------*/

      if (to - from < 64)
      {
	  do {    *--endt = *--endf;
	     } while (endf != from);
      }
      else
      {  for (;;)
	 {
	      endt -= (to - from);
	      endf -= (to - from);

	      if(endt < to)
		 break;

	      (void)bcopy (endf, endt, to - from);
	  }


          /*---------------------------------------------------------*/
          /* If SIZE wasn't a multiple of TO - FROM, there will be a */
          /* little left over.  The amount left over is              */
          /* (endt + (to - from)) - to, which is endt - from.        */
          /*---------------------------------------------------------*/

	  (void)bcopy (from, to, endt - from);
	}
    }
}

        /*-----------*/
#endif	/* Not emacs */
        /*-----------*/

#define memmove(to, from, size) safe_bcopy ((from), (to), (size))
#endif /* (defined (MEMMOVE_MISSING) || !defined(_LIBC) && !defined(STDC_HEADERS) && !defined(USG)) */


#define min(A, B) ((A) < (B) ? (A) : (B))


/*----------------------------*/
/* Debugging hook for realloc */
/*----------------------------*/

_PUBLIC __ptr_t (*__phrealloc_hook) __P ((const unsigned int, const __ptr_t __ptr, __malloc_size_t const __size, const char *));


/*---------------------------------------------------------------*/
/* Resize the given region to the new size, returning a pointer  */
/* to the (possibly moved) region.  This is optimized for speed; */
/* some benchmarks seem to indicate that greater compactness is  */
/* achieved by unconditionally allocating and copying to a       */
/* new region.  This module has incestuous knowledge of the      */
/* internals of both free and malloc.                            */
/*---------------------------------------------------------------*/

_PUBLIC __ptr_t phrealloc (const unsigned int hdes, const __ptr_t ptr, __malloc_size_t size, const char *name)
{
  __ptr_t result;

  int type,
      mapped,
      h_index;

  __malloc_size_t block,
                  blocks,
                  oldlimit,
                  req_size;


  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_lock(&htab_mutex);
  #endif /* PTHREAD_SUPPORT */

  /*--------------------------------------------------------------------------*/
  /* Does this persistent object already exits? If so, we cannot allocate it! */
  /*--------------------------------------------------------------------------*/

  if(name != (const char *)NULL && msm_phobject_exists(hdes,name))
  {  errno = EEXIST;

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_unlock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */

     return((__ptr_t *)NULL);
  }

  if(size == 0)
  {  (void)phfree (hdes, ptr);

     result = phmalloc (hdes, 0, name);

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_unlock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */

     return result;
  }
  else if (ptr == NULL)
  {  result = phmalloc(hdes, size, name);

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_unlock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */

     return result;
  }

  if(__phrealloc_hook != NULL)
  {
     result = (*__phrealloc_hook) (hdes, ptr, size, name);

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_unlock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */

     return result;
  }

  if((h_index = msm_map_objectaddr2index(hdes,ptr)) == (-1))
  {  errno = EACCES;

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_unlock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */

     return NULL;
  }

  block = BLOCK (hdes, ptr);

  req_size = size;
  type     = _pheapinfo[hdes][block].busy.type;
  switch (type)
  {
    case 0:

      /*----------------------------------------------------*/
      /* Maybe reallocate a large block to a small fragment */
      /*----------------------------------------------------*/

      if (size <= BLOCKSIZE / 2)
      {
	  result = phmalloc (hdes, size, (char *)NULL);
	  if(result != NULL)
          {
	      (void)memcpy (result, ptr, size);

              _no_phobject_mapping = 1;
	      _phfree_internal (hdes, ptr);
              _no_phobject_mapping = 1;

              h_index = msm_map_objectname2index(hdes,name);

              (void)msm_map_setsize(hdes,h_index,req_size);

              #ifdef PTHREAD_SUPPORT
              (void)pthread_mutex_unlock(&htab_mutex);
              #endif /* PTHREAD_SUPPORT */

	      return result;
          }
      }


      /*---------------------------------------------*/
      /* The new size is a large allocation as well; */
      /* see if we can hold it in place.             */
      /*---------------------------------------------*/

      blocks = BLOCKIFY (size);
      if (blocks < _pheapinfo[hdes][block].busy.info.size)
      {

          /*---------------------------------*/
	  /* The new size is smaller; return */
	  /* excess memory to the free list. */
          /*---------------------------------*/

	  _pheapinfo[hdes][block + blocks].busy.type      = 0;
	  _pheapinfo[hdes][block + blocks].busy.info.size = _pheapinfo[hdes][block].busy.info.size - blocks;
	  _pheapinfo[hdes][block].busy.info.size          = blocks;


          /*---------------------------------------------------------------------*/
          /* We have just created a new chunk by splitting a chunk in two.       */
          /* Now we will free this chunk; increment the statistics counter       */
          /* so it doesn't become wrong when _free_pheap_internal decrements it. */
          /*---------------------------------------------------------------------*/

	  ++_pheap_chunks_used[hdes];

          _no_phobject_mapping = 1;
	  _phfree_internal (hdes, ADDRESS (hdes, block + blocks));
          _no_phobject_mapping = 0;

          h_index         = msm_map_objectname2index(hdes,name); 

          _phobjectmap[hdes][h_index]->addr = result; 
	  result = ptr;
      }
      else if (blocks == _pheapinfo[hdes][block].busy.info.size)


         /*--------------------------*/
         /* No size change necessary */
         /*--------------------------*/

         result = ptr;
      else
      {

         /*------------------------------------------------------------*/
         /* Won't fit, so allocate a new region that will.             */
         /* Free the old region first in case there is sufficient      */
         /* adjacent free space to grow without moving.                */
         /* blocks = _pheapinfo[hdes][block].busy.info.size;           */
         /* Prevent free from actually returning memory to the system. */
         /*------------------------------------------------------------*/

	  oldlimit           = _pheaplimit[hdes];
	  _pheaplimit[hdes] = 0;

          _no_phobject_mapping = 1;
	  _phfree_internal (hdes, ptr);
          _no_phobject_mapping = 0;

	  _pheaplimit[hdes] = oldlimit;
	  result = phmalloc (hdes, size, (char *)NULL);
	  if (result == NULL)
          {

             /*--------------------------------------------------*/
             /* Now we're really in trouble.  We have to unfree  */
             /* the thing we just freed.  Unfortunately it might */
             /* have been coalesced with its neighbours.         */
             /*--------------------------------------------------*/

	      if(_pheapindex[hdes] == block)
	        (void) phmalloc (hdes, blocks * BLOCKSIZE, (char *)NULL);
	      else
              {
		  __ptr_t previous = phmalloc (hdes, (block - _pheapindex[hdes]) * BLOCKSIZE, (char *)NULL);
		  (void) phmalloc (hdes, blocks * BLOCKSIZE, (char *)NULL);

                  _no_phobject_mapping = 1;
		  _phfree_internal (hdes, previous);
                  _no_phobject_mapping = 1;
              }

              h_index = msm_map_objectname2index(hdes,name); 

              msm_unmap_object(hdes,h_index); 
 
              #ifdef PTHREAD_SUPPORT
              (void)pthread_mutex_unlock(&htab_mutex);
              #endif /* PTHREAD_SUPPORT */

	      return NULL;
	  }

	  if(ptr != result)
	     memmove (result, ptr, blocks * BLOCKSIZE);
	}
        break;

    default:


         /*-------------------------------------------*/
         /* Old size is a fragment; type is logarithm */
	 /* to base two of the fragment size.         */
         /*-------------------------------------------*/

         if (size > (__malloc_size_t) (1 << (type - 1)) && size <= (__malloc_size_t) (1 << type))


             /*-------------------------------------------*/
	     /* The new size is the same kind of fragment */
             /*-------------------------------------------*/

	     result = ptr;
         else
	 {  

            /*--------------------------------------------------*/
            /* The new size is different; allocate a new space, */
	    /* and copy the lesser of the new size and the old. */
            /*--------------------------------------------------*/

	    result = phmalloc (hdes, size, (char *)NULL);

            if(result == NULL)  
            {  h_index = msm_map_objectname2index(hdes,name);  

               msm_unmap_object(hdes,h_index);  

               #ifdef PTHREAD_SUPPORT
               (void)pthread_mutex_unlock(&htab_mutex);
               #endif /* PTHREAD_SUPPORT */

	       return NULL;
            }

	    (void)memcpy (result, ptr, min (size, (__malloc_size_t) 1 << type));
	    (void)phfree (hdes, ptr);
	}
        break;
    }

  h_index                           = msm_map_objectname2index(hdes,name);
  _phobjectmap[hdes][h_index]->addr = result; 

  (void)msm_map_setsize(hdes,h_index,req_size);
 
  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_unlock(&htab_mutex);
  #endif /* PTHREAD_SUPPORT */

  return result;
}
