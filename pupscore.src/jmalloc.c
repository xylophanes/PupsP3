/*-------------------------------------------------------------------------------------
    This version of the memory allocator uses bubbles of memory [mapped in and out of
    of the process adress space via mmap() and munmap()] rather than sbrk(). This
    means the code is (a) POSIX compliant and (b) can often make better use of
    memory resources than traditional memory allocation packages.   

    (C) M.A. O'Neill, 02-09-2019, mao@tumblingdice.co.uk.
-------------------------------------------------------------------------------------*/


#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <math.h>
#include <errno.h>
#include <typedefs.h>

#ifdef PTHREAD_SUPPORT
#include <pthread.h>
#endif /* PTHREAD_SUPPORT */


#define __NOT_LIB_SOURCE__
#include <xtypes.h>

#ifdef P3_SUPPORT
#include <utils.h>
#endif /* P3_SUPPORT */
#undef __NOT_LIB_SOURCE__ 


/*-----------------------------------*/
/* These definitions were in local.h */
/*-----------------------------------*/

#define _CKPT_FTABLE_SIZE 1024
#define GPAGESIZE         ((unsigned long)(4096))


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

_IMPORT  int     errno;
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
#define PTSZ              (sizeof(void *))           /* Also assuming its > sizeof int                */
                                                     /*-----------------------------------------------*/

#define MASK              0x17826a9b
#define JNULL             ((Jmalloc) 0)


#ifdef PTHREAD_SUPPORT
_PRIVATE pthread_mutex_t ckpt_malloc_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif /* PTHREAD_SUPPORT */

_PUBLIC Jmalloc         memlist;
_PUBLIC Jmalloc         start;

_PRIVATE struct jmalloc j_head;
_PRIVATE int            jminit             = 0;
_PRIVATE unsigned long  free_called        = 0;
_PRIVATE unsigned long  malloc_called      = 0;
_PRIVATE double         munmap_utilisation = 10.0;



/*------------------------------------------------------*/
/* Various macros used by the memory allocation package */
/*------------------------------------------------------*/

#define jloc(l)   ((void *) (&l->space))
#define jmal(l)   ((Jmalloc) (((void *)l) - JMSZ + PTSZ))


/*---------------------------------------------------------------------------- 
   Map a memory bubble of size bytes into the process address space.
   Note: size must be an integral multiple of GPAGESIZE. 
----------------------------------------------------------------------------*/ 

_PUBLIC void *mmap_sbrk(size_t size)

{   void               *ret    = (void *)NULL;
    _IMMORTAL int      fdes    = (-1);
    _IMMORTAL _BOOLEAN entered = FALSE;

    if(initialised == TRUE)
       errno = 0;

    if(size == 0)
       return(brk_stop); 

#ifdef NO_MMU
    if(brk_start == (void *)NULL)
       brk_start = (void *)START_ADDRESS;
#else /* NO_MMU */

#ifdef NO_MMAP_ANONYMOUS
    fdes = CKPT_OPEN("/dev/zero",2);
    brk_start = (void *)mmap((void *)0,size,PROT_READ | PROT_WRITE,MAP_PRIVATE,fdes,0);
    CKPT_CLOSE(fdes);
#else

    brk_start = (void *)mmap((void *)0,size,PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS,(-1),0);
#endif /* NO_MMAP_ANONYMOUS */
#endif /* NO_MMU */

    if(brk_start == (void *)NULL)
    {  if(initialised == TRUE && errno != 0)
       {  (void)fprintf(stderr,"error %d\n",errno);
          (void)fflush(stderr);
       }
 
       return((void *)(-1));
    }

    if(entered == FALSE)
    {  entered = TRUE;
       start_addr = brk_start;
    }

   if((void *)((unsigned long)brk_start + (unsigned long)size) > brk_stop)
      brk_stop = (void *)((unsigned long)brk_start + (unsigned long)size);

#ifdef NO_MMU
   if(brk_stop > END_ADDR)
      return((void *)NULL);
#endif /*  NO_MMU */

#ifdef JMALLOC_DEBUG
if(initialised == TRUE)
{   (void)fprintf(stderr,"START: 0x%010x   END: 0x%010x   SIZE 0x%010x  TOTAL: %04d [0x%010x]\n",
                  brk_start,brk_stop,size,(unsigned long)brk_stop - (unsigned long)start_addr,start_addr);
    (void)fflush(stderr);
}
#endif /* JMALLOC_DEBUG */

    return(brk_start);
}




