/*--------------------------------------------------------------------------------------
   Copyright 1990, 1991, 1992, 1993, 1995 Free Software Foundation, Inc.
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

   Shared heap modifications by Mark O'Neill, Tumbling DIce  <mao@tumblingdice.co.uk>
   (C) M.A. O'Neill. Tumbling Dice 2005-2024
--------------------------------------------------------------------------------------*/

#ifndef _PHMALLOC_H
#define _PHMALLOC_H	1

#ifdef _PHMALLOC_INTERNAL
#include <pheap.h>

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#if defined(_LIBC) || defined(STDC_HEADERS)
#include <string.h>

#else 

#ifndef memset
#define	memset(s, zero, n)	bzero ((s), (n))
#endif /* memset */

#ifndef memcpy
#define	memcpy(d, s, n)		bcopy ((s), (d), (n))

       /*--------*/
#endif /* memcpy */
       /*--------*

       /*--------*/
#endif /* memset */
       /*--------*/



#if defined (__GNU_LIBRARY__) || (defined (__STDC__) && __STDC__)
#include <limits.h>
#else
#ifndef CHAR_BIT
#define	CHAR_BIT	8

       /*----------*/
#endif /* CHAR_BIT */
       /*----------*/

       /*------------------------------------------------------*/
#endif /* __GNU_LIBRARY__) || (defined (__STDC__) && __STDC__) */
       /*------------------------------------------------------*/

#ifdef	HAVE_UNISTD_H
#include <unistd.h>

       /*---------------*/
#endif /* HAVE_UNISTD_H */
       /*---------------*/

       /*------------------*/
#endif /* _MALLOC_INTERNAL */
       /*------------------*/


