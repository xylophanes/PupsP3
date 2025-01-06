/*-------------------------------------------------------
    Persistent heap library. A persistent heap is an area
    of data memory which may be mapped serailly  int32_to the
    address spaces of multiple process.

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

    Version: 2.03
    Dated:   11th December 2024 
    Email:   mao@tumblingdice.co.uk
-------------------------------------------------------*/

/*-------------------------------------------------------------*/
/* If we have OpenMP support then we also have pthread support */
/* (OpenMP is built on pthreads for Linux distributions).      */
/*-------------------------------------------------------------*/

#if defined(_OPENMP) && !defined(PTHREAD_SUPPORT)
#define PTHREAD_SUPPORT
#endif /* PTHREAD_SUPPORT */


#include <me.h>
#include <utils.h>
#include <psrp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pheap.h> 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netlib.h>
#include <casino.h>
#include <errno.h>
#include <bsd/bsd.h>

#ifdef PTHREAD_SUPPORT
#include <tad.h>
#endif /* PTHREAD_SUPPORT */

#ifndef _PHMALLOC_INTERNAL
#define _PHMALLOC_INTERNAL
#include <phmalloc.h>
#endif /* PHMALLOC_INTERNAL */

#include <errno.h>


/*----------------------------------------------------------------*/
/* Get signal mapping appropriate to OS and hardware architecture */
/*----------------------------------------------------------------*/

#define __DEFINE__
#if defined(I386) || defined(X86_64)
#include <sig.linux.x86.h>
#endif /* I386 || X86_64 */

#ifdef ARMV6L
#include <sig.linux.arm.h>
#endif /* ARMV6L */

#ifdef ARMV7L
#include <sig.linux.arm.h>
#endif /* ARMV7L */

#ifdef AARCH64
#include <sig.linux.arm.h>
#endif /* AARCH64 */
#undef __DEFINE__



/*--------------------------------------------------------------------*/
/* Heaptable objects used by the persistent memory allocation package */
/*--------------------------------------------------------------------*/

// Is persistent heap initialised
_PUBLIC int32_t *__phmalloc_initialized                            = (int *)NULL;

// Number of objects in heap
_PUBLIC int32_t *_phobjects                                        = (int *)NULL;
 
// Number of objects in heap
_PUBLIC int32_t *_phobjects_allocated                              = (int *)NULL;

// Pointer to table of significant objects on persistent heaps
_PUBLIC phobmap_type ***_phobjectmap                               = (phobmap_type ***)NULL;



/*-----------------------------------------------------*/
/* List of client_info structures for client processes */
/* using persistent heaps                              */
/*-----------------------------------------------------*/

// Pointer to persistent heap parameter table (on persistent heap)
_PUBLIC uint64_t          **_pheap_parameters                      = (uint64_t **)NULL;
 
// Pointer to the base of the first block
_PUBLIC char **_pheapbase                                          = (char **)NULL;
 
// Block information table.  Allocated with align/__free (not malloc/free)
_PUBLIC malloc_info **_pheapinfo                                   = (malloc_info **)NULL;
 
// Number of info entries
_PUBLIC __malloc_size_t *pheapsize                                 = (__malloc_size_t *)NULL;
 
// Search index in the info table.
_PUBLIC __malloc_size_t *_pheapindex                               = (__malloc_size_t *)NULL;
 
// Limit of valid info table indices
_PUBLIC __malloc_size_t *_pheaplimit                               = (__malloc_size_t *)NULL;
 
// Free lists for each fragment size
_PUBLIC struct list **_phfraghead                                  = (struct list **)NULL;
 
// Instrumentation
_PUBLIC __malloc_size_t *_pheap_chunks_used                        = (__malloc_size_t *)NULL;
_PUBLIC __malloc_size_t *_pheap_bytes_used                         = (__malloc_size_t *)NULL;
_PUBLIC __malloc_size_t *_pheap_chunks_free                        = (__malloc_size_t *)NULL;
_PUBLIC __malloc_size_t *_pheap_bytes_free                         = (__malloc_size_t *)NULL;


/*--------------------------------------*/
/* TRUE if persistent heaps initialized */
/*--------------------------------------*/

_PUBLIC _BOOLEAN        do_msm_init                                = FALSE;


#ifdef PTHREAD_SUPPORT
/*---------------------------------------*/
/* Heap table mutex to allow thread-safe */
/* manipulation of persistent heap       */
/*---------------------------------------*/

_PUBLIC pthread_mutex_t htab_mutex                                 = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
_PUBLIC pthread_mutex_t phmalloc_mutex                             = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif /* PTHREAD_SUPPORT */


_IMPORT _PROTOTYPE  int32_t initialize_heap(int);


/*-----------------------*/ 
/* Persistent heap table */
/*-----------------------*/

_PUBLIC heap_type    *htable = (heap_type *)NULL;




/*------------------------------------*/
/* Functions exported by this modules */
/*------------------------------------*/

// Switch off presistent object map updating 
_PUBLIC int32_t _no_phobject_mapping;




/*--------------------------------------------*/
/* Variables which are private to this module */
/*--------------------------------------------*/
/*-----------------------------------------------------*/
/* TRUE if persistent heap maps exists (e.g. if we are */
/* attaching an existing persistent heap)              */
/*-----------------------------------------------------*/

_PRIVATE _BOOLEAN _phmaps_exist = FALSE;



/*-------------------------------------------*/
/* Function which are private to this module */
/*-------------------------------------------*/

// Relocate heap addresses 
_PROTOTYPE _PRIVATE  int32_t msm_relocate_heap_addresses(int, size_t,  int32_t);




/*--------------------------------------------------------*/ 
/* Convert free block addressing from global to local (in */
/* cached heap)                                           */
/*--------------------------------------------------------*/ 

_PROTOTYPE _PRIVATE void local_to_global_blocklist(int32_t, size_t);


/*--------------------------------------------------------*/
/* Convert free block addressing from local to global (in */
/* cached heap)                                           */
/*--------------------------------------------------------*/

_PROTOTYPE _PRIVATE void global_to_local_blocklist(uint32_t, size_t);



/*-------------------------------------------------*/
/* Slot and usage functions - used by slot manager */
/*-------------------------------------------------*/
/*---------------------*/
/* Slot usage function */
/*---------------------*/

_PRIVATE void pheap_slot(int32_t level)
{   (void)fprintf(stderr,"lib hseaplib %s: [ANSI C]\n",PHEAP_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(C) 2003-2024 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"PUPS/P3 persistent heap support library (gcc %s: built %s %s)\n\n",__VERSION__,__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");
    fflush(stderr);
}




/*----------------------------------------------------*/
/* Segment identification for persistent heap library */
/*----------------------------------------------------*/

#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT )() __attribute__ ((aligned(16))) = pheap_slot;
#endif /* SLOT */





/*----------------------------------------------------------------------*/
/* Initialise (local process) variables associated with persistent heap */
/*----------------------------------------------------------------------*/

_PUBLIC int32_t msm_init(const  int32_t max_pheaps)

{   uint32_t i;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[msm_init] attempt by non root thread to perform PUPS/P3 persistent heap operation");

    /*----------------------------------------------*/
    /* max_pheaps <= 0 implies that the application */
    /* is not using persistent heaps                */
    /*----------------------------------------------*/

    if(max_pheaps > MAX_PHEAPS)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    /*------------------------------------*/
    /* Initialised persistent heap tables */
    /*------------------------------------*/

    htable = (heap_type *)pups_calloc(max_pheaps,sizeof(heap_type));
    for(i=0; i<max_pheaps; ++i)
    {  (void)strlcpy(htable[i].name,"",SSIZE);

       htable[i].fd              = (-1);
       htable[i].m_cnt           = 0;
       htable[i].mtime           = 0L;
       htable[i].sdata           = 0;
       htable[i].edata           = 0;
       htable[i].ptrsize         = 8*sizeof(void *);
       htable[i].addr            = (void *)NULL;
       htable[i].exists          = FALSE;
       htable[i].addresses_local = FALSE;


       /*-------------------------------------------------------*/
       /* Get size of pointers (for persistent heap addressing) */
       /*-------------------------------------------------------*/


       /*--------------*/
       /* 64 bit heaps */
       /*--------------*/

       if(sizeof(void *) == 8)
       {

          /*-------------------------*/
          /* Big endian architecture */
          /*-------------------------*/

          if(pups_bigendian() == TRUE)
             htable[i].heapmagic = HEAPMAGIC64BIG;
          else
             htable[i].heapmagic = HEAPMAGIC64LITTLE;
       }


       /*--------------*/
       /* 32 bit heaps */
       /*--------------*/

       else if(sizeof(void *) == 4)
       {

          /*----------------------------*/
          /* Little endian architecture */
          /*----------------------------*/

          if(pups_bigendian() == TRUE)
             htable[i].heapmagic       = HEAPMAGIC32BIG;
          else
             htable[i].heapmagic       = HEAPMAGIC32LITTLE;
       }


       /*-----------------*/
       /* Error condition */
       /*-----------------*/

       else
       {  pups_set_errno(EINVAL);

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&htab_mutex);
          #endif /* PTHREAD_SUPPORT */

          return(-1);
       }
    }


    /*-----------------------------------------------------------*/
    /* Initialise other per heap persistent heap datatstructures */
    /*-----------------------------------------------------------*/

    _phobjectmap         = (phobmap_type ***)pups_calloc(max_pheaps,sizeof(phobmap_type **));
    for(i=0; i<max_pheaps; ++i)
        _phobjectmap[i] = (phobmap_type **)NULL;

    _phobjects           = (int32_t *)pups_calloc(max_pheaps,sizeof(int32_t));
    _phobjects_allocated = (int32_t *)pups_calloc(max_pheaps,sizeof(int32_t));

    _pheap_parameters    = (uint64_t          **)pups_calloc(max_pheaps,sizeof(uint64_t *));
    _pheapinfo           = (malloc_info **)      pups_calloc(max_pheaps,sizeof(malloc_info *));
    _pheapbase           = (char **)             pups_calloc(max_pheaps,sizeof(char *));
    pheapsize            = (__malloc_size_t *)   pups_calloc(max_pheaps,sizeof(__malloc_size_t));
    _pheapindex          = (__malloc_size_t *)   pups_calloc(max_pheaps,sizeof(__malloc_size_t));
    _pheaplimit          = (__malloc_size_t *)   pups_calloc(max_pheaps,sizeof(__malloc_size_t));
    _phfraghead          = (struct list **)      pups_calloc(max_pheaps,sizeof(struct list *));

    for(i=0; i<max_pheaps; ++i)
       _phfraghead[i] = (struct list *)pups_calloc(BLOCKLOG,sizeof(struct list));

    _pheap_chunks_used         = (__malloc_size_t *)pups_calloc(max_pheaps,sizeof(__malloc_size_t));
    _pheap_bytes_used          = (__malloc_size_t *)pups_calloc(max_pheaps,sizeof(__malloc_size_t));
    _pheap_chunks_free         = (__malloc_size_t *)pups_calloc(max_pheaps,sizeof(__malloc_size_t));
    _pheap_bytes_free          = (__malloc_size_t *)pups_calloc(max_pheaps,sizeof(__malloc_size_t));
    __phmalloc_initialized     = (int32_t *)       pups_calloc(max_pheaps,sizeof(int32_t));


    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): persistent heap table initialised (%d heaps slots)\n",
                                             date,appl_name,appl_pid,appl_host,appl_owner,max_pheaps);
       (void)fflush(stderr);
    }



    /*-------------------------------------*/
    /* Flag persistent heap initialisation */
    /*-------------------------------------*/

    do_msm_init = TRUE;

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*------------------------------------------------------------------*/
/* Extend (local process) variables associated with persistent heap */
/*------------------------------------------------------------------*/

