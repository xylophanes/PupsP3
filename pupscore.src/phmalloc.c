/*------------------------------------------------------------------------------
   Memory allocator `phmalloc'.
   Copyright 1990, 1991, 1992, 1993, 1994, 1995 Free Software Foundation, Inc.
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
   (C) 1998-2019 M.A. O'Neill, Tumbling Dice
-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <xtypes.h>
#include <errno.h>

#ifndef	_PHMALLOC_INTERNAL
#define _PHMALLOC_INTERNAL
#include <phmalloc.h>
#endif /* _PHMALLOC_INTERNAL */


/*-----------------------------------------------------------*/
/* Signifies whether heap addresses are local or heap global */
/*-----------------------------------------------------------*/

_IMPORT int addresses_local;

// Heap table (for local process)
_IMPORT heap_type *htable;

// How to really get more memory
__ptr_t (*__phmorecore) __P ((int, ptrdiff_t __size)) = __default_phmorecore;

// Debugging hook for `malloc'
__ptr_t (*__phmalloc_hook) __P ((const unsigned int, __malloc_size_t __size, const char *));

//Number of objects in heap
_IMPORT int *_phobjects;

// Number of objects slots in heap
_IMPORT int *_phobjects_allocated;

// Pointer to table of significant objects on persistent heaps
_IMPORT phobmap_type ***_phobjectmap;

// Pointer to persistent heap parameter table (on persistent heap)
_IMPORT unsigned long int **_pheap_parameters;

// Pointer to the base of the first block.  */
_IMPORT char **_pheapbase;

// Block information table.  Allocated with align/__free (not malloc/free)
_IMPORT malloc_info **_pheapinfo;

// Number of info entries
_IMPORT __malloc_size_t *pheapsize;

// Search index in the info table
_IMPORT __malloc_size_t *_pheapindex;

// Limit of valid info table indices
_IMPORT __malloc_size_t *_pheaplimit;

// Free lists for each fragment size
_IMPORT struct list **_phfraghead;



/*------------------------------------------------*/
/* Add persistent object to persistent object map */
/*------------------------------------------------*/

_PROTOTYPE _EXTERN int msm_map_object(const unsigned int, const unsigned intint, const void *, const char *);


/*-----------------------------------------------------*/
/* Remove persistent object from persistent object map */
/*-----------------------------------------------------*/

_PROTOTYPE _EXTERN int msm_unmap_object(const unsigned int, const unsigned int);


/*-----------------*/
/* Instrumentation */
/*-----------------*/

_IMPORT __malloc_size_t *_pheap_chunks_used;
_IMPORT __malloc_size_t *_pheap_bytes_used;
_IMPORT __malloc_size_t *_pheap_chunks_free;
_IMPORT __malloc_size_t *_pheap_bytes_free;


/*----------------------*/
/* Are you experienced? */
/*----------------------*/

_IMPORT int *__phmalloc_initialized;

void (*__malloc_initialize_hook) __P ((int));
void (*__after_phmorecore_hook) __P ((void));


/*---------------------------------------------*/
/* Set everything up and remember that we have */
/*---------------------------------------------*/

_PRIVATE int initialize __P ((int));


/*--------------------*/
/* Aligned allocation */
/*--------------------*/

_PRIVATE  __ptr_t align __P ((int, __malloc_size_t));
_PRIVATE  __ptr_t align (int hdes, __malloc_size_t size)
{
  __ptr_t result;
  unsigned long int adj;

  result = (*__phmorecore) (hdes, size);
  adj = (unsigned long int) ((unsigned long int) ((char *) result -
						  (char *) NULL)) % BLOCKSIZE;
  if (adj != 0)
  {
      adj = BLOCKSIZE - adj;
      (void) (*__phmorecore) (hdes, adj);
      result = (char *) result + adj;
  }

  if(__after_phmorecore_hook)
     (*__after_phmorecore_hook) ();

  return result;
}


/*------------------------------------------------------*/
/* Heap initialisation - called by heap_attach function */
/*------------------------------------------------------*/

