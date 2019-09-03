/*---------------------------------------------------------------------------
    Purpose: Fast cache library.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             NE3 54RT
             Tyne and Wear

    Version: 2.00 
    Dated:   30th August 2019 
    Email:   mao@tumblingdice.co.uk
---------------------------------------------------------------------------*/

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

/* Slot information function */
_PRIVATE void cache_slot(int level)
{   (void)fprintf(stderr,"lib cachelib %s: [ANSI C]\n",CACHELIB_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(c) 2001-2018 Tumbling Dice, Gosforth\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"PUPS/P3 fast caching library (built %s %s)\n",__TIME__,__DATE__);
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




/*-------------------------------------------*/
/* Variableswhich are private to this module */
/*-------------------------------------------*/


/*--------------------------*/
/* Cache table access mutex */
/*--------------------------*/

#ifdef PTHREAD_SUPPORT
_PRIVATE pthread_mutex_t cache_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif /* PTHREAD_SUPPORT */


/*-------------*/
/* Cache table */
/*-------------*/

/*_PRIVATE*/ _BOOLEAN cache_table_initialised = FALSE;
/*_PRIVATE*/          cache_type cache[MAX_CACHES];




/*-------------------------------------------*/
/* Function which are private to this module */
/*-------------------------------------------*/

// Print number of bytes in cache
_PRIVATE int print_bytes(FILE *, char *, long);

// Inversely map existing process memory into a
// ghost file, synchronising it with memory area
_PRIVATE void *mmap_invmap_cachememory(char *, unsigned long int);

// Memory map cache directly into the address space
// of the program
_PRIVATE void *mmap_fwdmap_cachememory(char *, unsigned long int);




/*--------------------------------------------------*/
/* Memory map cache directly into the address space */
/* of the program                                   */
/*--------------------------------------------------*/

_PRIVATE void *mmap_fwdmap_cachememory(char *file_name, unsigned long int size)

{  int   fd;
   void *cache_ptr = (void *)NULL;


   /*-------------------------------------------------*/
   /* File permissions must match memory mapping mode */
   /*-------------------------------------------------*/

   if((fd = open(file_name,O_RDWR)) == (-1))
      return((void *)NULL);

   if((cache_ptr = mmap(NULL,
                        size,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED,
                        fd,
                        0L)) == (void *)NULL)
   {   (void)close(fd);
       return((void *)NULL);
   }

   (void)close(fd);

   if(appl_verbose == TRUE)
   {  (void)strdate(date);
      (void)fprintf(stderr,"%s %s %d@%s:%s: cache mapped to file \"%s\" at 0x%lx virtual, %d\n",
                    date,appl_name,appl_pid,appl_host,appl_owner,file_name,(unsigned long int)cache_ptr,errno);
      (void)fflush(stderr);
   }

   return(cache_ptr);
}




/*-----------------------------------------------*/
/* Inversely map existing process memory into a  */
/* ghost file, synchronising it with memory area */
/*-----------------------------------------------*/

_PRIVATE void *mmap_invmap_cachememory(char *file_name, unsigned long int size)

{    int  fd         = (-1);
     void *cache_ptr = (void *)NULL;


     /*----------------------------------------------*/
     /* Creat a ghost file of the right size for the */
     /* cache and then map cache                     */
     /*----------------------------------------------*/

     if(access(file_name,F_OK | R_OK | W_OK) == (-1))
     {  if((fd = open(file_name,O_RDWR | O_CREAT,0600)) == (-1))
        {  pups_set_errno(ENOENT);
           return((void *)NULL);
        }
     }
     else
        fd = open(file_name,O_RDWR);

     (void)ftruncate(fd,size);
     (void)posix_fallocate(fd, 0, size);
     if((cache_ptr = mmap(NULL,
                          size,
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED,
                          fd,
                          0L)) == (void *)NULL)
    {    (void)close(fd);
         return((void *)NULL);
    }

    (void)close(fd);
    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s %d@%s:%s: cache inversely mapped to file \"%s\" at 0x%lx virtual\n",
                            date,appl_name,appl_pid,appl_host,appl_owner,file_name,(unsigned long int)cache_ptr);
       (void)fflush(stderr);
    }

    return(cache_ptr);
}




/*-------------------------------------*/
/* Return the index of a (named) cache */
/*-------------------------------------*/

_PUBLIC int cache_name2index(const char *name)

