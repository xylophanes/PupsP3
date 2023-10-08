/*----------------------------------------------------------------------------
    Purpose: thread and DLL support for PUPS environment

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.00 
    Dated:   4th January 2022
    E-mail:  mao@tumblingdice.co.uk
----------------------------------------------------------------------------*/
/*-------------------------------------------------------------*/
/* If we have OpenMP support then we also have pthread support */
/* (OpenMP is built on pthreads for Linux distributions).      */
/*-------------------------------------------------------------*/

#if defined(_OPENMP) && !defined(PTHREAD_SUPPORT)
#define PTHREAD_SUPPORT
#endif /* PTHREAD_SUPPORT */

#ifdef PTHREAD_SUPPORT 

#include <me.h>
#include <utils.h>
#include <psrp.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <syscall.h>


/*----------------------------------------------------------------*/
/* Get signal mapping appropriate to OS and hardware architecture */
/*----------------------------------------------------------------*/

#define __DEFINE__
#if defined(I386) || defined (X86_64)
#include <sig.linux.x86.h>
#endif

#ifdef ARMV6L
#include <sig.linux.arm.h>
#endif /* ARMV6L */

#ifdef ARMV7L
#include <sig.linux.arm.h>
#endif /* ARMV7L */

#ifdef AARCH64
#include <sig.linux.arm.h>
#endif /* AARCH64 */

#undef   __NOT_LIB_SOURCE__
#include <tad.h>
#define  __NOT_LIB_SOURCE__
#undef __DEFINE__





/*----------------------------------------------------------------------------
    Get application information for slot manager ...
----------------------------------------------------------------------------*/
/*---------------------------*/
/* Slot information function */
/*---------------------------*/

_PRIVATE _ROOT_THREAD void tadlib_slot(int level)
{   (void)fprintf(stderr,"lib threadlib %s: [ANSI C]\n",TADLIB_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(C) 2002-2022 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"PUPS/P3 thread and DLL support library (built %s %s)\n",__TIME__,__DATE__);
       (void)fflush(stderr);
    }

    (void)fprintf(stderr,"\n");
    (void)fflush(stderr);
}


/*-------------------------------------------*/
/* Segment identification for thread library */
/*-------------------------------------------*/

#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT )() __attribute__ ((aligned(16))) = tadlib_slot;
#endif /* SLOT */




/*----------------------------------------------------------------------*/
/* Public variables exported by this library (note this is not the best */
/* way of exporting the thread table -- it would be better to have all  */
/* the manipulation of the thread table done by functions defined here  */
/* which are themselves _PUBLIC functions of this library)              */
/*----------------------------------------------------------------------*/
                                                                                 /*------------------------------*/
_PUBLIC  pthread_mutex_t ttab_mutex    = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP; /* Mutex for thread ttab access */
_PRIVATE int             ttab_index    = (-1);                                   /* Current index into ttab      */
_PUBLIC  int             n_threads     = 0;                                      /* Number of running threads    */
_PUBLIC  ttab_type       ttab[MAX_THREADS];                                      /* Thread table                 */
                                                                                 /*------------------------------*/


/*---------------------------------------------*/
/* Functions which are private to this modules */
/*---------------------------------------------*/


/*-------------------------------------------------------*/
/* Thread laucher - initialise thread as PUPS thread and */
/* call its payload function                             */
/*-------------------------------------------------------*/

_PROTOTYPE _PRIVATE _THREADSAFE int pupsthread_launch(void *);

// Initialise thread table
_PROTOTYPE _PRIVATE void pupsthread_init_ttab(void);



/*--------------------------------------------*/
/* Variables which are private to this moudle */
/*--------------------------------------------*/

_PRIVATE pthread_key_t   pupsthread_error_key                        = (-1);




/*----------------------------------------------------------------------------
    Set per thread error number ...
----------------------------------------------------------------------------*/

_PUBLIC _THREADSAFE void pupsthread_set_errno(const unsigned int thread_errno)

{   (void)pthread_setspecific(pupsthread_error_key, (const void *)&thread_errno);
}



/*----------------------------------------------------------------------------
    Handler for SIGTHREAD (asychronous thread termination) ...
----------------------------------------------------------------------------*/

_PRIVATE _THREADSAFE int att_handler(int signum)