_PUBLIC int initialize_heap __P ((int));
_PUBLIC int initialize_heap (int hdes)
{    
     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_lock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */


     /*---------------------*/
     /* Is heap initialised */
     /*---------------------*/

     if (!initialize (hdes))
     {
        #ifdef PTHREAD_SUPPORT
        (void)pthread_mutex_unlock(&htab_mutex);
        #endif /* PTHREAD_SUPPORT */

        return (-1);
     }


     /*-------------------------*/
     /* Heap is not initialised */
     /*-------------------------*/

     else if(htable[hdes].exists == FALSE)
     {  int i;


        /*----------------------------*/
        /* Create mapped object table */
        /*----------------------------*/

        _phobjectmap[hdes]   = (phobmap_type **)phmalloc(hdes,PHOBMAP_QUANTUM*sizeof(phobmap_type **),
                                                                                         (char *)NULL);


        /*---------------------------------------------------------*/
        /* Initialise the first set of entries in the object table */
        /*---------------------------------------------------------*/

        for(i=0; i<PHOBMAP_QUANTUM; ++i)
           _phobjectmap[hdes][i] = (phobmap_type *)NULL;


        /*---------------------------------------------*/
        /* Heap root is always first persistent object */
        /*---------------------------------------------*/

        (void)msm_map_object(hdes,0,(void *)htable[hdes].sdata,htable[hdes].name);
        (void)msm_map_setinfo(hdes,0,"persistent heap root");
        (void)msm_map_setsize(hdes,0,htable[hdes].segment_size);


        /*--------------------------------------------------------*/
        /* Messy - but we must adjust the start and end of heap   */
        /* pointers to prepare for the first msm_isync_heaptables */
        /*--------------------------------------------------------*/

        htable[hdes].addresses_local = TRUE;
        (void)msm_sync_heaptables(hdes);
     }

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_unlock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */

     return 0;
}



/*----------------------------------------------*/
/* Set everything up and remember that we have. */
/*----------------------------------------------*/

_PRIVATE int initialize (int hdes)
{

  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_lock(&htab_mutex);
  #endif /* PTHREAD_SUPPORT */

  /*------------------------------------------------------------------------*/
  /* If we are importing a persistent heap which already exists             */
  /* we need to pull all of the parameters required by the                  */
  /* memory allocator from the shared heap. Mark O'Neill 14th November 1997 */
  /*------------------------------------------------------------------------*/

  if(htable[hdes].exists == FALSE)
  {

     /*-------------------------------------------------------------------*/
     /* Before doing anything else grab some memory on the shared heap to */
     /* store parameters. Mark O'Neill 14th November 1997                 */
     /*-------------------------------------------------------------------*/

     _pheap_parameters[hdes] = msm_sbrk (hdes, N_PARAMETERS*sizeof(long));

     if (__malloc_initialize_hook)
        (*__malloc_initialize_hook) (hdes);

     pheapsize[hdes] = HEAP / BLOCKSIZE;
     _pheapinfo[hdes] = (malloc_info *) align (hdes, pheapsize[hdes] * sizeof (malloc_info));
     if (_pheapinfo[hdes] == NULL)
     {
         #ifdef PTHREAD_SUPPORT
         (void)pthread_mutex_unlock(&htab_mutex);
         #endif /* PTHREAD_SUPPORT */

         return 0;
     }

     (void)memset (_pheapinfo[hdes], 0, pheapsize[hdes] * sizeof (malloc_info));

     _pheapinfo[hdes][0].free.size = 0;
     _pheapinfo[hdes][0].free.next = _pheapinfo[hdes][0].free.prev = 0;
     _pheapindex[hdes]             = 0;
     _pheapbase[hdes]              = (char *) _pheapinfo[hdes];


     /*-----------------------------------------------------------*/
     /* Account for the _pheapinfo block itself in the statistics */
     /*-----------------------------------------------------------*/

     _pheap_bytes_used[hdes]  = pheapsize[hdes] * sizeof (malloc_info) + 8*sizeof(int);
     _pheap_chunks_used[hdes] = 1;

     _phobjects[hdes]           = 0;
     _phobjects_allocated[hdes] = PHOBMAP_QUANTUM;


     /*--------------------------------------------------------*/
     /* Messy - but we must adjust the start and end of heap   */
     /* pointers to prepare for the first msm_isync_heaptables */
     /*--------------------------------------------------------*/

     //htable[hdes].addresses_local = TRUE;     
     //(void)msm_sync_heaptables(hdes);
   }



   /*---------------------------------------------------------*/
   /* Simply restore existing parameters from persistent heap */
   /* heap. Mark O'Neill, 14th November 1997                  */
   /*---------------------------------------------------------*/

   else
   { 

      /*---------------------------------------------------------------------*/
      /* We could have a problem here - if the heap has been extended        */
      /* (and is bigger than PHM_SBRK_SIZE bytes) we will need to munmap it  */
      /* and then remap it (possibly moving heap to a new location in the    */
      /* process address space                                               */
      /*---------------------------------------------------------------------*/

      _pheap_parameters[hdes] = (void *)((unsigned long)htable[hdes].addr + sizeof(long));
      htable[hdes].segment_size   = _pheap_parameters[hdes][17];
      if(htable[hdes].segment_size > PHM_SBRK_SIZE)
      {

         (void)msync((caddr_t)htable[hdes].addr,(size_t)PHM_SBRK_SIZE,MS_SYNC | MS_INVALIDATE); 

         (void)munmap((caddr_t)htable[hdes].addr,(size_t)PHM_SBRK_SIZE);
         htable[hdes].addr = (void *)mmap((caddr_t)htable[hdes].addr,
                                          (size_t)htable[hdes].segment_size,
                                          PROT_READ | PROT_WRITE,
                                          MAP_SHARED,
                                          htable[hdes].fd,
                                          (off_t)0);


         /*------------------------------------*/
         /* Make addresses local if nessessary */
         /*------------------------------------*/

         if(!htable[hdes].addresses_local)
            (void)msm_isync_heaptables(hdes);

#ifdef PHMALLOC_DEBUG
         (void)fprintf(stderr,"initialize: persistent heap %d dynamically remapped\n",hdes);
         (void)fflush(stderr);
#endif /* PHMALLOC_DEBUG */

      }
      else
      {

         /*------------------------------------*/
         /* Make addresses local if nessessary */
         /*------------------------------------*/

         if(!htable[hdes].addresses_local)
            (void)msm_isync_heaptables(hdes);
      }
   }

   __phmalloc_initialized[hdes] = 1;

   #ifdef PTHREAD_SUPPORT
   (void)pthread_mutex_unlock(&htab_mutex);
   #endif /* PTHREAD_SUPPORT */

   return 1;
}