_PUBLIC int32_t msm_extend(const uint32_t from_size, uint32_t to_size) 

{   uint32_t i;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(to_size < from_size)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[msm_extend] attempt by non root thread to perform PUPS/P3 persistent heap operation");


    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */


    /*-------------------------------*/
    /* Extend persistent heap tables */
    /*-------------------------------*/

    htable = (heap_type *)pups_realloc((void *)htable,to_size*sizeof(heap_type));
    for(i=from_size; i<to_size; ++i)
    {  (void)strlcpy(htable[i].name,"",SSIZE);

       htable[i].fd              = (-1);
       htable[i].mtime           = 0L;
       htable[i].sdata           = 0;
       htable[i].edata           = 0;
       htable[i].ptrsize         = 8*sizeof(void *);
       htable[i].addr            = (void *)NULL;
       htable[i].exists          = FALSE;
       htable[i].addresses_local = FALSE;


       /*------------------------------------------------------*/
       /* Get size of pointers (for persistentheap addressing) */
       /*------------------------------------------------------*/


       /*--------------*/
       /* 64 bit heaps */
       /*--------------*/

       if(sizeof(void *) == 8)
       {

          /*-------------------------*/
          /* Big endian architecture */
          /*-------------------------*/

          if(pups_bigendian() == TRUE)
             htable[i].heapmagic = HEAPMAGIC64BIG;
          else
             htable[i].heapmagic = HEAPMAGIC64LITTLE;
       }


       /*--------------*/
       /* 32 bit heaps */
       /*--------------*/

       else if(sizeof(void *) == 4)
       {

          /*----------------------------*/
          /* Little endian architecture */
          /*----------------------------*/

          if(pups_bigendian() == TRUE)
             htable[i].heapmagic       = HEAPMAGIC32BIG;
          else
             htable[i].heapmagic       = HEAPMAGIC32LITTLE;
       }


       /*-----------------*/
       /* Error condition */
       /*-----------------*/

       else
       {  pups_set_errno(EINVAL);

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&htab_mutex);
          #endif /* PTHREAD_SUPPORT */

          return(-1);
       }
    }


    /*-----------------------------------------------------------*/
    /* Initialise other per heap persistent heap datatstructures */
    /*-----------------------------------------------------------*/

    _phobjectmap               = (phobmap_type ***)pups_realloc((void *)_phobjectmap,to_size*sizeof(phobmap_type **));
    for(i=from_size; i<to_size; ++i)
        _phobjectmap[i] = (phobmap_type **)NULL;

    _phobjects                 = (int32_t *)pups_realloc((void *)_phobjects,to_size*sizeof(int32_t));
    _phobjects_allocated       = (int32_t *)pups_realloc((void *)_phobjects_allocated,to_size*sizeof(int32_t));

    _pheap_parameters          = (uint64_t    **)pups_realloc((void *)_pheap_parameters,to_size*sizeof(uint64_t *));
    _pheapinfo                 = (malloc_info **)pups_realloc((void *)_pheapinfo,to_size*sizeof(malloc_info *));
    _pheapbase                 = (char **)pups_realloc((void *)_pheapbase,to_size*sizeof(char *));
    pheapsize                  = (__malloc_size_t *)pups_realloc((void *)pheapsize,to_size*sizeof(__malloc_size_t));
    _pheapindex                = (__malloc_size_t *)pups_realloc((void *)_pheapindex,to_size*sizeof(__malloc_size_t));
    _pheaplimit                = (__malloc_size_t *)pups_realloc((void *)_pheapindex,to_size*sizeof(__malloc_size_t));

    _phfraghead                = (struct list **)pups_realloc((void *)_phfraghead,to_size*sizeof(struct list *));
    for(i=from_size; i<to_size; ++i)
        _phfraghead[i] = (struct list *)pups_calloc(BLOCKLOG,sizeof(struct list));

    _pheap_chunks_used         = (__malloc_size_t *)pups_realloc((void *)_pheap_chunks_used,to_size*sizeof(__malloc_size_t));
    _pheap_bytes_used          = (__malloc_size_t *)pups_realloc((void *)_pheap_bytes_used,to_size*sizeof(__malloc_size_t));
    _pheap_chunks_free         = (__malloc_size_t *)pups_realloc((void *)_pheap_chunks_free,to_size*sizeof(__malloc_size_t));
    _pheap_bytes_free          = (__malloc_size_t *)pups_realloc((void *)_pheap_bytes_free,to_size*sizeof(__malloc_size_t));
    __phmalloc_initialized     = (int32_t *)        pups_realloc((void *)__phmalloc_initialized,to_size*sizeof(int32_t));

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): persistent heap table extended  (from %d to %d heaps slots)\n",
                                               date,appl_name,appl_pid,appl_host,appl_owner,from_size,to_size);
       (void)fflush(stderr);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*-----------------------------------------------*/
/* Tell heap to destroy itself when we detach it */
/*-----------------------------------------------*/

_PUBLIC int32_t msm_set_autodestruct_state(const uint32_t hdes, const _BOOLEAN autodestruct)

