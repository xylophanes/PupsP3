/*-------------------------------------------------------------------------
    Purpose: header for multithread DLL support library.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.02 
    Dated:   27th February 2024
    E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------------*/


#ifndef TAD_H
#define TAD_H


/*------------------*/
/* Keep c2man happy */
/*------------------*/

#include <c2man.h>

#ifndef __C2MAN__
#include <me.h>
#include <utils.h>
#endif /* __C2MAN__ */


/***********/
/* Version */
/***********/

#define TADLIB_VERSION            "2.02"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE                     2048


/*----------------------------*/
/* Max mutexes on PUPS server */
/*----------------------------*/

#define MAX_MUTEXES               1024 


/*----------------------------*/
/* Max threads on PUPS server */
/*----------------------------*/

#define MAX_THREADS               1024 


/*------------------------*/
/* Max mutexes per thread */
/*------------------------*/

#define MAX_MUTEXES_PER_THREAD    64 


/*---------------------------------------*/
/* Default stack size (bytes per thread) */
/*---------------------------------------*/

#define DEFAULT_STACKSIZE         32768




/*------------------------*/
/* Macros defined by pc2c */
/*------------------------*/


#ifndef PTHREAD_SUPPORT
/*------------------------------------------------------------------*/
/* Dummy macro definitions -- used if we find ourselves with a pc2c */
/* translated code and without POSIX thread support                 */
/*------------------------------------------------------------------*/

#define _LOCK_THREAD_MUTEX(mutex)              // Lock mutex
#define _UNLOCK_THREAD_MUTEX(mutex)            // Unlock mutex
#define _DEFINE_FMUTEX(mutex)                  // Define mutex for current function
#define _UNDEFINE_FMUTEX(mutex)                // Undefine mutex
#define _USE_FMUTEX(mutex)                     // Use mutex with current function
#define _INIT_PUPS_THREADS                     // Initialise PUPS threads
#define _EXEC_THREAD_ROOT_THREAD_ONLY          // Excutable by root thread only
#define _ROOT_THREAD                           // Root thread function
#define _THREADSAFE                            // Thread safe function identifier 
#define PUPSTHREAD_MUTEX_LOCK  (-1)            // Wait to acquire mutex 


#else

#ifndef __C2MAN__
#include <pthread.h>
#endif /* _+_C2MAN__ */

/*---------------------------------------------------*/
/* Macros for use with pc2c and POSIX thread support */
/*---------------------------------------------------*/

#define _LOCK_THREAD_MUTEX(mutex)              pthread_mutex_lock(mutex);
#define _UNLOCK_THREAD_MUTEX(mutex)            pthread_mutex_unlock(mutex);
#define _DEFINE_FMUTEX(mutex)                  pthread_mutex_t mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#define _UNDEFINE_FMUTEX(mutex)
#define _USE_FMUTEX(mutex)
#define _INIT_PUPS_THREADS                     pupsthread_std_init();
#define _EXEC_ROOTHREAD_ONLY                   if(pthread_rootthread() == FALSE) return;
#define _ROOT_THREAD 
#define _THREADSAFE
#define PUPSTHREAD_MUTEX_LOCK  (-1)            // Wait to acquire mutex


/*---------------------------------*/
/* Data structure for thread table */
/*---------------------------------*/

typedef struct {   pthread_t tid;              // Thread identifier
                   int32_t   tpid;             // LWP processor identifier
                   char      tfuncname[SSIZE]; // Name of threaded function
                   char      state[SSIZE];     // Thread state (run or stop)
                   void      *tfunc;           // Address of threaded function
                   void      *targs;           // Arguments of threaded function
                   sigset_t  sigmask;          // Thread signal mask
               } ttab_type;


/*-------------------------------------------------------------*/
/* Variables exported by the multithreaded DLL support library */ 
/*-------------------------------------------------------------*/

#ifdef __NOT_LIB_SOURCE__


/*----------------------------------------------------------------------*/
/* Public variables exported by this library (note this is not the best */
/* way of exporting the thread table -- it would be better to have all  */
/* the manipulation of the thread table done by functions defined here  */
/* which are themselves _PUBLIC functions of this library)              */
/*----------------------------------------------------------------------*/