/*--------------------------------------------*/
/* Get neatly aligned memory, initializing or */
/* growing the heap info table as necessary.  */
/*--------------------------------------------*/

_PRIVATE __ptr_t phmorecore __P ((int, __malloc_size_t));
_PRIVATE __ptr_t phmorecore (int hdes, __malloc_size_t size)
{
  __ptr_t result;

  malloc_info *newinfo = (malloc_info *)NULL,
              *oldinfo = (malloc_info *)NULL;

  __malloc_size_t newsize;

  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_lock(&htab_mutex);
  #endif /* PTHREAD_SUPPORT */

  result = align (hdes, size);
  if (result == NULL)
  {
     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_unlock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */

     return (__ptr_t*)NULL;
  }

  /*-----------------------------------------*/
  /* Check if we need to grow the info table */
  /*-----------------------------------------*/

  if ((__malloc_size_t) BLOCK (hdes, (char *) result + size) > pheapsize[hdes])
  {
      newsize = pheapsize[hdes];
      while ((__malloc_size_t) BLOCK (hdes, (char *) result + size) > newsize)
      { 

#ifdef PHMALLOC_DEBUG
        (void)fprintf(stderr,"BLOCK: %0x%010x\n",(__malloc_size_t)BLOCK (hdes, (char *) result + size));
        (void)fflush(stderr);
#endif /* PHMALLOC_DEBUG */

	newsize *= 2;
      }

      newinfo = (malloc_info *) align (hdes, newsize * sizeof (malloc_info));
      if (newinfo == NULL)
      {
	  (*__phmorecore) (hdes, -size);

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&htab_mutex);
          #endif /* PTHREAD_SUPPORT */

	  return (__ptr_t*)NULL;
      }

      (void)memcpy (newinfo, _pheapinfo[hdes], pheapsize[hdes] * sizeof (malloc_info));
      (void)memset (&newinfo[pheapsize[hdes]], 0, (newsize - pheapsize[hdes]) * sizeof (malloc_info));

      oldinfo = _pheapinfo[hdes];
      newinfo[BLOCK (hdes, oldinfo)].busy.type = 0;
      newinfo[BLOCK (hdes, oldinfo)].busy.info.size = BLOCKIFY (pheapsize[hdes] * sizeof (malloc_info));
      _pheapinfo[hdes] = newinfo;


      /*-----------------------------------------------------------*/
      /* Account for the _pheapinfo block itself in the statistics */
      /*-----------------------------------------------------------*/

      _pheap_bytes_used[hdes] += newsize * sizeof (malloc_info);
      ++_pheap_chunks_used[hdes];
      _phfree_internal (hdes, oldinfo);
      pheapsize[hdes] = newsize;
    }


  _pheaplimit[hdes] = BLOCK (hdes, (char *) result + size);

