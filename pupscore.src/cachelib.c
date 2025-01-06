/*----------------------------------
    Purpose: Fast cache library

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             NE3 54RT
             Tyne and Wear

    Version: 4.19
    Dated:   10th October 2024 
    Email:   mao@tumblingdice.co.uk
----------------------------------*/

#include <me.h>
#include <utils.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <bsd/bsd.h>


/*-------------------------------------------------------------*/
/* If we have OpenMP support then we also have pthread support */
/* (OpenMP is built on pthreads for Linux distributions).      */
/*-------------------------------------------------------------*/

#if defined(_OPENMP) && !defined(PTHREAD_SUPPORT)
#define PTHREAD_SUPPORT
#endif /* PTHREAD_SUPPORT */


#ifdef PTHREAD_SUPPORT
#include <tad.h>
#endif /* PTHREAD_SUPPORT */


#undef  __NOT_LIB_SOURCE__
#include <cache.h>
#define __NOT_LIB_SOURCE__


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


/*----------------------------------------------*/
/* Get application information for slot manager */
/*----------------------------------------------*/
/*---------------------------*/
/* Slot information function */
/*---------------------------*/

_PRIVATE void cache_slot(int32_t level)
{   (void)fprintf(stderr,"lib cachelib %s: [ANSI C]\n",CACHELIB_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(c) 2001-2024 Tumbling Dice, Gosforth\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"PUPS/P3 fast caching library (gcc %s: built %s %s)\n",__VERSION__,__TIME__,__DATE__);
       (void)fflush(stderr);
    }

    (void)fprintf(stderr,"\n");
    (void)fflush(stderr);
}


/*------------------------------------------*/
/* Segment identification for cache library */
/*------------------------------------------*/

#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT )() __attribute__ ((aligned(16))) = cache_slot;
#endif /* SLOT */




/*--------------------------------------------*/
/* Variables which are private to this module */
/*--------------------------------------------*/
/*-------------*/
/* Cache table */
/*-------------*/

_PRIVATE _BOOLEAN cache_table_initialised = FALSE;
_PRIVATE          cache_type cache[MAX_CACHES];


/*-------------------------------------------*/
/* Function which are private to this module */
/*-------------------------------------------*/

// Print number of bytes in cache
_PRIVATE int32_t print_bytes(FILE *,
                             const char *,
                             const uint64_t);

// Inversely map existing process memory  int32_to a
// ghost file, synchronising it with memory area
_PRIVATE void *mmap_invmap_cachememory(uint32_t      ,
                                       const char   *,
                                       const  int32_t,
                                       const uint64_t);

// Memory map cache directly into the address space
// of the program
_PRIVATE void *mmap_fwdmap_cachememory(uint32_t      ,
                                       const char   *,
                                       const  int32_t,
                                       const uint64_t);

// Map cache memory
_PRIVATE void *cache_mmap_cachememory (uint32_t     ,
                                       const char  *,
                                       const int32_t,
                                       const uint64_t,
                                       uint64_t     *);




/*--------------------------------------------------*/
/* Memory map cache directly  int32_to the address space */
/* of the program                                   */
/*--------------------------------------------------*/

_PRIVATE void *mmap_fwdmap_cachememory(const uint32_t           c_index,  // Cache index
                                       const char       *cachefile_name,  // Cache file name
                                       const  int32_t         h_p_state,  // Hoemostatic protection state
                                       const uint64_t              size)  // Size of cache

{   des_t   fd;
    int32_t map_flags = 0;

    void *cache_ptr = (void *)NULL;


    /*-------------------------------------------------*/
    /* File permissions must match memory mapping mode */
    /*-------------------------------------------------*/
    /*---------------------*/
    /* Error - permissions */
    /*---------------------*/

    if((fd = pups_open(cachefile_name,O_RDWR,h_p_state)) == (-1))
    {  (void)snprintf("[mmap_fwdmap_cachememory] cannot open cachefile \"%s\"",SSIZE,cachefile_name);
       pups_error(errstr);
    }


    /*-------------------------*/
    /* Error - cache is in use */
    /*-------------------------*/

    else
    {  if(lockf(fd,F_TLOCK,0) == (-1))
       {  (void)snprintf("[mmap_fwdmap_cachememory] cachefile \"%s\" is in use",SSIZE,cachefile_name);
          pups_error(errstr);
       }
    }


    /*-------------------*/
    /* Read only mapping */
    /*-------------------*/

    if(cache[c_index].mmap & CACHE_PRIVATE)
       map_flags = MAP_PRIVATE;


    /*--------------------*/
    /* Read/write mapping */
    /*--------------------*/

    else if(cache[c_index].mmap & CACHE_PUBLIC)
       map_flags = MAP_SHARED;


    /*-----------------------------------------*/
    /* Populate mapping to prevent page faults */
    /*-----------------------------------------*/

    if(cache[c_index].mmap & CACHE_POPULATE)
       map_flags |= MAP_POPULATE;

    if((cache_ptr = mmap(NULL,
                         size,
                         PROT_READ  | PROT_WRITE,
                         map_flags,
                         fd,
                         0L)) == (void *)MAP_FAILED)
    {  (void)pups_close(fd);
       (void)snprintf(errstr,SSIZE,"[mmap_fwdmap_cachememory] cannot map cachfile \"%s\"  into process address space",cachefile_name);
       pups_error(errstr);  
    }

    cache[c_index].mmap_fd = fd;
    (void)strlcpy(cache[c_index].mmap_name,cachefile_name,SSIZE);

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): mapping to cachefile \"%s\" (at %016lx virtual), %d\n",
                     date,appl_name,appl_pid,appl_host,appl_owner,cachefile_name,(uint64_t)cache_ptr,errno);
       (void)fflush(stderr);
    }

    pups_set_errno(OK);
    return(cache_ptr);
}




/*-----------------------------------------------*/
/* Inversely map existing process memory into a  */
/* ghost file, synchronising it with memory area */
/*-----------------------------------------------*/

_PRIVATE void *mmap_invmap_cachememory(const uint32_t          c_index,  // Cache index
                                       const char      *cachefile_name,  // Name of cache file
                                       const int32_t         h_p_state,  // Homeostatic protection state
                                       uint64_t                   size)  // Size of cache

{   int32_t map_flags = 0;
    des_t   fd        = (-1);

    void *cache_ptr = (void *)NULL;


    /*----------------------------------------------*/
    /* Creat a ghost file of the right size for the */
    /* cache and then map cache                     */
    /*----------------------------------------------*/

    if(access(cachefile_name,F_OK | R_OK | W_OK) == (-1))
    {  if(pups_creat(cachefile_name,0600) == (-1))
       {  (void)snprintf(errstr,SSIZE,"[mmap_invmap_cachememory] cannot create cachefile \"%s\"",cachefile_name);
          pups_error(errstr);
       }
    }


    /*-------------------*/
    /* Error permissions */
    /*-------------------*/

    if((fd = pups_open(cachefile_name,O_RDWR,h_p_state)) == (-1))
    {  (void)snprintf(errstr,SSIZE,"[mmap_invmap_cachememory] cannot open cachefile \"%s\"",cachefile_name);
       pups_error(errstr);
    }


    /*-------------------------*/
    /* Error - cache is in use */
    /*-------------------------*/

    else
    {  if(lockf(fd,F_TLOCK,0) == (-1))
       {  (void)snprintf(errstr,SSIZE,"[mmap_invmap_cachememory] cannot open cachefile \"%s\"",cachefile_name);
          pups_error(errstr);
       }
    }


    /*---------------*/
    /* Set file size */
    /*---------------*/
 
    (void)ftruncate(fd,size);
    (void)posix_fallocate(fd, 0, size);


    /*-------------------*/
    /* Read only mapping */
    /*-------------------*/

    if(cache[c_index].mmap & CACHE_PRIVATE)
       map_flags = MAP_PRIVATE;


    /*--------------------*/
    /* Read/write mapping */
    /*--------------------*/

    else if(cache[c_index].mmap & CACHE_PUBLIC)
       map_flags = MAP_SHARED;


    /*-----------------------------------------*/
    /* Populate mapping to prevent page faults */
    /*-----------------------------------------*/

    if(cache[c_index].mmap & CACHE_POPULATE)
       map_flags |= MAP_POPULATE;


    if((cache_ptr = mmap(NULL,
                         size,
                         PROT_READ  | PROT_WRITE,
                         map_flags,
                         fd,
                         0L)) == (void *)MAP_FAILED)
   {    (void)pups_close(fd);
        (void)snprintf(errstr,SSIZE,"[mmap_invmap_cachememory] cannot map cachefile \"%s\"  into process address space",cachefile_name);
        pups_error(errstr);
   }


   /*------------------------------------------*/
   /* Tell Kernel we will be accessing cache   */
   /* and it would be a good idea to read some */
   /* pages ahead                              */
   /*------------------------------------------*/

   cache[c_index].mmap_fd = fd;
   (void)strlcpy(cache[c_index].mmap_name,cachefile_name,SSIZE);

   if(appl_verbose == TRUE)
   {  (void)strdate(date);
      (void)fprintf(stderr,"%s %s (%d@%s:%s): inverse mapping to cachefile \"%s\" (at %016lx virtual)\n",
                    date,appl_name,appl_pid,appl_host,appl_owner,cachefile_name,(uint64_t)cache_ptr);
      (void)fflush(stderr);
   }

   pups_set_errno(OK);
   return(cache_ptr);
}




/*-------------------------------------*/
/* Return the index of a (named) cache */
/*-------------------------------------*/

_PUBLIC int32_t cache_name2index(const _BOOLEAN have_cache_lock, const char *name)

{   uint32_t i;

    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(name == (const char *)NULL)
       pups_error("[cache_name2index] expecting cache name");


    for(i=0; i<MAX_CACHES; ++i)
    {  
       #ifdef PTHREAD_SUPPORT
       if(have_cache_lock == FALSE)
          (void)pthread_mutex_lock(&cache[i].mutex);
       #endif /* PTHREAD_SUPPORT */

       if(strcmp(cache[i].name,name) == 0)
       {
          #ifdef PTHREAD_SUPPORT
          if(have_cache_lock == FALSE)
             (void)pthread_mutex_unlock(&cache[i].mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);  
          return(i);
       }

       #ifdef PTHREAD_SUPPORT
       if(have_cache_lock == FALSE)
          (void)pthread_mutex_unlock(&cache[i].mutex);
       #endif /* PTHREAD_SUPPORT */
    }

    (void)snprintf(errstr,SSIZE,"[cache_name2index] cannot find cache \"%s\"",name);
    pups_error(errstr);
 
    return(-1);
}



/*------------------------------*/
/* Map cache memory - create an */
/* inverse map if backing file  */
/* does not exit                */
/*------------------------------*/

_PRIVATE void *cache_mmap_cachememory(const uint32_t     c_index,   // Cache index
                                      const char      *file_name,   // Cache file name
                                      const int32_t    h_p_state,   // Homeostatic protection state
                                      const uint64_t        size,   // Cache size
                                      uint64_t              *crc)   // Cache (64 bit) CRC 

{   void *cache_ptr = (void *)NULL;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[cache_mmap_cachememory] attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    /*--------------------------*/
    /* Create and map new cache */
    /*--------------------------*/
    
    if(access(file_name,F_OK | R_OK | W_OK) == (-1))
       cache_ptr = mmap_invmap_cachememory(c_index,file_name,h_p_state,size);


    /*--------------------*/
    /* Map existing cache */
    /*--------------------*/

    else
    {  uint64_t tmp_crc,
                old_size;


       /*---------------------------------*/
       /* Check CRC (is cache corrupted?) */
       /*---------------------------------*/

       cache_ptr = mmap_fwdmap_cachememory(c_index,file_name,h_p_state,size);


       /*---------------------------------*/
       /* Check CRC (is cache corrupted?) */
       /*---------------------------------*/

       if(crc != (uint64_t *)NULL)
       {  tmp_crc   = pups_crc_64(size,cache_ptr);

          if(tmp_crc != 0x0 && cache[c_index].crc != 0x0 && cache[c_index].crc != tmp_crc)
          {

             /*----------------------*/
             /* Unmap the cache and  */
             /* return CRC to caller */
             /*----------------------*/

             cache[c_index].cache_ptr = cache_ptr; 
             (void)cache_destroy(TRUE,FALSE,c_index); 

             *crc = 0x0;

             pups_set_errno(EINVAL);
             return((void *)NULL);
          }


          /*----------------------*/
          /* Return CRC to caller */
          /*----------------------*/

          else if(crc != (void *)NULL)
             *crc = tmp_crc;
       }
    }

    pups_set_errno(OK);
    return(cache_ptr);
}



/*------------------*/
/* Initialise cache */
/*------------------*/

_PUBLIC int32_t cache_table_init(void)

{    int32_t i,
             c_index;

    char mmap_file_name[SSIZE] = "";


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[cache_table_init] attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    /*--------------------------------*/
    /* Initialise cache table entries */
    /*--------------------------------*/

    if(cache_table_initialised == FALSE)
    {  for(i=0; i<MAX_CACHES; ++i)
       {   uint32_t j;
           pthread_mutexattr_t attr;

           (void)strlcpy(cache[i].path        ,"" ,SSIZE);
           (void)strlcpy(cache[i].name        ,"", SSIZE);
           (void)strlcpy(cache[i].mapinfo_name,"", SSIZE);
           (void)strlcpy(cache[i].mmap_name   ,"", SSIZE);
           (void)strlcpy(cache[i].auxinfo     ,"", SSIZE);

           (void)pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
           (void)pthread_mutex_init(&cache[i].mutex,&attr);

           cache[i].mapinfo_fd = (-1);
           cache[i].mmap_fd    = (-1);
           cache[i].mmap       = FALSE;
           cache[i].u_blocks   = 0;
           cache[i].n_blocks   = 0;
           cache[i].n_objects  = 0;
           cache[i].block_size = 0L;
           cache[i].cache_size = 0L;
           cache[i].crc        = 0L;
           cache[i].colsize    = 0;

           for(j=0; j<MAX_CACHE_BLOCK_OBJECTS; ++j)
           {  cache[i].object_size[i]   = 0L;
              cache[i].object_offset[i] = 0L;
              (void)strlcpy(cache[i].object_desc[i],"none",SSIZE);
           }

           cache[i].blockmap     = (block_mtype       *)NULL;
           cache[i].cache_ptr    = (void              *)NULL;
           cache[i].flags        = (_BYTE             *)NULL;
           cache[i].tag          = (uint32_t          *)NULL;
           cache[i].lifetime     = (uint64_t          *)NULL;
           cache[i].hubness      = (uint32_t          *)NULL;
           cache[i].rwlock       = (pthread_rwlock_t  *)NULL;
       }

       cache_table_initialised = TRUE;
    }

    pups_set_errno(OK);
    return(0);
}




/*---------------------*/
/* Add object to cache */
/*---------------------*/

_PUBLIC int32_t cache_add_object(const _BOOLEAN  have_cache_lock,  // If TRUE lock on cache held
                                 const char                *desc,  // Description of object
                                 const uint64_t             size,  // Size of object (bytes)
                                 const uint32_t          c_index)  // Cache index (identifier)

{   

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[cache_add_object] attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_add_object] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    if(size == 0)
      pups_error("[cache_add_object] object has zero size");


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    if(cache[c_index].n_objects > MAX_CACHE_BLOCK_OBJECTS)
       pups_error("[cache_add_object] too many objects in cache");


    /*--------------------*/
    /* Object description */
    /*--------------------*/

    if(desc != (char *)NULL)
       (void)strlcpy(cache[c_index].object_desc[cache[c_index].n_objects],desc,SSIZE);


    /*----------------------------------------------*/
    /* Set object size and increment object counter */
    /*----------------------------------------------*/

    cache[c_index].object_size[cache[c_index].n_objects] = size;
    ++cache[c_index].n_objects;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0); 
}





/*--------------------------*/
/* Get machine architecture */
/*--------------------------*/

_PRIVATE int32_t get_march(char *march)

{    struct utsname buf;

     /*--------------------------*/
     /* get machine architecture */
     /*--------------------------*/

     (void)uname(&buf);
     (void)strlcpy(march,buf.machine,SSIZE);

     pups_set_errno(OK);
     return(0);
}





/*----------------------------------------------------*/
/* Creat a dynamic multiple block cache with multiple */
/* objects per block                                  */
/*----------------------------------------------------*/

_PUBLIC int32_t cache_create(const _BOOLEAN  have_cache_lock,  // Lock held on cache if TRUE
                             const uint32_t             mmap,  // Memory mapping operation
                             const char                *name,  // Cache name
                             int32_t                n_blocks,  // Number of blocks in cache
                             uint64_t                   *crc,  // Cache (64 bit) CRC
                             const uint32_t          c_index)  // Cache indentifier