{ 

    #ifdef TADLIB_DEBUG
    (void)fprintf(stderr,"THREAD TERMINATED\n");
    (void)fflush(stderr);
    #endif /* TADLIB_DEBUG */

    (void)pthread_exit((void *)NULL);
}



/*----------------------------------------------------------------------------
    Handler for SIGTHREADSTOP and SIGTHREADRESTART (stop and restarts
    threads) ...
----------------------------------------------------------------------------*/


_PRIVATE _THREADSAFE int att_pausecont_handler(int signum)

{   int ret_signum;    // Return signal from sigwait()
    sigset_t sigmask;  // Set of signals that sigwait() is waiting for


    #ifdef TADLIB_DEBUG
    (void)fprintf(stderr,"STARTSTOP HANDLER\n");
    (void)fflush(stderr);
    #endif /* TADLIB_DEBUG */


    /*------------------------*/
    /* Spurius restart signal */
    /*------------------------*/

    if(signum == SIGTHREADRESTART)
    {

      #ifdef TADLIB_DEBUG
      (void)fprintf(stderr,"SPURIOUS SIGTHREADRESTART\n");
      (void)fflush(stderr);
      #endif /* TADLIB_DEBUG */

       return(0);
    }

    /*----------------------------------*/
    /* Set up signal mask for sigwait() */
    /*----------------------------------*/

    (void)sigemptyset(&sigmask);

    (void)sigaddset  (&sigmask,SIGTHREAD);
    (void)sigaddset  (&sigmask,SIGTHREADRESTART);

    #ifdef TADLIB_DEBUG
    (void)fprintf(stderr,"THREAD STOP\n");
    (void)fflush(stderr);
    #endif /* TADLIB_DEBUG */


    /*-----------------------------------------*/
    /* Wait for signal in mask to be delivered */
    /*-----------------------------------------*/

    (void)sigwait(&sigmask,&ret_signum);


    #ifdef TADLIB_DEBUG
    if(ret_signum == SIGTHREAD)
       (void)fprintf(stderr,"GOT SIGNAL SIGTHREAD\n");
    else if(ret_signum == SIGTHREADRESTART)
       (void)fprintf(stderr,"GOT SIGNAL SIGTHREADRESTART\n");
    (void)fflush(stderr);
    #endif /* TADLIB_DEBUG */


    /*---------------------------------------------------*/
    /* SIGTHREAD received - asynchronously cancel thread */
    /*---------------------------------------------------*/

    if(ret_signum == SIGTHREAD)
    { 

       #ifdef TADLIB_DEBUG
       (void)fprintf(stderr,"THREAD TERMINATED\n");
       (void)fflush(stderr);
       #endif /* TADLIB_DEBUG */

       (void)pthread_exit((void *)NULL);
    }

    #ifdef TADLIB_DEBUG
    (void)fprintf(stderr,"THREAD RESTART\n");
    (void)fflush(stderr);
    #endif /* TADLIB_DEBUG */

    return(0);
}




/*----------------------------------------------------------------------------
    Get per thread error number ...
----------------------------------------------------------------------------*/

_PUBLIC _THREADSAFE int pupsthread_get_errno(void)

{   int *ret = (int *)NULL;

    ret = (int *)pthread_getspecific(pupsthread_error_key);
    return((*ret));
}




/*-----------------------------------------------------------------------------
    Is this the root thread (e.g. not a POSIX thread or part of an OMP
    gang) ...
-----------------------------------------------------------------------------*/

_PUBLIC _THREADSAFE _BOOLEAN pupsthread_is_root_thread(void)

{   if(appl_root_tid == (-1) || appl_root_tid == syscall(SYS_gettid))
    {  pups_set_errno(OK);
       return(TRUE);
    }

    pups_set_errno(EACCES);
    return(FALSE);
}




/*----------------------------------------------------------------------------
   Translate function name to tid ...
----------------------------------------------------------------------------*/

_PUBLIC pthread_t pupsthread_tfuncname2tid(const char *tfuncname)

