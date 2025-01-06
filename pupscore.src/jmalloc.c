/*-----------------------------------------------------------------------------------
    This version of the memory allocator uses bubbles of memory [mapped in and out of
    of the process adress space via mmap() and munmap()] rather than sbrk(). This
    means the code is (a) POSIX compliant and (b) can often make better use of
    memory resources than traditional memory allocation packages.   

    (C) M.A. O'Neill, 29-12-2024, <mao@tumblingdice.co.uk>.
-----------------------------------------------------------------------------------*/

#ifdef BUBBLE_MEMORY_SUPPORT
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <math.h>
#include <errno.h>
#include <typedefs.h>
#include <stdatomic.h>
#include <stdint.h>


#ifdef PTHREAD_SUPPORT
#include <pthread.h>
#endif /* PTHREAD_SUPPORT */

#define __NOT_LIB_SOURCE__
#include <xtypes.h>

#ifdef P3_SUPPORT
#include <utils.h>
#include <tad.h>
#endif /* P3_SUPPORT */
#undef __NOT_LIB_SOURCE__ 


/*-----------------------------------*/
/* These definitions were in local.h */
/*-----------------------------------*/

#define _CKPT_FTABLE_SIZE 1024
#define GPAGESIZE         ((uint64_t)(4096))


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE             2048 



/*----------------------------------------*/
/* Functions imported by this application */
/*----------------------------------------*/

_IMPORT void exit(int);


/*----------------------------------------*/
/* Variables imported by this application */
/*----------------------------------------*/

_IMPORT int32_t  errno;
_PUBLIC _BOOLEAN initialised = FALSE; 
_PUBLIC void     *start_addr = (void *)NULL;


/*-------------------------------------------------------------------*/
/* Sycall interface to open() and close() is required by mmap_sbrk() */
/*-------------------------------------------------------------------*/

#define CKPT_OPEN(fname,oflag)          syscall(SYS_open, (fname), (oflag))
#define CKPT_CLOSE(fd)                  syscall(SYS_close, (fd))



/*--------------------------------------------*/
/* Variables which are private to this module */
/*--------------------------------------------*/

_PRIVATE void *brk_start = (void *)NULL,
              *brk_stop  = (void *)NULL;


/*----------------------------------------------------*/
/* Definitions used by the memory allocation packeage */
/*----------------------------------------------------*/

                                                     /*-----------------------------------------------*/
#define MUNMAP_THRESHOLD  1                          /* Memory bubble (minimum) utilisation threshold */
#define MALLOC_CRITICAL   2                          /* malloc() call which MUST complete             */
#define JMSZ              (sizeof(struct jmalloc))   /* Size of jmalloc structure                     */
#define PTSZ              (sizeof(void *))           /* Also assuming its > sizeof  int32_t                */
                                                     /*-----------------------------------------------*/

#define MASK              0x17826a9b
#define JNULL             ((Jmalloc) 0)


#ifdef PTHREAD_SUPPORT
_PRIVATE pthread_mutex_t ckpt_malloc_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif /* PTHREAD_SUPPORT */

_PUBLIC Jmalloc         memlist;
_PUBLIC Jmalloc         start;

_PRIVATE struct jmalloc j_head;
_PRIVATE int32_t        jminit             = 0;
_PRIVATE unsigned long  free_called        = 0;
_PRIVATE unsigned long  malloc_called      = 0;
_PRIVATE double         munmap_utilisation = 10.0;



/*------------------------------------------------------*/
/* Various macros used by the memory allocation package */
/*------------------------------------------------------*/

#define jloc(l)   ((void *) (&l->space))
#define jmal(l)   ((Jmalloc) (((void *)l) - JMSZ + PTSZ))


/*------------------------*/
/* Enter critical section */
/*------------------------*/

static atomic_flag flag = { 0 };
static int32_t     enter_malloc_critical(void)

{  while(atomic_flag_test_and_set(&flag) == 1)
         (void)usleep(100);

   return(0);
}



/*------------------------*/
/* Leave critical section */
/*------------------------*/

static int32_t leave_malloc_critical(void)

{  
    atomic_flag_clear(&flag);
    return(0);
}