/*---------------------------------------------------------------------------- 
    Initialise the memory list (first call to any allocation function.
    automatically calls do_init() ...
----------------------------------------------------------------------------*/ 

_PRIVATE void do_init(void)

{   if (!jminit)
    {  memlist        = &j_head;
       memlist->flink = memlist;
       memlist->blink = memlist;
       memlist->size  = 0;
       memlist->space = (void *) 0;
       start          = memlist;
       jminit         = 1;
    }
}




/*---------------------------------------------------------------------------- 
    Exit (on error) ... 
----------------------------------------------------------------------------*/ 

_PRIVATE void dump_core(void)
{
   memlist->space = (void *)NULL;

#ifdef P3_SUPPORT
   pups_exit(-1);
#endif /* P3_SUPPORT */

}




/*---------------------------------------------------------------------------- 
    Set a piece of memory as "being in use ...
----------------------------------------------------------------------------*/ 

_PRIVATE void *set_used(Jmalloc l)
{
  return jloc(l);
}




/*---------------------------------------------------------------------------- 
    Malloc -- return pointer to a region at least size bytes long.
    The required memory is mapped into the process address space
    using mmap() ...
----------------------------------------------------------------------------*/ 

_PUBLIC void *malloc(size_t size)
{
  int new_pages;

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

  if(initialised == TRUE)
     errno = 0;

  if(size <= 0)
  {  if(malloc_called > 0)
        errno = EINVAL;

     return NULL;
  }

  /*----------------------------------------------------*/
  /* We cannot permit signals to be handled in malloc() */
  /* if we are about to enter a critical section        */
  /*----------------------------------------------------*/

  (void)sigfillset(&set);

  #ifdef PTHREAD_SUPPORT
  (void)pthread_sigmask(SIG_BLOCK,&set,&old_set);
  #else
  (void)sigprocmask(SIG_BLOCK,&set,&old_set);
  #endif /* THREAD_SUPPORT */

 
  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_lock(&ckpt_malloc_mutex);
  #endif /* THREAD_SUPPORT */

  do_init();


  /*--------------------------------------------------------------------------------------------*/   
  /* In this memory allocator, the memory for each new item is explicitly mapped into the       */
  /* address space of the calling process. This means that each new item will occupy at least 1 */
  /* page of memory irrespective of its actual size. On modern machines, with large amounts of  */
  /* memory this is an acceptable overhead, given that the resulting code is much cleaner and   */
  /* easier to read than a conventional malloc package.                                         */
  /*--------------------------------------------------------------------------------------------*/   

  new_pages = (size + JMSZ) / GPAGESIZE + 1;
  newsize   = new_pages*GPAGESIZE;
  newl      = (Jmalloc) mmap_sbrk(newsize);


  /*-------------------------------------------------------------------------------*/
  /* Have we actually managed to map in a segment of memory of the requested size? */
  /*-------------------------------------------------------------------------------*/

  if (newl == (Jmalloc)(-1))
  {   

      #ifdef P3_SUPPORT
      if(appl_verbose == TRUE)
      {  char date[SSIZE];

         (void)strdate(date);
         (void)fprintf(stderr,"%s %s (%d@%s:%s): Jmalloc: out of memory\n",
                              date,appl_name,appl_pid,appl_host,appl_owner);
         (void)fprintf(stderr,"%s %s (%d@%s:%s): Trying to get 0x%010x bytes (chunk of 0x%010x)\n",
                                         date,appl_name,appl_pid,appl_host,appl_owner,size,newsize);
         (void)fflush(stderr);
      }
      #else
      (void)fprintf(stderr,"JMALLOC: out of memory\n");
      (void)fflush(stderr);
      #endif /* P3_SUPPORT */

     errno = ENOMEM; 

      #ifdef PTHREAD_SUPPORT
      (void)pthread_mutex_unlock(&ckpt_malloc_mutex);
      #endif /* PTHREAD_SUPPORT */


     /*---------------------*/
     /* Restore signal mask */
     /*---------------------*/
     #ifdef PTHREAD_SUPPORT
     (void)pthread_sigmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);
     #else
     (void)sigprocmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);
     #endif /* PTHREAD_SUPPORT */

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
if(l->mmap_size > 0)
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

   #ifdef PTHREAD_SUPPORT
   (void)pthread_mutex_unlock(&ckpt_malloc_mutex);
   #endif /* PTHREAD_SUPPORT */


   /*---------------------*/
   /* Restore signal mask */
   /*---------------------*/

   #ifdef PTHREAD_SUPPORT
   (void)pthread_sigmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);
   #else
   (void)sigprocmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);
   #endif /* PTHREAD_SUPPORT */

   return(loc);
}




