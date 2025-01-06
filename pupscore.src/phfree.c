/*-------------------------------------------------------------------------
    Free a block of memory allocated by `malloc'.
    Copyright 1990, 1991, 1992, 1994 Free Software Foundation, Inc.
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
    (C) 1998-2024 M.A. O'Neill, Tumbling Dice
-------------------------------------------------------------------------*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <xtypes.h>
#include <stdint.h>

#ifndef	_PHMALLOC_INTERNAL
#define _PHMALLOC_INTERNAL
#include <phmalloc.h>
#endif /* _PHMALLOC_INTERNAL */


_IMPORT heap_type         *htable;
_IMPORT __malloc_size_t   *pheapsize;


/*--------------------------------------------------*/
/* Add a persistent object to persistent object map */
/*--------------------------------------------------*/

_PROTOTYPE _EXTERN int32_t msm_map_object(const uint32_t, const uint32_t, const void *, const char *);


/*-----------------------------------------------------------------------------------*/
/* Remove a persistent object persistent object map within persistent memory segment */
/*-----------------------------------------------------------------------------------*/

_PROTOTYPE _EXTERN int32_t msm_unmap_object(const uint32_t, const uint32_t);


/*-------------------------*/
/* Debugging hook for free */
/*-------------------------*/

_PUBLIC void (*__phfree_hook) __P ((const uint32_t, const __ptr_t __ptr));


/*---------------------------------------*/
/* List of blocks allocated by memalign. */
/*---------------------------------------*/

_PUBLIC struct alignlist *_aligned_blocks = NULL;


/*-----------------------*/
/* Object mapping switch */
/*-----------------------*/

_PUBLIC int32_t _no_phobject_mapping;


/*---------------------------*/
/* Number of objects in heap */
/*---------------------------*/

_IMPORT int32_t *_phobjects;


/*------------------------------------------------------------*/
/* Return memory to the heap.                                 */
/* Like `free' but don't call a __free_hook if there is one.  */
/*------------------------------------------------------------*/

