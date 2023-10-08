/*------------------------------------------------------------------------------------
    Persistent heap library. A persistent heap is an area
    of data memory which may be mapped serailly into the
    address spaces of multiple process.

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

    Version: 2.00 
    Dated:   4th January 2022
    Email:   mao@tumblingdice.co.uk
-----------------------------------------------------------------------------------*/

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
_PUBLIC int *__phmalloc_initialized                                = (int *)NULL;

// Number of objects in heap
_PUBLIC int *_phobjects                                            = (int *)NULL;
 
// Number of objects in heap
_PUBLIC int *_phobjects_allocated                                  = (int *)NULL;

// Pointer to table of significant objects on persistent heaps
_PUBLIC phobmap_type ***_phobjectmap                               = (phobmap_type ***)NULL;



/*-----------------------------------------------------*/
/* List of client_info structures for client processes */
/* using persistent heaps                              */
/*-----------------------------------------------------*/

// Pointer to persistent heap parameter table (on persistent heap)
_PUBLIC unsigned long int **_pheap_parameters                      = (unsigned long int **)NULL;
 
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


_IMPORT _PROTOTYPE int initialize_heap(int);


/*-----------------------*/ 
/* Persistent heap table */
/*-----------------------*/

_PUBLIC heap_type    *htable = (heap_type *)NULL;




/*------------------------------------*/
/* Functions exported by this modules */
/*------------------------------------*/

// Switch off presistent object map updating 
_PUBLIC int _no_phobject_mapping;




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
_PROTOTYPE _PRIVATE int msm_relocate_heap_addresses(int, unsigned long int, int);




/*--------------------------------------------------------*/ 
/* Convert free block addressing from global to local (in */
/* cached heap)                                           */
/*--------------------------------------------------------*/ 

_PROTOTYPE _PRIVATE void local_to_global_blocklist(int, unsigned long int);


/*--------------------------------------------------------*/
/* Convert free block addressing from local to global (in */
/* cached heap)                                           */
/*--------------------------------------------------------*/

_PROTOTYPE _PRIVATE void global_to_local_blocklist(int, unsigned long int);



/*------------------------------------------------------------------------------
    Slot and usage functions - used by slot manager ...
------------------------------------------------------------------------------*/


/*---------------------*/
/* Slot usage function */
/*---------------------*/