_EXPORT pthread_mutex_t test_mutex;
_EXPORT pthread_mutex_t ttab_mutex;           // Mutex for thread tab access
_EXPORT int32_t         n_threads;            // Number of running threads
_EXPORT ttab_type       ttab[MAX_THREADS];    // Thread table 

#endif /*  __NOT_LIB_SOURCE__ */


/*------------------------------------*/
/* PUPS thread manipulation functions */
/*------------------------------------*/

// Is caller the root thread?
_PROTOTYPE _EXPORT _THREADSAFE _BOOLEAN pupsthread_is_root_thread(void);

// Set thread specific errno
_PROTOTYPE _EXPORT _THREADSAFE void pupsthread_set_errno(const uint32_t);

// Get thread specific errno
_PROTOTYPE _EXPORT _THREADSAFE int32_t pupsthread_get_errno(void);

// Lock mutex
_PROTOTYPE _EXPORT int32_t pupsthread_mutex_lock(const pthread_mutex_t *);

// Unlock mutex
_PROTOTYPE _EXPORT int32_t pupsthread_mutex_unlock(const pthread_mutex_t *);

// Get tid (thread identifier) associated with given function name
_PROTOTYPE _EXPORT pthread_t pupsthread_tfuncname2tid(const char *);

// Get name of function associated with tid
_PROTOTYPE _EXPORT int32_t pupsthread_tid2tfuncname(const pthread_t, char *);

// Get nid (thread number) associated with given function name
_PROTOTYPE _EXPORT int32_t pupsthread_tfuncname2nid(const char *);

// Get function name associated with given thread number (nid)
_PROTOTYPE _EXPORT int32_t pupsthread_nid2tfuncname(const uint32_t, char *);

// Get thread number (nid) associated with given thread handle
_PROTOTYPE _EXPORT int32_t pupsthread_tid2nid(const pthread_t);

// Translate (LWP) tpid to ttab index (nid)
_PROTOTYPE _EXPORT int32_t pupsthread_tpid2nid(const pid_t);

// Translate ttab index (nid) to (LWP) tpid
_PROTOTYPE _EXPORT pid_t pupsthread_nid2tpid(const uint32_t);

// Get thread handle associated with given thread number (nid)
_PROTOTYPE _EXPORT pthread_t pupsthread_nid2tid(const uint32_t);

// Show single entry in thread table [root thread]
_PROTOTYPE _EXPORT int32_t pupsthread_show_threadinfo(const FILE *, const uint32_t);

// Show contents of thread table [root thread]
_PROTOTYPE _EXPORT void pupsthread_show_ttab(const FILE *);

// Initialised PUPS threads [root thread]
_PROTOTYPE _EXPORT void pupsthread_init(void);

// Create a PUPS thread
_PROTOTYPE _EXPORT pthread_t pupsthread_create(const char *, const void *, const void *);

// Cancel PUPS thread
_PROTOTYPE _EXPORT int32_t pupsthread_cancel(const pthread_t);

// Stop PUPS thread
_PROTOTYPE _EXPORT int32_t pupsthread_pause(const pthread_t);

// Restart PUPS thread
_PROTOTYPE _EXPORT int32_t pupsthread_cont(const pthread_t);

// Cancel thread
_PROTOTYPE _EXPORT int32_t pupsthread_cancel(const pthread_t);

// Cancel all PUPS threads (except for calling root thread)
_PROTOTYPE _EXPORT int32_t pupsthread_cancel_all_other_threads(void);

// Exit PUPS thread (terminating it)
_PROTOTYPE _EXPORT _THREADSAFE void pupsthread_exit(const void *);

// Release mutexes
_PROTOTYPE _EXPORT void pupsthread_unlock_mutexes(void);


#endif /* PTHREAD_SUPPORT */

#ifdef _CPLUSPLUS
#   undef  _EXPORT
#   define _EXPORT extern "C"
#else
#   undef  _EXPORT                                                              
#   define _EXPORT extern                                   
#endif                                                                          

#endif  /* TAD_H */