{   int i;

    if(name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    for(i=0; i<MAX_CACHES; ++i)
    {  if(strcmp(cache[i].name,name) == 0)
       {

          #ifdef PTHREAD_SUPPORT
          (void)pupsthread_mutex_unlock(&cache_mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(i);
       }


    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return(-1);

}



/*------------------------------*/
/* Map cache memory - create an */
/* inverse map if backing file  */
/* does not exit                */
/*------------------------------*/

_PUBLIC void *cache_mmap_cachememory(char *file_name, unsigned long int size)

{   void *cache_ptr = (void *)NULL;

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("attempt by non root thread to perform PUPS/P3 memory mapped cache operation");

    
    if(access(file_name,F_OK | R_OK | W_OK) == (-1))
       cache_ptr = mmap_invmap_cachememory(file_name,size);
    else
    {  unsigned long int old_size;


       /*----------------------------------------------------*/
       /* Do we need to extend the size of the mapping file? */
       /*----------------------------------------------------*/

       old_size = pups_get_fsize(file_name);
       if(old_size < size)
          cache_ptr = mmap_invmap_cachememory(file_name,size);
       else
          cache_ptr = mmap_fwdmap_cachememory(file_name,size);
    }

    if(cache_ptr == (void *)NULL)
       pups_set_errno(ENOMEM);
    else
       pups_set_errno(OK);

    return(cache_ptr);
}




/*----------------------------*/
/* Initialise the cache table */
/*----------------------------*/

_PUBLIC int cache_table_init(void)

{   int i;

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    cache_table_initialised = TRUE;
    for(i=0; i<MAX_CACHES; ++i)
    {  (void)strlcpy(cache[i].name,     "",SSIZE);
       (void)strlcpy(cache[i].mmap_name,"",SSIZE);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*------------------*/
/* Initialise cache */
/*------------------*/

_PUBLIC int cache_init_cache(void)

{   int i,
        c_index;


    char mmap_file_name[SSIZE] = "";

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */


    /*--------------------------------------*/
    /* Have we initialised the cache table? */
    /*--------------------------------------*/

    if(cache_table_initialised == FALSE)
    {

       #ifdef PTHREAD_SUPPORT
       (void)pupsthread_mutex_unlock(&cache_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EINVAL);
       return(-1);
    }

    for(i=0; i<MAX_CACHES; ++i)
    {   int j;

        /*---------------------------------------*/
        /* Find unallocated entry in cache table */
        /*---------------------------------------*/

        (void)strlcpy(cache[i].name,"",SSIZE);
        cache[i].mmap = FALSE;
        cache[i].n_blocks   = 0;
        cache[i].n_objects  = 0;
        cache[i].block_size = 0L;
        cache[i].cache_size = 0L;

        for(j=0; j<MAX_CACHE_BLOCK_OBJECTS; ++j)
        {  cache[i].object_size[i]   = 0L;
           cache[i].object_offset[i] = 0L;
        }

        cache[i].cachemap     = (cache_mtype *)NULL;
        cache[i].cache_memory = (void *)NULL;
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(EINVAL);
    return(0);
}




/*---------------------*/
/* Add object to cache */
/*---------------------*/

_PUBLIC int cache_add_object(const unsigned long int size, const unsigned int c_index)

{   

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    /*------------------*/
    /* Check parameters */
    /*------------------*/

    if(c_index > MAX_CACHES)
    {  

       #ifdef PTHREAD_SUPPORT
       (void)pupsthread_mutex_unlock(&cache_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(ERANGE);
       return(-1);
    }
    else if(cache[c_index].n_objects > MAX_CACHE_BLOCK_OBJECTS || cache[c_index].cache_memory != (void *)NULL)
    {  

       #ifdef PTHREAD_SUPPORT
       (void)pupsthread_mutex_unlock(&cache_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(E2BIG);
       return(-1);
    }

    cache[c_index].object_size[cache[c_index].n_objects] = size;
    ++cache[c_index].n_objects;

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0); 
}




/*------------------------*/
/* Strip suffix from name */
/*------------------------*/

_PRIVATE int stripname(char *name, char *basename)

{   int i,
        size;

    size = strlen(name); 
    for(i=size; i>0; --i)
    {  if(name[i] == '/') 
          break;
       else if(name[i] == '.')
       {  (void)strlcpy(basename,(char *)name[i+1],SSIZE);
          return(0);
       }
    }

    (void)strlcpy(basename,name,SSIZE);
    return(-1);
}




/*--------------------------*/
/* Get machine architecture */
/*--------------------------*/

_PUBLIC int get_march(char *march)

{    char uname_cmd[SSIZE] = "";
     FILE *pstream         = (FILE *)NULL;

     if(march == (char *)NULL)
     {  pups_set_errno(EINVAL);
        return(-1);
     }


     /*--------------------------*/
     /* Set machine architecture */
     /*--------------------------*/

     (void)strlcpy(uname_cmd,"uname -m",SSIZE);
     if((pstream = popen(uname_cmd,"r")) == (FILE *)NULL)
     {  pups_set_errno(EPERM);
        return(-1);
     }

     (void)fscanf(pstream,"%s",march);
     (void)pclose(pstream);

     pups_set_errno(OK);
     return(0);
}




/*----------------------------------------------------*/
/* Creat a dynamic multiple block cache with multiple */
/* objects per block                                  */
/*----------------------------------------------------*/

_PUBLIC int cache_create(const _BOOLEAN mmap, const char *name, const unsigned int n_blocks, const unsigned int c_index)

{   int i;

    unsigned long int current_offset = 0L;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }
    else if(c_index > MAX_CACHES)
    {  pups_set_errno(ERANGE);
       return(-1);
    }
    else if(n_blocks != CACHE_USING_MAPINFO)
    {  char basename[SSIZE] = "";


       /*----------------*/
       /* Set cache name */
       /*----------------*/

       (void)stripname(name,basename);
       (void)strlcpy(cache[c_index].name,basename,SSIZE);
       if(mmap == TRUE)
       {  

          /*--------------------------*/
          /* Set machine architecture */
          /*--------------------------*/

          (void)get_march(cache[c_index].march);


          /*-----------------------------------*/
          /* Set name of (memory) mapping file */
          /*-----------------------------------*/

          cache[c_index].mmap = TRUE;
          (void)snprintf(cache[c_index].mmap_name,SSIZE,"%s.mmc",basename);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */


    if(n_blocks != CACHE_USING_MAPINFO)
    {

       /*---------------------------------------------*/
       /* Compute offsets of all blocks and mark them */
       /* unused and accessible                       */
       /*---------------------------------------------*/

       for(i=0; i<=cache[c_index].n_objects; ++i)
       {  current_offset                  += cache[c_index].object_size[i-1];
          cache[c_index].object_offset[i]  = current_offset;    
       }

       cache[c_index].flags = (_BYTE *)pups_calloc(n_blocks,sizeof(_BYTE));
       for(i=0; i<n_blocks; ++i)
           cache[c_index].flags[i] = 0;


       /*-----------------------------------------*/
       /* Get offsets into cache block (in bytes) */
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

    }


    /*---------------------------------------------*/
    /* Allocate contiguous memory for entire cache */
    /*---------------------------------------------*/

    if(cache[c_index].mmap == TRUE)
    {  char my_march[SSIZE] = "";


       /*----------------------------------------------*/
       /* Map cache segment into process address space */
       /*----------------------------------------------*/

       if((cache[c_index].cache_memory = (void *)cache_mmap_cachememory(cache[c_index].mmap_name, cache[c_index].n_blocks*cache[c_index].block_size)) == (void *)NULL)
       {

          #ifdef PTHREAD_SUPPORT
          (void)pupsthread_mutex_unlock(&cache_mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(ENOMEM);
          return(-1);
       }


       /*------------------------------------*/
       /* Check that the architecture used   */
       /* to create cache is compatible with */
       /* the one running calling process    */
       /*------------------------------------*/

       (void)get_march(my_march);
       if(strcmp(my_march,cache[c_index].march) != 0)
       {  (void)snprintf(errstr,SSIZE,"cache was created on \"%s\", but current architecture is \"%s\")",cache[c_index].march,my_march);
          error(errstr);
       }
    }
    else
    {

       /*------------------------------*/
       /* Create cache on process heap */
       /*------------------------------*/

       if((cache[c_index].cache_memory = (void *)pups_calloc(cache[c_index].n_blocks,cache[c_index].block_size)) == (void *)NULL)
       {

          #ifdef PTHREAD_SUPPORT
          (void)pupsthread_mutex_unlock(&cache_mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(ENOMEM);
          return(-1);
       }
    }


    /*----------------------------------*/
    /* Allocate cache block pointer map */
    /*----------------------------------*/

    if((cache[c_index].cachemap = (cache_mtype *)pups_calloc(cache[c_index].n_blocks,sizeof(cache_mtype))) == (cache_mtype *)NULL)
    {

       #ifdef PTHREAD_SUPPORT
       (void)pupsthread_mutex_unlock(&cache_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(ENOMEM);
       return(-1);
    }


    /*-----------------------------------------*/
    /* Map object pointers within cache blocks */
    /*-----------------------------------------*/

    for(i=0; i<cache[c_index].n_blocks; ++i)
    {  int j;

       for(j=0; j<cache[c_index].n_objects; ++j)                                                          /*----------------------------*/
          cache[c_index].cachemap[i].object_ptr[j] = (void *)(cache[c_index].object_offset[j]          +  /* Object offset within block */
                                                     (unsigned long int)i*cache[c_index].block_size    +  /* Block offset within cache  */
                                                     (unsigned long int)cache[c_index].cache_memory);     /* Base address of cache      */
                                                                                                          /*----------------------------*/
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}



/*---------------*/
/* Destroy cache */
/*---------------*/

_PUBLIC int cache_destroy(const unsigned int c_index)

{   int i; 

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index > MAX_CACHES)
    {  pups_set_errno(ERANGE);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */


    /*--------------------------------*/
    /* Clear cache datastructures and */
    /* free allocated memory          */
    /*--------------------------------*/

    (void)strlcpy(cache[c_index].name,     "",SSIZE);
    (void)strlcpy(cache[c_index].mmap_name,"",SSIZE);

    cache[c_index].n_blocks   = 0;
    cache[c_index].n_objects  = 0;
    cache[c_index].block_size = 0L;
    cache[c_index].cache_size = 0L;

    for(i=0; i<MAX_CACHE_BLOCK_OBJECTS; ++i)
    {  cache[c_index].object_size[i]   = 0L;
       cache[c_index].object_offset[i] = 0L;
    }


    /*---------------*/
    /* Free cachemap */
    /*---------------*/

    if(cache[c_index].cachemap != (cache_mtype *)NULL)
    {  (void)pups_free((void *)cache[c_index].cachemap);
       cache[c_index].cachemap = (cache_mtype *)NULL;
    }


    /*----------------------------*/
    /* Free (mapped) cache memory */
    /*----------------------------*/

    if(cache[c_index].cache_memory != (void *)NULL)
    {  if(cache[c_index].mmap == TRUE)
       {  (void)munmap((void *)cache[c_index].cache_memory,cache[c_index].cache_size);
          cache[c_index].mmap = FALSE;
       }
       else
          (void)pups_free((void *)cache[c_index].cache_memory);

       cache[c_index].cache_memory = (void *)NULL;
    }


    /*------------------*/
    /* Free cache flags */
    /*------------------*/

    if(cache[c_index].flags != (void *)NULL)
    {  (void)pups_free((void *)cache[c_index].flags);
       cache[c_index].flags = (_BYTE *)NULL;
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*----------------------------------*/
/* Print bytes in appropriate units */
/*----------------------------------*/

_PRIVATE int print_bytes(FILE *stream, char *text, long size)

{   char   eff_text[SSIZE] = "";
    double fsize;

    if(stream == (FILE *)NULL)
       return(-1);

    if(text == (char *)NULL || strcmp(text,"") == 0)
      (void)strlcpy(eff_text,"size",SSIZE);
    else
      (void)strlcpy(eff_text,text,SSIZE);

    if(size > GIGABYTE)
    {  fsize = (double)size / (double)GIGABYTE;
       (void)fprintf(stream,"    %s:  %4.2f Gigabytes\n",eff_text,fsize);
    }
    else if(size > MEGABYTE)
    {  fsize = (double)size / (double)MEGABYTE;
       (void)fprintf(stream,"    %s:  %4.2f Megabytes\n",eff_text,fsize);
    }
    else if(size > KILOBYTE)
    {  fsize = (double)size / (double)KILOBYTE;
       (void)fprintf(stream,"    %s:  %4.2f Kilobytes\n",eff_text,fsize);
    }
    else
       (void)fprintf(stream,"    %s:  %4d bytes\n",      eff_text,size);

    return(0);
}




/*--------------------------*/
/* Display cache statistics */
/*--------------------------*/

_PUBLIC int cache_display_statistics(const FILE *stream, const unsigned int c_index)

{   int i,
        used       = 0,
        accessible = 0;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(stream == (const FILE *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }
    else if(c_index > MAX_CACHES)
    {  pups_set_errno(ERANGE);
       return(-1);
    }

    if(stream == (FILE *)NULL || cache == (cache_type *)NULL)
       return(-1);

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */


    /*----------------------------------*/
    /* Find the number of object in use */
    /* and accessible                   */
    /*----------------------------------*/

    for(i=0; i<cache[c_index].n_blocks; ++i)
    {   if(cache[c_index].flags[i] & BLOCK_USED)
        {  ++used;

           if(!(cache[c_index].flags[i] & BLOCK_ACCESS))
              ++accessible;
        }
    }


    /*--------------------*/
    /* Name of this cache */
    /*--------------------*/

    (void)fprintf(stream,"\n    Cache statistics\n");
    (void)fprintf(stream,"    ================\n\n");
    (void)fprintf(stream,"    %-32s:\t\t\t\"%-32s\"\n",                                            "cache name",      cache[c_index].name);
    (void)fprintf(stream,"    %-32s:\t\t\tat 0x%lx virtual\n",                                     "cache located at",(unsigned long int)cache[c_index].cache_memory);
    (void)fprintf(stream,"    %-32s:\t\t\t%d blocks (of %d objects) %d used, %4d accessible\n","cache geometry",   cache[c_index].n_blocks,cache[c_index].n_objects,used,accessible);
 


    if(strcmp(cache[c_index].march,"") != 0) 
       (void)fprintf(stream,"    %-32s:\t\t\t\"%-.48s\"\n",                            "cache (machine) architecture",cache[c_index].march);

    if(cache[c_index].mmap == TRUE)
       (void)fprintf(stream,"    %-32s:\t\t\tmemory mapped (mmap file \"%-.48s\")\n\n","cache type",                  cache[c_index].mmap_name);
    else 
       (void)fprintf(stream,"    %-32s:\t\t\t(local) heap\n\n",                        "cache type");

    (void)fflush(stream);


    /*--------------------------*/
    /* Sizes of cache and block */
    /*--------------------------*/

    (void)print_bytes(stream,"cache size",cache[c_index].cache_size);


    /*---------------------------------------------*/
    /* Object sizes and offsets within cache block */ 
    /*---------------------------------------------*/

    (void)fprintf(stream,"\n    Object sizes and offsets within cache block\n");
    (void)fprintf(stream,"    ===========================================\n\n");
    (void)fflush(stream);


    /*--------------------------------*/
    /* Block object siees and offsets */
    /*--------------------------------*/

    for(i=0; i<cache[c_index].n_objects; ++i)
    {  char objectstr[SSIZE] = "";

       (void)snprintf(objectstr,SSIZE,"    object[%04d] size        ",i); 
       (void)print_bytes(stream,objectstr,cache[c_index].object_size[i]);

       (void)snprintf(objectstr,SSIZE,"    object[%04d] block offset",i); 
       (void)print_bytes(stream,objectstr,cache[c_index].object_offset[i]);
    }

    if(cache[c_index].n_objects == 0)
       (void)fprintf(stream,"    no objects allocated\n");
    else if(cache[c_index].n_objects == 1)
       (void)fprintf(stream,"\n\n    %04d object in cache\n\n",1);
    else
       (void)fprintf(stream,"\n\n    %04d objects in cache\n\n",cache[c_index].n_objects);
    (void)fflush(stream);


    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    return(0);
}




/*---------------------*/
/* Display cache table */
/*---------------------*/

_PUBLIC int cache_display(const FILE *stream)

{   int i,
        mapped_caches = 0;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    /*------------------*/
    /* Check parameters */
    /*------------------*/

    if(stream == (const FILE *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)fprintf(stream,"\n    Mapped data caches\n");
    (void)fprintf(stream,"    ==================\n\n");

    for(i=0; i<MAX_CACHES; ++i)
    {  if(strcmp(cache[i].name,"") != 0)
       {  ++mapped_caches;

          (void)fprintf(stream,"    %04d: (\"%-32s\"): mapped into process address space at 0x%010x virtual\n",i,
                                                                                                   cache[i].name,
                                                                            (unsigned long int)cache[i].cachemap);
          (void)fflush(stream);
       }
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

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}



/*------------------------------------*/
/* Return size of object within block */
/*------------------------------------*/

_PUBLIC int cache_object_size(const unsigned int block_index, const unsigned int object_index, const unsigned int c_index)

{   unsigned long int object_size;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index > MAX_CACHES)
    {  pups_set_errno(ERANGE);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    if(block_index  < 0                          ||
       block_index  > cache[c_index].n_blocks    ||
       object_index < 0                          ||
       object_index > cache[c_index].n_objects    )
    {  

       #ifdef PTHREAD_SUPPORT
       (void)pupsthread_mutex_unlock(&cache_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(ERANGE);
       return(-1);
    }

    object_size = cache[c_index].object_size[object_index];
   
    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK); 
    return(object_size);
}




/*---------------------------------------------------*/
/* Return (read only) pointer to object within block */
/*---------------------------------------------------*/

_PUBLIC const void *cache_access_object(const unsigned int cache_index, const unsigned int object_index, const unsigned int c_index)

{   void *object_ptr = (void *)NULL;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index > MAX_CACHES)
    {  pups_set_errno(ERANGE);
       return((void *)NULL);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    if(cache_index  < 0                           ||
       cache_index  >= cache[c_index].n_blocks    ||
       object_index < 0                           ||
       object_index >= cache[c_index].n_objects    )
    {

       #ifdef PTHREAD_SUPPORT
       (void)pupsthread_mutex_lock(&cache_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(ERANGE);
       return((void *)NULL);
    }


    /*--------------------------*/
    /* Is the block accessible? */
    /*--------------------------*/

    if(cache[c_index].flags[cache_index] & BLOCK_ACCESS)
    {

       #ifdef PTHREAD_SUPPORT
       (void)pupsthread_mutex_unlock(&cache_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EACCES);
       return((void *)NULL);
    } 

    object_ptr = cache[c_index].cachemap[cache_index].object_ptr[object_index]; 


#ifdef CACHELIB_DEBUG
(void)fprintf(stderr,"PTR 0x%010x CBLOCK %04d OBJECT %04d\n",(unsigned long int)object_ptr,cache_index,object_index);
(void)fflush(stderr);
#endif /* CACHELIB_DEBUG */


    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(object_ptr);
}




/*------------------------*/
/* Map 2D array to object */
/*------------------------*/

_PUBLIC void **cache_map_2D_array(const void *object_ptr, const unsigned int rows, const unsigned int cols, const unsigned long int size)

{   int  i;
    void **array_ptr = (void **)NULL;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("attempt by non root thread to perform PUPS/P3 memory mapped cache operation");


    if(object_ptr == (const void *) NULL)
    {  pups_set_errno(EINVAL);
       return((void **)NULL);
    }

    if((array_ptr = (void **)pups_calloc(rows,sizeof(void *))) == NULL)
    {  pups_set_errno(ENOMEM);
       return((void **)NULL);
    }

    for(i=0; i<rows; ++i)
       array_ptr[i] = (void *)((unsigned long int)object_ptr + (unsigned long int)(i*cols)*size);

    pups_set_errno(OK);
    return(array_ptr);
}



/*---------------------------------*/
/* Write contents of cache to file */
/*---------------------------------*/

_PUBLIC int cache_write(const char *wcache_file_name, const unsigned int c_index)

{   int fdes = (-1);

    if(c_index > MAX_CACHES || wcache_file_name == (const char *)NULL || strcmp(wcache_file_name,"") == 0)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if((fdes = open(wcache_file_name,O_CREAT | O_WRONLY, 0600)) == (-1))
    {  pups_set_errno(EPERM);
       return(-1);
    }


    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */ 

    (void)write(fdes,cache[c_index].cache_memory,cache[c_index].n_blocks*cache[c_index].block_size);


    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */ 

    (void)close(fdes);

    pups_set_errno(OK);
    return(0);
}



/*---------------------------------*/
/* Get the size of a cached object */
/*---------------------------------*/

_PUBLIC long int cache_get_object_size(const unsigned int c_index, const unsigned int o_index)

{   long int size;

    if(o_index > MAX_CACHE_BLOCK_OBJECTS - 1 || c_index > MAX_CACHES)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */ 

    size = (long int)cache[c_index].object_size[o_index];

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */ 

    pups_set_errno(OK);
    return(size);
}




/*-------------------------------*/
/* Get the size of a cache block */
/*-------------------------------*/

_PUBLIC long int cache_get_block_size(const unsigned int c_index)

{   int      i;
    long int size = 0L;

    if(c_index > MAX_CACHES)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */ 

    for(i=0; i<cache[c_index].n_objects; ++i)
       size += (long int)cache[c_index].object_size[i];

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */ 

    pups_set_errno(OK);
    return(size);
}



/*-------------------------------*/
/* Get number of blocks in cache */
/*-------------------------------*/

_PUBLIC long int cache_get_blocks(const unsigned int c_index)

{   int cache_blocks;

    if(c_index > MAX_CACHES)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    cache_blocks = cache[c_index].n_blocks;

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(cache_blocks);
}




/*--------------------------------------------*/
/* Get number of objectcts per block in cache */
/*--------------------------------------------*/

_PUBLIC long int cache_get_objects(const unsigned int c_index)

{   int block_objects;

    if(c_index > MAX_CACHES)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    block_objects = cache[c_index].n_objects;

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(block_objects);
}




/*----------------*/
/* Get cache name */
/*----------------*/

_PUBLIC _BOOLEAN cache_index2name(char *cache_name, const unsigned int c_index)

{   if(cache_name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(FALSE);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)strlcpy(cache_name,cache[c_index].name,SSIZE);

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    if(strcmp(cache_name,"") == 0)
       return(FALSE);

    return(TRUE);
}




/*-------------------*/
/* Get map file name */
/*-------------------*/

_PUBLIC _BOOLEAN cache_get_mmap_name(char *cache_mmap_name, const unsigned int c_index)

{   if(cache_mmap_name == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(FALSE);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)strlcpy(cache_mmap_name,cache[c_index].name,SSIZE);

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    if(strcmp(cache_mmap_name,"") == 0)
       return(FALSE);

    return(TRUE);
}




/*-------------------------*/
/* Is cache memory mapped? */
/*-------------------------*/

_PUBLIC _BOOLEAN cache_is_mapped(const unsigned int c_index)

{   _BOOLEAN mapped;

    if(c_index > MAX_CACHES)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    mapped = cache[c_index].mmap;

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(mapped);
}




/*---------------------------------*/
/* Write cache mapping information */
/*---------------------------------*/

_PUBLIC int cache_write_mapinfo(const char *map_name, const unsigned int c_index)

{   int i,
        fdes = (-1);

    char eff_map_name[SSIZE] = "";

    if(map_name == (char *)NULL || c_index > MAX_CACHES)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*------------------------------------*/
    /* If basename is specified expand it */
    /* to fully qualified mapfile name    */
    /*------------------------------------*/

    if(strin(map_name,".map") == FALSE)
       (void)snprintf(eff_map_name,SSIZE,"%s.map",map_name);
    else
       (void)strlcpy(eff_map_name,map_name,SSIZE);
 
    if((fdes = open(eff_map_name,O_WRONLY | O_CREAT,0600)) == (-1))
    {  pups_set_errno(EPERM);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */


    if(write(fdes,cache[c_index].name,SSIZE) == (-1))
    {  (void)close(fdes);
       pups_set_errno(ENOMEM);
       goto error_exit;
    }

    if(write(fdes,cache[c_index].mmap_name,SSIZE) == (-1))
    {  (void)close(fdes);
       pups_set_errno(ENOMEM);
       goto error_exit;
    }

    if(write(fdes,&cache[c_index].march,SSIZE) == (-1))
    {  (void)close(fdes);
       pups_set_errno(ENOMEM);
       goto error_exit;
    }
 
    if(write(fdes,&cache[c_index].mmap,sizeof(_BOOLEAN)) == (-1))
    {  (void)close(fdes);
       pups_set_errno(ENOMEM);
       goto error_exit;
    }

    if(write(fdes,&cache[c_index].n_blocks,sizeof(int)) == (-1))
    {  (void)close(fdes);
       pups_set_errno(ENOMEM);
       goto error_exit;
    }

    if(write(fdes,&cache[c_index].n_objects,sizeof(int)) == (-1))
    {  (void)close(fdes);
       pups_set_errno(ENOMEM);
       goto error_exit;
    }

    if(write(fdes,&cache[c_index].cache_size,sizeof(unsigned long int)) == (-1))
    {  (void)close(fdes);
       pups_set_errno(ENOMEM);
       goto error_exit;
    }

    if(write(fdes,&cache[c_index].block_size,sizeof(unsigned long int)) == (-1))
    {  (void)close(fdes);
       pups_set_errno(ENOMEM);
       goto error_exit;
    }

    // Write offsets of objects in cache blocks
    for(i=0; i<cache[c_index].n_objects; ++i)
    {   if(write(fdes,&cache[c_index].object_offset[i],sizeof(unsigned long int)) == (-1))
        {  (void)close(fdes);
           pups_set_errno(ENOMEM);
           goto error_exit;
        }
    }
 
    // Write sizes of objects cache blocks
    for(i=0; i<cache[c_index].n_objects; ++i) 
    {   if(write(fdes,&cache[c_index].object_size[i],sizeof(unsigned long int)) == (-1))
        {  (void)close(fdes);
           pups_set_errno(ENOMEM);
           goto error_exit;
        }
    }

    // Write cache block flags
    for(i=0; i<cache[c_index].n_blocks; ++i)
    {   if(write(fdes,&cache[c_index].flags[i],sizeof(_BYTE)) == (-1))
        {  (void)close(fdes);
           pups_set_errno(ENOMEM);
           goto error_exit;
        }
    }

    (void)close(fdes);
    pups_set_errno(OK);

error_exit:

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    return(0);
}




/*----------------------------------*/
/* Compress cache and map file into */
/* tar/xz archive                   */
/*----------------------------------*/

_PUBLIC int cache_archive(const _BOOLEAN compress, const _BOOLEAN delete, const char *cache_name)

{   int ret;

    char tar_cmd[SSIZE]   = "",
         basename[SSIZE]  = "",
         mapname[SSIZE]   = "",
         cachename[SSIZE] = "",
         *base_ptr        = (char *)NULL;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(cache_name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*----------------------------*/
    /* Get cache archive basename */
    /*----------------------------*/

    if((base_ptr = rindex(cache_name,'.')) != (char *)NULL)
       (void)stripname(cache_name,basename);

    (void)snprintf(mapname  ,SSIZE,"%s.map",basename); 
    (void)snprintf(cachename,SSIZE,"%s.mmc",basename);


    /*-----------------------------------*/
    /* Check that we have a map file and */
    /* corresponding cache               */
    /*-----------------------------------*/

    if(access(mapname,F_OK | R_OK | W_OK) == (-1) || access(mapname,F_OK  | R_OK | W_OK) == (-1))
    {  pups_set_errno(EINVAL);
       return(-1);
    }
    else
    {

       /*-------------------------------------*/
       /* Run tar and xz commands to produced */
       /* compressed cache archive            */
       /*-------------------------------------*/

       if(compress == TRUE)
          (void)snprintf(tar_cmd,SSIZE,"tar cvf - %s %s 2> /dev/null | xz > %s.tar.gz",mapname,cachename,basename);
       else
          (void)snprintf(tar_cmd,SSIZE,"tar cvf - %s %s 1> %s.tar 2> /dev/null",mapname,cachename,basename);

       ret = system(tar_cmd);

       if(WEXITSTATUS(ret) < 0)
       {  pups_set_errno(ENOEXEC);
          return(-1);
       }


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

_PUBLIC int cache_extract(const char *cache_archive)

{   int ret;

    char tar_cmd[SSIZE]           = "",
         basename[SSIZE]          = "",
         eff_cache_archive[SSIZE] = "";


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(cache_archive == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*------------------------------------------*/
    /* Set up extraction command taking note of */
    /* archive file type                        */
    /*------------------------------------------*/

    if(strin(cache_archive,".tar.xz") == TRUE)
    {  

       /*---------------------------------------*/
       /* Fully qualified compressed tar achive */
       /*---------------------------------------*/

       if(access(cache_archive,F_OK | R_OK | W_OK) == (-1))
       {  pups_set_errno(EINVAL);
          return(-1);
       }
       else
          (void)snprintf(tar_cmd,SSIZE,"xz -d <%s.tar.xz | tar xvf - > /dev/null",cache_archive);
    }
    else if(strin(cache_archive,".tar") == TRUE)
    {

       /*-----------------------------*/
       /* Fully qualified tar archive */
       /*-----------------------------*/

       if(access(cache_archive,F_OK | R_OK | W_OK) == (-1))
       {  pups_set_errno(EINVAL);
          return(-1);
       }
       else
          (void)snprintf(tar_cmd,SSIZE,"cat %s.tar | tar xvf - > /dev/null",cache_archive);
    }
    else
    {  

       /*---------------------------------------*/
       /* .map file or .mmc file - strip prefix */
       /*---------------------------------------*/

       if(strin(cache_archive,".map") == TRUE || strin(cache_archive,".mmc") == TRUE)
          stripname(cache_archive,basename);
       else if(strin(cache_archive,".") == TRUE)
       {  pups_set_errno(EINVAL);
          return(-1);
       }
       else
          (void)strlcpy(basename,cache_archive,SSIZE);

       (void)snprintf(eff_cache_archive,SSIZE,"%s.tar",basename);
       if(access(eff_cache_archive,F_OK | R_OK | W_OK) != (-1))


          /*----------------------*/
          /* Implicit tar archive */
          /*----------------------*/

          (void)snprintf(tar_cmd,SSIZE,"cat %s | tar xvf - > /dev/null",eff_cache_archive);
       else
       {  (void)snprintf(eff_cache_archive,SSIZE,"%s.tar.xz",cache_archive);
          if(access(eff_cache_archive,F_OK | R_OK | W_OK) != (-1))


             /*---------------------------------*/
             /* Implicit compressed tar archive */
             /*---------------------------------*/

             (void)snprintf(tar_cmd,SSIZE,"xz -d <%s | tar xvf - > /dev/null",eff_cache_archive);
          else
          {  pups_set_errno(EINVAL);
             return(-1);
          }
       }
    }


    /*---------------------------------------*/
    /* Extract cache and associated map file */
    /*---------------------------------------*/

    ret = system(tar_cmd);
    if(WEXITSTATUS(ret) < 0)
    {  pups_set_errno(ENOEXEC);
       return(-1);
    }

    pups_set_errno(OK);
    return(0);
}




/*--------------------------------*/
/* Read cache mapping information */
/*--------------------------------*/

_PUBLIC int cache_read_mapinfo(const char *map_name, const unsigned int c_index)

{   int i,
        fdes = (-1);

    char eff_map_name[SSIZE] = "";

    if(map_name == (const char *)NULL || c_index > MAX_CACHES)
    {  pups_set_errno(EINVAL);
       goto error_exit;
    }


    /*--------------------------*/
    /* Can specify full mapname */
    /* or basename              */
    /*--------------------------*/

    (void)strlcpy(eff_map_name,map_name,SSIZE);
    if((fdes = open(eff_map_name,O_RDONLY)) == (-1))
    {  (void)snprintf(eff_map_name,SSIZE,"%s.map",map_name);

       if((fdes = open(eff_map_name,O_RDONLY)) == (-1))
       {  pups_set_errno(EPERM);
          return(-1);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    if(read(fdes,cache[c_index].name,SSIZE) == (-1))
    {  (void)close(fdes);
       pups_set_errno(EIO);
       goto error_exit;
    }

    if(read(fdes,cache[c_index].mmap_name,SSIZE) == (-1))
    {  (void)close(fdes);
       pups_set_errno(EIO);
       goto error_exit;
    }

    if(read(fdes,cache[c_index].march,SSIZE) == (-1))
    {  (void)close(fdes);
       pups_set_errno(EIO);
       goto error_exit;
    }

    if(read(fdes,&cache[c_index].mmap,sizeof(_BOOLEAN)) == (-1))
    {  (void)close(fdes);
       pups_set_errno(EIO);
       goto error_exit;
    }

    if(read(fdes,&cache[c_index].n_blocks,sizeof(int)) == (-1))
    {  (void)close(fdes);
       pups_set_errno(EIO);
       goto error_exit;
    }

    if(read(fdes,&cache[c_index].n_objects,sizeof(int)) == (-1))
    {  (void)close(fdes);
       pups_set_errno(EIO);
       goto error_exit;
    }

    if(read(fdes,&cache[c_index].cache_size,sizeof(unsigned long int)) == (-1))
    {  (void)close(fdes);
       pups_set_errno(EIO);
       goto error_exit;
    }

    if(read(fdes,&cache[c_index].block_size,sizeof(unsigned long int)) == (-1))
    {  (void)close(fdes);
       pups_set_errno(EIO);
       goto error_exit;
    }


    /*-----------------------------------------*/
    /* Read offsets of objects in cache blocks */
    /*-----------------------------------------*/

    for(i=0; i<cache[c_index].n_objects; ++i)
    {   if(read(fdes,&cache[c_index].object_offset[i],sizeof(unsigned long int)) == (-1))
        {  (void)close(fdes);
           pups_set_errno(EIO);
           goto error_exit;
        }
    }


    /*---------------------------------------*/
    /* Read sizes of objects in cache blocks */
    /*---------------------------------------*/

    for(i=0; i<cache[c_index].n_objects; ++i)
    {   if(read(fdes,&cache[c_index].object_size[i],sizeof(unsigned long int)) == (-1))
        {  (void)close(fdes);
           pups_set_errno(EIO);
           goto error_exit;
        }
    }


    /*--------------------------------------*/
    /* Allocate space for cache block flags */
    /*--------------------------------------*/

    cache[c_index].flags = (_BYTE *)pups_calloc(cache[c_index].n_blocks,sizeof(_BYTE));


    /*------------------------*/
    /* Read cache block flags */
    /*------------------------*/

    for(i=0; i<cache[c_index].n_blocks; ++i)
    {   if(read(fdes,&cache[c_index].flags[i],sizeof(_BYTE)) == (-1))
        {  (void)close(fdes);
           pups_set_errno(EIO);
           goto error_exit;
        }
    }


    (void)close(fdes);
    pups_set_errno(OK);

    return(0);

error_exit:

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    return(-1);
}




/*-------------------------*/
/* Add data to cache block */
/*-------------------------*/

                                                                   /*-------------------------------------------------*/
_PUBLIC int cache_add_block(const void              *data,         /* Location of data to cache                       */
                            const unsigned long int size,          /* Size of data (in bytes)                         */
                            const unsigned int      flags,         /* Access flags                                    */
                            const unsigned int      object_index,  /* Object within cache block where data is written */
                            const unsigned int      cache_block,   /* Cache block where data is written               */
                            const unsigned int      c_index)       /* Cache index                                     */
                                                                   /*-------------------------------------------------*/

{   int  i;
    void *cache_ptr = (void *)NULL;

    if(c_index > MAX_CACHES || data == (const void *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    if(flags & LOCK_CACHE)
       (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    if(cache[c_index].block_size < size)
    {

       #ifdef PTHREAD_SUPPORT
       (void)pupsthread_mutex_unlock(&cache_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EINVAL);
       return(-1);
    }


    /*----------------------------------------------------*/
    /* Are we inserting data into a specific cache block? */
    /*----------------------------------------------------*/

    if(cache_block != ANY_BLOCK)
    {

       /*-------------------------------*/
       /* Get pointer to cache block we */
       /* writing data to               */
       /*-------------------------------*/

       if((cache_ptr = cache_access_object(cache_block,object_index,c_index)) == (void *)NULL)
       {

          #ifdef PTHREAD_SUPPORT
          (void)pupsthread_mutex_unlock(&cache_mutex);
          #endif /* PTHREAD_SUPPORT */

          return(-1);
       }

       /*--------------------*/
       /* Copy data to cache */
       /*--------------------*/

       (void)bcopy(data,cache_ptr,size);

       if(flags & UNLOCK_CACHE)
       {  cache[c_index].flags[cache_block] |= BLOCK_USED;

          #ifdef PTHREAD_SUPPORT
          (void)pupsthread_mutex_unlock(&cache_mutex);
          #endif /* PTHREAD_SUPPORT */

       }

#ifdef DEBUG
(void)fprintf(stderr,"CACHE %d, BLOCK %d, OBJECT %d\n",c_index,cache_block,object_index);
(void)fflush(stderr);
#endif /* DEBUG */

       pups_set_errno(OK);
       return(0);
    } 


    /*--------------------------------------------------------*/
    /* Do we have an unused block in the cache we can re-use? */
    /*--------------------------------------------------------*/

    for(i=0; i<cache[c_index].n_blocks; ++i)
    {  if(! cache[c_index].flags[i] & BLOCK_USED)
       {

          /*-------------------------------*/
          /* Get pointer to cache block we */
          /* writing data to               */
          /*-------------------------------*/

          if((cache_ptr = cache_access_object(i,object_index,c_index)) == (void *)NULL)
          {

              #ifdef PTHREAD_SUPPORT
              (void)pupsthread_mutex_unlock(&cache_mutex);
              #endif /* PTHREAD_SUPPORT */

              pups_set_errno(EACCES);
              return(-1);
          }


          /*--------------------*/
          /* Copy data to cache */
          /*--------------------*/
 
          (void)bcopy(data,cache_ptr,size);

          if(flags & UNLOCK_CACHE)
          {  cache[c_index].flags[i] |= BLOCK_USED;

             #ifdef PTHREAD_SUPPORT
             (void)pupsthread_mutex_unlock(&cache_mutex);
             #endif /* PTHREAD_SUPPORT */

          }

#ifdef DEBUG
(void)fprintf(stderr,"ANY CACHE %d, BLOCK %d, OBJECT %d\n",c_index,cache_block,object_index);
(void)fflush(stderr);
#endif /* DEBUG */

          pups_set_errno(OK);
          return(i);
       }
    }


    /*--------------------------------------------------*/
    /* If not it an error (for now). We may dynamically */
    /* extend this datastructure later                  */
    /*--------------------------------------------------*/ 

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ERANGE);
    return(-1);
    
}



/*------------------------------*/
/* Delete block data from cache */
/*------------------------------*/

_PUBLIC _BOOLEAN cache_delete_block(const unsigned int cache_block, const unsigned int c_index)

{

    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(c_index > MAX_CACHES)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    if(cache[c_index].flags[cache_block] & BLOCK_USED)
    {  cache[c_index].flags[cache_block] ^= BLOCK_USED; 

       #ifdef PTHREAD_SUPPORT
       (void)pupsthread_mutex_unlock(&cache_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(OK);
       return(TRUE);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(FALSE);
}




/*-------------*/
/* Clear cache */
/*-------------*/

_PUBLIC int cache_clear(const unsigned int c_index)

{   int i;


    /*---------------*/
    /* Sanity check  */
    /*---------------*/

    if(c_index > MAX_CACHES)
    {  pups_set_errno(ERANGE);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    for(i=0; i<cache[c_index].n_blocks; ++i)
    {  

       /*------------------------*/
       /* Clear block usage flag */
       /*------------------------*/

       if(cache[c_index].flags[i] & BLOCK_USED)
          cache[c_index].flags[i] ^= BLOCK_USED;


       /*-------------------------*/
       /* Clear block access flag */
       /*-------------------------*/

       if(cache[c_index].flags[i] & BLOCK_ACCESS)
          cache[c_index].flags[i] ^= BLOCK_ACCESS;
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*---------------------------------*/
/* Make cache block as accessible  */
/*---------------------------------*/

_PUBLIC _BOOLEAN cache_block_set_access(const _BOOLEAN access, const unsigned int cache_block, const unsigned int c_index)

{

    /*------------------*/
    /* Sanity check */
    /*------------------*/

    if(c_index > MAX_CACHES || (access != FALSE && access != TRUE))
    {  pups_set_errno(ERANGE);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    if(access == FALSE)
    {  if(cache[c_index].flags[cache_block] & BLOCK_ACCESS)
       {  cache[c_index].flags[cache_block] ^= BLOCK_ACCESS;

          #ifdef PTHREAD_SUPPORT
          (void)pupsthread_mutex_unlock(&cache_mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(TRUE);
       }
       else
       {

          #ifdef PTHREAD_SUPPORT
          (void)pupsthread_mutex_unlock(&cache_mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(FALSE);
       }
    }
    else
       cache[c_index].flags[cache_block] |= BLOCK_ACCESS;
       
    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(TRUE);
}




/*----------------------------------*/
/* Test if cach block is accessible */
/*----------------------------------*/

_PUBLIC _BOOLEAN cache_block_test_access(const unsigned int cache_block, const unsigned int c_index)

{   _BOOLEAN ret = FALSE;


    /*--------------*/
    /* Sanity check */
    /*--------------*/
    
    if(c_index > MAX_CACHES)
    {  pups_set_errno(ERANGE);
       return(FALSE);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */


    /*----------------------------*/
    /* test to see if cache block */
    /* is accessible              */
    /*----------------------------*/

    if(cache[c_index].flags[cache_block] & BLOCK_ACCESS)
       ret = TRUE;

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(ret);
}




/*------------------------*/
/* Set cache access state */
/*------------------------*/

_PUBLIC int cache_set_access(const _BOOLEAN access, const unsigned int c_index)

{   int i;


    /*---------------*/
    /* Sanity check  */
    /*---------------*/

    if(c_index > MAX_CACHES || (access != FALSE && access != TRUE))
    {  pups_set_errno(ERANGE);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    for(i=0; i<cache[c_index].n_blocks; ++i)
    {  if(access == TRUE)
          cache[c_index].flags[i] |= BLOCK_ACCESS;
       else
       {  if(cache[c_index].flags[i] & BLOCK_ACCESS)
             cache[c_index].flags[i] ^= BLOCK_ACCESS;
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_unlock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}



/*--------------*/
/* Resize cache */
/*--------------*/

_PUBLIC int cache_resize(const unsigned int n_blocks, const unsigned int c_index)

{   int               fd         = (-1);
    unsigned long int new_size;
    void              *cache_ptr = (void *)NULL;
 

    /*--------------*/
    /* Sanity check */
    /*--------------*/
    
    if(c_index > MAX_CACHES)
    {  pups_set_errno(ERANGE);
       return(FALSE);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pupsthread_mutex_lock(&cache_mutex);
    #endif /* PTHREAD_SUPPORT */


    /*---------------------------------------*/
    /* If cache size hasn't changed we don't */
    /* have to do anything                   */
    /*---------------------------------------*/

    if(n_blocks == cache[c_index].n_blocks)
    {

       #ifdef PTHREAD_SUPPORT
       (void)pupsthread_mutex_unlock(&cache_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(OK);
       return(0);
    }


    /*-------------------------------------------*/
    /* Cache has shrunk - all we need to do is   */
    /* remap it and then adjust number of blocks */
    /*-------------------------------------------*/

    else if(n_blocks < cache[c_index].n_blocks)
    {  new_size = n_blocks*cache[c_index].block_size;


       if(strcmp(cache[c_index].mmap_name,"") == 0)
       {  
          /*---------------------------------------------------*/
          /* Cache is on local heap so extend it using realloc */
          /*---------------------------------------------------*/

          cache_ptr = pups_realloc(cache[c_index].cache_memory,new_size);
       }
       else
       {

          /*--------------------------------------------------*/
          /* MREMAP_FIXED makes sure that the resized mapping */
          /* starts at the same place in the process addreess */
          /* space as the initial mapping                     */
          /*--------------------------------------------------*/

          fd = open(cache[c_index].mmap_name,O_RDWR);
          (void)ftruncate(fd,new_size);
          (void)posix_fallocate(fd,0,new_size);
          (void)close(fd);

          cache_ptr = mremap(cache[c_index].cache_memory,
                             cache[c_index].cache_size,
                             new_size,
                             MREMAP_FIXED,
                             cache[c_index].cache_memory);
       }

       cache[c_index].flags        = (_BYTE *)pups_realloc(cache[c_index].flags,n_blocks*sizeof(_BYTE));
       cache[c_index].cachemap     = (void *) pups_realloc(cache[c_index].cachemap,n_blocks*sizeof(cache_mtype)); 
       cache[c_index].n_blocks     = n_blocks;
       cache[c_index].cache_size   = new_size;

       #ifdef PTHREAD_SUPPORT
       (void)pupsthread_mutex_unlock(&cache_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(OK);
       return(0);         
    }


    /*---------------------------------------------*/
    /* We are extending the mapping so we need to  */
    /* remap, adjust the number of blocks and then */
    /* initialise the new blocks created           */
    /*---------------------------------------------*/

    new_size = n_blocks*cache[c_index].block_size;

    if(strcmp(cache[c_index].mmap_name,"") == 0)
    {

       /*---------------------------------------------------*/
       /* Cache is on local heap so extend it using realloc */
       /*---------------------------------------------------*/

       cache_ptr = pups_realloc(cache[c_index].cache_memory,new_size);
    }
    else
    {

        /*--------------------------------------------------*/
        /* MREMAP_FIXED makes sure that the resized mapping */
        /* starts at the same place in the process address  */
        /* space as the initial mapping                     */
        /*--------------------------------------------------*/

        fd = open(cache[c_index].mmap_name,O_RDWR);
        (void)posix_fallocate(fd,0,new_size);
        (void)close(fd);

        cache_ptr = mremap(cache[c_index].cache_memory,
                           cache[c_index].cache_size,
                           new_size,
                           MREMAP_MAYMOVE);

     }

     if(cache_ptr == (void *)NULL)
     {

        #ifdef PTHREAD_SUPPORT
        (void)pupsthread_mutex_unlock(&cache_mutex);
        #endif /* PTHREAD_SUPPORT */

        pups_set_errno(ENOMEM);
        return(-1);
     }
     else
     {  int i;

        cache[c_index].cache_memory = cache_ptr;
        cache[c_index].flags        = (_BYTE *)pups_realloc(cache[c_index].flags,n_blocks*sizeof(_BYTE));
        cache[c_index].cachemap     = pups_realloc(cache[c_index].cachemap,n_blocks*sizeof(cache_mtype)); 


        /*------------------------------*/
        /* Recompute offsets into cache */
        /*------------------------------*/

        for(i=0;  i<n_blocks; ++i)
        {  int j;

           for(j=0; j<cache[c_index].n_objects; ++j)                                                          /*----------------------------*/
           {  cache[c_index].cachemap[i].object_ptr[j] = (void *)(cache[c_index].object_offset[j]          +  /* Object offset within block */
                                                         (unsigned long int)i*cache[c_index].block_size    +  /* Block offset within cache  */
                                                         (unsigned long int)cache[c_index].cache_memory);     /* Base address of cache      */
           }                                                                                                  /*----------------------------*/

#ifdef DEBUG
(void)fprintf(stderr,"BLOCK %d (of %d)\n",i,n_blocks-1);
(void)fflush(stderr);
#endif /* DEBUG */


           /*------------------------*/
           /* Initialise extra flags */
           /*------------------------*/

           if(i > cache[c_index].n_blocks)
              cache[c_index].flags[i] = 0;
        }

        cache[c_index].n_blocks   = n_blocks;
        cache[c_index].cache_size = new_size;
     }

     #ifdef PTHREAD_SUPPORT
     (void)pupsthread_mutex_unlock(&cache_mutex);
     #endif /* PTHREAD_SUPPORT */

     pups_set_errno(OK);
     return(0);
}