/*-----------------------------------------------------------------------*/ 
/* Map a memory bubble of size bytes  int32_to the process address space */
/*   Note: size must be an integral multiple of GPAGESIZE                */
/*-----------------------------------------------------------------------*/ 

_PUBLIC void *mmap_sbrk(size_t size)

{   void               *ret    = (void *)NULL;
    _IMMORTAL des_t    fdes    = (-1);
    _IMMORTAL _BOOLEAN entered = FALSE;

    if (initialised == TRUE)
       errno = 0;

    if (size == 0)
       return(brk_stop); 

    #ifdef NO_MMU
    if (brk_start == (void *)NULL)
       brk_start =  (void *)START_ADDRESS;
    #else /* NO_MMU */

    #ifdef NO_MMAP_ANONYMOUS
    fdes = CKPT_OPEN("/dev/zero",2);
    brk_start = (void *)mmap((void *)0,size,PROT_READ | PROT_WRITE,MAP_PRIVATE,fdes,0);
    CKPT_CLOSE(fdes);
    #else

    brk_start = (void *)mmap((void *)0,size,PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS,(-1),0);
    #endif /* NO_MMAP_ANONYMOUS */
    #endif /* NO_MMU */

    if (brk_start == (void *)NULL)
    {  if (initialised == TRUE && errno != 0)
       {  (void)fprintf(stderr,"error %d\n",errno);
          (void)fflush(stderr);
       }
 
       return((void *)NULL);
    }

    if (entered == FALSE)
    {  entered = TRUE;
       start_addr = brk_start;
    }

    if ((void *)((unsigned long)brk_start + (unsigned long)size) > brk_stop)
       brk_stop = (void *)((unsigned long)brk_start + (unsigned long)size);

    #ifdef NO_MMU
    if (brk_stop > END_ADDR)
       return((void *)NULL);
    #endif /*  NO_MMU */

#ifdef JMALLOC_DEBUG
if (initialised == TRUE)
{  (void)fprintf(stderr,"START: 0x%010x   END: 0x%010x   SIZE 0x%010x  TOTAL: %04d [0x%010x]\n",
                 brk_start,brk_stop,size,(unsigned long)brk_stop - (unsigned long)start_addr,start_addr);
   (void)fflush(stderr);
}
#endif /* JMALLOC_DEBUG */

    return(brk_start);
}




/*-------------------------------------------------------------------*/
/* Initialise the memory list (first call to any allocation function */
/* automatically calls do_init()                                     */
/*-------------------------------------------------------------------*/ 

_PRIVATE void do_init(void)

{    if (!jminit)
     {  memlist        = &j_head;
        memlist->flink = memlist;
        memlist->blink = memlist;
        memlist->size  = 0;
        memlist->space = (void *) 0;
        start          = memlist;
        jminit         = 1;
     }
}




/*-----------------*/
/* Exit (on error) */
/*-----------------*/ 

_PRIVATE void dump_core(void)
{
    memlist->space = (void *)NULL;

    #ifdef P3_SUPPORT
    pups_exit(255);
    #endif /* P3_SUPPORT */

}




/*---------------------------------------*/ 
/* Set a piece of memory as being in use */
/*---------------------------------------*/ 

_PRIVATE void *set_used(Jmalloc l)
{
  return jloc(l);
}




/*---------------------------------------------------------------*/
/* Malloc -- return pointer to a region at least size bytes long */
/* The required memory is mapped  int32_to the process address space  */
/* using mmap()                                                  */
/*---------------------------------------------------------------*/ 
/*------------------------------------------*/
/* malloc() called by calloc() or realloc() */
/*------------------------------------------*/