{   
    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(hdes >= appl_max_pheaps || htable[hdes].exists == FALSE ||
       (autodestruct != FALSE && autodestruct != TRUE)          )
    { 

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EINVAL);
       return(-1);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[msm_set_autodestruct_state] attempt by non root thread to perform PUPS/P3 persistent heap operation");


    htable[hdes].autodestruct = autodestruct;

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*---------------------------------------------------------------------------------*/
/* Return a handle to the persistent heap whose pathname is map_f_path. The flags  */
/* specifies the (file) I/O mode the persistent heap, and mode specifies the level */
/* of file protection                                                              */
/*---------------------------------------------------------------------------------*/

_PUBLIC int32_t msm_heap_attach(const char *map_f_path, const int32_t attach_mode)

{   uint32_t i;

    int32_t  flags,
             ret,
             start      = 0;

    size_t   size,
             offset     = 0L;

    _BOOLEAN map_exists = FALSE;



    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(htable == (const heap_type *)NULL && (! attach_mode & CREATE_PHEAP) && (! attach_mode & ATTACH_PHEAP))
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[msm_heap_attach] attempt by non root thread to perform PUPS/P3 persistent heap operation");


    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    /*-----------------------------------------------------------*/
    /* If creation mode is not specified and the persistent heap */
    /* does not exist this is an error                           */
    /*-----------------------------------------------------------*/

    if( !(attach_mode & CREATE_PHEAP) && access(map_f_path,F_OK | R_OK | W_OK) == (-1))
    {  
       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EEXIST);
       return(-1);
    }

    for(i=0; i<appl_max_pheaps; ++i)
    {  

       /*----------------------------------------------------------------------------*/
       /* Check to make sure that we aren't already attached to this persistent heap */
       /*----------------------------------------------------------------------------*/

       if(htable[i].name != (char *)NULL && strcmp(htable[i].name,map_f_path) == 0)
       {

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&htab_mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(EALREADY);
          return(-1);
       }

       if(htable[i].addr == (void *)NULL)
       {   int32_t ret,
                   f_index;

          _BOOLEAN lock_tried = FALSE;

          if(access(map_f_path,F_OK | R_OK | W_OK) == (-1))
          {  htable[i].exists = FALSE;


             /*----------------------------------------------------------*/
	     /* We need to avoid a potential race condition at the time  */
	     /* of file creation here. If we have failed to win the race */
	     /* to create the file simply attach it.                     */
             /*----------------------------------------------------------*/

             if(pups_creat(map_f_path,0644) == (-1))
             {

                #ifdef PTHREAD_SUPPORT
                (void)pthread_mutex_unlock(&htab_mutex);
                #endif /* PTHREAD_SUPPORT */


                /*--------------------------------------------*/
                /* Delay to make sure winner initialises heap */
                /*--------------------------------------------*/

	        (void)pups_usleep(1000000);

                if(attach_mode & LIVE_PHEAP)
                   return(msm_heap_attach(map_f_path, ATTACH_PHEAP | LIVE_PHEAP));
                else
                   return(msm_heap_attach(map_f_path, ATTACH_PHEAP));
	     }
          }
          else
             htable[i].exists = TRUE;


          /*-----------------------------------------------------------*/
	  /* At present flags are the same for all map modes. This may */
	  /* change in the future.                                     */
          /*-----------------------------------------------------------*/

	  flags = O_RDWR;


          /*--------------------------------------------------------*/
          /* Protect heap mapping homeostatically if asked to do so */
          /*--------------------------------------------------------*/

          if(attach_mode & LIVE)
             htable[i].fd = pups_open(map_f_path, flags, LIVE);
          else
             htable[i].fd = pups_open(map_f_path, flags, DEAD);

          if(htable[i].fd == (-1))
          {
             #ifdef PTHREAD_SUPPORT
             (void)pthread_mutex_unlock(&htab_mutex);
             #endif /* PTHREAD_SUPPORT */

             pups_set_errno(EPERM);
             return(-1);
          }


          /*-----------------------------------------------------*/
          /* Lock the persistent heap so we have exlusive access */
          /*-----------------------------------------------------*/

          (void)pups_lockf(htable[i].fd,PUPS_WRLOCK,0);


          /*----------------------------------*/
          /* Set file type to persistent heap */
          /*----------------------------------*/

          f_index = pups_get_ftab_index(htable[i].fd);
          ftab[f_index].pheap = TRUE;


          /*---------------------*/
          /* New persistent heap */
          /*---------------------*/

          if(htable[i].exists == FALSE)
          {  (void)pups_lseek(htable[i].fd,PHM_BSTORE_SIZE - 1,SEEK_END);
             (void)write(htable[i].fd,"b",1);
             (void)pups_lseek(htable[i].fd,0,SEEK_SET);
          }


          /*-----------------------------------*/
          /* Is file existing persistent heap? */
          /*-----------------------------------*/

          else
          {  uint64_t magic,
                      htag;


             /*---------------------------------*/
             /* Check this is a valid heap file */
             /*---------------------------------*/

             if(pups_lseek(htable[i].fd,MAGIC_OFFSET*sizeof(uint64_t),SEEK_SET) == (-1))
             {  htable[i].fd = pups_close(htable[i].fd);

                #ifdef PTHREAD_SUPPORT
                (void)pthread_mutex_unlock(&htab_mutex);
                #endif /* PTHREAD_SUPPORT */

                pups_set_errno(EINVAL);
                return(-1);
             } 


             /*---------------------------------------------------*/
             /* Get heap magic (used to identify persistent heap) */
             /*---------------------------------------------------*/

             (void)pups_pipe_read(htable[i].fd,&magic,sizeof(uint64_t));


             /*-----------------------------------*/
             /* Is this the correct type of heap? */
             /*-----------------------------------*/

             if(sizeof(void *) == 8)
             {  if(pups_bigendian() == TRUE)
                   ret = (magic == HEAPMAGIC64BIG);
                else
                   ret = (magic == HEAPMAGIC64LITTLE);
             }
             else
             {  if(pups_bigendian() == TRUE)
                   ret = (magic == HEAPMAGIC32BIG);
                else
                   ret = (magic == HEAPMAGIC32LITTLE);
             }


             /*-----------------------*/
             /* Not a persistent heap */
             /*-----------------------*/

             if(! ret)
             {  htable[i].fd = pups_close(htable[i].fd);

                #ifdef PTHREAD_SUPPORT
                (void)pthread_mutex_unlock(&htab_mutex);
                #endif /* PTHREAD_SUPPORT */

                if(appl_verbose == TRUE)
                {  (void)strdate(date);
                   (void)fprintf(stderr,"%s %s (%d@%s:%s): not a valid persistent heap (%s)\n",
		                       date,appl_name,appl_pid,appl_host,appl_owner,map_f_path);
                   (void)fflush(stderr);
                }

                pups_set_errno(EINVAL);
                return(-1);
             }


             /*------------------------------*/
             /* Check that heap is not stale */
             /*------------------------------*/

             (void)pups_pipe_read(htable[i].fd,&htag,sizeof(uint64_t));
             if(appl_vtag != NO_VERSION_CONTROL && htag != appl_vtag)
             {  htable[i].fd = pups_close(htable[i].fd);

                #ifdef PTHREAD_SUPPORT
                (void)pthread_mutex_unlock(&htab_mutex);
                #endif /* PTHREAD_SUPPORT */

                if(appl_verbose == TRUE)
                {  (void)strdate(date);
                   (void)fprintf(stderr,"%s %s (%d@%s:%s): stale persistent heap (%s)\n",date,appl_name,appl_pid,appl_host,appl_owner,map_f_path);
                   (void)fflush(stderr);
                }

                pups_set_errno(EINVAL);
                return(-1);
             }


             /*----------------------------------------------------------------*/
             /* Check that the host whose process is trying to attach the heap */
             /* can support the address width of this heap                     */
             /*----------------------------------------------------------------*/

             if(htable[i].ptrsize != 8*sizeof(void *))
             {  htable[i].fd = pups_close(htable[i].fd);

                #ifdef PTHREAD_SUPPORT
                (void)pthread_mutex_unlock(&htab_mutex);
                #endif /* PTHREAD_SUPPORT */

                if(appl_verbose == TRUE)
                {  (void)strdate(date);
                   (void)fprintf(stderr,"%s %s (%d@%s:%s): heap (%s) address mode conflict\n",date,appl_name,appl_pid,appl_host,appl_owner,map_f_path);
                   (void)fflush(stderr);
                }

                pups_set_errno(EINVAL);
                return(-1);
             }
          }


          /*-------------------------------------*/
          /* Map heap into process address space */
          /*-------------------------------------*/

          if((htable[i].addr = (void *)mmap(0,
                                            PHM_SBRK_SIZE,
                                            PROT_READ  | PROT_WRITE,
                                            MAP_SHARED,
                                            htable[i].fd,
                                            offset)) == (void *)NULL)
          {  htable[i].fd = pups_close(htable[i].fd);

             #ifdef PTHREAD_SUPPORT
             (void)pthread_mutex_unlock(&htab_mutex);
             #endif /* PTHREAD_SUPPORT */

             pups_set_errno(EAGAIN);
             return(-1);
          }


          /*--------------------------------------------------*/
          /* Save heap map and prot modes (for checkpointing) */ 
          /*--------------------------------------------------*/

          htable[i].segment_size = PHM_SBRK_SIZE;
          htable[i].flags        = flags;


          /*---------------------------------------------------*/
          /* First sizeof(long) bytes is the size of this heap */
          /*---------------------------------------------------*/

          (void)strlcpy(htable[i].name,map_f_path,SSIZE);


          /*----------------------------------------------------------*/
          /* This is a dummy which will be overwritten if the heap we */
          /* are mapping already exists                               */
          /*----------------------------------------------------------*/

          htable[i].sdata = (uint64_t)htable[i].addr + sizeof(uint64_t);
          htable[i].edata = (uint64_t)htable[i].addr + sizeof(uint64_t);


          /*-------------------------------*/
          /* This is a new persistent heap */
          /*-------------------------------*/

          if(htable[i].exists == FALSE)
          {

             /*-----------------------------------*/
             /* Length of allocated heap in bytes */
             /*-----------------------------------*/

             if(appl_verbose == TRUE)
             {  (void)strdate(date);
                (void)fprintf(stderr,
                              "(%s %s (%d@%s:%s): %03d bit persistent heap %d [%-32s], length %016lx created and attached at %016lx virtual\n",
                                                                                                  date,appl_name,appl_pid,appl_host,appl_owner,
                                                                                                                             htable[i].ptrsize,
                                                                                                                                             i,
                                                                                                                                    map_f_path,
                                                                                                             htable[i].edata - htable[i].sdata,
                                                                                                                               htable[i].sdata);
                (void)fflush(stderr);
             }


             /*---------------------------------*/
             /* Initialise heap datastrutctures */
             /*---------------------------------*/

             (void)initialize_heap(i);
             break;
          }

          _phmaps_exist = TRUE;


          /*--------------------------------*/
          /* Initialise heap datastructures */
          /*--------------------------------*/

          (void)initialize_heap(i);

          if(appl_verbose == TRUE)
	  {     (void)fprintf(stderr,
                              "(%s %s (%d@%s:%s): %03d bit persistent heap %d (%-32s), length %016lx (segment %016lx) attached at %016lx virtual\n",
                                                                                                       date,appl_name,appl_pid,appl_host,appl_owner,
                                                                                                                                  htable[i].ptrsize,
                                                                                                                                                  i,
                                                                                                                                         map_f_path,
                                                                                                                  htable[i].edata - htable[i].sdata,
                                                                                                                   (uint64_t)htable[i].segment_size,
						                                                                                    htable[i].sdata);
             (void)fflush(stderr);
          }          

          break;
       }
    }


    /*----------------------------*/
    /* Persistent heap table full */
    /*----------------------------*/

    if(i == appl_max_pheaps)
    {

      #ifdef PTHREAD_SUPPORT
      (void)pthread_mutex_unlock(&htab_mutex);
      #endif /* PTHREAD_SUPPORT */

      pups_set_errno(EACCES);
      return(-1);
    }


    /*------------------------------------------*/
    /* Make addresses in persistent heap local  */
    /* to this process so we can access objects */
    /* located on it                            */
    /*------------------------------------------*/

    if(htable[i].addresses_local == FALSE)
       (void)msm_isync_heaptables(i);


    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(i);
}




