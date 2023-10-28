/*------------------------------------------------------------------------------
    Purpose: Meta virtual page demand dynamic object support library.

    Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

    Version: 2.02 
    Dated:   4th January 2023
    E-mail:  mao@tumblingdice.co..uk
------------------------------------------------------------------------------*/

#include <me.h>
#include <utils.h>

#undef  __NOT_LIB_SOURCE__
#include <mvm.h>
#define __NOT_LIB_SOURCE__

#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


/*----------------------------------------------*/
/* Get application information for slot manager */
/*----------------------------------------------*/
/*---------------------------*/
/* Slot information function */
/*---------------------------*/

_PRIVATE void mvm_slot(int level)
{   (void)fprintf(stderr,"lib mvmlib %s: [ANSI C]\n",MVMLIB_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(c) 1995-2023 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"Meta virtual paged object library (built %s %s)\n",__TIME__,__DATE__);
       (void)fflush(stderr);
    }

    (void)fprintf(stderr,"\n");
    (void)fflush(stderr);
}


/*----------------------------------------*/
/* Segment identification for mvm library */
/*----------------------------------------*/

#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT ) = mvm_slot;
#endif /* SLOT */




/*--------------------------------------------*/
/* Variables which are private to this module */
/*--------------------------------------------*/

_PRIVATE mvm_type *mvmtab[MVM_TABLE_SIZE] = { (mvm_type *)NULL };





/*------------------------------------------------------------------------------
    Functions which are private to this module ...
------------------------------------------------------------------------------*/

// Lock list updater
_PROTOTYPE _PRIVATE int mvm_update_lock_map(int, mvm_type *);

// Create a usage keyed index table for a meta virtual memory structure
_PROTOTYPE _PRIVATE int mvm_index_phys_pages(mvm_type *);

// Read a page from backing store
_PROTOTYPE _PRIVATE int mvm_read_page_from_backing_store(mvm_type *, int);

// Read a page from backing store
_PROTOTYPE _PRIVATE int mvm_write_page_to_backing_store(mvm_type *, int);




/*------------------------------------------------------------------------------
    Get a line from image file on disk - this is a modeified version 
    of the algorithm used in the Alvey MMI-137 version of the cached
    stereomatcher code ...
------------------------------------------------------------------------------*/


_PUBLIC int mvm_page(int      v_page,   // Page to cache
	             mvm_type   *mvm)   // Meta virtual memory mapper
		   