{    int32_t i,
             h_p_state      = DEAD;

    uint64_t current_offset = 0L;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[cache_create] attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    /*--------------*/
    /* Sanity check */
    /*--------------*/
    /*---------*/
    /* No name */
    /*---------*/

    if(name == (const char *)NULL)
       pups_error("[cache_create] expecting cache name");


    /*-------------*/
    /* Range error */
    /*-------------*/
 
    else if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_create] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */


    /*----------------------*/
    /* Cache already in use */
    /*----------------------*/

    else if(cache[c_index].cache_ptr != (void *)NULL)
       pups_error("[cache_create] cache is already in use");


    /*----------------*/
    /* Allocate cache */
    /*----------------*/

    if((mmap & CACHE_USE_MAPINFO) == 0)
    {  char path[SSIZE]      = "",
            basename[SSIZE]  = "";


       /*---------------------------------*/
       /* Sanity check (number of blocks) */
       /*---------------------------------*/

       if(n_blocks <= 0)
          pups_error("[cache_create] number of blocks in cache <= 0");


       /*----------------*/
       /* Set cache path */
       /*----------------*/

       (void)strbranch(name,path);
       (void)strlcpy (cache[c_index].path,path,SSIZE);


       /*----------------*/
       /* Set cache name */
       /*----------------*/

       (void)strleaf(name,basename);
       (void)strtrnc(basename,'.',1);
       (void)strlcpy (cache[c_index].name,basename,SSIZE);

       cache[c_index].mmap = CACHE_MMAP;


       /*-----------------*/
       /* Private mapping */
       /*-----------------*/

       if(mmap & CACHE_PRIVATE)
          cache[c_index].mmap |= CACHE_PRIVATE;


       /*----------------*/
       /* Public mapping */
       /*----------------*/

       else if(mmap & CACHE_PUBLIC)
          cache[c_index].mmap |= CACHE_PUBLIC;


       /*---------*/
       /* Default */
       /*---------*/

       else
          cache[c_index].mmap |= CACHE_PUBLIC;


       /*-----------------*/
       /* Preload mapping */
       /*-----------------*/

       if(mmap & CACHE_POPULATE)
          cache[c_index].mmap |= CACHE_POPULATE;


       /*----------------------*/
       /* No preloaded mapping */
       /*----------------------*/

       else if(mmap & CACHE_DEPOPULATE)
          cache[c_index].mmap |= CACHE_DEPOPULATE;


       /*---------*/
       /* Default */
       /*---------*/

       else
          cache[c_index].mmap != CACHE_DEPOPULATE;


       /*------------------------*/
       /* Homeostatic protection */
       /*------------------------*/

       if(mmap & CACHE_LIVE)
       {  cache[c_index].mmap |= CACHE_LIVE; 
          h_p_state = LIVE;
       }
    }


    /*-----------------------------*/
    /* Always honour populate and  */
    /* populate flags irrespective */
    /* of how we get other cache   */
    /* parameters                  */
    /*-----------------------------*/

    else
    {  cache[c_index].mmap = CACHE_MMAP;


       /*-----------------*/
       /* Private mapping */
       /*-----------------*/

       if(mmap & CACHE_PRIVATE)
          cache[c_index].mmap |= CACHE_PRIVATE;


       /*----------------------*/
       /* Public mapping */
       /*----------------------*/

       else if(mmap & CACHE_PUBLIC)
          cache[c_index].mmap |= CACHE_PUBLIC; 


       /*------------*/
       /* Preloading */
       /*------------*/

       if(mmap & CACHE_POPULATE)
          cache[c_index].mmap |= CACHE_POPULATE;


       /*---------------*/
       /* No preloading */
       /*---------------*/

       else if(mmap & CACHE_DEPOPULATE)
          cache[c_index].mmap |= CACHE_DEPOPULATE; 


       /*------------------------*/
       /* Homeostatic protection */
       /*------------------------*/

       if(mmap & CACHE_LIVE)
       {  cache[c_index].mmap |= CACHE_LIVE; 
          h_p_state = LIVE;
       }
    }


    /*-----------------*/
    /* Build new cache */
    /*-----------------*/

    if((mmap & CACHE_USE_MAPINFO) == 0)
    {

       /*---------------------------------------------*/
       /* Compute offsets of all blocks and mark them */
       /* unused and accessible                       */
       /*---------------------------------------------*/

       for(i=0; i<=cache[c_index].n_objects; ++i)
       {  current_offset                  += cache[c_index].object_size[i-1];
          cache[c_index].object_offset[i]  = current_offset;    
       }


       /*----------------------*/
       /* Allocate block flags */
       /*----------------------*/

       if(cache[c_index].flags == (_BYTE *)NULL)
          cache[c_index].flags = (_BYTE *)pups_calloc(n_blocks,sizeof(_BYTE));

       for(i=0; i<n_blocks; ++i)
           cache[c_index].flags[i] = 0;


       /*---------------------*/
       /* Allocate block tags */
       /*---------------------*/

       if(cache[c_index].tag == (uint32_t *)NULL)
          cache[c_index].tag = (uint32_t  *)pups_calloc(n_blocks,sizeof(uint32_t));

       for(i=0; i<n_blocks; ++i)
           cache[c_index].tag[i] = 0;


       /*-------------------*/
       /* Allocate lifetime */
       /*-------------------*/

       if(cache[c_index].lifetime == (uint64_t *)NULL)
          cache[c_index].lifetime = (uint64_t  *)pups_calloc(n_blocks,sizeof(uint64_t));

       for(i=0; i<n_blocks; ++i)
           cache[c_index].lifetime[i] = BLOCK_IMMORTAL;


       /*------------------*/
       /* Allocate hubness */
       /*------------------*/

       if(cache[c_index].hubness == (uint32_t *)NULL)
          cache[c_index].hubness = (uint32_t  *)pups_calloc(n_blocks,sizeof(uint32_t ));

       for(i=0; i<n_blocks; ++i)
           cache[c_index].hubness[i] = 0;


       /*------------------*/
       /* Allocate binding */
       /*------------------*/

       if(cache[c_index].binding == (uint32_t *)NULL)
          cache[c_index].binding = (uint32_t  *)pups_calloc(n_blocks,sizeof(uint32_t));

       for(i=0; i<n_blocks; ++i)
           cache[c_index].binding[i] = 0;


       /*------------------------*/
       /* Allocate block rwlocks */
       /*------------------------*/

       if(cache[c_index].rwlock == (pthread_rwlock_t *)NULL)
          cache[c_index].rwlock = (pthread_rwlock_t *)pups_calloc(n_blocks,sizeof(pthread_rwlock_t));

       for(i=0; i<n_blocks; ++i)
           (void)pthread_rwlock_init(&cache[c_index].rwlock[i],(pthread_rwlockattr_t *)NULL);


       /*-----------------------------------------*/
       /* Get offsets  into cache block (in bytes) */
       /*-----------------------------------------*/
       /*---------------------------------------*/
       /* Size of entire cache block (in bytes) */
       /*---------------------------------------*/

       cache[c_index].block_size = current_offset + cache[c_index].object_size[cache[c_index].n_objects-1];


       /*---------------------------------*/
       /* Size of entire cache (in bytes) */
       /*---------------------------------*/

       cache[c_index].n_blocks   = n_blocks;
       cache[c_index].cache_size = cache[c_index].n_blocks*cache[c_index].block_size;


       /*-----------------------------------------------------*/
       /* Record architecture of machine used to create cache */
       /*-----------------------------------------------------*/

       (void)get_march(cache[c_index].march);
    }


    /*--------------------*/
    /* Map existing cache */
    /*--------------------*/

    else
    {  char my_march[SSIZE] = "";


       /*------------------------------------*/
       /* Check that the architecture used   */
       /* to create cache is compatible with */
       /* the one running calling process    */
       /*------------------------------------*/

       (void)get_march(my_march);
       if(strcmp(my_march,cache[c_index].march) != 0)
       {  (void)snprintf(errstr,SSIZE,"cache create] cache was created on \"%s\", but current architecture is \"%s\")",cache[c_index].march,my_march);
           pups_error(errstr);
       }
    }


    /*---------------------------------------------*/
    /* Allocate contiguous memory for entire cache */
    /*---------------------------------------------*/
    /*----------------------------------------------*/
    /* Map cache segment into process address space */
    /*----------------------------------------------*/
    /*--------------------------------*/
    /* Check for errors mapping cache */
    /*--------------------------------*/ 

    if((cache[c_index].cache_ptr = (void *)cache_mmap_cachememory(c_index,
                                                                  name,
                                                                  h_p_state,
                                                                  cache[c_index].n_blocks*cache[c_index].block_size,
                                                                                                               crc)) == (void *)NULL)

    {

        /*---------------------------------------*/
        /* If we are not returning CRC to caller */
        /* this is a fatal error                 */
        /*---------------------------------------*/

        if(crc == (void *)NULL)
           pups_error("[cache_create] failed to map cache  into process address space");


        /*----------------------*/
        /* Return CRC to caller */
        /*----------------------*/

        else
        {  pups_set_errno(EINVAL);
           return(-1);
        }
    }


    /*----------------------------------*/
    /* Allocate cache block pointer map */
    /*----------------------------------*/

    if(cache[c_index].blockmap == (block_mtype *)NULL)
       cache[c_index].blockmap = (block_mtype *)pups_calloc(cache[c_index].n_blocks,sizeof(block_mtype));


    /*-----------------------------------------*/
    /* Map object pointers within cache blocks */
    /*-----------------------------------------*/

    for(i=0; i<cache[c_index].n_blocks; ++i)
    {  uint32_t j;

       for(j=0; j<cache[c_index].n_objects; ++j)                                                          /*----------------------------*/
          cache[c_index].blockmap[i].object_ptr[j] = (void *)(cache[c_index].object_offset[j]          +  /* Object offset within block */
                                                     (uint64_t         )i*cache[c_index].block_size    +  /* Block offset within cache  */
                                                     (uint64_t         )cache[c_index].cache_ptr);        /* Base address of cache      */
                                                                                                          /*----------------------------*/
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*------------*/
/* Sync cache */
/*------------*/

_PUBLIC int32_t cache_msync(const _BOOLEAN have_cache_lock, const uint32_t c_index)

{
    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */


    /*------------------------------------*/
    /* Only sychronise cache if it public */
    /*------------------------------------*/

    if((cache[c_index].mmap & CACHE_PRIVATE) == 0)
    {
       /*-------------------------------------*/
       /* Synchronise cache with disk image   */
       /* and block until operation completes */
       /*-------------------------------------*/

       (void)msync((void *)cache[c_index].cache_ptr,
                   cache[c_index].cache_size,
                   MS_SYNC | MS_INVALIDATE);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */
}




/*------------------------------------------------------*/
/* Make all file descriptors associated with cache live */
/* (i.e. homeostatic)                                   */
/*------------------------------------------------------*/

_PUBLIC int32_t cache_live(const _BOOLEAN have_cache_lock, const uint32_t c_index)

{    int32_t h_p_level_1,
             h_p_level_2;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[cache destroy] attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_destroy] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */


    /*-----------------*/
    /* Make cache live */
    /*-----------------*/

    if((h_p_level_1 = pups_fd_alive(cache[c_index].mmap_fd,   "pups_default_fd_homeostat",&pups_default_fd_homeostat)) == (-1))
    {  
       #ifdef PTHREAD_SUPPORT
       if(have_cache_lock == FALSE)
          (void)pthread_mutex_unlock(&cache[c_index].mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EINVAL);
       return(-1);
    }


    /*------------------------*/
    /* Make mapinfo file live */
    /*------------------------*/

    if((h_p_level_2 = pups_fd_alive(cache[c_index].mapinfo_fd,"pups_default_fd_homeostat",&pups_default_fd_homeostat)) == (-1)  ||
        h_p_level_2 != h_p_level_1                                                                                               )
    {
       #ifdef PTHREAD_SUPPORT
       if(have_cache_lock == FALSE)
          (void)pthread_mutex_unlock(&cache[c_index].mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EINVAL);
       return(-1);
    }


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(h_p_level_1);
}




/*------------------------------------------------------*/
/* Make all file descriptors associated with cache dead */
/* (i.e. non homeostatic)                               */
/*------------------------------------------------------*/

_PUBLIC int32_t cache_dead(const _BOOLEAN have_cache_lock, const uint32_t c_index)

{    int32_t h_p_level_1,
             h_p_level_2;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[cache destroy] attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_destroy] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */


    /*------------------*/
    /* Make cache dead  */
    /*------------------*/

    if((h_p_level_1 = pups_fd_dead(cache[c_index].mmap_fd)) == (-1))
    {
       #ifdef PTHREAD_SUPPORT
       if(have_cache_lock == FALSE)
          (void)pthread_mutex_unlock(&cache[c_index].mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EINVAL);
       return(-1);
    }


    /*-------------------------*/
    /* Make  mapinfo file dead */
    /*-------------------------*/

    if((h_p_level_2 = pups_fd_dead(cache[c_index].mapinfo_fd)) == (-1)  ||
        h_p_level_1 != h_p_level_2                                       )
    {
       #ifdef PTHREAD_SUPPORT
       if(have_cache_lock == FALSE)
          (void)pthread_mutex_unlock(&cache[c_index].mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EINVAL);
       return(-1);
    }



    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(h_p_level_1);
}



/*----------------------------------------*/
/* Destroy cache (optionally deleting it) */
/*----------------------------------------*/

_PUBLIC int32_t cache_destroy(const _BOOLEAN have_cache_lock, const _BOOLEAN delete_cache, const uint32_t c_index)