#ifdef PHMALLOC_DEBUG
  (void)fprintf(stderr,"MORECORE BLOCK: %d (%d)\n",_pheaplimit[hdes],BLOCK (hdes, (char *)result)); 
  (void)fflush(stderr); 
#endif /* PHMALLOC_DEBUG */
 
  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_unlock(&htab_mutex);
  #endif /* PTHREAD_SUPPORT */

  return result;
}


/*-----------------------------------------*/
/* Allocate memory from (persistent) heap. */
/*-----------------------------------------*/

_PUBLIC __ptr_t phmalloc (const unsigned int hdes, __malloc_size_t  size, const char *name)
{ int             h_index;
  __ptr_t         result;
  __malloc_size_t block, blocks, lastblocks, start, req_size, i;
  struct list     *next = (struct list *)NULL;

  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_lock(&htab_mutex);
  #endif /* PTHREAD_SUPPORT */


  /*--------------------------------------------------------------------------------------*/
  /* Before we do anything else, check whether we have a valid persistent heap descriptor */
  /*--------------------------------------------------------------------------------------*/

  if(hdes > appl_max_pheaps || htable[hdes].addr == (void *)NULL)
  {  errno = EACCES;

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_unlock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */

     return (__ptr_t*)NULL;
  }


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


  /*------------------------------------------------------------------*/
  /* ANSI C allows `malloc (0)' to either return NULL, or to return a */
  /* valid address you can realloc and free (though not dereference). */
  /*------------------------------------------------------------------*/

#if	0
  if (size == 0)
  {
     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_unlock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */

     return (__ptr_t*)NULL;
  }
#endif /* 0 */


  if (__phmalloc_hook != NULL)
  {  result = (*__phmalloc_hook) (hdes, size, name);

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_unlock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */

     return result;
  }

  req_size = size;


  /*----------------------------------------------------------------------*/
  /* Does this shared object already exits? If so, we cannot allocate it! */
  /*----------------------------------------------------------------------*/

  if(name != (const char *)NULL && msm_phobject_exists(hdes,name))
  {  errno = EEXIST;

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_unlock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */

     return((__ptr_t *)NULL);
  }

   if (size < sizeof (struct list))
      size = sizeof (struct list);


  /*-----------------------------------------------------------*/
  /* Determine the allocation policy based on the request size */
  /*-----------------------------------------------------------*/

  if (size <= BLOCKSIZE / 2)
  {

      /*-----------------------------------------------------------*/
      /* Small allocation to receive a fragment of a block.        */
      /* Determine the logarithm to base two of the fragment size. */
      /*-----------------------------------------------------------*/

      register __malloc_size_t log = 1;

      --size;
      while ((size /= 2) != 0)
	++log;


      /*------------------------------------*/
      /* Look in the fragment lists for a   */
      /* free fragment of the desired size. */
      /*------------------------------------*/

      next = _phfraghead[hdes][log].next;
      if (next != NULL)
      {

          /*--------------------------------------------------------*/
	  /* There are free fragments of this size.                 */
	  /* Pop a fragment out of the fragment list and return it. */
	  /* Update the block's nfree and first counters.           */
          /*--------------------------------------------------------*/

	  result = (__ptr_t) next;
	  next->prev->next = next->next;

	  if (next->next != NULL)
	    next->next->prev = next->prev;

	  block = BLOCK (hdes, result);
	  if (--_pheapinfo[hdes][block].busy.info.frag.nfree != 0)
	    _pheapinfo[hdes][block].busy.info.frag.first = (unsigned long int)
	                                                   ((unsigned long int) ((char *) next->next - (char *) NULL)
	                                                                                          % BLOCKSIZE) >> log;


          /*-----------------------*/
	  /* Update the statistics */
          /*-----------------------*/

	  ++_pheap_chunks_used[hdes];
	  _pheap_bytes_used[hdes] += 1 << log;
	  --_pheap_chunks_free[hdes];
	  _pheap_bytes_free[hdes] -= 1 << log;
      }
      else
      {

          /*-----------------------------------------------------------*/
	  /* No free fragments of the desired size, so get a new block */
	  /* and break it into fragments, returning the first.         */
          /*-----------------------------------------------------------*/

          if(_no_phobject_mapping == 0)
          {  _no_phobject_mapping = 1;
	     result = phmalloc (hdes, BLOCKSIZE, (char *)NULL);
             _no_phobject_mapping = 0;
          }
          else
             result = phmalloc (hdes, BLOCKSIZE, (char *)NULL);

	  if (result == NULL)
          {
             #ifdef PTHREAD_SUPPORT
             (void)pthread_mutex_unlock(&htab_mutex);
             #endif /* PTHREAD_SUPPORT */

	     return NULL;
          }


          /*-----------------------------------------------------*/
	  /* Link all fragments but the first into the free list */
          /*-----------------------------------------------------*/

	  for (i = 1; i < (__malloc_size_t) (BLOCKSIZE >> log); ++i)
	  {
	      next = (struct list *) ((char *) result + (i << log));
	      next->next = _phfraghead[hdes][log].next;
	      next->prev = &_phfraghead[hdes][log];
	      next->prev->next = next;
	      if (next->next != NULL)
		next->next->prev = next;
	  }


          /*--------------------------------------------------------*/
	  /* Initialize the nfree and first counters for this block */
          /*--------------------------------------------------------*/

	  block = BLOCK (hdes, result);
	  _pheapinfo[hdes][block].busy.type = log;
	  _pheapinfo[hdes][block].busy.info.frag.nfree = i - 1;
	  _pheapinfo[hdes][block].busy.info.frag.first = i - 1;

	  _pheap_chunks_free[hdes] += (BLOCKSIZE >> log) - 1;
	  _pheap_bytes_free[hdes]  += BLOCKSIZE - (1 << log);
	  _pheap_bytes_used[hdes]  -= BLOCKSIZE - (1 << log);
	}
  }
  else
  {

      /*----------------------------------------------------------------------*/
      /* Large allocation to receive one or more blocks.                      */
      /* Search the free list in a circle starting at the last place visited. */
      /* If we loop completely around without finding a large enough          */
      /* space we will have to get more memory from the system.               */
      /*----------------------------------------------------------------------*/

      blocks = BLOCKIFY (size);
      start = block = _pheapindex[hdes];
      while (_pheapinfo[hdes][block].free.size < blocks)
      {
	  block = _pheapinfo[hdes][block].free.next;
	  if (block == start)
          {

              /*------------------------------------------------------*/
	      /* Need to get more from the system.  Check to see if   */
              /* the new core will be contiguous with the final free  */
              /* block; if so we don't need to get as much.           */
              /*------------------------------------------------------*/

	      block = _pheapinfo[hdes][0].free.prev;
	      lastblocks = _pheapinfo[hdes][block].free.size;
	      if (_pheaplimit[hdes] != 0 && block + lastblocks == _pheaplimit[hdes]         &&
		  (*__phmorecore) (hdes,0) == ADDRESS (hdes, block + lastblocks)            &&
		  (phmorecore (hdes, (blocks - lastblocks) * BLOCKSIZE))              != NULL)
              {

                  /*--------------------------------------------------*/
 		  /* Which block we are extending (the `final free    */
 		  /* block' referred to above) might have changed, if */
                  /* it got combined with a freed info table.         */
                  /*--------------------------------------------------*/

 		  block = _pheapinfo[hdes][0].free.prev;
  		  _pheapinfo[hdes][block].free.size += (blocks - lastblocks);
		  _pheap_bytes_free[hdes] += (blocks - lastblocks) * BLOCKSIZE;
		  continue;
              }

	      result = phmorecore (hdes, blocks * BLOCKSIZE);
	      if (result == NULL)
              {
                 #ifdef PTHREAD_SUPPORT
                 (void)pthread_mutex_unlock(&htab_mutex);
                 #endif /* PTHREAD_SUPPORT */

		 return NULL;
              }

	      block = BLOCK (hdes, result);
	      _pheapinfo[hdes][block].busy.type = 0;
	      _pheapinfo[hdes][block].busy.info.size = blocks;
	      ++_pheap_chunks_used[hdes];
	      _pheap_bytes_used[hdes] += blocks * BLOCKSIZE;

              if(_no_phobject_mapping == 0 && name != (void *)NULL)
              {  h_index = msm_get_free_mapslot(hdes);
                 (void)msm_map_object(hdes,h_index,result,name);
                 (void)msm_map_setinfo(hdes,h_index,"persistent GMAP object");
                 (void)msm_map_setsize(hdes,h_index,req_size);
                 _pheap_parameters[hdes][8]     = _phobjects[hdes];
                 _pheap_parameters[hdes][9]     = _phobjects_allocated[hdes];
              }

              #ifdef PTHREAD_SUPPORT
              (void)pthread_mutex_unlock(&htab_mutex);
              #endif /* PTHREAD_SUPPORT */

	      return result;
           }
	}


      /*---------------------------------------------------------*/
      /* At this point we have found a suitable free list entry. */
      /* Figure out how to remove what we need from the list.    */
      /*---------------------------------------------------------*/

      result = ADDRESS (hdes, block);
      if (_pheapinfo[hdes][block].free.size > blocks)
      {

          /*-------------------------------------------------*/
	  /* The block we found has a bit left over,         */
	  /* so relink the tail end back into the free list. */
          /*-------------------------------------------------*/

	  _pheapinfo[hdes][block + blocks].free.size                     = _pheapinfo[hdes][block].free.size - blocks;
	  _pheapinfo[hdes][block + blocks].free.next                     = _pheapinfo[hdes][block].free.next;
	  _pheapinfo[hdes][block + blocks].free.prev                     = _pheapinfo[hdes][block].free.prev;
	  _pheapinfo[hdes][_pheapinfo[hdes][block].free.prev].free.next = _pheapindex[hdes] = block + blocks;
          _pheapinfo[hdes][_pheapinfo[hdes][block].free.next].free.prev = _pheapindex[hdes] = block + blocks;
      }
      else
      {

          /*---------------------------------------------*/
	  /* The block exactly matches our requirements, */
	  /* so just remove it from the list.            */
          /*---------------------------------------------*/

	  _pheapinfo[hdes][_pheapinfo[hdes][block].free.next].free.prev = _pheapinfo[hdes][block].free.prev;
	  _pheapinfo[hdes][_pheapinfo[hdes][block].free.prev].free.next = _pheapindex[hdes] = _pheapinfo[hdes][block].free.next;
	  --_pheap_chunks_free[hdes];
	}

      _pheapinfo[hdes][block].busy.type = 0;
      _pheapinfo[hdes][block].busy.info.size = blocks;

      ++_pheap_chunks_used[hdes];

      _pheap_bytes_used[hdes] += blocks * BLOCKSIZE;
      _pheap_bytes_free[hdes] -= blocks * BLOCKSIZE;


      /*-----------------------------------------------------------------*/
      /* Mark all the blocks of the object just allocated except for the */
      /* first with a negative number so you can find the first block by */
      /* adding that adjustment.                                         */
      /*-----------------------------------------------------------------*/

      while(--blocks > 0)
	_pheapinfo[hdes][block + blocks].busy.info.size = -blocks;
  }

  if(_no_phobject_mapping == 0 && name != (char *)NULL)
  {  h_index         = msm_get_free_mapslot(hdes);

     (void)msm_map_object (hdes,h_index,result,name);
     (void)msm_map_setinfo(hdes,h_index,"persistent GMAP object");
     (void)msm_map_setsize(hdes,h_index,req_size);

     _pheap_parameters[hdes][8] = _phobjects[hdes];
  }

  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_unlock(&htab_mutex);
  #endif /* PTHREAD_SUPPORT */

  return result;
}