/*---------------------------------------------------------------------------- 
    Free -- free the memory bubble associated with loc, unmapping memory
    associated with it from the process address space ... 
----------------------------------------------------------------------------*/ 

_PUBLIC void free(const void *loc)
{
  Jmalloc l;
  Jmalloc pl, nl;

  int   i;
  _BYTE *addr = (_BYTE *)NULL; 

  sigset_t set,
           old_set;

  if(loc == (void *)NULL)
  {  errno = EINVAL;
     return;

  }


  /*-------------------------------------------------*/
  /* We cannot allow signals to be handled in free() */
  /*-------------------------------------------------*/

  (void)sigfillset(&set);

  #ifdef PTHREAD_SUPPORT
  (void)pthread_sigmask(SIG_BLOCK,&set,&old_set);
  #else
  (void)sigprocmask(SIG_BLOCK,&set,&old_set);
  #endif /* PTHREAD_SUPPORT */

  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_lock(&ckpt_malloc_mutex);
  #endif /* PTHREAD_SUPPORT */

  do_init();
  free_called++;
  l = jmal(loc); 

  pl = l->blink;
  nl = l->flink;   

#ifdef JMALLOC_DEBUG
(void)fprintf(stderr,"FREE BLOCK UNMAPPED (size %x at at 0x%010x)\n",l->mmap_size,l);
(void)fflush(stderr);
#endif /* JMALLOC_DEBUG */

  if(l == brk_start)
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

  if(initialised == TRUE)
     errno = 0;

  (void)munmap((void *)l,(size_t)l->mmap_size);

#ifdef JMALLOC_DEBUG
(void)fprintf(stderr,"MUNMAP returned errno %d\n\n",errno);
(void)fflush(stderr);
#endif /* JMALLOC_DEBUG */

  if(initialised == TRUE && errno != 0)
  {  (void)fprintf(stderr,"errno is %d\n",errno);
     (void)fflush(stderr);

     exit(-1);
  }                 

  pl->flink = nl;
  nl->blink = pl;
  start     = nl;

#ifdef JMALLOC_DEBUG
(void)fprintf(stderr,"free called (%04d) for ptr 0x%010x\n",free_called,(unsigned long)loc);
(void)fflush(stderr);
#endif /* JMALLOC_DEBUG */

  if(initialised == TRUE)
     errno = 0;

  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_unlock(&ckpt_malloc_mutex);
  #endif /* PTHREAD_SUPPORT */


  /*---------------------*/
  /* Restore signal mask */
  /*---------------------*/

   #ifdef PTHREAD_SUPPORT
  (void)pthread_sigmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);
  #else
  (void)sigprocmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);
  #endif /* PTHREAD_SUPPORT */

  return;
}




/*---------------------------------------------------------------------------- 
    Realloc -- allocate more space to the region pointed to by ptr
    possibly moving the objects stored within the region ...
----------------------------------------------------------------------------*/ 

_PUBLIC void *realloc(const void *ptr, const size_t size)