_PRIVATE void *malloc_internal(const _BOOLEAN is_internal, size_t size)
{
    int32_t new_pages;
 
    Jmalloc l,
            h;

    void   *tmp = (void *)NULL;
    Jmalloc newl;

    void   *loc = (void *)NULL;
    size_t newsize;

    sigset_t set,
             old_set;


    /*-------------------------------------------------------------------*/
    /* Fix for glibc3 calling malloc() before errno location is defined! */
    /*-------------------------------------------------------------------*/

    if (initialised == TRUE)
       errno = 0;

    if (size <= 0)
    {  if (malloc_called > 0)
          errno = EINVAL;

       return NULL;
    }


    /*----------------------------------------------------*/
    /* We cannot permit signals to be handled in malloc() */
    /* if we are about to enter a critical section        */
    /*----------------------------------------------------*/

    if (is_internal == FALSE)
    {  (void)sigfillset (&set);
       (void)sigprocmask(SIG_SETMASK,&set,&old_set);
       (void)enter_malloc_critical();
    }

    do_init();


    /*--------------------------------------------------------------------------------------------*/   
    /* In this memory allocator, the memory for each new item is explicitly mapped  int32_to the       */
    /* address space of the calling process. This means that each new item will occupy at least 1 */
    /* page of memory irrespective of its actual size. On modern machines, with large amounts of  */
    /* memory this is an acceptable overhead, given that the resulting code is much cleaner and   */
    /* easier to manipulate than a conventional malloc package.                                   */
     /*--------------------------------------------------------------------------------------------*/   

    new_pages = (size + JMSZ) / GPAGESIZE + 1;
    newsize   = new_pages*GPAGESIZE;
    newl      = (Jmalloc) mmap_sbrk(newsize);


    /*-------------------------------------------------------------------------------*/
    /* Have we actually managed to map in a segment of memory of the requested size? */
    /*-------------------------------------------------------------------------------*/

    if (newl == (Jmalloc)(NULL))
    {   

       #ifdef P3_SUPPORT
       if (appl_verbose == TRUE)
       {  char date[SSIZE];

          (void)strdate(date);
          (void)fprintf(stderr,"%s (%d@%s:%s): Jmalloc: out of memory\n",
                                 appl_name,appl_pid,appl_host,appl_owner);
          (void)fprintf(stderr,"%s (%d@%s:%s): Trying to get 0x%010x bytes (chunk of 0x%010x)\n",
                                            appl_name,appl_pid,appl_host,appl_owner,size,newsize);
          (void)fflush(stderr);
       }
       #else
       (void)fprintf(stderr,"JMALLOC: out of memory\n");
       (void)fflush(stderr);
       #endif /* P3_SUPPORT */

       errno = ENOMEM; 

       /*---------------------*/
       /* Restore signal mask */
       /*---------------------*/

       if (is_internal == FALSE)
       {  (void)leave_malloc_critical();
          (void)sigprocmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);
       }

       return NULL;
    }


    /*-----------------------------------------------------*/
    /* Mark this chunk as the start of a new memory mapped */
    /* area.                                               */
    /*-----------------------------------------------------*/


    newl->mmap_size    = newsize;
    newl->flink        = memlist;
    newl->blink        = memlist->blink;
    newl->flink->blink = newl;
    newl->blink->flink = newl;
    newl->size         = size;
    l                  = newl;

#ifdef JMALLOC_DEBUG
if (l->mmap_size > 0)
{  (void)fprintf(stderr,"MMAP ADDRESS (size %d, %f pages, %f allocated)\n\n",
                 l->size,(double)l->size/(double)GPAGESIZE,(double)newsize/(double)GPAGESIZE);
   (void)fflush(stderr);
}
#endif /* JMALLOC_DEBUG */

#ifdef JMALLOC_DEBUG
(void)fprintf(stderr,"malloc (%d) returned 0x%010x\n",malloc_called,(unsigned long)l);
(void)fflush(stderr);
#endif /* JMALLOC_DEBUG */

    loc = set_used(l);
    malloc_called++;


    /*---------------------*/
    /* Restore signal mask */
    /*---------------------*/
 
    if (is_internal == FALSE)
    {  (void)leave_malloc_critical();
       (void)sigprocmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);
    }
 
    return(loc);
}


/*--------------------------------------*/
/* malloc() called by external function */
/*--------------------------------------*/

_PUBLIC void *malloc(size_t size)

{    return(malloc_internal(FALSE,size));
}




/*---------------------------------------------------------------------*/ 
/*Free -- free the memory bubble associated with loc, unmapping memory */
/* associated with it from the process address space                   */ 
/*---------------------------------------------------------------------*/ 
/*----------------------------------------*/
/* free() called by calloc() or realloc() */
/*----------------------------------------*/