{   uint32_t i; 


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[cache destroy] attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_destroy] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */


    /*----------------------------*/
    /* Free (mapped) cache memory */
    /*----------------------------*/

    if(cache[c_index].cache_ptr != (void *)NULL)
    { 

       /*-------------*/
       /* Unmap cache */
       /*-------------*/

       if(munmap(cache[c_index].cache_ptr,cache[c_index].cache_size) == (-1))
         pups_error("[cache_destroy] cannot unmap cache");


       (void)strlcpy(cache[c_index].mapinfo_name,"",SSIZE);
       (void)strlcpy(cache[c_index].mmap_name,   "",SSIZE);
       

       /*-------------------------------*/
       /* Release lock on (mapped) file */
       /*-------------------------------*/

       (void)lockf(cache[c_index].mmap_fd,F_ULOCK,0);


       /*----------------------------------*/
       /* Close descriptor to mapinfo file */
       /*----------------------------------*/

       (void)pups_close(cache[c_index].mapinfo_fd);


       /*----------------------------------*/
       /* Close descriptor to mapping file */
       /*----------------------------------*/

       (void)pups_close(cache[c_index].mmap_fd);

       cache[c_index].mmap_fd = (-1);
       cache[c_index].mmap    = 0;


       /*--------------------------------*/
       /* Delete cache if asked to do so */
       /*--------------------------------*/

       if(delete_cache == TRUE)
       {  (void)unlink(cache[c_index].mapinfo_name);
          (void)unlink(cache[c_index].mmap_name);
       }
    }

    /*--------------------------------*/
    /* Clear cache datastructures and */
    /* free allocated memory          */
    /*--------------------------------*/

    (void)strlcpy(cache[c_index].path,"",SSIZE);
    (void)strlcpy(cache[c_index].name,"",SSIZE);

    cache[c_index].u_blocks   = 0;
    cache[c_index].n_blocks   = 0;
    cache[c_index].n_objects  = 0;
    cache[c_index].block_size = 0L;

    for(i=0; i<MAX_CACHE_BLOCK_OBJECTS; ++i)
    {  cache[c_index].object_size[i]   = 0L;
       cache[c_index].object_offset[i] = 0L;
    }


    /*---------------*/
    /* Free blockmap */
    /*---------------*/

    if(cache[c_index].blockmap != (block_mtype *)NULL)
    {  (void)pups_free((void *)cache[c_index].blockmap);
       cache[c_index].blockmap = (block_mtype *)NULL;
    }

    cache[c_index].cache_ptr  = (void *)NULL;
    cache[c_index].cache_size = 0L;


    /*------------------*/
    /* Free block flags */
    /*------------------*/

    if(cache[c_index].flags != (_BYTE *)NULL)
    {  (void)pups_free((void *)cache[c_index].flags);
       cache[c_index].flags = (_BYTE *)NULL;
    }


    /*-----------------*/
    /* Free block tags */
    /*-----------------*/

    if(cache[c_index].tag != (uint32_t *)NULL)
    {  (void)pups_free((void *)cache[c_index].tag);
       cache[c_index].tag = (uint32_t  *)NULL;
    }


    /*---------------------*/
    /* Free block lifetime */
    /*---------------------*/

    if(cache[c_index].lifetime != (int64_t *)NULL)
    {  (void)pups_free((void *)cache[c_index].lifetime);
       cache[c_index].lifetime = (uint64_t *)NULL;
    }


    /*--------------------*/
    /* Free block hubness */
    /*--------------------*/

    if(cache[c_index].hubness != (uint32_t *)NULL)
    {  (void)pups_free((void *)cache[c_index].hubness);
       cache[c_index].hubness = (uint32_t  *)NULL;
    }


    /*--------------------*/
    /* Free block binding */
    /*--------------------*/

    if(cache[c_index].binding != (uint32_t *)NULL)
    {  (void)pups_free((void *)cache[c_index].binding);
       cache[c_index].binding = (uint32_t  *)NULL;
    }


    /*---------------------------*/
    /* Free block access rwlocks */
    /*---------------------------*/

    if(cache[c_index].rwlock != (pthread_rwlock_t *)NULL)
    {  (void)pups_free((void *)cache[c_index].rwlock);
       cache[c_index].rwlock = (pthread_rwlock_t *)NULL;
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*-------------------*/
/* Detach all caches */
/*-------------------*/

_PUBLIC void cache_exit(void)

{   uint32_t i;


    /*---------------------------------------*/
    /* Loop over all caches detaching those  */
    /* which are in use. We don't care who   */
    /* has the lock as we will be destroying */
    /* the cache anyway                      */
    /*---------------------------------------*/

    for(i=0; i<MAX_CACHES; ++i)
    {  if(cache[i].cache_ptr != (void *)NULL)
          (void)cache_destroy(TRUE,FALSE,i);
    }
}




/*----------------------------------*/
/* Print bytes in appropriate units */
/*----------------------------------*/

_PRIVATE  int32_t print_bytes(FILE           *stream,  // Status/logging stream
                              const char       *text,  // Test to be printed
                              const uint64_t    size)  // Number of bytes for formatted print

{   char   eff_text[SSIZE] = "";
    double fsize;

    if(text == (char *)NULL || strcmp(text,"") == 0)
      (void)strlcpy(eff_text,"size",SSIZE);
    else
      (void)strlcpy(eff_text,text,SSIZE);

    if(size > GIGABYTE)
    {  fsize = (double)size / (double)GIGABYTE;
       (void)fprintf(stream,"    %s:  %6.2f Gigabytes\n",eff_text,fsize);
    }
    else if(size > MEGABYTE)
    {  fsize = (double)size / (double)MEGABYTE;
       (void)fprintf(stream,"    %s:  %6.2f Megabytes\n",eff_text,fsize);
    }
    else if(size > KILOBYTE)
    {  fsize = (double)size / (double)KILOBYTE;
       (void)fprintf(stream,"    %s:  %6.2f Kilobytes\n",eff_text,fsize);
    }
    else
       (void)fprintf(stream,"    %s:  %6d bytes\n",      eff_text,size);

    pups_set_errno(OK);
    return(0);
}




/*--------------------------*/
/* Display cache statistics */
/*--------------------------*/

_PUBLIC  int32_t cache_display_statistics(const _BOOLEAN  have_cache_lock,  // TRUE if lock held on cache
                                          FILE                    *stream,  // Status/log stream 
                                          const uint32_t          c_index)  // Cache index

{    int32_t i,
             used   = 0,
             tagged = 0;

    char blockstr[SSIZE] = "";


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[cache_display_statistics] attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(stream == (const FILE *)NULL)
       pups_error("[cache_display_statistics] status/logging stream is not allocated");
    else if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_display_statistics] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */


    /*--------------------*/
    /* Name of this cache */
    /*--------------------*/

    (void)fprintf(stream,"\n    Cache statistics\n");
    (void)fprintf(stream,"    ================\n\n");
    (void)fprintf(stream,"    %-32s:  %d\n",                                       "cache identifier"             ,c_index);
    (void)fprintf(stream,"    %-32s:  %d\n",                                       "cache co-ordination list size",cache[c_index].colsize);
    (void)fprintf(stream,"    %-32s:  %016lx\n",                                   "cache 64 bit CRC"             ,cache[c_index].crc);
    (void)fprintf(stream,"    %-32s:  \"%s\"\n",                                   "cache path"                   ,cache[c_index].path);
    (void)fprintf(stream,"    %-32s:  \"%s\"\n",                                   "cache name"                   ,cache[c_index].name);
    (void)fprintf(stream,"    %-32s:  %016lx virtual\n",                           "cache located at"             ,(uint64_t         )cache[c_index].cache_ptr);
    (void)fprintf(stream,"    %-32s:  %s\n",                                       "cache (machine) architecture" ,cache[c_index].march);

    if(strcmp(cache[c_index].auxinfo,"") == 0)
       (void)fprintf(stream,"    %-32s:  %s\n",                                       "cache auxilliary data","none");
    else
       (void)fprintf(stream,"    %-32s:  %s\n",                                       "cache auxilliary data",cache[c_index].auxinfo);


    /*---------------------------*/
    /* Cache mapping information */
    /*---------------------------*/

    (void)fprintf(stream,"    %-32s:  memory mapped\n","cache type");
    (void)fprintf(stream,"    %-32s:  \"%-.48s.map\"\n", "cache mapinfo file",cache[c_index].mapinfo_name);


    /*------------------*/
    /* Hidden mmap file */
    /*------------------*/

    if(cache[c_index].mmap_name[0] == '.')
      (void)fprintf(stream,"    %-32s:  \"%-.48s\"\n",   "cache mmap file", &cache[c_index].mmap_name[1]);


    /*-------------------*/
    /* Visible mmap file */
    /*-------------------*/

    else
      (void)fprintf(stream,"    %-32s:  \"%-.48s\"\n",   "cache mmap file", cache[c_index].mmap_name);


    /*-----------------------------------*/
    /* Cache pages preloaded into memory */
    /* to reduce page faults             */
    /*-----------------------------------*/

    if(cache[c_index].mmap & CACHE_POPULATE)
       (void)fprintf(stream,"    %-32s:  enabled\n",              "cache preloading");
    else
       (void)fprintf(stream,"    %-32s:  disabled\n",             "cache preloading");


    /*---------------------------------------*/
    /* Cache mapped privately - this is used */
    /* when cache is readonly                */
    /*---------------------------------------*/

    if(cache[c_index].mmap & CACHE_PRIVATE)
       (void)fprintf(stream,"    %-32s:  private (read only)\n",  "cache access");
    else
       (void)fprintf(stream,"    %-32s:  public  (read/write)\n", "cache access");
    (void)fflush(stream);


    /*--------------------------*/
    /* Sizes of cache and block */
    /*--------------------------*/

    (void)fprintf(stream,"\n\n    Cache geometry\n");
    (void)fprintf(stream,"    ==============\n\n");
    (void)fflush(stream);

    (void)fprintf    (stream,"    %-32s:  %d blocks of %d objects (%d used)\n","cache format",cache[c_index].n_blocks,cache[c_index].n_objects,cache[c_index].u_blocks);
    (void)print_bytes(stream,"cache size                      ",            cache[c_index].cache_size);
    (void)print_bytes(stream,"block size                      ",            cache[c_index].block_size);


    /*---------------------------------------------*/
    /* Object sizes and offsets within cache block */ 
    /*---------------------------------------------*/

    (void)fprintf(stream,"\n\n    Block geometry\n");
    (void)fprintf(stream,"    ==============\n\n");
    (void)fflush(stream);


    /*--------------------------------*/
    /* Block object sizes and offsets */
    /*--------------------------------*/

    for(i=0; i<cache[c_index].n_objects; ++i)
    {  char objectstr[SSIZE] = "";


       /*----------------*/
       /* Object details */
       /*----------------*/

       (void)fprintf(stream,"    object [%04d] %s       :  %s\n",i,"description",cache[c_index].object_desc[i]);

       (void)snprintf(objectstr,SSIZE,"object [%04d] size              ",i); 
       (void)print_bytes(stream,objectstr,   cache[c_index].object_size[i]);
   
       (void)snprintf(objectstr,SSIZE,"object [%04d] block offset      ",i); 
       (void)print_bytes(stream,objectstr,cache[c_index].object_offset[i]);

       if(i < cache[c_index].n_objects - 1)
       {  (void)fprintf(stream,"\n");
          (void)fflush(stream);
       }
    }


    /*--------------------------------------*/
    /* Objects allocated within cache block */
    /*--------------------------------------*/

    if(cache[c_index].n_objects == 0)
       (void)fprintf(stream,"    no objects allocated\n");
    else
    {  if(cache[c_index].n_objects == 1)
          (void)fprintf(stream,"\n\n    %04d object in cache block\n\n",1);
       else
          (void)fprintf(stream,"\n\n    %04d objects in cache block\n\n",cache[c_index].n_objects);
    }
    (void)fflush(stream);


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*---------------------*/
/* Display cache table */
/*---------------------*/

_PUBLIC int32_t cache_display(const _BOOLEAN have_cache_lock, const FILE *stream)

{    int32_t i,
             mapped_caches    = 0;

    char mapoptstr[SSIZE]  = ""; 


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[cache_display] attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(stream == (const FILE *)NULL)
       pups_error("[cache_display] status/logging stream is not allocated");

    (void)fprintf(stream,"\n    Mapped data caches\n");
    (void)fprintf(stream,"    ==================\n\n");

    for(i=0; i<MAX_CACHES; ++i)
    {  double fsize;
       char   unitstr[SSIZE] = "";
 
       #ifdef PTHREAD_SUPPORT
       if(have_cache_lock == FALSE)
          (void)pthread_mutex_lock(&cache[i].mutex);
       #endif /* PTHREAD_SUPPORT */

       if(strcmp(cache[i].name,"") != 0)
       {  ++mapped_caches;


          /*------------------------------------------*/
          /* Get cache size in most appropriate units */
          /*------------------------------------------*/
          /*-----------*/
          /* Gigabytes */
          /*-----------*/

          if(cache[i].cache_size > GIGABYTE)
          {  fsize = (double)cache[i].cache_size / (double)GIGABYTE;
             (void)strlcpy(unitstr,"Gbytes",SSIZE);
          }

          /*-----------*/
          /* Megabytes */
          /*-----------*/

          else if(cache[i].cache_size > MEGABYTE)
          {  fsize = (double)cache[i].cache_size / (double)MEGABYTE;
             (void)strlcpy(unitstr,"Mbytes",SSIZE);
          }


          /*-----------*/
          /* Kilobytes */
          /*-----------*/

          else if(cache[i].cache_size > KILOBYTE)
          {  fsize = (double)cache[i].cache_size / (double)KILOBYTE;
             (void)strlcpy(unitstr,"Kbytes",SSIZE);
          }

          /*-------*/
          /* Bytes */
          /*-------*/

          else
          {  fsize = (double)cache[i].cache_size;
             (void)strlcpy(unitstr," Bytes",SSIZE);
          }


          /*---------------------------*/
          /* Get cache mapping options */
          /*---------------------------*/

          if(cache[i].mmap & CACHE_PRIVATE)
          {

              /*-----------------------------------------*/
              /* Cache private (read only) and preloaded */
              /*-----------------------------------------*/

              if(cache[i].mmap & CACHE_POPULATE)
                 (void)strlcpy(mapoptstr,"[private, preload]",SSIZE);


              /*------------------------------*/
              /* Cache private, no preloading */
              /*------------------------------*/

              else
                 (void)strlcpy(mapoptstr,"[private]",SSIZE);
          }


          /*-----------------------------------------*/
          /* Cache public (read/write) and preloaded */
          /*-----------------------------------------*/

          else if(cache[i].mmap & CACHE_POPULATE)
                 (void)strlcpy(mapoptstr,"[public, preload]",SSIZE);


          /*--------------*/
          /* Cache public */
          /*--------------*/

          else
                 (void)strlcpy(mapoptstr,"[public]",SSIZE);



          /*-------------------------------------------------*/
          /* Gbytes, Mbytes or Kbytes - use fractional units */
          /*-------------------------------------------------*/

          if(strcmp(unitstr," Bytes") != 0)
             (void)fprintf(stream,"    %04d: (\"%-24s\" path \"%-32s\"): %08d blocks, %7.3f %s mapped into process address space (at %016lx virtual) %s\n",
                                                                                                                                                         i,
                                                                                                                                             cache[i].name,
                                                                                                                                             cache[i].path,
                                                                                                                                         cache[i].n_blocks,
                                                                                                                                                     fsize,
                                                                                                                                                   unitstr,
                                                                                                                              (uint64_t)cache[i].cache_ptr,
                                                                                                                                                 mapoptstr);

          /*---------------------------*/
          /* Bytes - use integer units */
          /*---------------------------*/

          else
             (void)fprintf(stream,"    %04d: (\"%-32s\" path \"%-24s\"): %04d blocks, %04d %s mapped into process address space (at %016lx virtual) %s\n",
                                                                                                                                                        i,
                                                                                                                                            cache[i].name,
                                                                                                                                            cache[i].path,
                                                                                                                                        cache[i].n_blocks,
                                                                                                                                      cache[i].cache_size,
                                                                                                                                                  unitstr,
                                                                                                                             (uint64_t)cache[i].cache_ptr,
                                                                                                                                                mapoptstr);
          (void)fflush(stream);
       }

       #ifdef PTHREAD_SUPPORT
       if(have_cache_lock == FALSE)
         (void)pthread_mutex_unlock(&cache[i].mutex);
       #endif /* PTHREAD_SUPPORT */
    }

    if(mapped_caches == 0)
       (void)fprintf(stream,"    none (%d caches available)\n\n",MAX_CACHES);
    else if(mapped_caches == 1)
       (void)fprintf(stream,"\n\n    %04d cache mapped (%d caches available)\n\n",1,
                                                                     MAX_CACHES - 1);
    else
       (void)fprintf(stream,"\n\n    %04d cache mapped (%d caches available)\n\n",
                                         mapped_caches,MAX_CACHES - mapped_caches);
    (void)fflush(stream);

    pups_set_errno(OK);
    return(0);
}




/*------------------------------------*/
/* Return size of object within block */
/*------------------------------------*/

_PUBLIC int32_t cache_object_size(const _BOOLEAN  have_cache_lock,  // TRUE if lock held on cache
                                  const uint32_t      block_index,  // Block index (within cache) 
                                  const uint32_t     object_index,  // Object index (within block)
                                  const uint32_t          c_index)  // Cache index 

{   uint64_t object_size;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("cache_object_size] attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_object_size] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    if(block_index  > cache[c_index].n_blocks    ||
       object_index > cache[c_index].n_objects    )
    {  (void)snprintf(errstr,SSIZE,"[cache_object_size] block index range error [block index (%d) > max blocks (%d)\n",block_index,cache[c_index].n_objects);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_rwlock_rdlock(&cache[c_index].rwlock[block_index]);
    #endif /* PTHREAD_SUPPORT */

    object_size = cache[c_index].object_size[object_index];
  

    #ifdef PTHREAD_SUPPORT
    (void)pthread_rwlock_unlock(&cache[c_index].rwlock[block_index]);
    #endif /* PTHREAD_SUPPORT */

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(object_size);
}




/*---------------------------------------------------*/
/* Return (read only) pointer to object within block */
/*---------------------------------------------------*/

_PUBLIC const void *cache_access_object(const uint32_t  cache_lock_state,  // State of cache lock
                                        const uint32_t  block_locktype,    // Type of lock on accessed object (RDLOCK or WRLOCK)
                                        const uint32_t  block_index,       // Block index of access object 
                                        const uint32_t  object_index,      // Object index of accessed object
                                        const uint32_t  c_index)           // Cache index of accessed object

{   void *object_ptr = (void *)NULL;


    /*---------------*/
    /* Sanity checks */
    /*---------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_access_object] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    if(cache_lock_state != CACHE_LOCK && cache_lock_state != CACHE_HAVELOCK)
        pups_error("[cache_access_object] expecting cache lock state \"CACHE_LOCK\" or \"CACHE_HAVELOCK\"");

    if(block_locktype != BLOCK_WRLOCK && block_locktype != BLOCK_RDLOCK && block_locktype != BLOCK_HAVELOCK)
        pups_error("[cache_access_object] expecting lock type \"BLOCK_RDLOCK\", \"BLOCK_WRLOCK\", or \"BLOCK_HAVELOCK\"");

    #ifdef PTHREAD_SUPPORT
    if(cache_lock_state == CACHE_LOCK)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */


    /*-------------------------*/
    /* Block index range error */
    /*-------------------------*/

    if(block_index  >= cache[c_index].n_blocks)
    {  (void)snprintf(errstr,SSIZE,"[cache_access_object] block index range error [block index (%d) > max blocks (%d)\n",block_index,cache[c_index].n_blocks);
       pups_error(errstr);
    }


    /*--------------------------*/
    /* Object index range error */
    /*--------------------------*/

    if(object_index >= cache[c_index].n_objects)
    {  (void)snprintf(errstr,SSIZE,"[cache_access_object] object index range error [object index (%d) > max objects (%d)\n",object_index,cache[c_index].n_objects);
       pups_error(errstr);
    }


    /*---------------------*/
    /* Writelock on object */
    /*---------------------*/

    if(block_locktype == BLOCK_WRLOCK)
    {
       #ifdef PTHREAD_SUPPORT
       (void)pthread_rwlock_wrlock(&cache[c_index].rwlock[block_index]);
       #endif /* PTHREAD_SUPPORT */
    }


    /*--------------------*/
    /* Readlock on object */
    /*--------------------*/

    else if(block_locktype == BLOCK_RDLOCK)
    {
       #ifdef PTHREAD_SUPPORT
       (void)pthread_rwlock_rdlock(&cache[c_index].rwlock[block_index]);
       #endif /* PTHREAD_SUPPORT */
    }


    object_ptr = cache[c_index].blockmap[block_index].object_ptr[object_index]; 


    #ifdef CACHELIB_DEBUG
    (void)fprintf(stderr,"PTR %016lx CBLOCK %04d OBJECT %04d\n",(uint64_t)object_ptr,block_index,object_index);
    (void)fflush(stderr);
    #endif /* CACHELIB_DEBUG */

    pups_set_errno(OK);
    return(object_ptr);
}




/*------------------------*/
/* Map 2D array to object */
/*------------------------*/

_PUBLIC void **cache_map_2D_array(const void      *object_ptr,  // Pointer to cached object
                                  const uint32_t         rows,  // Number of rows in (2D) object matrix
                                  const uint32_t         cols,  // Number of cols in (2D) object matrix
                                  const uint64_t         size)  // Size of object

{    int32_t  i;
    void      **array_ptr = (void **)NULL;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[cache_map_2D_array] attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(object_ptr == (const void *) NULL)
       pups_error("[cache_map_2D_array] object pointer NULL");

    array_ptr = (void **)pups_calloc(rows,sizeof(void *));

    for(i=0; i<rows; ++i)
       array_ptr[i] = (void *)((uint64_t)object_ptr + (uint64_t)(i*cols)*size);

    pups_set_errno(OK);
    return(array_ptr);
}



/*---------------------------------*/
/* Write contents of cache to file */
/*---------------------------------*/

_PUBLIC int32_t cache_write(const des_t                  fd,  // File descriptor to write to
                            const _BOOLEAN  have_cache_lock,  // If TRUE lock held on cache
                            const uint32_t          c_index)  // Index of cache to be written

{   des_t eff_fd;
    char  eff_wcache_file_name[SSIZE] = "";


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[cache write] attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_write] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */ 


    /*-------------------------------*/
    /* Use specified file descriptor */
    /*-------------------------------*/

    if(fd != (-1))
       eff_fd = fd;


    /*-------------------------------------*/
    /* Use cache specified file descriptor */
    /*-------------------------------------*/

    else
       eff_fd = cache[c_index].mmap_fd;


    /*-----------------------------------*/
    /* Write (mapped) cache back         */
    /* making sue EINTR is taken care of */
    /*-----------------------------------*/

    (void)pups_lseek(eff_fd,SEEK_SET,0);


    /*------------------------------------*/
    /* Make sure the full file is written */
    /* and that EINTR is taken care of    */
    /*------------------------------------*/

    (void)pups_write(eff_fd,cache[c_index].cache_ptr,cache[c_index].cache_size);


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */ 

    pups_set_errno(OK);
    return(0);
}



/*---------------------------------*/
/* Get the size of a cached object */
/*---------------------------------*/

_PUBLIC int64_t  cache_get_object_size(const _BOOLEAN   have_cache_lock,  // If TRUE lock held on cache
                                       const uint32_t           c_index,  // Cache index
                                       const uint32_t           o_index)  // Object for which size is required

{    int64_t  size;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_get_object_size] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */ 


    /*--------------------------*/
    /* Object index range error */
    /*--------------------------*/

    if(o_index >= cache[c_index].n_objects)
    {  (void)snprintf(errstr,SSIZE,"[cache_get_object_size] object index range error [object index (%d) > max objects (%d)\n",o_index,cache[c_index].n_objects);
       pups_error(errstr);
    }
    else
       size = (int64_t )cache[c_index].object_size[o_index];

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */ 

    pups_set_errno(OK);
    return(size);
}