{
  Jmalloc l,
          l2,
          nl,
          newl;

  _BYTE   *loc  = (_BYTE *)NULL;
  _BYTE   *loc2 = (_BYTE *)NULL;

  size_t i,
         new_size;

  sigset_t set,
           old_set;

  if(initialised == TRUE)
     errno = 0;

  if(size == 0)
  {  if(ptr != (void *)NULL)
        (void)free(ptr);

     return((void *)NULL);
  }


  /*-------------------------------*/
  /* This case reduces to malloc() */
  /*-------------------------------*/

  if(ptr == (const void *)NULL && size > 0)
  {  loc = (void *)malloc(size);
     return(loc);
  }

  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_lock(&ckpt_malloc_mutex);
  #endif /* PTHREAD_SUPPORT */


  /*----------------------------------------------------*/
  /* We cannot allow signals to be handled in realloc() */
  /*----------------------------------------------------*/

  (void)sigfillset(&set);

  #ifdef PTHREAD_SUPPORT
  (void)pthread_sigmask(SIG_BLOCK,&set,&old_set);
  #else
  (void)sigprocmask(SIG_BLOCK,&set,&old_set);
  #endif /* PTHREAD_SUPPORT */

  loc = (_BYTE *)ptr;
  do_init();

  l = jmal(loc); 


  /*---------------------------------------------------------------------------*/
  /* Can we fit the reallocated memory segment into the current memory mapped  */
  /* region? If so lets do it! If we cannot do this we will need to allocate a */
  /* new (bigger) block, copy the contents of the currently allocated block to */
  /* this new block, and then unmap the current block from the process address */
  /* space.                                                                    */
  /*---------------------------------------------------------------------------*/

#ifdef JMALLOC_DEBUG
(void)fprintf(stderr,"REALLOC: block size 0x%010x (new size 0x%010x)\n",l->mmap_size - JMSZ - PTRSZ, size);
(void)fflush(stderr);
#endif /* JMALLOC_DEBUG */

  if (size < l->mmap_size - JMSZ - 2*PTSZ)
  {     double utilisation;

        new_size    = size;
        utilisation = 100.0*(double)new_size/(double)l->mmap_size;

#ifdef JMALLOC_DEBUG
(void)fprintf(stderr,"REALLOC: block (0x%010x, size 0x%010x) utilisation: %6.3f %%\n",l->size,size,utilisation);
(void)fflush(stderr);
#endif /* JMALLOC_DEBUG */

        if(l->mmap_size == GPAGESIZE || utilisation > munmap_utilisation) 
        {  l->size   = new_size;

           #ifdef PTHREAD_SUPPORT
           (void)pthread_mutex_unlock(&ckpt_malloc_mutex);
           #endif /* PTHREAD_SUPPORT */


           /*---------------------*/
           /* Restore signal mask */
           /*---------------------*/

           #ifdef PTHREAD_SUPPORT
           (void)pthread_sigmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);
           #else
           (void)sigprocmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);
           #endif /* PTHREAD_SUPPORT */

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
  else
     new_size = l->size;


  /*---------------------------------------------------------------------------*/
  /* We need to allocate a new block, copy the contents of the old block to it */
  /* and then unmap the old block from the process address space               */
  /* If the block is bigger than 64M the we will need to flush data to /tmp to */
  /* prevent an acute memory shortage occuring.                                */
  /*---------------------------------------------------------------------------*/

  loc2 = malloc(size);
  for (i = 0; i < new_size; i++)
      loc2[i] = loc[i];
  free(loc);

#ifdef JMALLOC_DEBUG
(void)fprintf(stderr,"REALLOC [0x%010x] returned 0x%010x (with loc1 free)\n",(unsigned long)ptr,(unsigned long)l);
(void)fflush(stderr);
#endif /* JMALLOC_DEBUG */


  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_unlock(&ckpt_malloc_mutex);
  #endif /* PTHREAD_SUPPORT */


  /*---------------------*/
  /* Restore signal mask */
  /*---------------------*/

  #ifdef PTHREAD_SUPPORT
  (void)pthread_sigmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);
  #else
  (void)sigprocmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);
  #endif /* PTHREAD_SUPPORT */

  return loc2;
}




/*---------------------------------------------------------------------------- 
    Calloc -- allocate space for nelem items of size elsize ... 
----------------------------------------------------------------------------*/ 

_PUBLIC void *calloc(const size_t nelem, const size_t elsize)

{
  int    *iptr = (int *) NULL;
  _BYTE  *ptr  = (void *)NULL;
  int sz;
  int i;

  sigset_t set,
           old_set;

  if(initialised == TRUE)
     errno = 0;


  /*---------------------------------------------------*/
  /* We cannot allow signals to be handled in calloc() */
  /*---------------------------------------------------*/

  (void)sigfillset(&set);

  #ifdef PTHREAD_SUPPORT
  (void)pthread_sigmask(SIG_BLOCK,&set,&old_set);
  #else
  (void)sigprocmask(SIG_BLOCK,&set,&old_set);
  #endif /* PTHREAD_SUPPORT */

  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_lock(&ckpt_malloc_mutex);
  #endif /* PTHREAD_SUPPORT */

  sz  = nelem*elsize;
  ptr = malloc(sz);
  if(ptr == (_BYTE *)NULL)
  {

     #ifdef PTHREAD_SUPPORT
     (void)pthread_mutex_unlock(&ckpt_malloc_mutex);
     #endif /* PTHREAD_SUPPORT */


     /*---------------------*/
     /* Restore signal mask */
     /*---------------------*/

     #ifdef PTHREAD_SUPPORT
     (void)pthread_sigmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);
     #else
     (void)sigprocmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);
     #endif /* PTHREAD_SUPPORT */

     return NULL;
  }

  iptr = (int *) ptr;
  for (i = 0; i < sz/sizeof(int); i++)
      iptr[i] = 0;

  for (i = i * sizeof(int); i < sz; i++)
      ptr[i] = 0;

  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_unlock(&ckpt_malloc_mutex);
  #endif /* PTHREAD_SUPPORT */


  /*---------------------*/
  /* Restore signal mask */
  /*---------------------*/

  #ifdef PTHREAD_SUPPORT
  (void)pthread_sigmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);
  #else
  (void)sigprocmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);
  #endif /* PTHREAD_SUPPORT */

  return ((void *)ptr);
}