_PRIVATE void free_internal(const _BOOLEAN is_internal, const void *loc)
{
    Jmalloc l,
            pl,
            nl;

    int32_t i;
    _BYTE   *addr = (_BYTE *)NULL; 

    sigset_t set,
             old_set;

    if (loc == (void *)NULL)
    {  errno = EINVAL;
       return;
    }


    /*-------------------------------------------------*/
    /* We cannot allow signals to be handled in free() */
    /*-------------------------------------------------*/

    if (is_internal == FALSE)
    {  (void)sigfillset (&set);
       (void)sigprocmask(SIG_SETMASK,&set,&old_set);
       (void)enter_malloc_critical();
    }

    do_init();
    free_called++;
    l = jmal(loc); 

    pl = l->blink;
    nl = l->flink;   

#ifdef JMALLOC_DEBUG
(void)fprintf(stderr,"FREE BLOCK UNMAPPED (size %x at at 0x%010x)\n",l->mmap_size,l);
(void)fflush(stderr);
#endif /* JMALLOC_DEBUG */

    if (l == brk_start)
    {  brk_stop = l;

#ifdef JMALLOC_DEBUG
(void)fprintf(stderr,"BRK is now 0x%010x\n",brk_stop);
(void)fflush(stderr);
#endif /* JMALLOC_DEBUG */

    }


    /*-------------------------------------------------------------*/
    /* Create a fractional block. This is the difference between   */
    /* the size of the object to be freed and the percentage of it */
    /* which could be unmapped                                     */
    /*-------------------------------------------------------------*/

    if (initialised == TRUE)
       errno = 0;

    (void)munmap((void *)l,(size_t)l->mmap_size);

#ifdef JMALLOC_DEBUG
(void)fprintf(stderr,"MUNMAP returned errno %d\n\n",errno);
(void)fflush(stderr);
#endif /* JMALLOC_DEBUG */

    if (initialised == TRUE && errno != 0)
    {  (void)fprintf(stderr,"errno is %d\n",errno);
       (void)fflush(stderr);

       exit(255);
    }                 

    pl->flink = nl;
    nl->blink = pl;
    start     = nl;

#ifdef JMALLOC_DEBUG
(void)fprintf(stderr,"free called (%04d) for ptr 0x%010x\n",free_called,(unsigned long)loc);
(void)fflush(stderr);
#endif /* JMALLOC_DEBUG */

    if (initialised == TRUE)
       errno = 0;


  /*---------------------*/
  /* Restore signal mask */
  /*---------------------*/

    if (is_internal == FALSE)
    {  (void)leave_malloc_critical();
       (void)sigprocmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);
    }

    return;
}


/*-----------------------------*/
/* Called by external function */
/*-----------------------------*/

_PUBLIC void free(const void *loc)

{   return(free_internal(FALSE,loc));
}



/*----------------------------------------------------------------*/
/* Realloc -- allocate more space to the region pointed to by ptr */
/* possibly moving the objects stored within the region           */
/*----------------------------------------------------------------*/ 

_PUBLIC void *realloc(const void *ptr, const size_t size)