{   int i,
        phys_page;

    unsigned long int bytes_read,
                      v_page_offset;

    if(v_page < 0 || mvm == (mvm_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    /*------------------------------------------------------------------*/
    /* Do we already have the line we need "paged" into the image cache */
    /* buffer                                                           */
    /*------------------------------------------------------------------*/

    if((phys_page = mvm->v_page_map[v_page]) != PAGE_NOT_USED)
    {  ++mvm->page_status[phys_page].v_page_usage;
       mvm_update_lock_map(phys_page,mvm);

       pups_set_errno(OK);
       return;
    }


    /*------------------------------------------------------------------------*/
    /* We have not got the required line "paged" into the image cache buffer. */
    /* There are two possibilities here (a) There may still be space to       */
    /* "page" the line directly into the cache buffer or (b) we may need      */
    /* to remove data from the cache buffer to make space for the new item.   */
    /*                                                                        */
    /* If the second case pertains then we will expell that item of date      */
    /* which has currently been accessed the least - before this is done      */
    /* we will have to sort the list of pages ...                             */
    /* -----------------------------------------------------------------------*/

    if(mvm->page_slots == mvm->max_page_slots)
    {  if(mvm->sched_policy == MVM_AGED_AND_ORDERED)
       {  if(mvm->aged_and_ordered == FALSE)
             mvm_age_and_order(mvm);
       }


       /*----------------------------------------------------*/
       /* Find page which is not currently locked and in use */
       /*----------------------------------------------------*/

       if(mvm->clock_ptr >= mvm->max_page_slots)
          mvm->clock_ptr = 0;

       while(mvm->ulist_ptr < mvm->max_page_slots                                             &&
             mvm->page_status[mvm->usage_map[mvm->clock_ptr]].locked == mvm->current_lock_state)
       {     ++mvm->clock_ptr;
             ++mvm->ulist_ptr;

             if(mvm->clock_ptr >= mvm->max_page_slots)
                mvm->clock_ptr = 0;
       }

       if(mvm->ulist_ptr == mvm->max_page_slots)
       {

          #ifdef DEBUG
          if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@%s:%s): all pages locked and in use for mvm %s\n",
                                        date,appl_name,appl_pid,appl_host,appl_owner,mvm->name);

             (void)fprintf(stderr,"%s %s (%d@%s:%s): Increasing number of physical pages\n",
                                     date,appl_name,appl_pid,appl_host,appl_owner,mvm->name);

             (void)fflush(stderr);
          }
          #endif /* DEBUG */

          ++mvm->max_page_slots;
          goto allocate_more_resources;
       }

       phys_page = mvm->usage_map[mvm->clock_ptr];


       /*--------------------------------------------------------------------*/
       /* The buffer is fully allocated - throw out the "page" of image data */
       /*  which is currently the least accessed and use the data buffer     */
       /* currently allocated to it for the incoming line of data which      */
       /* actually needs it                                                  */
       /*--------------------------------------------------------------------*/

       #ifdef DEBUG
       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): page %d in mvm %s swapped for %d\n",
                                                                                 date,
                                                                            appl_name,
                                                                             appl_pid,
                                                                            appl_host,
                                                                           appl_owner,
                                                   mvm->page_status[phys_page].v_page,
                                                                            mvm->name,
                                                                               v_page);
          (void)fflush(stderr);
       }
       #endif /* DEBUG */

        if(mvm->r_w_state == MVM_READ_WRITE)
           mvm_write_page_to_backing_store(mvm,mvm->page_status[phys_page].v_page);

        mvm->vmem[mvm->page_status[phys_page].v_page]       = (void *)NULL;
        mvm->v_page_map[mvm->page_status[phys_page].v_page] = PAGE_NOT_USED;
        mvm->vmem[v_page]                                   = mvm->page_status[phys_page].v_page_location;
        mvm->v_page_map[v_page]                             = phys_page;
        mvm->page_status[phys_page].v_page                  = v_page;
        mvm->page_status[phys_page].v_page_usage            = 1;
        mvm_update_lock_map(phys_page,mvm);

        if(mvm->clock_ptr < mvm->max_page_slots)
           ++mvm->clock_ptr;

        goto page_in;
    }


    /*---------------------------------------------------------------------*/
    /* Allocate more memory so we can read the line into the virtual image */
    /* buffer                                                              */
    /*---------------------------------------------------------------------*/

allocate_more_resources:

    phys_page           = mvm->page_slots;
    mvm->vmem[v_page]   = (_BYTE *)pups_malloc(mvm->v_page_size*sizeof(_BYTE));

    if(phys_page == 0)
    {  mvm->page_status = (page_status_type *)pups_malloc(sizeof(page_status_type));
       mvm->usage_map   = (int *)             pups_malloc(sizeof(int));
    }
    else
    {  mvm->page_status = (page_status_type *)pups_realloc((void *)mvm->page_status,
                                                       (phys_page + 1)*sizeof(page_status_type));

       mvm->usage_map   = (int *)             pups_realloc((void *)mvm->usage_map,
                                                       (phys_page + 1)*sizeof(int));
    }

    #ifdef DEBUG
    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): virtual data cache extended (%d resident pages) in mvm %s\n",
                                                                                                       date,
                                                                                                  appl_name,
                                                                                                   appl_pid,
                                                                                                  appl_host,
                                                                                                 appl_owner,
                                                                                                  phys_page,
                                                                                                  mvm->name);
       (void)fflush(stderr);
    }
    #endif /* DEBUG */

    if(mvm->page_slots == mvm->max_page_slots && appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s: cache now full (%d resident pages) in mvm %s\n",
                                                                                         date,
                                                                                    appl_name,
                                                                                     appl_pid,
                                                                                    appl_host,
                                                                                   appl_owner,
                                                                          mvm->max_page_slots,
                                                                                    mvm->name);
       (void)fflush(stderr);
    }


    /*-------------------------------------------------------*/
    /* Adjust the cache buffer to reflect the new allocation */
    /*-------------------------------------------------------*/
    
    mvm->page_status[phys_page].v_page_location = mvm->vmem[v_page];
    mvm->page_status[phys_page].v_page          = v_page;
    mvm->page_status[phys_page].v_page_usage    = 1;
    mvm->v_page_map[v_page]                     = phys_page;
    mvm->usage_map[phys_page]                   = phys_page;
    mvm_update_lock_map(phys_page,mvm);
    ++mvm->page_slots;


