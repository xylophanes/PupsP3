/*---------------------------------------
    Purpose: Portable hash access library

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 3.00 
    Dated:   10th December 2024
    E-Mail:  mao@tumblingdice.co.uk
--------------------------------------*/

#include <me.h>
#include <utils.h>
#include <casino.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <bsd/bsd.h>


/*----------------------------------------*/
/* Do not expand _EXTERN for this library */
/*----------------------------------------*/

#undef   __NOT_LIB_SOURCE__
#include <hash.h>
#define  __NOT_LIB_SOURCE__



/*-------------------------------------------------*/
/* Slot and usage functions - used by slot manager */
/*-------------------------------------------------*/
/*---------------------*/
/* Slot usage function */
/*---------------------*/

_PRIVATE void hashlib_slot(int32_t level)
{   (void)fprintf(stderr,"lib hashlib %s: [ANSI C]\n",HASH_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(C) 1995-2024 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"PUPS/P3 hash storage library (gcc %s: built %s %s)\n\n",__VERSION__,__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");
    (void)fflush(stderr);
}




/*-----------------------------------------*/
/* Segment identification for hash library */
/*-----------------------------------------*/

#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT )() __attribute__ ((aligned(16))) = hashlib_slot;
#endif /* SLOT */




/*-----------------------------------------------*/
/* Private variables used by the hashing library */
/*-----------------------------------------------*/

#ifdef PTHREAD_SUPPORT
pthread_mutex_t hash_table_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif /* PTHREAD_SUPPORT */



/*-------------------------------------------------*/
/* Prototypes of functions private to this library */
/*-------------------------------------------------*/
/*---------------------------------*/
/* Routine to generate hashing key */
/*---------------------------------*/

_PROTOTYPE _PRIVATE int32_t hash_key(int32_t, hash_table_type *);




/*-------------------*/
/* Generate hash key */
/*-------------------*/

_PRIVATE  int32_t hash_key(int32_t h_index, hash_table_type *hash_table)

{    int32_t w;


    /*-------------------------------------------------------------*/
    /* Seed random number generator so that random number produced */
    /* is a function of the index                                  */
    /*-------------------------------------------------------------*/

    r_init = (long)(-h_index);


    /*-------------------*/
    /* Generate hash key */
    /*-------------------*/

    w = (int)(ran1()*(FTYPE)(hash_table->size - 1));
    return(w);
}



 
/*----------------------------*/
/* Create a hash table object */
/*----------------------------*/

_PUBLIC hash_table_type *hash_table_create(const uint32_t    size, const char *name)

{   uint32_t        i;
    hash_table_type *hash_table = (hash_table_type *)NULL;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[hash_table_create] attempt by non root thread to perform PUPS/P3 hash operation");

    if(size < 0 || name == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return((hash_table_type *)NULL);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&hash_table_mutex);
    #endif /* PTHREAD_SUPPORT */

    if((hash_table = (hash_table_type *)pups_malloc(sizeof(hash_table_type))) == (hash_table_type *)NULL)
       pups_error("[hash_table_create] cannot allocate memory [hash_table]");

    if((hash_table->hashentry = (hash_type *)pups_calloc(size,sizeof(hash_type))) == (hash_type *)NULL)
       pups_error("[hash_table_create] cannot allocate memory [hash_entry array] ");
     
    hash_table->size  = size;
    (void)strlcpy(hash_table->name,name,SSIZE);


    /*------------------------------*/
    /* Initialise the object status */
    /*------------------------------*/

    for(i=0; i<size; ++i)
    {   hash_table->hashentry[i].size   = 0;
        hash_table->hashentry[i].used   = 0;
        hash_table->hashentry[i].index  = (int   *)NULL;
        hash_table->hashentry[i].object = (void **)NULL;
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&hash_table_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(hash_table);
}




/*-----------------------------*/
/* Destroy a hash table object */
/*-----------------------------*/

_PUBLIC hash_table_type *hash_table_destroy(hash_table_type *hash_table)

{   uint32_t i;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[hash_table_destroy] attempt by non root thread to perform PUPS/P3 hash operationy");

    if(hash_table == (hash_table_type *)NULL)
    {  pups_set_errno(EINVAL);
       return((hash_table_type *)NULL);
    }


    /*------------------------------------------------*/
    /* Free all memory associated with the hash table */
    /*------------------------------------------------*/

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&hash_table_mutex);
    #endif /* PTHREAD_SUPPORT */

    for(i=0; i<hash_table->size; ++i)
    {  (void)pups_free((void *)hash_table->hashentry[i].index);
       (void)pups_free((void *)hash_table->hashentry[i].object);
    }

    (void)pups_free((void *)hash_table);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&hash_table_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return((hash_table_type *)NULL);
}




/*-----------------------------------*/
/* Return object at a given location */
/*-----------------------------------*/

_PUBLIC int32_t hash_get_object(const uint32_t h_index, void *object, const char *object_type, hash_table_type *hash_table)

{   uint32_t  i;
    int32_t   hash_index;


    /*------------------*/
    /* Check parameters */
    /*------------------*/

    if(h_index       <   0                  || 
       object      == (void *)NULL          ||
       object_type == (char *)NULL          ||
       hash_table  == (hash_table_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&hash_table_mutex);
    #endif /* PTHREAD_SUPPORT */


    /*------------------------------------------*/
    /* Get index  int32_to the tables of hashed data */
    /*------------------------------------------*/

    hash_index = hash_key(h_index,hash_table);

    #ifdef HASHLIB_DEBUG
    (void)fprintf(stderr,"Hash key %d (from index %d)\n",hash_index,h_index);
    (void)fflush(stderr);
    #endif /* HASHLIB_DEBUG */


    /*---------------------------------------------------------*/
    /* Follow hash chain until we find index which corresponds */
    /* the index we are looking for. Note that we have strong  */
    /* type checking: the object types MUST match              */
    /*---------------------------------------------------------*/

    for(i=0; i<hash_table->hashentry[hash_index].size; ++i)
    {   if(hash_table->hashentry[hash_index].index[i] == h_index && strcmp(hash_table->hashentry[hash_index].object_type[i],object_type) == 0)
        {  (void)memcpy(object,hash_table->hashentry[hash_index].object[i],hash_table->hashentry[hash_index].object_size[i]); 

           #ifdef HASHLIB_DEBUG
           (void)fprintf(stderr,"Hash key %d: extract OK\n",hash_index);
           (void)fflush(stderr);
           #endif /* HASHLIB_DEBUG */

           #ifdef PTHREAD_SUPPORT
           (void)pthread_mutex_unlock(&hash_table_mutex);
           #endif /* PTHREAD_SUPPORT */

           pups_set_errno(OK);
           return(0);
        }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&hash_table_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return(-1);
}




/*--------------------------*/
/* Put object in hash table */
/*--------------------------*/

_PUBLIC int32_t hash_put_object(const uint32_t h_index, const void *object, const char *object_type, const size_t object_size, hash_table_type *hash_table)       

{   uint32_t i,
             hash_index,
             chain_index;


    /*------------------*/
    /* Check parameters */
    /*------------------*/

    if(h_index    <  0                    ||
      object      == (void *)NULL         ||
      object_type == (char *)NULL         ||
      object_size < 0                     ||
      hash_table == (hash_table_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&hash_table_mutex);
    #endif /* PTHREAD_SUPPORT */


    /*------------------------*/
    /* Get hash key for index */
    /*------------------------*/

    hash_index = hash_key(h_index,hash_table);


    /*------------------------------------------------*/
    /* Try and find a free slot at insertion location */
    /*------------------------------------------------*/

    for(i=0; i<hash_table->hashentry[hash_index].size; ++i)
    {  if(hash_table->hashentry[hash_index].index[i] == (-1)) 
       {   

           /*-----------------------*/
           /* Record type of object */
           /*-----------------------*/

           (void)strlcpy(hash_table->hashentry[hash_index].object_type[i],object_type,SSIZE);


           /*------------------------------------------------------------------------*/
           /* We may need to change amount of memory dynamically allocated to object */          
           /*------------------------------------------------------------------------*/

           if(hash_table->hashentry[hash_index].object_size[i] != object_size)
           {  hash_table->hashentry[hash_index].object[i]      = (void *)realloc((void *)hash_table->hashentry[hash_index].object[i],object_size); 
              hash_table->hashentry[hash_index].object_size[i] = object_size;
           }


           /*------------------------------------*/
           /* Store the object in the hash table */
           /*------------------------------------*/

           (void)memcpy(hash_table->hashentry[hash_index].object[i],object,object_size);
           hash_table->hashentry[hash_index].index[i] = h_index;

           ++hash_table->hashentry[hash_index].used;

           #ifdef PTHREAD_SUPPORT
           (void)pthread_mutex_unlock(&hash_table_mutex);
           #endif /* PTHREAD_SUPPORT */

           #ifdef HASHLIB_DEBUG
           (void)fprintf(stderr,"Used %d: Insert at hash %d, chain %d\n",h_index,hash_index,i);
           (void)fflush(stderr);
           #endif /* HASHLIB_DEBUG */

           pups_set_errno(OK);
           return(0);
       }
    } 


    /*----------------------------------------------------*/
    /* Insert object  int32_to hash table extending hash chain */
    /* for the insertion location                         */
    /*----------------------------------------------------*/

    chain_index = hash_table->hashentry[hash_index].used;

    if(hash_table->hashentry[hash_index].size == hash_table->hashentry[hash_index].used)
    {  hash_table->hashentry[hash_index].size += ALLOC_QUANTUM;

       hash_table->hashentry[hash_index].object_type  = (char **)pups_realloc((void *)hash_table->hashentry[hash_index].object_type,
                                                                          hash_table->hashentry[hash_index].size*sizeof(char *));
       if(hash_table->hashentry[hash_index].object_type == (char **)NULL)
          pups_error("[hash_put_object] failed to extend hash chain (cannot reallocate memory [extend object_type array])");

       hash_table->hashentry[hash_index].object = (void **)pups_realloc((void *)hash_table->hashentry[hash_index].object,
                                                                    hash_table->hashentry[hash_index].size*sizeof(void *));

       if(hash_table->hashentry[hash_index].object == (void **)NULL)
          pups_error("[hash_put_object] failed to extend hash chain (cannot reallocate memory [extend object array])");

       hash_table->hashentry[hash_index].index = (int32_t *)  pups_realloc((void *)hash_table->hashentry[hash_index].index,
                                                                    hash_table->hashentry[hash_index].size*sizeof(int32_t));

       if(hash_table->hashentry[hash_index].index == (int32_t *)NULL)
          pups_error("[hash_put_object] failed to extend hash chain (cannot allocate memory [extend index array])");

       hash_table->hashentry[hash_index].object_size  = (size_t *)  pups_realloc((void *)hash_table->hashentry[hash_index].object_size,
                                                                                hash_table->hashentry[hash_index].size*sizeof(int32_t));
       if(hash_table->hashentry[hash_index].object_size == (size_t *)NULL)
          pups_error("[hash_put_object] failed to extend hash chain (cannot reallocate memory [extend object_size array])");
    }

    if((hash_table->hashentry[hash_index].object_type[chain_index] = (void *)pups_malloc(SSIZE)) == (void *)NULL)
       pups_error("[hash_put_object] failed to extend hash chain (cannot allocate memory [object type])");
    else 
       (void)strlcpy(hash_table->hashentry[hash_index].object_type[chain_index],object_type,SSIZE);

    if((hash_table->hashentry[hash_index].object[chain_index] = (void *)pups_malloc(object_size)) ==(void *)NULL)
       pups_error("[hash_put_object] failed to extend hash chain (cannot allocate memory [object])");
    else 
       (void)memcpy(hash_table->hashentry[hash_index].object[chain_index],object,object_size);

    hash_table->hashentry[hash_index].index[chain_index]       = h_index;
    hash_table->hashentry[hash_index].object_size[chain_index] = object_size;

    ++hash_table->hashentry[hash_index].used;

    #ifdef HASHLIB_DEBUG
    (void)fprintf(stderr,"%d: Insert at hash %d, chain %d\n",index,hash_index,chain_index);
    (void)fflush(stderr);
    #endif /* HASHLIB_DEBUG */

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&hash_table_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*------------------------------------------*/
/* Routine to delete object from hash table */
/*------------------------------------------*/

_PUBLIC int32_t hash_delete_object(const uint32_t h_index, hash_table_type *hash_table)

{   uint32_t i,
             hash_index;


    /*------------------*/
    /* Check parameters */
    /*------------------*/

    if(h_index    <  0                     ||
       hash_table == (hash_table_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&hash_table_mutex);
    #endif /* PTHREAD_SUPPORT */


    /*------------------------------------------*/ 
    /* Get index into the tables of hashed data */
    /*------------------------------------------*/ 

    hash_index = hash_key(h_index,hash_table);

    for(i=0; i<hash_table->hashentry[hash_index].size; ++i)
    {  if(hash_table->hashentry[hash_index].index[i] == h_index)
       {  hash_table->hashentry[hash_index].index[i]       = (-1);
          hash_table->hashentry[hash_index].object_size[i] = (-1);
          (void)strlcpy(hash_table->hashentry[hash_index].object_type[i],"",SSIZE);

          --hash_table->hashentry[hash_index].used;

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&hash_table_mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(0);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&hash_table_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return(-1);
}




/*----------------------*/
/* Show hash statistics */
/*----------------------*/

_PUBLIC int32_t hash_show_stats(const FILE *stream, const _BOOLEAN full_stats, hash_table_type *hash_table)

{   uint32_t i,
             j,
             cnt        = 0,
             object_cnt = 0,
             chain_sum  = 0;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[hash_show_stats] attempt by non root thread to perform PUPS/P3 hash operation");

    if(stream == (FILE *)NULL               ||
        hash_table == (hash_table_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&hash_table_mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)fprintf(stream,"\n    Hash table \"%s\"\n\n",hash_table->name);

    for(i=0; i<hash_table->size; ++i)
    {  if(hash_table->hashentry[i].used > 0)
       {  (void)fprintf(stream,"    %04d: chain length: %04d, links free: %04d",i,
                                                    hash_table->hashentry[i].size,
                                                    hash_table->hashentry[i].size - hash_table->hashentry[i].used);

          object_cnt = 0;
          for(j=1; j<hash_table->hashentry[i].size; ++j)
          {  if(hash_table->hashentry[i].object[j] != hash_table->hashentry[i].object[j-1])
                ++object_cnt;
          }

          if(object_cnt > 1)
             (void)fprintf(stream," (%04d objects in chain)\n",object_cnt);
          else
             (void)fprintf(stream," (%04d object in chain)\n",1);
          (void)fflush(stream);

          if(full_stats == TRUE && hash_table->hashentry[i].used > 0)
          {  (void)fprintf(stream,"    types: "); 
             for(j=0; j<hash_table->hashentry[i].size; ++j)
             {  if(hash_table->hashentry[i].object_type[j] != (char *)NULL)
                {  if(j != 0 && j % 5 == 0)
                      (void)fprintf(stream," %s\n    ",hash_table->hashentry[i].object_type[j]);
                   else
                      (void)fprintf(stream," %s",hash_table->hashentry[i].object_type[j]);
                }
             }

             (void)fprintf(stream,"\n");
             (void)fflush(stream); 
          }

          chain_sum += object_cnt;
          ++cnt;
       }
    }

    if(cnt > 0)
       (void)fprintf(stream,"\n\n    %04d hash table entries (%04d slots free)  [utilisation %4.2F percent]\n\n",
                                                                                                             cnt,
                                                                                                hash_table->size,
                                                                  100.0*(FTYPE)chain_sum/(FTYPE)hash_table->size);
    else
       (void)fprintf(stream,"    hash table is empty (%04d slots free)\n\n",hash_table->size); 
    (void)fflush(stream);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&hash_table_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
}