/*-------------------------------*/
/* Get the size of a cache block */
/*-------------------------------*/

_PUBLIC int64_t  cache_get_block_size(const _BOOLEAN have_cache_lock, const uint32_t  c_index)

{    int32_t  i;
     int64_t  size = 0L;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_get_block_size] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */ 

    for(i=0; i<cache[c_index].n_objects; ++i)
       size += (int64_t )cache[c_index].object_size[i];

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */ 

    pups_set_errno(OK);
    return(size);
}




/*---------*/
/* Taglist */
/*---------*/

_PRIVATE taglist_type taglist[MAX_TAGLIST_SIZE];

/*--------------------*/
/* Initialise taglist */
/*--------------------*/

_PRIVATE void taglist_init(void)

{   uint32_t i;

    for(i=0; i<MAX_TAGLIST_SIZE; ++i)
    {  taglist[i].tag           = (-1);
       taglist[i].tagged_blocks = 0;
    }
}


/*----------------------*/
/* Add entry to taglist */
/*----------------------*/

_PRIVATE  int32_t update_taglist(const int32_t tag)

{   uint32_t i;

    for(i=0; i<MAX_TAGLIST_SIZE; ++i)
    {  

       /*-----------------------------------------------*/
       /* Tag found so increment (tagged block) counter */
       /* and return to caller                          */
       /*-----------------------------------------------*/

       if(taglist[i].tag == tag)
       {  ++taglist[i].tagged_blocks;

          pups_set_errno(OK);
          return(i);
       }


       /*--------------------------------------*/
       /* Failed to find tag so add it to list */
       /*--------------------------------------*/

       else if(taglist[i].tag == (-1))
          break; 
    }


    /*--------------------*/
    /* No room at the Inn */
    /*--------------------*/

    if(i == MAX_TAGLIST_SIZE)
       pups_error("[update_taglist] too many tags in list");

    return(-1);
}




/*------------------------------------------------*/
/* Get statistics for all tags in specified cache */
/*------------------------------------------------*/

_PUBLIC int32_t cache_show_blocktag_stats(const _BOOLEAN have_cache_lock, FILE *stream, const int32_t c_index)

{   uint32_t i,
             tags          = 0,
             tagged_blocks = 0;


    /*---------------*/
    /* Sanity checks */
    /*---------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_show_blocktag_stats] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    if(stream == (FILE *)NULL)
       pups_error("[cache_show_blocktag_stats] status/log stream not specified");


    /*-------------------*/
    /* Initilise taglist */
    /*-------------------*/

    taglist_init();


    /*------------------------------------------*/
    /* Search specified cache for matching tags */
    /*------------------------------------------*/

    for(i=0; i<cache[c_index].n_blocks; ++i)
    {

       #ifdef PTHREAD_SUPPORT
       if(have_cache_lock == FALSE)
          (void)pthread_rwlock_rdlock(&cache[c_index].rwlock[i]);
       #endif /* PTHREAD_SUPPORT */

       if(cache[c_index].flags[i] & BLOCK_USED)
          (void)update_taglist(cache[c_index].tag[i]);

       #ifdef PTHREAD_SUPPORT
       if(have_cache_lock == FALSE)
          (void)pthread_rwlock_unlock(&cache[c_index].rwlock[i]);
       #endif /* PTHREAD_SUPPORT */
    }


    /*-------------------------------------------*/
    /* Report tag statistics for specified cache */
    /*-------------------------------------------*/

    for(i=0; i<MAX_TAGLIST_SIZE; ++i)
    {
       if(taglist[i].tag >= 0)
       {
          if(tags == 0)
          {  (void)fprintf(stream,"\n");
             (void)fflush(stream);
          }

          if(taglist[i].tag == SCRATCH_TAG)
             (void)fprintf(stream,"    tag scratch: %06d tagged blocks\n",taglist[i].tag,taglist[i].tagged_blocks);
          else
             (void)fprintf(stream,"    tag    %04d: %06d tagged blocks\n",taglist[i].tag,taglist[i].tagged_blocks);
 
          (void)fflush(stream);
          ++tags;
       }
    }

    if(tags > 1)
       (void)fprintf(stream,"\n    %d tag(s) [cache %d]\n",tags,c_index);
    else
       (void)fprintf(stream,"\n    no tags [cache %d]\n",tags,c_index);

    pups_set_errno(OK);
    return(0);
}




/*-------------------------------*/
/* Get number of blocks in cache */
/*-------------------------------*/

_PUBLIC int32_t cache_get_blocks(const _BOOLEAN have_cache_lock, const uint32_t c_index)

{    int32_t cache_blocks;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_get_blocks] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    cache_blocks = cache[c_index].n_blocks;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(cache_blocks);
}




/*------------------------------------------*/
/* Get number of objects per block in cache */
/*------------------------------------------*/

_PUBLIC int32_t cache_get_objects(const _BOOLEAN have_cache_lock, const uint32_t c_index)

{    int32_t block_objects;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_get_objects] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    block_objects = cache[c_index].n_objects;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(block_objects);
}




/*----------------*/
/* Get cache path */
/*----------------*/

_PUBLIC int32_t cache_index2path(const _BOOLEAN  have_cache_lock,  // If TRUE lock held on cache
                                 char                *cache_path,  // Path to cache
                                 const uint32_t          c_index)  // Index of cache

{

    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(cache_path == (const char *)NULL)
       pups_error("[cache_index2name] cache path is NULL");

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)strlcpy(cache_path,cache[c_index].path,SSIZE);

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    if(strcmp(cache_path,"") == 0)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    pups_set_errno(OK);
    return(0);
}




/*----------------*/
/* Get cache name */
/*----------------*/

_PUBLIC int32_t cache_index2name(const _BOOLEAN  have_cache_lock,  // If TRUE lock held on cache
                                 char                *cache_name,  // Name of cache
                                 const uint32_t          c_index)  // Index of cache

{

    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(cache_name == (const char *)NULL)
       pups_error("[cache_index2name] cache name is NULL");

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)strlcpy(cache_name,cache[c_index].name,SSIZE);

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    if(strcmp(cache_name,"") == 0)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    pups_set_errno(OK);
    return(0);
}




/*-----------------------*/
/* Get mapinfo file name */
/*-----------------------*/

_PUBLIC int32_t cache_get_mapinfo_name(const _BOOLEAN      have_cache_lock,  // If TRUE lock held on cache
                                       char            *cache_mapinfo_name,  // Name of cache mapping information file
                                       const uint32_t              c_index)  // Index of cache

{

    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(cache_mapinfo_name == (char *)NULL)
       pups_error("[cache_index2name] expecting mapinfo file name");

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)strlcpy(cache_mapinfo_name,cache[c_index].mapinfo_name,SSIZE);

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    if(strcmp(cache_mapinfo_name,"") == 0)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    pups_set_errno(OK);
    return(0);
}




/*-------------------*/
/* Get map file name */
/*-------------------*/

_PUBLIC int32_t cache_get_mmap_name(const _BOOLEAN    have_cache_lock,  // If TRUE lock held on cache
                                    char             *cache_mmap_name,  // Name of cache mapping file
                                    const uint32_t            c_index)  // Index of cache

{

    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(cache_mmap_name == (char *)NULL)
       pups_error("[cache_index2name] expecting map file name");


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)strlcpy(cache_mmap_name,cache[c_index].mmap_name,SSIZE);

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    if(strcmp(cache_mmap_name,"") == 0)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    pups_set_errno(OK);
    return(0);
}




/*-------------------------*/
/* Is cache memory mapped? */
/*-------------------------*/

_PUBLIC _BOOLEAN cache_is_mapped(const _BOOLEAN have_cache_lock, const uint32_t c_index)

{   _BOOLEAN mapped;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_is_mapped] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    mapped = cache[c_index].mmap;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(mapped);
}




/*---------------------------------------------------------*/
/* Is cache mapped at specified location (in file system)? */
/*---------------------------------------------------------*/

_PUBLIC _BOOLEAN cache_is_mapped_at(const _BOOLEAN have_cache_lock, const char *cache_pathname, const uint32_t c_index)

{   _BOOLEAN mapped = FALSE;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_is_mapped] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    if(strcmp(cache[c_index].mmap_name,cache_pathname) == 0)
       mapped = TRUE;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(mapped);
}




/*----------------------------*/
/* Is cache (already) loaded? */
/*----------------------------*/

_PUBLIC _BOOLEAN cache_already_loaded(const _BOOLEAN have_cache_lock, const char *cache_name, const uint32_t c_index, uint32_t *c_l_index)

{   uint32_t  i;
    _BOOLEAN  ret = FALSE;


    /*---------------*/
    /* Sanity checks */
    /*---------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_is_loaded] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    if(cache_name == (char *)NULL)
       pups_error("[cache_is_loaded] expecting cache name\n");


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */


    /*--------------------*/
    /* Search cache table */
    /*--------------------*/

    pups_set_errno(OK);
    for(i=0; i<MAX_CACHES; ++i)
    {  if(strcmp(cache[i].name,cache_name) == 0)
       {  ret = TRUE;

          if(c_l_index != (uint32_t *)NULL)
             *c_l_index = i;

          pups_set_errno(EINVAL);
          break;
       }
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    return(ret);
}




/*---------------------*/
/* Is cache allocated? */
/*---------------------*/

_PUBLIC void *cache_is_allocated(const _BOOLEAN have_cache_lock, const uint32_t c_index)

{   void *cache_ptr = (void *)NULL;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_is_allocated] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    cache_ptr = cache[c_index].cache_ptr;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(cache_ptr);
}




/*---------------------------------*/
/* Write cache mapping information */
/*---------------------------------*/

_PUBLIC uint64_t cache_write_mapinfo(const _BOOLEAN     try_homeostatic_protection,  // Try homeostatic mapfile protection if TRUE
                                     const _BOOLEAN                have_cache_lock,  // If TRUE lock held on cache
                                     const char                          *map_name,  // Name of file containing cache mapping info
                                     const uint32_t                          mmap,   // Memroy mapping flags
                                     const uint32_t                       c_index)   // Index of cached to be mapped