page_in:

    /*------------------------------------------------------------------------*/
    /* Now we are acutally in a position to "page in" the page of memory from */
    /* backing store - note for caching to work the backing store MUST be     */
    /* located on a seekable device                                           */
    /*------------------------------------------------------------------------*/

    mvm_read_page_from_backing_store(mvm,v_page);

    pups_set_errno(OK);
    return(0);
}




/*--------------------------------------------------------------------------------
    Routine to age the pages in the mapper structure and sort them into to
    age order starting with the least used [youngest] page ...
--------------------------------------------------------------------------------*/

_PUBLIC int mvm_age_and_order(mvm_type *mvm)

{   int i;

    if(mvm == (mvm_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*--------------------------------------------------------------------------------
    /* Make sure that we do not try to age and order on subsequent calls to the
    /* pager until we have called mvm_init again ...
    /*--------------------------------------------------------------------------------*/

    mvm->aged_and_ordered = TRUE;


    /*----------------------------------------------------------------------------*/
    /* Sort the pages - at the end of the sort the least used pages will be       */
    /* at the top of the cache status usage index array - note we cannot actually */
    /* sort the pages as they must remain at fixed (and known) locations so we    */
    /* can memory map efficiently                                                 */
    /*----------------------------------------------------------------------------*/

    mvm_index_phys_pages(mvm);


    #ifdef AGE_MV_CACHE
    /*----------------------------------------------------------------------*/
    /* Then "age" the pages in the cache - decrement the usage count of all */
    /* cache pages by one - note if the first page has a usage count of 1   */
    /* there is no need to age the pages as it will be used anyway          */
    /*----------------------------------------------------------------------*/

   if(mvm->page_status[mvm->usage_map[1]].v_page_usage > 1)
   {  for(i=0; i<mvm->max_page_slots; ++i)
	 if(mvm->page_status[i].v_page_usage > 1)
	    --mvm->page_status[i].v_page_usage[i];
   }
   #endif /* AGE_MV_CACHE */

   pups_set_errno(OK);
   return(0);

}



/*--------------------------------------------------------------------------------
    Initialise an image cache ...
--------------------------------------------------------------------------------*/

_PUBLIC void **mvm_init(char             *name,  // Name of virtual memory 
                        _BOOLEAN   initialised,  // TRUE if swap initialised
                        int          r_w_state,  // Read write access state
                        int       sched_policy,  // Scheduling policy
                        int       v_page_slots,  // Pages in virtual memory
                        int     max_page_slots,  // Pages in physical memory
                        int                 fd,  // Backing store descriptor
                        long       v_page_size,  // Page size in virtual memory
                        mvm_type          *mvm)  // Virtual memory structure

{   int  i;
    char errstr[SSIZE] = "";

    if(mvm            ==  (mvm_type *)NULL    ||
       name           ==  (char *)NULL        ||
       v_page_slots   <   0                   ||
       max_page_slots <   0                   ||
       fd             <   0                   ||
       v_page_size    <   0                    )
    {  pups_set_errno(EINVAL);
       return((void **)NULL);
    }


    /*------------------------------------------*/
    /* Initialise meta virtual memory structure */
    /*------------------------------------------*/

    (void)strlcpy(mvm->name,name,SSIZE);
    mvm->v_page_slots       = v_page_slots;
    mvm->max_page_slots     = max_page_slots;
    mvm->page_slots         = 0;
    mvm->ulist_ptr          = 0;
    mvm->clock_ptr          = 0;
    mvm->fd                 = fd;
    mvm->v_page_base        = pups_lseek(fd,0,SEEK_CUR);
    mvm->v_page_size        = v_page_size;
    mvm->aged_and_ordered   = FALSE;
    mvm->initialised        = initialised;


    /*--------------------------------------*/
    /* Allocate the first set of item locks */
    /*--------------------------------------*/

    mvm->next_lock = 0;
    mvm->max_lock  = MVM_QUANTUM;
    mvm->lock_map  = (int *)pups_malloc(MVM_QUANTUM*sizeof(int));


    /*-----------------------*/
    /* Set scheduling policy */
    /*-----------------------*/

    mvm->sched_policy = sched_policy;


    /*-------------------------*/
    /* Read/write access state */
    /*-------------------------*/

    mvm->r_w_state  = r_w_state;


    /*--------------------------------------------------------------------------*/ 
    /* Allocate associated virtual memory map and associated page mapping array */
    /*--------------------------------------------------------------------------*/ 

    if((mvm->vmem = (void **)pups_malloc(mvm->v_page_slots*sizeof(void *))) == (void **)NULL)
    {   pups_set_errno(ENOMEM);
        return((void **)NULL);
    }

    if((mvm->v_page_map = (int *)pups_malloc(mvm->v_page_slots*sizeof(int))) == (int *)NULL)
    {   pups_set_errno(ENOMEM);
        return((void **)NULL);
    }

    for(i=0; i<mvm->v_page_slots; ++i)
       mvm->v_page_map[i] = PAGE_NOT_USED;


    /*--------------------------------*/
    /* Add MVM to table of open MVM's */
    /*--------------------------------*/

    for(i=0; i<MVM_TABLE_SIZE; ++i)
    {  if(mvmtab[i] == (mvm_type *)NULL)
       {  mvmtab[i] = mvm;

          pups_set_errno(OK);
          return(mvm->vmem);
       }
    }

    (void)snprintf(errstr,SSIZE,"mvm_init: too many metata virtual memory objects (max per process %d)\n",MVM_TABLE_SIZE);
    error(errstr); 

    pups_exit(255);
}



    
/*---------------------------------------------------------------------------------
    Destroy meta virtual memory structure ...
---------------------------------------------------------------------------------*/

_PUBLIC int mvm_destroy(mvm_type *mvm)

{   int i;

    if(mvm == (mvm_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*---------------------------*/
    /* Remove MVM from MVM table */
    /*---------------------------*/

    for(i=0; i<MVM_TABLE_SIZE; ++i)
    {  if(mvmtab[i] == mvm)
          mvmtab[i] = (mvm_type *)NULL;
    }


    /*------------------------------------------------*/
    /* Write any pages in mvm object to backing store */
    /* if it has been marked read/write               */
    /*------------------------------------------------*/

    if(mvm->r_w_state == MVM_READ_WRITE)
    {  for(i=0; i<mvm->v_page_slots; ++i)
          if(mvm->v_page_map[i] != PAGE_NOT_USED)
             mvm_write_page_to_backing_store(mvm,mvm->page_status[i].v_page);            
    }

    for(i=0; i<mvm->v_page_slots; ++i)
    {  if(mvm->v_page_map[i] != PAGE_NOT_USED)
          (void *)pups_free((void *)mvm->vmem[i]); 
    }

    (void *)pups_free((void *)mvm->lock_map);
    (void *)pups_free((void *)mvm->v_page_map);

    pups_set_errno(OK);
    return(0);
}



/*---------------------------------------------------------------------------------
    Reset image cache usage list pointer ...
---------------------------------------------------------------------------------*/

_PUBLIC int mvm_reset_pager(mvm_type *mvm)

{   int i;

    if(mvm == (mvm_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    mvm->ulist_ptr        = 0;
    mvm->aged_and_ordered = FALSE;


    /*---------------------------------------------------------*/
    /* Release all locked pages and reset the lock map counter */
    /*---------------------------------------------------------*/

    ++mvm->current_lock_state;
    mvm->next_lock = 0;

    pups_set_errno(OK);
    return(0);
}



/*---------------------------------------------------------------------------------
    Update the lock map of current meta virtual object ...
---------------------------------------------------------------------------------*/

_PRIVATE int mvm_update_lock_map(int phys_page, mvm_type *mvm)

{   int i;

    if(phys_page < 0 || mvm == (mvm_type *)NULL)
       return(-1);


    /*-----------------------------------------------------*/
    /* Is the page already locked - if it is simply return */
    /*-----------------------------------------------------*/

    for(i=0; i<mvm->next_lock; ++i)
    {  if(mvm->lock_map[i] == phys_page)
          return(0);
    }


    /*-------------------------------------------------------------------*/
    /* Add the page to the table of locked pages, extending the table if */
    /* necessary                                                         */
    /*-------------------------------------------------------------------*/

    if(mvm->next_lock == mvm->max_lock)
    {  mvm->max_lock += MVM_QUANTUM;
       if((mvm->lock_map = (int *)pups_realloc((void *)mvm->lock_map,mvm->max_lock*sizeof(int))) == (int *)NULL)
           return(-1);
    }

    mvm->page_status[phys_page].locked = mvm->current_lock_state;
    mvm->lock_map[mvm->next_lock] = phys_page; 
    ++mvm->next_lock;

    return(0);
}




/*---------------------------------------------------------------------------------
    Build index table of current page usage levels ...
---------------------------------------------------------------------------------*/

_PRIVATE int mvm_index_phys_pages(mvm_type *mvm)

{   int i,
        j,
        l,
        ir,
        indxt,
        q;

    if(mvm == (mvm_type *)NULL)
       return(-1);

    for(j=1; j<mvm->page_slots; ++j)
       mvm->usage_map[j] = j;

    l  = ((mvm->page_slots - 1) >> 1) + 1;
    ir = mvm->page_slots - 1;

    for(;;)
    {  if(l > 1)
          q = mvm->page_status[(indxt = mvm->usage_map[--l])].v_page_usage;
       else
       {  q = mvm->page_status[(indxt = mvm->usage_map[ir])].v_page_usage;
          mvm->usage_map[ir] = mvm->usage_map[1];

          if(--ir == 1)
          {  mvm->usage_map[1] = indxt;
             return;
          }
       }

       i = l;
       j = l << 1;

       while(j <= ir)
       {  if(j < ir                                               &&
             mvm->page_status[mvm->usage_map[j]].v_page_usage      <
             mvm->page_status[mvm->usage_map[j+1]].v_page_usage    )
             ++j; 

          if(q < mvm->page_status[mvm->usage_map[j]].v_page_usage)
          {  mvm->usage_map[i] = mvm->usage_map[j];
             j += (i=j);
          }
          else
             j = ir + 1;
       }

       mvm->usage_map[i] = indxt;
   }
}



/*---------------------------------------------------------------------------------
    Write a page of memory to backing store - note that this page is locked
    when this is done to avoid race condtions (if we have multiple readers
    and or writers) ...
---------------------------------------------------------------------------------*/

_PRIVATE int mvm_read_page_from_backing_store(mvm_type *mvm, int v_page)

{   unsigned long int v_page_offset,
                      bytes_read; 

    struct flock page_flock;

    if(v_page < 0 || mvm == (mvm_type *)NULL)
       return(-1);

    v_page_offset = mvm->v_page_size*v_page;

    if((pups_lseek(mvm->fd,mvm->v_page_base + v_page_offset,L_SET)) == (-1))
       return(-1);


    #ifdef SUPPORT_SHARED_MVM
    /*--------------------------*/
    /* Set lock on file segment */
    /*--------------------------*/

    page_flock.l_type   = F_RDLCK;
    page_flock.l_whence = mvm->v_page_base + v_page_offset;
    page_flock.l_start  = 1;
    page_flock.l_len    = mvm->v_page_size*sizeof(_BYTE);
    (void)fcntl(mvm->fd,F_SETLKW,&page_flock);
    #endif /* SUPPORT_SHARED_MVM */

    if(mvm->initialised == TRUE && (bytes_read = pups_pipe_read(mvm->fd,mvm->vmem[v_page],mvm->v_page_size*sizeof(_BYTE))) != mvm->v_page_size*sizeof(_BYTE))
    {  (void)fprintf(stderr,"mvm_page: page read failed for mvm %s (page %d, offset %d: %d bytes read)\n",
                                                                                         mvm->name,v_page,
                                                                                            v_page_offset,
                                                                                               bytes_read);
       (void)fflush(stderr);
    }


    #ifdef SUPPORT_SHARED_MVM
    /*------------------------------*/
    /* Release lock on file segment */
    /*------------------------------*/

    (void)fcntl(mvm->fd,F_UNLCK,&page_flock);
    #endif /* SUPPORT_SHARED_MVM */

    return(0);
}




/*---------------------------------------------------------------------------------
    Write a page of memory to backing store ...
---------------------------------------------------------------------------------*/
 
_PRIVATE int mvm_write_page_to_backing_store(mvm_type *mvm, int v_page) 
 
{   unsigned long int v_page_offset,
                      bytes_written; 

    struct flock page_flock;

    if(v_page < 0 || mvm == (mvm_type *)NULL)
       return(-1);

    v_page_offset = mvm->v_page_size*v_page;
    if((pups_lseek(mvm->fd,mvm->v_page_base + v_page_offset,L_SET)) == (-1))
       return(-1);

    #ifdef SUPPORT_SHARED_MVM
    /*--------------------------*/
    /* Set lock on file segment */
    /*--------------------------*/

    page_flock.l_type   = F_WRLCK;
    page_flock.l_whence = mvm->v_page_base + v_page_offset;
    page_flock.l_start  = 1;
    page_flock.l_len    = mvm->v_page_size*sizeof(_BYTE);
    (void)fcntl(mvm->fd,F_SETLKW,&page_flock);
    #endif /* SUPPORT_SHARED_MVM */

    if((bytes_written = write(mvm->fd,mvm->vmem[v_page], mvm->v_page_size*sizeof(_BYTE))) != mvm->v_page_size*sizeof(_BYTE))
    {  (void)fprintf(stderr,"mvm_page: page write failed for mvm %s (page %d, offset %d: %d bytes read)\n",
                                                                                          mvm->name,v_page,
                                                                                             v_page_offset,
                                                                                             bytes_written);
       (void)fflush(stderr);
    }   

    #ifdef SUPPORT_SHARED_MVM
    /*------------------------------*/
    /* Release lock on file segment */
    /*------------------------------*/

    (void)fcntl(mvm->fd,F_UNLCK,&page_flock);
    #endif /* SUPPORT_SHARED_MVM */

    return(0);
}




/*---------------------------------------------------------------------------------
    Create an MVM swap file and return a handle to it ...
---------------------------------------------------------------------------------*/
 
_PUBLIC int mvm_create_named_swapfile(char *swap_file_name,
                                      int            pages,
                                      long       page_size)
 
{   int   i,
          swap_handle = (-1);

    _BYTE *buf        = (_BYTE *)NULL;
    struct flock page_flock;

    if(swap_file_name == (char *)NULL || pages < 0 || page_size < 0L)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*--------------------------------------------------*/
    /* Check that this swap file does not already exist */
    /*--------------------------------------------------*/

    if(access(swap_file_name,F_OK) != (-1))
    {  pups_set_errno(EEXIST);
       return(-1);
    }


    /*------------------*/
    /* Create swap file */
    /*------------------*/

    if(pups_creat(swap_file_name,0644) == (-1))
       return(-1);

    pups_set_errno(OK);
    swap_handle = pups_open(swap_file_name,2,LIVE);

    #ifdef SUPPORT_SHARED_MVM
    /*--------------------------*/
    /* Set lock on file segment */
    /*--------------------------*/

    page_flock.l_type   = F_WRLCK;
    page_flock.l_whence = 0;
    page_flock.l_start  = 0;
    page_flock.l_len    = 1;


    /*-----------------------------------------------------*/
    /* If we fail to get lock on paging file simply return */
    /* someone else is taking care of initialisation!      */
    /*-----------------------------------------------------*/

    if(fcntl(swap_handle,F_SETLK,&page_flock) == (-1))
    {  page_flock.l_type   = F_RDLCK;
       page_flock.l_whence = 0;
       page_flock.l_start  = 0;
       page_flock.l_len    = 1;


       /*--------------------------------------*/
       /* Acquire a read lock before we return */
       /*--------------------------------------*/

       (void)fcntl(swap_handle,F_SETLKW,&page_flock);

       pups_set_errno(OK);
       return(swap_handle);
    }
    #endif /* SUPPORT_SHARED_MVM */


    /*----------------*/
    /* Fill swap file */
    /*----------------*/

    buf = (_BYTE *)pups_malloc(page_size);
    for(i=0; i<pages; ++i)
       (void)write(swap_handle,buf,page_size);


    /*------------------------------*/
    /* Set pointer to start of file */
    /*------------------------------*/

    (void)pups_lseek(swap_handle,0,SEEK_SET);


    #ifdef SUPPORT_SHARED_MVM
    /*-----------------------------------*/
    /* Downgrade write lock to read lock */
    /*-----------------------------------*/

    page_flock.l_type   = F_RDLCK;
    page_flock.l_whence = 0;
    page_flock.l_start  = 0;
    page_flock.l_len    = 1;
    (void)fcntl(swap_handle,F_SETLK,&page_flock);
    #endif /* SUPPORT_SHARED_MVM */

    return(swap_handle);
}




/*----------------------------------------------------------------------------------
    Create an MVM swapfile using an open file handle ...
----------------------------------------------------------------------------------*/

_PUBLIC int mvm_create_swapfile(int swap_handle,
                                int       pages,
                                long  page_size)


{   unsigned long int offset;
    struct flock      page_flock;

    if(swap_handle < 0 || pages < 0 || page_size < 0L)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    #ifdef SUPPORT_SHARED_MVM
    /*--------------------------*/
    /* Set lock on file segment */
    /*--------------------------*/

    page_flock.l_type   = F_WRLCK;
    page_flock.l_whence = 0;
    page_flock.l_start  = 0;
    page_flock.l_len    = 1;


    /*-----------------------------------------------------*/
    /* If we fail to get lock on paging file simply return */
    /* someone else is taking care of initialisation!      */
    /*-----------------------------------------------------*/

    if(fcntl(swap_handle,F_SETLK,&page_flock) == (-1))
    {  page_flock.l_type   = F_RDLCK;
       page_flock.l_whence = 0;
       page_flock.l_start  = 0;
       page_flock.l_len    = 1;


       /*--------------------------------------*/
       /* Acquire a read lock before we return */
       /*--------------------------------------*/

       (void)fcntl(swap_handle,F_SETLKW,&page_flock);

       pups_set_errno(EBUSY);
       return(-1);
    }
    #endif /* SUPPORT_SHARED_MVM */

    pups_set_errno(OK);
    offset = pups_lseek(swap_handle,0,SEEK_CUR); 
    (void)pups_lseek(swap_handle,(long)pages*page_size,SEEK_END);
    (void)pups_lseek(swap_handle,0,SEEK_SET);


    #ifdef SUPPORT_SHARED_MVM
    /*-----------------------------------*/
    /* Release lock on paging file       */
    /* Downgrade write lock to read lock */
    /*-----------------------------------*/

    page_flock.l_type   = F_RDLCK;
    page_flock.l_whence = 0;
    page_flock.l_start  = 0;
    page_flock.l_len    = 1;
    (void)fcntl(swap_handle,F_SETLK,&page_flock);
    #endif /* SUPPORT_SHARED_MVM */

    return(0);
}




/*----------------------------------------------------------------------------------
    Delete MVM swapfile ...
----------------------------------------------------------------------------------*/
 
_PUBLIC int mvm_delete_swapfile(int swap_file_handle, char *swap_file_name)
 
{   struct flock page_flock;

    if(swap_file_handle < 0 || swap_file_name == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    pups_set_errno(OK); 
    (void)pups_close((long)swap_file_handle);

    #ifdef SUPPORT_SHARED_MVM
    /*---------------------------------------------------------*/
    /* Release lock on paging file                             */
    /* The last process to leave the room turns off the lights */
    /* and shuts the door!                                     */
    /*---------------------------------------------------------*/

    page_flock.l_type   = F_WRLCK;
    page_flock.l_whence = 0;
    page_flock.l_start  = 0;
    page_flock.l_len    = 1;


    /*---------------------------------------------------------------*/
    /* If we are the last process to hold a read lock on this        */
    /* swapfile we can upgrade it to a write lock. At this point     */
    /* we are the exclusive owner of this file and can safely delete */
    /* it.                                                           */
    /*---------------------------------------------------------------*/

    if(fcntl(swap_file_handle,F_GETLK,&page_flock) == 0)
    #endif /* SUPPORT_SHARED_MVM */
       (void)unlink(swap_file_name);

    if(pups_get_errno() == OK)
       return(0);
    else
       return(-1);
}




/*----------------------------------------------------------------------------------
    Mark MVM swapfile initialised ...
----------------------------------------------------------------------------------*/

_PUBLIC int mvm_initialised(mvm_type *mvm)

{   if(mvm == (mvm_type *)NULL)
    {  pups_set_errno(OK);
       return(-1);
    }
 
    mvm->initialised = TRUE;

    pups_set_errno(OK);
    return(0);
}





/*----------------------------------------------------------------------------------
    Set maximum (resident) cache for MVM object ...
----------------------------------------------------------------------------------*/

_PUBLIC void mvm_change_cache_size(int max_page_slots, mvm_type *mvm)

{   if(mvm == (mvm_type *)NULL)
    {  pups_set_errno(EINVAL);
       return;
    }

    mvm->max_page_slots = max_page_slots;
    pups_set_errno(OK);
}




/*----------------------------------------------------------------------------------
    Show MVM object statitics ...
----------------------------------------------------------------------------------*/

_PUBLIC mvm_stat(FILE *stream, mvm_type *mvm)

{   struct flock page_flock;

    if(mvm == (mvm_type *)NULL || stream == (FILE *)NULL)
    {  pups_set_errno(EINVAL);
       return;
    }

    (void)fprintf(stream,"    Meta virtual memory object: %s\n\n",mvm->name);
    (void)fprintf(stream,"    Page size (virtual)              :  %d\n",mvm->v_page_slots);
    (void)fprintf(stream,"    Page size (physical)             :  %d\n",mvm->max_page_slots);
    (void)fprintf(stream,"    Physical page slots in use       :  %d\n\n",mvm->page_slots); 

    page_flock.l_type   = F_RDLCK;
    page_flock.l_whence = 0;
    page_flock.l_start  = 0;
    page_flock.l_len    = 1;

    #ifdef SUPPORT_SHARED_MVM 
    if(fcntl(mvm->fd,F_GETLK,&page_flock) == (-1))
       (void)fprintf(stream,"    MVM ownership                    :  shared\n");
    else
       (void)fprintf(stream,"    MVM ownership                    :  exclusive\n");
    #endif /* SUPPORT_SHARED_MVM */

    (void)fprintf(stream,"    Paging array at 0x%010x virtual\n",(long)mvm->vmem);

    if(mvm->r_w_state == MVM_READ_WRITE)
       (void)fprintf(stream,"    MVM is read/write\n");
    else
       (void)fprintf(stream,"    MVM is read only\n");



    (void)fprintf(stream,"    Paging array at 0x%010x virtual\n",(long)mvm->vmem);

    if(mvm->r_w_state == MVM_READ_WRITE)
       (void)fprintf(stream,"    MVM is read/write\n");
    else
       (void)fprintf(stream,"    MVM is read only\n");

    if(mvm->sched_policy == MVM_AGED_AND_ORDERED)
       (void)fprintf(stream,"    Using age order scheduling for page replacement\n");
    else
       (void)fprintf(stream,"    Using round robin scheduling for page replacement\n");
  
    if(mvm->initialised == TRUE)
       (void)fprintf(stream,"    MVM is initialised\n");
    else
       (void)fprintf(stream,"    MVM is uninitialised\n");    

    (void)fflush(stream);

    pups_set_errno(OK);
    return;
}




/*----------------------------------------------------------------------------------
    Show MVM objects attached to current application ...
----------------------------------------------------------------------------------*/

_PUBLIC void mvm_show_mvm_objects(FILE *stream)

{   int i,
        cnt = 0;

    if(stream == (FILE *)NULL)
    {  pups_set_errno(EINVAL);
       return;
    }

    (void)fprintf(stream,"    MVM objects map for %s\n\n",appl_name);

    for(i=0; i<MVM_TABLE_SIZE; ++i)
    {   if(mvmtab[i] != (mvm_type *)NULL)
        {  (void)fprintf(stream,"    %8d: %s (at 0x%010x virtual)\n",cnt,mvmtab[i]->name,(long)mvmtab[i]->vmem);
           (void)fflush(stream);

           ++cnt;
        }
    }

    if(cnt == 0)
       (void)fprintf(stream,"    no MVM slots in use (%d MVM slots available)\n",MVM_TABLE_SIZE);
    else if(cnt == 1)
       (void)fprintf(stream,"\n    1 MVM slot in use (%d MVM slots available)\n",MVM_TABLE_SIZE - cnt);
    else  
       (void)fprintf(stream,"\n    %d MVM slots in use (%d MVM slots available)\n",cnt,MVM_TABLE_SIZE - cnt);

    (void)fflush(stream); 
}