_PRIVATE void pheap_slot(int level)
{   (void)fprintf(stderr,"lib hseaplib %s: [ANSI C]\n",PHEAP_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(C) 2003-2022 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"PUPS/P3 persistent heap support library (built %s %s)\n\n",__TIME__,__DATE__);
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





/*-----------------------------------------------------------------------------------
    Initialise (local process) variables associated with persistent heap ...
-----------------------------------------------------------------------------------*/

_PUBLIC int msm_init(const int max_pheaps)

{   int i;


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

    _phobjects           = (int *)pups_calloc(max_pheaps,sizeof(int));
    _phobjects_allocated = (int *)pups_calloc(max_pheaps,sizeof(int));

    _pheap_parameters    = (unsigned long int **)pups_calloc(max_pheaps,sizeof(unsigned long int *));
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
    __phmalloc_initialized     = (int *)            pups_calloc(max_pheaps,sizeof(int));


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




/*-----------------------------------------------------------------------------------
    Extend (local process) variables associated with persistent heap ...
-----------------------------------------------------------------------------------*/

_PUBLIC int msm_extend(const unsigned int from_size, unsigned int to_size) 

{   int i;


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

    _phobjects                 = (int *)pups_realloc((void *)_phobjects,to_size*sizeof(int));
    _phobjects_allocated       = (int *)pups_realloc((void *)_phobjects_allocated,to_size*sizeof(int));

    _pheap_parameters          = (unsigned long int **)pups_realloc((void *)_pheap_parameters,to_size*sizeof(unsigned long int *));
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
    __phmalloc_initialized     = (int *)            pups_realloc((void *)__phmalloc_initialized,to_size*sizeof(int));

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




/*-----------------------------------------------------------------------------------
    Tell heap to destroy itself when we detach it ...
-----------------------------------------------------------------------------------*/

_PUBLIC int msm_set_autodestruct_state(const unsigned int hdes, const _BOOLEAN autodestruct)

{   

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */


    /*--------------*
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





/*-----------------------------------------------------------------------------------
    Return a handle to the persistent heap whose pathname is map_f_path. The flags
    specifies the (file) I/O mode the persistent heap, and mode specifies the level
    of file protection ...
-----------------------------------------------------------------------------------*/

_PUBLIC int msm_heap_attach(const char *map_f_path, const int attach_mode)

{   int i,
	flags,
        ret,
        start     = 0;

    size_t size,
           offset = 0L;

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
       {  int ret,
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
          {  unsigned long int magic,
                               htag;


             /*---------------------------------*/
             /* Check this is a valid heap file */
             /*---------------------------------*/

             if(pups_lseek(htable[i].fd,MAGIC_OFFSET*sizeof(unsigned long int),SEEK_SET) == (-1))
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

             (void)pups_pipe_read(htable[i].fd,&magic,sizeof(unsigned long int));


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

             (void)pups_pipe_read(htable[i].fd,&htag,sizeof(unsigned long int));
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

          htable[i].sdata = (unsigned long int)htable[i].addr + sizeof(unsigned long int);
          htable[i].edata = (unsigned long int)htable[i].addr + sizeof(unsigned long int);


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
                                                                                                          (unsigned long int)htable[i].segment_size,
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




/*----------------------------------------------------------------------------------------
    Detach a persistent heap -- if flags | O_DESTROY the persistent heap is unlinked
    and destroyed ...
----------------------------------------------------------------------------------------*/

_PUBLIC int msm_heap_detach(const unsigned int hdes, const int flags)

{   int  i;
    char args[SSIZE] = "";


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
                                                                                      (unsigned long int)htable[hdes].segment_size,
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




/*------------------------------------------------------------------------------------------
    sbrk on the persistent heap whose heap descriptor is hdes - size extra bytes will be
    mapped into the address spaces of the processes attached to the heap ...
------------------------------------------------------------------------------------------*/

_PUBLIC void *msm_sbrk(const unsigned int hdes, const size_t size)

{   void *ptr = (void *)NULL;

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

    brk_core_size   = htable[hdes].edata - htable[hdes].sdata - sizeof(int) + size;

    if(brk_core_size > htable[hdes].segment_size)
       brk_core_needed = brk_core_size -  htable[hdes].segment_size;
    else
       brk_core_needed = 0;

    if(brk_core_needed > 0)
    {  int n_brk_segments;

       unsigned long int seg_used;
       void              *old_addr = (void *)NULL;
       long              offset    = 0L;

       #ifdef PHEAP_DEBUG
       print_heaptables(hdes,"BEFORE EXTEND",(unsigned long int)htable[hdes].addr);
       #endif /* PHEAP_DEBUG */


       /*----------------------------------------------------------------------*/
       /* Get number of SBRK segments currently in use by this persistent heap */
       /*----------------------------------------------------------------------*/

       seg_used = htable[hdes].edata - htable[hdes].sdata   - (unsigned long)sizeof(int); 


       /*------------------------------------------------------------------*/
       /* Extend backing store (file) object - note that we do this BEFORE */
       /* the heap is flushed so that the isync to pull the heap back into */
       /* local address space after remapping sees the adjusted segment    */
       /* size                                                             */
       /*------------------------------------------------------------------*/

       old_segment_size          =  htable[hdes].segment_size;
       n_brk_segments            =  (int)ceil((double)brk_core_needed / (double)PHM_SBRK_SIZE);
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

       n_brk_segments            =  (int)ceil((double)brk_core_needed / (double)PHM_SBRK_SIZE);
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
      (void)fprint_heaptables(hdes,"AFTER EXTEND",(unsigned long int)htable[hdes].addr);
      #endif /* PHEAP_DEBUG */
    } 

    if(size > 0)
    {  ptr                        =  (void *)(htable[hdes].edata);
       htable[hdes].edata         += size;
    }
    else
    {  htable[hdes].edata         -= size;
       ptr                        =  (void *)(htable[hdes].edata);
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




/*---------------------------------------------------------------------------------------------
    Routine to stat a heap (in a similar manner to stat on a file) ...
---------------------------------------------------------------------------------------------*/

_PUBLIC int msm_hstat(const unsigned int hdes, heap_type *heapinfo)


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




/*-----------------------------------------------------------------------------------------------
    Find first free named persistent object slot in the map area ...
-----------------------------------------------------------------------------------------------*/

_PUBLIC int msm_get_free_mapslot(const unsigned int hdes)

{   int i,
        h_index;


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




/*-----------------------------------------------------------------------------------------------
    Find mapped object (returning its index) ...
-----------------------------------------------------------------------------------------------*/

_PUBLIC int msm_find_mapped_object(const unsigned int hdes, const void *ptr)

{   int i;


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




/*-----------------------------------------------------------------------------------------------
    Add object to the persitent heap object map ...
-----------------------------------------------------------------------------------------------*/

_PUBLIC int msm_map_object(const unsigned int hdes, const unsigned int h_index, const void *ptr, const char *name)

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





/*-----------------------------------------------------------------------------------------------
    Remove persisent object from object map ...
-----------------------------------------------------------------------------------------------*/

_PUBLIC int msm_unmap_object(const unsigned int hdes, const unsigned int h_index)

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




/*-----------------------------------------------------------------------------------------------
    Translate persistent object name to index ...
-----------------------------------------------------------------------------------------------*/

_PUBLIC int msm_map_objectname2index(const unsigned int hdes, const char *name)

{   int i;


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





/*-----------------------------------------------------------------------------------------------
    Translate persistent object address to index ...
-----------------------------------------------------------------------------------------------*/

_PUBLIC int msm_map_objectaddr2index(const unsigned int hdes, const void *addr)

{   int i;


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




/*-----------------------------------------------------------------------------------------------
    Translate persistent object name to address ...
-----------------------------------------------------------------------------------------------*/

_PUBLIC void *msm_map_objectname2addr(const unsigned int hdes, const char *name)

{   int  i;
    void *addr = (void *)NULL;


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
 




/*-----------------------------------------------------------------------------------------------
    Translate persistent object index to name ...
-----------------------------------------------------------------------------------------------*/

_PUBLIC char *map_objectindex2name(const unsigned int hdes, const unsigned int h_index)

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




/*-----------------------------------------------------------------------------------------------
    Translate persistent object index to address ...
-----------------------------------------------------------------------------------------------*/

_PUBLIC char *map_objectindex2addr(const unsigned int hdes, const unsigned int h_index)

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




/*------------------------------------------------------------------------------------------------
    Make sure that a mapname is unique ...
------------------------------------------------------------------------------------------------*/

_PUBLIC int msm_mapname_unique(const unsigned int hdes, const char *name)

{   return(msm_map_objectname2index(hdes,name));
}
  



/*------------------------------------------------------------------------------------------------
    Set the info field for a mapped object ...
------------------------------------------------------------------------------------------------*/

_PUBLIC int msm_map_setinfo(const unsigned int hdes, const unsigned int h_index, const char *info)

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





/*------------------------------------------------------------------------------------------------
    Set the size field for a mapped object ...
------------------------------------------------------------------------------------------------*/

_PUBLIC int msm_map_setsize(const unsigned int hdes, const unsigned int h_index, size_t size)

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




/*------------------------------------------------------------------------------------------------
    Display statistics of object on persistent heap ...
------------------------------------------------------------------------------------------------*/

_PUBLIC int msm_show_mapped_object(const unsigned int hdes, const unsigned int h_index, const FILE *stream)

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
    (void)fprintf(stream,"    object location       :  %016lx virtual\n",(unsigned long int)_phobjectmap[hdes][h_index]->addr);
    (void)fprintf(stream,"    object size           :  %016lx bytes\n\n",_phobjectmap[hdes][h_index]->size);
     
    (void)fflush(stream);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}
 




/*------------------------------------------------------------------------------------------------
    Display objects on persistent heap ...
------------------------------------------------------------------------------------------------*/

_PUBLIC int msm_show_mapped_objects(const unsigned int hdes, const FILE *stream)

{   int i,
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
                                                               (unsigned long int)htable[hdes].addr);
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
                                                      (unsigned long int)_phobjectmap[hdes][i]->addr,
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

    
          



/*------------------------------------------------------------------------------------------------
    Translate persistent heap hdes (heap descriptor) to name ...
------------------------------------------------------------------------------------------------*/

_PUBLIC char *msm_hdes2name(const unsigned int hdes)

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




/*------------------------------------------------------------------------------------------------
    Translate persistent heap name to hdes (heap descriptor) ...
------------------------------------------------------------------------------------------------*/

_PUBLIC int msm_name2hdes(const char *name)

{   int i;


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




/*-----------------------------------------------------------------------------------------------
    Make heap table addresses process local ...
-----------------------------------------------------------------------------------------------*/

_PUBLIC int msm_isync_heaptables(const unsigned int hdes)

{    unsigned long int offset;
     char          info[SSIZE] = "";


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

     offset = (unsigned long int)htable[hdes].addr;

     _pheap_parameters[hdes]      = (unsigned long int *)((unsigned long int)sizeof(long)              + offset);


     _pheapindex[hdes]            = _pheap_parameters[hdes][0];
     _pheap_bytes_used[hdes]      = _pheap_parameters[hdes][1];
     _pheap_bytes_free[hdes]      = _pheap_parameters[hdes][2];
     _pheap_chunks_used[hdes]     = _pheap_parameters[hdes][3];
     _pheap_chunks_free[hdes]     = _pheap_parameters[hdes][4];
     pheapsize[hdes]              = _pheap_parameters[hdes][5];
     _pheaplimit[hdes]            = _pheap_parameters[hdes][6];
     _pheapinfo[hdes]             = (malloc_info *)((unsigned long int)_pheap_parameters[hdes][7]     + offset);
     _phobjects[hdes]             = _pheap_parameters[hdes][8];
     _phobjects_allocated[hdes]   = _pheap_parameters[hdes][9];
     _phobjectmap[hdes]           = (phobmap_type  **)((unsigned long int)_pheap_parameters[hdes][10] + offset);
     _pheapbase[hdes]             = (char           *)((unsigned long int)_pheap_parameters[hdes][11] + offset);
     htable[hdes].sdata           = _pheap_parameters[hdes][15]                                       + offset;
     htable[hdes].edata           = _pheap_parameters[hdes][16]                                       + offset;
     htable[hdes].segment_size    = _pheap_parameters[hdes][17];
     htable[hdes].heapmagic       = _pheap_parameters[hdes][18];
     htable[hdes].ptrsize         = _pheap_parameters[hdes][20];


     /*--------------------------------------------------------------------------*/
     /* Map all addresses in persistent memory into the address space of the the */
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

 


/*-----------------------------------------------------------------------------------------------
    Make heap table addresses global ...
-----------------------------------------------------------------------------------------------*/

_PUBLIC int msm_sync_heaptables(const unsigned int hdes)

{    unsigned long int offset;
     char              info[SSIZE] = "";


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

     offset = (unsigned long int)htable[hdes].addr;


     /*----------------------------------------------------------------*/
     /* Map heap parameter table addresses into global address space   */
     /* of persistent heap                                             */
     /*----------------------------------------------------------------*/

     _pheap_parameters[hdes][0]    = _pheapindex[hdes];
     _pheap_parameters[hdes][1]    = _pheap_bytes_used[hdes];
     _pheap_parameters[hdes][2]    = _pheap_bytes_free[hdes];
     _pheap_parameters[hdes][3]    = _pheap_chunks_used[hdes];
     _pheap_parameters[hdes][4]    = _pheap_chunks_free[hdes];
     _pheap_parameters[hdes][5]    = pheapsize[hdes];
     _pheap_parameters[hdes][6]    = _pheaplimit[hdes];
     _pheap_parameters[hdes][7]    = (long int)((unsigned long int)_pheapinfo[hdes]     - offset);
     _pheap_parameters[hdes][8]    = _phobjects[hdes];
     _pheap_parameters[hdes][9]    = _phobjects_allocated[hdes];
     _pheap_parameters[hdes][10]   = (long int)((unsigned long int)_phobjectmap[hdes]   - offset);
     _pheap_parameters[hdes][11]   = (long int)((unsigned long int)_pheapbase[hdes]     - offset);
     _pheap_parameters[hdes][15]   = htable[hdes].sdata                                 - offset;
     _pheap_parameters[hdes][16]   = htable[hdes].edata                                 - offset;
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

     _pheap_parameters[hdes]     = (unsigned long int *)((unsigned long int)sizeof(long) - offset);

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_unlock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */

     pups_set_errno(OK);
     return(0);
}




/*-----------------------------------------------------------------------------------------------
    Display heap information ...
-----------------------------------------------------------------------------------------------*/

_PUBLIC int msm_print_heaptables(const FILE *stream, const unsigned int hdes, const char *info, const size_t offset)

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
     (void)fprintf(stream,"    heap info         : %010x\n",      (unsigned long int)_pheap_parameters[hdes][7]);
     (void)fprintf(stream,"    heap objects      : %04d\n",       _pheap_parameters[hdes][8]);
     (void)fprintf(stream,"    heap object slots : %04d\n",       _pheap_parameters[hdes][9]);
     (void)fprintf(stream,"    heap object table : %016lx\n",     (unsigned long int)_pheap_parameters[hdes][10]);
     (void)fprintf(stream,"    heap base         : %016lx\n\n",   (unsigned long int)_pheap_parameters[hdes][11]);
     (void)fprintf(stream,"    heap clients      : %04d\n",       _pheap_parameters[hdes][12]);
     (void)fprintf(stream,"    heap client slots : %04d\n",       _pheap_parameters[hdes][13]);
     (void)fprintf(stream,"    heap client table : %016lx\n",     (unsigned long int)_pheap_parameters[hdes][14]);
     (void)fprintf(stream,"    heap base         : %016lx\n",     (unsigned long int)_pheap_parameters[hdes][15]);
     (void)fprintf(stream,"    heap top          : %016lx\n",     (unsigned long int)_pheap_parameters[hdes][16]);
     (void)fprintf(stream,"    heap segment size : %016lx\n",     (unsigned long int)_pheap_parameters[hdes][17]);
     (void)fprintf(stream,"    heapmagic         : %016lx\n",     (unsigned long int)_pheap_parameters[hdes][18]);
     (void)fprintf(stream,"    heap vtag         : %04d\n",       (unsigned long int)_pheap_parameters[hdes][19]);
     (void)fprintf(stream,"    heap address size : %04d (bits)\n",(unsigned long int)_pheap_parameters[hdes][20]);
     (void)fflush(stream);

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_unlock(&htab_mutex);
     #endif /* PTHREAD_SUPPORT */

     pups_set_errno(OK);
     return(0);
} 





/*-----------------------------------------------------------------------------------------------
    Relocate persistent heap (after munmap/mmap extension remapping) ...
-----------------------------------------------------------------------------------------------*/


_PRIVATE int msm_relocate_heap_addresses(int hdes, unsigned long int offset, int offset_op)

{   int  i;
 

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
    {  (void)fprintf(stderr,"    heap %04d mapped into process memory map at %016lx virtual\n",hdes,offset);
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
                {  _phobjectmap[hdes][i]       = (phobmap_type *)((unsigned long int)_phobjectmap[hdes][i] + offset); 
                   _phobjectmap[hdes][i]->addr = (void *)((unsigned long int)_phobjectmap[hdes][i]->addr   + offset);
                }
             }
             else
             {  _phobjectmap[hdes][i]->addr = (void *)((unsigned long int)_phobjectmap[hdes][i]->addr   - offset);
                _phobjectmap[hdes][i]       = (phobmap_type *)((unsigned long int)_phobjectmap[hdes][i] - offset); 
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




/*-----------------------------------------------------------------------------------------------
    Convert free block list from global to local addressing  ...
-----------------------------------------------------------------------------------------------*/

_PRIVATE void global_to_local_blocklist(int hdes, unsigned long int offset)

{   int    i;
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
       {  next = (struct list *)((unsigned long int)next + offset);

          if(fraghead_address == TRUE)
          {  _phfraghead[hdes][i].next = next;

             #ifdef PHEAP_DEBUG
             (void)fprintf(stderr,"log: %04d pointer %016lx\n",i,(unsigned long int)_phfraghead[hdes][i].next);
             (void)fflush(stderr);
             #endif /* PHEAP_DEBUG */
          }


          /*-----------------------------------------------------*/ 
          /* "walk" the free block list relocating its addresses */
          /*-----------------------------------------------------*/ 

          while(next->next != (struct list *)NULL)
          {  next->next = (struct list *)((unsigned long int)next->next + offset);
             
             if(next->prev != (struct list *)NULL)
             {  if(fraghead_address == TRUE)
                {  //next->prev->next = next + offset;
                   fraghead_address = FALSE;
                }
                else
                   next->prev = (struct list *)((unsigned long int)next->prev + offset);
             }


             /*---------------------------------*/ 
             /* Step to next free block in list */
             /*---------------------------------*/ 

             next = next->next;
          }
       }
    }
} 




/*-----------------------------------------------------------------------------------------------
    Convert free block list from local to global addressing  ...
-----------------------------------------------------------------------------------------------*/

_PRIVATE void local_to_global_blocklist(int hdes, unsigned long int offset)

{   int    i;
    struct list *next = (struct list *)NULL,
                *step = (struct list *)NULL;

    _BOOLEAN    fraghead_address;

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
             next->next = (struct list *)((unsigned long int)next->next - offset);
 
             if(next->prev != (struct list *)NULL)
             {  if(fraghead_address == TRUE)
                   next->prev->next = next - offset; 
                else
                   next->prev = (struct list *)((unsigned long int)next->prev - offset);
             }


             /*---------------------------------*/ 
             /* Step to next free block in list */
             /*---------------------------------*/ 

             next = (struct list *)((unsigned long)next - offset);

             if(fraghead_address == TRUE)
             {  fraghead_address = FALSE;
                _phfraghead[hdes][i].next = next;

                #ifdef PHEAP_DEBUG 
                (void)fprintf(stderr,"log: %04d pointer %016lx\n",i,(unsigned long int)_phfraghead[hdes][i].next);
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
          {  _phfraghead[hdes][i].next = (struct list *)((unsigned long int)next - offset);
 
             #ifdef PHEAP_DEBUG
             (void)fprintf(stderr,"log: %04d pointer %016lx\n",i,(unsigned long int)_phfraghead[hdes][i].next);
             (void)fflush(stderr);
             #endif /* PHEAP_DEBUG */
          }
       }   
    }   
}




/*------------------------------------------------------------------------------------------------
    Persistent heap exit function (which detaches any persistent heaps) ...
------------------------------------------------------------------------------------------------*/

_PUBLIC void msm_exit(const unsigned int flags)

{   int i,
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





/*-------------------------------------------------------------------------------------------------
    Check to see if an object already exists ...
-------------------------------------------------------------------------------------------------*/

_PUBLIC _BOOLEAN msm_phobject_exists(const unsigned int hdes, const char *name)

{   int i;

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




/*-------------------------------------------------------------------------------------------------
    Make sure heap addresses are in either absolute (local) or relative ...
-------------------------------------------------------------------------------------------------*/

_PUBLIC int msm_map_address_mode(const unsigned int hdes, const unsigned int mode)

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




/*-------------------------------------------------------------------------------------------------
    Translate heap name to heap table index ...
-------------------------------------------------------------------------------------------------*/

_PUBLIC int msm_name2index(const char *name)

{   int i;


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




/*-------------------------------------------------------------------------------------------------
    Translate heap fdes to heap table index ...
-------------------------------------------------------------------------------------------------*/

_PUBLIC int msm_fdes2hdes(const int fdes)

{   int i;


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




/*-------------------------------------------------------------------------------------------------
    Check to see if an address lies within persistent heap ...
-------------------------------------------------------------------------------------------------*/

_PUBLIC _BOOLEAN msm_addr_in_heap(const unsigned int hdes, const void *ptr)

{   unsigned long int addr;


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

    addr = (unsigned long int)ptr;
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




/*------------------------------------------------------------------------------------------------
    Resolve address pointer on persistent heap into address space of calling process ...
------------------------------------------------------------------------------------------------*/

_PUBLIC void *msm_map_haddr_to_paddr(const unsigned int hdes, const void *ptr)

{   unsigned long int addr;


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

    addr = (unsigned long int)ptr;


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



 
/*------------------------------------------------------------------------------------------------
    Resolve address pointer on persistent heap into address space of calling process ...
------------------------------------------------------------------------------------------------*/

_PUBLIC void *msm_map_paddr_to_haddr(const unsigned int hdes, const void *ptr)

{   unsigned long int addr;


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

    addr = (unsigned long int)ptr;


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

_PUBLIC int msm_save_heapstate(const char *ssave_dir)

{   int i;


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
/*---------------------------------------------------------------------------
     Allocate memory on a page boundary.
     Copyright (C) 1991, 1992, 1993, 1994 Free Software Foundation, Inc.

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
     (C) 1998-2022 M.A. O'Neill, Tumbling Dice
---------------------------------------------------------------------------*/

#ifndef GLIBC
#if defined (__GNU_LIBRARY__) || defined (_LIBC)
#include <stddef.h>
#include <sys/cdefs.h>
extern size_t __getpagesize __P ((void));
#else
#define	 __getpagesize()	getpagesize()
#endif /* defined (__GNU_LIBRARY__) || defined (_LIBC) */
#endif /* GLIBC                                        */


#ifndef	_PHMALLOC_INTERNAL
#define	_PHMALLOC_INTERNAL
#include <phmalloc.h>
#endif /* _PHMALLOC_INTERNAL */
#include <xtypes.h>


_PRIVATE __malloc_size_t pagesize;

_PUBLIC __ptr_t phvalloc (const unsigned int hdes, const __malloc_size_t size, const char *name)
{

  if(pagesize == 0)
    pagesize = __getpagesize();

  return phmemalign (hdes, pagesize, size, name);
}
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
   (C) 1998-2022 M.A. O'Neill, Tumbling Dice
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

  /*----------------------------------------------------------------------------*/
  /* If we are importing a persistent heap which already exists                 */
  /* we need to pull all of the parameters required by the                      */
  /* memory allocator from the persistent heap. Mark O'Neill 14th November 1997 */
  /*----------------------------------------------------------------------------*/

  if(htable[hdes].exists == FALSE)
  {

     /*-----------------------------------------------------------------------*/
     /* Before doing anything else grab some memory on the persistent heap to */
     /* store parameters. Mark O'Neill 14th November 1997                     */
     /*-----------------------------------------------------------------------*/

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
/*--------------------------------------------------------------------------------------------------------
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
    (C) 1998-2022 M.A. O'Neill, Tumbling Dice
-------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <xtypes.h>

#ifndef	_PHMALLOC_INTERNAL
#define _PHMALLOC_INTERNAL
#include <phmalloc.h>
#endif /* _PHMALLOC_INTERNAL */


_IMPORT heap_type         *htable;
_IMPORT __malloc_size_t   *pheapsize;


/*--------------------------------------------------*/
/* Add a persistent object to persistent object map */
/*--------------------------------------------------*/

_PROTOTYPE _EXTERN int msm_map_object(const unsigned int, const unsigned int, const void *, const char *);


/*-----------------------------------------------------------------------------------*/
/* Remove a persistent object persistent object map within persistent memory segment */
/*-----------------------------------------------------------------------------------*/

_PROTOTYPE _EXTERN int msm_unmap_object(const unsigned int, const unsigned int);


/*-------------------------*/
/* Debugging hook for free */
/*-------------------------*/

_PUBLIC void (*__phfree_hook) __P ((const unsigned int, const __ptr_t __ptr));


/*---------------------------------------*/
/* List of blocks allocated by memalign. */
/*---------------------------------------*/

_PUBLIC struct alignlist *_aligned_blocks = NULL;


/*-----------------------*/
/* Object mapping switch */
/*-----------------------*/

_PUBLIC int _no_phobject_mapping;


/*---------------------------*/
/* Number of objects in heap */
/*---------------------------*/

_IMPORT int *_phobjects;


/*------------------------------------------------------------*/
/* Return memory to the heap.                                 */
/* Like `free' but don't call a __free_hook if there is one.  */
/*------------------------------------------------------------*/

_PUBLIC void _phfree_internal (const unsigned int hdes, const __ptr_t ptr)
{
  int type,
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
      /* Determine how to link this block into the free list.  */
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
	  /* Really link this block back into the free list.  */
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
	  _pheapinfo[hdes][block].busy.info.frag.first = (unsigned long int)
	  ((unsigned long int) ((char *) ptr - (char *) NULL) % BLOCKSIZE >> type);

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

void *phfree (const unsigned int hdes, const __ptr_t ptr)

{ struct alignlist *l;

  if (ptr == NULL)
    return;

  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_lock(&htab_mutex);
  #endif /* PTHREAD_SUPPORT */

  for (l = _aligned_blocks; l != NULL; l = l->next)
    if (l->aligned == ptr)
      {

                                /*-----------------------------------*/
	l->aligned = NULL;	/* Mark the slot in the list as free */
                                /*-----------------------------------*/

	ptr = l->exact;
	break;
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
/*---------------------------------------------------------------------------
    Copyright (C) 1991, 1993, 1994 Free Software Foundation, Inc.
    This file is part of the GNU C Library.

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

    Shared heap code by Mark O'Neill (mao@tumblingdice.co.uk)
    (C) 1998-2022 M.A. O'Neill, Tumbling Dice
--------------------------------------------------------------------------*/

#ifndef	_PHMALLOC_INTERNAL
#define _PHMALLOC_INTERNAL
#include <phmalloc.h>
#endif /* _PHMALLOC_INTERNAL */

#include <xtypes.h>

#ifdef _LIBC
#include <ansidecl.h>
#include <gnu-stabs.h>
#undef	cfree

function_alias(cfree, free, void, (ptr), DEFUN(cfree, (ptr), PTR ptr))

#else

_PUBLIC void phcfree (int hdes,  __ptr_t ptr)
{   phfree (hdes, ptr);
}

#endif /* LIBC */
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
/*---------------------------------------------------------------------------
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
   (C) 1998-2022 M.A. O'Neill, Tumbling Dice
--------------------------------------------------------------------------*/

#include <errno.h>

#ifndef	_PHMALLOC_INTERNAL
#define	_PHMALLOC_INTERNAL
#include <phmalloc.h>
#endif /* SHMALLOC_INTERNAL */

#include <xtypes.h>


/*-----------------------------------------------------------*/
/* Allocate an array of NMEMB elements each SIZE bytes long. */
/* The entire array is initialized to zeros.                 */
/*-----------------------------------------------------------*/

_PUBLIC __ptr_t phcalloc (const unsigned int hdes, const __malloc_size_t nmemb, const __malloc_size_t size, const char *name)
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