/*----------------------------------------------------------------------------------*/
/* Detach a persistent heap -- if flags | O_DESTROY the persistent heap is unlinked */
/* and destroyed                                                                    */
/*----------------------------------------------------------------------------------*/

_PUBLIC int32_t msm_heap_detach(const uint32_t hdes, const int32_t flags)

{   char args[SSIZE] = "";


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(hdes < 0 || hdes >= appl_max_pheaps)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[msm_heap_detach] attempt by non root thread to perform PUPS/P3 persistent heap operation");


    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    if(htable[hdes].fd == (-1))
    {

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EACCES);
       return(-1);
    }


    /*---------------------------------------------------------------------------*/
    /* Close mapping file (open of persistent heap is homeostatically protected) */
    /*---------------------------------------------------------------------------*/

    if(pups_fd_islive(htable[hdes].fd) == TRUE)
       (void)pups_close(htable[hdes].fd);


    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,
                     "%s %s (%d@%s:%s): persistent heap %04d (%-32s), length %016lx (segment %016lx) detached from %016lx virtual",
                                                                                      date,appl_name,appl_pid,appl_host,appl_owner,
                                                                                                                              hdes,
                                                                                                                 htable[hdes].name,
                                                                                           htable[hdes].edata - htable[hdes].sdata,
                                                                                               (uint64_t)htable[hdes].segment_size,
                                                                                                                htable[hdes].sdata);


       /*----------------------------------------*/
       /* Display details of pending destruction */
       /*----------------------------------------*/

       if(flags & O_DESTROY)
          (void)fprintf(stderr," (destroyed)\n");
       else
          (void)fprintf(stderr,"\n");
       (void)fflush(stderr);

       (void)fflush(stderr);
    }


    /*-------------------------------------*/
    /* Make addresses global so they       */
    /* can be mapped into the address      */
    /* space of the next process attaching */
    /* the heap                            */
    /*-------------------------------------*/

    (void)msm_sync_heaptables(hdes);


    /*-----------------------------------------------*/
    /* Update of objects on persistent heaps happens */
    /* when an object is unlocked                    */
    /*-----------------------------------------------*/

    (void)msync((caddr_t)htable[hdes].addr,htable[hdes].segment_size,MS_SYNC | MS_INVALIDATE);
    (void)munmap((caddr_t)htable[hdes].addr,htable[hdes].segment_size);


    /*--------------------------------*/
    /* Unlock heap so other processes */
    /* can access it                  */
    /*--------------------------------*/

    (void)pups_lockf(htable[hdes].fd,PUPS_UNLOCK,0);


    /*-----------------------------*/
    /* Close mapping file for heap */
    /*-----------------------------*/

    (void)pups_close(htable[hdes].fd);
 

    /*--------------------------------------*/
    /* No new clients can attach heap       */
    /* once it is unlinked but existing     */
    /* clients remain attached to its inode */
    /*--------------------------------------*/

    if(flags & O_DESTROY || htable[hdes].autodestruct == TRUE)
       (void)unlink(htable[hdes].name);


    /*---------------------------*/
    /* Clear entry in heap table */
    /*---------------------------*/

    htable[hdes].fd           = (-1);
    htable[hdes].sdata        = 0;
    htable[hdes].edata        = 0;
    htable[hdes].segment_size = 0;
    htable[hdes].addr         = (void *)NULL;

    (void)strlcpy(htable[hdes].name,"",SSIZE);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);     
    return(0);
}




/*--------------------------------------------------------------------------------------*/
/* sbrk on the persistent heap whose heap descriptor is hdes - size extra bytes will be */
/* mapped into the address spaces of the processes attached to the heap                 */
/*--------------------------------------------------------------------------------------*/

_PUBLIC void *msm_sbrk(const uint32_t hdes, const size_t size)