{
    Jmalloc l,
            l2,
            nl,
            newl;

    _BYTE   *loc  = (_BYTE *)NULL;
    _BYTE   *loc2 = (_BYTE *)NULL;

    size_t i,
           old_size,
           new_size;

    sigset_t set,
             old_set;

    if (initialised == TRUE)
       errno = 0;


    /*---------------------------------------------*/
    /* We are not in critical section of realloc() */
    /* so free() is effectively external           */
    /*---------------------------------------------*/

    new_size = size;
    if (new_size == 0)
    {  if (ptr != (void *)NULL)
          free(ptr);

       return((void *)NULL);
    }


    /*-------------------------------*/
    /* This case reduces to malloc() */
    /*-------------------------------*/

    if (ptr == (const void *)NULL && new_size > 0)
    {  loc = (void *)malloc_internal(TRUE,new_size);
       return(loc);
    }


    /*----------------------------------------------------*/
    /* We cannot allow signals to be handled in realloc() */
    /*----------------------------------------------------*/

    (void)sigfillset (&set);
    (void)sigprocmask(SIG_SETMASK,&set,&old_set);
    (void)enter_malloc_critical();

    loc = (_BYTE *)ptr;
    do_init();

    l        = jmal(loc); 
    old_size = l->size;


    /*---------------------------------------------------------------------------*/
    /* Can we fit the reallocated memory segment  int32_to the current memory mapped  */
    /* region? If so lets do it! If we cannot do this we will need to allocate a */
    /* new (bigger) block, copy the contents of the currently allocated block to */
    /* this new block, and then unmap the current block from the process address */
    /* space.                                                                    */
    /*---------------------------------------------------------------------------*/

#ifdef JMALLOC_DEBUG
(void)fprintf(stderr,"REALLOC: block size 0x%010x (new size 0x%010x)\n",l->mmap_size - JMSZ - PTRSZ, size);
(void)fflush(stderr);
#endif /* JMALLOC_DEBUG */

    if (new_size < l->mmap_size - JMSZ - 2*PTSZ)
    {  double utilisation;

       utilisation = 100.0*(double)new_size/(double)l->mmap_size;

#ifdef JMALLOC_DEBUG
(void)fprintf(stderr,"REALLOC: block (0x%010x, size 0x%010x) utilisation: %6.3f %%\n",l->size,size,utilisation);
(void)fflush(stderr);
#endif /* JMALLOC_DEBUG */


       /*--------------------------*/
       /* Can this bubble be used? */
       /*--------------------------*/

       if (l->mmap_size == GPAGESIZE || utilisation > munmap_utilisation) 
       {  l->size   = new_size;


          /*---------------------*/
          /* Restore signal mask */
          /*---------------------*/

          (void)leave_malloc_critical();
          (void)sigprocmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);


#ifdef JMALLOC_DEBUG
(void)fprintf(stderr,"REALLOC: adjusting block of 0x%010x (new size 0x%010x)\n",l->size,size);
(void)fflush(stderr);
#endif /* JMALLOC_DEBUG */

          return loc;
       }        


       /*----------------------------------------------------------------------------*/
       /* If utilisation of the mapped region is less than 10% then unmap the region */
       /* and return memory to kernel (so other processes can use it)                */
       /*----------------------------------------------------------------------------*/

#ifdef JMALLOC_DEBUG
(void)fprintf(stderr,"REALLOC: freeing block of 0x%010x (new size 0x%010x) [utilisation %6.3f %%]\n",l->size,size,utilisation);
(void)fflush(stderr);
#endif /* JMALLOC_DEBUG */

    }


    /*---------------------------------------------------------------------------*/
    /* We need to allocate a new block, copy the contents of the old block to it */
    /* and then unmap the old block from the process address space               */
    /* If the block is bigger than 64M the we will need to flush data to /tmp to */
    /* prevent an acute memory shortage occuring.                                */
    /*---------------------------------------------------------------------------*/

    loc2 = malloc_internal(TRUE,new_size);
    for (i = 0; i < old_size; i++)
        loc2[i] = loc[i];


    /*-------------------------------------------*/
    /* In critical section so free() is internal */
    /*-------------------------------------------*/

    free_internal(TRUE,loc);

#ifdef JMALLOC_DEBUG
(void)fprintf(stderr,"REALLOC [0x%010x] returned 0x%010x (with loc1 free)\n",(unsigned long)ptr,(unsigned long)l);
(void)fflush(stderr);
#endif /* JMALLOC_DEBUG */


    /*---------------------*/
    /* Restore signal mask */
    /*---------------------*/

    (void)leave_malloc_critical();
    (void)sigprocmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);

    return loc2;
}




/*---------------------------------------------------------*/ 
/* Calloc -- allocate space for nelem items of size elsize */ 
/*---------------------------------------------------------*/ 

_PUBLIC void *calloc(const size_t nelem, const size_t elsize)