#ifdef	__cplusplus
extern "C"
{

       /*-------------*/
#endif /* __cplusplus */
       /*-------------*/

#if defined (__cplusplus) || (defined (__STDC__) && __STDC__)
#undef	__P
#define	__P(args)	args
#undef	__ptr_t
#define	__ptr_t		void *
#else /* Not C++ or ANSI C.  */
#undef	__P
#define	__P(args)	()
#undef	const
#define	const
#undef	__ptr_t
#define	__ptr_t		char *
#endif /* C++ or ANSI C.  */

#if defined (__STDC__) && __STDC__
#include <stddef.h>
#define	__malloc_size_t		size_t
#define	__malloc_ptrdiff_t	ptrdiff_t
#else
#define	__malloc_size_t		unsigned long 
#define	__malloc_ptrdiff_t	long	
#endif

#ifndef	NULL
#define	NULL	0
#endif


/*--------------------------------*/
/* Allocate SIZE bytes of memory. */
/*--------------------------------*/

extern __ptr_t phmalloc __P ((const uint32_t, __malloc_size_t __size, const char *));

/*--------------------------------------------------*/
/* Re-allocate the previously allocated block       */
/* in __ptr_t, making the new block SIZE bytes long */
/*--------------------------------------------------*/

extern __ptr_t phrealloc __P ((const uint32_t, const __ptr_t __ptr, __malloc_size_t __size, const char *));


/*------------------------------------------------------------------*/
/* Allocate NMEMB elements of SIZE bytes each, all initialized to 0 */
/*------------------------------------------------------------------*/

extern __ptr_t phcalloc __P ((const uint32_t, const __malloc_size_t __nmemb, const __malloc_size_t __size, const char *));


/*-----------------------------------------------------------*/
/* Free a block allocated by `malloc', `realloc' or `calloc' */
/*-----------------------------------------------------------*/

extern void *phfree __P ((const uint32_t, const __ptr_t __ptr));


/*--------------------------------------------------*/
/* Allocate SIZE bytes allocated to ALIGNMENT bytes */
/*--------------------------------------------------*/

extern __ptr_t phmemalign __P ((const uint32_t, const __malloc_size_t __alignment, __malloc_size_t __size, const char *));


/*----------------------------------------*/
/* Allocate SIZE bytes on a page boundary */
/*----------------------------------------*/

extern __ptr_t phvalloc __P ((const uint32_t, __malloc_size_t __size, const char *));


#ifdef _PHMALLOC_INTERNAL


/*-------------------------------------------------------------------*/
/* The allocator divides the heap  int32_to blocks of fixed size; large   */
/* requests receive one or more whole blocks, and small requests     */
/* receive a fragment of a block.  Fragment sizes are powers of two  */
/* and all fragments of a block are the same size.  When all the     */
/* fragments in a block have been freed, the block itself is freed.  */
/*-------------------------------------------------------------------*/


#define INT_BIT		(CHAR_BIT * sizeof(int))
#define BLOCKLOG	(INT_BIT > 16 ? 12 : 9)
#define BLOCKSIZE	(1 << BLOCKLOG)
#define BLOCKIFY(SIZE)	(((SIZE) + BLOCKSIZE - 1) / BLOCKSIZE)


/*------------------------------------------------------------------*/
/* Determine the amount of memory spanned by the initial heap table */
/* (not an absolute limit)                                          */
/*------------------------------------------------------------------*/

#define HEAP		(INT_BIT > 16 ? 4194304 : 65536)


/*--------------------------------------------------------------------*/
/* Number of contiguous free blocks allowed to build up at the end of */
/* memory before they will be returned to the system                  */
/*--------------------------------------------------------------------*/

#define FINAL_FREE_BLOCKS	8


/*---------------------------------------------*/
/* Data structure giving per-block information */
/*---------------------------------------------*/

typedef union
  {


    /*-------------------------------------*/
    /* Heap information for a busy block.  */
    /*-------------------------------------*/

    struct
      {

        /*--------------------------------------------------------------*/
	/* Zero for a large (multiblock) object, or positive giving the */
	/* logarithm to the base two of the fragment size               */
        /*--------------------------------------------------------------*/

	int type;
	union
	  {
	    struct
	      {
		__malloc_size_t nfree; // Free frags in a fragmented block
		__malloc_size_t first; // First free fragment of the block
	      } frag;

            /*-------------------------------------------------------------*/
	    /* For a large object, in its first block, this has the number */
	    /* of blocks in the object.  In the other blocks, this has a   */
	    /* negative number which says how far back the first block is  */
            /*-------------------------------------------------------------*/

	    __malloc_ptrdiff_t size;
	  } info;
      } busy;


    /*-------------------------------------------*/
    /* Heap information for a free block         */
    /* (that may be the first of a free cluster) */
    /*-------------------------------------------*/

    struct
      {
	__malloc_size_t size;	// Size (in blocks) of a free cluster
	__malloc_size_t next;	// Index of next free cluster
	__malloc_size_t prev;	// Index of previous free cluster
      } free;
  } malloc_info;


/*------------------------------------------*/
/* Pointer to persistent heap object tables */
/*------------------------------------------*/

extern phobmap_type ***_shobjectmap;


/*----------------------------------------------------*/
/* Pointer to heap parameter table on persistent heap */
/*----------------------------------------------------*/

extern  int32_t **pheap_parameters;


/*------------------------------------*/
/* Pointer to first block of the heap */
/*------------------------------------*/

extern char **_pheapbase;


/*------------------------------------------------------------*/
/* Table indexed by block number giving per-block information */
/*------------------------------------------------------------*/

extern malloc_info **_pheapinfo;


/*----------------------------------------*/
/* Address to block number and vice versa */
/*----------------------------------------*/

#define BLOCK(H, A)	(((char *) (A) - _pheapbase[H]) / BLOCKSIZE + 1)
#define ADDRESS(H, B)	((__ptr_t) (((B) - 1) * BLOCKSIZE + _pheapbase[H]))


/*-----------------------------------------*/
/* Current search index for the heap table */
/*-----------------------------------------*/

extern __malloc_size_t *_pheapindex;


/*-----------------------------------*/
/* Limit of valid info table indices */
/*-----------------------------------*/

extern __malloc_size_t *_pheaplimit;


/*---------------------------------------*/
/* Doubly linked lists of free fragments */
/*---------------------------------------*/

struct list
  {
    struct list *next;

    /*-----------------------------------------------------------------*/
    /* Spill for upper 4 bytes of pointer allows 32/64 bit compability */
    /*-----------------------------------------------------------------*/

    void        *spill_next;
    struct list *prev;

    /*-----------------------------------------------------------------*/
    /* Spill for upper 4 bytes of pointer allows 32/64 bit compability */
    /*-----------------------------------------------------------------*/

    void        *spill_prev;

  };


/*------------------------------------------*/
/* Free list headers for each fragment size */
/*------------------------------------------*/

extern struct list **_phfraghead;


/*--------------------------------------------------------*/
/* List of blocks allocated with `memalign' (or `valloc') */
/*--------------------------------------------------------*/

struct alignlist
  {
    struct alignlist *next;


    /*-----------------------------------------------------------------*/
    /* Spill for upper 4 bytes of pointer allows 32/64 bit compability */
    /*-----------------------------------------------------------------*/

    void    *spill_next;       /*--------------------------------------*/
    __ptr_t aligned;           /* The address that memaligned returned */
                               /*--------------------------------------*/


    /*-----------------------------------------------------------------*/
    /* Spill for upper 4 bytes of pointer allows 32/64 bit compability */
    /*-----------------------------------------------------------------*/

    void        *spill_aligned;    /*----------------------------------*/
    __ptr_t     exact;             /* The address that malloc returned */
                                   /*----------------------------------*/


    /*-----------------------------------------------------------------*/
    /* Spill for upper 4 bytes of pointer allows 32/64 bit compability */
    /*-----------------------------------------------------------------*/

    void        *spill_exact;

  };

extern struct alignlist *_aligned_blocks;


/*-----------------*/
/* Instrumentation */
/*-----------------*/

extern __malloc_size_t *_pheap_chunks_used;
extern __malloc_size_t *_pheap_bytes_used;
extern __malloc_size_t *_pheap_chunks_free;
extern __malloc_size_t *_pheap_bytes_free;

/*-------------------------------------------------------------*/
/* Internal version of `free' used in `shmorecore' (malloc.c). */
/*-------------------------------------------------------------*/

extern void _phfree_internal __P ((const uint32_t   , const __ptr_t __ptr));

#endif /* _MALLOC_INTERNAL.  */


/*-----------------------------------------------------*/
/* Given an address in the middle of a malloc'd object */
/* return the address of the beginning of the object   */
/*-----------------------------------------------------*/

extern __ptr_t phmalloc_find_object_address __P ((int, __ptr_t __ptr));


/*---------------------------------------------------------*/
/* Underlying allocation function; successive calls should */
/* return contiguous pieces of memory                      */
/*---------------------------------------------------------*/

extern __ptr_t (*__phmorecore) __P ((int, __malloc_ptrdiff_t __size));


/*---------------------------------*/
/* Default value of `__phmorecore' */
/*---------------------------------*/

extern __ptr_t __default_phmorecore __P ((int, __malloc_ptrdiff_t __size));


/*------------------------------------------------------*/
/* If not NULL, this function is called after each time */
/* `__shmorecore' is called to increase the data size.  */
/*------------------------------------------------------*/

extern void (*__after_phmorecore_hook) __P ((void));


/*-----------------------------------------------------------------*/
/* Nonzero if `malloc' has been called and done its initialization */
/*-----------------------------------------------------------------*/

extern  int32_t *__phmalloc_initialized;


/*------------------------------*/
/* Hooks for debugging versions */
/*------------------------------*/

extern void (*__phmalloc_initialize_hook) __P ((int));
extern void (*__phfree_hook) __P ((const uint32_t   , const __ptr_t __ptr));
extern __ptr_t (*__phmalloc_hook) __P ((const uint32_t   , __malloc_size_t __size, const char *));
extern __ptr_t (*__phrealloc_hook) __P ((const uint32_t   , const __ptr_t __ptr, __malloc_size_t __size, const char *));
extern __ptr_t (*__phmemalign_hook) __P ((const uint32_t   , __malloc_size_t __size, const __malloc_size_t __alignment, const char *));


/*-------------------------------------------------------------------------*/
/* Return values for `mprobe': these are the kinds of inconsistencies that */
/* `mcheck' enables detection of                                           */
/*-------------------------------------------------------------------------*/

enum mcheck_status
  {

                                /*-----------------------------------------*/
    MCHECK_DISABLED = -1,	/* Consistency checking is not turned on.  */
    MCHECK_OK,			/* Block is fine                           */
    MCHECK_FREE,		/* Block freed twice                       */
    MCHECK_HEAD,		/* Memory before the block was clobbered.  */
    MCHECK_TAIL			/* Memory after the block was clobbered.   */
                                /*-----------------------------------------*/

  };


/*-------------------------------------------------------------------------*/
/* Activate a standard collection of debugging hooks.  This must be called */
/* before `malloc' is ever called.  ABORTFUNC is called with an error code */
/* (see enum above) when an inconsistency is detected.  If ABORTFUNC is    */
/* null, the standard function prints on stderr and then calls `abort'     */
/*-------------------------------------------------------------------------*/

extern  int32_t mcheck __P ((void (*__abortfunc) __P ((enum mcheck_status))));


/*-------------------------------------------------------------------------*/
/* Check for aberrations in a particular malloc'd block.  You must have    */
/* called `mcheck' already.  These are the same checks that `mcheck' does  */
/* when you free or reallocate a block                                     */
/*-------------------------------------------------------------------------*/

extern enum mcheck_status mprobe __P ((__ptr_t __ptr));


/*-------------------------------------------------*/
/* Activate a standard collection of tracing hooks */
/*-------------------------------------------------*/

extern void mtrace __P ((void));
extern void muntrace __P ((void));


/*----------------------------------*/
/* Statistics available to the user */
/*----------------------------------*/

struct mstats
  {

                                 /*---------------------------------------*/
    __malloc_size_t bytes_total; /* Total size of the heap                */
    __malloc_size_t chunks_used; /* Chunks allocated by the user          */
    __malloc_size_t bytes_used;	 /* Byte total of user-allocated chunks   */
    __malloc_size_t chunks_free; /* Chunks in the free list               */
    __malloc_size_t bytes_free;	 /* Byte total of chunks in the free list */
                                 /*---------------------------------------*/

  };


/*--------------------------------*/
/* Pick up the current statistics */
/*--------------------------------*/

extern struct mstats phmstats __P ((int));


/*---------------------------------------------------------------*/
/* Call WARNFUN with a warning message when memory usage is high */
/*---------------------------------------------------------------*/

extern void memory_warnings __P ((__ptr_t __start,
				  void (*__warnfun) __P ((const char *))));



/*----------------------*/
/* Relocating allocator */
/*----------------------*/


/*-----------------------------------------------------------*/
/* Allocate SIZE bytes, and store the address in *HANDLEPTR  */
/*-----------------------------------------------------------*/

extern __ptr_t phr_alloc __P ((int, __ptr_t *__handleptr, __malloc_size_t __size));


/*-----------------------------------------*/
/* Free the storage allocated in HANDLEPTR */
/*-----------------------------------------*/

extern void phr_alloc_free __P ((int, __ptr_t *__handleptr));


/*-----------------------------------------------------*/
/* Adjust the block at HANDLEPTR to be SIZE bytes long */
/*-----------------------------------------------------*/

extern __ptr_t phr_re_alloc __P ((__ptr_t *__handleptr, __malloc_size_t __size));


#ifdef	__cplusplus
}

       /*------------*/
#endif /*__cplusplus */
       /*------------*/


       /*-----------*/
#endif /* malloc.h  */
       /*-----------*/