{    int32_t i,
             h_p_state           = DEAD;

    des_t fd                     = (-1);

    char map_path[SSIZE]         = "",
         map_pathname[SSIZE]     = "",
         cache_pathname[SSIZE]   = "",
         eff_cache_name[SSIZE]   = "",
         eff_mapfile_name[SSIZE] = "";

    _BOOLEAN is_live             = FALSE;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_write_mapinfo] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    if(map_name == (char *)NULL)
       pups_error("[cache_write_mapinfo] expecting mapinfo file name");


    /*---------------------------------*/
    /* Get map file path and leaf names */
    /*---------------------------------*/

    if(strin(map_name,"/") == TRUE)
    {  (void)strleaf(map_name,eff_mapfile_name);
       (void)strlcpy (map_path,map_name,SSIZE);
       (void)strtrnc(map_path,'/',1);
    }


    /*----------------------------------------------*/
    /* No path - just a leaf (in current directory) */
    /*----------------------------------------------*/

    else
       (void)strlcpy(eff_mapfile_name,map_name,SSIZE);


    /*--------------------------------------*/
    /* Generate map file name from basename */
    /*--------------------------------------*/

    (void)strtrnc(eff_mapfile_name,'.',1);
    (void)strlcpy (eff_cache_name,eff_mapfile_name,SSIZE);
    (void)strlcat (eff_mapfile_name,".map",SSIZE);


    /*----------------------------*/
    /* Generate map file pathname */
    /*----------------------------*/
    /*---------------------------*/
    /* Absolute or relative path */
    /*---------------------------*/

    if(strin(map_name,"/") == TRUE)
    {  (void)snprintf(map_pathname,SSIZE,  "%s/%s",map_path,eff_mapfile_name);
       (void)snprintf(cache_pathname,SSIZE,"%s/%s",map_path,eff_cache_name);
    }


    /*---------*/
    /* No path */
    /*---------*/

    else
    {  (void)strlcpy(map_pathname,  eff_mapfile_name,SSIZE);
       (void)strlcpy(cache_pathname,eff_cache_name,SSIZE);
    }
  
    if(access(map_pathname,F_OK | R_OK | W_OK) == (-1))
    {  if(pups_creat(map_pathname,0600) == (-1))
          pups_error("[cache_write_mapinfo] cannot create mapinfo file");
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */


    /*-----------------------------------------*/
    /* Only apply homeostatic protection if we */
    /* have not previously read map file       */
    /*-----------------------------------------*/

    if(mmap & CACHE_LIVE)
       h_p_state = LIVE;

    if(strcmp(cache[c_index].mmap_name,"") == 0)
    {

       /*---------------------------------*/
       /* Homeostatic map file protection */
       /*---------------------------------*/

       if(try_homeostatic_protection == TRUE)
       {

          /*----------------------------*/
          /* Is file already protected? */
          /*----------------------------*/

          if(pups_isalive(map_pathname) == FALSE) 
          {  if((fd = pups_open(map_pathname,O_WRONLY,h_p_state)) == (-1))
                pups_error("[cache_write_mapinfo] cannot open mapinfo file");

             is_live = TRUE;
          }


          /*------------------------------------------*/
          /* If it is don't protect it more than once */
          /*------------------------------------------*/

          else
          {  if((fd = pups_open(map_pathname,O_WRONLY,DEAD)) == (-1))
                pups_error("[cache_write_mapinfo] cannot open mapinfo file");
          }
       }


       /*------------------------------------*/
       /* No homeostatic map file protection */
       /*------------------------------------*/

       else
       {  if((fd = pups_open(map_pathname,O_WRONLY,DEAD)) == (-1))
              pups_error("[cache_write_mapinfo] cannot open mapinfo file");
       }
    }


    /*---------------------------------------------*/
    /* Homeostatic map file protection was decided */
    /* when map file was initially read            */
    /*---------------------------------------------*/

    else
    {  if((fd = pups_open(map_pathname,O_WRONLY,DEAD)) == (-1))
          pups_error("[cache_write_mapinfo] cannot open mapinfo file");
    }

    // Cache CRC
    cache[c_index].crc = pups_crc_64(cache[c_index].cache_size,cache[c_index].cache_ptr);
    if(pups_write(fd,(void *)&cache[c_index].crc,sizeof(uint64_t)) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Cache path
    if(pups_write(fd,map_path,256) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Cache name
    if(pups_write(fd,eff_cache_name,256) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Cache mapinfo file name
    if(pups_write(fd,map_pathname,256) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Cache mapfile name
    if(pups_write(fd,eff_mapfile_name,256) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Architecture of machine used to generate cache
    if(pups_write(fd,(void *)cache[c_index].march,256) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Auxilliary data
    if(pups_write(fd,(void *)cache[c_index].auxinfo,SSIZE) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }


    // Memory mapping flags  
    if(pups_write(fd,(void *)&cache[c_index].mmap,sizeof(uint32_t)) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Number of used blocks in cache
    if(pups_write(fd,(void *)&cache[c_index].u_blocks,sizeof(int32_t)) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }
 
    // Number of blocks in cache
    if(pups_write(fd,(void *)&cache[c_index].n_blocks,sizeof(int32_t)) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Number of objects per block
    if(pups_write(fd,(void *)&cache[c_index].n_objects,sizeof(int32_t)) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Cache size (bytes)
    if(pups_write(fd,(void *)&cache[c_index].cache_size,sizeof(uint64_t)) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Block size (bytes)
    if(pups_write(fd,(void *)&cache[c_index].block_size,sizeof(uint64_t)) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Co-ordination list size
    if(pups_write(fd,(void *)&cache[c_index].colsize,sizeof(uint32_t)) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Write object descriptions
    for(i=0; i<cache[c_index].n_objects; ++i)
    {  if(pups_write(fd,(void *)&cache[c_index].object_desc[i],256*sizeof(_BYTE)) == (-1))
        {  (void)pups_close(fd);
           goto error_exit;
        }
    }

    // Write offsets of objects in cache blocks (bytes)
    for(i=0; i<cache[c_index].n_objects; ++i)
    {   if(pups_write(fd,(void *)&cache[c_index].object_offset[i],sizeof(uint64_t)) == (-1))
        {  (void)pups_close(fd);
           goto error_exit;
        }
    }
 
    // Write sizes of objects in cache blocks (bytes)
    for(i=0; i<cache[c_index].n_objects; ++i) 
    {   if(pups_write(fd,(void *)&cache[c_index].object_size[i],sizeof(uint64_t)) == (-1))
        {  (void)pups_close(fd);
           goto error_exit;
        }
    }

    // Write cache block flags
    for(i=0; i<cache[c_index].n_blocks; ++i)
    {   if(pups_write(fd,(void *)&cache[c_index].flags[i],sizeof(_BYTE)) == (-1))
        {  (void)pups_close(fd);
           goto error_exit;
        }
    }

    // Write cache tags
    for(i=0; i<cache[c_index].n_blocks; ++i)
    {   if(pups_write(fd,(void *)&cache[c_index].tag[i],sizeof(uint32_t)) == (-1))
        {  (void)pups_close(fd);
           goto error_exit;
        }
    }

    // Write cache lifetimes 
    for(i=0; i<cache[c_index].n_blocks; ++i)
    {   if(pups_write(fd,(void *)&cache[c_index].lifetime[i],sizeof(int32_t)) == (-1))
        {  (void)pups_close(fd);
           goto error_exit;
        }
    }

    // Write cache hubnesses 
    for(i=0; i<cache[c_index].n_blocks; ++i)
    {   if(pups_write(fd,(void *)&cache[c_index].hubness[i],sizeof(uint32_t   )) == (-1))
        {  (void)pups_close(fd);
           goto error_exit;
        }
    }

    // Write cache binding
    for(i=0; i<cache[c_index].n_blocks; ++i)
    {   if(pups_write(fd,(void *)&cache[c_index].binding[i],sizeof(uint32_t)) == (-1))
        {  (void)pups_close(fd);
           goto error_exit;
        }
    }


    if(is_live == FALSE)
       (void)pups_close(fd);
    else
       cache[c_index].mapinfo_fd = fd;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(cache[c_index].crc);

error_exit:

    pups_error("[cache_write_mapinfo] write failed");
}




/*----------------------------------*/
/* Compress cache and map file  int32_to */
/* tar/xz archive                   */
/*----------------------------------*/

_PUBLIC int32_t cache_archive(const _BOOLEAN    compress,  // Compress archive file if TRUE
                              const _BOOLEAN      delete,  // Delete .mmc and .map files when archived
                              const char     *cache_name)  // Basename of ,mmc and .map file to archive

{   int32_t ret;

    char    tar_cmd[SSIZE]   = "",
            basename[SSIZE]  = "",
            mapname[SSIZE]   = "",
            cachename[SSIZE] = "";


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(cache_name == (const char *)NULL)
       pups_error("[cache_archive] expecting cache name");


    /*----------------------------*/
    /* Get cache archive basename */
    /*----------------------------*/

    if(strin(cache_name,".") == TRUE)
    {  (void)strlcpy(basename,cache_name,SSIZE);
       (void)strtrnc(basename,'.',1);
    }

    (void)snprintf(mapname  ,SSIZE,"%s.map",basename); 
    (void)snprintf(cachename,SSIZE,"%s.mmc",basename);


    /*-----------------------------------*/
    /* Check that we have a map file and */
    /* corresponding cache               */
    /*-----------------------------------*/

    if(access(mapname,F_OK | R_OK | W_OK) == (-1))
       pups_error("[cache_archive] cannot access mapinfo file");
    else if(access(cachename,F_OK | R_OK | W_OK) == (-1))
       pups_error("[cache_archive] cannot access cache file");


    /*-------------------------------------*/
    /* Run tar and xz commands to produced */
    /* compressed cache archive            */
    /*-------------------------------------*/

    else
    {  if(compress == TRUE)
          (void)snprintf(tar_cmd,SSIZE,"tar cvf - %s %s 2> /dev/null | xz > %s.tar.gz",mapname,cachename,basename);
       else
          (void)snprintf(tar_cmd,SSIZE,"tar cvf - %s %s 1> %s.tar 2> /dev/null",mapname,cachename,basename);

       ret = system(tar_cmd);

       if(WEXITSTATUS(ret) < 0)
          pups_error("[cache_archive] failed to run archive command");


       /*-----------------------------------------*/
       /* Remove files once we have archived them */
       /*-----------------------------------------*/

       if(delete == TRUE)
       {  (void)unlink(mapname);
          (void)unlink(cachename);
       }
    }
  
    pups_set_errno(OK); 
    return(0); 
}




/*--------------------------*/
/* Uncompress cache archive */
/*--------------------------*/

_PUBLIC int32_t cache_extract(const char *cache_archive)

{   int32_t ret;

    char    tar_cmd[SSIZE]           = "",
            basename[SSIZE]          = "",
            eff_cache_archive[SSIZE] = "";


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(cache_archive == (const char *)NULL)
       pups_error("[cache_extract] expecting archive name");


    /*------------------------------------------*/
    /* Set up extraction command taking note of */
    /* archive file type                        */
    /*------------------------------------------*/
    /*---------------------------------------*/
    /* Fully qualified compressed tar achive */
    /*---------------------------------------*/

    if(strin(cache_archive,".tar.xz") == TRUE)
    {  if(access(cache_archive,F_OK | R_OK | W_OK) == (-1))
          pups_error("[cache_extract] cannot open archive file");
       else
          (void)snprintf(tar_cmd,SSIZE,"xz -d <%s.tar.xz | tar xvf - > /dev/null",cache_archive);
    }


    /*-----------------------------*/
    /* Fully qualified tar archive */
    /*-----------------------------*/

    else if(strin(cache_archive,".tar") == TRUE)
    {  if(access(cache_archive,F_OK | R_OK | W_OK) == (-1))
          pups_error("[cache_extract] cannot open archive file");
       else
          (void)snprintf(tar_cmd,SSIZE,"cat %s.tar | tar xvf - > /dev/null",cache_archive);
    }


    /*---------------------------------------*/
    /* .map file or .mmc file - strip prefix */
    /*---------------------------------------*/

    else
    {  if(strin(cache_archive,".map") == TRUE || strin(cache_archive,".mmc") == TRUE)
       {  (void)strlcpy(basename,cache_archive,SSIZE);
          (void)strtrnc(basename,'.',1);
       }
       else if(strin(cache_archive,".") == TRUE)
          pups_error("[cache_extract] invalid archive file name");
       else
          (void)strlcpy(basename,cache_archive,SSIZE);

       (void)snprintf(eff_cache_archive,SSIZE,"%s.tar",basename);


       /*----------------------*/
       /* Implicit tar archive */
       /*----------------------*/

       if(access(eff_cache_archive,F_OK | R_OK | W_OK) != (-1))
          (void)snprintf(tar_cmd,SSIZE,"cat %s | tar xvf - > /dev/null",eff_cache_archive);


       /*---------------------------------*/
       /* Implicit compressed tar archive */
       /*---------------------------------*/

       else
       {  (void)snprintf(eff_cache_archive,SSIZE,"%s.tar.xz",cache_archive);
          if(access(eff_cache_archive,F_OK | R_OK | W_OK) != (-1))
             (void)snprintf(tar_cmd,SSIZE,"xz -d <%s | tar xvf - > /dev/null",eff_cache_archive);
          else
             pups_error("[cache_extract] invalid archive file name");
       }
    }


    /*---------------------------------------*/
    /* Extract cache and associated map file */
    /*---------------------------------------*/

    ret = system(tar_cmd);
    if(WEXITSTATUS(ret) < 0)
       pups_error("[cache_archive] failed to run archive command");
    
    pups_set_errno(OK);  
    return(0);
}




/*--------------------------------*/
/* Read cache mapping information */
/*--------------------------------*/

_PUBLIC uint64_t  cache_read_mapinfo(const _BOOLEAN     try_homeostatic_protection,  // If TRUE enable homeostatic protection for map file
                                     const _BOOLEAN                have_cache_lock,  // If TRUE lock held on cache
                                     const char                          *map_name,  // Name of cache map info file
                                     const uint32_t                          mmap,   // Memory mapping flags
                                     const uint32_t                       c_index)   // Cache index (to map cache  int32_to)

{    int32_t i,
             h_p_state       = DEAD;

    des_t fd                 = (-1);

    char map_path[SSIZE]     = "",
         map_pathname[SSIZE] = "",
         eff_map_name[SSIZE] = "";

    _BOOLEAN is_live = FALSE;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[cache_read_mapinfo] attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    /*--------------*/
    /* Sanity check */
    /*--------------*/
    /*-------------*/
    /* Range error */
    /*-------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_read_mapinfo] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }


    /*------------------------*/
    /* Mapping file not found */
    /*------------------------*/

    else if(map_name == (const char *)NULL)
       pups_error("[cache_read_mapinfo] expecting mapinfo file name");


    /*---------------------------------*/
    /* Get map file path and leaf names */
    /*---------------------------------*/

    if(strin(map_name,"/") == TRUE)
    {  (void)strleaf(map_name,eff_map_name);
       (void)strlcpy (map_path,map_name,SSIZE);
       (void)strtrnc(map_path,'/',1);
    }


    /*----------------------------------------------*/
    /* No path - just a leaf (in current directory) */
    /*----------------------------------------------*/
 
    else
       (void)strlcpy(eff_map_name,map_name,SSIZE);


    /*--------------------------------------*/
    /* Generate map file name from basename */
    /*--------------------------------------*/

    (void)strtrnc(eff_map_name,'.',1);
    (void)strlcat (eff_map_name,".map",SSIZE);


    /*----------------------------*/
    /* Generate map file pathname */
    /*----------------------------*/
    /*---------------------------*/
    /* Absolute or relative path */
    /*---------------------------*/

    if(strin(map_name,"/") == TRUE)
       (void)snprintf(map_pathname,SSIZE,"%s/%s",map_path,eff_map_name);


    /*---------*/
    /* No path */
    /*---------*/

    else
       (void)strlcpy(map_pathname,eff_map_name,SSIZE);


    /*-------------------------------------------*/
    /* Try to protect file if requested to do so */
    /*-------------------------------------------*/

    if(mmap & CACHE_LIVE)
       h_p_state = LIVE;

    if(try_homeostatic_protection == TRUE)
    {  if(pups_isalive(map_pathname) == FALSE)
       {  if((fd = pups_open(map_pathname,O_RDONLY,h_p_state)) == (-1))
             pups_error("[cache_read_mapinfo] failed to open mapinfo file");

          is_live = TRUE;
       }


       /*-----------------------------------*/
       /* Don't protect file more than once */
       /*-----------------------------------*/

       else
       {  if((fd = pups_open(map_pathname,O_RDONLY,DEAD)) == (-1))
             pups_error("[cache_read_mapinfo] failed to open mapinfo file");
       }
    }


    /*------------------------------------*/
    /* No homeostatic map file protection */
    /*------------------------------------*/

    else
    {  if((fd = pups_open(map_pathname,O_RDONLY,DEAD)) == (-1))
          pups_error("[cache_read_mapinfo] failed to open mapinfo file");
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */


    // Cache CRC
    if(pups_read(fd,(void *)&cache[c_index].crc,sizeof(uint64_t)) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Cache path 
    if(pups_read(fd,(void *)cache[c_index].path,256) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Cache name
    if(pups_read(fd,(void *)cache[c_index].name,256) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Cache mapinfo file name
    if(pups_read(fd,(void *)cache[c_index].mapinfo_name,256) == (-1))
    {  (void)close(fd);
       goto error_exit;
    }

    // Cache mapfile name
    if(pups_read(fd,(void *)cache[c_index].mmap_name,256) == (-1))
    {  (void)close(fd);
       goto error_exit;
    }

    // Architecture of machine used to generate cache
    if(pups_read(fd,(void *)cache[c_index].march,256) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Auxilliary information 
    if(pups_read(fd,(void *)cache[c_index].auxinfo,SSIZE) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Cache mapping flags
    if(pups_read(fd,(void *)&cache[c_index].mmap,sizeof(uint32_t)) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Number of used blocks in cache
    if(pups_read(fd,(void *)&cache[c_index].u_blocks,sizeof(int32_t)) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Number of blocks in cache
    if(pups_read(fd,(void *)&cache[c_index].n_blocks,sizeof(int32_t)) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Number of objects per block
    if(pups_read(fd,(void *)&cache[c_index].n_objects,sizeof(int32_t)) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Size of cache (bytes)
    if(pups_read(fd,(void *)&cache[c_index].cache_size,sizeof(uint64_t)) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Size of cache block (bytes)
    if(pups_read(fd,(void *)&cache[c_index].block_size,sizeof(uint64_t)) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Co-ordination list size 
    if(pups_read(fd,(void *)&cache[c_index].colsize,sizeof(uint32_t)) == (-1))
    {  (void)pups_close(fd);
       goto error_exit;
    }

    // Read descriptions of objects in cache blocks
    for(i=0; i<cache[c_index].n_objects; ++i)
    {   if(pups_read(fd,(void *)&cache[c_index].object_desc[i],256*sizeof(_BYTE)) == (-1))
        {  (void)pups_close(fd);
           goto error_exit;
        }
    }

    // Read offsets of objects in cache blocks (bytes)
    for(i=0; i<cache[c_index].n_objects; ++i)
    {   if(pups_read(fd,(void *)&cache[c_index].object_offset[i],sizeof(uint64_t)) == (-1))
        {  (void)pups_close(fd);
           goto error_exit;
        }
    }

    // Read sizes of objects in cache blocks (bytes)
    for(i=0; i<cache[c_index].n_objects; ++i)
    {   if(pups_read(fd,(void *)&cache[c_index].object_size[i],sizeof(uint64_t)) == (-1))
        {  (void)pups_close(fd);
           goto error_exit;
        }
    }

    // Allocate space for cache block flags
    if(cache[c_index].flags == (_BYTE *)NULL)
       cache[c_index].flags = (_BYTE *)pups_calloc(cache[c_index].n_blocks,sizeof(_BYTE));

    // Read cache block flags
    for(i=0; i<cache[c_index].n_blocks; ++i)
    {   if(pups_read(fd,(void *)&cache[c_index].flags[i],sizeof(_BYTE)) == (-1))
        {  (void)pups_close(fd);
           goto error_exit;
        }
    }

    // Allocate space for cache block tags
    if(cache[c_index].tag == (uint32_t *)NULL)
       cache[c_index].tag = (uint32_t  *)pups_calloc(cache[c_index].n_blocks,sizeof(uint32_t));

    // Read cache block tags
    for(i=0; i<cache[c_index].n_blocks; ++i)
    {   if(pups_read(fd,(void *)&cache[c_index].tag[i],sizeof(int32_t)) == (-1))
        {  (void)pups_close(fd);
           goto error_exit;
        }
    }

    // Allocate space for cache block lifetimes
    if(cache[c_index].lifetime == (uint64_t *)NULL)
       cache[c_index].lifetime = (uint64_t  *)pups_calloc(cache[c_index].n_blocks,sizeof(uint64_t));

    // Read cache block lifetimes 
    for(i=0; i<cache[c_index].n_blocks; ++i)
    {   if(pups_read(fd,(void *)&cache[c_index].lifetime[i],sizeof(int64_t)) == (-1))
        {  (void)pups_close(fd);
           goto error_exit;
        }
    }

    // Allocate space for cache block hubnesses 
    if(cache[c_index].hubness == (uint32_t *)NULL)
       cache[c_index].hubness = (uint32_t  *)pups_calloc(cache[c_index].n_blocks,sizeof(uint32_t));

    // Read cache block hubnesses 
    for(i=0; i<cache[c_index].n_blocks; ++i)
    {   if(pups_read(fd,(void *)&cache[c_index].hubness[i],sizeof(uint32_t   )) == (-1))
        {  (void)pups_close(fd);
           goto error_exit;
        }
    }

    // Allocate space for cache block binding 
    if(cache[c_index].binding == (uint32_t *)NULL)
       cache[c_index].binding = (uint32_t  *)pups_calloc(cache[c_index].n_blocks,sizeof(uint32_t));

    // Read cache block bindings 
    for(i=0; i<cache[c_index].n_blocks; ++i)
    {   if(pups_read(fd,(void *)&cache[c_index].binding[i],sizeof(uint32_t)) == (-1))
        {  (void)pups_close(fd);
           goto error_exit;
        }
    }

    // Allocate space for rwlocks
    if(cache[c_index].rwlock == (pthread_rwlock_t *)NULL)
    // Allocate space for rwlocks
    if(cache[c_index].rwlock == (pthread_rwlock_t *)NULL)
       cache[c_index].rwlock = (pthread_rwlock_t *)pups_calloc(cache[c_index].n_blocks,sizeof(pthread_rwlock_t));

    // Initialise rwlocks
    for(i=0; i<cache[c_index].n_blocks; ++i)
        (void)pthread_rwlock_init(&cache[c_index].rwlock[i],(pthread_rwlockattr_t *)NULL);


    /*---------------------------------------------*/
    /* Keep file descriptor open if we want        */
    /* PUPS/P3 homeostatic protection for map file */
    /*---------------------------------------------*/

    if(is_live == FALSE)
       (void)pups_close(fd);
    else
       cache[c_index].mapinfo_fd = fd;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(cache[c_index].crc);

error_exit:

    pups_error("[cache_read_mapinfo] read failed");
}




/*---------------------------------------------*/
/* Reset all block (read/write) locks in cache */
/*---------------------------------------------*/

_PUBLIC int32_t cache_reset_blocklocks(const _BOOLEAN have_cache_lock,  // TRUE if lock held on specified cache 
                                       const uint32_t         c_index)  // Cache index

{   uint32_t i;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_unlock_block] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */


    /*-------------------*/
    /* Reset block locks */
    /*-------------------*/

    for(i=0; i<cache[c_index].n_blocks; ++i)
        (void)pthread_rwlock_init(&cache[c_index].rwlock[i],(pthread_rwlockattr_t *)NULL);

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*------------------------------------*/
/* Get number of used blocks in cache */
/*------------------------------------*/

_PUBLIC int32_t cache_blocks_used(const _BOOLEAN have_cache_lock, const uint32_t c_index)

{    int32_t blocks_used = 0;


    /*---------------*/
    /* Sanity checks */
    /*---------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_blocks_used] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    blocks_used = cache[c_index].u_blocks;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(blocks_used);
}



/*--------------------------------------*/
/* Reset number of used blocks in cache */
/*--------------------------------------*/

_PUBLIC int32_t cache_blocks_reset_used(const _BOOLEAN have_cache_lock, const uint32_t c_index)

{

    /*---------------*/
    /* Sanity checks */
    /*---------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_blocks_reset_used] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    cache[c_index].u_blocks = 0;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}



/*------------*/
/* Lock block */
/*------------*/

_PUBLIC int32_t cache_lock_block(const uint32_t  cache_lock_state, // If TRUE lock held on cache
                                 const uint32_t  block_locktype,   // Type of lock (RDLOCK or WRLOCK)
                                 const uint32_t  c_index,          // Cache index 
                                 const uint32_t  block_index)      // Block index of block locked 

{

    /*---------------*/
    /* Sanity checks */
    /*---------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_lock_block] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    if(cache_lock_state != CACHE_LOCK && cache_lock_state != CACHE_HAVELOCK)
        pups_error("[cache_access_object] expecting cache lock state \"CACHE_LOCK\" or \"CACHE_HAVELOCK\"");

    if(block_locktype != BLOCK_RDLOCK && block_locktype != BLOCK_WRLOCK)
       pups_error("[cache_lock_block] expecting \"BLOCK_RDLOCK\" or \"BLOCK_WRLOCK\" for lock type");

    #ifdef PTHREAD_SUPPORT
    if(cache_lock_state == CACHE_LOCK)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */


    /*-------------------------*/
    /* Block index range error */
    /*-------------------------*/

    if(block_index >= cache[c_index].n_blocks)
    {  (void)snprintf(errstr,SSIZE,"[cache_lock_block] block index range error [cache block (%d) > max blocks (%d)\n",block_index,cache[c_index].n_blocks);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(block_locktype == RDLOCK)
       (void)pthread_rwlock_rdlock(&cache[c_index].rwlock[block_index]);
    else
       (void)pthread_rwlock_wrlock(&cache[c_index].rwlock[block_index]);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*--------------*/
/* Unlock block */
/*--------------*/

_PUBLIC int32_t cache_unlock_block(const uint32_t    cache_lock_state,  // State of cache lock 
                                   const uint32_t             c_index,  // Cache index
                                   const uint32_t         block_index)  // Block index of block unlocked

{

    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_unlock_block] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    if(cache_lock_state != CACHE_UNLOCK && cache_lock_state != CACHE_HAVELOCK)
        pups_error("[cache_access_object] expecting cache lock state \"CACHE_UNLOCK\" or \"CACHE_HAVELOCK\"");


    /*-------------------------*/
    /* Block index range error */
    /*-------------------------*/

    if(block_index >= cache[c_index].n_blocks)
    {  (void)snprintf(errstr,SSIZE,"[cache_unlock_block] block index range error [block index (%d) > max blocks (%d)\n",block_index,cache[c_index].n_blocks);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_rwlock_unlock(&cache[c_index].rwlock[block_index]);
    if(cache_lock_state == CACHE_UNLOCK)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*---------------------------*/
/* Set/reset cache used flag */
/*---------------------------*/

_PUBLIC int32_t cache_block_set_used(const _BOOLEAN     have_cache_lock,  // If TRUE lock held on cache 
                                     const uint32_t             c_index,  // Cache index
                                     const uint32_t         block_index,  // Block index
                                     const _BOOLEAN                used)  // If TRUE mark block as used

{

    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_set_used] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */


    /*-------------------------*/
    /* Block index range error */
    /*-------------------------*/

    if(block_index >= cache[c_index].n_blocks)
    {  (void)snprintf(errstr,SSIZE,"[cache_set_used] block index range error [block index (%d) > max blockss (%d)\n",block_index,cache[c_index].n_blocks);
       pups_error(errstr);
    }
    else
    {  if(used == TRUE)
       {  cache[c_index].flags[block_index] |=  BLOCK_USED;
          ++cache[c_index].u_blocks;
       }
       else
       {  cache[c_index].flags[block_index] &= ~BLOCK_USED;

          if(cache[c_index].u_blocks > 0)
             --cache[c_index].u_blocks;
       }
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*------------------*/
/* Is cache in use? */
/*------------------*/

_PUBLIC int32_t cache_in_use(const _BOOLEAN  have_cache_lock,  // If TRUE lock held on cache
                             const uint32_t          c_index)  // Cache index
{   _BOOLEAN ret = FALSE;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_block_in_use] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    if(cache[c_index].cache_ptr != (void *)NULL)
       ret = TRUE;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(ret);
}




/*------------------*/
/* Is block in use? */
/*------------------*/

_PUBLIC int32_t cache_block_in_use(const _BOOLEAN  have_cache_lock,  // If TRUE lock held on cache
                                   const uint32_t          c_index,  // Cache index
                                   const uint32_t      block_index)  // Block index of block tested

{   _BOOLEAN ret = FALSE;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_block_in_use] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */


    /*-------------------------*/
    /* Block index range error */
    /*-------------------------*/

    if(block_index >= cache[c_index].n_blocks)
    {  (void)snprintf(errstr,SSIZE,"[cache_block_in_use] block index range error [block index (%d) > max blocks (%d)\n",block_index,cache[c_index].n_blocks);
       pups_error(errstr);
    }
    else if(cache[c_index].flags[block_index] & BLOCK_USED)
       ret = TRUE;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(ret);
}




/*---------------------------*/
/* Set/reset cache busy flag */
/*---------------------------*/

_PUBLIC int32_t cache_set_busy(const _BOOLEAN have_cache_lock, const uint32_t c_index, const _BOOLEAN busy)

{

    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_set)busy] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    if(busy == TRUE)
       cache[c_index].busy = TRUE;
    else
       cache[c_index].busy = FALSE;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*----------------*/
/* Is cache busy? */
/*----------------*/

_PUBLIC _BOOLEAN cache_is_busy(const _BOOLEAN have_cache_lock, const uint32_t c_index)

{   _BOOLEAN busy = FALSE;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_is_mapped] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    busy = cache[c_index].busy;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
      (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(busy);
}





/*-----------------------------*/
/* Add data to cache block     */
/* block MUST be locked before */
/* doing this                  */
/*-----------------------------*/

_PUBLIC int32_t cache_add_block(const void              *data,               // Location of data to cache
                                const uint64_t          size,                // Size of data (in bytes)
                                const uint32_t          flags,               // Access flags 
                                const uint32_t          object_index,        // Object within cache block where data is written 
                                const  int32_t          block_index,         // Cache block where data is written 
                                const  int32_t          tag,                 // Block identifier 
                                const _BOOLEAN          cache_access_state,  // If TRUE lock held on cache
                                const uint32_t          c_index)             // Cache index 

{   uint32_t i,
             block_access_flag,
             new_block_index;

    void     *cache_ptr = (void *)NULL;


    /*---------------*/
    /* Sanity checks */
    /*---------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_add_block] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    if(data == (const void *)NULL)
      pups_error("[cache_add_block] null data pointer");

    if(flags & BLOCK_RDLOCK)
       pups_error("[cache_add_block] need write access to block");


    /*-------------------------------*/
    /* Get pointer to cache block we */
    /* writing data to               */
    /*-------------------------------*/
    /*-----------------------*/
    /* Write access to block */
    /*-----------------------*/      

    if(flags & BLOCK_WRLOCK)
       block_access_flag = BLOCK_WRLOCK;


    /*----------------------------*/
    /* Have write access to block */
    /*----------------------------*/

    else if((flags & BLOCK_HAVELOCK) || (flags & BLOCK_LOADED))
       block_access_flag = BLOCK_HAVELOCK;


    /*-------*/
    /* Error */
    /*-------*/

    else
       pups_error("[cache_add_block] either \"BLOCK_WRLOCK\",  \"BLOCK_HAVELOCK\" or \"BLOCK_LOADED\" expected");


    /*----------------------------------------------------*/
    /* Are we inserting data  int32_to a specific cache block? */
    /*----------------------------------------------------*/

    if(block_index != ANY_CACHE_BLOCK)
    {

       /*-----------------------------*/
       /* Error - cannot access block */
       /*-----------------------------*/
 
       if((cache_ptr = cache_access_object(cache_access_state,block_access_flag,block_index,object_index,c_index)) == (void *)NULL)
          pups_error("[cache_add_block] cannot access specified block");


       /*-----------------------------------*/
       /* Copy data to cache, tag block and */
       /* increment number of used blocks   */ 
       /*-----------------------------------*/

       else
       {  (void)bcopy(data,cache_ptr,size);
          ++cache[c_index].u_blocks;

          /*----------------------------------------------------------*/
          /* Mark block in use when we have written all objects to it */
          /*----------------------------------------------------------*/

          if(flags & BLOCK_LOADED)
          {  cache[c_index].flags[block_index] |= BLOCK_USED;
             cache[c_index].tag[block_index]    = tag;


             /*-----------------------------------------*/
             /* Unlock block (now data has been loaded) */
             /*-----------------------------------------*/

             (void)cache_unlock_block(CACHE_UNLOCK,c_index,block_index);
          }

          #ifdef DEBUG
          (void)fprintf(stderr,"CACHE %d, BLOCK %d, OBJECT %d\n",c_index,block_index,object_index);
          (void)fflush(stderr);
          #endif /* DEBUG */

          pups_set_errno(OK);
          return(block_index);
       }
    } 


    /*--------------------------------------------------------*/
    /* Do we have an unused block in the cache we can re-use? */
    /*--------------------------------------------------------*/

    for(i=0; i<cache[c_index].n_blocks; ++i)
    { 

       if(! (cache[c_index].flags[i] & BLOCK_USED))
       {
          /*---------------------------*/
          /* Error cannot access block */
          /*---------------------------*/

          if((cache_ptr = cache_access_object(cache_access_state,block_access_flag,i,object_index,c_index)) == (void *)NULL)
             pups_error("[cache_add_block] cannot access specified block");


          /*----------------------------------*/
          /* Copy data to cache and tag block */
          /*----------------------------------*/

          else
          {  (void)bcopy(data,cache_ptr,size);


             /*----------------------------------------------------------*/
             /* Mark block in use when we have written all objects to it */
             /*----------------------------------------------------------*/

             if(flags & BLOCK_LOADED)
             {  cache[c_index].flags[i] |= BLOCK_USED;
                cache[c_index].tag[i]    = tag;


                /*-----------------------------------------*/
                /* Unlock block (now data has been loaded) */
                /*-----------------------------------------*/

                (void)cache_unlock_block(CACHE_UNLOCK,c_index,i);

                #ifdef PTHREAD_SUPPORT 
                if(cache_access_state != CACHE_HAVELOCK)
                  (void)pthread_mutex_unlock(&cache[c_index].mutex);
                #endif /* PTREAD_SUPPORT */
             }

             #ifdef DEBUG
             (void)fprintf(stderr,"ANY CACHE %d, BLOCK %d, OBJECT %d\n",c_index,block_index,object_index);
             (void)fflush(stderr);
             #endif /* DEBUG */

             pups_set_errno(OK);
             return(i);
          }
       }
    }


    /*--------------*/
    /* Extend cache */
    /*--------------*/ 

    new_block_index          = cache[c_index].n_blocks + 1;
    cache[c_index].n_blocks += BLOCK_ALLOC_QUANTUM;
    (void)cache_resize(FALSE,cache[c_index].n_blocks,c_index);


    /*-------*/
    /* Error */
    /*-------*/

    if((cache_ptr = cache_access_object(CACHE_LOCK,block_access_flag,new_block_index,object_index,c_index)) == (void *)NULL)
       pups_error("[cache_add_block] cannot access specified block");


    /*----------------------------------*/
    /* Copy data to cache and tag block */
    /*----------------------------------*/

    else
    {  (void)bcopy(data,cache_ptr,size);


       /*----------------------------------------------------------*/
       /* Mark block in use when we have written all objects to it */
       /*----------------------------------------------------------*/

       if(flags & BLOCK_LOADED)
       {  cache[c_index].flags[new_block_index] |= BLOCK_USED;
          cache[c_index].tag[new_block_index]    = tag;


          /*-----------------------------------------*/
          /* Unlock block (now data has been loaded) */
          /*-----------------------------------------*/

          (void)cache_unlock_block(CACHE_UNLOCK,c_index,new_block_index);
       }

       #ifdef DEBUG
       (void)fprintf(stderr,"ANY CACHE %d, BLOCK %d, OBJECT %d\n",c_index,new_block_index,object_index);
       (void)fflush(stderr);
       #endif /* DEBUG */
    }

    #ifdef PTHREAD_SUPPORT 
    if(cache_access_state != CACHE_HAVELOCK)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTREAD_SUPPORT */

    pups_set_errno(OK);
    return(new_block_index);
}




/*---------------------------------*/
/* Delete block of data from cache */
/*---------------------------------*/

_PUBLIC _BOOLEAN cache_delete_block(const _BOOLEAN     have_cache_lock,  // TRUE if lock held on cache
                                    const _BOOLEAN     have_block_lock,  // TRUE if lock held on block
                                    const uint32_t             c_index,  // Cache index 
                                    const uint32_t         block_index)  // Block to be deleted

{   _BOOLEAN ret = FALSE;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_delete_block] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    if(block_index >= cache[c_index].n_blocks)
    {  (void)snprintf(errstr,SSIZE,"[cache_delete_block] block index range error [block index (%d) > max blocks (%d)\n",block_index,cache[c_index].n_blocks);
       pups_error(errstr);
    }


    /*------------------------------------*/
    /* If block is used mark it as unused */
    /*------------------------------------*/

    #ifdef PTHREAD_SUPPORT
    if(have_block_lock == FALSE)
       (void)pthread_rwlock_wrlock(&cache[c_index].rwlock[block_index]);
    #endif /* PTHREAD_SUPPORT */

    if(cache[c_index].flags[block_index] & BLOCK_USED)
    {  cache[c_index].flags[block_index] &= ~BLOCK_USED; 

       if(cache[c_index].u_blocks > 0)
         --cache[c_index].u_blocks;

       ret = TRUE;
    }

    #ifdef PTHREAD_SUPPORT
    if(have_block_lock == FALSE)
       (void)pthread_rwlock_unlock(&cache[c_index].rwlock[block_index]);

    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(ret);
}




/*--------------------------------*/
/* Restore block of data to cache */
/*--------------------------------*/

_PUBLIC _BOOLEAN cache_restore_block(const _BOOLEAN     have_cache_lock,  // TRUE if lock held on cache
                                     const _BOOLEAN     have_block_lock,  // TRUE if lock held on block
                                     const uint32_t             c_index,  // Cache index
                                     const uint32_t         block_index)  // Block to be restored 

{   _BOOLEAN ret = FALSE;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_restore_block] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    if(block_index >= cache[c_index].n_blocks)
    {  (void)snprintf(errstr,SSIZE,"[cache_restore_block] block index range error [block index (%d) > max blocks (%d)\n",block_index,cache[c_index].n_blocks);
       pups_error(errstr);
    }


    /*------------------------------------*/
    /* If block is unused mark it as used */
    /*------------------------------------*/

    #ifdef PTHREAD_SUPPORT
    if(have_block_lock == FALSE)
       (void)pthread_rwlock_wrlock(&cache[c_index].rwlock[block_index]);
    #endif /* PTHREAD_SUPPORT */

    if(cache[c_index].flags[block_index] & ~BLOCK_USED)
    {  cache[c_index].flags[block_index] |= BLOCK_USED;

       ++cache[c_index].u_blocks;
       ret = TRUE;
    }

    #ifdef PTHREAD_SUPPORT
    if(have_block_lock == FALSE)
       (void)pthread_rwlock_unlock(&cache[c_index].rwlock[block_index]);

    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(ret);
}




/*-------------*/
/* Clear cache */
/*-------------*/

_PUBLIC int32_t cache_clear(const _BOOLEAN have_cache_lock,  // If TRUE lock held on cache
                            const uint32_t         c_index,  // Cache index
                            const int32_t              tag)  // Tag i.d. (which must tag i.d. of blocks cleared)

{    int32_t i,
             n_cleared = 0;


    /*---------------*/
    /* Sanity check  */
    /*---------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_clear] cache index range error [cache index (%d) > max caches (%d)]\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);


    /*---------------------------------------------*/
    /* Reset all blocklocks before cache is walked */
    /*---------------------------------------------*/

    (void)cache_reset_blocklocks(have_cache_lock,c_index);
    #endif /* PTHREAD_SUPPORT */

    for(i=0; i<cache[c_index].n_blocks; ++i)
    {  

       /*-------------------------------------------*/
       /* Clear block usage flag if block is in use */
       /*-------------------------------------------*/

       if(cache[c_index].flags[i] & BLOCK_USED)
       {

          /*----------------------------------------------*/
          /* Clear all tagged cache blocks. If ALL_BLOCKS */
          /* is specified clear the entire cache          */
          /*----------------------------------------------*/

          if(tag == ALL_CACHE_BLOCKS || cache[c_index].tag[i] == tag)
          {  cache[c_index].flags[i] &= ~BLOCK_USED;


             if(cache[c_index].u_blocks > 0)
                --cache[c_index].u_blocks;

             ++n_cleared;
          }
       }
    }


    /*--------------------*/
    /* Reset object count */
    /*--------------------*/

    cache[c_index].n_objects = 0;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(n_cleared);
}




/*----------------------*/
/* Merge pair of caches */
/*----------------------*/

_PUBLIC int32_t cache_merge(const _BOOLEAN have_cache_locks,  // If TRUE locks held on caches
                            const uint32_t        c_index_1,  // Cache index 1
                            const uint32_t        c_index_2,  // Cache index 2
                            const int32_t               tag)  // Tag i.d. (of merged blocks in cache 1)

{   uint32_t i,
             n_merged   = 0,
             n_blocks_1 = 0,
             n_blocks_2 = 0;


    /*----------------*/
    /* Sanity checks  */
    /*----------------*/

    if(c_index_1 >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_merge] cache index range error [cache index (%d) > max caches (%d)]\n",c_index_1,MAX_CACHES);
       pups_error(errstr);
    }

    if(c_index_2 >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_merge] cache index range error [cache index (%d) > max caches (%d)]\n",c_index_2,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_locks == FALSE)
    {  (void)pthread_mutex_lock(&cache[c_index_1].mutex);
       (void)pthread_mutex_lock(&cache[c_index_2].mutex);
    }
    #endif /* PTHREAD_SUPPORT */


    /*------------------------------------------------------------------*/
    /* Check (machine) architecture of caches to be merged is identical */
    /*------------------------------------------------------------------*/

    if(strcmp(cache[c_index_1].march,cache[c_index_2].march) != 0)
    {   (void)snprintf(errstr,SSIZE,"[cache_merge] blocks (in caches %d and %d) have different (machine) architectures\n",
                                                                                               c_index_1,c_index_2);
        pups_error(errstr);
    }


    /*------------------------------------------------------*/
    /* Check blocksize of caches to be merged is identical? */
    /*------------------------------------------------------*/

    if(cache[c_index_1].block_size != cache[c_index_2].block_size)
    {   (void)snprintf(errstr,SSIZE,"[cache_merge] blocks (in caches %d and %d) have different sizes\n",
                                                                             c_index_1,c_index_2);
        pups_error(errstr);
    }


    /*-----------------------------*/
    /* Number of blocks in cache 1 */
    /*-----------------------------*/

    n_blocks_1 = cache[c_index_1].n_blocks;


    /*-----------------------------*/
    /* Number of blocks in cache 1 */
    /*-----------------------------*/

    n_blocks_2 = cache[c_index_2].n_blocks;


    /*------------------------------------------*/
    /* Resize cache 1 (so there is enough space */
    /* to merge the contents of cache 2         */
    /*------------------------------------------*/
   
    (void)cache_resize(have_cache_locks,(n_blocks_1 + n_blocks_2),c_index_1);


    /*--------------*/
    /* Merge caches */
    /*--------------*/

    for(i=n_blocks_1; i<(n_blocks_1 + n_blocks_2); ++i)
    {

       /*------------*/
       /* Copy block */
       /*------------*/
                                                                        /*---------------------------*/
       (void)memcpy(cache[c_index_1].blockmap[i].object_ptr[0],         /* Base of destination block */
                    cache[c_index_2].blockmap[n_merged].object_ptr[0],  /* Base of source block      */
                    cache[c_index_2].block_size);                       /* size of block             */
                                                                        /*---------------------------*/
                    

       /*----------------------------*/
       /* Update cache datastructure */
       /*----------------------------*/

       cache[c_index_1].flags[i] = cache[c_index_2].flags[n_merged];
       cache[c_index_1].tag[i]   = tag;
       ++n_merged;
    }


    /*-----------------------------*/
    /* Update used blocks in cache */
    /*-----------------------------*/

    cache[c_index_1].u_blocks += cache[c_index_2].u_blocks;


    #ifdef PTHREAD_SUPPORT
    if(have_cache_locks == FALSE)
    {  (void)pthread_mutex_unlock(&cache[c_index_1].mutex);
       (void)pthread_mutex_unlock(&cache[c_index_2].mutex);
    }
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(n_blocks_2);
}




/*------------------------------------------*/
/* Get string repesentation of blockag i.d. */
/*------------------------------------------*/

_PUBLIC int32_t cache_get_blocktag_id_str(const uint32_t blocktag_id, char *blocktag_id_str)

{

    /*------------------------------------------------*/
    /* String values corresponding to block tag i.d's */
    /*------------------------------------------------*/
    /*-----*/
    /* All */
    /*-----*/

    if(blocktag_id == ALL_CACHE_BLOCKS)
       (void)strlcpy(blocktag_id_str,"all",SSIZE);


    /*---------*/
    /* Scratch */
    /*---------*/

    else if(blocktag_id == SCRATCH_TAG)
       (void)strlcpy(blocktag_id_str,"scratch",SSIZE);


    /*--------------------*/
    /* Abitrary block tag */
    /*--------------------*/

    else
       (void)snprintf(blocktag_id_str,SSIZE,"%x",blocktag_id);

    pups_set_errno(OK);
    return(0);
}




/*-------------------------------------*/
/* Show tag of specified cache block   */
/*-------------------------------------*/

_PUBLIC int32_t cache_get_blocktag(const _BOOLEAN  have_cache_lock,  // If TRUE lock held on cache
		                   const uint32_t          c_index,  // Cache index
                                   const uint32_t      block_index)  // Cache block index

{    int32_t tag;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_get_blocktag] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }


    /*-------------------------*/
    /* Block index range error */
    /*-------------------------*/

    if(block_index >= cache[c_index].n_blocks)
    {  (void)snprintf(errstr,SSIZE,"[cache_get_blocktag] block index range error [block index (%d) > max blocks (%d)\n",block_index, cache[c_index].n_blocks);
       pups_error(errstr);
    }


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    tag = cache[c_index].tag[block_index];

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(tag); 
}




/*----------------------------------*/
/* Set tag of specified cache block */
/*----------------------------------*/

_PUBLIC int32_t cache_set_blocktag(const _BOOLEAN  have_cache_lock,  // If TRUE lock held on cache
                                   const uint32_t              tag,  // Block tag identifier
                                   const uint32_t          c_index,  // Cache index
                                   const uint32_t      block_index)  // Cache block index

{ 

    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_set_blocktag] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }


    /*-------------------------*/
    /* Block index range error */
    /*-------------------------*/

    if(block_index >= cache[c_index].n_blocks)
    {  (void)snprintf(errstr,SSIZE,"[cache_set_blocktag] block index range error [block index (%d) > max blocks (%d)\n",block_index, cache[c_index].n_blocks);
       pups_error(errstr);
    }


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    cache[c_index].tag[block_index] = tag;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*---------------------------------------*/
/* Change tags of specified cache blocks */
/*---------------------------------------*/

_PUBLIC int32_t cache_change_blocktag(const _BOOLEAN  have_cache_lock,  // If TRUE lock held on cache
                                      const uint32_t          c_index,  // Cache index
                                      const int32_t          from_tag,  // Tag i.d. to be replaced
                                      const int32_t            to_tag)  // Replacement tag i.d.

{    int32_t i,
             n_tags = 0;


    /*---------------*/
    /* Sanity check  */
    /*---------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_change_blocktag] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    for(i=0; i<cache[c_index].n_blocks; ++i)
    {

       #ifdef PTHREAD_SUPPORT
       (void)pthread_rwlock_wrlock(&cache[c_index].rwlock[i]);
       #endif /* PTHREAD_SUPPORT */

       /*----------------------------------------------*/
       /* Clear all tagged cache blocks. If ALL_BLOCKS */
       /* is specified clear the entire cache          */
       /*----------------------------------------------*/

       if(cache[c_index].tag[i] == from_tag)
       {

          /*---------------------------------------------*/
          /* Change blck tag from 'from_tag' to 'to_tag' */
          /*---------------------------------------------*/

          cache[c_index].tag[i] = to_tag;
          ++n_tags;
       }

       #ifdef PTHREAD_SUPPORT
       (void)pthread_rwlock_unlock(&cache[c_index].rwlock[i]);
       #endif /* PTHREAD_SUPPORT */

    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(n_tags);
}




/*----------------------------------------*/
/* Show lifetime of specified cache block */
/*----------------------------------------*/

_PUBLIC int64_t cache_get_blocklifetime(const _BOOLEAN  have_cache_lock,  // If TRUE lock held on cache
                                        const uint32_t          c_index,  // Cache index
                                        const uint32_t      block_index)  // Cache block index

{    int32_t lifetime = 0;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_get_blocklifetime] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }


    /*-------------------------*/
    /* Block index range error */
    /*-------------------------*/

    if(block_index >= cache[c_index].n_blocks)
    {  (void)snprintf(errstr,SSIZE,"[cache_get_blocklifetime] block index range error [block index (%d) > max blocks (%d)\n",block_index, cache[c_index].n_blocks);
       pups_error(errstr);
    }


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    lifetime = cache[c_index].lifetime[block_index];

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(lifetime); 
}




/*---------------------------------------*/
/* Set lifetime of specified cache block */
/*---------------------------------------*/

_PUBLIC int32_t cache_set_blocklifetime(const _BOOLEAN  have_cache_lock,  // If TRUE lock held on cache
                                        const int64_t          lifetime,  // Block lifetime MAO - query should this be int64_t was int32_t ?
                                        const uint32_t          c_index,  // Cache index
                                        const uint32_t      block_index)  // Cache block index

{ 

    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_set_blocklifetime] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }


    /*-------------------------*/
    /* Block index range error */
    /*-------------------------*/

    if(block_index >= cache[c_index].n_blocks)
    {  (void)snprintf(errstr,SSIZE,"[cache_set_blocklifetime] block index range error [block index (%d) > max blocks (%d)\n",block_index, cache[c_index].n_blocks);
       pups_error(errstr);
    }


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    cache[c_index].lifetime[block_index] = lifetime;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}





/*--------------------------------------*/
/* Get hubness of specified cache block */
/*--------------------------------------*/

_PUBLIC uint32_t cache_get_blockhubness(const _BOOLEAN  have_cache_lock,  // If TRUE lock held on cache
                                        const uint32_t          c_index,  // Cache index
                                        const uint32_t      block_index)  // Cache block index

{   uint32_t hubness = 0;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_get_blockhubness] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }


    /*-------------------------*/
    /* Block index range error */
    /*-------------------------*/

    if(block_index >= cache[c_index].n_blocks)
    {  (void)snprintf(errstr,SSIZE,"[cache_get_blockhubness] block index range error [block index (%d) > max blocks (%d)\n",block_index, cache[c_index].n_blocks);
       pups_error(errstr);
    }


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    hubness = cache[c_index].hubness[block_index];

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(hubness); 
}




/*--------------------------------------*/
/* Set hubness of specified cache block */
/*--------------------------------------*/

_PUBLIC int32_t cache_set_blockhubness(const _BOOLEAN  have_cache_lock,  // If TRUE lock held on cache
                                       const uint32_t          hubness,   // Block hubness
                                       const uint32_t          c_index,   // Cache index
                                       const uint32_t      block_index)   // Cache block index

{ 

    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_set_blockhubness] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }


    /*-------------------------*/
    /* Block index range error */
    /*-------------------------*/

    if(block_index >= cache[c_index].n_blocks)
    {  (void)snprintf(errstr,SSIZE,"[cache_set_blockhubness] block index range error [block index (%d) > max blocks (%d)\n",block_index, cache[c_index].n_blocks);
       pups_error(errstr);
    }


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    cache[c_index].hubness[block_index] = hubness;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}





/*--------------------------------------*/
/* Get binding of specified cache block */
/*--------------------------------------*/

_PUBLIC uint32_t cache_get_blockbinding(const _BOOLEAN  have_cache_lock,  // If TRUE lock held on cache
                                        const uint32_t          c_index,  // Cache index
                                        const uint32_t      block_index)  // Cache block index

{   uint32_t binding = 0;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_get_binding] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }


    /*-------------------------*/
    /* Block index range error */
    /*-------------------------*/

    if(block_index >= cache[c_index].n_blocks)
    {  (void)snprintf(errstr,SSIZE,"[cache_get_binding] block index range error [block index (%d) > max blocks (%d)\n",block_index, cache[c_index].n_blocks);
       pups_error(errstr);
    }


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    binding = cache[c_index].hubness[block_index];

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(binding); 
}




/*--------------------------------------*/
/* Set binding of specified cache block */
/*--------------------------------------*/

_PUBLIC int32_t cache_set_blockbinding(const _BOOLEAN  have_cache_lock,   // If TRUE lock held on cache
                                       const uint32_t          binding,   // Block binding 
                                       const uint32_t          c_index,   // Cache index
                                       const uint32_t      block_index)   // Cache block index

{ 

    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_set_blockbinding] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }


    /*-------------------------*/
    /* Block index range error */
    /*-------------------------*/

    if(block_index >= cache[c_index].n_blocks)
    {  (void)snprintf(errstr,SSIZE,"[cache_set_blockbinding] block index range error [block index (%d) > max blocks (%d)\n",block_index, cache[c_index].n_blocks);
       pups_error(errstr);
    }


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    cache[c_index].binding[block_index] = binding;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}





/*---------------------------------------*/
/* Get co-ordination list size for cache */
/*---------------------------------------*/

_PUBLIC uint32_t cache_get_colsize(const _BOOLEAN  have_cache_lock,  // If TRUE lock held on cache
                                   const uint32_t          c_index)  // Cache index

{   uint32_t colsize = 0;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_set_colsize] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    colsize = cache[c_index].colsize;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(colsize); 
}




/*---------------------------------------*/
/* Set co-ordination list-size for cache */
/*---------------------------------------*/

_PUBLIC int32_t cache_set_colsize(const _BOOLEAN  have_cache_lock,  // If TRUE lock held on cache
                                  uint32_t                colsize,  // Co-ordination list size 
                                  const uint32_t          c_index)  // Cache index

{ 

    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_set_colsize] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    cache[c_index].colsize = colsize;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*--------------------------*/
/* Swap cache table entries */
/*--------------------------*/

_PRIVATE void swap_cache_table_entries(int c_index, const int32_t index_1, const int32_t index_2)

{   uint32_t         tmp_int;
    pthread_rwlock_t tmp_rwlock;
    block_mtype      tmp_blockmap;


    /*-----------------*/
    /* Swap block maps */
    /*-----------------*/

    tmp_blockmap                     = cache[c_index].blockmap[index_1];
    cache[c_index].blockmap[index_1] = cache[c_index].blockmap[index_2];
    cache[c_index].blockmap[index_2] = tmp_blockmap;


    /*-----------------*/
    /* Swap block tags */
    /*-----------------*/

    tmp_int                          = cache[c_index].tag[index_1];
    cache[c_index].tag[index_1]      = cache[c_index].tag[index_2];
    cache[c_index].tag[index_2]      = tmp_int;


    /*------------------*/
    /* Swap block flags */
    /*------------------*/

    tmp_int                          = cache[c_index].flags[index_1];
    cache[c_index].flags[index_1]    = cache[c_index].flags[index_2];
    cache[c_index].flags[index_2]    = tmp_int;


    /*-----------------------------*/
    /* Swap block read/write locks */
    /*-----------------------------*/

    tmp_rwlock                       = cache[c_index].rwlock[index_1];
    cache[c_index].rwlock[index_1]   = cache[c_index].rwlock[index_2];
    cache[c_index].rwlock[index_2]   = tmp_rwlock;
}
 



/*---------------------------*/
/* Move block (to fill hole) */
/*---------------------------*/

_PRIVATE void move_block(const int32_t c_index, const int32_t block_index, const int32_t hole_index)

{   uint32_t i;

    void     *to_ptr   = (void *)NULL,
             *from_ptr = (void *)NULL;
                                                                                       /*-------------------------------------*/
    to_ptr   = (void *)(cache[c_index].cache_ptr                                   +   /* Base of cache                       */
                       (uint64_t         )hole_index  * cache[c_index].block_size  );  /* Location of hole to file in cache   */
                                                                                       /*-------------------------------------*/

                                                                                        /*------------------------------------*/ 
    from_ptr = (void *)(cache[c_index].cache_ptr                                    +   /* Base of cache                      */
                       (uint64_t         )block_index * cache[c_index].block_size   );  /* Location of block to move in cache */
                                                                                        /*------------------------------------*/

    /*------------*/
    /* Move block */
    /*------------*/

    (void)memcpy(to_ptr,from_ptr,(uint64_t         )cache[c_index].block_size);


    /*----------------------------*/
    /* Update cache table entries */
    /* to reflect moved block     */
    /*----------------------------*/

    swap_cache_table_entries(c_index,hole_index,block_index);

    cache[c_index].flags[hole_index]  |=  BLOCK_USED;
    cache[c_index].flags[block_index] &= ~BLOCK_USED;
}




/*---------------*/
/* Compact cache */
/*---------------*/

_PUBLIC int32_t cache_compact(const _BOOLEAN have_cache_lock,  // If TRUE we hold lock on cache
                              const  int32_t         c_index)  // Cache index

{   uint32_t i,
             n_blocks,
             freed_blocks = 0,
             hole_index   = 0,
             used_blocks  = 0,
             free_blocks  = 0,
             shrink_size  = 0;

    _BOOLEAN hole_fill = FALSE;


    /*---------------*/
    /* Sanity check  */
    /*---------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_change_blocktag] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);


    /*---------------------------------------------*/
    /* Reset all blocklocks before cache is walked */
    /*---------------------------------------------*/

    //(void)cache_reset_blocklocks(have_cache_lock,c_index);
    #endif /* PTHREAD_SUPPORT */


    /*------------------------------------------------*/
    /* Walk cache filling in any holes. Note this is  */
    /* a strictly serial operation as there iterative */
    /* dependencies                                   */                
    /*------------------------------------------------*/


    n_blocks = cache_get_blocks(TRUE,c_index);
    for(i=0; i<n_blocks; ++i)
    {  


        /*-----------------------------------------*/
        /* First free block so start filling holes */
        /*-----------------------------------------*/

        if(! (cache[c_index].flags[i] & BLOCK_USED))
        {  if(hole_fill == FALSE)
           {  hole_index = i;
              hole_fill  = TRUE;
           }
        }


        /*--------------------------------------*/
        /* Used block so move it to full bubble */
        /*--------------------------------------*/

        else if(hole_fill == TRUE)
        {  if(cache[c_index].flags[i] & BLOCK_USED)
           {  (void)move_block(c_index,i,hole_index);
              ++hole_index;
              ++freed_blocks;
           }
        }

    }


    /*------------------------------------------------*/
    /* We can only shrink a public (read/write) cache */
    /*------------------------------------------------*/

    if(cache[c_index].mmap & CACHE_PUBLIC)
    {

       /*--------------*/
       /* Shrink cache */
       /*--------------*/

       used_blocks = cache_blocks_used(TRUE,c_index);
       free_blocks = n_blocks - used_blocks;
       shrink_size = n_blocks - BLOCK_ALLOC_QUANTUM * (free_blocks / BLOCK_ALLOC_QUANTUM);


       /*------------------------*/
       /* Shrink cache if we can */
       /*------------------------*/

       if(shrink_size > 0)
          (void)cache_resize(TRUE, shrink_size, c_index);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(freed_blocks);
}




/*--------------*/
/* Resize cache */
/*--------------*/

_PUBLIC int32_t cache_resize(const _BOOLEAN  have_cache_lock,  // Lock on cache held if TRUE
                             const uint32_t         n_blocks,  // New size of cache (in blocks)
                             const uint32_t          c_index)  // Cache index

{    int32_t i,
             map_flags  = 0;

    des_t    fd         = (-1);

    uint64_t new_size;
    void     *cache_ptr = (void *)NULL;
 

    /*--------------*/
    /* Sanity check */
    /*--------------*/
    
    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_resize] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */


    /*---------------------------------------*/
    /* If cache size hasn't changed we don't */
    /* have to do anything                   */
    /*---------------------------------------*/

    if(n_blocks == cache[c_index].n_blocks)
    {

       #ifdef PTHREAD_SUPPORT
       if(have_cache_lock == FALSE)
          (void)pthread_mutex_unlock(&cache[c_index].mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(OK);
       return(0);
    }


    /*---------------------------*/
    /* Cache is memory mapped so */
    /* get mapping flags         */
    /*---------------------------*/
    /*-------------------*/
    /* Read only mapping */
    /*-------------------*/

    if(cache[c_index].mmap & CACHE_PRIVATE)
       map_flags = MAP_PRIVATE;


    /*--------------------*/
    /* Read/write mapping */
    /*--------------------*/

    else if(cache[c_index].mmap & CACHE_PUBLIC)
       map_flags = MAP_SHARED;


    /*-----------------------------------------*/
    /* Populate mapping to prevent page faults */
    /*-----------------------------------------*/

    if(cache[c_index].mmap & CACHE_POPULATE)
       map_flags |= MAP_POPULATE;


    /*----------------------------------------*/
    /* Adjust the number of blocks and then   */
    /* initialise the new blocks created      */
    /*----------------------------------------*/

    new_size = n_blocks*cache[c_index].block_size;


    /*----------------------*/
    /* Unmap memory segment */
    /*----------------------*/

    if(munmap(cache[c_index].cache_ptr,cache[c_index].cache_size) == (-1))
       pups_error("[cache_resize] cannot unmap cache");

    //munmap(cache[c_index].cache_ptr,cache[c_index].cache_size);



    /*---------------------*/
    /* Extend backing file */
    /*---------------------*/

    (void)posix_fallocate(cache[c_index].mmap_fd,0,new_size);


    /*------------------------------------------*/
    /* Remap (and possibly move) memory segment */
    /*------------------------------------------*/
    /*-------*/
    /* Error */
    /*-------*/

    if((cache_ptr = mmap(NULL,
                         new_size,
                         PROT_READ  | PROT_WRITE,
                         map_flags,
                         cache[c_index].mmap_fd,
                         0L)) == (void *)MAP_FAILED)
       pups_error("[cache_resize] could not extend cache");


    /*-----------------------------*/
    /* Reallocate block parameters */
    /*-----------------------------*/

    cache[c_index].flags       = (_BYTE            *)pups_realloc((void *)cache[c_index].flags,   n_blocks*sizeof(_BYTE));
    cache[c_index].tag         = (uint32_t         *)pups_realloc((void *)cache[c_index].tag,     n_blocks*sizeof(uint32_t   ));
    cache[c_index].lifetime    = (uint64_t         *)pups_realloc((void *)cache[c_index].lifetime,n_blocks*sizeof(int));
    cache[c_index].rwlock      = (pthread_rwlock_t *)pups_realloc((void *)cache[c_index].rwlock,  n_blocks*sizeof(pthread_rwlock_t));
    cache[c_index].blockmap    = (block_mtype      *)pups_realloc((void *)cache[c_index].blockmap,n_blocks*sizeof(block_mtype));
    cache[c_index].n_blocks    = n_blocks;
    cache[c_index].cache_size  = new_size;
    cache[c_index].cache_ptr   = cache_ptr;


    /*-------------------------------------------------------*/
    /* Recompute cache offsets (and initialise extra blocks) */
    /*-------------------------------------------------------*/

    for(i=0;  i<n_blocks; ++i)
    {  uint32_t    j;

       for(j=0; j<cache[c_index].n_objects; ++j)                                                          /*----------------------------*/
       {  cache[c_index].blockmap[i].object_ptr[j] = (void *)(cache[c_index].object_offset[j]          +  /* Object offset within block */
                                                     (uint64_t         )i*cache[c_index].block_size    +  /* Block offset within cache  */
                                                     (uint64_t         )cache[c_index].cache_ptr);        /* Base address of cache      */
       }                                                                                                  /*----------------------------*/

       #ifdef DEBUG
       (void)fprintf(stderr,"BLOCK %d (of %d)\n",i,n_blocks-1);
       (void)fflush(stderr);
       #endif /* DEBUG */


       /*---------------------------------------*/
       /* Initialise extra per block parameters */
       /*---------------------------------------*/

       if(i > cache[c_index].n_blocks)
       {

          /*------------------------*/
          /* Initialise extra flags */
          /*------------------------*/
           
          cache[c_index].flags[i] = 0;


          /*-----------------------------*/
          /* Initialise extra block tags */
          /*-----------------------------*/

          cache[c_index].tag[i] = 0;


          /*----------------------------------*/
          /* Initialise extra block lifetimes */
          /*----------------------------------*/

          cache[c_index].tag[i] = BLOCK_IMMORTAL;


          /*-------------------------*/
          /* Initialise extra wlocks */
          /*------------------------*/

          (void)pthread_rwlock_init(&cache[c_index].rwlock[i],(pthread_rwlockattr_t *)NULL);
       }
    }


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}



/*------------*/
/* Lock cache */
/*------------*/

_PUBLIC int32_t cache_lock(const uint32_t  c_index)

{ 

    /*--------------*/
    /* Sanity check */
    /*--------------*/
    
    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_lock] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    (void)pthread_mutex_lock(&cache[c_index].mutex);

    pups_set_errno(OK);
    return(0);
}


/*--------------*/
/* Unlock cache */
/*--------------*/

_PUBLIC int32_t cache_unlock(const uint32_t c_index)

{

    /*--------------*/
    /* Sanity check */
    /*--------------*/
    
    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_unlock] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    (void)pthread_mutex_unlock(&cache[c_index].mutex);

    pups_set_errno(OK);
    return(0);
}



/*-------------------------------*/
/* Is cache private (read only)? */
/*-------------------------------*/

_PUBLIC _BOOLEAN cache_is_private(const _BOOLEAN have_cache_lock, const uint32_t c_index)

{   _BOOLEAN ret = FALSE;


    /*--------------*/
    /* Sanity check */
    /*--------------*/
    
    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_is_private] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    if(cache[c_index].mmap & CACHE_PRIVATE)
       ret = TRUE;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(ret);
}




/*---------------------*/
/* Is cache preloaded? */
/*---------------------*/

_PUBLIC _BOOLEAN cache_is_preloaded(const _BOOLEAN have_cache_lock, const uint32_t c_index)

{   _BOOLEAN ret = FALSE;


    /*--------------*/
    /* Sanity check */
    /*--------------*/
    
    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_is_preloaded] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    if(cache[c_index].mmap & CACHE_POPULATE)
       ret = TRUE;

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(ret);
}


/*---------------------------------*/
/* Set cache auxilliary data field */
/*---------------------------------*/

_PUBLIC int32_t cache_set_auxinfo(const _BOOLEAN have_cache_lock, const char *auxinfo, const uint32_t c_index)

{ 

    /*---------------*/
    /* Sanity checks */
    /*---------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_set_auxinfo] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    if(auxinfo == (char *)NULL)
       pups_error("[cache_set_auxinfo] auxinfo parameter is NULL");

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)strlcpy(cache[c_index].auxinfo,auxinfo,SSIZE);

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*---------------------------------*/
/* Get cache auxilliary data field */
/*---------------------------------*/

_PUBLIC int32_t cache_get_auxinfo(const _BOOLEAN have_cache_lock, char *auxinfo, const uint32_t c_index)

{ 

    /*---------------*/
    /* Sanity checks */
    /*---------------*/

    if(c_index >= MAX_CACHES)
    {  (void)snprintf(errstr,SSIZE,"[cache_get_auxinfo] cache index range error [cache index (%d) > max caches (%d)\n",c_index,MAX_CACHES);
       pups_error(errstr);
    }

    if(auxinfo == (char *)NULL)
       pups_error("[cache_get_auxinfo] auxinfo parameter is NULL");


    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_lock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)strlcpy(auxinfo,cache[c_index].auxinfo,SSIZE);

    #ifdef PTHREAD_SUPPORT
    if(have_cache_lock == FALSE)
       (void)pthread_mutex_unlock(&cache[c_index].mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}