_PUBLIC void _phfree_internal (const uint32_t hdes, const __ptr_t ptr)
{
   int32_t type,
           h_index;

  __malloc_size_t block,
                  blocks,
                  i;

  struct list *prev = (struct list *)NULL,
              *next = (struct list *)NULL;

  if(_no_phobject_mapping == 0)
  {  if((h_index = msm_find_mapped_object(hdes,ptr)) == (-1))
     {  errno = EINVAL;
        return;
     }
  }

  block = BLOCK (hdes, ptr);

#ifdef DEBUG
  (void)fprintf(stderr,"SHFREE BLOCK: %d\n",block);
  (void)fflush(stderr);
#endif /* DEBUG */

  type = _pheapinfo[hdes][block].busy.type;
  switch (type)
  {
    case 0:


      /*--------------------------------------------*/
      /* Get as many statistics as early as we can. */
      /*--------------------------------------------*/

      --_pheap_chunks_used[hdes];
      _pheap_bytes_used[hdes] -= _pheapinfo[hdes][block].busy.info.size * BLOCKSIZE;
      _pheap_bytes_free[hdes] += _pheapinfo[hdes][block].busy.info.size * BLOCKSIZE;


      /*----------------------------------------------------------------*/
      /* Find the free cluster previous to this one in the free list.   */
      /* Start searching at the last block referenced; this may benefit */
      /* programs with locality of allocation.                          */
      /*----------------------------------------------------------------*/

      i = _pheapindex[hdes];
      if(i > block)
      {  while (i > block)
	   i = _pheapinfo[hdes][i].free.prev;
      }
      else
      {  do {    i = _pheapinfo[hdes][i].free.next;
	    } while (i > 0 && i < block);
	 i = _pheapinfo[hdes][i].free.prev;
      }


      /*-------------------------------------------------------*/
      /* Determine how to link this block  int32_to the free list.  */
      /*-------------------------------------------------------*/

      if (block == i + _pheapinfo[hdes][i].free.size)
      {

          /*--------------------------------------------*/
	  /* Coalesce this block with its predecessor.  */
          /*--------------------------------------------*/

	  _pheapinfo[hdes][i].free.size += _pheapinfo[hdes][block].busy.info.size;
	  block = i;
      }
      else
      {

          /*--------------------------------------------------*/
	  /* Really link this block back  int32_to the free list.  */
          /*--------------------------------------------------*/

	  _pheapinfo[hdes][block].free.size = _pheapinfo[hdes][block].busy.info.size;
	  _pheapinfo[hdes][block].free.next = _pheapinfo[hdes][i].free.next;
	  _pheapinfo[hdes][block].free.prev = i;
	  _pheapinfo[hdes][i].free.next = block;
	  _pheapinfo[hdes][_pheapinfo[hdes][block].free.next].free.prev = block;
	  ++_pheap_chunks_free[hdes];
      }


      /*-------------------------------------------------------------*/
      /* Now that the block is linked in, see if we can coalesce it  */
      /* with its successor (by deleting its successor from the list */
      /* and adding in its size).                                    */
      /*-------------------------------------------------------------*/

      if (block + _pheapinfo[hdes][block].free.size == _pheapinfo[hdes][block].free.next)
      {
	  _pheapinfo[hdes][block].free.size                             += _pheapinfo[hdes][_pheapinfo[hdes][block].free.next].free.size;
	  _pheapinfo[hdes][block].free.next                              = _pheapinfo[hdes][_pheapinfo[hdes][block].free.next].free.next;
	  _pheapinfo[hdes][_pheapinfo[hdes][block].free.next].free.prev = block;
	  --_pheap_chunks_free[hdes];
      }


      /*------------------------------------------------*/
      /* Now see if we can return stuff to the system.  */
      /*------------------------------------------------*/

      blocks = _pheapinfo[hdes][block].free.size;
      if (blocks >= FINAL_FREE_BLOCKS                               &&
          block + blocks == _pheaplimit[hdes]                       &&
	  (*__phmorecore) (hdes, 0) == ADDRESS (hdes, block + blocks))
      {
	  __malloc_size_t bytes  = blocks * BLOCKSIZE;
	  _pheaplimit[hdes]     -= blocks;

	  (*__phmorecore) (hdes, -bytes);
	  _pheapinfo[hdes][_pheapinfo[hdes][block].free.prev].free.next = _pheapinfo[hdes][block].free.next;
	  _pheapinfo[hdes][_pheapinfo[hdes][block].free.next].free.prev = _pheapinfo[hdes][block].free.prev;
	  block = _pheapinfo[hdes][block].free.prev;
	  --_pheap_chunks_free[hdes];
	  _pheap_bytes_free[hdes] -= bytes;
      }


      /*----------------------------------------------*/
      /* Set the next search to begin at this block.  */
      /*----------------------------------------------*/

      _pheapindex[hdes] = block;
      break;

    default:


      /*-----------------------------*/
      /* Do some of the statistics.  */
      /*-----------------------------*/

      --_pheap_chunks_used[hdes];
      _pheap_bytes_used[hdes] -= 1 << type;
      ++_pheap_chunks_free[hdes];
      _pheap_bytes_free[hdes] += 1 << type;


      /*------------------------------------------------------------*/
      /* Get the address of the first free fragment in this block.  */
      /*------------------------------------------------------------*/

      prev = (struct list *) ((char *) ADDRESS (hdes, block) + (_pheapinfo[hdes][block].busy.info.frag.first << type));

      if(_pheapinfo[hdes][block].busy.info.frag.nfree == (BLOCKSIZE >> type) - 1)
      {

          /*------------------------------------------------------*/
	  /* If all fragments of this block are free, remove them */
	  /* from the fragment list and free the whole block.     */
          /*------------------------------------------------------*/

	  next = prev;
	  for (i = 1; i < (__malloc_size_t) (BLOCKSIZE >> type); ++i)
	  {    next = next->next;


               /*-------------------------------------------------------------*/
               /* Check for partially relocated addresses which tend to occur */
               /* when an attached client falls over unexpectedly             */
               /*-------------------------------------------------------------*/

               if((unsigned long)next < htable[hdes].sdata   || 
                  (unsigned long)next > htable[hdes].edata    ) 
                  next = (struct list *)((unsigned long)next + (unsigned long)htable[hdes].addr);
          }

	  prev->prev->next = next;
	  if(next != NULL)
	     next->prev = prev->prev;

	  _pheapinfo[hdes][block].busy.type      = 0;
	  _pheapinfo[hdes][block].busy.info.size = 1;


          /*--------------------------------*/
	  /* Keep the statistics accurate.  */
          /*--------------------------------*/

	  ++_pheap_chunks_used[hdes];
	  _pheap_bytes_used[hdes]  += BLOCKSIZE;
	  _pheap_chunks_free[hdes] -= BLOCKSIZE >> type;
	  _pheap_bytes_free[hdes]  -= BLOCKSIZE;

	  phfree (hdes, ADDRESS (hdes, block));
      }
      else if (_pheapinfo[hdes][block].busy.info.frag.nfree != 0)
      {

          /*------------------------------------------------------*/
	  /* If some fragments of this block are free, link this  */
	  /* fragment into the fragment list after the first free */
          /* fragment of this block.                              */
          /*------------------------------------------------------*/

	  next = (struct list *) ptr;
	  next->next = prev->next;
	  next->prev = prev;
	  prev->next = next;

	  if (next->next != NULL)
	    next->next->prev = next;
	  ++_pheapinfo[hdes][block].busy.info.frag.nfree;
      }
      else
      {

          /*---------------------------------------------------*/
	  /* No fragments of this block are free, so link this */
	  /* fragment into the fragment list and announce that */
          /* it is the first free fragment of this block.      */
          /*---------------------------------------------------*/

	  prev = (struct list *) ptr;
	  _pheapinfo[hdes][block].busy.info.frag.nfree = 1;
	  _pheapinfo[hdes][block].busy.info.frag.first = (uint64_t)
	  ((uint64_t)((char *) ptr - (char *) NULL) % BLOCKSIZE >> type);

	  prev->next       = _phfraghead[hdes][type].next;
	  prev->prev       = &_phfraghead[hdes][type];
	  prev->prev->next = prev;

	  if(prev->next != NULL)
	    prev->next->prev = prev;
      }
      break;
    }


    /*---------------------------------------------------------------------*/
    /* Update persistent object table to reflect fact that we have removed */
    /* an object. Mark O'Neill 31/3/98                                     */
    /*---------------------------------------------------------------------*/

    (void)msm_unmap_object(hdes,h_index);

#ifdef DEBUG
(void)fprintf(stderr,"PHFREE EXIT\n");
(void)fflush(stderr);
#endif /* DEBUG */

}



/*----------------------------*/
/* Return memory to the heap. */
/*----------------------------*/

void *phfree (const uint32_t hdes, const __ptr_t ptr)

{ struct alignlist *l;

  if (ptr == NULL)
    return((void *)NULL);

  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_lock(&htab_mutex);
  #endif /* PTHREAD_SUPPORT */

  for (l = _aligned_blocks; l != NULL; l = l->next)
  {   if (l->aligned == ptr)
      {

                                /*-----------------------------------*/
 	 l->aligned = NULL;	/* Mark the slot in the list as free */
                                /*-----------------------------------*/

	 ptr = l->exact;
	 break;
      }
  }

  if (__phfree_hook != NULL)
     (*__phfree_hook) (hdes, ptr);
  else
     _phfree_internal (hdes, ptr);

  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_unlock(&htab_mutex);
  #endif /* PTHREAD_SUPPORT */

  return((void *)NULL);
}