/*---------------------------------------------------------------------------- 
    Set the quantum for flushing out low useage memory bubbles ... 
----------------------------------------------------------------------------*/ 

_PUBLIC int mallopt(const int cmd, const int value)
{

    /*---------------------------------------------------------------------*/
    /* Set the minimum percentage utilisation for a memory bubble. If the  */
    /* utilisation falls below this threshold, the bubble is unmapped from */ 
    /* the process address space.                                          */
    /*---------------------------------------------------------------------*/

    if(cmd == MUNMAP_THRESHOLD)
    {  if(value == (-1))
          return((int)munmap_utilisation);
       else
       {  munmap_utilisation = value;
          return(0);
       }
    }

    return(0);
}



/*---------------------------------------------------------------------------- 
    Print statistics on the number of memory bubbles currently mapped into
    the process address space ...
----------------------------------------------------------------------------*/ 

_PUBLIC void jmalloc_usage(const int show_bubbles, const FILE *stream)

{ Jmalloc next = (Jmalloc)NULL;

  int  cnt = 0;

  unsigned long mem_used = 0,
           mem_allocated = 0;

  double utilisation;


  #ifdef P3_SUPPORT
  /*----------------------------------*/
  /* Only the root thread can process */
  /* PSRP requests                    */
  /*----------------------------------*/

  if(pupsthread_is_root_thread() == FALSE)
     error("attempt by non root thread to perform global jmalloc operation");
  #endif /* P3_SUPPORT */

  errno = 0;
  if(stream == (const FILE *)NULL)
  {  errno = EINVAL;
     return;
  }

  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_lock(&ckpt_malloc_mutex);
  #endif /* THREAD_SUPPORT */

  do_init(); 

  (void)fprintf(stream,"\n\n    Memory bubbles mapped into process address space\n");
  (void)fprintf(stream,"    ================================================\n");

  if(show_bubbles == TRUE)
     (void)fprintf(stream,"\n");

  (void)fflush(stream);


  /*----------------------------------------------------------------*/
  /* In order to extract statistics we need to walk the linked list */
  /* of mapped memory bubbles.                                      */
  /*----------------------------------------------------------------*/

  next = memlist;
  do {    if(show_bubbles == TRUE)
          {  if(next->size > 0)
                utilisation = 100.0*(double)next->size/(double)next->mmap_size;
             else
                utilisation = 0.0;

             (void)fprintf(stream,"    %04d: bubble at 0x%010x virtual, size 0x%010x, used 0x%010x (utilisation %4.2f %%)\n",
                                                            cnt,(unsigned long)next + JMSZ,next->size,next->mmap_size,utilisation);
             (void)fflush(stream);
          }

          ++cnt;

          mem_used      += next->size;
          mem_allocated += next->mmap_size;

          next = next->flink;
      } while(next != start);

   utilisation = 100.0*(double)mem_used/(double)mem_allocated;
   (void)fprintf(stream,"\n\n    %04d bubbles currently mapped into process address space (utilisation theshold is %4.2f %%)\n",
                                                                                                         cnt,munmap_utilisation);
   (void)fprintf(stream,"    Total memory used 0x%010x bytes, allocated 0x%010x bytes (utilisation %4.2f %%)\n\n",
                                                                                    mem_used,mem_allocated,utilisation);
   (void)fflush(stream); 

  #ifdef PTHREAD_SUPPORT
  (void)pthread_mutex_unlock(&ckpt_malloc_mutex);
  #endif /* PTHREAD_SUPPORT */

}