{
    int32_t  *iptr = (int *) NULL;
    _BYTE     *ptr  = (void *)NULL;
    uint64_t  sz;
    uint32_t  i;

    sigset_t set,
             old_set;

    if (initialised == TRUE)
       errno = 0;


    /*---------------------------------------------------*/
    /* We cannot allow signals to be handled in calloc() */
    /*---------------------------------------------------*/

    (void)sigfillset (&set);
    (void)sigprocmask(SIG_SETMASK,&set,&old_set);
    (void)enter_malloc_critical();

    sz  = nelem*elsize;
    ptr = malloc_internal(TRUE,sz);
    if (ptr == (_BYTE *)NULL)
    {

       /*---------------------*/
       /* Restore signal mask */
       /*---------------------*/

       (void)leave_malloc_critical();
       (void)sigprocmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);

       return NULL;
    }

    iptr = (int *) ptr;
    for (i = 0; i < sz/sizeof(int); i++)
        iptr[i] = 0;

    for (i = i * sizeof(int); i < sz; i++)
        ptr[i] = 0;


    /*---------------------*/
    /* Restore signal mask */
    /*---------------------*/

    (void)leave_malloc_critical();
    (void)sigprocmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);

    return ((void *)ptr);
}




/*------------------------------------------------------------*/
/* Set the quantum for flushing out low useage memory bubbles */
/*------------------------------------------------------------*/ 

_PUBLIC  int32_t mallopt(const  int32_t cmd, const  int32_t value)
{

    /*---------------------------------------------------------------------*/
    /* Set the minimum percentage utilisation for a memory bubble. If the  */
    /* utilisation falls below this threshold, the bubble is unmapped from */ 
    /* the process address space.                                          */
    /*---------------------------------------------------------------------*/

    if (cmd == MUNMAP_THRESHOLD)
    {  if (value == (-1))
          return((int)munmap_utilisation);
       else
       {  munmap_utilisation = value;
          return(0);
       }
    }

    return(0);
}



/*------------------------------------------------------------------------*/
/* Print statistics on the number of memory bubbles currently mapped  int32_to */
/* the process address space                                              */
/*------------------------------------------------------------------------*/ 

_PUBLIC void jmalloc_usage(const  int32_t show_bubbles, const FILE *stream)

{   Jmalloc next = (Jmalloc)NULL;

    int32_t  cnt = 0;
 
    unsigned long mem_used = 0,
             mem_allocated = 0;

    double utilisation;


    #ifdef P3_SUPPORT
    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if (pupsthread_is_root_thread() == FALSE)
       pups_error("attempt by non root thread to perform global jmalloc operation");
    #endif /* P3_SUPPORT */

    errno = 0;
    if (stream == (const FILE *)NULL)
    {  errno = EINVAL;
       return;
    }

    (void)enter_malloc_critical();

    do_init(); 

    (void)fprintf(stream,"\n\n    Memory bubbles mapped  int32_to process address space\n");
    (void)fprintf(stream,"    ================================================\n");
 
    if (show_bubbles == TRUE)
       (void)fprintf(stream,"\n");

    (void)fflush(stream);


    /*----------------------------------------------------------------*/
    /* In order to extract statistics we need to walk the linked list */
    /* of mapped memory bubbles.                                      */
    /*----------------------------------------------------------------*/

    next = memlist;
    do {    if (show_bubbles == TRUE)
            {  if (next->size > 0)
                  utilisation = 100.0*(double)next->size/(double)next->mmap_size;
               else
                  utilisation = 0.0;

               (void)fprintf(stream,"    %04d: bubble at 0x%010x virtual, size 0x%010x, used 0x%010x (utilisation %5.2f %%)\n",
                                                         cnt,(unsigned long)next + JMSZ,next->size,next->mmap_size,utilisation);
               (void)fflush(stream);
            }

            ++cnt;

            mem_used      += next->size;
            mem_allocated += next->mmap_size;

            next = next->flink;
        } while(next != start);

     utilisation = 100.0*(double)mem_used/(double)mem_allocated;
     (void)fprintf(stream,"\n\n    %04d bubbles currently mapped  int32_to process address space (utilisation theshold is %5.2f %%)\n",
                                                                                                         cnt,munmap_utilisation);
     (void)fprintf(stream,"    Total memory used 0x%010x bytes, allocated 0x%010x bytes (utilisation %5.2f %%)\n\n",
                                                                                 mem_used,mem_allocated,utilisation);
     (void)fflush(stream); 

     (void)leave_malloc_critical();
}
#endif /* BUBBLE_MEMORY_SUPPORT */