{   int       i;
    pthread_t tid;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(tfuncname == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return((pthread_t)NULL);
    }


    /*------------------------------------------------------*/
    /* Make sure we don't clash with (exiting) thread which */
    /* will modify ttab                                     */
    /*------------------------------------------------------*/

    (void)pthread_mutex_lock(&ttab_mutex);
    for(i=0; i<MAX_THREADS; ++i)
    {  if(strcmp(ttab[i].tfuncname,tfuncname) == 0)
       {  tid = ttab[i].tid;
          (void)pthread_mutex_unlock(&ttab_mutex);
 
          pups_set_errno(OK);
          return(tid);
       }
    }
    (void)pthread_mutex_unlock(&ttab_mutex);

    pups_set_errno(ESRCH);
    return((pthread_t)NULL);
}




/*-------------------------------------------------------------------------------
    Translate tid to function name ...
--------------------------------------------------------------------------------*/

_PUBLIC int pupsthread_tid2tfuncname(const pthread_t tid, char *tfuncname)

{   int  i;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(tfuncname == (char *)NULL || tid == (const pthread_t)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*------------------------------------------------------*/
    /* Make sure we don't clash with (exiting) thread which */
    /* will modify ttab                                     */
    /*------------------------------------------------------*/

    (void)pthread_mutex_lock(&ttab_mutex);
    for(i=0; i<MAX_THREADS; ++i)
    {  if(ttab[i].tid == tid)
       {  (void)strlcpy(tfuncname,ttab[i].tfuncname,SSIZE);
          (void)pthread_mutex_unlock(&ttab_mutex);

          pups_set_errno(OK);
          return(0);
       }
    }
    (void)pthread_mutex_unlock(&ttab_mutex);

    pups_set_errno(ESRCH);
    return(-1);
}




/*-----------------------------------------------------------------------------
    Translate function name to ttab index (nid) ...
-----------------------------------------------------------------------------*/

_PUBLIC int pupsthread_tfuncname2nid(const char *tfuncname)

{   int i;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(tfuncname == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*------------------------------------------------------*/
    /* Make sure we don't clash with (exiting) thread which */
    /* will modify ttab                                     */
    /*------------------------------------------------------*/

    (void)pthread_mutex_lock(&ttab_mutex);
    for(i=0; i<MAX_THREADS; ++i)
    {  if(strcmp(ttab[i].tfuncname,tfuncname) == 0)
       {  (void)pthread_mutex_unlock(&ttab_mutex);

          pups_set_errno(OK);
          return(i);
       }
    }
    (void)pthread_mutex_unlock(&ttab_mutex);

    pups_set_errno(ESRCH);
    return(-1);
}




/*-----------------------------------------------------------------------------
   Translate ttab index (nid) to function name ...
-----------------------------------------------------------------------------*/

_PUBLIC int pupsthread_nid2tfuncname(const unsigned int nid, char *tfuncname)

{   int i;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(tfuncname == (char *)NULL || nid > MAX_THREADS)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    /*------------------------------------------------------*/
    /* Make sure we don't clash with (exiting) thread which */
    /* will modify ttab                                     */
    /*------------------------------------------------------*/

    (void)pthread_mutex_lock(&ttab_mutex);
    (void)strlcpy(tfuncname,ttab[nid].tfuncname,SSIZE);
    (void)pthread_mutex_unlock(&ttab_mutex);

    pups_set_errno(OK);
    return(0);
}




/*-----------------------------------------------------------------------------
    Translate tid to ttab index (nid) ...
-----------------------------------------------------------------------------*/

_PUBLIC int pupsthread_tid2nid(const pthread_t tid)

{   int i;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(tid == (const pthread_t)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*------------------------------------------------------*/
    /* Make sure we don't clash with (exiting) thread which */
    /* will modify ttab                                     */
    /*------------------------------------------------------*/

    (void)pthread_mutex_lock(&ttab_mutex);
    for(i=0; i<MAX_THREADS; ++i)
    {  if(ttab[i].tid == tid)
       {  (void)pthread_mutex_unlock(&ttab_mutex);

          pups_set_errno(OK);
          return(i);
       }
    }
    (void)pthread_mutex_unlock(&ttab_mutex);

    pups_set_errno(ESRCH);
    return(-1);
}




/*-----------------------------------------------------------------------------
   Translate ttab index (nid) to tid ...
-----------------------------------------------------------------------------*/

_PUBLIC pthread_t pupsthread_nid2tid(const unsigned int nid)

{   int       i;
    pthread_t tid = (pthread_t)NULL;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(nid > MAX_THREADS)
    {  pups_set_errno(EINVAL);
       return((pthread_t)NULL);
    }


    /*------------------------------------------------------*/
    /* Make sure we don't clash with (exiting) thread which */
    /* will modify ttab                                     */
    /*------------------------------------------------------*/

    (void)pthread_mutex_lock(&ttab_mutex);
    tid = ttab[nid].tid;
    (void)pthread_mutex_unlock(&ttab_mutex);

    pups_set_errno(OK);
    return(tid);
}




/*-----------------------------------------------------------------------------
    Translate (LWP) tpid to ttab index (nid) ...
-----------------------------------------------------------------------------*/

_PUBLIC int pupsthread_tpid2nid(const unsigned int tpid)

{   int i;


    /*------------------------------------------------------*/
    /* Make sure we don't clash with (exiting) thread which */
    /* will modify ttab                                     */
    /*------------------------------------------------------*/

    (void)pthread_mutex_lock(&ttab_mutex);
    for(i=0; i<MAX_THREADS; ++i)
    {  if(ttab[i].tpid == tpid)
       {  (void)pthread_mutex_unlock(&ttab_mutex);

          pups_set_errno(OK);
          return(i);
       }
    }
    (void)pthread_mutex_unlock(&ttab_mutex);

    pups_set_errno(ESRCH);
    return(-1);
}




/*-----------------------------------------------------------------------------
    Translate ttab index (nid) to (LWP) tpid ...
-----------------------------------------------------------------------------*/

_PUBLIC int pupsthread_nid2tpid(const unsigned int nid)

{   int i,
        tpid;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(nid > MAX_THREADS)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*------------------------------------------------------*/
    /* Make sure we don't clash with (exiting) thread which */
    /* will modify ttab                                     */
    /*------------------------------------------------------*/

    (void)pthread_mutex_lock(&ttab_mutex);
    tpid = ttab[nid].tpid;
    (void)pthread_mutex_unlock(&ttab_mutex);

    pups_set_errno(OK);
    return(tpid);
}




/*--------------------------------------------------------------------------------
    Get index to next free slot in thread table ...
--------------------------------------------------------------------------------*/

_PRIVATE int pupsthread_get_slot(void)

{   int i;


    /*------------------------------------------------------*/
    /* We need to lock this section as we can only have one */
    /* thread manipulating ttab at any one time             */
    /*------------------------------------------------------*/

    (void)pthread_mutex_lock(&ttab_mutex);

    for(i=0; i<MAX_THREADS; ++i)
    {  if(ttab[i].tid == (pthread_t)NULL)
       {  if(i == n_threads)
             n_threads = i + 1;

          (void)pthread_mutex_unlock(&ttab_mutex);
          return(i);
       }
    } 

    (void)pthread_mutex_unlock(&ttab_mutex);
    return(-1);
}




/*---------------------------------------------------------------------------------
    Find thread table entry given tid ...
---------------------------------------------------------------------------------*/

_PRIVATE int pupsthread_find_slot(pthread_t tid)

{  int i;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(tid == (const pthread_t)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*------------------------------------------------------*/
    /* We need to lock this section as we can only have one */
    /* thread manipulating ttab at any one time             */
    /*------------------------------------------------------*/

    (void)pthread_mutex_lock(&ttab_mutex);

    for(i=0; i<MAX_THREADS; ++i)
    {  if(ttab[i].tid == tid)
       {  (void)pthread_mutex_unlock(&ttab_mutex);
          return(i);
       }
    }

    (void)pthread_mutex_unlock(&ttab_mutex);
    return(-1);
}




/*---------------------------------------------------------------------------------
    Clear a thread table entry ...
---------------------------------------------------------------------------------*/

_PRIVATE int pupsthread_clear_slot(unsigned int t_index)

{   char tio_name[SSIZE] = ""; 

    if(t_index >= MAX_THREADS)
       return(-1);


    /*------------------------------------------------------*/
    /* We need to lock this section as we can only have one */
    /* thread manipulating ttab at any one time             */
    /*------------------------------------------------------*/

    (void)pthread_mutex_lock(&ttab_mutex);

    ttab[t_index].tid        = (pthread_t)NULL;
    ttab[t_index].tpid       = (-1);
    ttab[t_index].tfunc      = (void *)NULL;
    ttab[t_index].targs      = (void *)NULL;
    (void)strlcpy(ttab[t_index].tfuncname,"none",SSIZE);
    (void)strlcpy(ttab[t_index].state,    "",SSIZE);

    --n_threads;

    (void)pthread_mutex_unlock(&ttab_mutex);
    return(0);
}



 
/*--------------------------------------------------------------------------------------
    Show ttab entry for active thread ...
--------------------------------------------------------------------------------------*/

_PUBLIC int pupsthread_show_threadinfo(const FILE *stream, const unsigned int nid)

{    

    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(stream == (const FILE *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*---------------------------*/
    /* Range error (ttab bounds) */
    /*---------------------------*/

    if(nid > MAX_THREADS)
    {  pups_set_errno(ERANGE);
       return(-1);
    }


    /*-------------------------------------------------*/
    /* Check to see thread corresponding to nid exists */
    /*-------------------------------------------------*/

    (void)pthread_mutex_lock(&ttab_mutex);


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[pups_show_threadinfo] attempt by non root thread to perform PUPS/P3 global thread operation");


    /*-----------------------*/
    /* Thread does not exist */
    /*-----------------------*/

    else if(ttab[nid].tid == (pthread_t)NULL)
    {   (void)pthread_mutex_unlock(&ttab_mutex);

       pups_set_errno(EINVAL);
       return(-1);
    }


    (void)fprintf(stream,"\n    Thread: %04d\n",nid);
    (void)fprintf(stream,"    ============\n\n");
    (void)fflush(stream);

    (void)fprintf(stream,"    tpid [LWP]      :  %04d\n",                         ttab[nid].tpid);
    (void)fprintf(stream,"    tid             :  %016lx\n",                       (unsigned long)ttab[nid].tid);
    (void)fprintf(stream,"    payload function:  \"%-32s\" (at %016lx virtual)\n",ttab[nid].tfuncname,(unsigned long int)ttab[nid].tid);
    (void)fprintf(stream,"    argument pointer:  %016lx\n",                       (unsigned long int)ttab[nid].targs);
    (void)fprintf(stream,"    state           :  %-16s\n\n",                       ttab[nid].state);
    
    (void)pthread_mutex_unlock(&ttab_mutex);

    pups_set_errno(OK);
    return(0);
}




/*--------------------------------------------------------------------------------------
    Show ttab entries for PSRP servers active threads ...
--------------------------------------------------------------------------------------*/

_PUBLIC void pupsthread_show_ttab(const FILE *stream)

{   int i,
        threads = 0;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(stream == (FILE *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[pups_show_ttab] attempt by non root thread to perform PUPS/P3 global thread operation");


    /*------------------------------------------------------*/
    /* Make sure we don't clash with (exiting) thread which */
    /* will modify ttab                                     */
    /*------------------------------------------------------*/

    (void)pthread_mutex_lock(&ttab_mutex);

    (void)fprintf(stream,"\n    Active threads\n",appl_name,getpid(),appl_host);
    (void)fprintf(stream,"    ==============\n\n");
    (void)fflush(stream);

    for(i=0; i<MAX_THREADS; ++i)
    {  if(ttab[i].tid != (pthread_t)NULL)
       {  int  d_index;
          char psrp_object_info[SSIZE] = "";

          if((d_index = lookup_psrp_object_by_handle(ttab[i].tfunc)) != (-1))
          {  char typestr[246] = "";

             if(psrp_object_list[d_index].object_type == PSRP_STATIC_FUNCTION)
                (void)strlcpy(typestr,"static psrp function",SSIZE);
             else if(psrp_object_list[d_index].object_type == PSRP_DYNAMIC_FUNCTION)
                (void)strlcpy(typestr,"dynamic psrp function",SSIZE);
             else
                (void)strlcpy(typestr,"psrp object",SSIZE);

             (void)snprintf(psrp_object_info,SSIZE," [%-16s \"%-16s\"]",typestr,psrp_object_list[d_index].object_tag[0]);
          }
          (void)fprintf(stream,"    %04d: tname: \"%-32s\" (payload \"%-32s\", tid %016lx (LWP %04d) at %016lx virtual) state: %-16s\n",i,
                                                                                                                        ttab[i].tfuncname,
                                                                                                                         psrp_object_info,
                                                                                                           (unsigned long int)ttab[i].tid,
                                                                                                                             ttab[i].tpid,
                                                                                                         (unsigned long int)ttab[i].tfunc,
                                                                                                                            ttab[i].state);
          (void)fflush(stream);

          ++threads;
       }
    }

    if(threads == 0)
       (void)fprintf(stream,"    no active threads [other than root thread] (%04d slots available)\n\n",MAX_THREADS);
    else if(threads == 1)
       (void)fprintf(stream,"\n\n    %04d active thread (%04d slots available)\n\n",1,MAX_THREADS - 1);
    else
       (void)fprintf(stream,"\n\n    %04d active threads (%04d slots available\n\n",threads,MAX_THREADS - threads);
    (void)fflush(stream);

    (void)pthread_mutex_unlock(&ttab_mutex);
    pups_set_errno(OK);
}




/*--------------------------------------------------------------------------------
    Initialise thread table (note this function IS threadsafe as it gets run
    only once by the root thread before any other threads are created) ...
--------------------------------------------------------------------------------*/

_PRIVATE void pupsthread_init_ttab(void)
{   int i;

    n_threads = 0;
    for(i=0; i<MAX_THREADS; ++i)
    {  ttab[i].tid        = (pthread_t)NULL;
       ttab[i].tpid       = (-1);
       ttab[i].tfunc      = (void *)NULL;
       ttab[i].targs      = (void *)NULL;
       (void)strlcpy(ttab[i].tfuncname,"none",SSIZE);
       (void)strlcpy(ttab[i].state,    "",SSIZE);
    }
}




/*----------------------------------------------------------------------------
    Thread initialisation routine called by main in a PUPS application
    which is threaded ...
----------------------------------------------------------------------------*/

_PUBLIC void pupsthread_init(void)

{   _IMMORTAL _BOOLEAN once = FALSE;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[pupsthread_init] attempt by non root thread to perform PUPS/P3 global thread operation");

    if(once == FALSE)
    {  pupsthread_init_ttab();
       once = TRUE;
    }

    pups_set_errno(OK);
}




/*----------------------------------------------------------------------------
    Create a new thread (setting up the threads environment). The first
    argument is the address of the payload function ...
----------------------------------------------------------------------------*/

_PUBLIC pthread_t pupsthread_create(const char *tfuncname,  // Payload function to be run by thread
                                    const void     *tfunc,  // Thread payload function
                                    const void     *targs)  // Argument list (string)

{   int ret,
        t_index;

    sigset_t       sigmask;
    pthread_t      tid;
    pthread_attr_t attr;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(tfuncname == (const char *)NULL || tfunc == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return((pthread_t)NULL);
    }

    if((t_index = pupsthread_get_slot()) == (-1))
    {  pups_set_errno(ENOSPC);
       return((pthread_t)NULL);
    }


    /*-------------------------------------------------------------------*/
    /* Set up (default) attributes for the thread we are about to create */
    /*-------------------------------------------------------------------*/

    (void)pthread_attr_init(&attr);
    (void)pthread_attr_setstacksize(&attr,DEFAULT_STACKSIZE);


    /*-------------------------------------------------------------------*/
    /* Set up mask for signals which will be blocked in all threads      */
    /* other than the root thread                                        */
    /*-------------------------------------------------------------------*/

    (void)sigfillset(&sigmask);

    (void)sigdelset (&sigmask,SIGTHREAD);         // Used by pupsthread_cancel()
    (void)sigdelset (&sigmask,SIGTHREADSTOP);     // Used by pupsthread_pause()
    (void)sigdelset (&sigmask,SIGTHREADRESTART);  // Used by pupsthread_cont()

    (void)pthread_sigmask(SIG_SETMASK,&sigmask,(sigset_t *)NULL);


    /*-------------------------------------*/
    /* Handler for asychronous thread exit */
    /*-------------------------------------*/

    (void)pups_sighandle(SIGTHREAD,       "thread_handler",          (void *)att_handler,          &sigmask);
    (void)pups_sighandle(SIGTHREADSTOP,   "thread_pausecont_handler",(void *)att_pausecont_handler,&sigmask);
    (void)pups_sighandle(SIGTHREADRESTART,"thread_pausecont_handler",(void *)att_pausecont_handler,&sigmask);


    /*----------------------------*/
    /* Set up slot for new thread */
    /*----------------------------*/

    (void)pthread_mutex_lock(&ttab_mutex);

    ttab[t_index].tfunc    = tfunc;
    ttab[t_index].targs    = targs;
    ttab[t_index].sigmask  = sigmask;

    (void)strlcpy(ttab[t_index].tfuncname,tfuncname,SSIZE);
    (void)strlcpy(ttab[t_index].state,"run",SSIZE);


    /*-------------------------------------------------------------*/
    /* We have finished accessing the thread table - release mutex */
    /*-------------------------------------------------------------*/

    (void)pthread_mutex_unlock(&ttab_mutex);

    /*-------------------------------------------------------*/
    /* Create the thread (returning NULL if we get an error) */
    /*-------------------------------------------------------*/

    if((ret = pthread_create(&tid,&attr,(void *)pupsthread_launch,(void *)&t_index)) == (-1))
    {  

       /*-------------------------------------------------------*/
       /* Thread creation has failed - clear thread table entry */ 
       /*-------------------------------------------------------*/

       (void)pupsthread_clear_slot(t_index);


       /*---------------------------*/
       /* Set error for this thread */
       /*---------------------------*/

       pups_set_errno(ENOEXEC);
       return((pthread_t)NULL);
    }
    else
       pups_usleep(10000);

    pups_set_errno(OK);
    return(tid);
}




/*----------------------------------------------------------------------------
    Thread launcher - perform thread initilisation and then start payload
    function ...
----------------------------------------------------------------------------*/

_PRIVATE _THREADSAFE int pupsthread_launch(void *arg)

{   int t_index,
        (*tfunc)(void *);

    void *targs = (void *)NULL;

    if(arg == (void *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*------------------------------------*/
    /* Payload function is first argument */
    /*------------------------------------*/

    t_index            = *((int *)arg);


    /*---------------------------------------------*/
    /* Fill reamining fields in thread table entry */
    /*---------------------------------------------*/

    (void)pthread_mutex_lock(&ttab_mutex);

    ttab[t_index].tid     = pthread_self();
    ttab[t_index].tpid    = syscall(SYS_gettid);
    tfunc                 = ttab[t_index].tfunc;
    targs                 = ttab[t_index].targs;

    (void)pthread_mutex_unlock(&ttab_mutex);

    if((tfunc)(targs) == (-1))
    {  

       /*--------------------------------*/
       /* Thread has aborted with errors */
       /*--------------------------------*/

       pups_set_errno(ENOEXEC);
       (void)pthread_exit((void *)NULL);
    }

    pups_set_errno(OK);
    (void)pthread_exit((void *)NULL);
}





/*----------------------------------------------------------------------------
    Pause thread ...
----------------------------------------------------------------------------*/

_PUBLIC int pupsthread_pause(const pthread_t tid)

{   int t_index;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(tid == (const pthread_t)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*-----------------------------*/
    /* We cannot pause root thread */
    /*-----------------------------*/

    if(tid == appl_root_tid)
    {  pups_set_errno(EPERM);
       return(-1);
    }


    /*----------------------------------------*/
    /* Get index into table of active threads */
    /*----------------------------------------*/

    (void)pthread_mutex_lock(&ttab_mutex);
    if((t_index = pupsthread_find_slot(tid)) == (-1))
    {  (void)pthread_mutex_unlock(&ttab_mutex);

       pups_set_errno(EINVAL);
       return(-1);
    }


    /*------------------------------------------*/
    /* Check to see if thread is already paused */
    /*------------------------------------------*/

    if(strcmp(ttab[t_index].state,"pause") == 0)
    {  (void)pthread_mutex_unlock(&ttab_mutex);

       pups_set_errno(EINVAL);
       return(-1);
    }

    (void)pthread_kill(tid,SIGTHREADSTOP);
    (void)strlcpy(ttab[t_index].state,"pause",SSIZE);

    (void)pthread_mutex_unlock(&ttab_mutex);

    pups_set_errno(OK);
    return(0);
}




/*----------------------------------------------------------------------------
    Restart thread ...
----------------------------------------------------------------------------*/

_PUBLIC int pupsthread_cont(pthread_t tid)

{   int t_index;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(tid == (const pthread_t)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*-----------------------------*/
    /* We cannot pause root thread */
    /*-----------------------------*/

    if(tid == appl_root_tid)
    {  pups_set_errno(EPERM);
       return(-1);
    }


    /*----------------------------------------*/
    /* Get index into table of active threads */
    /*----------------------------------------*/

    (void)pthread_mutex_lock(&ttab_mutex);
    if((t_index = pupsthread_find_slot(tid)) == (-1))
    {  (void)pthread_mutex_unlock(&ttab_mutex);

       pups_set_errno(EINVAL);
       return(-1);
    }


    /*-------------------------------------------*/
    /* Check to see if thread is already running */
    /*-------------------------------------------*/

    if(strcmp(ttab[t_index].state,"run") == 0)
    {  (void)pthread_mutex_unlock(&ttab_mutex);

       pups_set_errno(EINVAL);
       return(-1);
    }


    /*-------------------------------*/
    /* Thread is paused - restart it */
    /*-------------------------------*/

    else
    {  (void)pthread_kill(tid,SIGTHREADRESTART);
       (void)strlcpy(ttab[t_index].state,"run",SSIZE);
    }

    (void)pthread_mutex_unlock(&ttab_mutex);

    pups_set_errno(OK);
    return(0);
}





/*----------------------------------------------------------------------------
    Cancel a thread -- in the PUPS environment cancelling of threads
    is asychronous so the thread terminates immediately ...
----------------------------------------------------------------------------*/

_PUBLIC int pupsthread_cancel(const pthread_t tid)

{   int  t_index;
    char tio_name[SSIZE] = "";


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(tid == (const pthread_t)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*------------------------------*/
    /* We cannot cancel root thread */
    /*------------------------------*/

    if(tid == appl_root_tid)
    {  pups_set_errno(EPERM);
       return(-1);
    }


    /*----------------------------------------*/
    /* Get index into table of active threads */
    /*----------------------------------------*/

    (void)pthread_mutex_lock(&ttab_mutex);
    if((t_index = pupsthread_find_slot(tid)) == (-1))
    {  (void)pthread_mutex_unlock(&ttab_mutex);

       pups_set_errno(EINVAL);
       return(-1);
    }


    /*-------------------------------------*/
    /* Terminate the thread asynchronously */
    /*-------------------------------------*/

    (void)pthread_kill(tid,SIGTHREAD);


    /*-----------------------------------*/
    /* Clear table entry for this thread */
    /*-----------------------------------*/

    (void)pupsthread_clear_slot(t_index);
    (void)pthread_mutex_unlock(&ttab_mutex);

    pups_set_errno(OK);
    return(0);
}




/*-----------------------------------------------------------------------------
    Routine to cancel all currently running threads except caller ...
-----------------------------------------------------------------------------*/

_PUBLIC int pupsthread_cancel_all_other_threads(void)

{   int i,
        mytid,
        my_index;


    /*--------------------------------*/
    /* Caller must be the root thread */
    /*--------------------------------*/

    mytid = syscall(SYS_gettid);
    if(mytid == appl_root_tid)
    {  pups_set_errno(EPERM);
       return(-1);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[pupsthread_cancel_all_other_threads] attempt by non root thread to perform PUPS/P3 global thread operation");

    (void)pthread_mutex_lock(&ttab_mutex);
    for(i=0; i<n_threads; ++i)
    {  if(ttab[i].tid == pthread_self())
          my_index = i;
       else if(ttab[i].tid != (pthread_t)NULL)
          (void)pupsthread_cancel(ttab[i].tid);
    }      
    (void)pthread_mutex_unlock(&ttab_mutex);

    pups_set_errno(OK);
    return(0);
}




/*----------------------------------------------------------------------------
    Terminate a thread deleting its thread table entry ...
----------------------------------------------------------------------------*/

_PUBLIC _THREADSAFE void pupsthread_exit(void *retval)

{   int       t_index;
    pthread_t tid;

    tid = pthread_self();

    pups_set_errno(OK);

    if((t_index = pupsthread_tid2nid(tid)) == (-1))
    {  pups_set_errno(EINVAL);
       return;
    }


    /*-----------------------------------*/
    /* Clear table entry for this thread */
    /*-----------------------------------*/

    (void)pupsthread_clear_slot(t_index);


    /*------------------------------*/
    /* Finally terminate the thread */
    /*------------------------------*/

    pthread_exit(retval);
}

#endif /* PTHREAD_SUPPORT */