{   void   *ptr = (void *)NULL;

    size_t brk_core_size,
           brk_core_needed,
           old_segment_size;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(hdes < 0 || hdes >= appl_max_pheaps)
    {  pups_set_errno(EINVAL);
       return((void *)NULL);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    if(htable[hdes].fd == (-1))
    {  

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EACCES);
       return((void *)NULL);
    }


    /*------------------------------------------------------------------*/
    /* Do we need to mmap more resources into the address space of this */
    /* process?                                                         */
    /*------------------------------------------------------------------*/

    brk_core_size   = htable[hdes].edata - htable[hdes].sdata - sizeof(int32_t) + size;

    if(brk_core_size > htable[hdes].segment_size)
       brk_core_needed = brk_core_size -  htable[hdes].segment_size;
    else
       brk_core_needed = 0;

    if(brk_core_needed > 0)
    {   int32_t n_brk_segments;

       uint64_t seg_used;
       void     *old_addr = (void *)NULL;
       uint64_t offset    = 0L;

       #ifdef PHEAP_DEBUG
       print_heaptables(hdes,"BEFORE EXTEND",(uint64_t         )htable[hdes].addr);
       #endif /* PHEAP_DEBUG */


       /*----------------------------------------------------------------------*/
       /* Get number of SBRK segments currently in use by this persistent heap */
       /*----------------------------------------------------------------------*/

       seg_used = htable[hdes].edata - htable[hdes].sdata   - (uint64_t)sizeof(int32_t); 


       /*------------------------------------------------------------------*/
       /* Extend backing store (file) object - note that we do this BEFORE */
       /* the heap is flushed so that the isync to pull the heap back into */
       /* local address space after remapping sees the adjusted segment    */
       /* size                                                             */
       /*------------------------------------------------------------------*/

       old_segment_size          =  htable[hdes].segment_size;
       n_brk_segments            =  (int32_t)ceil((double)brk_core_needed / (double)PHM_SBRK_SIZE);
       htable[hdes].segment_size += PHM_SBRK_SIZE*n_brk_segments;


       /*------------------------------------------------------------------------*/
       /* Flush local changes to persistent memory before brk extends persistent */
       /* memory segment                                                         */
       /* Remember that we must save the old mapping address in order to         */
       /* be able to update all pointers in the (possibly moved) mmap'ed         */
       /* persistent heap                                                        */
       /*------------------------------------------------------------------------*/


       #ifdef PHEAP_DEBUG
       (void)fprintf(stderr,"EXTEND BRK SEGMENT\n");
       (void)fflush(stderr);
       #endif /* PHEAP_DEBUG */

       (void)msm_sync_heaptables(hdes);
       (void)msync((caddr_t)htable[hdes].addr,old_segment_size,MS_SYNC | MS_INVALIDATE);
       (void)munmap((caddr_t)htable[hdes].addr,old_segment_size);


       /*------------------------------------*/
       /* Extend backing store (file) object */
       /*------------------------------------*/

       n_brk_segments            =  (int32_t)ceil((double)brk_core_needed / (double)PHM_SBRK_SIZE);
       htable[hdes].segment_size += PHM_SBRK_SIZE*n_brk_segments;

       #ifdef PHEAP_DEBUG
       (void)fprintf(stderr,"msm_sbrk: EXTENDING MAPPED SEGMENT (by %04d brk segments = %016lx bytes)\n",
                                                                                          n_brk_segments,
                                                                            PHM_SBRK_SIZE*n_brk_segments);
       (void)fflush(stderr);
       #endif /* PHEAP_DEBUG */


       /*----------------------*/
       /* Extend memory object */
       /*----------------------*/

       htable[hdes].addr = (void *)mmap((caddr_t)htable[hdes].addr,
                                        (size_t)htable[hdes].segment_size,
                                        PROT_READ  | PROT_WRITE,
                                        MAP_SHARED,
                                        htable[hdes].fd,
                                        (off_t)offset);


       /*--------------------*/
       /* Adjust sbrk limits */
       /*--------------------*/

       (void)msm_isync_heaptables(hdes);

       #ifdef PHEAP_DEBUG
      (void)fprint_heaptables(hdes,"AFTER EXTEND",(uint64_t)htable[hdes].addr);
      #endif /* PHEAP_DEBUG */
    } 

    if(size > 0)
    {  ptr                   =  (void *)(htable[hdes].edata);
       htable[hdes].edata   += size;
    }
    else
    {  htable[hdes].edata   -= size;
       ptr                   =  (void *)(htable[hdes].edata);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    #ifdef PHEAP_DEBUG
    (void)fprintf(stderr,"msm_sbrk: heap %04d (%-32s) extended by %016lx bytes\n",hdes,htable[hdes].name,size);
    (void)fflush(stderr);
    #endif /* PHEAP_DEBUG */

    return(ptr);
}




/*----------------------------------------------------------------*/
/* Routine to stat a heap (in a similar manner to stat on a file) */
/*----------------------------------------------------------------*/

_PUBLIC int32_t msm_hstat(const uint32_t hdes, heap_type *heapinfo)


{

    /*--------------*/
    /* Sanity check */
    /*--------------*/
   
    if(hdes<0 || hdes >= appl_max_pheaps            ||
       heapinfo       == (const heap_type *)NULL     )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    if(htable[hdes].fd == (-1))
    {  
       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EACCES);
       return(-1);
    }

    *heapinfo = htable[hdes];

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*--------------------------------------------------------------*/
/* Find first free named persistent object slot in the map area */
/*--------------------------------------------------------------*/

_PUBLIC int32_t msm_get_free_mapslot(const uint32_t hdes)

{   uint32_t i;
     int32_t h_index;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(hdes >= appl_max_pheaps)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    for(i=0; i<_phobjects_allocated[hdes]; ++i)
    {  if(_phobjectmap[hdes][i] == (phobmap_type *)NULL)
       {

          #ifdef PHEAP_DEBUG
          (void)fprintf(stderr,"msm_get_free_mapslot: returned slot %d\n",i);
          (void)fflush(stderr);
          #endif /* PHEAP_DEBUG */

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&htab_mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(i);
       }
    }

    h_index                     =  _phobjects_allocated[hdes];
    _phobjects_allocated[hdes] += PHOBMAP_QUANTUM;
    if((_phobjectmap[hdes] = (phobmap_type **)phrealloc(hdes,
                                                        (void *)_phobjectmap[hdes],
                                                        _phobjects_allocated[hdes],
                                                        "address map")) == (phobmap_type **)NULL)
    {  

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(ENOMEM);
       return(-1);
    }

    for(i=h_index; i<_phobjects_allocated[hdes]; ++i)
       _phobjectmap[hdes][i] = (phobmap_type *)NULL;

    #ifdef PHEAP_DEBUG
    (void)fprintf(stderr,"msm_get_free_mapslot: map table extended by %d slots\n",PHOBMAP_QUANTUM);
    (void)fflush(stderr);
    #endif /* PHEAP_DEBUG */

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(h_index);
}




/*------------------------------------------*/
/* Find mapped object (returning its index) */
/*------------------------------------------*/

_PUBLIC int32_t msm_find_mapped_object(const uint32_t hdes, const void *ptr)

{   uint32_t i;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(hdes<0 || hdes >= appl_max_pheaps || ptr == (void *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    if(_no_phobject_mapping == 1)
    {  

      #ifdef PTHREAD_SUPPORT
      (void)pthread_mutex_unlock(&htab_mutex);
      #endif /* PTHREAD_SUPPORT */

      pups_set_errno(EACCES);
      return(-1);
    }

    for(i=0; i<_phobjects_allocated[hdes]; ++i)
    {  if(_phobjectmap[hdes][i] != (phobmap_type *)NULL &&
          _phobjectmap[hdes][i]->addr == ptr             )
       {  
          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&htab_mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(i);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return(-1);
} 




/*---------------------------------------------*/
/* Add object to the persitent heap object map */
/*---------------------------------------------*/

_PUBLIC int32_t msm_map_object(const uint32_t hdes, const uint32_t h_index, const void *ptr, const char *name)

{

    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(hdes >= appl_max_pheaps       ||
       name == (const char *)NULL    ||
       ptr  == (const void *)NULL     )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */


    /*---------------------------------*/
    /* Sanity checks (heap statistics) */
    /*---------------------------------*/

    if(h_index > _phobjects_allocated[hdes])
    {

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(ERANGE);
       return(-1);
    }

    _no_phobject_mapping = 1;
    if((_phobjectmap[hdes][h_index] = (phobmap_type *)phmalloc(hdes,sizeof(phobmap_type),(char *)NULL)) == (phobmap_type *)NULL)
    {
       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(ENOMEM);
       return(-1);
    }


    /*--------------------------------------*/
    /* Pointer to the object we have mapped */
    /*--------------------------------------*/

    _no_phobject_mapping               = 0;
    _phobjectmap[hdes][h_index]->addr  = ptr;
    _phobjectmap[hdes][h_index]->size  = 0;

    if(name != (char *)NULL)
       (void)strlcpy(_phobjectmap[hdes][h_index]->name,name,SSIZE);

    (void)strlcpy(_phobjectmap[hdes][h_index]->info,"type unknown",SSIZE);
    ++_phobjects[hdes];

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}





/*-----------------------------------------*/
/* Remove persisent object from object map */
/*-----------------------------------------*/

_PUBLIC int32_t msm_unmap_object(const uint32_t hdes, const uint32_t h_index)

{

    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(hdes<0 || hdes >= appl_max_pheaps)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */


    /*--------------------------------*/
    /* Sanity check (heap statistics) */
    /*--------------------------------*/

    if(h_index < 0 || h_index > _phobjects_allocated[hdes])
    {

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(ERANGE);
       return(-1);
    }

    if(_phobjectmap[hdes][h_index] == (phobmap_type *)NULL)
    {
       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EACCES);
       return(-1);
    }

    _phobjectmap[hdes][h_index]->addr = (void *)NULL;
    (void)strlcpy(_phobjectmap[hdes][h_index]->name,"",SSIZE);
    --_phobjects[hdes];

    _no_phobject_mapping = 1;
    _phobjectmap[hdes][h_index] = (phobmap_type *)phfree(hdes,_phobjectmap[hdes][h_index]);
    _no_phobject_mapping = 0;

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*-------------------------------------------*/
/* Translate persistent object name to index */
/*-------------------------------------------*/

_PUBLIC int32_t msm_map_objectname2index(const uint32_t hdes, const char *name)

{   uint32_t i;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(hdes >= appl_max_pheaps || name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    for(i=0; i<_phobjects_allocated[hdes]; ++i)
    {  if(_phobjectmap[hdes][i] != (phobmap_type *)NULL && strcmp(_phobjectmap[hdes][i]->name,name) == 0)
       {
          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&htab_mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(i);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return(-1);
}





/*----------------------------------------------*/
/* Translate persistent object address to index */
/*----------------------------------------------*/

_PUBLIC int32_t msm_map_objectaddr2index(const uint32_t hdes, const void *addr)

{   uint32_t i;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(hdes >= appl_max_pheaps || addr == (const void *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    for(i=0; i<_phobjects_allocated[hdes]; ++i)
    {  if(_phobjectmap[hdes][i] != (phobmap_type *)NULL && _phobjectmap[hdes][i]->addr == addr)
       {
          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&htab_mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(i);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return(-1);
}




/*---------------------------------------------*/
/* Translate persistent object name to address */
/*---------------------------------------------*/

_PUBLIC void *msm_map_objectname2addr(const uint32_t hdes, const char *name)

{   uint32_t i;
    void     *addr = (void *)NULL;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(hdes >= appl_max_pheaps       ||
       name == (const char *)NULL     )
    {  pups_set_errno(EINVAL);
       return((void *)NULL);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    for(i=0; i<_phobjects_allocated[hdes]; ++i)
    {  if(strcmp(_phobjectmap[hdes][i]->name,name) == 0)
       {  addr = _phobjectmap[hdes][i]->addr;

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&htab_mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(addr);
       }
    }   

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return((void *)NULL);
}
 




/*-------------------------------------------*/
/* Translate persistent object index to name */
/*-------------------------------------------*/

_PUBLIC char *map_objectindex2name(const uint32_t  hdes, const uint32_t h_index)

{   char *name = (char *)NULL;

    
    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(hdes >= appl_max_pheaps)
    {  pups_set_errno(EINVAL);
       return((void *)NULL);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */


    /*--------------------------------*/
    /* Sanity check (persistent heap) */
    /*--------------------------------*/

    if(h_index > _phobjects_allocated[hdes])
    {
       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(ERANGE);
       return((char *)NULL);
    }


    /*----------*/
    /* Get name */
    /*----------*/

    if(_phobjectmap[hdes][h_index]->addr != (void *)NULL) 
    {  name = _phobjectmap[hdes][h_index]->name;

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(OK);
       return(name); 
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH); 
    return((char *)NULL); 
} 




/*----------------------------------------------*/
/* Translate persistent object index to address */
/*----------------------------------------------*/

_PUBLIC char *map_objectindex2addr(const uint32_t hdes, const uint32_t h_index)

{   void *addr = (void *)NULL;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(hdes >= appl_max_pheaps)
    {  pups_set_errno(EINVAL);
       return((void *)NULL);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */


    /*--------------------------------*/
    /* Sanity check (persistent heap) */
    /*--------------------------------*/

    if(h_index > _phobjects_allocated[hdes])
    {
       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(ERANGE);
       return((char *)NULL);
    }


    /*-------------*/
    /* Get address */
    /*-------------*/

    if(_phobjectmap[hdes][h_index]->addr != (void *)NULL)
    {  addr = _phobjectmap[hdes][h_index]->addr;

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(OK);
       return(addr);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return((char *)NULL);
}




/*------------------------------------*/
/* Make sure that a mapname is unique */
/*------------------------------------*/

_PUBLIC  int32_t msm_mapname_unique(const uint32_t hdes, const char *name)

{   return(msm_map_objectname2index(hdes,name));
}
  



/*----------------------------------------*/
/* Set the info field for a mapped object */
/*----------------------------------------*/

_PUBLIC int32_t msm_map_setinfo(const uint32_t hdes, const uint32_t h_index, const char *info)

{

    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(hdes >= appl_max_pheaps    ||
       info == (char *)NULL        )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */


    /*--------------------------------*/
    /* Sanity check (persistent heap) */
    /*--------------------------------*/

    if(h_index>_phobjects_allocated[hdes])
    {
       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(ERANGE);
       return(-1);
    }

    (void)strlcpy(_phobjectmap[hdes][h_index]->info,info,SSIZE);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}





/*----------------------------------------*/
/* Set the size field for a mapped object */
/*----------------------------------------*/

_PUBLIC int32_t msm_map_setsize(const uint32_t hdes, const uint32_t h_index, size_t size)

{

    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(hdes >= appl_max_pheaps)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */
 

    /*--------------------------------*/
    /* Sanity check (persistent heap) */
    /*--------------------------------*/

    if(h_index>_phobjects_allocated[hdes])
    {
       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(ERANGE);
       return(-1);
    }

    _phobjectmap[hdes][h_index]->size = size;

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*-------------------------------------------------*/
/* Display statistics of object on persistent heap */
/*-------------------------------------------------*/

_PUBLIC int32_t msm_show_mapped_object(const uint32_t hdes, const uint32_t h_index, const FILE *stream)

{   char size_str[SSIZE]   = "",
         endian_str[SSIZE] = "";


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(hdes >= appl_max_pheaps || stream == (const FILE *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[msm_show_mapped_object] attempt by non root thread to perform PUPS/P3 persistent heap operation");


    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */


    /*--------------------------------*/
    /* Sanity check (persistent heap) */
    /*--------------------------------*/

    if(h_index > _phobjects_allocated[hdes])
    {  pups_set_errno(ERANGE);

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       return(-1);
    }

    if(_phobjectmap[hdes][h_index]->addr == (void *)NULL)
    {  pups_set_errno(EINVAL);

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       return(-1); 
    } 


    /*-----------------------*/
    /* Size of mapped object */
    /*-----------------------*/
 
    if(_phobjectmap[hdes][h_index]->size != 0)
       (void)snprintf(size_str,SSIZE,"%016lx bytes",_phobjectmap[hdes][h_index]->size);
    else
       (void)snprintf(size_str,SSIZE,"unknown");


    /*-----------------------------*/
    /* Get endian-ness of hardware */
    /*-----------------------------*/
 
    if(pups_bigendian() == TRUE)
       (void)strlcpy(endian_str,"big endian",SSIZE);
    else
       (void)strlcpy(endian_str,"little endian",SSIZE);


    /*---------------------------*/
    /* Display details of object */
    /*---------------------------*/

    (void)fprintf(stream,"\n    object                :  \"%s\"\n",      _phobjectmap[hdes][h_index]->name);
    (void)fprintf(stream,"    object descriptor     :  %04d\n",          h_index);
    (void)fprintf(stream,"    heap                  :  \"%s\"\n",        msm_hdes2name(hdes));
    (void)fprintf(stream,"    heap descriptor       :  %04d\n",          hdes);
    (void)fprintf(stream,"    object information    :  %s\n",            _phobjectmap[hdes][h_index]->info);
    (void)fprintf(stream,"    pointer size          :  %04d bytes\n",    htable[hdes].ptrsize);
    (void)fprintf(stream,"    pointer configuration :  %s\n",            endian_str);
    (void)fprintf(stream,"    object location       :  %016lx virtual\n",(uint64_t         )_phobjectmap[hdes][h_index]->addr);
    (void)fprintf(stream,"    object size           :  %016lx bytes\n\n",_phobjectmap[hdes][h_index]->size);
     
    (void)fflush(stream);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}
 




/*------------------------------------*/
/* Display objects on persistent heap */
/*------------------------------------*/

_PUBLIC int32_t msm_show_mapped_objects(const uint32_t hdes, const FILE *stream)

{   uint32_t i,
             objects = 0;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(hdes >= appl_max_pheaps || stream == (FILE *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[msm_show_mapped_objects] attempt by non root thread to perform PUPS/P3 persistent heap operation");


    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */


    (void)fprintf(stream,"\n\n    Persistent object map for heap \"%-32s\" (at %016lx virtual)\n\n",
                                                                                msm_hdes2name(hdes),
                                                                        (uint64_t)htable[hdes].addr);
    (void)fflush(stream);


    /*----------------------------*/
    /* Display each mapped object */
    /*----------------------------*/

    for(i=0; i<_phobjects_allocated[hdes]; ++i)
    {  if(_phobjectmap[hdes][i] != (void *)NULL)
       {  char size_str[SSIZE]    = "";

          if(_phobjectmap[hdes][i]->size != 0)
             (void)snprintf(size_str,SSIZE,"%016lx bytes",_phobjectmap[hdes][i]->size);
          else
             (void)snprintf(size_str,SSIZE,"unknown");

          (void)fprintf(stream,"    %04d: object \"%-32s\" [%-32s] at %016lx virtual [size %-16s]\n",
                                                                                                   i,
                                                                         _phobjectmap[hdes][i]->name,
                                                                         _phobjectmap[hdes][i]->info,
                                                               (uint64_t)_phobjectmap[hdes][i]->addr,
                                                                                            size_str);
          (void)fflush(stream);

          ++objects;
       }
    }


    /*------------------------------------*/
    /* Display number of atatched objects */
    /*------------------------------------*/

    if(objects == 0)
        (void)fprintf(stream,"    no mapped objects (%04d map slots free)\n\n",_phobjects_allocated[hdes]);
    else if(objects == 1)
        (void)fprintf(stream,"\n\n   %04d mapped object (%04d map slots free)\n\n",1,_phobjects_allocated[hdes]);
    else
    {  (void)fprintf(stream,"\n\n    %04d mapped objects (%04d map slots free)\n\n",_phobjects[hdes],
                                                       _phobjects_allocated[hdes] - _phobjects[hdes]);
       (void)fflush(stream);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}

    
          



/*----------------------------------------------------------*/
/* Translate persistent heap hdes (heap descriptor) to name */
/*----------------------------------------------------------*/

_PUBLIC char *msm_hdes2name(const uint32_t hdes)

{

    /*--------------*/
    /* Sanity check */
    /*--------------*/
 
    if(hdes >= appl_max_pheaps)
    {  pups_set_errno(EINVAL);
       return((void *)NULL);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    if(htable[hdes].addr != (void *)NULL)
    {

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(OK);
       return(htable[hdes].name);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(EACCES);
    return((char *)NULL);
}




/*----------------------------------------------------------*/
/* Translate persistent heap name to hdes (heap descriptor) */
/*----------------------------------------------------------*/

_PUBLIC int32_t msm_name2hdes(const char *name)

{   uint32_t i;


    /*--------------*/
    /* Sanity check */
    /*--------------*/
 
    if(name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    for(i=0; i<appl_max_pheaps; ++i)
    {  if(htable[i].fd != (-1) && strcmp(htable[i].name,name) == 0)
       {

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&htab_mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(i);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return(-1);
}




/*-----------------------------------------*/
/* Make heap table addresses process local */
/*-----------------------------------------*/

_PUBLIC int32_t msm_isync_heaptables(const uint32_t hdes)

{    uint64_t offset;
     char     info[SSIZE] = "";


     /*--------------*/
     /* Sanity check */
     /*--------------*/
 
     if(hdes > appl_max_pheaps)
     {  pups_set_errno(EINVAL);
        return(-1);
     }

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_lock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */


     /*---------------------------------------------------*/
     /* Relocation offset (for persistent heap addresses) */
     /*---------------------------------------------------*/

     offset = (uint64_t)htable[hdes].addr;

     _pheap_parameters[hdes]      = (uint64_t *)((uint64_t)sizeof(uint64_t)                  + offset);
     _pheapindex[hdes]            = _pheap_parameters[hdes][0];
     _pheap_bytes_used[hdes]      = _pheap_parameters[hdes][1];
     _pheap_bytes_free[hdes]      = _pheap_parameters[hdes][2];
     _pheap_chunks_used[hdes]     = _pheap_parameters[hdes][3];
     _pheap_chunks_free[hdes]     = _pheap_parameters[hdes][4];
     pheapsize[hdes]              = _pheap_parameters[hdes][5];
     _pheaplimit[hdes]            = _pheap_parameters[hdes][6];
     _pheapinfo[hdes]             = (malloc_info *)((uint64_t)_pheap_parameters[hdes][7]     + offset);
     _phobjects[hdes]             = _pheap_parameters[hdes][8];
     _phobjects_allocated[hdes]   = _pheap_parameters[hdes][9];
     _phobjectmap[hdes]           = (phobmap_type  **)((uint64_t)_pheap_parameters[hdes][10] + offset);
     _pheapbase[hdes]             = (char           *)((uint64_t)_pheap_parameters[hdes][11] + offset);
     htable[hdes].sdata           = _pheap_parameters[hdes][15]                              + offset;
     htable[hdes].edata           = _pheap_parameters[hdes][16]                              + offset;
     htable[hdes].segment_size    = _pheap_parameters[hdes][17];
     htable[hdes].heapmagic       = _pheap_parameters[hdes][18];
     htable[hdes].ptrsize         = _pheap_parameters[hdes][20];


     /*--------------------------------------------------------------------------*/
     /* Map all addresses in persistent memory  int32_to the address space of the the */
     /* current process                                                          */
     /*--------------------------------------------------------------------------*/

     (void)msm_relocate_heap_addresses(hdes,offset,ADD_OFFSET);

     #ifdef PHEAP_DEBUG
     (void)snprintf(info,SSIZE,"hdes %d (%s): persistent heap tables inversely sychronised",hdes,htable[hdes].name);
     print_heaptables(hdes,info,offset);
     #endif /* PHEAP_DEBUG */


     /*-----------------------------*/
     /* Record of heap address mode */
     /*-----------------------------*/

     htable[hdes].addresses_local = TRUE; 

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_unlock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */

     pups_set_errno(OK);
     return(0);
}

 


/*----------------------------------*/
/* Make heap table addresses global */
/*----------------------------------*/

_PUBLIC int32_t msm_sync_heaptables(const uint32_t hdes)

{    uint64_t offset;
     char     info[SSIZE] = "";


     /*--------------*/
     /* Sanity check */
     /*--------------*/
    
     if(hdes > appl_max_pheaps)
     {  pups_set_errno(EINVAL);
        return(-1);
     }

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_lock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */


     /*----------------------------------------------------------------*/
     /* Compute offset between global and local process address spaces */
     /*----------------------------------------------------------------*/

     offset = (uint64_t)htable[hdes].addr;


     /*----------------------------------------------------------------*/
     /* Map heap parameter table addresses  int32_to global address space   */
     /* of persistent heap                                             */
     /*----------------------------------------------------------------*/

     _pheap_parameters[hdes][0]    = _pheapindex[hdes];
     _pheap_parameters[hdes][1]    = _pheap_bytes_used[hdes];
     _pheap_parameters[hdes][2]    = _pheap_bytes_free[hdes];
     _pheap_parameters[hdes][3]    = _pheap_chunks_used[hdes];
     _pheap_parameters[hdes][4]    = _pheap_chunks_free[hdes];
     _pheap_parameters[hdes][5]    = pheapsize[hdes];
     _pheap_parameters[hdes][6]    = _pheaplimit[hdes];
     _pheap_parameters[hdes][7]    = (int64_t)((uint64_t)_pheapinfo[hdes]     - offset);
     _pheap_parameters[hdes][8]    = _phobjects[hdes];
     _pheap_parameters[hdes][9]    = _phobjects_allocated[hdes];
     _pheap_parameters[hdes][10]   = (int64_t)((uint64_t)_phobjectmap[hdes]   - offset);
     _pheap_parameters[hdes][11]   = (int64_t)((uint64_t)_pheapbase[hdes]     - offset);
     _pheap_parameters[hdes][15]   = htable[hdes].sdata                       - offset;
     _pheap_parameters[hdes][16]   = htable[hdes].edata                       - offset;
     _pheap_parameters[hdes][17]   = htable[hdes].segment_size;
     _pheap_parameters[hdes][18]   = htable[hdes].heapmagic;
     _pheap_parameters[hdes][19]   = appl_vtag;
     _pheap_parameters[hdes][20]   = sizeof(void *)*8; 


     /*--------------------------------------------------------------------*/
     /* Map process addresses into global address space of persistent heap */
     /*--------------------------------------------------------------------*/

     (void)msm_relocate_heap_addresses(hdes,offset,SUBTRACT_OFFSET);


     #ifdef PHEAP_DEBUG 
     (void)snprintf(info,SSIZE,"hdes %d (%s): persistent heap tables sychronised",hdes,htable[hdes].name);
     print_heaptables(hdes,info,offset);
     #endif /* PHEAP_DEBUG */
 

     /*------------------------*/
     /* Record addressing mode */
     /*------------------------*/

     _pheap_parameters[hdes][14]  = FALSE;
     htable[hdes].addresses_local = FALSE;

     _pheap_parameters[hdes]      = (uint64_t *)((uint64_t)sizeof(uint64_t) - offset);

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_unlock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */

     pups_set_errno(OK);
     return(0);
}




/*--------------------------*/
/* Display heap information */
/*--------------------------*/

_PUBLIC int32_t msm_print_heaptables(const FILE *stream, const uint32_t hdes, const char *info, const size_t offset)

{    

     /*--------------*/
     /* Sanity check */
     /*--------------*/

     if(stream == (const FILE *)NULL || hdes>appl_max_pheaps || offset == 0)
     {  pups_set_errno(EINVAL);
        return(-1);
     }


     #ifndef PHEAP_DEBUG
     /*----------------------------------*/
     /* Only the root thread can process */
     /* PSRP requests                    */
     /*----------------------------------*/

     if(pupsthread_is_root_thread() == FALSE)
        pups_error("[msm_print_heaptables] attempt by non root thread to perform PUPS/P3 persistent heap operation");
     #endif /* PHEAP_DEBUG */

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_lock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */

     (void)fprintf(stream,"\n\n%s\n\n",info);
     (void)fprintf(stream,"    hdes %04d address table at %016lx virtual\n",hdes,offset);

     (void)fprintf(stream,"    heap index        : %08ld\n",      _pheap_parameters[hdes][0]);
     (void)fprintf(stream,"    bytes used        : %08ld\n",      _pheap_parameters[hdes][1]);
     (void)fprintf(stream,"    bytes free        : %08ld\n",      _pheap_parameters[hdes][2]);
     (void)fprintf(stream,"    chunks used       : %08ld\n",      _pheap_parameters[hdes][3]);
     (void)fprintf(stream,"    chunks free       : %08ld\n",      _pheap_parameters[hdes][4]);
     (void)fprintf(stream,"    heap size         : %08ld\n",      _pheap_parameters[hdes][5]);
     (void)fprintf(stream,"    heap limit        : %08ld\n",      _pheap_parameters[hdes][6]);
     (void)fprintf(stream,"    heap info         : %010x\n",      (uint64_t)_pheap_parameters[hdes][7]);
     (void)fprintf(stream,"    heap objects      : %04d\n",       _pheap_parameters[hdes][8]);
     (void)fprintf(stream,"    heap object slots : %04d\n",       _pheap_parameters[hdes][9]);
     (void)fprintf(stream,"    heap object table : %016lx\n",     (uint64_t)_pheap_parameters[hdes][10]);
     (void)fprintf(stream,"    heap base         : %016lx\n\n",   (uint64_t)_pheap_parameters[hdes][11]);
     (void)fprintf(stream,"    heap clients      : %04d\n",       _pheap_parameters[hdes][12]);
     (void)fprintf(stream,"    heap client slots : %04d\n",       _pheap_parameters[hdes][13]);
     (void)fprintf(stream,"    heap client table : %016lx\n",     (uint64_t)_pheap_parameters[hdes][14]);
     (void)fprintf(stream,"    heap base         : %016lx\n",     (uint64_t)_pheap_parameters[hdes][15]);
     (void)fprintf(stream,"    heap top          : %016lx\n",     (uint64_t)_pheap_parameters[hdes][16]);
     (void)fprintf(stream,"    heap segment size : %016lx\n",     (uint64_t)_pheap_parameters[hdes][17]);
     (void)fprintf(stream,"    heapmagic         : %016lx\n",     (uint64_t)_pheap_parameters[hdes][18]);
     (void)fprintf(stream,"    heap vtag         : %04d\n",       (uint64_t)_pheap_parameters[hdes][19]);
     (void)fprintf(stream,"    heap address size : %04d (bits)\n",(uint64_t)_pheap_parameters[hdes][20]);
     (void)fflush(stream);

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_unlock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */

     pups_set_errno(OK);
     return(0);
} 





/*------------------------------------------------------------------*/
/* Relocate persistent heap (after munmap/mmap extension remapping) */ 
/*------------------------------------------------------------------*/


_PRIVATE int32_t msm_relocate_heap_addresses(int hdes, size_t offset,  int32_t offset_op)

{   uint32_t i;
 

    /*--------------*/
    /* Sanity check */
    /*--------------*/
 
    if(hdes   <  0                                                ||
       hdes   >  appl_max_pheaps                                  ||
       offset == 0                                                ||
      (offset_op != SUBTRACT_OFFSET && offset_op != ADD_OFFSET)    )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    #ifdef PHEAP_DEBUG
    if(offset_op == ADD_OFFSET)
    {  (void)fprintf(stderr,"    heap %04d mapped  int32_to process memory map at %016lx virtual\n",hdes,offset);
       (void)fflush(stderr);
    }
    else
    {  (void)fprintf(stderr,"    heap %04d unmapped to global address map\n",hdes);
       (void)fflush(stderr);
    }
    #endif /* PHEAP_DEBUG */


    /*-------------------------------------------------------------*/
    /* Relocate all pointer addresses with the persistent heap map */
    /*-------------------------------------------------------------*/

    if(_phobjectmap[hdes] != (phobmap_type **)NULL)
    {  for(i=0; i<_phobjects_allocated[hdes]; ++i)
       {  if(_phobjectmap[hdes][i] != (phobmap_type *)NULL)
          {  if(offset_op == ADD_OFFSET)
             {  if(_phmaps_exist == TRUE)
                {  _phobjectmap[hdes][i]       = (phobmap_type *)((uint64_t)_phobjectmap[hdes][i] + offset); 
                   _phobjectmap[hdes][i]->addr = (void *)((uint64_t        )_phobjectmap[hdes][i]->addr   + offset);
                }
             }
             else
             {  _phobjectmap[hdes][i]->addr = (void *)((uint64_t        )_phobjectmap[hdes][i]->addr   - offset);
                _phobjectmap[hdes][i]       = (phobmap_type *)((uint64_t)_phobjectmap[hdes][i] - offset); 
             }
          }
       }

       if(_phmaps_exist == FALSE)
          _phmaps_exist = TRUE;
    }


    /*-------------------------*/
    /* Adjust free block lists */
    /*-------------------------*/

    if(offset_op == ADD_OFFSET && htable[hdes].addresses_local == FALSE)
       global_to_local_blocklist(hdes, offset);
    else
       local_to_global_blocklist(hdes, offset);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*---------------------------------------------------------*/
/* Convert free block list from global to local addressing */
/*---------------------------------------------------------*/

_PRIVATE void global_to_local_blocklist(uint32_t hdes, size_t offset)

{   uint32_t    i;
    struct list *next = (struct list *)NULL;

    _BOOLEAN fraghead_address;


    if(htable[hdes].addresses_local == TRUE)
       return;


    /*----------------------------------*/
    /* Relocate all free block pointers */
    /*----------------------------------*/

    for(i=0; i<BLOCKLOG; ++i)
    {  next             = _phfraghead[hdes][i].next;
       fraghead_address = TRUE;
 
       if(next != (struct list *)NULL)
       {  next = (struct list *)((uint64_t)next + offset);

          if(fraghead_address == TRUE)
          {  _phfraghead[hdes][i].next = next;

             #ifdef PHEAP_DEBUG
             (void)fprintf(stderr,"log: %04d pointer %016lx\n",i,(uint64_t         )_phfraghead[hdes][i].next);
             (void)fflush(stderr);
             #endif /* PHEAP_DEBUG */
          }


          /*-----------------------------------------------------*/ 
          /* "walk" the free block list relocating its addresses */
          /*-----------------------------------------------------*/ 

          while(next->next != (struct list *)NULL)
          {  next->next = (struct list *)((uint64_t)next->next + offset);
             
             if(next->prev != (struct list *)NULL)
             {  if(fraghead_address == TRUE)
                {  //next->prev->next = next + offset;
                   fraghead_address = FALSE;
                }
                else
                   next->prev = (struct list *)((uint64_t)next->prev + offset);
             }


             /*---------------------------------*/ 
             /* Step to next free block in list */
             /*---------------------------------*/ 

             next = next->next;
          }
       }
    }
} 




/*---------------------------------------------------------*/
/* Convert free block list from local to global addressing */
/*---------------------------------------------------------*/

_PRIVATE void local_to_global_blocklist(int hdes, size_t offset)

{   uint32_t     i;
    struct list  *next = (struct list *)NULL,
                 *step = (struct list *)NULL;

    _BOOLEAN     fraghead_address;

    if(htable[hdes].addresses_local == FALSE)
       return;


    /*----------------------------------*/
    /* Relocate all free block pointers */
    /*----------------------------------*/

    for(i=0; i<BLOCKLOG; ++i)
    {  next             = _phfraghead[hdes][i].next;
       fraghead_address = TRUE;
 
       if(next != (struct list *)NULL)
       {

          /*-----------------------------------------------------*/
          /* "walk" the free block list relocating its addresses */
          /*-----------------------------------------------------*/

          while(next->next != (struct list *)NULL)
          {  
             step       = next->next;
             next->next = (struct list *)((uint64_t)next->next - offset);
 
             if(next->prev != (struct list *)NULL)
             {  if(fraghead_address == TRUE)
                   next->prev->next = next - offset; 
                else
                   next->prev = (struct list *)((uint64_t)next->prev - offset);
             }


             /*---------------------------------*/ 
             /* Step to next free block in list */
             /*---------------------------------*/ 

             next = (struct list *)((unsigned long)next - offset);

             if(fraghead_address == TRUE)
             {  fraghead_address = FALSE;
                _phfraghead[hdes][i].next = next;

                #ifdef PHEAP_DEBUG 
                (void)fprintf(stderr,"log: %04d pointer %016lx\n",i,(uint64_t)_phfraghead[hdes][i].next);
                (void)fflush(stderr);
                #endif /* PHEAP_DEBUG */
             }
             next = step;
          }


          /*-------------------------------------------------*/
          /* Special case if the fraghead is also the end of */
          /* the linked list                                 */
          /*-------------------------------------------------*/

          if(fraghead_address == TRUE)
          {  _phfraghead[hdes][i].next = (struct list *)((uint64_t)next - offset);
 
             #ifdef PHEAP_DEBUG
             (void)fprintf(stderr,"log: %04d pointer %016lx\n",i,(uint64_t)_phfraghead[hdes][i].next);
             (void)fflush(stderr);
             #endif /* PHEAP_DEBUG */
          }
       }   
    }   
}




/*---------------------------------------------------------------------*/
/* Persistent heap exit function (which detaches any persistent heaps) */
/*---------------------------------------------------------------------*/

_PUBLIC void msm_exit(const uint32_t flags)

{   uint32_t i,
             cnt = 0;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[msm_exit] attempt by non root thread to perform PUPS/P3 global persistent heap operation");

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): detaching persistent heaps\n",
                                date,appl_name,appl_pid,appl_host,appl_owner);
       (void)fflush(stderr);
    }

    for(i=0; i<appl_max_pheaps; ++i)
    {  if(msm_heap_detach(i,flags) == 0)
          ++cnt;
    }

    if(appl_verbose == TRUE) 
    {  if(cnt > 1)
          (void)fprintf(stderr,"%s %s (%d@%s:%s): %d persistent heaps detached\n",date,appl_name,appl_pid,appl_host,appl_owner,cnt);
       else if(cnt == 1)
          (void)fprintf(stderr,"%s %s (%d@%s:%s): 1 persistent heap detached\n",date,appl_name,appl_pid,appl_host,appl_owner);
       else
          (void)fprintf(stderr,"%s %s (%d@%s:%s): no persistent heaps to detach\n",date,appl_name,appl_pid,appl_host,appl_owner);
       (void)fflush(stderr);
    }
} 





/*------------------------------------------*/
/* Check to see if an object already exists */
/*------------------------------------------*/

_PUBLIC _BOOLEAN msm_phobject_exists(const uint32_t hdes, const char *name)

{   uint32_t i;

    if(hdes <  0                ||
       hdes >= appl_max_pheaps  ||
       name == (char *)NULL      )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    if(htable[hdes].addresses_local == FALSE)
       (void)msm_isync_heaptables(hdes);

    for(i=0; i<_phobjects_allocated[hdes]; ++i)
    {  if(_phobjectmap[hdes]    != (phobmap_type **)NULL    &&
          _phobjectmap[hdes][i] != (phobmap_type *) NULL    &&
          strcmp(_phobjectmap[hdes][i]->name,name) == 0      )
       {

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&htab_mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(TRUE);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return(FALSE);
}




/*---------------------------------------------------------------------*/
/* Make sure heap addresses are in either absolute (local) or relative */
/*---------------------------------------------------------------------*/

_PUBLIC int32_t msm_map_address_mode(const uint32_t  hdes, const uint32_t mode)

{
    
    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(htable[hdes].fd == (-1)                             ||
       hdes            < 0                                 ||
       hdes            >= appl_max_pheaps                  ||
       (mode != PHM_MAP_LOCAL && mode != PHM_MAP_GLOBAL)    )
    {

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EINVAL);
       return(-1);
    }
   
    if(htable[hdes].addresses_local == TRUE && mode == PHM_MAP_GLOBAL)
       (void)msm_sync_heaptables(hdes);
    else
    {  if(htable[hdes].addresses_local == FALSE && mode == PHM_MAP_LOCAL)
          (void)msm_isync_heaptables(hdes);  
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*-----------------------------------------*/
/* Translate heap name to heap table index */
/*-----------------------------------------*/

_PUBLIC int32_t msm_name2index(const char *name)

{   uint32_t i;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    for(i=0; i<appl_max_pheaps; ++i)
    {  if(htable[i].fd != (-1) && strcmp(htable[i].name,name) == 0)
       {  pups_set_errno(OK);
          return(i);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return(-1);
}




/*-----------------------------------------*/
/* Translate heap fdes to heap table index */
/*-----------------------------------------*/

_PUBLIC int32_t msm_fdes2hdes(const des_t fdes)

{   uint32_t i;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(fdes >= appl_max_files)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    for(i=0; i<appl_max_pheaps; ++i)
    {  if(htable[i].fd == fdes)
       {

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&htab_mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(i);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return(-1);
}




/*--------------------------------------------------------*/
/* Check to see if an address lies within persistent heap */
/*--------------------------------------------------------*/

_PUBLIC _BOOLEAN msm_addr_in_heap(const uint32_t hdes, const void *ptr)

{   uint64_t addr;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(ptr  == (const void *)NULL   ||
       hdes >= appl_max_pheaps       )
    {  pups_set_errno(EINVAL);
       return(FALSE);
    } 

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    addr = (uint64_t)ptr;
    if(addr < htable[hdes].sdata || addr > htable[hdes].edata)
    {  

      #ifdef PTHREAD_SUPPORT
      (void)pthread_mutex_unlock(&htab_mutex);
      #endif /* PTHREAD_SUPPORT */

       pups_set_errno(ERANGE);
       return(FALSE);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(TRUE);
}




/*----------------------------------------------------------------------------------*/
/* Resolve address pointer on persistent heap into address space of calling process */
/*----------------------------------------------------------------------------------*/

_PUBLIC void *msm_map_haddr_to_paddr(const uint32_t hdes, const void *ptr)

{   uint64_t addr;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(ptr  == (const void *)NULL    ||
       hdes >= appl_max_pheaps        )
    {  pups_set_errno(EINVAL);
       return((void *)NULL);
    } 

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    addr = (uint64_t)ptr;


    /*-----------------------------*/
    /* Check that address is valid */
    /*-----------------------------*/

    if(addr > (htable[hdes].edata - htable[hdes].sdata))
    {  

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(ERANGE);
       return((void *)NULL);
    }


    /*-----------------------------------------------------*/
    /* Convert address from (global) heap address space to */
    /* (local) process address space                       */
    /*-----------------------------------------------------*/

    ptr = (void *)(htable[hdes].sdata + addr);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(ptr);
}



 
/*-----------------------------------------------------------------------------------------*/
/* Resolve address pointer in address space of calling process to relative address on heap */
/*-----------------------------------------------------------------------------------------*/

_PUBLIC void *msm_map_paddr_to_haddr(const uint32_t hdes, const void *ptr)

{   uint64_t addr;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(ptr  == (const void *)NULL    ||
       hdes >= appl_max_pheaps        )
    {  pups_set_errno(EINVAL);
       return((void *)NULL);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    addr = (uint64_t)ptr;


    /*-----------------------------*/
    /* Check that address is valid */
    /*-----------------------------*/

    if(addr < htable[hdes].edata || addr >  htable[hdes].sdata)
    {  

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&htab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(ERANGE);
       return((void *)NULL);
    }


    /*-----------------------------------------------------*/
    /* Convert address from (global) heap address space to */
    /* (local) process address space                       */
    /*-----------------------------------------------------*/

    ptr = (void *)(htable[hdes].sdata - addr);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(ptr);
}




/*---------------------------------------------------*/
/* Save heap state (Criu state save support routine) */
/*---------------------------------------------------*/

_PUBLIC int32_t msm_save_heapstate(const char *ssave_dir)

{   uint32_t i;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(ssave_dir == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */


    /*---------------------------*/
    /* Save any (attached) heaps */
    /*---------------------------*/

    for(i=0; i<appl_max_pheaps; ++i)
    {  if(htable[i].fd != (-1))
       {  char ssave_pathname[SSIZE] = "";

          (void)snprintf(ssave_pathname,SSIZE,"%s/%s",ssave_dir,htable[i].name);
          (void)pups_copy_file(PUPS_FILE_COPY, htable[i].name,ssave_pathname);
       }
    }

    
    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}
