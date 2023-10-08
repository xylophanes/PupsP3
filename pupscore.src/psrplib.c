/*------------------------------------------------------------------------------
    Purpose: Support library for PSRP enabled applications.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 5.06 
    Dated:   6th October 2023 
    E-mail:  mao@tumblingdice.co.uk
------------------------------------------------------------------------------*/

#include <me.h>
#include <utils.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <setjmp.h>
#include <netlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <limits.h>

#define SSIZE SSIZE

/*-------------------------------------------------------------*/
/* If we have OpenMP support then we also have pthread support */
/* (OpenMP is built on pthreads for Linux distributions).      */
/*-------------------------------------------------------------*/

#if defined(_OPENMP) && !defined(PTHREAD_SUPPORT)
#define PTHREAD_SUPPORT
#endif /* PTHREAD_SUPPORT */


#ifdef PERSISTENT_HEAP_SUPPORT
#include <pheap.h>
#endif /* PERSISTENT_HEAP_SUPPORT */

#ifdef BUBBLE_MEMORY_SUPPORT
#include <bubble.h>
#endif /* BUBBLE_MEMORY_SUPPORT */

#ifdef DLL_SUPPORT
#include <dll.h>
#endif /* DLL_SUPPORT */

#ifdef PERSISTENT_HEAP_SUPPORT
#include <pheap.h>
#endif /* PERSISTENT_HEAP_SUPPORT */

#ifdef DLL_SUPPORT 
#include <dlfcn.h>
#include <dll.h>
#endif /* DLL_SUPPORT */

#ifdef PTHREAD_SUPPORT
#include <tad.h>
#endif /* PTHREAD_SUPPORT */

#ifndef NO_SCHED_YIELD
#include <sched.h>
#endif /* NO_SCHED_YIELD */

#undef  __NOT_LIB_SOURCE__
#include <psrp.h>
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



/*---------------------------------------------*/
/* Variables which are imported by this module */
/*---------------------------------------------*/


                                                        /*-------------------------------------------------*/
_IMPORT int  pupsighold_cnt[];                          /* Counter for signal hold count                   */
_IMPORT int  rkill_pid;                                 /* Xkill PID                                       */
_IMPORT int  n_abort_funcs_alloc;                       /* Number of abort functions allocated             */
_IMPORT void (*pups_abort_f[MAX_FUNCS])(char *);        /* Abort function point array                      */
_IMPORT char *pups_abort_arg[MAX_FUNCS];                /* Abort function argument pointer array           */
                                                        /*-------------------------------------------------*/




/*---------------------------------------------*/
/* Variables which are exported by this module */
/*---------------------------------------------*/

_PUBLIC _BOOLEAN psrp_child_instance            = FALSE;
_PUBLIC _BOOLEAN psrp_mode                      = FALSE;
_PUBLIC int      psrp_instances                 = 0;
_PUBLIC int      max_trys                       = MAX_TRYS;
_PUBLIC int      psrp_seg_cnt                   = 0;
_PUBLIC int      chlockdes                      = (-1);
_PUBLIC char     channel_name[SSIZE]            = "";
_PUBLIC char     channel_name_in[SSIZE]         = "";
_PUBLIC char     channel_name_out[SSIZE]        = "";
_PUBLIC psrp_channel_type *psrp_current_sic     = (psrp_channel_type *)NULL;
_PUBLIC psrp_object_type  *psrp_object_list     = (psrp_object_type *)NULL;



/*--------------------------------------------*/
/* Variables which are private to this module */
/*--------------------------------------------*/

_PRIVATE int      segmentation_delay            = SEGMENTATION_DELAY;
_PRIVATE _BOOLEAN terminate_current_instance    = FALSE;
_PRIVATE char     psrp_c_code[SSIZE]            = "";
_PRIVATE _BOOLEAN psrp_requests_ignored         = TRUE;
_PRIVATE _BOOLEAN overforking                   = FALSE;
_PRIVATE _BOOLEAN psrp_abrtflag_enable          = FALSE;
_PRIVATE _BOOLEAN psrp_abrtflag                 = FALSE;
_PRIVATE _BOOLEAN psrp_error_handling           = TRUE;
_PRIVATE int      overforked_child_pid          = (-1);
_PRIVATE int      ssh_pid                       = (-1);
_PRIVATE int      ssh_sic_index                 = (-1);
_PRIVATE char     psrp_password[SSIZE]          = "";



/*-------------------------------------------------*/
/* Slot and usage functions - used by slot manager */
/*-------------------------------------------------*/

_PRIVATE void psrplib_slot(int level)
{   (void)fprintf(stderr,"lib psrplib %s: [ANSI C]\n",PSRPLIB_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(C) 1995-2022 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"PUPS/P3 multiuser threadsafe PSRP [SUPUPS] library (built %s %s)\n\n",__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");
    (void)fflush(stderr);
}


/*-----------------------------------------*/
/* Segment identification for psrp library */
/*-----------------------------------------*/

#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT )() __attribute__ ((aligned(16))) = psrplib_slot;
#endif




/*------------------------------------*/
/* Variables imported by this library */
/*------------------------------------*/


/*-------------------------------------------------------------------*/
/* Public variables exported by the multithread dynamic link library */
/* support library                                                   */
/*-------------------------------------------------------------------*/
 
_PUBLIC FILE     *psrp_in                  = (FILE *)NULL;  // I/P from PSRP client
_PUBLIC FILE     *psrp_out                 = (FILE *)NULL;  // O/P to PSRP client 
_PUBLIC _BOOLEAN psrp_reactivate_client    = FALSE;         // Reactivate attached client
_PUBLIC _BOOLEAN connected_once            = FALSE;         // TRUE if connection made
_PUBLIC _BOOLEAN in_psrp_new_segment       = FALSE;         // TRUE if in segment code 
_PUBLIC _BOOLEAN in_psrp_handler           = FALSE;         // TRUE if in PSRP handler code
_PUBLIC _BOOLEAN in_chan_handler           = FALSE;         // TRUE if in chan_handler code


/*----------------------------------------*/
/* Multiple client access database tables */
/*----------------------------------------*/

_PUBLIC int      n_clients = MAX_CLIENTS;                   // Number of clients
_PUBLIC int      c_client  = 0;                             // Active client
_PUBLIC char     psrp_client_name[MAX_CLIENTS][SSIZE];      // Name of client application
_PUBLIC char     psrp_client_host[MAX_CLIENTS][SSIZE];      // Name of localhost
_PUBLIC char     psrp_remote_hostpath[MAX_CLIENTS][SSIZE];  // Path to basal client
_PUBLIC int      psrp_client_pid[MAX_CLIENTS];              // Clients pid
_PUBLIC int      psrp_transactions[MAX_CLIENTS];            // PSRP transaction count
_PUBLIC char     psrp_client_efname[MAX_CLIENTS][SSIZE];    // Name of client exit func
_PUBLIC int      (*psrp_client_exitf[MAX_CLIENTS])(int);    // Client exit function



/*--------------------------------------------*/
/* Variables which are private to this module */
/*--------------------------------------------*/

_PRIVATE _BOOLEAN psrp_channel_open        = FALSE;         // TRUE if PSRP channel open
_PRIVATE _BOOLEAN siginit_pending          = FALSE;         // TRUE if SIGINIT pending
_PRIVATE _BOOLEAN sigchan_pending          = FALSE;         // TRUE if SIGCHAN pending



/*---------------------------------------------*/
/* Functions which are imported by this module */
/*---------------------------------------------*/

// Handler for PUPS virtual interval timers
_PROTOTYPE _IMPORT int vt_handler(const int);



     
/*--------------------------------------------*/
/* Functions which are private to this module */
/*--------------------------------------------*/

// Clear a SIC channel descriptor slot
_PROTOTYPE _PRIVATE void psrp_clear_channel_slot(psrp_channel_type *);

// Set stdio background detach state
_PROTOTYPE _PRIVATE int psrp_builtin_set_nodetach(const int, const char *[]);

// Set PSRP (vitimer) quantum
_PROTOTYPE _PRIVATE int psrp_builtin_set_vitimer_quantum(const int, const char *[]);

// Set (secure) server authentication token
_PROTOTYPE _PRIVATE int psrp_builtin_set_secure(const int, const char *[]);

// Reactivate connected clients
_PROTOTYPE _PRIVATE int psrp_reactivate_clients(void);

// Handler for SIGCLIENT
_PROTOTYPE _PRIVATE int cdoss_handler(int);

#ifdef MAIL_SUPPORT

// Save PSRP channels
_PROTOTYPE _PRIVATE void psrp_save_channels(void);

// Restore PSRP channels
_PROTOTYPE _PRIVATE void psrp_restore_channels(void);

// Parse PRSP commands (from MIME mail interface)
_PROTOTYPE _PRIVATE int psrp_parse_requests(char *);

// Set parts of MIME message to be saved from PSRP API
_PROTOTYPE _PRIVATE int psrp_builtin_set_mime_type(const int, const char *[]);

#endif /* MAIL_SUPPORT */


// Display (fast) caches mapped into the address space of caller
_PROTOTYPE _PRIVATE int psrp_builtin_show_caches(const int, const char *[]);

// Parse a PSRP request (noting which interface originated it)
_PROTOTYPE _PRIVATE int psrp_parse_request(char *, int);

// Show host information
_PROTOTYPE _PRIVATE int psrp_builtin_show_hinfo(const int, const char *[]);


#ifdef BUBBLE_MEMORY_SUPPORT 
// Set (J)malloc options
_PROTOTYPE _PRIVATE int psrp_builtin_set_mbubble_utilisation_threshold(const int, const char *[]);

// Display (J)malloc statistics
_PROTOTYPE _PRIVATE int psrp_builtin_show_malloc_stats(const int, const char *[]);
#endif /* BUBBLE_MEMORY_SUPPORT */

#ifdef CRIU_SUPPORT
// Enable/disable (Criu) state saving
_PROTOTYPE _PRIVATE int psrp_builtin_ssave(const int, const char *[]);
#endif /* CRIU_SUPPORT */


// Process exits if its effective parent is terminated
_PROTOTYPE _PRIVATE int psrp_builtin_set_pexit(const int, const char *[]);

// Process does not exit if its effective parent is terminated
_PROTOTYPE _PRIVATE int psrp_builtin_set_unpexit(const int, const char *[]);

// Set (effective) parent
_PROTOTYPE _PRIVATE int psrp_builtin_set_parent(const int, const char *[]);

// Set system context to rooted (system context cannot migrate)
_PROTOTYPE _PRIVATE int psrp_builtin_set_rooted(const int, const char *[]);

// Set system context to unrooted (system context can migrate)
_PROTOTYPE _PRIVATE int psrp_builtin_set_unrooted(const int, const char *[]);
  
// Set current working directory for PSRP server
_PROTOTYPE _PRIVATE int psrp_builtin_set_cwd(const int, const char *[]);
   
// Set number of times operation is retryed (before aborting)
_PROTOTYPE _PRIVATE int psrp_builtin_set_trys(const int, const char *[]);

// Create trailfile and associated destructor lyosome
_PROTOTYPE _PRIVATE void psrp_create_trailfile(char *, char *, char *, char *);

// Protect a file
_PROTOTYPE _PRIVATE int psrp_builtin_file_live(const int, const char *argv[]);

// Unprotect a file
_PROTOTYPE _PRIVATE int psrp_builtin_file_dead(const int, const char *argv[]);

// Show resource usage for this server
_PROTOTYPE _PRIVATE int psrp_builtin_show_rusage(const int, const char *[]);

// Set resource usage for this server
_PROTOTYPE _PRIVATE int psrp_builtin_set_rlimit(const int, const char *[]);

// Builtin to display crontab contents
_PROTOTYPE _PRIVATE int psrp_builtin_show_crontab(const int, const char *[]);

// Builtin to schedule a crontab operation
_PROTOTYPE _PRIVATE int psrp_builtin_crontab_schedule(const int, const char *[]);

// Builtin to unschedule a crontab operation
_PROTOTYPE _PRIVATE int psrp_builtin_crontab_unschedule(const int, const char *[]);

// Check to see if any cron operations are pending
_PROTOTYPE _PRIVATE void psrp_crontab_checkschedule(char *);

// Add an alias to a PSRP object
_PROTOTYPE _PRIVATE int psrp_builtin_alias(const int, const char *[]);

// Builtin to show process status
_PROTOTYPE _PRIVATE int psrp_builtin_show_procstatus(const int, const char *[]);

// Show the (current) clients attached to PSRP server
_PROTOTYPE _PRIVATE int psrp_builtin_show_clients(const int, const char *[]);

// Create a new proces instance of PSRP server process
_PROTOTYPE _PRIVATE int psrp_builtin_new_instance(const int, const char *[]);

// Return number of clients connected to server
_PROTOTYPE _PRIVATE int psrp_connected_clients(void);

// Get index in connected client array
_PROTOTYPE _PRIVATE int psrp_get_client_slot(int);

// Clear index in connected client array
_PROTOTYPE _PRIVATE int psrp_clear_client_slot(int);

// Show clients attached to server
_PROTOTYPE _PRIVATE int psrp_show_clients(void);

// Flag end of (interrupt driven) PSRP operation
_PROTOTYPE _PRIVATE void psrp_endop(char *op_tag);

// Show clients connected to this server
_PROTOTYPE _PRIVATE int psrp_builtin_show_clients(const int, const char *[]);

// Remove a tag from a tag list (alias)
_PROTOTYPE _PRIVATE int psrp_builtin_unalias(const int, const char *[]);

// Show aliases for objects bound to this PRSP server
_PROTOTYPE _PRIVATE int psrp_builtin_showaliases(const int, const char *[]);

// Detach an object from the PSRP dispatch list
_PROTOTYPE _PRIVATE int psrp_detach_object(unsigned int);

// Get argument vector from request string
_PROTOTYPE _PRIVATE void psrp_argvec(int *, char *);

// Switch transaction/error logging on/off
_PROTOTYPE _PRIVATE int psrp_builtin_appl_verbose(const int, const char *[]);

// Toggle (client) error handling on/off
_PROTOTYPE _PRIVATE int psrp_builtin_error_handling(const int, const char *[]);

// Delete object from PSRP dispatch table
_PROTOTYPE _PRIVATE int psrp_builtin_transactions(const int, const char *[]);

// Delete object from PSRP dispatch table
_PROTOTYPE _PRIVATE int psrp_builtin_detach_object(const int, const char *[]);

// Show state of PSRP bindings (to client)
_PROTOTYPE _PRIVATE int psrp_builtin_show_psrp_state(const int, const char *[]);

// Show bindings of all PSRP objects
_PROTOTYPE _PRIVATE int psrp_builtin_show_psrp_bind_type(const int, const char *[]);

// Terminate PSRP server process
_PROTOTYPE _PRIVATE int psrp_builtin_terminate_process(const int, const char *[]);


#ifdef SSH_SUPPORT
// Set ssh compression
_PROTOTYPE _PRIVATE int psrp_builtin_ssh_compress(const int, const char *[]);

// Set remote ssh port
_PROTOTYPE _PRIVATE int psrp_builtin_ssh_port(const int, const char *[]);
#endif /* SSH_SUPPORT */

// Display help for psrp_handler
_PROTOTYPE _PRIVATE int psrp_builtin_help(const int, const char *[]);

#ifdef DLL_SUPPORT
// Show DLL orifices which are bound to this application
_PROTOTYPE _PRIVATE int psrp_builtin_show_attached_orifices(const int, const char *[]);
#endif /* DLL_SUPPORT */

#ifdef PTHREAD_SUPPORT
// Show threads running in PSRP server
_PROTOTYPE _PRIVATE int psrp_builtin_show_threads(const int, const char *[]);

// Start thread running in PSRP server
_PROTOTYPE _PRIVATE int psrp_builtin_launch_thread(const int, const char *[]);

// Pause thread runnin in PSRP server
_PROTOTYPE _PRIVATE int psrp_builtin_pause_thread(const int, const char *[]);

// Pause thread runnin in PSRP server
_PROTOTYPE _PRIVATE int psrp_builtin_restart_thread(const int, const char *[]);

// Kill thread running in PSRP server
_PROTOTYPE _PRIVATE int psrp_builtin_kill_thread(const int, const char *[]);
#endif /* PTHREAD_SUPPORT */

// Show this applications concurrently held link file locks
_PROTOTYPE _PRIVATE int psrp_builtin_show_link_file_locks(const int, const char *[]);

// Show this applications flock locks
_PROTOTYPE _PRIVATE int psrp_builtin_show_flock_locks(const int, const char *[]);

// Show this applications non-default signal handlers
_PROTOTYPE _PRIVATE int psrp_builtin_show_sigstatus(const int, const char *[]);

// Show this applications signal mask/ pending signal status
_PROTOTYPE _PRIVATE int psrp_builtin_show_sigmaskstatus(const int, const char *[]);

// Show streams/file descriptors opened by this application
_PROTOTYPE _PRIVATE int psrp_builtin_show_open_fdescriptors(const int, const char *[]);

// Show (active) children of this application
_PROTOTYPE _PRIVATE int psrp_builtin_show_children(const int, const char *[]);

// Show slaved interation client channels open
_PROTOTYPE _PRIVATE int psrp_builtin_show_open_sics(const int, const char *[]);

// Show exit functions registered for this application 
_PROTOTYPE _PRIVATE int psrp_builtin_pups_show_exit_f(const int, const char *[]);

// Show abort functions registered for this application
_PROTOTYPE _PRIVATE int psrp_builtin_pups_show_abort_f(const int, const char *[]);

// Show entrance functions registered for this application
_PROTOTYPE _PRIVATE int psrp_builtin_pups_show_entrance_f(const int, const char *[]);

// Show status of PUPS virtual timers
_PROTOTYPE _PRIVATE int psrp_builtin_pups_show_vitimers(const int, const char *[]);

// show persistent heaps mapped into process address space
_PROTOTYPE _PRIVATE int psrp_builtin_show_persistent_heaps(const int, const char *[]);

// show persistent heaps mapped into process address space
_PROTOTYPE _PRIVATE int psrp_builtin_show_htobjects(const int, const char *[]);

// Load a new dispatch table
_PROTOTYPE _PRIVATE int psrp_builtin_autosave_dispatch_table(const int, const char *[]);

// Load a new dispatch table
_PROTOTYPE _PRIVATE int psrp_builtin_load_dispatch_table(const int, const char *[]);

// Save dispatch table
_PROTOTYPE _PRIVATE int psrp_builtin_save_dispatch_table(const int, const char *[]);

// Reset dispatch table to it initial state */
_PROTOTYPE _PRIVATE int psrp_builtin_reset_dispatch_table(const int, const char *[]);

// Extend the child table
_PROTOTYPE _PRIVATE int psrp_builtin_extend_chtab(const int, const char *[]);


#ifdef PERSISTENT_HEAP_SUPPORT
// Attach a persistent heap to PSRP handler
_PROTOTYPE _PRIVATE int psrp_builtin_attach_persistent_heap(const int, const char *[]);

// Extend persistent heap table
_PROTOTYPE _PRIVATE int psrp_builtin_extend_htab(const int, const char *[]);
#endif /* PERSISTENT_HEAP_SUPPORT */


#ifdef DLL_SUPPORT
// Attach dynamic function to PSRP handler
_PROTOTYPE _PRIVATE int psrp_builtin_attach_dynamic_function(const int, const char *[]);

// Extend orifice [DLL] table
_PROTOTYPE _PRIVATE int psrp_builtin_extend_ortab(const int, const char *[]);
#endif /* DLL_SUPPORT */


// Show number of (fast) caches memory mapped into process address space
_PROTOTYPE _PRIVATE int psrp_builtin_show_caches(const int, const char *[]);

// Display statistics of a mapped (fast) cache
_PROTOTYPE _PRIVATE int psrp_builtin_display_cache(const int, const char *[]);

// Extend virtual timer table
_PROTOTYPE _PRIVATE int psrp_builtin_extend_vitab(const int, const char *[]);

// Extend the file table
_PROTOTYPE _PRIVATE int psrp_builtin_extend_ftab(const int, const char *[]);

// Overlay PSRP server with new command 
_PROTOTYPE _PRIVATE int psrp_builtin_overlay_server_process(const int, const char *[]);

// Overfork PSRP server with new command
_PROTOTYPE _PRIVATE int psrp_builtin_overfork_server_process(const int, const char *[]);

// Attach dynamic function to PSRP handler
_PROTOTYPE _PRIVATE int psrp_builtin_attach_dbag(const int, const char *[]);

// Handler for SIGALRM
_PROTOTYPE _PRIVATE void psrp_homeostat(void *, char *);

// Handler for SIGCHAN
_PROTOTYPE _PRIVATE int chan_handler(int);

// Handler for SIGPSRP 
_PROTOTYPE _PRIVATE int psrp_handler(int);

// Handler for propagated SIGABRT
_PROTOTYPE _PRIVATE int abrt_handler(int);

// Initialise channel table for slaved interation clients
_PROTOTYPE _PRIVATE void psrp_initsic(void);


/*--------------------------------------------*/
/* Variables which are private to this module */
/*--------------------------------------------*/

_PRIVATE sigjmp_buf        chan_env;                                         // Env at chan_handler entry
_PRIVATE sigjmp_buf        psrp_env;                                         // Env at psrp_handler entry
_PRIVATE int               req_r_cnt[MAX_CLIENTS];                           // Attached client request counts
_PRIVATE char              old_request_str[MAX_CLIENTS][SSIZE];              // Last request for each attached client
_PRIVATE _BOOLEAN          psrp_log[MAX_CLIENTS]              =  { FALSE };  // Per client transaction logging flags


/*---------------------------------------------------*/
/* Variables which are private to the PSRP subsystem */
/*---------------------------------------------------*/

_PRIVATE int               psrp_object_list_size;
_PRIVATE int               psrp_object_list_used;
_PRIVATE int               psrp_bind_status  = PSRP_STATIC_FUNCTION;
_PRIVATE _BOOLEAN          psrp_ignore       = FALSE;
_PRIVATE psrp_channel_type channel[PSRP_MAX_SIC_CHANNELS];
_PRIVATE psrp_crontab_type crontab[MAX_CRON_SLOTS];



/*--------------------------------------------------------------------*/
/* Ignore PSRP requests (this routine also sets the appl_psrp flag to */
/* TRUE)                                                              */
/*--------------------------------------------------------------------*/

_PUBLIC void psrp_ignore_requests(void)

{   if(appl_root_thread != 0 && pupsthread_is_root_thread() == FALSE)
    {  (void)fprintf(stderr,"psrp_ignore_requests: attempt by non root thread to perform PUPS/P3 PSRP operation");
       (void)fflush(stderr);

       pups_exit(255);
    } 

    ignore_pups_signals   = TRUE;
    (void)pupshold(PSRP_SIGS); 
    appl_psrp             = TRUE;
    psrp_requests_ignored = TRUE;
}




/*----------------------*/
/* Accept PSRP requests */
/*----------------------*/

_PUBLIC void psrp_accept_requests(void)

{

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_accept_requests] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(psrp_requests_ignored == FALSE)
       return;
    else
       psrp_requests_ignored = FALSE; 


    /*--------------------------------------------------------------------------------*/
    /* Unlink channel segment lock (if any)  - each client will then grab the seglock */
    /* reconnect and release (via its client handler)                                 */
    /*--------------------------------------------------------------------------------*/

    if(psrp_reactivate_client == TRUE && chlockdes != (-1))
    {  (void)pups_release_fd_lock(chlockdes);
       (void)close(chlockdes);

       #ifdef PSRPLIB_DEBUG
       (void)fprintf(stderr,"PSRPLIB LOCK RELEASE\n");
       (void)fflush(stderr);
       #endif /* PSRPLIB_DEBUG */

       psrp_reactivate_client = FALSE;
    }

    ignore_pups_signals = FALSE;
    (void)pupsrelse(PSRP_SIGS);

    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,"PSRPLIB PSRP ACTIVATED\n");
    (void)fflush(stderr);
    #endif /* PSRPLIB_DEBUG */
}




/*----------------------------------*/
/* Import PSRP dispatch table items */
/*----------------------------------*/

_PUBLIC int psrp_load_dispatch_table(const _BOOLEAN          search,    // If TRUE search for objects
                                     const char *autoload_file_name)    // Dispatch table file name

{   int i,
        tag_index,
        h_mode,
        cnt           = 0,
        aliases       = 0,
        psrp_objects  = 0;

    unsigned long int f_pos = 0;

    char object_tag[SSIZE]        = "",
         object_type[SSIZE]       = "",
         object_f_name[SSIZE]     = "",
         homeostasis_mode[SSIZE]  = "",
         alias_name[SSIZE]        = "",
         next_line[SSIZE]         = "",
         tmp_f_name[SSIZE]        = "",
         tmp_str[SSIZE]           = "";

    FILE *raw_autoload_file_stream = (FILE *)NULL,
         *autoload_file_stream     = (FILE *)NULL;

    struct stat buf;


    if(autoload_file_name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_load_dispatch_table] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(access(autoload_file_name,F_OK | R_OK | W_OK) == (-1))
    {  if(search == TRUE)
       {  

          /*------------------------------------------*/
          /* If we can't find the dispatch table file */
          /* look for it in /usr/local/lib            */
          /* Firstly, we must strip the path (if any) */
          /* from the dispatch table file name        */
          /*------------------------------------------*/

          if(strrextr(autoload_file_name,tmp_str,'/') == TRUE)
            (void)strlcpy(autoload_file_name,tmp_str,SSIZE);


          /*--------------------------------*/
          /* Get pathname to dispatch table */
          /*--------------------------------*/

          if(strccpy(tmp_str,(char *)pups_search_path("DISPATCH_PATH",autoload_file_name)) != (char *)NULL)
             (void)strlcpy(autoload_file_name,tmp_str,SSIZE); 
       }

       pups_set_errno(EINVAL);
       return(-1);
    }

    (void)stat(autoload_file_name,&buf);
    if(buf.st_mode & !S_IFREG)
    {  pups_set_errno(ESPIPE);
       return(-1);
    }


    /*---------------------------------------------------------------*/
    /* We need to lock the disparch table file as multiple processes */
    /* could be accessing the same file                              */
    /*---------------------------------------------------------------*/

    (void)pups_get_link_file_lock(WAIT_FOREVER,autoload_file_name);
    raw_autoload_file_stream = fopen(autoload_file_name,"r");

    (void)snprintf(tmp_f_name,SSIZE,"/tmp/psrp.autoload.%d.tmp",appl_pid);
    autoload_file_stream = pups_strp_commnts('#',raw_autoload_file_stream,tmp_f_name);

    (void)fgets(tmp_str,SSIZE,autoload_file_stream);

    if(sscanf(tmp_str,"%d",&psrp_objects) != 1)
    {  (void)pups_release_link_file_lock(autoload_file_name);
       pups_error("[psrp_load_dispatch_table] corrupt PSRP resource file");
    }

    if(appl_verbose == TRUE)
    {  strdate(date);

       if(psrp_objects == 1)
          (void)fprintf(stderr,"%s %s (%d@%s:%s): 1 PSRP object definition to process\n",date,
                                                                                    appl_name,
                                                                                     appl_pid,
                                                                                    appl_host,
                                                                                   appl_owner);
        else
           (void)fprintf(stderr,"%s %s (%d@%s:%s): %d PSRP object definitions to process\n",date,
                                                                                       appl_name,
                                                                                        appl_pid,
                                                                                       appl_host,
                                                                                      appl_owner,
                                                                                    psrp_objects);
       (void)fflush(stderr);
    }

    do {    int ret;


            /*----------------------------------*/
            /* Have all objects been processed? */ 
            /*----------------------------------*/

            if(cnt < psrp_objects)
            {

	       ret = fscanf(autoload_file_stream,"%s %s",object_type,homeostasis_mode);

               if(ret == 0 || ret > 2)
               {  (void)fclose(autoload_file_stream);
                  (void)pups_release_link_file_lock(autoload_file_name);
                  (void)unlink(tmp_f_name);
                  pups_error("[psrp_load_dispatch_table] corrupt PSRP resource file");
               }

               if(strcmp(homeostasis_mode,"LIVE") == 0)
                  h_mode = LIVE;
               else if(strcmp(homeostasis_mode,"DEAD") == 0)
                  h_mode = DEAD;
               else
               {  (void)strlcpy(object_f_name,homeostasis_mode,SSIZE);
                  (void)strlcpy(homeostasis_mode,"UNDEFINED",SSIZE);
               }
              
               if(strcmp(homeostasis_mode,"LIVE") == 0) 
               {  if(fscanf(autoload_file_stream,"%s",object_f_name) != 1)
                  {  (void)fclose(autoload_file_stream);
                     (void)pups_release_link_file_lock(autoload_file_name);
                     (void)unlink(tmp_f_name);
                     pups_error("[psrp_load_dispatch_table] corrupt PSRP resource file");
                  }
               }
 
               if(fscanf(autoload_file_stream,"%d",&aliases) != 1 || feof(autoload_file_stream) == 1)
               {  (void)fclose(autoload_file_stream);
                  (void)pups_release_link_file_lock(autoload_file_name);
                  (void)unlink(tmp_f_name);
                  pups_error("[psrp_load_dispatch_table] corrupt PSRP resource file");
               }

               if(fscanf(autoload_file_stream,"%s",object_tag) != 1)
               {  (void)fclose(autoload_file_stream);
                  (void)pups_release_link_file_lock(autoload_file_name);
                  (void)unlink(tmp_f_name);
                  pups_error("[psrp_load_dispatch_table] corrupt PSRP resource file");
               }

               (void)strdate(date);

               #ifdef DLL_SUPPORT
               if(strcmp(object_type,"dynamic_function") == 0)
                  (void)psrp_attach_dynamic_function(search,object_tag,object_f_name);
               #endif /* DLL_SUPPORT */

               if(strcmp(object_type,"dynamic_databag") == 0)
                  (void)psrp_attach_dynamic_databag(search,object_tag,object_f_name,h_mode);

               #ifdef PERSISTENT_HEAP_SUPPORT
               if(strcmp(object_type,"persistent_heap") == 0)
                  (void)psrp_attach_persistent_heap(search,object_tag,object_f_name,h_mode);
               #endif /* PERSISTENT_HEAP_SUPPORT */ 


               /*-----------------------*/
               /* Display loaded object */
               /*-----------------------*/
 
               if(appl_verbose == TRUE)
               {  if(aliases - 1 == 1)
                     (void)fprintf(stderr,"%s %s (%d@%s:%s): processing %s %s (%s) (1 alias)\n",
                                                                                           date,
                                                                                      appl_name,
                                                                                       appl_pid,
                                                                                      appl_host,
                                                                                     appl_owner,
                                                                                    object_type,
                                                                                     object_tag,
                                                                               homeostasis_mode);
                  else
                     (void)fprintf(stderr,"%s %s (%d@%s:%s): processing %s %s (%s) (%d aliases)\n",
                                                                                              date,
                                                                                         appl_name,
                                                                                          appl_pid,
                                                                                         appl_host,
                                                                                        appl_owner,
                                                                                       object_type,
                                                                                        object_tag,
                                                                                  homeostasis_mode,
                                                                                         aliases-1);
                  (void)fflush(stderr);
               }

               /*-----------------------------------------------------------------------*/ 
               /* Add aliases - this is possibly not the most efficient way of aliasing */
               /*-----------------------------------------------------------------------*/ 

               if(aliases > 1)
               {  for(i=1; i<aliases; ++i)
                  {  if(fscanf(autoload_file_stream,"%s",alias_name) != 1)
                     {  (void)fclose(autoload_file_stream);
                        (void)pups_release_link_file_lock(autoload_file_name);
                        (void)unlink(tmp_f_name);
                        pups_error("[psrp_load_dispatch_table] corrupt PSRP resource file");
                     }

                     psrp_alias(object_tag,alias_name);
                  }
               }
            }

            ++cnt;
        } while(cnt < psrp_objects);

    if(appl_verbose == TRUE)
    {  (void)fprintf(stderr,"%s %s (%d@%s:%s): PSRP resource file %s loaded\n",date,
                                                                          appl_name,
                                                                           appl_pid,
                                                                          appl_host,
                                                                         appl_owner,
                                                                 autoload_file_name);
       (void)fflush(stderr);
    }
          
    (void)fclose(autoload_file_stream);
    (void)pups_release_link_file_lock(autoload_file_name);
    (void)unlink(tmp_f_name);

    pups_set_errno(OK);
    return(0);
}




/*----------------------------------------------------------------------------
    Initialise PSRP action handler system ...
----------------------------------------------------------------------------*/

_PUBLIC void psrp_init(const int bind_status, const int (*psrp_process_status)(const int,  const char *argv[]))

{   int i,
        authentication;

    char strdum[SSIZE]  = "";

    FILE *autoload_file_stream = (FILE *)NULL;

    sigset_t init_set,
             chan_set,
             psrp_set,
             abrt_set;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_init] attempt by non root thread to perform PUPS/P3 PSRP operation");

    psrp_object_list_size = 0;
    psrp_object_list_used = 0;
    psrp_bind_status      = bind_status;
    appl_psrp             = TRUE;


    /*---------------------------------------------*/
    /* Initialise multiple access client database. */
    /*---------------------------------------------*/

    /* Initialise slots for PSRP client connections */
    for(i=0; i<MAX_CLIENTS; ++i)
        (void)psrp_clear_client_slot(i);


    /*-------------------------------------------------*/
    /* Create communication channels for this process. */
    /*-------------------------------------------------*/

    (void)snprintf(channel_name_out,SSIZE,"%s/psrp#%s#fifo#out#%d#%d",appl_fifo_dir,appl_ch_name,appl_pid,appl_uid); 
    (void)snprintf(channel_name_in,SSIZE, "%s/psrp#%s#fifo#in#%d#%d", appl_fifo_dir,appl_ch_name,appl_pid,appl_uid);
    (void)snprintf(channel_name,SSIZE,    "%s/psrp#%s#fifo#IO#%d#%d", appl_fifo_dir,appl_ch_name,appl_pid,appl_uid);

    if(access((char *)channel_name_in,F_OK | R_OK | W_OK) == (-1))
       (void)mkfifo(channel_name_in,0600);

    if(access((char *)channel_name_out,F_OK | R_OK | W_OK) == (-1))
       (void)mkfifo(channel_name_out,0600);


    /*---------------------------------------------------------------------*/
    /* Make sure that we delete the communications channel pipes when this */
    /* process exits.                                                      */
    /*---------------------------------------------------------------------*/

    if(appl_verbose == TRUE)
    {  strdate(date);
       (void)fprintf(stderr,
                     "%s %s (%d@%s:%s): PSRP (protocol %5.2F) handler initialised on channel %s/psrp:%s:%s:fifo:IO:%d\n",
                                                                                                                    date,
                                                                                                               appl_name,
                                                                                                                appl_pid, 
                                                                                                               appl_host, 
                                                                                                              appl_owner, 
                                                                                                   PSRP_PROTOCOL_VERSION,
                                                                                                           appl_fifo_dir,
                                                                                                               appl_name, 
                                                                                                               appl_host, 
                                                                                                                appl_pid);
       (void)fflush(stderr);
    }

    if(psrp_bind_status & PSRP_HOMEOSTATIC_STREAMS)
    {  if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): Streams: PSRP streams are homeostatic\n",
                                              date,appl_name,appl_pid,appl_host,appl_owner);
       }

       (void)pups_setvitimer("psrp_homeostat",1,VT_CONTINUOUS,1,NULL,(void *)psrp_homeostat);
    }

    if(appl_verbose == TRUE && psrp_bind_status & PSRP_PERSISTENT_HEAP)
    {  (void)strdate(date);

       if(appl_verbose == TRUE)
          (void)fprintf(stderr,"%s %s (%d@%s:%s): Heaps: can import persistent heaps\n",
                                           date,appl_name,appl_pid,appl_host,appl_owner);
    }

    if(appl_verbose == TRUE && psrp_bind_status & PSRP_STATIC_DATABAG)
    {  (void)strdate(date);

       (void)fprintf(stderr,"%s %s (%d@%s:%s): Databags:  static binding enabled (cannot import databags)\n",
                                                                date,appl_name,appl_pid,appl_host,appl_owner);
    }

    if(appl_verbose == TRUE && psrp_bind_status & PSRP_DYNAMIC_DATABAG)
    {  (void)strdate(date);

       (void)fprintf(stderr,"%s %s (%d@%s:%s): Databags:  dynamic binding enabled (can import databags)\n",
                                                              date,appl_name,appl_pid,appl_host,appl_owner);
    }

    if(appl_verbose == TRUE && psrp_bind_status & PSRP_STATIC_FUNCTION)
    {  (void)strdate(date);

       (void)fprintf(stderr,"%s %s (%d@%s:%s): Functions: static binding enabled (cannot import DLL functions)\n",
                                                                     date,appl_name,appl_pid,appl_host,appl_owner);
    }

    #ifdef DLL_SUPPORT
    if(appl_verbose == TRUE && psrp_bind_status & PSRP_DYNAMIC_FUNCTION)
    {  (void)strdate(date);

       (void)fprintf(stderr,"%s %s (%d@%s:%s): Functions: dynamic binding enabled (can import DLL functions)\n",
                                                                   date,appl_name,appl_pid,appl_host,appl_owner);
    }
    #endif /* DLL_SUPPORT */

    #ifdef PERSISTENT_HEAP_SUPPORT
    if(appl_verbose == TRUE && psrp_bind_status & PSRP_PERSISTENT_HEAP)
    {  (void)strdate(date);

       (void)fprintf(stderr,"%s %s (%d@%s:%s): Functions: persistent heap  binding enabled (can attach persistent heaps)\n\n",
                                                                                 date,appl_name,appl_pid,appl_host,appl_owner);
        
    }
    #endif /* PERSISTENT_HEAP_SUPPORT */


    /*----------------------------------------------------------------------*/
    /* Initialise slaved PSRP client channel table (thse slaved clients are */
    /* used for peer-to-peer communication between PSRP servers).           */
    /*----------------------------------------------------------------------*/

    psrp_initsic();


    /*-----------------------------------------*/
    /* Initialise handler and error functions. */
    /*-----------------------------------------*/

    pups_register_exit_f("psrp_exit",&psrp_exit,(char *)NULL);


    /*-----------------------------------------------*/
    /* Signals blocked while CHAN handler is running */
    /*-----------------------------------------------*/

    (void)sigemptyset(&chan_set);
    (void)sigaddset(&chan_set,SIGINIT);
    (void)sigaddset(&chan_set,SIGCHAN);
    (void)sigaddset(&chan_set,SIGPSRP);
    (void)sigaddset(&chan_set,SIGCHLD);
    (void)sigaddset(&init_set,SIGABRT);
    (void)sigaddset(&init_set,SIGALRM);


    /*-----------------------------------------------*/
    /* Signals blocked while PSRP handler is running */
    /*-----------------------------------------------*/

    (void)sigemptyset(&psrp_set);
    (void)sigaddset(&psrp_set,SIGINIT);
    (void)sigaddset(&psrp_set,SIGCHAN);
    (void)sigaddset(&chan_set,SIGPSRP);
    (void)sigaddset(&psrp_set,SIGCHLD);
    (void)sigaddset(&psrp_set,SIGABRT);
    (void)sigaddset(&init_set,SIGALRM);


    /*-----------------------------------------------------*/
    /* Signals blocked when connection is being intialised */
    /*-----------------------------------------------------*/

    (void)sigemptyset(&init_set);
    (void)sigaddset(&chan_set,SIGINIT);
    (void)sigaddset(&init_set,SIGCHAN);
    (void)sigaddset(&init_set,SIGPSRP);
    (void)sigaddset(&init_set,SIGCHLD);
    (void)sigaddset(&init_set,SIGABRT);
    (void)sigaddset(&init_set,SIGALRM);


    /*-----------------------------------------------------*/
    /* Signals blecked while SIGABRT - client interrupt is */
    /* being serviced                                      */
    /*-----------------------------------------------------*/

    (void)sigfillset(&abrt_set);
    (void)sigdelset(&abrt_set,SIGSEGV);
    (void)sigdelset(&abrt_set,SIGTERM);

    (void)pups_sighandle(SIGINIT,  "chan_handler", (void *)chan_handler, &init_set);
    (void)pups_sighandle(SIGCHAN,  "chan_handler", (void *)chan_handler, &chan_set);
    (void)pups_sighandle(SIGPSRP,  "psrp_handler", (void *)psrp_handler, &psrp_set);
    (void)pups_sighandle(SIGABRT,  "abrt_handler", (void *)abrt_handler, &abrt_set);
    (void)pups_sighandle(SIGCLIENT,"cdoss_handler",(void *)cdoss_handler,&abrt_set);
    (void)pups_sighandle(SIGALIVE, "ignore",SIG_IGN, (sigset_t *)NULL);


    /*-------------------------------------------------------------------*/
    /* Otherwise attach status function if server is running status only */ 
    /*-------------------------------------------------------------------*/

    if((void *)psrp_process_status != (void *)NULL)
       psrp_attach_static_function("status",psrp_process_status);


    #ifdef MAIL_SUPPORT
    /*---------------------------------------------------------------------*/
    /* If the process is mailable start the mail hoemostat so we check our */
    /* mailbox regularly                                                   */
    /*---------------------------------------------------------------------*/

    if(appl_mailable == TRUE)
       (void)pups_setvitimer("mail_hoemostat",1,VT_CONTINUOUS,100,NULL,(void *)&psrp_mail_homeostat);
    #endif /* MAIL_SUPPORT */


    psrp_mode = TRUE;
    pups_set_errno(OK);
}




/*----------------------------------------------------------------------------
    If an autoload file exists for this PSRP server load the objects
    in it and their (user defined attributes) ...
----------------------------------------------------------------------------*/

_PUBLIC _BOOLEAN psrp_load_default_dispatch_table(void)

{   char autoload_file_name[SSIZE] = "";


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_load_default_dispatch_table] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(appl_psrp_load == FALSE)
       return(FALSE);

    (void)strdate(date);
    (void)snprintf(autoload_file_name,SSIZE,"%s/.%s.psrp",appl_home,appl_name);
    if(access(autoload_file_name, F_OK | R_OK | W_OK) == (-1))
    {  pups_set_errno(ENFILE);
       return(FALSE);
    }

    if(psrp_load_dispatch_table(TRUE,autoload_file_name) == (-1))
       return(FALSE);

    pups_set_errno(OK);
    return(TRUE);
}




/*----------------------------------------------------------------------------
   Find action slot index by name ...
----------------------------------------------------------------------------*/

_PUBLIC int psrp_find_action_slot_index(const char *object_tag)

{   int i,
        j;


    if(object_tag == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_find_action_slot_index] attempt by non root thread to perform PUPS/P3 PSRP operation");

    for(i=0; i<psrp_object_list_used; ++i)
    {  if(psrp_object_list[i].aliases > 0)
       {

          /*--------------------------------*/
          /* Search tag list for object_tag */
          /*--------------------------------*/

          for(j=0; j<psrp_object_list[i].aliases_allocated; ++j)
          {  if(psrp_object_list[i].object_tag[j] != (char *)NULL           &&
                strcmp(psrp_object_list[i].object_tag[j],object_tag) == 0    )
             {  pups_set_errno(OK);
                return(i);
             }
          }
       }
    }

    pups_set_errno(ESRCH);
    return(-1);
}




/*----------------------------------------------------------------------------
   Get the first free slot in the dispatch table allocating more space if
   necessary ...
----------------------------------------------------------------------------*/

_PUBLIC int psrp_get_action_slot_index(void)

{   int i;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_get_action_slot_index] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(psrp_object_list_size == psrp_object_list_used)
    {  psrp_object_list_used += PSRP_DISPATCH_TABLE_SIZE;
       psrp_object_list = (psrp_object_type *)pups_realloc((void *)psrp_object_list,
                                                           psrp_object_list_used*sizeof(psrp_object_type));


       /*---------------------------------------*/
       /* Clean out the newly allocated entries */
       /*---------------------------------------*/

       for(i=psrp_object_list_used-PSRP_DISPATCH_TABLE_SIZE; i<psrp_object_list_used; ++i)
       {   psrp_object_list[i].aliases_allocated = 0;
           psrp_object_list[i].aliases           = 0;
           psrp_object_list[i].object_tag        = (char **)NULL;
           psrp_object_list[i].object_f_name     = (char *)NULL;
           psrp_object_list[i].hid               = (-1);
           psrp_object_list[i].object_type       = (-1);
           psrp_object_list[i].object_size       = (-1);
           psrp_object_list[i].object_handle     = (void *)NULL;
       }

       ++psrp_object_list_size;

       pups_set_errno(OK);
       return(psrp_object_list_size-1);
    }

    for(i=0; i<psrp_object_list_used; ++i)
    {  if(psrp_object_list[i].object_handle == NULL)
       {  ++psrp_object_list_size;

          pups_set_errno(OK);
          return(i);
       }
    }

    pups_set_errno(ESRCH);
    return(-1);
}


    

/*----------------------------------------------------------------------------
   Attach a static databag to the PSRP handler dispatch list ...
----------------------------------------------------------------------------*/

_PUBLIC int psrp_attach_static_databag(const char               *object_tag,
                                       const unsigned long  int databag_size,
                                       const _BYTE              *databag_handle)

{   int slot_index,
        tag_index;


    if(object_tag == (const char *)NULL || databag_handle == (const void *)NULL)
    {  pups_set_errno(EINVAL);
       return(PSRP_DISPATCH_ERROR);
    }

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_attach_static_databag] attempt by non root thread to perform PUPS/P3 PSRP operation");


    if((slot_index = psrp_find_action_slot_index(object_tag)) == (-1))
        slot_index = psrp_get_action_slot_index();
    else
    {  if(appl_verbose == TRUE)
       {  if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@%s:%s): overwriting existing object %s\n",
                               date,appl_name,appl_pid,appl_host,appl_owner,object_tag);
             (void)fflush(stderr);
          }
       }
    }

    psrp_object_list[slot_index].aliases           = 0;
    psrp_object_list[slot_index].aliases_allocated = 0;
    tag_index = psrp_get_tag_index(slot_index);

    (void)strlcpy(psrp_object_list[slot_index].object_tag[tag_index],object_tag,SSIZE);

    if(psrp_object_list[slot_index].object_tag[tag_index] == (char *)NULL)
       psrp_object_list[slot_index].object_tag[tag_index] = (char *)pups_malloc(SSIZE);

    psrp_object_list[slot_index].object_handle     = (void *)databag_handle;
    psrp_object_list[slot_index].object_size       = databag_size;
    psrp_object_list[slot_index].object_type       = PSRP_STATIC_DATABAG;

    if(appl_verbose == TRUE )
    {  (void)fprintf(stderr,"%s %s (%d@%s:%s): static databag \"%-32s\" attached (at offset %016lx virtual)\n",
                                                                                                          date,
                                                                                                     appl_name,
                                                                                                      appl_pid,
                                                                                                     appl_host,
                                                                                                    appl_owner,
                                                                                                    object_tag,
                                                                             (unsigned long int)databag_handle);
       (void)fflush(stderr);
    }


    pups_set_errno(OK);
    return(PSRP_OK);
}



/*----------------------------------------------------------------------------
    Attach a dynamic databag to the PSRP handler dispatch list ...
----------------------------------------------------------------------------*/

_PUBLIC int psrp_attach_dynamic_databag(const _BOOLEAN     search,  // If TRUE search DISPATCH_PATH directories
                                        const char    *object_tag,  // Object (root) tag
                                        const char *bag_file_name,  // Name of file containing bag data
                                        const int          h_mode)  // Databag homeostasis mode
 
{   int  slot_index,
         tag_index,
         bag_size,
         bag_bytes_read,
         bag_des;
 
    _BYTE *bag_handle = (_BYTE *)NULL;
 

    if(object_tag == (const char *)NULL || bag_file_name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(PSRP_DISPATCH_ERROR);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_attach_dynamic_databag] attempt by non root thread to perform PUPS/P3 PSRP operation");


    /*-------------------------------------------------*/
    /* Check to see if object exists on specified path */
    /* if it doesn't search the directories specified  */
    /* in the DISPATCH_PATH environment variable       */
    /*-------------------------------------------------*/

    if(access(bag_file_name,F_OK | R_OK | W_OK) == (-1))
    {  if(search == TRUE)
       {  char tmp_str[SSIZE] = "";

          if(strrext(bag_file_name,tmp_str,'/') == TRUE)
             (void)strlcpy(bag_file_name,tmp_str,SSIZE);

          if(strccpy(tmp_str,(char *)pups_search_path("DISPATCH_PATH",bag_file_name)) == (char *)NULL)
          {  pups_set_errno(EEXIST);
             return(PSRP_DISPATCH_ERROR);
          }
          (void)strlcpy(bag_file_name,tmp_str,SSIZE);
       }
       else
       {  pups_set_errno(EEXIST);
          return(PSRP_DISPATCH_ERROR);
       }
    }

    if((slot_index = psrp_find_action_slot_index(object_tag)) == (-1))
        slot_index = psrp_get_action_slot_index();
    else
    {  if(appl_verbose == FALSE)
       {  if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@%s:%s): overwriting existing object %s\n",
                               date,appl_name,appl_pid,appl_host,appl_owner,object_tag);
             (void)fflush(stderr);
          }
       }
    }


    psrp_object_list[slot_index].aliases           = 0;
    psrp_object_list[slot_index].aliases_allocated = 0;
    tag_index                                      = psrp_get_tag_index(slot_index);
    bag_des                                        = pups_open(bag_file_name,0,FALSE);

    if(h_mode == LIVE)
    {  char homeostat_name[SSIZE] = "";

       (void)snprintf(homeostat_name,SSIZE,"default_fd_homeostat: %s",bag_file_name);
       (void)pups_creator(bag_des);
       (void)pups_fd_alive(bag_des,homeostat_name,&pups_default_fd_homeostat);
    }   
 
    bag_size = 0;
    do {

           /*----------------------------------------------------------*/
           /* Read the contents into bag allocating memory dynamically */
           /*----------------------------------------------------------*/

           bag_size += PSRP_BAG_TABLE_SIZE;
           bag_handle     = (_BYTE *)pups_realloc((void *)bag_handle,bag_size);
           bag_bytes_read = pups_pipe_read(bag_des,bag_handle,PSRP_BAG_TABLE_SIZE);
       } while(bag_bytes_read == PSRP_BAG_TABLE_SIZE);

    if(psrp_object_list[slot_index].object_tag[tag_index] == (char *)NULL) 
       psrp_object_list[slot_index].object_tag[tag_index] = (char *)pups_malloc(SSIZE);

    if(psrp_object_list[slot_index].object_f_name == (char *)NULL)
       psrp_object_list[slot_index].object_f_name = (char *)pups_malloc(SSIZE);

    if(psrp_object_list[slot_index].object_handle == (void *)NULL)
       psrp_object_list[slot_index].object_handle = (void *)bag_handle;

    psrp_object_list[slot_index].object_size = bag_size;
    psrp_object_list[slot_index].object_type = PSRP_DYNAMIC_DATABAG;

    (void)strlcpy(psrp_object_list[slot_index].object_tag[tag_index],object_tag,SSIZE);
    (void)strlcpy(psrp_object_list[slot_index].object_f_name,bag_file_name,SSIZE);

    if(appl_verbose == TRUE)
    {  (void)fprintf(stderr,"%s %s (%d@%s:%s): dynamic databag \"%-32s\" attached (from %s at %016lx virtual)\n",
                                                                                                            date,
                                                                                                       appl_name,
                                                                                                        appl_pid,
                                                                                                       appl_host,
                                                                                                      appl_owner,
                                                                                                      object_tag,
                                                                                                   bag_file_name,
                                                                                   (unsigned long int)bag_handle);
       (void)fflush(stderr);
    }

    pups_set_errno(OK); 
    return(PSRP_OK);
}





#ifdef PERSISTENT_HEAP_SUPPORT
/*----------------------------------------------------------------------------
    Attach a persistent heap to the PSRP handler dispatch list ...
----------------------------------------------------------------------------*/

_PUBLIC int psrp_attach_persistent_heap(const _BOOLEAN      search,    // If TRUE search DISPATCH_PATH directories for heap
                                        const char       *heap_tag,    // (Root) tag of persistent heap
                                        const char *heap_file_name,    // Name of file containing heap data
                                        const int           h_mode)    // Heap homeostatsis mode
 
{   int  slot_index,
         tag_index;

    if(heap_tag == (const char *)NULL || heap_file_name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(PSRP_DISPATCH_ERROR);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_attach_persistent_heap] attempt by non root thread to perform PUPS/P3 PSRP operation");


    /*-------------------------------------------------*/ 
    /* Check to see if object exists on specified path */
    /* if it doesn't search the directories specified  */
    /* in the DISPATCH_PATH environment variable       */
    /*-------------------------------------------------*/

    if(access(heap_file_name,F_OK | R_OK | W_OK) == (-1))
    {  if(search == TRUE)
       {  char tmp_str[SSIZE] = "";

          if(strrext(heap_file_name,tmp_str,'/') == TRUE)
             (void)strlcpy(heap_file_name,tmp_str,SSIZE);

          if(strccpy(tmp_str,(char *)pups_search_path("DISPATCH_PATH",heap_file_name)) == (char *)NULL)
          {  pups_set_errno(EEXIST);
             return(PSRP_DISPATCH_ERROR);
          }
          (void)strlcpy(heap_file_name,tmp_str,SSIZE);
       }
       else
       {  pups_set_errno(EEXIST);
          return(PSRP_DISPATCH_ERROR);
       }
    }

    if((slot_index = psrp_find_action_slot_index(heap_tag)) == (-1))
        slot_index = psrp_get_action_slot_index();
    else
    {  if(appl_verbose == FALSE)
       {  if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@%s:%s): overwriting existing object %s\n",
                                 date,appl_name,appl_pid,appl_host,appl_owner,heap_tag);
             (void)fflush(stderr);
          }
       }
    }

 
    if(access(heap_file_name,F_OK | R_OK | W_OK) == (-1) || heap_tag == (char *)NULL)  
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    psrp_object_list[slot_index].aliases           = 0;
    psrp_object_list[slot_index].aliases_allocated = 0;

    tag_index   = psrp_get_tag_index(slot_index);   

    if((psrp_object_list[slot_index].hid = msm_heap_attach(heap_file_name,h_mode)) == (-1))
       return(PSRP_DISPATCH_ERROR);

    if(psrp_object_list[slot_index].object_tag[tag_index] == (char *)NULL)
       psrp_object_list[slot_index].object_tag[tag_index] = (char *)pups_malloc(SSIZE);

    if(psrp_object_list[slot_index].object_f_name == (char *)NULL)
       psrp_object_list[slot_index].object_f_name = (char *)pups_malloc(SSIZE);

    psrp_object_list[slot_index].object_handle         = (void *)htable[psrp_object_list[slot_index].hid].addr;
    psrp_object_list[slot_index].object_size           = (-1);
    psrp_object_list[slot_index].object_type           = PSRP_PERSISTENT_HEAP;

    (void)strlcpy(psrp_object_list[slot_index].object_tag[tag_index],heap_tag,SSIZE);
    (void)strlcpy(psrp_object_list[slot_index].object_f_name,heap_file_name,SSIZE);

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): persistent heap \"%-32s\" attached (from %s at %016lx virtual)\n",
                                                                                                            date,
                                                                                                       appl_name,
                                                                                                        appl_pid,
                                                                                                       appl_host,
                                                                                                      appl_owner,
                                                                                                        heap_tag,
                                                                                                  heap_file_name,
                                                (unsigned long int)htable[psrp_object_list[slot_index].hid].addr);
       (void)fflush(stderr);
    }

    pups_set_errno(OK);
    return(PSRP_OK);
}
#endif /* PERSISTENT_HEAP_SUPPPRT */

 


/*----------------------------------------------------------------------------
    Attach an action function to the PSRP handler dispatch list ...
----------------------------------------------------------------------------*/

_PUBLIC int psrp_attach_static_function(const char    *object_tag,
                                        const void *object_handle)
 
{   int slot_index,
        tag_index;


    if(object_tag == (const char *)NULL || object_handle == (const void *)NULL)
    {  pups_set_errno(EINVAL);
       return(PSRP_DISPATCH_ERROR);
    }
 

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_attach_static_function] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if((slot_index = psrp_find_action_slot_index(object_tag)) == (-1))
        slot_index = psrp_get_action_slot_index();
    else
    {  if(appl_verbose == FALSE)
       {  if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@%s:%s): overwriting existing object %s\n",
                               date,appl_name,appl_pid,appl_host,appl_owner,object_tag);
             (void)fflush(stderr);
          }
       }
    }

    psrp_object_list[slot_index].aliases               = 0;
    psrp_object_list[slot_index].aliases_allocated     = 0;
    tag_index                                          = psrp_get_tag_index(slot_index);

    if(psrp_object_list[slot_index].object_tag[tag_index] == (char *)NULL)
       psrp_object_list[slot_index].object_tag[tag_index] = (char *)pups_malloc(SSIZE);

    psrp_object_list[slot_index].object_handle         = object_handle;
    psrp_object_list[slot_index].object_size           = 0;
    psrp_object_list[slot_index].object_type           = PSRP_STATIC_FUNCTION;

    (void)strlcpy(psrp_object_list[slot_index].object_tag[tag_index],object_tag,SSIZE);
 
    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): static function \"%-32s\" attached (at offset %016lx virtual)\n",
                                                                                                           date,
                                                                                                      appl_name,
                                                                                                       appl_pid,
                                                                                                      appl_host,
                                                                                                     appl_owner,
                                                                                                     object_tag,
                                                                               (unsigned long int)object_handle);
       (void)fflush(stderr);
    }

    pups_set_errno(OK);
    return(PSRP_OK);
}




#ifdef DLL_SUPPORT
/*----------------------------------------------------------------------------
    Routine to load a dynamic object into one of the slots of the PSRP
    system. This object must conform to PSRP semantics - e.g it must
    be a pointer to an int function with arguments of int argc and
    char **argv ...
----------------------------------------------------------------------------*/
 
_PUBLIC int psrp_attach_dynamic_function(const _BOOLEAN  search,  // Search for DLL containing object
                                         const char *object_tag,  // Tag of dyanmic object (function)
                                         const char   *dll_name)  // Name of DLL contaaining object
 
{   int  slot_index,
         tag_index;

    char dll_path_name[SSIZE] = "";
 
    void *object_handle = (void *)NULL;


    if(object_tag == (const char *)NULL || dll_name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(PSRP_DISPATCH_ERROR);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_attach_dynamic_function] attempt by non root thread to perform PUPS/P3 PSRP operation");


    /*------------------------------------------------*/
    /* Check to see if object exist on specified path */
    /* if it doesn't search the directories specified */
    /* in the DISPATCH_PATH environment variable      */
    /*------------------------------------------------*/

    if(access(dll_name,F_OK | R_OK | W_OK) == (-1))
    {  if(search == TRUE)
       {  char tmp_str[SSIZE] = ""; 

          if(strrext(dll_name,tmp_str,'/') == TRUE)
             (void)strlcpy(dll_path_name,tmp_str,SSIZE);
          else
          {  pups_set_errno(EEXIST);
             return(PSRP_DISPATCH_ERROR);
          }

          if(strccpy(tmp_str,pups_search_path("DISPATCH_PATH",dll_path_name)) == (char *)NULL)
          {  pups_set_errno(EEXIST);
             return(PSRP_DISPATCH_ERROR);
          }
          (void)strlcpy(dll_path_name,tmp_str,SSIZE);
       }
       else
       {  pups_set_errno(EEXIST);
          return(PSRP_DISPATCH_ERROR);
       }
    }   
    else
       (void)strlcpy(dll_path_name,dll_name,SSIZE);


    /*--------------------------------*/
    /* Try to load the dynamic object */
    /*--------------------------------*/

 
    if((object_handle = pups_bind_orifice_handle(dll_path_name,
                                                 object_tag,
                                                 "int (*)(int, char *[])")) == (void *)NULL)
    {  pups_set_errno(EEXIST);
       return(PSRP_DISPATCH_ERROR);
    }

    if((slot_index = psrp_find_action_slot_index(object_tag)) == (-1))
        slot_index = psrp_get_action_slot_index();
    else
    {  if(appl_verbose == TRUE)
       {  if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@%s:%s): overwriting existing object %s\n",
                               date,appl_name,appl_pid,appl_host,appl_owner,object_tag);
             (void)fflush(stderr);
          }
       }
    }


    psrp_object_list[slot_index].aliases               = 0;
    psrp_object_list[slot_index].aliases_allocated     = 0;
    tag_index                                          = psrp_get_tag_index(slot_index);

    if(psrp_object_list[slot_index].object_tag[tag_index] == (char *)NULL)
       psrp_object_list[slot_index].object_tag[tag_index] = (char *)pups_malloc(SSIZE);

    if(psrp_object_list[slot_index].object_f_name == (char *)NULL)
       psrp_object_list[slot_index].object_f_name = (char *)pups_malloc(SSIZE);

    psrp_object_list[slot_index].object_handle         = object_handle;
    psrp_object_list[slot_index].object_size           = 0;
    psrp_object_list[slot_index].object_type           = PSRP_DYNAMIC_FUNCTION;
 
    (void)strlcpy(psrp_object_list[slot_index].object_tag[tag_index],object_tag,SSIZE);
    (void)strlcpy(psrp_object_list[slot_index].object_f_name,dll_name,SSIZE);

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): dynamic function \"%-32s\" attached (from %s at %016lx virtual)\n",
                                                                                                             date,
                                                                                                        appl_name,
                                                                                                         appl_pid,
                                                                                                        appl_host,
                                                                                                       appl_owner,
                                                                                                       object_tag,
                                                                                                         dll_name,
                                                                                 (unsigned long int)object_handle);
       (void)fflush(stderr);
    }

    pups_set_errno(OK);
    return(PSRP_OK);
}
#endif  /* DLL_SUPPORT */
 
 


/*----------------------------------------------------------------------------
    Show function currently bound to PSRP action list ...
----------------------------------------------------------------------------*/

_PUBLIC void psrp_show_object_list(void)

{   int  i;
    char pname[SSIZE] = "";


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_show_object_list] attempt by non root thread to perform PUPS/P3 PSRP operation");

    (void)fprintf(psrp_out,"\n\n    PSRP server status\n");
    (void)fprintf(psrp_out,"    ==================\n\n");
    (void)fflush(psrp_out);

    #ifdef SECURE
    if(strcmp(HD_DEVICE,"/dev/dummy") == 0)
       (void)fprintf(psrp_out,"    %s (%d@%s) using soft dongle authentication (for licencing)\n",
                                                                                         appl_name,
                                                                                          appl_pid,
                                                                                         appl_host);
    else
       (void)fprintf(psrp_out,"    %s (%d@%s) using disk serial number authentication (for licensing)\n",
                                                                                                appl_name,
                                                                                                 appl_pid,
                                                                                                appl_host);

    #ifdef SINGLE_HOST_LICENSE
    (void)fprintf(psrp_out,"    %s (%d@%s) has single copy single host license\n",
                                                                        appl_name,
                                                                         appl_pid,
                                                                        appl_host);
    #else
    (void)fprintf(psrp_out,"    %s (%d@%s) has single copy multiple host license\n",
                                                                           appl_name,
                                                                            appl_pid,
                                                                           appl_host);
    #endif /* SINGLE_HOST_LICENSE */

    #else
    (void)fprintf(psrp_out,"    %s (%d@%s) has multiple copy multiple host (unrestricted) license\n",
                                                                                            appl_name,
                                                                                             appl_pid,
                                                                                            appl_host);
    #endif /* SECURE */

   
    #ifdef PSRP_AUTHENTICATE
    if(appl_secure == TRUE)
    {  (void)fprintf(psrp_out,
                  "    %s (%d@%s) is secure (client authentication enabled)\n",
                                                                     appl_name,
                                                                      appl_pid,
                                                                     appl_host);
       (void)fflush(psrp_out);
    }
    #endif /* PSRP_AUNTHENTICATE */

    #ifdef MAIL_SUPPORT


    /*---------*/
    /* Postbox */
    /*---------*/

    if(appl_mailable == TRUE)
    {  if(strcmp(appl_mime_type,"all") == 0)
          (void)fprintf(psrp_out,"    %s (%d@%s) has postbox \"%s\" (extracting all MIME message parts)\n",
                                                                                                  appl_name,
                                                                                                   appl_pid,
                                                                                                  appl_host,
                                                                                                  appl_mdir);
       else
          (void)fprintf(psrp_out,"    %s (%d@%s) has postbox \"%s\" (extracting MIME message part \"%s\")\n",
                                                                                                    appl_name,
                                                                                                     appl_pid,
                                                                                                    appl_host,
                                                                                                    appl_mdir,
                                                                                               appl_mime_type);
    }


    /*------------*/
    /* No postbox */
    /*------------*/

    else
       (void)fprintf(psrp_out,"    %s (%d@%s) does not have a postbox (does not support e-mail interaction)\n",appl_name,appl_pid,appl_host);
    (void)fflush(psrp_out);
    #endif /* MAIL_SUPPORT */



    #ifndef NO_NET
    if(appl_rooted == TRUE)
    {  (void)fprintf(psrp_out,"    %s (%d@%s) is rooted (system context cannot migrate)\n",appl_name,appl_pid,appl_host);
       (void)fflush(psrp_out);
    }
    else
    {  (void)fprintf(psrp_out,"    %s (%d@%s) is not rooted (system context can migrate via tunnel daemon)\n",appl_name,appl_pid,appl_host);
       (void)fflush(psrp_out);
    }     
    #endif /* NO_NET */

    (void)psrp_pid_to_pname(appl_ppid,pname);
    if(appl_ppid_exit == TRUE)
       (void)fprintf(psrp_out,"    %s (%d@%s) will exit when parent process %d (\"%s\") terminates\n",appl_name,appl_pid,appl_host,appl_ppid,pname);
    else
       (void)fprintf(psrp_out,"    %s (%d@%s) will not exit when parent process %d (\"%s\") terminates\n",appl_name,appl_pid,appl_host,appl_ppid,pname);

    (void)fprintf(psrp_out,"\n");
    (void)fflush(psrp_out);

    if(psrp_object_list_size == 0)
    {  (void)fprintf(psrp_out,"    No PSRP objects bound to dispatch handler (%04d slots free)\n",psrp_object_list_used);
       (void)fflush(psrp_out);
    }
    else
    {  (void)fprintf(psrp_out,"    %d PSRP object slots in use\n",psrp_object_list_size);
       (void)fprintf(psrp_out,"    PSRP objects bound to %s (%d@%s)\n\n",appl_name,appl_pid,appl_host);
       (void)fflush(psrp_out);

       for(i=0; i<psrp_object_list_used; ++i)
       {  if(psrp_object_list[i].object_handle != (void *)NULL)
          {  (void)fprintf(psrp_out,"    %04d: \"%-32s\" (entry at %016lx virtual)    ",i,
                                                        psrp_object_list[i].object_tag[0],
                                     (unsigned long int)psrp_object_list[i].object_handle);
              (void)fflush(psrp_out);

              switch(psrp_object_list[i].object_type)
              {   case PSRP_STATIC_DATABAG:    (void)fprintf(psrp_out,"  Static databag (%d bytes)",
                                                                    psrp_object_list[i].object_size);
   
                                               break; 

                  case PSRP_PERSISTENT_HEAP:   (void)fprintf(psrp_out,"  Persistent heap(%d bytes)\n",
                                                                      psrp_object_list[i].object_size);
                                               break;

                  case PSRP_DYNAMIC_DATABAG:   (void)fprintf(psrp_out,"  Dynamic databag (%d bytes)\n",
                                                                       psrp_object_list[i].object_size);
                                               break;

                  case PSRP_DYNAMIC_FUNCTION:  (void)fprintf(psrp_out,"  Dynamic function\n");
                                               break;

                  case PSRP_STATIC_FUNCTION:   (void)fprintf(psrp_out,"  Static function\n");
                                               break;

                  default:                     break;
              }
          }

          (void)fflush(psrp_out);
       }

       if(psrp_object_list_used > 1)
          (void)fprintf(psrp_out,"\n\n    %04d PSRP objects bound to dispatch handler (%04d slots free)\n\n",psrp_object_list_used,psrp_object_list_used - psrp_object_list_size);
       else if(psrp_object_list_used  == 1)
          (void)fprintf(psrp_out,"\n\n    %04d PSRP object bound to dispatch handler (%04d slots free)\n\n",1,psrp_object_list_used - 1);

       (void)fflush(psrp_out);
    }

    if(appl_wait == TRUE)
       (void)fprintf(psrp_out,"\n\n    Server state is \"wait\"\n");
    else
       (void)fprintf(psrp_out,"\n\n    Server state is \"run\"\n");

    (void)fflush(psrp_out);
}




/*----------------------------------------------------------------------------
    Routine to re-initialise the dispatch table -- all dynamic objects are
    detached and all static objects are restored to their initial states
    (e.g. any aliases associated with static objects are removed) ...
----------------------------------------------------------------------------*/

_PUBLIC void psrp_reset_dispatch_table(void)

{   int i;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_reset_dispatch_table] attempt by non root thread to perform PUPS/P3 PSRP operation");

    for(i=0; i<psrp_object_list_used; ++i)
    {  if(psrp_object_list[i].object_handle != (void *)NULL && psrp_object_list[i].aliases > 0)
          (void)psrp_detach_object(i); 

    }
}




/*----------------------------------------------------------------------------
    Routine to detach object from dispatch list by name ...
----------------------------------------------------------------------------*/
 
_PUBLIC _BOOLEAN psrp_detach_object_by_name(const char *object_tag)
 
{    int i,
         slot_index;


    if(object_tag == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(FALSE);
    }

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_detach_object_by_name] attempt by non root thread to perform PUPS/P3 PSRP operation");

    for(i=0; i<psrp_object_list_used; ++i)
    {  if(psrp_object_list[i].object_handle != (void *)NULL && (slot_index = psrp_isearch_tag_list(object_tag,i)) >= 0)
       {  

          if(appl_verbose == FALSE)
          {  if(appl_verbose == TRUE)
             {  (void)strdate(date);
                (void)fprintf(stderr,"%s %s (%d@%s:%s): object \"%-32s\" (index %d at %016lx virtual) detached\n",
                                                                     date,appl_name,appl_pid,appl_host,appl_owner,
                                                                          psrp_object_list[slot_index].object_tag,
                                                                                                       slot_index,
                                                    (unsigned long int)psrp_object_list[slot_index].object_handle);
                (void)fflush(stderr);
             }
          }

          pups_set_errno(OK);
          return(psrp_detach_object(i));
       }
    }

    pups_set_errno(ESRCH);
    return(PSRP_DISPATCH_ERROR);
}




/*----------------------------------------------------------------------------
    Routine to detach object from dispatch list by name ...
----------------------------------------------------------------------------*/
 
_PUBLIC _BOOLEAN psrp_detach_object_by_handle(const void *object_handle)
 
{   int i;
 
    if(object_handle == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(FALSE);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_detach_object_by_handle] attempt by non root thread to perform PUPS/P3 PSRP operation");

    for(i=0; i<psrp_object_list_used; ++i)
    {  if(psrp_object_list[i].object_handle != (void *)NULL && psrp_object_list[i].object_handle == object_handle)
       { 

          if(appl_verbose == FALSE)
          {  if(appl_verbose == TRUE)
             {  (void)strdate(date);
                (void)fprintf(stderr,"%s %s (%d@%s:%s): object \"%-32s\" (index %d at %016lx virtual) detached\n",
                                                                     date,appl_name,appl_pid,appl_host,appl_owner,
                                                                                   psrp_object_list[i].object_tag,
                                                                                                                i,
                                                             (unsigned long int)psrp_object_list[i].object_handle);
                (void)fflush(stderr);
             }
          }

          pups_set_errno(OK);
          return(psrp_detach_object(i));
       } 
    }

    pups_set_errno(OK);
    return(PSRP_DISPATCH_ERROR);
}
 
 
 
 
/*--------------------------------------------------------------------*/
/* Routine to detach an object taking note of its type and contextual */
/* information                                                        */
/*--------------------------------------------------------------------*/

_PRIVATE int psrp_detach_object(unsigned int slot_index)
 
{   int i,
        type;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_detach_object] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(slot_index > psrp_object_list_used)
    {  pups_set_errno(EINVAL);
       return(PSRP_DISPATCH_ERROR);
    }


    /*-----------------------------------------------------------------------*/
    /* Log object deletion (note wrapper function has already checked index  */
    /* validity)                                                             */
    /*-----------------------------------------------------------------------*/

    if(appl_verbose == TRUE)
    {  (void)strdate(date);

       if(psrp_object_list[slot_index].object_type == PSRP_STATIC_FUNCTION ||
          psrp_object_list[slot_index].object_type == PSRP_STATIC_DATABAG   )
       {  if(psrp_object_list[slot_index].aliases > 1)
             (void)fprintf(stderr,"%s %s (%d@%s:%s): static PSRP object \"%s\" reinitialised (%d alias(es) deleted)\n",
                                                                                                                  date,
                                                                                                             appl_name,
                                                                                                              appl_pid,
                                                                                                             appl_host,
                                                                                                            appl_owner,
                                                                            psrp_object_list[slot_index].object_tag[0],
                                                                              psrp_object_list[slot_index].aliases - 1);
          else
             (void)fprintf(stderr,"%s %s (%d@%s:%s): static PSRP object \"%s\" is in intial state (no aliases)\n",
                                                                                                             date,
                                                                                                        appl_name,
                                                                                                         appl_pid,
                                                                                                        appl_host,
                                                                                                       appl_owner,
                                                                       psrp_object_list[slot_index].object_tag[0]);
       }
       else
          (void)fprintf(stderr,"%s %s (%d@%s:%s): dynamic PSRP object \"%s\" detached\n",
                                                                                    date,
                                                                               appl_name,
                                                                                appl_pid,
                                                                               appl_host,
                                                                              appl_owner,
                                              psrp_object_list[slot_index].object_tag[0]);
       (void)fflush(stderr);
    }


    /*--------------------------------------------------------------------*/
    /* In the case of a static object - we simply reset it to its initial */
    /* state. That means all aliases must be removed                      */
    /*--------------------------------------------------------------------*/

    if(psrp_object_list[slot_index].object_type == PSRP_STATIC_FUNCTION ||
       psrp_object_list[slot_index].object_type == PSRP_STATIC_DATABAG   )
    {  for(i=1; i< psrp_object_list[slot_index].aliases_allocated; ++i)
           if(psrp_object_list[slot_index].object_tag[i] != (char *)NULL)
              psrp_object_list[slot_index].object_tag[i] = pups_free((void *)psrp_object_list[slot_index].object_tag[i]);

       psrp_object_list[slot_index].aliases = 1;

       pups_set_errno(OK);
       return(psrp_object_list[slot_index].object_type);
    }


    /*-------------------------------------------------------*/   
    /* Object is dynamic - process it according to its type. */
    /*-------------------------------------------------------*/    


    /*--------------------------------------------------------------------------*/
    /* If the object is dynamic, the type of processing is dependent on         */
    /* whether it is a dynamic function, a dynamic databag of a persistentheap. */
    /* In all cases the slot in the dispatch table is cleared, and any          */
    /* resources associated with the object are released.                       */
    /*--------------------------------------------------------------------------*/

    type = psrp_object_list[slot_index].object_type;
    switch(type)
    {
 
       #ifdef DLL_SUPPORT
       case PSRP_DYNAMIC_FUNCTION:  /*--------------------------------------*/
                                    /* Only dynamic objects can be detached */
                                    /*--------------------------------------*/

                                    if(pups_free_orifice_handle(psrp_object_list[slot_index].object_handle) != (void *)NULL)
                                       return(PSRP_DISPATCH_ERROR);
 
                                    break;
       #endif /* DLL_SUPPORT */
 
       case PSRP_DYNAMIC_DATABAG:   /*----------------------------------------------------------------*/
                                    /* A databag can always be detached given that we have permission */
                                    /* To allocate it in the first place                              */
                                    /*----------------------------------------------------------------*/

                                    (void)pups_free((char *)psrp_object_list[slot_index].object_handle);
                                    break;

       #ifdef PERSISTENT_HEAP_SUPPORT
       case PSRP_PERSISTENT_HEAP:   /*------------------------*/ 
                                    /* Detach persistent heap */
                                    /*------------------------*/
 
                                    (void)msm_heap_detach(psrp_object_list[slot_index].hid,O_DESTROY);
                                    break;
       #endif /* PERSISTENT_HEAP_SUPPORT */
 
       default:                     break;
    }


    /*----------------------------------------------------*/ 
    /* Delete other attributes associated with the object */
    /*----------------------------------------------------*/

    for(i=0; i< psrp_object_list[slot_index].aliases_allocated; ++i)
        if(psrp_object_list[slot_index].object_tag[i] != (char *)NULL)
           (void)pups_free((void *)psrp_object_list[slot_index].object_tag[i]);
 
    (void)pups_free((void *)psrp_object_list[slot_index].object_tag);
    (void)pups_free((void *)psrp_object_list[slot_index].object_f_name);

    psrp_object_list[slot_index].object_tag        = (char **)NULL;
    psrp_object_list[slot_index].object_f_name     = (char *)NULL;
    psrp_object_list[slot_index].object_handle     = (void *)NULL;
    psrp_object_list[slot_index].object_type       = PSRP_NONE;
    psrp_object_list[slot_index].object_size       = 0;
    psrp_object_list[slot_index].object_state      = PSRP_NONE;
    psrp_object_list[slot_index].aliases           = 0;
    psrp_object_list[slot_index].aliases_allocated = 0;
    psrp_object_list[slot_index].hid               = (-1);
 
    --psrp_object_list_size;

    pups_set_errno(OK); 
    return(type);
}




/*----------------------------------------------------------*/
/* Handler for client disconnect in response to server stop */
/*----------------------------------------------------------*/

_PRIVATE int cdoss_handler(int signum)
{   psrp_chbrk_handler(c_client,PSRP_CLIENT_TERMINATED);

    if(in_psrp_handler == TRUE)
       (void)pups_sigvector(SIGALRM, &psrp_env);

    return(0);
}



/*----------------------------------------------------------*/
/* Handler for client disconnect in response to server stop */
/*----------------------------------------------------------*/

_PUBLIC int psrp_cont_handler(const int signum)
{   

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_cont_handler] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(in_psrp_handler == TRUE)
       (void)pups_sigvector(SIGALRM, &psrp_env);

    return(0);
}




/*-------------------------------------*/
/* Register client side abort callback */
/*-------------------------------------*/

_PROTOTYPE _PUBLIC _BOOLEAN (*on_abort_callback_f)(void);
_PRIVATE   char             on_abort_callback_f_name[SSIZE] = "";

_PUBLIC int psrp_register_on_abort_callback_f(const char *on_abort_f_name, const void *on_abort_f)

{   

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[register_psrp_on_abort] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(on_abort_f_name == (const char *)NULL   ||
       on_abort_f      == (const void *)NULL    )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    on_abort_callback_f = on_abort_f;
    (void)strlcpy(on_abort_callback_f_name,on_abort_f_name,SSIZE);

    pups_set_errno(OK);
    return(0);
}




/*---------------------------------------*/
/* Deregister client side abort callback */
/*---------------------------------------*/

_PUBLIC void psrp_deregister_on_abort_callback_f(void)

{

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[register_psrp_on_abort] attempt by non root thread to perform PUPS/P3 PSRP operation");

     on_abort_callback_f = (void *)NULL;   
     (void)strlcpy(on_abort_callback_f_name,"",SSIZE);
}




/*-----------------------------------------------------*/
/* Display client side abort callback function details */
/*-----------------------------------------------------*/

_PUBLIC int psrp_show_on_abort_callback_f(const FILE *stream)

{   if(stream == (FILE *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if(on_abort_callback_f != (void *)NULL)
    {  (void)fprintf(stream,"\n    Client side abort callback function \"%-32s\" installed at %016lx virtual\n\n",
                                                  on_abort_callback_f_name,(unsigned long int)on_abort_callback_f);
       (void)fflush(stream);
    }

    pups_set_errno(OK);
    return(0);
}



 
/*--------------------------*/
/* Handler for client abort */
/*--------------------------*/

_PRIVATE int abrt_handler(int signum)

{   int i,
        f_index,
        client_pid;

    _BOOLEAN omp_abort          = FALSE,
             close_psrp_channel = FALSE;


    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,"PSRPLIB %d SIGABRT\n",appl_pid);
    (void)fflush(stderr);
    #endif /* PSRPLIB_DEBUG */


    /*---------------------------------*/
    /* Set interrupt flag (if enabled) */
    /*---------------------------------*/

    if(psrp_abrtflag_enable == TRUE)
       psrp_abrtflag = TRUE;


    /*--------------------*/
    /* Callback for abort */
    /*--------------------*/

    if((void *)on_abort_callback_f != (void *)NULL)
    { 

       /*----------------------------*/
       /* Abort callback function    */
       /* returns TRUE if aborted    */
       /* code is parallelised using */
       /* OpenMP                     */
       /*----------------------------*/

       omp_abort = on_abort_callback_f();
    } 


    /*-----------------------------------------*/
    /* Get pid of client which requested abort */
    /*-----------------------------------------*/

    if(psrp_in == (FILE *)NULL)
    {  

       /*------------------------------------------*/
       /* Note homeostasis for the PSRP channel is */
       /* pups_maintained by the psrp_homeostat    */ 
       /*------------------------------------------*/

       psrp_in  = pups_fopen(channel_name_in, "r+",DEAD);
       psrp_out = pups_fopen(channel_name_out,"r+",DEAD);
       close_psrp_channel = TRUE;
    }

    (void)efscanf(psrp_in,"%d",&client_pid);


    /*-----------------------------------------*/
    /* Execute registered PUPS abort functions */
    /*-----------------------------------------*/

    for(i=0; i<n_abort_funcs_alloc; ++i)
    {  if((void *)pups_abort_f[i] != (void *)NULL)
          (*pups_abort_f[i])(pups_abort_arg[i]);
    }


    /*-----------------------------------*/
    /* Tell psrp client we have finished */
    /* handling abort operation          */
    /*-----------------------------------*/
                            
    psrp_endop("abrt");


    /*------------------------------------------------*/
    /* If we have an overforked child -- terminate it */
    /*------------------------------------------------*/

    if(overforking == TRUE)
    {  (void)kill(overforked_child_pid,SIGTERM);
       overforking          = FALSE;
       overforked_child_pid = (-1);
    }

    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,""PSRPLIB %d SIGABRT stage 2\n",appl_pid);
    (void)fflush(stderr);
    #endif /* PSRPLIB_DEBUG */


    /*-----------------------------------------------------------*/
    /* If we are talking to a slaved client forward signal to it */
    /* and destroy channel                                       */
    /*-----------------------------------------------------------*/

    if(psrp_current_sic != (psrp_channel_type *)NULL)
    {  

       #ifdef PSRPLIB_DEBUG
       (void)fprintf(stderr,""PSRPLIB %d SIGABRT stage 3 (propagate to SIC: %d)\n",appl_pid,psrp_current_sic->scp);
       (void)fflush(stderr);
       #endif /* PSRPLIB_DEBUG */

       (void)kill(psrp_current_sic->scp,SIGINT);

       #ifdef PSRPLIB_DEBUG
       (void)fprintf(stderr,""PSRPLIB %d SIGABRT stage 4: SIC closed\n",appl_pid);
       (void)fflush(stderr);
       #endif /* PSRPLIB_DEBUG */

    }

    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,""PSRPLIB %d SIGABRT stage 5: abrt sent\n",appl_pid);
    (void)fflush(stderr);
    #endif /* PSRPLIB_DEBUG */

    if(close_psrp_channel == TRUE)
    {  psrp_in  = pups_fclose(psrp_in);
       psrp_out = pups_fclose(psrp_out);
    }

    if(appl_verbose == TRUE)
    {  (void)strdate(date); 

       (void)fprintf(stderr,"%s %s (%d@%s:%s): PSRP interrupt received (from PSRP client \"%s\" %d@%s)\n",
                     date,appl_name,appl_pid,appl_host,appl_owner,psrp_client_name[c_client],client_pid,appl_host);
       (void)fflush(stderr);
    }


    /*---------------------------------------------------*/
    /* If we were talking to a socket close it down now  */
    /* so resources on remote end of socket are released */
    /*---------------------------------------------------*/

    if((f_index = pups_get_ftab_index_by_id(c_client)) != (-1))
    {  if(appl_verbose == TRUE)
       {  (void)strdate(date);

          (void)fprintf(stderr,"%s %s (%d@%s): datalink [%s] closed\n",
                        date,appl_name,appl_pid,appl_host,appl_owner,ftab[f_index].fname);
          (void)fflush(stderr);
       }

       if(ftab[f_index].stream != (FILE *)NULL)
          (void)pups_fclose(ftab[f_index].stream);
       else
          (void)pups_close(ftab[f_index].fdes);
    }


    /*-------------------------------------------------------------*/
    /* Client exit handler is provided so any application specific */
    /* files etc can be cleaned up when PSRP channel is closed     */
    /*-------------------------------------------------------------*/

    if((void *)psrp_client_exitf[c_client] != (void *)NULL)
    {  (void)(*psrp_client_exitf[c_client])(c_client);
       psrp_client_exitf[c_client] = (void *)NULL;

       if(appl_verbose == TRUE)
       {  (void)fprintf(stderr,"%s %s (%d@%s:%s): PSRP client connection exit handler \"%s\" executed\n",
                               date,appl_name,appl_pid,appl_host,appl_owner,psrp_client_efname[c_client]);
          (void)fflush(stderr);
       }

       (void)strlcpy(psrp_client_efname[c_client],"",SSIZE);
    }

    if(req_r_cnt[c_client] > 0)
    {  if(appl_verbose == TRUE)
          (void)fprintf(stderr,"%s %s (%d@%s:%s): PSRP (protocol %5.2F) last request repeated %d times\n",
                                                                                                     date,
                                                                                                appl_name,
                                                                                                 appl_pid,
                                                                                                appl_host,
                                                                                               appl_owner,
                                                                                    PSRP_PROTOCOL_VERSION,
                                                                                  req_r_cnt[c_client] + 1);
       req_r_cnt[c_client] = 0;
       (void)strlcpy(old_request_str[c_client],"",SSIZE);
    }


    /*-----------------------------------------*/
    /* The PUPS/P3 function abort is serial    */
    /* OMP parallel codes have to handle the   */
    /* client side of the abort themselves due */
    /* to limitations of OpemMP abort handling */
    /*-----------------------------------------*/

    if(omp_abort == FALSE)
    {

       /*---------------------------------------------------------------------*/
       /* We don't need to do anything unless we are in the PSRP handler code */
       /* or the chan handler code                                            */
       /*---------------------------------------------------------------------*/

       if(in_psrp_handler == TRUE)
       {  

          /*------------------------------------------------*/
          /* Need to adjust count here to stop mismatch exit*/
          /*------------------------------------------------*/

          ++pupsighold_cnt[SIGALRM];
          (void)pups_sigvector(SIGALRM, &psrp_env);
       }
       else if(in_chan_handler == TRUE)
       {  

          /*------------------------------------------------*/
          /* Need to adjust count here to stop mismatch exit*/
          /*------------------------------------------------*/

          ++pupsighold_cnt[SIGALRM];
          (void)pups_sigvector(SIGALRM, &chan_env);
       }
    }    

    return(0);
}



/*-----------------------------------------------------------------------*/
/* Handler for SIGCHAN -- supplies information about server to connected */
/* PSRP client function                                                  */
/*-----------------------------------------------------------------------*/

_PRIVATE int chan_handler(int signum)

{   sigset_t set;

    char  psrp_channel_name[SSIZE] = "",
          recreated[SSIZE]         = "",
          psrp_channel_op[SSIZE]   = "";

    _IMMORTAL _BOOLEAN entered = FALSE;


    /*-----------------------------------------------------*/
    /* Sometimes, because of latency, we can get a SIGCHAN */
    /* raised in psrp_new_segment - this is spurious and   */
    /* must not be processed                               */
    /*-----------------------------------------------------*/

    if(in_psrp_new_segment == TRUE)
       return(0);

    in_chan_handler = TRUE;
    if(sigsetjmp(chan_env,0) > 0)
    {  psrp_in  = pups_fclose(psrp_in);
       psrp_out = pups_fclose(psrp_out);

       in_chan_handler     = FALSE;

       return(0);
    }

    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,""PSRPLIB CHAN open (%d)\n",signum);
    (void)fflush(stderr);
    #endif /* PSRPLIB_DEBUG */


    /*-----------------------------------------*/
    /* Open an input client -> process channel */
    /*-----------------------------------------*/

    (void)snprintf(psrp_channel_name,SSIZE,"%s/psrp#%s#fifo#in#%d#%d",appl_fifo_dir,appl_ch_name,appl_pid,getuid());


    /*----------------------------------------------------------*/
    /* Homeostatsis for the psrp channels is pups_maintained by */
    /* the psrp_homeostat                                       */
    /*----------------------------------------------------------*/

    if((psrp_in = pups_fopen(psrp_channel_name,"r+",DEAD)) == (FILE *)NULL)
    {  if(appl_verbose == TRUE)
       {  (void)fprintf(stderr,
                  "error(%d@%s): failed to open input stream (%s) for status request\n",appl_ch_name,appl_pid,appl_host);
          (void)fflush(stderr);
       }

       in_chan_handler     = FALSE;
       return(0);
    }


    /*-------------------------------------------*/
    /* Open an output process -> client channel  */
    /*-------------------------------------------*/

    (void)snprintf(psrp_channel_name,SSIZE,"%s/psrp#%s#fifo#out#%d#%d",appl_fifo_dir,appl_ch_name,appl_pid,getuid());


    /*----------------------------------------------------------*/
    /* Homeostatsis for the psrp channels is pups_maintained by */
    /* the psrp_homeostat                                       */
    /*----------------------------------------------------------*/

    if((psrp_out = pups_fopen(psrp_channel_name,"r+",DEAD)) == (FILE *)NULL)
    {  if(appl_verbose == TRUE)
       {  (void)fprintf(stderr,
                  "error(%d@%s): failed to open output stream (%s) for status request\n",appl_ch_name,appl_pid,appl_host);
          (void)fflush(stderr);
       }

       psrp_in = pups_fclose(psrp_in);
       return(0);
    }

    ftab[pups_get_ftab_index(fileno(psrp_in))].psrp  = TRUE;
    ftab[pups_get_ftab_index(fileno(psrp_out))].psrp = TRUE;

    if(psrp_bind_status & PSRP_HOMEOSTATIC_STREAMS)
    {  ftab[pups_get_ftab_index(fileno(psrp_in)) ].homeostatic = 1;
       ftab[pups_get_ftab_index(fileno(psrp_out))].homeostatic = 1;
    }


    /*--------------------------------------------------------*/
    /* A client wants to connect to us -- service the request */
    /*--------------------------------------------------------*/

    if(signum == SIGINIT)
    {  int  psrp_op_pid;
       char tmp_str[SSIZE] = "";


       /*----------------------------*/
       /* Get PSRP channel operation */ 
       /*----------------------------*/

       (void)efgets(tmp_str,SSIZE,psrp_in);
       (void)sscanf(tmp_str,"%s %d %s",psrp_channel_op,&psrp_op_pid,psrp_password);

       #ifdef PSRPLIB_DEBUG
       (void)fprintf(stderr,""PSRPLIB CLIENT OP: %s\n",tmp_str);
       (void)fflush(stderr);
       #endif /* PSRPLIB_DEBUG */


       /*-------------------------------------------------------------------*/
       /* Newly segmented servers pick up any existing authentication token */
       /*-------------------------------------------------------------------*/

       if(psrp_seg_cnt > 0 && entered == FALSE && strcmp(psrp_password,"notset") != 0)
       {  entered     = TRUE;
          appl_secure = TRUE;
          (void)strlcpy(appl_password,psrp_password,SSIZE);

          if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s(%d@%s:%s): segmented server secure\n",
                            date,appl_name,appl_pid,appl_host,appl_owner);
             (void)fflush(stderr);
          }
       }

       #ifdef PSRPLIB_DEBUG
       (void)fprintf(stderr,""PSRPLIB SIGINIT (op = %s): password is %s:%s\n",psrp_channel_op,psrp_password,appl_password);
       (void)fflush(stderr);
       #endif /* PSRPLIB_DEBUG */

       if(strncmp(psrp_channel_op,"CLOSE",5) == 0)
       {  psrp_channel_open = FALSE;

          #ifdef PSRPLIB_DEBUG
          (void)fprintf(stderr,""PSRPLIB SIGINIT (disconnect)\n");
          (void)fflush(stderr);
          #endif /* PSRPLIB_DEBUG */


          /*------------------------------------------------------*/
          /* If we have a channel exit function installed it must */
          /* be executed now                                      */
          /*------------------------------------------------------*/

          if((void *)psrp_client_exitf[c_client] != (void *)NULL)
          {  (void)(*psrp_client_exitf[c_client])(c_client);
             psrp_client_exitf[c_client] = (void *)NULL;
             (void)strlcpy(psrp_client_efname[c_client],"",SSIZE);
          }

          if(appl_verbose == TRUE)
          {  char tmpstr[SSIZE] = "";

             (void)strdate(date);
             if(psrp_transactions[c_client] == 1)
                (void)strlcpy(tmpstr,"transaction",SSIZE);
             else
                (void)strlcpy(tmpstr,"transactions",SSIZE);

             if(strcmp(psrp_remote_hostpath[c_client],"notset") == 0)
                (void)fprintf(stderr,"%s %s (%d@%s:%s): client %s (%d@%s) has disconnected (%d %s)\n",
                                                                                                 date,
                                                                                            appl_name,
                                                                                             appl_pid,
                                                                                            appl_host,
                                                                                           appl_owner,
                                                                           psrp_client_name[c_client],
                                                                            psrp_client_pid[c_client],
                                                                           psrp_client_host[c_client],
                                                                          psrp_transactions[c_client],
                                                                                               tmpstr);
             else
                (void)fprintf(stderr,"%s %s (%d@%s:%s): client %s (%d@%s) has disconnected [from %s] (%d %s)\n",
                                                                                                           date,
                                                                                                      appl_name,
                                                                                                       appl_pid,
                                                                                                      appl_host,
                                                                                                     appl_owner,
                                                                                     psrp_client_name[c_client],
                                                                                      psrp_client_pid[c_client],
                                                                                     psrp_client_host[c_client],
                                                                                 psrp_remote_hostpath[c_client],
                                                                                    psrp_transactions[c_client],
                                                                                                        tmpstr);

             (void)fflush(stderr);
          }


          /*-------------------------------------------------------*/
          /* Clear this client from the table of connected clients */
          /*-------------------------------------------------------*/

          c_client = psrp_clear_client_slot(c_client);


          /*-------------------------------------------------------*/
          /* Tell client we have finished servicing SIGINT (CLOSE) */
          /*-------------------------------------------------------*/

          psrp_endop("close"); 

          psrp_in  = pups_fclose(psrp_in);
          psrp_out = pups_fclose(psrp_out);

          #ifdef PSRPLIB_DEBUG
          (void)fprintf(stderr,""PSRPLIB CHAN CLOSE close\n");
          (void)fflush(stderr);
          #endif /* PSRPLIB_DEBUG */

          in_chan_handler     = FALSE;

          return(0);
       }
       else if(strncmp(psrp_channel_op,"OPEN",4) == 0                                        ||
                      (strncmp(psrp_channel_op,"GRAB",4) == 0 && in_psrp_new_segment == FALSE))
       {  int  trys = 0,
               fdes = (-1);

          char client_info[SSIZE] = "";

          if(strncmp(psrp_channel_op,"OPEN",4) == 0)
          {  

             /*-------------------------------------------------------*/
             /* Flag the fact we have connected at least once in this */
             /* segment                                               */
             /*-------------------------------------------------------*/

             connected_once = TRUE;
          }


          /*---------------------------------------*/
          /* Find a free slot for the next client  */
          /* or return its slot if we have already */
          /* connected to it                       */
          /*---------------------------------------*/

          c_client = psrp_get_client_slot(psrp_op_pid);


          /*-----------------------------------------------------*/
          /* If we have no free virtual channels tell the client */
          /*-----------------------------------------------------*/

          if(c_client == ENOCH)
          {  (void)fprintf(psrp_out,"%s: PSRP connect error (no PSRP channels free)\n",appl_name);
             (void)fflush(psrp_out);

             if(appl_verbose == TRUE)
             {  (void)strdate(date);
                (void)fprintf(stderr,"%s %s (%d@%s:%s): PSRP connect error (no PSRP channels free)\n",
                                                         date,appl_name,appl_pid,appl_host,appl_owner);
                (void)fflush(stderr);
             }

             psrp_in  = pups_fclose(psrp_in);
             psrp_out = pups_fclose(psrp_out);

             in_chan_handler     = FALSE;
             connected_once      = FALSE;

             return(0);
          }
          else
          {  (void)fprintf(psrp_out,"%d\n",psrp_seg_cnt);
             (void)fflush(psrp_out);
          }

          #ifdef PSRPLIB_DEBUG
          if(strncmp(psrp_channel_op,"OPEN",4) == 0)
             (void)fprintf(stderr,""PSRPLIB SIGINIT (connect)\n");
          else
             (void)fprintf(stderr,""PSRPLIB SIGINIT (grab)\n");

          (void)fflush(stderr);
          #endif /* PSRPLIB_DEBUG */

          (void)efgets(client_info,SSIZE,psrp_in);

          #ifdef PSRPLIB_DEBUG
          (void)fprintf(stderr,""PSRPLIB CLIENT INFO %s\n",client_info);
          (void)fflush(stderr);
          #endif /* PSRPLIB_DEBUG */

          if(sscanf(client_info,"%s%d%s%s",psrp_client_name[c_client],
                                           &psrp_client_pid[c_client],
                                           psrp_client_host[c_client],
                                           psrp_remote_hostpath[c_client]) != 4)
             (void)strlcpy(psrp_remote_hostpath[c_client],"notset",SSIZE);


          /*---------------------------------------------------------------*/
          /* We can only flag channel open once we have a valid client pid */
          /* otherwise the homeostat will try to kill(0,SIGALIVE) with     */
          /* disasterous results                                           */
          /*---------------------------------------------------------------*/

          psrp_channel_open           = TRUE;

          if(strncmp(psrp_channel_op,"OPEN",4) == 0)
          {  if(appl_verbose == TRUE)
             {  strdate(date);

                if(strcmp(psrp_remote_hostpath[c_client],"notset") == 0)
                   (void)fprintf(stderr,"%s %s (%d@%s:%s): client %s (%d@%s) has connected\n",date,
                                                                                         appl_name,
                                                                                          appl_pid,
                                                                                         appl_host,
                                                                                        appl_owner,
                                                                        psrp_client_name[c_client],
                                                                         psrp_client_pid[c_client],
                                                                        psrp_client_host[c_client]);
                else
                   (void)fprintf(stderr,"%s %s (%d@%s:%s): client %s (%d@%s) has connected [from %s]\n",date,
                                                                                                   appl_name,
                                                                                                    appl_pid,
                                                                                                   appl_host,
                                                                                                  appl_owner,
                                                                                  psrp_client_name[c_client],
                                                                                   psrp_client_pid[c_client],
                                                                                  psrp_client_host[c_client],
                                                                              psrp_remote_hostpath[c_client]);

                (void)fflush(stderr);
             }
             psrp_transactions[c_client] = 0;
          }


          /*---------------------------------------------------------------*/
          /* Tell client we have finished servicing SIGINIT (grab or open) */
          /*---------------------------------------------------------------*/

          psrp_endop("grope");

          psrp_in  = pups_fclose(psrp_in);
          psrp_out = pups_fclose(psrp_out);

          #ifdef PSRPLIB_DEBUG
          (void)fprintf(stderr,""PSRPLIB CHAN GROPE close\n");
          (void)fflush(stderr);
          #endif /* PSRPLIB_DEBUG */

          in_chan_handler     = FALSE;

          return(0);
       }
    }

    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,""PSRPLIB SIGCHAN (%016lx)\n",(unsigned long int)psrp_out);
    (void)fflush(stderr);
    #endif /* PSRPLIB_DEBUG */


    /*------------------------------------------------*/
    /* Tell client we have finished servicing SIGCHAN */
    /*------------------------------------------------*/

    psrp_endop("chan");

    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,""PSRPLIB SIGCHAN DONE\n");
    (void)fflush(stderr);
    #endif /* PSRPLIB_DEBUG */

    in_chan_handler     = FALSE;
    return(0);
}




/*-------------------------------------------*/
/* Check PSRP communication FIFOS are intact */
/*-------------------------------------------*/

_PRIVATE void psrp_homeostat(void *t_info, char *args)

{    int idum  = 0,
         trys  = 0;

     if(psrp_channel_open == TRUE)
     {  int i,
            ret;

        char tcode[SSIZE] = "";


        /*------------------------------------------------------*/
        /* Check (non active) connected clients are still alive */
        /*------------------------------------------------------*/


        for(i=0; i<MAX_CLIENTS; ++i)
        { 
            /*------------------------------*/
            /* Do not process active client */
            /*------------------------------*/
 
            if(i != c_client && psrp_client_pid[i] != (-1)) 
            {  do {    ret = pups_statkill(psrp_client_pid[i],SIGALIVE); 
                       ++trys;
                  } while(trys < max_trys && ret < 0);

               if(ret < 0)
                  psrp_chbrk_handler(i,PSRP_CLIENT_TERMINATED);
            }
        }


        /*--------------------------------------------------*/
        /* Check that (active) current client is still alive */
        /*---------------------------------------------------*/

        do {    ret = pups_statkill(psrp_client_pid[c_client],SIGALIVE);
                   ++trys;
           } while(trys < max_trys && ret < 0);

        if(ret < 0)
           psrp_chbrk_handler(c_client,PSRP_CLIENT_TERMINATED);
     }

     if(access(channel_name_in,F_OK | R_OK | W_OK)  == (-1))
     {  (void)mkfifo(channel_name_in,0600);
        psrp_chbrk_handler(idum,PSRP_IPC_LOST);
     } 

     if(access(channel_name_out,F_OK | R_OK | W_OK) == (-1))
     {  (void)mkfifo(channel_name_out,0600);
        psrp_chbrk_handler(idum,PSRP_OPC_LOST);
     } 

     (void)pups_sigvector(SIGALRM,(sigjmp_buf *)NULL);
}




/*------------*/
/* Empty FIFO */
/*------------*/

_PUBLIC void empty_fifo(const int fdes)

{   long int bytes_out;
    _BYTE    ch_buf[512] = "";


return;

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[empty_fifo] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(fcntl(fdes,F_SETFL, fcntl(fdes,F_GETFL,0) | O_NONBLOCK) == (-1))
    {  pups_set_errno(errno);
       return;
    }

    do {    bytes_out  = (int)read(fdes,ch_buf,512);

            #ifdef _PSRPLIB_DEBUG
            (void)fprintf(stderr,"PSRPLIB fdes %d flushed (%d bytes out)\n",fdes,bytes_out);
            (void)fflush(stderr);
            #endif /* PSRPLIB_DEBUG */ 
 
       } while(bytes_out > 0);

    (void)fcntl(fdes,F_SETFL, fcntl(fdes,F_GETFL,0) & ~O_NONBLOCK);
}




/*----------------------------------------------*/
/* Handler for unexpected PSRP channel timeouts */
/*----------------------------------------------*/

_PUBLIC int psrp_chbrk_handler(const unsigned int c_index, const int event)

{   int  bytes_out,
         fdes_out = (-1); 

    char channel_name[SSIZE] = "";

    (void)snprintf(channel_name,SSIZE,"%s#%s/psrp#%s#%d#%d",appl_name,appl_fifo_dir,appl_ch_name,appl_pid,getuid()); 
    if(appl_verbose == TRUE)
    {  strdate(date);

       switch(event)
       {    case PSRP_CLIENT_TERMINATED:    if(strcmp(psrp_remote_hostpath[c_index],"notset") == 0)
                                            {  if(psrp_transactions[c_index] == 0)
                                                  (void)fprintf(stderr,
                                                                "%s %s (%d@%s:%s): connection to client (%s) broken on channel \"%s\" (0 client transactons)\n",
                                                                              date,appl_name,appl_pid,appl_host,appl_owner,psrp_client_name[c_index],channel_name);
                                               else if(psrp_transactions[c_index] == 1)
                                                  (void)fprintf(stderr,
                                                        "%s %s (%d@%s:%s): connection to client (%s) broken on channel \"%s\" (1 client transaction)\n",
                                                                      date,appl_name,appl_pid,appl_host,appl_owner,psrp_client_name[c_index],channel_name);
                                               else 
                                                  (void)fprintf(stderr,
                                                               "%s %s (%d@%s:%s): connection to client (%s) broken on channel \"%s\" (%d client transactions)\n",
                                                               date,appl_name,appl_pid,appl_host,appl_owner,psrp_client_name[c_index],channel_name,psrp_transactions[c_index]);
                                            }
                                            else
                                            {  if(psrp_transactions[c_index] == 0)
                                                  (void)fprintf(stderr,
                                                                "%s %s (%d@%s:%s): connection to client (%s [via %s]) on channel \"%s\" broken (0 client transactons)\n",
                                                                date,appl_name,appl_pid,appl_host,appl_owner,psrp_client_name[c_index],psrp_remote_hostpath[c_index],channel_name);
                                               else if(psrp_transactions[c_index] == 1)
                                                   (void)fprintf(stderr,
                                                                 "%s %s (%d@%s:%s): connection to client (%s [via %s]) on channel \"%s\" broken (1 client transaction)\n",
                                                                 date,appl_name,appl_pid,appl_host,appl_owner,psrp_client_name[c_index],psrp_remote_hostpath[c_index],channel_name);
                                               else
                                                  (void)fprintf(stderr,
                                                                "%s %s (%d@%s:%s): connection to client (%s [via %s]) on channel \"%s\" broken (%d client transactions)\n",
                                                                date,appl_name,appl_pid,appl_host,appl_owner,psrp_client_name[c_index],psrp_remote_hostpath[c_index],channel_name,psrp_transactions[c_index]);
                                            }

                                            if(c_index == c_client)
                                            {  

                                               /*------------------------------------------------------*/
                                               /* If we have a channel exit function installed it must */
                                               /* be executed now                                      */
                                               /*------------------------------------------------------*/

                                               if((void *)psrp_client_exitf[c_client] != (void *)NULL)
                                               {  (void)(*psrp_client_exitf[c_client])(c_client);
                                                  psrp_client_exitf[c_client] = (void *)NULL; 
                                                  (void)strlcpy(psrp_client_efname[c_client],"",SSIZE);
                                               }


                                               /*--------------------------------*/
                                               /* Dispose of input FIFO contents */
                                               /*--------------------------------*/

                                               if(psrp_in != (FILE *)NULL)
                                               {  (void)fflush(psrp_in);
                                                  empty_fifo(fileno(psrp_in));
                                                  psrp_in  = pups_fclose(psrp_in); 
                                               }


                                               /*---------------------------------*/
                                               /* Dispose of output FIFO contents */
                                               /*---------------------------------*/

                                               if(psrp_out != (FILE *)NULL)
                                               {  (void)fflush(psrp_out);
                                                  (void)empty_fifo(fileno(psrp_out));
                                                  psrp_out = pups_fclose(psrp_out); 
                                               }

                                               if(appl_verbose == TRUE)
                                               {  strdate(date);
                                                  (void)fprintf(stderr,
                                                  "%s %s (%d@%s:%s): channel lock released (accepting new PSRP connections on channel \"%s\")\n",
                                                                                       date,appl_name,appl_pid,appl_host,appl_owner,channel_name);
                                               }
                                            }
                                            break;


            case PSRP_IPC_LOST:             strdate(date);
                                            (void)fprintf(stderr,"%s %s (%d@%s:%s): PSRP input stream on channel \"%s\" lost -- re-establishing\n",
                                                                                         date,appl_name,appl_pid,appl_host,appl_owner,channel_name);
                                            break;


            case PSRP_OPC_LOST:             strdate(date);
                                            (void)fprintf(stderr,"%s %s (%d@%s:%s): PSRP output stream on channel \"%s\" lost -- re-establishing\n",
                                                                                          date,appl_name,appl_pid,appl_host,appl_owner,channel_name);
                                            break;

            default:                        break;
       }

       if(appl_verbose == TRUE)
          (void)fflush(stderr);
    }


    /*-----------------------------------------*/
    /* Must remember to clear the clients slot */
    /*-----------------------------------------*/

    if(event == PSRP_CLIENT_TERMINATED)
    {  (void)psrp_clear_client_slot(c_index);

       if(n_clients == 0)
          psrp_channel_open = FALSE;
    }

    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,"PSRPLIB PSRP close DONE\n");
    (void)fflush(stderr);
    #endif /* PSRPLIB_DEBUG */


    /*----------------------------------------*/
    /* Only vector if broken client is active */
    /*----------------------------------------*/

    if(c_client == c_index &&  in_psrp_handler == TRUE)
       (void)pups_sigvector(SIGALRM, &psrp_env);

    return(0);
}




/*-----------------------------------------------------------------*/
/* Search object table for requested action and return its address */
/*-----------------------------------------------------------------*/

_PRIVATE void *psrp_find_action_object(char *fname)

{   int  i;
    void *ret = (void *)NULL;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_find_action_object] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(fname == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return((void *)NULL);
    }

    for(i=0; i<psrp_object_list_used; ++i)
       if(psrp_object_list[i].object_handle != (void *)NULL && psrp_isearch_tag_list(fname,i) >= 0)
       {   int init,
               (*func)(int, char **);


           /*-----------------------------------------------------------------------*/
           /* Dispatch the requested action - note the actual form of the action is */
           /* determined by the actual type of object that we have.                 */
           /*-----------------------------------------------------------------------*/

           switch(psrp_object_list[i].object_type)
           {   

               /*---------------------------------*/
               /* Object is an excutable function */
               /*---------------------------------*/

               case PSRP_STATIC_FUNCTION:
               case PSRP_DYNAMIC_FUNCTION:  ret  = psrp_object_list[i].object_handle;
                                            break;

               /*--------------------------------------*/
               /* Object is a PSRP data area (databag) */
               /*--------------------------------------*/

               case PSRP_STATIC_DATABAG:
               case PSRP_DYNAMIC_DATABAG:   (void)fprintf(psrp_out, "    object %s is a databag (and cannot be executed!)\n",
                                                                                               psrp_object_list[i].object_tag);
                                            break;


               /*-----------------------------*/
               /* Object is a persistent heap */
               /*-----------------------------*/

               case PSRP_PERSISTENT_HEAP:   (void)fprintf(psrp_out,"    object %s is a persistent heap (and cannot be executed!)\n",
                                                                                                     psrp_object_list[i].object_tag);
                                            break;

               default:                     break;
           }
       }

    return(ret);
}




/*------------------------------------------*/
/* Search object table for requested action */
/*------------------------------------------*/

_PRIVATE _BOOLEAN psrp_exec_action_object(const int argc, int *status, const char *argv[])

{   int i;

    _BOOLEAN ret = FALSE;

    for(i=0; i<psrp_object_list_used; ++i)
       if(psrp_object_list[i].object_handle != (void *)NULL && psrp_isearch_tag_list(argv[0],i) >= 0)
       {   int init,
               (*func)(int, char **) = (void *)NULL;


           /*-----------------------------------------------------------------------*/
           /* Dispatch the requested action - note the actual form of the action is */
           /* determined by the actual type of object that we have.                 */
           /*-----------------------------------------------------------------------*/

           switch(psrp_object_list[i].object_type)
           {

               /*---------------------------------*/
               /* Object is an excutable function */
               /*---------------------------------*/

               case PSRP_STATIC_FUNCTION:
               case PSRP_DYNAMIC_FUNCTION:  func    = psrp_object_list[i].object_handle;
                                            *status = (*func)(argc,(char **)argv);
                                            ret     = TRUE;
                                            break;


               /*--------------------------------------*/
               /* Object is a PSRP data area (databag) */
               /*--------------------------------------*/

               case PSRP_STATIC_DATABAG:
               case PSRP_DYNAMIC_DATABAG:   (void)fprintf(psrp_out,
                                            "    object %s is a databag (and cannot be executed!)\n",
                                                                      psrp_object_list[i].object_tag);
                                            break;


               /*-----------------------------*/
               /* Object is a persistent heap */
               /*-----------------------------*/

               case PSRP_PERSISTENT_HEAP:   (void)fprintf(psrp_out,
                                            "    object %s is a persistent heap (and cannot be executed!)\n",
                                                                              psrp_object_list[i].object_tag);
                                            break;

               default:                     break;
           }
       }

    return(ret);
}




#ifdef MAIL_SUPPORT
/*--------------------------------------------------------------*/
/* Dynamically set type for part(s) of MIME message to be saved */
/*--------------------------------------------------------------*/

_PRIVATE int psrp_builtin_set_mime_type(const int argc, const char *argv[])

{   _BOOLEAN default_mime_type = FALSE;

    if(strcmp("mtype",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc == 1)
    {   if(strcmp(appl_mime_type,"all") == 0)
           (void)fprintf(psrp_out,"\nall parts of MIME messages currently saved [default]\n\n");
        else
           (void)fprintf(psrp_out,"\nparts of type \"%s\" of MIME messages currently saved\n\n",appl_mime_type);
       (void)fflush(psrp_out);
    }

    if(strcmp(argv[1],"help") == 0 || strcmp(argv[1],"usage") == 0)
    {  (void)fprintf(psrp_out,"\n\nUsage mtype [help | usage] [<MIME part type> | default]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"default") == 0)
    {  (void)strlcpy(appl_mime_type,"all",SSIZE);
       default_mime_type = TRUE;
    }
    else
    {  if(strcmp(argv[1],"all")         == 0    ||
          strcmp(argv[1],"text")        == 0    ||
          strcmp(argv[1],"multipart")   == 0    ||
          strcmp(argv[1],"application") == 0    ||
          strcmp(argv[1],"image")       == 0    ||
          strcmp(argv[1],"audio")       == 0    ||
          strcmp(argv[1],"video")       == 0     )
          (void)strlcpy(appl_mime_type,argv[1],SSIZE);
       else
       {  (void)fprintf(psrp_out,"\nincorrect mime type \"%s\" specified\n\n",argv[1]);
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }
    }

    if(default_mime_type == TRUE)
       (void)fprintf(psrp_out,"\nset mime type (save) to \"%s\" [default]\n\n",appl_mime_type);
    else
       (void)fprintf(psrp_out,"\nset mime type (save) to \"%s\"\n\n",appl_mime_type);
    (void)fflush(psrp_out);

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       if(default_mime_type == TRUE)
          (void)fprintf(stderr,"%s %s(%d@%s:%s): set mime type (save) to \"%s\"\n",
                       date,appl_name,appl_pid,appl_host,appl_owner,appl_mime_type);
       else
          (void)fprintf(stderr,"%s %s(%d@%s:%s): set mime type (save) to \"%s\"\n",
                       date,appl_name,appl_pid,appl_host,appl_owner,appl_mime_type);
    }

    return(PSRP_OK);
}




/*---------------------------------------------------------------------*/
/* Send PSRP requests (originated from the MIME mail interface) to the */
/* PSRP request parser                                                 */
/*---------------------------------------------------------------------*/

_PRIVATE int psrp_parse_requests(char *requests)

{   char next_request[SSIZE] = "";

    while(strext(' ',next_request,requests) != END_STRING)
         psrp_parse_request(next_request,MAIL_FACE);

    return(0);
}
#endif /* MAIL_SUPPORT */




/*----------------------------------------------------------------------------*/
/* Routine to duplicate a PSRP process - this give the user their own private */
/* copy of the PSRP process which they can subsequently modify                */
/*----------------------------------------------------------------------------*/

_PUBLIC int psrp_new_instance(const _BOOLEAN t_c_instance, const char *instance_name, const char *host_name)

{

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_new_instance] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(instance_name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(PSRP_DISPATCH_ERROR);
    }

    if(appl_verbose == TRUE)
    {  (void)strdate(date); 
       (void)fprintf(stderr,"%s %s(%d@%s:%s): rcCreating a new instance of PSRP server\"\n",
                                               date,appl_name,appl_pid,appl_host,appl_owner);
       (void)fflush(stderr);
    }


    /*------------------------------------------------------------------*/
    /* Create new instance of this process. The existing PSRP process   */
    /* is forked, so the new instance initially has the same context as */
    /* as its parent                                                    */
    /*------------------------------------------------------------------*/

    #ifdef SSH_SUPPORT
    if(psrp_new_segment(instance_name,host_name,ssh_remote_port,(char *)NULL) == (-1))
    #else
    if(psrp_new_segment(instance_name,host_name,(char *)NULL) == (-1))
    #endif /* SSH_SUPPORT */

    {  if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s(%d@%s:%s): failed to create new instance \"%s\"\n",
                              date,appl_name,appl_pid,appl_host,appl_owner,instance_name);
          (void)fflush(stderr);
       }
       
       return(PSRP_ERROR);
    }
    else if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s(%d@%s:%s): new instance \"%s\" created\n",
                     date,appl_name,appl_pid,appl_host,appl_owner,instance_name);

        if(t_c_instance == TRUE)
           (void)fprintf(stderr,"%s %s(%d@%s:%s): initial instance terminated\n\n",
                                      date,appl_name,appl_pid,appl_host,appl_owner);
       (void)fflush(stderr);
    }

    psrp_child_instance = TRUE;
    ++psrp_instances;

    pups_set_errno(OK);
    return(PSRP_OK);
}





/*-------------------------------------------------------------------------*/
/* Parse PSRP request (taking account of the interface which has generated */
/* the request)                                                            */
/*-------------------------------------------------------------------------*/
/*------------------------------------------------*/
/* Argument vector decode workspace pointer array */
/*------------------------------------------------*/

_PRIVATE char *r_argv[SSIZE] = { [0 ... 255] = (char *)NULL };
    
_PRIVATE int psrp_parse_request(char *request, int interface)

{   int r_argc,
        status;
    
    /*---------------------------------------------*/
    /* Transform request to a vector of arguments. */
    /*---------------------------------------------*/

    psrp_argvec(&r_argc,request);

    /*---------------------------------------------------------------------*/
    /* The only builtins are detach which removes dynamic entries from the */
    /* PSRP handler dispatch table and show which shows the functions      */
    /* currently loaded in the PSRP handler dispatch table.                */
    /*---------------------------------------------------------------------*/

    #ifdef PSRP_AUTHENTICATE
    /* Change (secure) server authentication token */
    if(psrp_builtin_set_secure(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;
    #endif /* PSRP_AUTHENTICATE */


    /*-------------------------------------------------------------------*/
    /* Set quantum for PSRP handler. The smaller the quantum, the better */
    /* the response time, but the greater the system loading.            */
    /*-------------------------------------------------------------------*/

    else if(psrp_builtin_set_vitimer_quantum(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*-----------------------*/
    /* Show host information */
    /*-----------------------*/

    else if(psrp_builtin_show_hinfo(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    #ifdef BUBBLE_MEMORY_SUPPORT
    /*----------------------------------------------*/
    /* Set/show memory bubble utilisation threshold */
    /*----------------------------------------------*/

    else if(psrp_builtin_set_mbubble_utilisation_threshold(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*------------------------*/
    /* Show malloc statistics */
    /*------------------------*/

    else if(psrp_builtin_show_malloc_stats(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;
    #endif /* BUBBLE_MEMORY_SUPPORT */


    #ifdef CRIU_SUPPORT
    /*------------------------------------*/
    /* Enable/disable (Criu) state saving */
    /*------------------------------------*/

    else if(psrp_builtin_ssave(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;
    #endif /* CRIU_SUPPORT */


    /*--------------------------------------------------*/  
    /* Set PSRP server stdio detach on background state */
    /*--------------------------------------------------*/  

    else if(psrp_builtin_set_nodetach(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*--------------------------------------------------*/
    /* Set PSRP servers system context migration status */
    /*--------------------------------------------------*/

    else if(psrp_builtin_set_rooted(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*----------------------------------------------------*/
    /* Reset PSRP servers system context migration status */
    /*----------------------------------------------------*/

    else if(psrp_builtin_set_unrooted(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*-------------------------------------*/
    /* Set PSRP servers (effective) parent */
    /*-------------------------------------*/

    else if(psrp_builtin_set_parent(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*-------------------------------------------------*/
    /* Set PSRP servers (effective) parent exit status */
    /*-------------------------------------------------*/

    else if(psrp_builtin_set_pexit(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;  


    /*--------------------------------------------*/
    /* Set PSRP servers current working directory */
    /*--------------------------------------------*/

    else if(psrp_builtin_set_cwd(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*-----------------------------------------------------------------------*/               
    /* Set number of times an operation will be retried (before aborting it) */
    /*-----------------------------------------------------------------------*/               

    else if(psrp_builtin_set_trys(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*-------------------------*/         
    /* Protect a file or files */
    /*-------------------------*/         

    else if(psrp_builtin_file_live(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*---------------------------*/
    /* Unprotect a file or files */
    /*---------------------------*/

    else if(psrp_builtin_file_dead(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*-------------------------------------*/
    /* Show resource usage for this server */
    /*-------------------------------------*/

    else if(psrp_builtin_show_rusage(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*------------------------------------*/
    /* Set resource usage for this server */
    /*------------------------------------*/

    else if(psrp_builtin_set_rlimit(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*------------------------------------------*/
    /* Toggle server transaction logging on/off */
    /*------------------------------------------*/

    else if(psrp_builtin_appl_verbose(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*------------------------------------------*/
    /* Toggle server side error handling on/off */
    /*------------------------------------------*/

    else if(psrp_builtin_error_handling(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*---------------------------------------*/
    /* Builtin to toggle PSRP status logging */
    /*---------------------------------------*/

    else if(psrp_builtin_transactions(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*-=---------------------------------------------*/
    /* Add a scheduling slot to this servers crontab */
    /*-=---------------------------------------------*/

    else if(psrp_builtin_crontab_schedule(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*----------------------------------------------------*/
    /* Remove a schedulign slot from this servers crontab */
    /*----------------------------------------------------*/

    else if(psrp_builtin_crontab_unschedule(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*--------------*/
    /* Show crontab */
    /*--------------*/

    else if(psrp_builtin_show_crontab(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;

    /*----------------------------------------------*/
    /* Delete action function bound to PSRP handler */
    /*----------------------------------------------*/

    else if(psrp_builtin_detach_object(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*------------------------------------------*/
    /* Set automatic dispatch table save status */
    /*------------------------------------------*/

    else if(psrp_builtin_autosave_dispatch_table(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*---------------------------------------------------------------*/
    /* Load a new dispatch table. If the new dispatch table contains */
    /* PSRP objects with the same name(s) as existing objects, the   */
    /* new objects will overwrite the old                            */
    /*---------------------------------------------------------------*/

    else if(psrp_builtin_load_dispatch_table(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*-----------------------------*/  
    /* Save current dispatch table */
    /*-----------------------------*/  

    else if(psrp_builtin_save_dispatch_table(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*------------------------------------------*/  
    /* Show client current PSRP action bindings */ 
    /*------------------------------------------*/  

    else if(psrp_builtin_show_psrp_state(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*----------------------*/
    /* Reset dispatch table */
    /*----------------------*/

    else if(psrp_builtin_reset_dispatch_table(r_argc,(char **)r_argv) == PSRP_OK)
       goto object_dispatched;


    /*----------------------------------------------------------------*/
    /* Tell client process the type of PSRP object bindings permitted */
    /*----------------------------------------------------------------*/

    else if(psrp_builtin_show_psrp_bind_type(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*-----------------------------------------------*/
    /* Display the builtin commands for this handler */
    /*-----------------------------------------------*/

    else if(psrp_builtin_help(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*----------------------------------------*/
    /* Show concurrently held link file locks */
    /*----------------------------------------*/

    else if(psrp_builtin_show_link_file_locks(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*--------------------*/
    /* Show (flock) locks */
    /*--------------------*/

    else if(psrp_builtin_show_flock_locks(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*-----------------------------------------------------------------*/
    /* Show non default signal handlers installed for this application */
    /*-----------------------------------------------------------------*/

    else if(psrp_builtin_show_sigstatus(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*--------------------------------------------------------*/
    /* Show signal mask/ signals pending for this application */
    /*--------------------------------------------------------*/

    else if(psrp_builtin_show_sigmaskstatus(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*----------------------------------------------------------*/
    /* Show streams/file descriptors opened by this application */
    /*----------------------------------------------------------*/

    else if(psrp_builtin_show_open_fdescriptors(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*--------------------------------------------*/
    /* Show (active) children of this application */
    /*--------------------------------------------*/

    else if(psrp_builtin_show_children(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*---------------------------*/
    /* Show mapped (fast) caches */
    /*---------------------------*/

    else if(psrp_builtin_show_caches(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    #ifdef DLL_SUPPORT
    /*---------------------------------------------------*/
    /* Show orifices which are bound to this application */
    /*---------------------------------------------------*/

    else if(psrp_builtin_show_attached_orifices(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;
    #endif /* DLL_SUPPORT */


    #ifdef PTHREAD_SUPPORT 
    /*-------------------------------------------------------*/
    /* Show all threads running in this PSRP server instance */
    /*-------------------------------------------------------*/

    else if(psrp_builtin_show_threads(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*------------------------------------------------------------*/
    /* Launch a new thread of execution (bound to named function) */
    /*------------------------------------------------------------*/

    else if(psrp_builtin_launch_thread(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*---------------------------------*/
    /* Terminate a thread of execution */
    /*---------------------------------*/

    else if(psrp_builtin_kill_thread(r_argc,r_argv) == PSRP_OK)
      goto object_dispatched;


    /*---------------------------*/
    /* Pause thread of execution */
    /*---------------------------*/

    else if(psrp_builtin_pause_thread(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*-----------------------------*/
    /* Restart thread of execution */
    /*-----------------------------*/

    else if(psrp_builtin_restart_thread(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;
    #endif /* PTHREAD_SUPPORT */


    /*---------------------------------------------*/
    /* Show slaved interation client channels open */
    /*---------------------------------------------*/

    else if(psrp_builtin_show_open_sics(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*----------------------------------------------*/
    /* Show entrance functions for this application */
    /*----------------------------------------------*/

    else if(psrp_builtin_pups_show_entrance_f(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*------------------------------------------*/
    /* Show exit functions for this application */
    /*------------------------------------------*/

    else if(psrp_builtin_pups_show_exit_f(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*-------------------------------------------*/
    /* Show abort functions for this application */
    /*-------------------------------------------*/

    else if(psrp_builtin_pups_show_abort_f(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*------------------------------------------------------------*/
    /* Show status entry in /proc filesystem for this application */
    /*------------------------------------------------------------*/

    else if(psrp_builtin_show_procstatus(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*---------------------------------------*/
    /* Show clients connected to this server */
    /*---------------------------------------*/

    else if(psrp_builtin_show_clients(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*-------------------------------------------------------------------------*/
    /* Show status of virtual interval timers associated with this application */
    /*-------------------------------------------------------------------------*/

    else if(psrp_builtin_pups_show_vitimers(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*-------------------------------------------*/
    /* Alias an attached function of the handler */
    /*-------------------------------------------*/

    else if(psrp_builtin_alias(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*---------------------------------------------*/
    /* Unalias an attached function of the handler */
    /*---------------------------------------------*/

    else if(psrp_builtin_unalias(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*---------------------------------------------------------*/   
    /* Show the aliases of an attached function of the handler */
    /*---------------------------------------------------------*/   

    else if(psrp_builtin_showaliases(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*--------------------*/
    /* Extend child table */
    /*--------------------*/

    else if(psrp_builtin_extend_chtab(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*----------------------------*/
    /* Extend virtual timer table */
    /*----------------------------*/

    else if(psrp_builtin_extend_vitab(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*-------------------*/
    /* Extend file table */
    /*-------------------*/

    else if(psrp_builtin_extend_ftab(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    #ifdef PERSISTENT_HEAP_SUPPORT
    /*--------------------------*/
    /* Extend persistent heap table */
    /*--------------------------*/
    else if(psrp_bind_status & PSRP_PERSISTENT_HEAP && psrp_builtin_extend_htab(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;
    #endif /* PERSISTENT_HEAP_SUPPORT */


    #ifdef DLL_SUPPORT
    /*----------------------------*/
    /* Extend (DLL) orifice table */
    /*----------------------------*/

    else if(psrp_bind_status & PSRP_DYNAMIC_FUNCTION &&  psrp_builtin_extend_ortab(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;
    #endif /* DLL_SUPPORT */


    /*-----------------------------------------*/
    /* Produce new instance of current process */
    /*-----------------------------------------*/

    else if(psrp_builtin_new_instance(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*----------------------------------------------*/
    /* Overlay the current process with new command */
    /*----------------------------------------------*/

    else if(psrp_builtin_overlay_server_process(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*-----------------------------------------------*/
    /* Overfork the current process with new command */
    /*-----------------------------------------------*/

    else if(psrp_builtin_overfork_server_process(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    #ifdef DLL_SUPPORT
    /*----------------------------------------------------*/
    /* Attach a dynamic function to PSRP dispatch handler */
    /*----------------------------------------------------*/
    else if(psrp_builtin_attach_dynamic_function(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;
    #endif /* DLL_SUPPORT */


    /*---------------------------------------------------*/
    /* Attach a dynamic databag to PSRP dispatch handler */
    /*---------------------------------------------------*/

    else if(psrp_builtin_attach_dbag(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    #ifdef PERSISTENT_HEAP_SUPPORT
    /*---------------------------------------------------*/
    /* Attach a persistent heap to PSRP dispatch handler */
    /*---------------------------------------------------*/

    else if(psrp_builtin_attach_persistent_heap(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*---------------------------------------------------------*/
    /* Show persistent heaps mapped into process address space */
    /*---------------------------------------------------------*/

    else if(psrp_builtin_show_persistent_heaps(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;
    #endif /* PERSISTENT_HEAP_SUPPORT */


    /*-----------------------------------------*/ 
    /* Show tracked heap objects on local heap */
    /*-----------------------------------------*/ 

    else if(psrp_builtin_show_htobjects(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*--------------------------------------*/ 
    /* Client termination of server process */
    /*--------------------------------------*/ 

    else if(psrp_builtin_terminate_process(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    #ifdef SSH_SUPPORT
    /*--------------------------*/
    /* Set ssh compression mode */
    /*--------------------------*/

    else if(psrp_builtin_ssh_compress(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;


    /*---------------------*/
    /* Set remote ssh port */
    /*---------------------*/

    else if(psrp_builtin_ssh_port(r_argc,r_argv) == PSRP_OK)
       goto object_dispatched;
    #endif /* SSH_SUPPORT */


/*-----------------------------------------------------------------------*/
/* Search list of user defined action functions bound to current process */
/*-----------------------------------------------------------------------*/

re_dispatch:


    /*---------------------------*/
    /* Process next PSRP command */
    /*---------------------------*/

    if(psrp_exec_action_object(r_argc,&status,r_argv) == TRUE)
    {

       /*--------------------------------------*/
       /* Command error                        */
       /* Non zero return code indicates error */
       /*--------------------------------------*/

       if(status < 0  && psrp_error_handling == TRUE)
       {  (void)fprintf(psrp_out,"    Command returned error (%s)\n",r_argv[0]);
          (void)fflush(psrp_out);

          (void)strlcpy(psrp_c_code,"err",SSIZE);

          if(appl_verbose == TRUE)
          {  strdate(date);

            (void)fprintf(stderr,"%s %s (%d@%s:%s): PSRP (protocol %5.2F) request \"%s\" error (dispatch failure)\n",
                                                                                                                date,
                                                                                                           appl_name,
                                                                                                            appl_pid,
                                                                                                           appl_host,
                                                                                                          appl_owner,
                                                                                               PSRP_PROTOCOL_VERSION,
                                                                                                             request);
            (void)fflush(stderr);
          }

          goto object_dispatched;
       }


       /*-----------------*/
       /* PSRP command ok */
       /*-----------------*/

       else
          goto object_dispatched;
    }


    /*-------------------*/
    /* Command not found */
    /*-------------------*/

    (void)fprintf(psrp_out,"    Illegal command [%s]\n",r_argv[0]);
    (void)fflush(psrp_out);

    (void)strlcpy(psrp_c_code,"illcerr",SSIZE);

    if(appl_verbose == TRUE)
    {  strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): PSRP (protocol %5.2F) request \"%s\" not recognised (dispatch failure)\n",
                                                                                                                    date,
                                                                                                               appl_name,
                                                                                                                appl_pid,
                                                                                                               appl_host,
                                                                                                              appl_owner,
                                                                                                   PSRP_PROTOCOL_VERSION,
                                                                                                                 request);
       (void)fflush(stderr);
    }
 

    if(appl_verbose == TRUE)
    {  (void)fprintf(psrp_out,"\nPSRP (protocol %5.2F) request \"%s\" dispatch failure (%d@%s)\n\n",
                                                    PSRP_PROTOCOL_VERSION,request,appl_pid,appl_host);
       (void)fflush(psrp_out);
    }

object_dispatched:    

    if(interface == PSRP_FACE)
    {

       /*-----------------------------------------------------------------------*/
       /* Tell client that the request has been processed and reset the handler */
       /* for future PSRP requests.                                             */
       /*-----------------------------------------------------------------------*/


       /*--------------------------------------------------------------*/
       /* If PSRP service function has not returned its own error code */
       /* simply return "ok"                                           */
       /*--------------------------------------------------------------*/

       if(strncmp(psrp_c_code,"none",4) == 0)
          (void)strlcpy(psrp_c_code,"ok",SSIZE);


       /*------------------------------------------------------*/
       /* Tell client this request is now processed and return */
       /* its exit code                                        */
       /*------------------------------------------------------*/

       (void)fprintf(psrp_out,"EOT %s\n",psrp_c_code);
       (void)fflush(psrp_out);


       /*------------------------------------------------------*/
       /* Tell client that we have finished processing SIGPSRP */
       /*------------------------------------------------------*/

       psrp_endop("psrp");
    }
}




/*---------------------------------------------------------------*/
/* PSRP hander routine - handles process status request protocol */
/* [PSRP] interrupts                                             */
/*---------------------------------------------------------------*/

_PRIVATE int psrp_handler(int signum)

{   int i,
        idum,
        r_argc,
        status;

    char request[SSIZE]             = "",
         get_object_command[SSIZE]  = "",
         psrp_channel_name[SSIZE]   = "",
         request_str[SSIZE]         = "",
         psrp_ckpt_status[SSIZE]    = "";

    sigset_t set;


    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,"PSRPLIB IN PSRP HANDLER\n");
    (void)fflush(stderr);
    #endif /* PSRPLIB_DEBUG */


    /*-----------------------------------------------------*/
    /* Sometimes, because of latency, we can get a SIGPSRP */
    /* raised in psrp_new_segment - this is spurious and   */
    /* must not be processed                               */
    /*-----------------------------------------------------*/

    if(in_psrp_new_segment == TRUE)
       return(0);

    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,"PSRPLIB SIGPSRP\n");
    (void)fflush(stderr);
    #endif /* PSRPLIB_DEBUG */


    /*-----------------------------------------------------*/
    /* Environment restore if SIGABRT recieved by process. */
    /*-----------------------------------------------------*/

    ++psrp_transactions[c_client];

    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,"PSRPLIB CCLIENT:%d, CNT:%d\n",c_client,psrp_transactions[c_client]);
    (void)fflush(stderr);
    #endif /* PSRPLIB_DEBUG */

    if(sigsetjmp(psrp_env,1) > 0)
    {
 
       #ifdef PSRPLIB_DEBUG
       (void)fprintf(stderr,""PSRPLIB SIGPSRP3 (ABORT)\n");
       (void)fflush(stderr);
       #endif /* PSRPLIB_DEBUG */

       if(psrp_current_sic != (psrp_channel_type *)NULL)
       {  psrp_destroy_slaved_interaction_client(psrp_current_sic,TRUE);
          psrp_unset_current_sic();
       }


       /*-----------------------*/
       /* Close PSRP connection */
       /*-----------------------*/

       if(psrp_in != (FILE *)NULL)
       {  empty_fifo(fileno(psrp_in));
          psrp_in  = pups_fclose(psrp_in);
       }

       if(psrp_out != (FILE *)NULL)
       {  empty_fifo(fileno(psrp_out));
          psrp_out = pups_fclose(psrp_out);
       }

       in_psrp_handler     = FALSE;


       /*--------------------------------------------------*/
       /* Make sure that virtual timer system is restarted */
       /* after PSRP request is aborted                    */
       /*(void)pupsigrelse(SIGALRM);                       */
       /*--------------------------------------------------*/

       (void)pups_malarm(1);

       #ifdef PSRPLIB_DEBUG
       (void)fprintf(stderr,"ABORT EXIT\n");
       (void)fflush(stderr);
       #endif /* PSRPLIB_DEBUG */
  
       return(0);
    }

    in_psrp_handler = TRUE;
    (void)strlcpy(psrp_c_code,"none",SSIZE);
    (void)sigemptyset(&set);
    (void)sigaddset(&set,SIGABRT);
    (void)pups_sigprocmask(SIG_UNBLOCK,&set,(sigset_t *)NULL);


    /*------------------------------------------------------------------------*/
    /* Tell PSRP client that this process supports the PSRP protocol and will */
    /* provide status information to the PSRP client.                         */
    /*------------------------------------------------------------------------*/

    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,""PSRPLIB SIGPSRP2\n");
    (void)fflush(stderr);
    #endif /* PSRPLIB_DEBUG */
 

    (void)snprintf(psrp_channel_name,SSIZE,"%s/psrp#%s#%d#%d",appl_fifo_dir,appl_name,appl_pid,getuid());
    (void)fprintf(psrp_out,"(%s)\n",psrp_channel_name);
    (void)fflush(psrp_out);


    /*-----------------------------------------------------------------------*/
    /* Read request string from client - this is used by the client to tell  */
    /* the handler which PSRP action function it wishes to be dispatched on  */
    /* its behalf.                                                           */
    /*-----------------------------------------------------------------------*/

    (void)efgets(request,SSIZE,psrp_in);
    (void)strlcpy(request_str,request,SSIZE);
    request_str[pups_strlen(request_str) - 2] = '\0';


    #ifdef PSRP_AUTHENTICATE
    /*----------------------------------------------*/
    /* Authenticate from psrp_passwd here if secure */
    /*----------------------------------------------*/

    if(appl_secure == TRUE && pups_check_appl_password(psrp_password) == FALSE)
    {  (void)fprintf(psrp_out,"\nSecure server authentication failure (PSRP connection closed)\n\n");
       (void)fflush(psrp_out);

       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          if(strcmp(psrp_remote_hostpath[c_client],"notset") == 0)
             (void)fprintf(stderr,"%s %s (%d@%s:%s): secure server authentication failure [client %s (%d@localhost)]\n",
                                                                                                                   date,
                                                                                                              appl_name,
                                                                                                               appl_pid,
                                                                                                              appl_host,
                                                                                                             appl_owner,
                                                                                             psrp_client_name[c_client],
                                                                                              psrp_client_pid[c_client]);
          else
             (void)fprintf(stderr,"%s %s (%d@%s:%s): secure server authentication failure [client %s (%d@%s)]\n",
                                                                                                            date,
                                                                                                       appl_name,
                                                                                                        appl_pid,
                                                                                                       appl_host,
                                                                                                      appl_owner,
                                                                                      psrp_client_name[c_client],
                                                                                       psrp_client_pid[c_client],
                                                                                  psrp_remote_hostpath[c_client]);
          (void)fflush(stderr);
       }

       /*------------------------------------------------------*/
       /* Tell client this request is now processed and return */
       /* its exit code                                        */
       /*------------------------------------------------------*/

       (void)fprintf(psrp_out,"EOT safail\n");
       (void)fflush(psrp_out);


       /*------------------------------------------------------*/
       /* Tell client that we have finished processing SIGPSRP */
       /*------------------------------------------------------*/

       psrp_endop("psrp");

       psrp_in  = pups_fclose(psrp_in);
       psrp_out = pups_fclose(psrp_out);

       in_psrp_handler = FALSE;
       return(0);
    }
    #endif /* PSRP_AUTHENTICATE */


    if(psrp_log[c_client] == TRUE && strncmp(request_str,"secure",6) != 0)
    {  if(strcmp(psrp_remote_hostpath[c_client],"notset") == 0)
          (void)fprintf(psrp_out,
                         "\nPSRP (protocol %5.2F) request \"%s\" received by %s (%d@%s:%s) from %s (%d@%s)\n\n",
                                                                                          PSRP_PROTOCOL_VERSION,
                                                                                          request_str,appl_name,
                                                                                  appl_pid,appl_host,appl_owner,
                                                                                     psrp_client_name[c_client],
                                                                                      psrp_client_pid[c_client],
                                                                                     psrp_client_host[c_client]);
        else
           (void)fprintf(psrp_out,
                          "\nPSRP (protocol %5.2F) request \"%s\" received by %s (%d@%s:%s) from %s (%d@%s) [from %s]\n\n",
                                                                                                     PSRP_PROTOCOL_VERSION,
                                                                                                     request_str,appl_name,
                                                                                             appl_pid,appl_host,appl_owner,
                                                                                                psrp_client_name[c_client],
                                                                                                 psrp_client_pid[c_client],
                                                                                                psrp_client_host[c_client],
                                                                                            psrp_remote_hostpath[c_client]);
        (void)fflush(psrp_out);
    }

    if(appl_verbose == TRUE && strncmp(request_str,"secure",6) != 0)
    {  strdate(date);

       if(strcmp(request_str,old_request_str[c_client]) != 0)
       {  if(req_r_cnt[c_client] > 0)
             (void)fprintf(stderr,"%s %s (%d@%s:%s): PSRP (protocol %5.2F) last request repeated %d times\n",
                                                                                                        date,
                                                                                                   appl_name,
                                                                                                    appl_pid,
                                                                                                   appl_host,
                                                                                                  appl_owner,
                                                                                       PSRP_PROTOCOL_VERSION,
                                                                                     req_r_cnt[c_client] + 1);

          if(strcmp(psrp_remote_hostpath[c_client],"notset") == 0)
             (void)fprintf(stderr,"%s %s (%d@%s:%s): PSRP (protocol %5.2F) request \"%s\" received from %s (%d@%s)\n",
                                                                                                                 date,
                                                                                                            appl_name,
                                                                                                             appl_pid,
                                                                                                            appl_host,
                                                                                                           appl_owner,
                                                                                                PSRP_PROTOCOL_VERSION,
                                                                                                          request_str,
                                                                                           psrp_client_name[c_client],
                                                                                            psrp_client_pid[c_client],
                                                                                           psrp_client_host[c_client]);
          else
             (void)fprintf(stderr,"%s %s (%d@%s:%s): PSRP (protocol %5.2F) request \"%s\" received from %s (%d@%s) [from %s]\n",
                                                                                                                           date,
                                                                                                                      appl_name,
                                                                                                                       appl_pid,
                                                                                                                      appl_host,
                                                                                                                     appl_owner,
                                                                                                          PSRP_PROTOCOL_VERSION,
                                                                                                                    request_str,
                                                                                                     psrp_client_name[c_client],
                                                                                                      psrp_client_pid[c_client],
                                                                                                     psrp_client_host[c_client],
                                                                                                 psrp_remote_hostpath[c_client]);
          req_r_cnt[c_client] = 0;
       }
       else
          ++req_r_cnt[c_client];

       (void)fflush(stderr);
       (void)strlcpy(old_request_str[c_client],request_str,SSIZE);
    }


    /*----------------------------------------------------------------------------*/
    /* Parse PSRP request (noting request has been originated by a client talking */
    /* PSRP protocols).                                                           */
    /*----------------------------------------------------------------------------*/

    psrp_parse_request(request_str,PSRP_FACE);

    psrp_in  = pups_fclose(psrp_in);
    psrp_out = pups_fclose(psrp_out);

    in_psrp_handler = FALSE;

    return(0);
}




/*---------------------*/
/* Save dispatch table */
/*---------------------*/

_PUBLIC int psrp_save_dispatch_table(const char *autoload_file_name)
                       
{   int i,
        j,
        objects = 0;

    FILE *autoload_stream = (FILE *)NULL;                                 


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_save_dispatch_table] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(autoload_file_name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    if(autoload_file_name == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if(access(autoload_file_name,F_OK | R_OK | W_OK) == (-1))
    {  (void)pups_creat(autoload_file_name,0644);
       if((autoload_stream = fopen(autoload_file_name,"w")) == (FILE *)NULL)
       {  pups_set_errno(EIO);
          return(-1);
       }
    }
    else
    {  if((autoload_stream = fopen(autoload_file_name,"w+")) == (FILE *)NULL)
       {  pups_set_errno(EPERM);
          return(-1);
       }
    }


    /*---------------------------------------------------------------*/
    /* We need to lock the dispatch table file as multiple processes */
    /* could be accessing the same file                              */
    /*---------------------------------------------------------------*/

    (void)pups_get_link_file_lock(WAIT_FOREVER,autoload_file_name);

    if(psrp_object_list_size == 0)
    {  (void)fclose(autoload_stream);
       (void)unlink(autoload_file_name);
       (void)pups_release_link_file_lock(autoload_file_name);

       pups_set_errno(OK);
       return(PSRP_DISPATCH_ERROR);
    }


    /*------------------------------------------------*/
    /* Get the number of valid objects (e.g. dispatch */
    /* table slots which are both allocated and used) */
    /*------------------------------------------------*/

    for(i=0; i<psrp_object_list_used; ++i)
    {  if(psrp_object_list[i].object_handle != (void *)NULL && psrp_object_list[i].aliases > 0)
          ++objects;
    }

    (void)fprintf(autoload_stream,"#-------------------------------------------------------------------\n");
    (void)fprintf(autoload_stream,"#    PSRP dispatch table resource file version 1.0                  \n");
    (void)fprintf(autoload_stream,"#    (C) M.A. O'Neill, Tumbling Dice, Gosforth, 2022                \n");
    (void)fprintf(autoload_stream,"#-------------------------------------------------------------------\n");
    (void)fprintf(autoload_stream,"\n\n");

    (void)fprintf(autoload_stream,"# Number of dispatch table objects saved\n");
    (void)fprintf(autoload_stream,"%d\n\n",objects);
    (void)fflush(autoload_stream);


    /*-------------------------------------*/
    /* Save objects in PSRP dispatch table */
    /*-------------------------------------*/

    for(i=0; i<psrp_object_list_used; ++i)     
    {   if(psrp_object_list[i].aliases > 0)
        {  (void)fprintf(autoload_stream,"\n\n#-------------------------------------------------------------------\n");
           (void)fprintf(autoload_stream,"#    PSRP (root tag) object: %s\n",psrp_object_list[i].object_tag[0]);
           (void)fprintf(autoload_stream,"#-------------------------------------------------------------------\n\n");

           (void)fprintf(autoload_stream,"# object type\n");
           (void)fflush(autoload_stream);

           if(psrp_object_list[i].object_type == PSRP_DYNAMIC_FUNCTION)
              (void)fprintf(autoload_stream,"dynamic_function\n\n");

           if(psrp_object_list[i].object_type == PSRP_STATIC_FUNCTION)
              (void)fprintf(autoload_stream,"static_function\n\n");

           if(psrp_object_list[i].object_type == PSRP_DYNAMIC_DATABAG)
              (void)fprintf(autoload_stream,"dynamic_databag\n\n");

           if(psrp_object_list[i].object_type == PSRP_STATIC_DATABAG)
           {  int slot_index;

              slot_index = pups_fname2index(psrp_object_list[i].object_f_name);
              if(ftab[slot_index].homeostatic == 1)
                 (void)fprintf(autoload_stream,"static_databag LIVE\n\n");
              else
                 (void)fprintf(autoload_stream,"static_databag DEAD\n\n");
           }

           #ifdef HAVE_DLL_SUPPORT 
           if(psrp_object_list[i].object_type == PSRP_PERSISTENT_HEAP)
           {  int f_index;


              /*--------------------------------*/
              /* Get index into PUPS file table */
              /*--------------------------------*/

              f_index = pups_fname2index(psrp_object_list[i].object_f_name);

              if(ftab[f_index].homeostatic == 1)
                 (void)fprintf(autoload_stream,"dynamic function LIVEn\n");
              else
                 (void)fprintf(autoload_stream,"dynamic function\n\n");
           }
           #endif /* HAVE_DLL_SUPPORT */

           #ifdef PERSISTENT_HEAP_SUPPORT
           if(psrp_object_list[i].object_type == PSRP_PERSISTENT_HEAP)
           {  int hdes,
		  f_index;


              /*--------------------------------*/
	      /* Get index into PUPS file table */
              /*--------------------------------*/

              f_index = pups_fname2index(psrp_object_list[i].object_f_name);


              /*-------------------------------------------*/
	      /* Get mapping mode for this persistent heap */
              /*-------------------------------------------*/

              if(ftab[f_index].homeostatic == 1)
                 (void)fprintf(autoload_stream,"persistent_heap LIVE\n\n");
              else
                 (void)fprintf(autoload_stream,"persistent_heap DEAD\n\n");
           }
           #endif /* PERSISTENT_HEAP_SUPPORT */

           (void)fprintf(autoload_stream,"# File where object is located\n");

           if(psrp_object_list[i].object_f_name != (char *)NULL)
           {  (void)fprintf(autoload_stream,"%s\n\n",psrp_object_list[i].object_f_name);
              (void)fflush(autoload_stream);
           }
           else
           {  (void)fprintf(autoload_stream,"none\n\n");
              (void)fflush(autoload_stream);
           }
 
           (void)fprintf(autoload_stream,"# Object alias(es)\n");
           (void)fflush(autoload_stream);
           (void)fprintf(autoload_stream,"%d\n\n",psrp_object_list[i].aliases);

           (void)fprintf(autoload_stream,"# Object root tag");
           if(psrp_object_list[i].aliases > 1)
              (void)fprintf(autoload_stream," and alias list\n");
           else
              (void)fprintf(autoload_stream,"\n");

           (void)fprintf(autoload_stream,"%s ",psrp_object_list[i].object_tag[0]);
           (void)fflush(autoload_stream);

           if(psrp_object_list[i].aliases > 1)
           {  for(j=1; j<psrp_object_list[i].aliases_allocated; ++j)
                 if(psrp_object_list[i].object_tag[j] != (char *)NULL)
                    (void)fprintf(autoload_stream,"%s ",psrp_object_list[i].object_tag[j]);
           }

           (void)fprintf(autoload_stream,"\n\n");
           (void)fflush(autoload_stream);
        }
    }

    (void)fclose(autoload_stream);
    (void)pups_release_link_file_lock(autoload_file_name);

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): dispatch table saved (to PSRP resource file %s)\n",
                                  date,appl_name,appl_pid,appl_host,appl_owner,autoload_file_name);
       (void)fflush(stderr);
    }

    pups_set_errno(OK);
    return(PSRP_OK);
}




/*-------------------------------------------------*/
/* Remove communication channel when process exits */
/*-------------------------------------------------*/

_PUBLIC void psrp_exit(void)
 
{   int i,
        j;
 
    char autoload_file_name[SSIZE] = "";


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_exit] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(appl_psrp_save == TRUE)
    {  

       /*---------------------------------------------------------------*/
       /* Save the names of those objects which are to be automatically */
       /* loaded at the next invocation of this server                  */
       /*---------------------------------------------------------------*/

       if(getuid() != 0)
          (void)snprintf(autoload_file_name,SSIZE,"%s/.%s.psrp",appl_home,appl_name);
       else
          (void)snprintf(autoload_file_name,SSIZE,"/%s.psrp",appl_name);

       if(appl_verbose == TRUE)
       {  strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): saving PSRP dispatch table (to [default] PSRP resource file %s)\n",
                                                     date,appl_name,appl_pid,appl_host,appl_owner,autoload_file_name);
          (void)fflush(stderr);
       }
    
       psrp_save_dispatch_table(autoload_file_name);
    }


    /*------------------------------*/
    /* Delete communication channel */
    /*------------------------------*/

    (void)unlink(channel_name_in);
    (void)unlink(channel_name_out);


    /*-------------------------------------------------*/
    /* Release memory allocated to PSRP server process */
    /*-------------------------------------------------*/

    for(i=0; i<psrp_object_list_used; ++i)
       psrp_destroy_object(i);
}




/*---------------------------------------*/
/* Get PSRP dispatch table entry by name */
/*---------------------------------------*/

_PUBLIC int lookup_psrp_object_by_name(const char *name)

{   int i;

    if(name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[lookup_psrp_object_by_name] attempt by non root thread to perform PUPS/P3 PSRP operation");

    for(i=0; i<psrp_object_list_used; ++i)
    {  if(psrp_object_list[i].object_handle != (void *)NULL && psrp_isearch_tag_list(name,i) >= 0)
       {  pups_set_errno(OK);
          return(i);
       }
    }

    pups_set_errno(ESRCH);
    return(-1);
}




/*-----------------------------------------*/
/* Get PSRP dispatch table entry by handle */
/*-----------------------------------------*/

_PUBLIC int lookup_psrp_object_by_handle(const void *object_handle)
 
{   int i; 
 

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[lookup_psrp_object_by_handle] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(object_handle == (const void *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    for(i=0; i<psrp_object_list_used; ++i)
       if(psrp_object_list[i].object_handle == object_handle)
       {  pups_set_errno(OK);
          return(i);
       }

    pups_set_errno(ESRCH);
    return(-1); 
}




/*----------------------------------*/
/* Get PSRP object handle from name */
/*----------------------------------*/

_PUBLIC void *psrp_get_handle_from_name(const char *name)

{   int d_index;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_get_handle_from_name] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if((d_index = lookup_psrp_object_by_name(name)) == (-1))
       return((void *)NULL);

    pups_set_errno(OK);
    return(psrp_object_list[d_index].object_handle);
}




/*---------------------------------*/
/*Get PSRP object name from handle */
/*---------------------------------*/

_PUBLIC int psrp_get_name_from_handle(const void *handle, char *name)

{   int d_index;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_get_name_from_handle] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(handle == (const void *)NULL || name == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if((d_index = lookup_psrp_object_by_handle(handle)) == (-1))
       return(-1);

    (void)strlcpy(name,psrp_object_list[d_index].object_handle,SSIZE);
    pups_set_errno(OK);
    return(0);
}




/*--------------------------------*/
/* Get PSRP object type from name */
/*--------------------------------*/

_PUBLIC int psrp_get_type_from_name(const char *name)

{   int d_index;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_get_type_from_name] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if((d_index = lookup_psrp_object_by_name(name)) == (-1))
       return(-1);

    pups_set_errno(OK);
    return(psrp_object_list[d_index].object_type);
}




/*----------------------------------*/
/* Get PSRP object type from handle */
/*----------------------------------*/

_PUBLIC int psrp_get_type_from_handle(const void *handle)

{   int d_index;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_get_type_from_handle] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(handle == (const void *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if((d_index = lookup_psrp_object_by_handle(handle)) == (-1))
       return(-1);

    pups_set_errno(OK);
    return(psrp_object_list[d_index].object_type);
}




/*-------------------------------------------------------------------------------------*/
/* Parse PSRP dispatch request into UNIX command line format so that it can be further */
/* processed by the comamnd line decoding utilities                                    */
/*-------------------------------------------------------------------------------------*/

_PRIVATE void psrp_argvec(int *r_argc, char *request)

{   int  i,
         ret;

    char next_arg[SSIZE] = "";

    _IMMORTAL _BOOLEAN entered = FALSE;

    for(i=0; i<256; ++i)
    {  if(entered == FALSE)
          r_argv[i] = (char *)pups_malloc(SSIZE);
       (void)strlcpy(r_argv[i],"",SSIZE);
    }
    entered = TRUE;

    *r_argc = 0;


    /*--------------------------------------------*/
    /* Reset internal pointers in strext function */
    /*--------------------------------------------*/

    (void)strext(' ',(char *)NULL,(char *)NULL);
    do {    ret = strext(' ',next_arg,request);

            (void)strlcpy(r_argv[(*r_argc)],next_arg,SSIZE);
            ++(*r_argc);

            if(*r_argc > 256)
               pups_error("[psrp_argvec] too man arguments (max 256)");

       } while(ret == TRUE);
}




/*-----------------------------------------------*/
/* Standard initialisation function for builtins */
/*-----------------------------------------------*/

_PUBLIC _BOOLEAN psrp_std_init(const int argc, const char *argv[])

{   int i,
        init,
        ptr;

    char                item[SSIZE] = "";
    psrp_object_type *psrp_object;



    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_std_init] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(argv == (const char **)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*-------------------------------------*/
    /* Reset argument decode status vector */
    /*-------------------------------------*/

    t_args = argc;


    /*---------------------------------------------------------*/
    /* By the time we have got here we have implicitly decoded */
    /* the first argument                                      */
    /*---------------------------------------------------------*/

    argd[0] = TRUE;
    for(i=1; i<255; ++i)
       argd[i] = FALSE;

    return(TRUE);
}



 
/*----------------------------------------*/
/* PSRP argument decode checking function */
/*----------------------------------------*/

_PUBLIC _BOOLEAN psrp_t_arg_errs(const int argc, const _BOOLEAN argd[], const char *args[])

{   int      i;
    _BOOLEAN parse_failed = FALSE;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_t_arg_errs] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(argd == (_BOOLEAN *)NULL || args == (char **)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    for(i=0; i<t_args; ++i)
    {   if(argd[i] == FALSE)
        {  if(parse_failed == FALSE)
           {  (void)fprintf(psrp_out,"\n\n");
              parse_failed = TRUE;
           }

           (void)fprintf(psrp_out,"psrp_t_arg_errs: failed to parse: %s\n",args[i]);
           (void)fflush(psrp_out);
        }
    }

    pups_set_errno(OK);
    if(parse_failed == TRUE)
       return(FALSE);
    else
       return(TRUE);
}




/*-------------------------------------------------*/
/* Switch error logging on or off from PSRP client */
/*-------------------------------------------------*/

_PRIVATE int psrp_builtin_appl_verbose(const int argc, const char *argv[])

{   if(strcmp("appl_verbose",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc != 2)
    {  (void)fprintf(psrp_out,"\nusage: appl_verbose [on | off]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"on") == 0)
    {  (void)fprintf(psrp_out,"\nserver transaction/error logging enabled\n\n");
       (void)fflush(psrp_out);

       appl_verbose = TRUE;
       return(PSRP_OK);
    }
    else
    {  if(strcmp(argv[1],"off") == 0)
       {  (void)fprintf(psrp_out,"\nserver transaction/error logging disabled\n\n");
          (void)fflush(psrp_out);

          appl_verbose = FALSE;
       }
       else
       {  (void)fprintf(psrp_out,"\nusage: appl_verbose [on | off]\n\n");
          (void)fflush(psrp_out);
       }

       return(PSRP_OK);
    }

    return(PSRP_DISPATCH_ERROR);
}




/*--------------------------------------------------*/
/* Switch error handling on or off from PSRP client */
/*--------------------------------------------------*/

_PRIVATE int psrp_builtin_error_handling(const int argc, const char *argv[])

{   if(strcmp("error_handling",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc != 2)
    {  (void)fprintf(psrp_out,"\nusage: error_handling [on | off]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"on") == 0)
    {  (void)fprintf(psrp_out,"\nserver error handling enabled\n\n");
       (void)fflush(psrp_out);

       psrp_error_handling = TRUE;
       return(PSRP_OK);
    }
    else
    {  if(strcmp(argv[1],"off") == 0)
       {  (void)fprintf(psrp_out,"\nerror handling disabled\n\n");
          (void)fflush(psrp_out);

          psrp_error_handling = FALSE;
       }
       else
       {  (void)fprintf(psrp_out,"\nusage: error_handling [on | off]\n\n");
          (void)fflush(psrp_out);
       }

       return(PSRP_OK);
    }

    return(PSRP_DISPATCH_ERROR);
}



#ifdef SSH_SUPPORT
/*------------------------------------------*/
/* Set ssh compression mode for PSRP client */
/*------------------------------------------*/

_PRIVATE int psrp_builtin_ssh_compress(const int argc, const char *argv[])

{   if(strcmp("compress",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc != 2)
    {  (void)fprintf(psrp_out,"\nusage: compress [on | off]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"on") == 0)
    {  (void)fprintf(psrp_out,"\nserver ssh compression enabled\n\n");
       (void)fflush(psrp_out);

       ssh_compression = TRUE;
       return(PSRP_OK);
    }
    else
    {  if(strcmp(argv[1],"off") == 0)
       {  (void)fprintf(psrp_out,"\nserver ssh compression disabled\n\n");
          (void)fflush(psrp_out);

          ssh_compression = FALSE;
       }
       else
       {  (void)fprintf(psrp_out,"\nusage: compress  [on | off]\n\n");
          (void)fflush(psrp_out);
       }

       return(PSRP_OK);
    }

    return(PSRP_DISPATCH_ERROR);
}




/*------------------------------------------*/
/* Set ssh remote host port for PSRP client */
/*------------------------------------------*/

_PRIVATE int psrp_builtin_ssh_port(const int argc, const char *argv[])

{   char port_id[SSIZE] = "";

    if(strcmp("port",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc != 2)
    {  (void)fprintf(psrp_out,"\nusage: port <port number | default>\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(sscanf(argv[0],"%d",&port_id) != 1 || port_id < 0)
    {  if(strcmp(port_id,"default") == 0)


          /*---------------------------*/
          /* ssh is using default port */
          /*---------------------------*/

          (void)strlcpy(ssh_remote_port,"",SSIZE);
       else 
       {  

          /*------------------------------------------------*/
          /* ssh is using user defined port. This is useful */
          /* when the remote psrp server is running is a    */
          /* container                                      */
          /*------------------------------------------------*/

          (void)fprintf(psrp_out,"remote ssh port must be an (>= 0)\n");
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }
    }
    else 
       (void)strlcpy(ssh_remote_port,port_id,SSIZE);

    if(strcmp(port_id,"default") == 0)
       (void)fprintf(psrp_out,"using default port (22) for ssh\n");
    else
       (void)fprintf(psrp_out,"using (remote) port (%s) for ssh\n",ssh_remote_port);
    (void)fflush(psrp_out);
    
    return(PSRP_OK);
}
#endif /* SSH_SUPPORT */




/*-------------------------------------------------*/
/* Switch error logging on or off from PSRP client */
/*-------------------------------------------------*/

_PRIVATE int psrp_builtin_transactions(const int argc, const char *argv[])

{   if(strcmp("log",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc != 2)
    {  (void)fprintf(psrp_out,"\nusage: transactions [on | off]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"on") == 0)
    {  (void)fprintf(psrp_out,"\nserver transaction logging enabled\n\n");
       (void)fflush(psrp_out);

       psrp_log[c_client] = TRUE;
       return(PSRP_OK);
    }
    else
    {  if(strcmp(argv[1],"off") == 0)
       {  (void)fprintf(psrp_out,"\nserver transaction logging disabled\n\n");
          (void)fflush(psrp_out);

          psrp_log[c_client] = FALSE;
          return(PSRP_OK);
       }
       else
       {  (void)fprintf(psrp_out,"\nusage: transactions [on | off]\n\n");
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }
    }

    return(PSRP_DISPATCH_ERROR);
}





/*-------------------------*/
/* Load new dispatch table */
/*-------------------------*/

_PRIVATE int psrp_builtin_autosave_dispatch_table(const int argc, const char *argv[])

{   int  init,
         ptr,
         ret_code;

    char item[SSIZE] = "";

    if(strcmp("autosave",argv[0]) != 0)
        return(PSRP_DISPATCH_ERROR);

    if(argc > 2)
    {  (void)fprintf(psrp_out,"\nusage: autosave [on | off]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(argc > 1)
    {  if(strcmp(argv[1],"on") == 0)
          appl_psrp_save = TRUE;
       else if(strcmp(argv[1],"off") == 0)
          appl_psrp_save = FALSE;
       else
       {  (void)fprintf(psrp_out,"   \nusage: autosave [on | off]\n\n");
          (void)fflush(psrp_out);         
 
          return(PSRP_OK);
       }
    }

    if(appl_psrp_save == TRUE)
       (void)fprintf(psrp_out,"\n   Dispatch table automatically saved when application exits (to ~/.%s.psrp)\n\n",
                                                                                                         appl_name);
    else
       (void)fprintf(psrp_out,"\n   Dispatch table is not automatically saved when application exits\n\n");

    (void)fflush(psrp_out);

    return(PSRP_OK);

}




/*--------------------------*/
/* Load new dispatch table  */
/*--------------------------*/

_PRIVATE int psrp_builtin_load_dispatch_table(const int argc, const char *argv[])

{   int  init,
         ptr,
         ret_code;

    char item[SSIZE] = "";

    if(strcmp("load",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc != 2)
    {  (void)fprintf(psrp_out,"\nusage: load <PSRP resource file name>\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    (void)psrp_load_dispatch_table(TRUE,argv[1]);

    (void)fprintf(psrp_out,"\nPSRP resource file %s loaded (commands appended to existing dispatch table)\n",argv[1]);
    (void)fflush(psrp_out);

    return(PSRP_OK);
  
}



/*---------------------*/
/* Save dispatch table */
/*---------------------*/

_PRIVATE int psrp_builtin_save_dispatch_table(const int argc, const char *argv[])

{   int  init,
         ptr,
         ret_code;

    char item[SSIZE] = "";

    if(strcmp("save",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc != 2)
    {  (void)fprintf(psrp_out,"\nusage: save <PSRP resource file name>\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    (void)psrp_save_dispatch_table(argv[1]);

    (void)fprintf(psrp_out,"\nPSRP resource file %s saved (contains all current dispatch table objects)\n\n",argv[1]);
    (void)fflush(psrp_out);

    return(PSRP_OK);

}




/*----------------------*/
/* Reset dispatch table */
/*----------------------*/

_PRIVATE int psrp_builtin_reset_dispatch_table(const int argc, const char *argv[])

{   int  init,
         ptr,
         ret_code;

    char item[SSIZE] = "";

    if(strcmp("reset",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    (void)psrp_reset_dispatch_table();

    (void)fprintf(psrp_out,"\ndispatch table reset to default state (static aliases deleted and dynamic objects detached)\n\n");
    (void)fflush(psrp_out);

    return(PSRP_OK);
}





/*-------------------------*/
/* Builtin function detach */
/*-------------------------*/

_PRIVATE int psrp_builtin_detach_object(const int argc, const char *argv[])

{   int  i,
         ret_code;

    char item[SSIZE] = "";

    if(strcmp("detach",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc < 2)
    {  psrp_error("Usage: detach <object-list>");
       return(PSRP_OK);
    }

    for(i=1; i<argc; ++i)
    {  

        /*---------------------------------------------------------------*/
        /* If object is "all" then detach all dynamic objects and re-set */
        /* alias list of all static objects                              */
        /*---------------------------------------------------------------*/

        if(strcmp(argv[i],"all") == 0)
        {  psrp_reset_dispatch_table();

           (void)fprintf(psrp_out,"\nall objects detached\n");
           (void)fflush(psrp_out);

           return(PSRP_OK);
        }

        ret_code = psrp_detach_object_by_name(argv[i]);
 
        switch(ret_code)
        {   case PSRP_DISPATCH_ERROR:    (void)fprintf(psrp_out,"\ndynamic object not in PSRP handler dispatch_list (%s)\n",argv[i]);
                                         break;
 
            case PSRP_DYNAMIC_FUNCTION:  (void)fprintf(psrp_out,"\ndynamic function detached (%s)\n",argv[i]);
                                         break;

            case PSRP_DYNAMIC_DATABAG:   (void)fprintf(psrp_out,"\ndynamic databag detached (%s)\n",argv[i]);
                                         break;

            case PSRP_PERSISTENT_HEAP:   (void)fprintf(psrp_out,"\npersistent heap detached (%s)\n",argv[i]);
                                         break;

            default:                     (void)fprintf(psrp_out,"\ndynamic object detached (%s)\n",argv[i]);
                                         break;
        }
        (void)fflush(psrp_out);
    }

    (void)fprintf(psrp_out,"\n");
    (void)fflush(psrp_out);

    return(PSRP_OK);
}



/*---------------------------------------------------*/
/* Builtin function to display current PSRP bindings */
/*---------------------------------------------------*/

_PRIVATE int psrp_builtin_show_hinfo(const int argc, const char *argv[])

{   char machtype[SSIZE]  = "",
         ostype[SSIZE]    = "",
         hosttype[SSIZE]  = "";

    if(strcmp("hinfo",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);


    /*---------------------------------------------*/
    /* Set up command tail decoder for this object */
    /*---------------------------------------------*/

    if(psrp_std_init(argc,argv) == FALSE)
       return(PSRP_OK);


    /*------------------------------------------------------*/
    /* Check that there is no junk left on the command line */
    /*------------------------------------------------------*/

    if(psrp_t_arg_errs(argc,argd,argv) == FALSE)
       return(PSRP_OK);


    /*------------------------------------------*/
    /* Show client current PSRP action bindings */
    /*------------------------------------------*/

    if(getenv("MACHTYPE") != (char *)NULL && getenv("OSTYPE") != (char *)NULL && getenv("HOSTTYPE") != (char *)NULL)
    {  (void)strlcpy(machtype,getenv("MACHTYPE"),SSIZE);
       (void)strlcpy(ostype,  getenv("OSTYPE"),SSIZE);
       (void)strlcpy(hosttype,getenv("HOSTTYPE"),SSIZE);

       (void)fprintf(psrp_out,"\nPSRP server \"%s\" running on %s.%s (%s)\n",appl_name,machtype,ostype,hosttype);
       (void)fprintf(psrp_out,"using named pipes (FIFO) for PSRP channels\n\n");
       (void)fflush(psrp_out);
    }
    else
    {  (void)fprintf(psrp_out,"\nHost information is not available\n\n");
       (void)fflush(psrp_out);
    }

    return(PSRP_OK);
}




/*---------------------------------------------------*/
/* Builtin function to display current PSRP bindings */
/*---------------------------------------------------*/
 
_PRIVATE int psrp_builtin_show_psrp_state(const int argc, const char *argv[])
 
{   if(strcmp("show",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR); 


    /*---------------------------------------------*/
    /* Set up command tail decoder for this object */
    /*---------------------------------------------*/

    if(psrp_std_init(argc,argv) == FALSE)
       return(PSRP_OK);


    /*------------------------------------------------------*/ 
    /* Check that there is no junk left on the command line */
    /*------------------------------------------------------*/ 

    if(psrp_t_arg_errs(argc,argd,argv) == FALSE)
       return(PSRP_OK);


    /*------------------------------------------*/
    /* Show client current PSRP action bindings */ 
    /*------------------------------------------*/

    psrp_show_object_list();

    return(PSRP_OK);
}



/*-------------------------------------------------------------------------------*/
/* Builtin function to display current object binding types permitted on process */
/*-------------------------------------------------------------------------------*/

_PRIVATE int psrp_builtin_show_psrp_bind_type(const int argc, const char *argv[])

{   if(strcmp("bindtype",argv[0]) != 0) 
       return(PSRP_DISPATCH_ERROR);  


    /*---------------------------------------------*/
    /* Set up command tail decoder for this object */
    /*---------------------------------------------*/

    if(psrp_std_init(argc,argv) == FALSE)
       return(PSRP_OK);


    /*------------------------------------------------------*/ 
    /* Check that there is no junk left on the command line */
    /*------------------------------------------------------*/ 

    if(psrp_t_arg_errs(argc,argd,argv) == FALSE)    
       return(PSRP_OK);


    /*----------------------------------------------------------------*/
    /* Tell client process the type of PSRP object bindings permitted */
    /*----------------------------------------------------------------*/

    (void)fprintf(psrp_out,"\n    PSRP server interface (protocol %5.2F) bind status\n\n",PSRP_PROTOCOL_VERSION);
    (void)fflush(psrp_out);

    if(psrp_bind_status & PSRP_STATUS_ONLY)
       (void)fprintf(psrp_out,"    PSRP interface supports status reporting only\n");
    else
    {  if(psrp_bind_status & PSRP_STATIC_DATABAG)
          (void)fprintf(psrp_out,"    Databags:  static binding enabled (cannot import databags)\n");

       if(psrp_bind_status & PSRP_DYNAMIC_DATABAG)
          (void)fprintf(psrp_out,"    Databags:  dynamic binding enabled (can import databags)\n");

       if(psrp_bind_status & PSRP_STATIC_FUNCTION)
          (void)fprintf(psrp_out,"    Functions: static binding enabled (cannot import DLL functions)\n");

       if(psrp_bind_status & PSRP_PERSISTENT_HEAP)
          (void)fprintf(psrp_out,"    Heaps: dynamic binding enabled (can import persistent heaps) [%d heap slots allocated]\n",appl_max_pheaps);

       #ifdef DLL_SUPPORT
       if(psrp_bind_status & PSRP_DYNAMIC_FUNCTION)
          (void)fprintf(psrp_out,"    Functions: dynamic binding enabled (can import DLL functions) [%d orifices allocated]\n\n",appl_max_orifices);
       #endif /* DLL_SUPPORT */

    }

    if(psrp_bind_status & PSRP_HOMEOSTATIC_STREAMS)
       (void)fprintf(psrp_out,"    Streams: PSRP channels are hoemostatic\n\n");
    
    (void)fflush(psrp_out);
    return(PSRP_OK);
}




/*---------------------------------------------------*/
/* Builtin function to terminate PSRP server process */
/*---------------------------------------------------*/

_PRIVATE int psrp_builtin_terminate_process(const int argc, const char *argv[])
 
{   if(strcmp("terminate",argv[0]) != 0 && strcmp("kill",argv[0]) != 0) 
       return(PSRP_DISPATCH_ERROR); 


    /*---------------------------------------------*/
    /* Set up command tail decoder for this object */
    /*---------------------------------------------*/

    if(psrp_std_init(argc,argv) == FALSE)
       return(PSRP_OK);


    /*------------------------------------------------------*/ 
    /* Check that there is no junk left on the command line */
    /*------------------------------------------------------*/ 

    if(psrp_t_arg_errs(argc,argd,argv) == FALSE)
       return(PSRP_OK);


    /*--------------------------------------*/
    /* Client termination of server process */
    /*--------------------------------------*/

    (void)fprintf(psrp_out,"\n%s (%d@%s) terminated by PSRP client\n\n",appl_name,appl_pid,appl_host);
    (void)fprintf(psrp_out,"CST\n");
    (void)fflush(psrp_out);


    /*------------------------------------------------------------------------------------*/
    /* Commit suicide explicitly to invoke handler - just in case we are a session leader */
    /*------------------------------------------------------------------------------------*/

    (void)raise(SIGTERM);

    pups_exit(255);
}




/*----------------------------------------------------------------*/
/* Builtin function to add an alias the tag list of a PSRP object */
/*----------------------------------------------------------------*/
 
_PRIVATE int psrp_builtin_alias(const int argc, const char *argv[])
 
{   if(strcmp("alias",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);
 
    if(argc != 3 || strcmp(argv[1],"-help") == 0 || strcmp(argv[1],"-usage") == 0)
    {  psrp_error("usage: alias <PSRP object name> <alias to PSRP object name>");
       return(PSRP_OK);
    }
 
    if(psrp_alias(argv[1],argv[2]) == PSRP_DISPATCH_ERROR)
    {  if(psrp_alias(argv[2],argv[1]) == PSRP_DISPATCH_ERROR)
       {  psrp_error("psrp_builtin_alias: problem with alias (builtin or not unique or not found)");
          return(PSRP_OK);
       }

       (void)fprintf(psrp_out,"\"%s\" is now an alias of \"%s\"\n",argv[1],argv[2]);
       (void)fflush(psrp_out);
    }   

    (void)fprintf(psrp_out,"\"%s\" is now an alias of \"%s\"\n",argv[2],argv[1]);
    (void)fflush(psrp_out);
 
    return(PSRP_OK);
}




/*----------------------------------------------------------*/
/* Builtin function to show clients attached to this server */
/*----------------------------------------------------------*/

_PRIVATE int psrp_builtin_show_clients(const int argc, const char *argv[])

{   if(strcmp("clients",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc > 1 || strcmp(r_argv[1],"usage") == 0)
    {  (void)fprintf(psrp_out,"usage: clients [usage]\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(argc != 1)
    {  psrp_error("usage: clients");
       return(PSRP_OK);
    }

    (void)psrp_show_clients();
    return(PSRP_OK);
}


 

/*----------------------------------------------------------------*/
/* Builtin function to add an alias the tag list of a PSRP object */
/*----------------------------------------------------------------*/
 
_PRIVATE int psrp_builtin_unalias(const int argc, const char *argv[])
 
{   int  ret;
    char basename[SSIZE] = "";
 
    if(strcmp("unalias",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);
 
    if(argc != 2 || strcmp(r_argv[1],"usage") == 0)
    {  psrp_error("usage: unalias <PSRP object name alias>");
       return(PSRP_OK);
    }
 
    if((ret = psrp_unalias(argv[1],basename)) == PSRP_TAG_ERROR)
       psrp_error("psrp_builtin_alias: alias cannot be removed (or object would become untagged)");
    else
    {  if(ret == PSRP_DISPATCH_ERROR)
       {  (void)fprintf(psrp_out,"psrp_builtin_alias: \"%s\" is not a tag name of any attached PSRP object\n",argv[1]);
          (void)fflush(psrp_out);
       }
       else
       {  (void)fprintf(psrp_out,"\"%s\" is no longer an alias of \"%s\"\n",argv[1],basename);
          (void)fflush(psrp_out);



       }
    }
       
    return(PSRP_OK);
}
 
 
 
 

/*----------------------------------------------*/
/* Builtin function to show aliases of tag name */
/*----------------------------------------------*/

_PRIVATE int psrp_builtin_showaliases(const int argc, const char *argv[])

{   if(strcmp("showaliases",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc != 2)
    {  psrp_error("usage: showaliases <of PSRP object tagname>");
       return(PSRP_OK);
    }

    psrp_show_aliases(argv[1]);
    return(PSRP_OK);
}




/*-------------------------------------------------------------------*/
/* Create a new instance of the current process - the old process is */
/* terminated and a new instance of it is created - this permits the */
/* user to modify the PSRP dispatch tables associated with a process */
/* without effecting the parent                                      */
/*-------------------------------------------------------------------*/

_PRIVATE int psrp_builtin_new_instance(const int r_argc, const char *r_argv[])

{ 

    int ret;

    char n_i_name[SSIZE] = "",
         n_i_host[SSIZE] = "notset"; 

    if(strcmp("new",r_argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(r_argc < 2 || r_argc > 4 || strcmp(r_argv[1],"-usage") == 0)
    {  (void)fprintf(psrp_out,"usage: new [keep | segment (current instance)] [<new instance name>] [<exec host>]\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(r_argc == 4 && strcmp(r_argv[1],"segment") == 0 && appl_rooted == TRUE)
    {  char rooted[SSIZE]          = "",
            lyosome_command[SSIZE] = "";

       (void)fprintf(psrp_out,"\nPSRP server \"%s\" is rooted (system context cannot migrate to new host)\n\n",appl_name);
       (void)fflush(psrp_out);

       (void)snprintf(rooted,SSIZE,"%s/%d.rooted",appl_fifo_dir,appl_pid);
       (void)pups_creat(rooted,0600);

       /*-------------------------------------------------------------*/
       /* Start lyosome process which destroys trail file after delay */
       /* of 15 seconds - this should be long enough for clients to   */
       /* attach to new instance                                      */
       /*-------------------------------------------------------------*/

       (void)snprintf(lyosome_command,SSIZE,"lyosome -lifetime 15 %s&",rooted);
       (void)system(lyosome_command);   

       return(PSRP_OK);
    }


    /*-------------------------------------*/
    /* Is the current instance to be kept? */
    /*-------------------------------------*/

    if(strcmp(r_argv[1],"segment") == 0)
       terminate_current_instance = TRUE;
    else if(strcmp(r_argv[1],"fork") == 0)
       terminate_current_instance = FALSE;
    else
    {  (void)fprintf(psrp_out,"usage: new [fork | segment (current instance)] [<new instance name> | default] [<exec host>]\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*--------------------------------------*/
    /* Get default name of the new instance */
    /*--------------------------------------*/

    (void)strlcpy(n_i_name,appl_name,SSIZE);
    (void)strlcpy(n_i_host,"notset",SSIZE);


    /*-----------------------------------------*/
    /* New instance remote - if name is set to */
    /* default it keeps the same name as the   */
    /* current instance                        */
    /*-----------------------------------------*/

    if(r_argc == 4)
    {  

       /*--------------------------*/
       /* Get name of new instance */
       /* (if not defaulted)       */
       /*--------------------------*/

       if(strcmp(r_argv[2],"default") == 0)
          (void)strlcpy(n_i_name,appl_name,SSIZE);
       else
          (void)strlcpy(n_i_name,r_argv[2],SSIZE);


       /*--------------------------------------------------------*/
       /* We cannot have a duplicate instance with the same name */
       /* as our current server                                  */
       /*--------------------------------------------------------*/

       if(terminate_current_instance  == FALSE    &&
          strcmp(r_argv[2],appl_name)   == 0      &&
          strcmp(r_argv[3],"localhost") == 0       )
       {  (void)fprintf(psrp_out,"\ncannot have duplicate local instance with same name (%s)\n",appl_name);
          (void)fflush(psrp_out);
 
          return(PSRP_OK);
       }
       else
       {

          /*----------------------------------*/   
          /* Get name of remote host (if any) */
          /*----------------------------------*/   

          (void)strlcpy(n_i_host,r_argv[3],SSIZE);
       }

       if(terminate_current_instance == TRUE)
          (void)fprintf(psrp_out,"\ncreating new [segmented] instance of \"%s\" as \"%s\" (on host %s)\n",
                                                                               appl_name,n_i_name,n_i_host);
       else
          (void)fprintf(psrp_out,"\ncreating new instance of \"%s\" as \"%s\" (on host %s)\n",
                                                                   appl_name,n_i_name,n_i_host);

       (void)fprintf(psrp_out,"... waiting for %d seconds for remote instance to start\n\n",segmentation_delay);
       (void)fflush(psrp_out);
    }


    /*----------------------------------*/
    /* New instance local with new name */
    /*----------------------------------*/

    else if(r_argc == 3)
    {  

       /*--------------------------------------------------------*/
       /* We cannot have a duplicate instance with the same name */
       /* as our current server                                  */
       /*--------------------------------------------------------*/

       if(terminate_current_instance  == FALSE    &&
          strcmp(r_argv[2],appl_name) ==0          )
       {  (void)fprintf(psrp_out,"\ncannot have duplicate local instance with same name (%s)\n",appl_name);
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }
       else
          (void)strlcpy(n_i_name,r_argv[2],SSIZE);

       if(terminate_current_instance == TRUE)
          (void)fprintf(psrp_out,"\ncreating new [segmented] instance of \"%s\" as \"%s\"\n\n",appl_name,n_i_name);
       else
          (void)fprintf(psrp_out,"\ncreating new instance of \"%s\" as \"%s\"\n",appl_name,n_i_name);
       (void)fflush(psrp_out);
    }


    /*--------------------------------------*/
    /* New local instance with default name */
    /*--------------------------------------*/

    else
    {   if(terminate_current_instance == TRUE)
           (void)fprintf(psrp_out,"\ncreating new [segmented] instance of \"%s\"\n",appl_name);
        else
           (void)fprintf(psrp_out,"\ncreating new instance of \"%s\"\n",appl_name);
        (void)fflush(psrp_out);
    }


    /*--------------------*/
    /* Start new instance */
    /*--------------------*/

    if(strcmp(n_i_host,"notset") == 0)
       ret = psrp_new_instance(terminate_current_instance,n_i_name,(char *)NULL);
    else
       ret = psrp_new_instance(terminate_current_instance,n_i_name,n_i_host);


    if(ret < 0)
    {  (void)fprintf(psrp_out,"failed to created new instance\n\n");
       (void)fflush(psrp_out);
    }   

    /*------------------------------------------------------*/
    /* Tell client this request is now processed and return */
    /* its exit code                                        */
    /*------------------------------------------------------*/

    (void)fprintf(psrp_out,"EOT %s\n",psrp_c_code);
    (void)fflush(psrp_out);


    /*------------------------------------------------------*/
    /* Tell client that we have finished processing SIGPSRP */
    /*------------------------------------------------------*/

    psrp_endop("psrp");
    pups_sleep(1);

    return(PSRP_OK);
}





/*---------------------------------*/
/* Get state of the current object */
/*---------------------------------*/

_PUBLIC int psrp_ostate(const char *object_tag)

{   int i;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
      pups_error("[psrp_ostate] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(object_tag == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    for(i=0; i<psrp_object_list_used; ++i)
       if(psrp_object_list[i].object_handle != (void *)NULL && psrp_isearch_tag_list(object_tag,i) >= 0)
       {  pups_set_errno(OK);
          return(psrp_object_list[i].object_state);
       }

    pups_set_errno(ESRCH);
    return(PSRP_DISPATCH_ERROR);
}




/*-----------------------------------------------------------*/
/* Builtin help function (displays server builtin functions) */
/*-----------------------------------------------------------*/
 
_PRIVATE int psrp_builtin_help(const int argc, const char *argv[])
 
{   if(strcmp("shelp",argv[0]) != 0) 
       return(PSRP_DISPATCH_ERROR); 


    /*---------------------------------------------*/
    /* Set up command tail decoder for this object */
    /*---------------------------------------------*/

    if(psrp_std_init(argc,argv) == FALSE)
       return(PSRP_OK);


    /*------------------------------------------------------*/ 
    /* Check that there is no junk left on the command line */
    /*------------------------------------------------------*/ 

    if(psrp_t_arg_errs(argc,argd,argv) == FALSE)
       return(PSRP_OK);


    /*-------------------------------------------*/  
    /* Display on line help for builtin commands */
    /*-------------------------------------------*/  

    (void)fprintf(psrp_out,"\n\n    PSRP request handler version 2.16 (protocol %5.2F)\n",PSRP_PROTOCOL_VERSION);
    (void)fprintf(psrp_out,"    (C) M.A. O'Neill, Tumbling Dice, 1995-2022\n\n");
    (void)fprintf(psrp_out,"    Builtin commands\n");
    (void)fprintf(psrp_out,"    ================\n\n");
    (void)fflush(psrp_out);

    (void)fprintf(psrp_out,"    General P3 server control/status commands\n");
    (void)fprintf(psrp_out,"    =========================================\n\n");
    (void)fprintf(psrp_out,"    log            [on | off]        :   switch server transaction logging on or off\n");
    (void)fprintf(psrp_out,"    verbose        [on | off]        :   switch server error logging on or off\n");
    (void)fprintf(psrp_out,"    errorhandling  [on | off]        :   toggle error handling on or off\n\n");
    (void)fprintf(psrp_out,"    hinfo                            :   display host information\n");
    (void)fprintf(psrp_out,"    show                             :   display PSRP handler status\n");
    (void)fprintf(psrp_out,"    clients                          :   display clients connected to this server\n");
    (void)fprintf(psrp_out,"    bindtype                         :   display current object binding (static or dynamic)\n");
    (void)fprintf(psrp_out,"    help                             :   display on line help information\n");
    (void)fprintf(psrp_out,"    strys                            :   set number of time PSRP server retrys operation (before aborting)\n");
    (void)fprintf(psrp_out,"    nodetach       [on | off]        :   toggle stdio (when in background) on or off\n");    
    (void)fprintf(psrp_out,"    cwd            <path>            :   set PSRP servers current working directory\n");
    (void)fprintf(psrp_out,"    rusage                           :   show current resource usage status\n"); 
    (void)fprintf(psrp_out,"    rset                             :   set resource usage limits\n"); 
    (void)fprintf(psrp_out,"    pstat                            :   display /proc filesystem status entry\n");
    (void)fprintf(psrp_out,"    terminate | kill                 :   terminate (this) PSRP server process\n");
    (void)fprintf(psrp_out,"    P3 server virtual timer control/status functions\n");
    (void)fprintf(psrp_out,"    ================================================\n\n");
    (void)fprintf(psrp_out,"    quantum        [<time> | default]:   set PSRP (vitimer) quantum\n");
    (void)fprintf(psrp_out,"    vitstat                          :   display status of PUPS virtual interval timers\n\n");
    (void)fprintf(psrp_out,"    vitab          [<n> | shrink]    :   display [or set to <n> or Procrustes shrink] number of virtal timer table slots\n\n");
    (void)fprintf(psrp_out,"    P3 server child control/status functions\n");
    (void)fprintf(psrp_out,"    =========================================\n\n");
    (void)fprintf(psrp_out,"    cstat                            :   display active children`\n");
    (void)fprintf(psrp_out,"    htab          [<n> | shrink]    :   display [or set to <n> or Procrustes shrink] number of child table slots\n\n");
    (void)fprintf(psrp_out,"    P3 server command alias control/status functions\n");
    (void)fprintf(psrp_out,"    ================================================\n\n");
    (void)fprintf(psrp_out,"    showaliases    <n>               :   show aliases of PSRP object <n>                  \n");
    (void)fprintf(psrp_out,"    alias          <n> <a>           :   alias <n> to <a>\n");
    (void)fprintf(psrp_out,"    unalias        <a>               :   remove alias <a>\n\n");
    (void)fprintf(psrp_out,"    P3 server entry/exit routine control/status functions\n");
    (void)fprintf(psrp_out,"    =====================================================\n\n");
    (void)fprintf(psrp_out,"    atentrance                       :   display list of (PUPS) application entrance functions\n");
    (void)fprintf(psrp_out,"    atexit                           :   display list of (PUPS) application exit functions\n");
    (void)fprintf(psrp_out,"    atabort                          :   display list of (PUPS) application abort functions\n");
    (void)fflush(psrp_out);


    #ifdef SSH_SUPPORT
    (void)fprintf(psrp_out,"    P3 server ssh control functions\n");
    (void)fprintf(psrp_out,"    ===============================\n\n");
    (void)fprintf(psrp_out,"    compress                         :   toggle ssh compression mode\n");
    (void)fprintf(psrp_out,"    port           <port number>     :   set remote ssh port\n\n\n");
    (void)fflush(psrp_out);
    #endif /* SSH_SUPPORT */


    #ifdef MAIL_SUPPORT
    (void)fprintf(psrp_out,"    P3 server mail control/status functions\n");
    (void)fprintf(psrp_out,"    =======================================\n\n");
    (void)fprintf(psrp_out,"    mtype          <mime part type>  :   set part type (for parts of MIME messages to be saved)\n\n\n");
    (void)fflush(psrp_out);
    #endif /* MAIL_SUPPORT */


    #ifdef PSRP_AUTHENTICATE
    (void)fprintf(psrp_out,"    P3 server security control/status functions\n");
    (void)fprintf(psrp_out,"    ===========================================\n\n");
    (void)fprintf(psrp_out,"    secure         <token>           :   set (secure) server authentication token\n\n\n");
    (void)fflush(psrp_out);
    #endif /* PSRP_AUTHENTICATE */


    #ifdef CRIU_SUPPORT
    (void)fprintf(psrp_out,"    P3 server (Criu) state save control/status functions\n");
    (void)fprintf(psrp_out,"    ====================================================\n\n");
    (void)fprintf(psrp_out,"    ssave                           |  :   Report state saving status\n");
    (void)fprintf(psrp_out,"                   [-t <poll time>]    :   Save state every <poll time> seconds (default 60 seconds)\n");
    (void)fprintf(psrp_out,"                   [-enable       ] |  :   Enable state saving\n");
    (void)fprintf(psrp_out,"                   [-disable      ] |  :   Disable state saving\n");
    (void)fprintf(psrp_out,"                   [-cd <criu dir>] |  :   Set Criu directory (default /tmp)\n\n\n");
    (void)fflush(psrp_out);
    #endif /* CRIU_SUPPORT */


    #ifdef BUBBLE_MEMORY_SUPPORT
    (void)fprintf(psrp_out,"    P3 server bubble memory control/status functions\n");
    (void)fprintf(psrp_out,"    ================================================\n\n");
    (void)fprintf(psrp_out,"    mstat                              :   show memory allocation statistics\n");
    (void)fprintf(psrp_out,"    mset           <threshold>         :   set memory bubble utilisation threshold %% (-usage for more detailed help)\n\n\n");
    (void)fflush(psrp_out);
    #endif /* BUBBLE_MEMORY_SUPPPORT */

    (void)fprintf(psrp_out,"    P3 server (attached) file control/status functions\n");
    (void)fprintf(psrp_out,"    ==================================================\n\n");
    (void)fprintf(psrp_out,"    lflstat                            :   show concurrently held link file locks\n");
    (void)fprintf(psrp_out,"    flstat                             :   show currently held flockfile locks\n");
    (void)fprintf(psrp_out,"    fstat                              :   display open file descriptors\n");
    (void)fprintf(psrp_out,"    ftab           [<n> | shrink]      :   display [or set to <n> or Procrustes shrink] number of file table slots\n\n\n");
    (void)fflush(psrp_out);

    (void)fprintf(psrp_out,"    P3 server (attached) signal control/status functions\n");
    (void)fprintf(psrp_out,"    ====================================================\n\n");
    (void)fprintf(psrp_out,"    sigstat                            :   display non default signal handlers\n");
    (void)fprintf(psrp_out,"    sigstat        <sig list>          :   display detailed data for selected signal handlers\n");
    (void)fprintf(psrp_out,"    maskstat                           :   display signal mask and pending signals\n\n\n");
    (void)fflush(psrp_out);

    (void)fprintf(psrp_out,"    P3 server (attached) fast cache status\n");
    (void)fprintf(psrp_out,"    ======================================\n\n");
    (void)fprintf(psrp_out,"    cachestat                          :   display (fast) caches mapped into process address space\n\n\n");
    (void)fflush(psrp_out);

    (void)fprintf(psrp_out,"    P3 server process cron scheduler control/status functions\n");
    (void)fprintf(psrp_out,"    =========================================================\n\n");
    (void)fprintf(psrp_out,"    schedule       <f> <t> [<func>]    :   schedule function <func> between times <f> and <t> using cron homeostat\n");
    (void)fprintf(psrp_out,"                                    :   if <func> is omitted sleep between times <f> and <t>\n");
    (void)fprintf(psrp_out,"    unschedule    <index>              :   unschedule a previously scheduled crontab slot\n");
    (void)fprintf(psrp_out,"    crontstat                          :   display crontab\n\n\n");
    (void)fflush(psrp_out);
   
    (void)fprintf(psrp_out,"    P3 server process general hoeostatis control/status functions\n");
    (void)fprintf(psrp_out,"    =============================================================\n\n");
    (void)fprintf(psrp_out,"    sicstat                            :   display open slaved interaction client channels\n");
    (void)fprintf(psrp_out,"    rooted                             :   set process to rooted mode (system context cannot migrate)\n");
    (void)fprintf(psrp_out,"    unrooted                           :   set process to unrooted mode (system context can migrate)\n");
    (void)fprintf(psrp_out,"    parent         <name | PID>        :   set name/PID of effective parent process\n");
    (void)fprintf(psrp_out,"    pexit                              :   process  exits if effective parent terminated\n");
    (void)fprintf(psrp_out,"    unpexit                            :   process does not exit if effective parent terminated\n");
    (void)fprintf(psrp_out,"    live           <f1> <f2> ...       :   protects list of (open) files against damage or deletion\n");
    (void)fprintf(psrp_out,"    dead           <f1> <f2> ...       :   unprotects list of (open) files\n");
    (void)fflush(psrp_out);

    (void)fprintf(psrp_out,"    new            <f|>s> <n> <h>      :   create new instance <n> of server, n=instance, f=fork, s=segment, h=hostname\n");
    (void)fprintf(psrp_out,"    overlay        <command>           :   Overlay server process with <command> (server resumes when command exits)\n");
    (void)fprintf(psrp_out,"    overfork       <command>           :   Overfork server process with <command>\n");
    (void)fflush(psrp_out);

    (void)fprintf(psrp_out,"    autosave       [on | off]          :   switch automatic saving of dipatch table (at exit) on or off\n");
    (void)fprintf(psrp_out,"    autosave                           :   display status of automatic dispatch table saving (at exit)\n");
    (void)fprintf(psrp_out,"    save           <n>                 :   save dispatch table (to PSRP ressource file <n>)\n");
    (void)fprintf(psrp_out,"    load           <n>                 :   load PSRP resource file <n> (appending objects to current dispatch table)\n");
    (void)fprintf(psrp_out,"    reset                              :   reset dispatch table (returning it to its default state)\n");

    if(psrp_bind_status & PSRP_DYNAMIC_DATABAG)
       (void)fprintf(psrp_out,"    bag         <n> <f>   [LIVE|DEAD]  :   bind dynamic databag in <f> to PSRP handler <n>\n");
    (void)fprintf(psrp_out,"\n\n");
    (void)fflush(psrp_out);
    

    #ifdef DLL_SUPPORT 
    (void)fprintf(psrp_out,"    P3 server process DLL (dynamically linked object) control/status functions\n");
    (void)fprintf(psrp_out,"    ==========================================================================\n\n");

    if(psrp_bind_status & PSRP_DYNAMIC_FUNCTION)
    {  (void)fprintf(psrp_out,"    dll         <name> <object DLL>    :   bind dynamic function in <object DLL> to PSRP handler with <name>\n");
       (void)fprintf(psrp_out,"    ostat                              :   display orifices which are bound to application\n");
       (void)fprintf(psrp_out,"    ortab       [<n>]                  :   display [or set to <n>] number of orifice [DLL] table slots\n\n\n");
    }

    if(psrp_bind_status & PSRP_DYNAMIC_FUNCTION || psrp_bind_status & PSRP_DYNAMIC_DATABAG || psrp_bind_status & PSRP_PERSISTENT_HEAP)
    {  (void)fprintf(psrp_out,"    detach      <item-list>            :   detach PSRP objects function in <item-list>\n");
       (void)fprintf(psrp_out,"                                       :   if item list contains \"all\" reset dispatch table\n");
    }

    (void)fprintf(psrp_out,"    dllstat     <name>                 :   show orifices exported by DLL <name>\n\n\n");
    (void)fflush(psrp_out);
    #endif /* DLL_SUPPORT */


    #ifdef PTHREAD_SUPPORT
    (void)fprintf(psrp_out,"    P3 server process thread control/status functions\n");
    (void)fprintf(psrp_out,"    =================================================\n\n");
    (void)fprintf(psrp_out,"    tstat          [<nid>]             :   show running threads, or optionally info for thread <nid>\n");
    (void)fprintf(psrp_out,"    tstart         [<n> [ <a>]         :   run (static or dynamic payload function) [name <a>] on thread <n>\n");
    (void)fprintf(psrp_out,"    tpause         [<n> | <all>]       :   pause thread <n> or all threads if <all> specifed\n");
    (void)fprintf(psrp_out,"    tcont          [<n> | <all>]       :   restart thread <n> or all threads if <all> specifed\n");
    (void)fprintf(psrp_out,"    tkill          [<n> | <all>]       :   kill thread <n> or all threads if <all> specifed\n\n\n");
    #endif /* PTHREAD_SUPPORT */
 
    (void)fprintf(psrp_out,"    P3 server process heap control/status functions\n");
    (void)fprintf(psrp_out,"    ===============================================\n\n");
    (void)fflush(psrp_out);


    #ifdef PERSISTENT_HEAP_SUPPORT
    if(psrp_bind_status & PSRP_PERSISTENT_HEAP)
    {  (void)fprintf(psrp_out,"    heap     <n> <f>  [LIVE|DEAD       :   map persistent heap in <f> to PSRP handler <n>\n");
       (void)fprintf(psrp_out,"    hstat    [<h | id> [<o | id>]      :   display pesistent heap details\n");
       (void)fprintf(psrp_out,"    htab     [<n> | shrink]            :   display [or set to <n>] or Procrustes shrink number of persistent heap table slots\n");
    }
    #endif /* PERSISTENT_HEAP_SUPPORT */

    (void)fprintf(psrp_out,"    hostat      <o1> <o2> ... <on>i    :   Show statistics for tracked heap object <tracked object>\n");
    (void)fprintf(psrp_out,"    hostat                             :   Show statistics for all tracked heap objects\n");
    (void)fprintf(psrp_out,"\n");
    (void)fflush(psrp_out);

    return(PSRP_OK);
}



#ifdef DLL_SUPPORT
/*---------------------------------------------------*/
/* Show orifices which are bound to this application */
/*---------------------------------------------------*/

_PRIVATE int psrp_builtin_show_attached_orifices(const int argc, const char *argv[])

{   if(strcmp("ostat",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc != 1)
    {  (void)fprintf(psrp_out,"Usage: ostat\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    (void)pups_show_orifices(psrp_out);
    return(PSRP_OK);
}
#endif /* DLL_SUPPORT */




#ifdef PTHREAD_SUPPORT 
/*-------------------------------------------------------*/
/* Show threads of execution running in this application */
/*-------------------------------------------------------*/

_PRIVATE int psrp_builtin_show_threads(const int argc, const char *argv[])

{   if(strcmp("tstat",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc > 2 || (argc == 1 && strcmp(argv[0],"-usage") == 0))
    {  (void)fprintf(psrp_out,"Usage: tstat [<nid>]\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }
    else if(argc == 2)
    {  int nid;


       /*--------------------------------*/
       /* Check that we have a valid nid */
       /*--------------------------------*/

       if(sscanf(argv[1],"%d",&nid) != 1 || nid < 0)
       {  (void)fprintf(psrp_out,"expecting integer value (>= 0) for (thread) nid\n");
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }


       /*--------------------------------------*/
       /* Show ttab entry for specified thread */
       /*--------------------------------------*/

       if(pupsthread_show_threadinfo(psrp_out,nid) < 0)
       {  (void)fprintf(psrp_out,"specfied thread (nid %d) does not exist\n",nid);
          (void)fflush(psrp_out);
       
          return(PSRP_OK);
       }


    }
    else
       (void)pupsthread_show_ttab(psrp_out);

    return(PSRP_OK);
}




/*---------------------------------------------------------------*/
/* Start a new thread of execution (using dispatch table object) */
/*---------------------------------------------------------------*/

_PRIVATE int psrp_builtin_launch_thread(const int argc, const char *argv[])

{   int i,
        d_index;

    char tname[SSIZE] = "";

    void *tfunc = (void *)NULL,
         *targs = (void *)NULL;

    pthread_t tid;

    if(strcmp("tstart",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc == 1 || argc == 2 && (strcmp(argv[1],"-usage") == 0))
    {  (void)fprintf(psrp_out,"Usage: tstart [-usage] | [<thread name> <payload name> [<argument string>]]\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*-------------------------------------------------------------------*/
    /* Check that payload is in dispatch table and that it is executable */
    /*-------------------------------------------------------------------*/

    if((d_index = lookup_psrp_object_by_name(argv[2])) == (-1))
    {  (void)fprintf(psrp_out,"Payload function \"%s\" is not in disptach table\n",argv[2]);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*--------------------------------------------------------*/
    /* Check object type - if its not PSRP_STATIC_FUNCTION or */
    /* PSRP_DYNAMIC_FUNCTION we can't execute it!             */
    /*--------------------------------------------------------*/

    if(psrp_object_list[d_index].object_type != PSRP_STATIC_FUNCTION    &&
       psrp_object_list[d_index].object_type != PSRP_DYNAMIC_FUNCTION    )  
    {  (void)fprintf(psrp_out,"Payload function \"%s\" is not a static or dynamic executable\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*-----------------------------------------*/
    /* Build argument list for thread launcher */
    /* and get name of thread                  */
    /*-----------------------------------------*/

    tfunc = (void *)psrp_object_list[d_index].object_handle;
    (void)strlcpy(tname,argv[1],SSIZE);


    /*------------------------------------------*/
    /* We have some arguments - typically these */
    /* will be passed as a string which will be */
    /* parsed by the thread payload function    */
    /*------------------------------------------*/

    if(argc == 4)
       targs = argv[3];
                                                             /*----------------------------------------------*/
    if((tid = pupsthread_create(tname,                       /* Name of payload function to be run by thread */
                                tfunc,                       /* Payload function to be run by thread         */
                                targs)) != (pthread_t)NULL)  /* Payload arguments                            */
                                                             /*----------------------------------------------*/
    {  int tpid,
           t_index;

       t_index = pupsthread_tid2nid(tid);
       tpid    = pupsthread_nid2tpid(t_index);
       (void)fprintf(psrp_out,"Thread %d (LWP: %d, payload function: \"%-32s\") launched (at %016lx virtual)\n",
                                                                      t_index,tpid,tname,(unsigned long int)tid);
       (void)fflush(psrp_out);
    }
    else
    {  (void)fprintf(psrp_out,"Failed to start thread\n");
       (void)fflush(psrp_out);
    }
   
    return(PSRP_OK);
}




/*--------------------------------------------*/
/* Pause thread executing in this application */
/*--------------------------------------------*/

_PRIVATE int psrp_builtin_pause_thread(const int argc, const char *argv[])

{   pthread_t tid;

    if(strcmp("tpause",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc == 1 || argc == 2 && strcmp(argv[1],"-usage") == 0)
    {  (void)fprintf(psrp_out,"Usage: tpause [-usage] | [<thread name> | [all]]\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*------------------------*/
    /* Pause specified thread */
    /*------------------------*/

    if(strcmp(argv[1],"all") != 0)
    {  if((tid = pupsthread_tfuncname2tid(argv[1])) == (pthread_t)NULL)
       {  (void)fprintf(psrp_out,"Name \"%s\" is not associated with a running thread\n",argv[1]);
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }

       if(pupsthread_pause(tid) == (-1))
       {  (void)fprintf(psrp_out,"Thread (%lx, name \"%s\") failed to stop\n",(unsigned long int)tid,argv[1]);
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }

       (void)fprintf(psrp_out,"Thread (%lx, name \"%s\") paused\n",(unsigned long int)tid,argv[1]);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*-------------------*/
    /* Pause all threads */
    /*-------------------*/

    else
    {  int i,
           t_cnt = 0;

       char tmpname[SSIZE] = "";

       (void)pthread_mutex_lock(&ttab_mutex);


       /*---------------------------------------------------*/
       /* Pause all active threads (other than root thread) */
       /*---------------------------------------------------*/

       for(i=0; i<MAX_THREADS; ++i)
       {  if(ttab[i].tid != (pthread_t *)NULL)
          {  tid = ttab[i].tid;
             (void)strlcpy(tmpname,ttab[i].tfuncname,SSIZE);
             if(pupsthread_pause(tid) == (-1))
             {  (void)fprintf(psrp_out,"Thread (%lx, name \"%s\") failed to stop\n",(unsigned long int)tid,tmpname);
                (void)fflush(psrp_out);
             }
             else
             {  (void)fprintf(psrp_out,"Thread (%lx, name \"%s\") stopped\n",(unsigned long int)tid,tmpname);
                (void)fflush(psrp_out);

                ++t_cnt;
             }
          }
       }

       (void)pthread_mutex_unlock(&ttab_mutex);


       if(t_cnt == 0)
          (void)fprintf(psrp_out,"\nno threads stopped\n");
       else
          (void)fprintf(psrp_out,"\n%d threads stopped\n",t_cnt);
       (void)fflush(psrp_out);
    }

    return(PSRP_OK);
}




/*----------------------------------------------*/
/* Restart thread executing in this application */
/*----------------------------------------------*/

_PRIVATE int psrp_builtin_restart_thread(const int argc, const char *argv[])

{   pthread_t tid;

    if(strcmp("tcont",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc == 1 || argc == 2 && strcmp(argv[1],"-usage") == 0)
    {  (void)fprintf(psrp_out,"Usage: tcont [-usage] | [<thread name> | [all]]\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*--------------------------*/
    /* Restart specified thread */
    /*--------------------------*/

    if(strcmp(argv[1],"all") != 0)
    {  if((tid = pupsthread_tfuncname2tid(argv[1])) == (pthread_t)NULL)
       {  (void)fprintf(psrp_out,"Name \"%s\" is not associated with a running thread\n",argv[1]);
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }

       if(pupsthread_cont(tid) == (-1))
       {  (void)fprintf(psrp_out,"Thread (%lx, name \"%s\") failed to restart\n",(unsigned long int)tid,argv[1]);
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }

       (void)fprintf(psrp_out,"Thread (%lx, name \"%s\") restarted\n",(unsigned long int)tid,argv[1]);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*---------------------*/
    /* Restart all threads */
    /*---------------------*/

    else
    {  int i,
           t_cnt = 0;

       char tmpname[SSIZE] = "";

       (void)pthread_mutex_lock(&ttab_mutex);


       /*-----------------------------------------------------*/
       /* Restart all active threads (other than root thread) */
       /*-----------------------------------------------------*/

       for(i=0; i<MAX_THREADS; ++i)
       {  if(ttab[i].tid != (pthread_t *)NULL)
          {  tid = ttab[i].tid;
             (void)strlcpy(tmpname,ttab[i].tfuncname,SSIZE);
             if(pupsthread_cont(tid) == (-1))
             {  (void)fprintf(psrp_out,"Thread (%lx, name \"%s\") failed to restart\n",(unsigned long int)tid,tmpname);
                (void)fflush(psrp_out);
             }
             else
             {  (void)fprintf(psrp_out,"Thread (%lx, name \"%s\") restarted\n",(unsigned long int)tid,tmpname);
                (void)fflush(psrp_out);

                ++t_cnt;
             }
          }
       }

       (void)pthread_mutex_unlock(&ttab_mutex);


       if(t_cnt == 0)
          (void)fprintf(psrp_out,"\nno threads restarted\n");
       else
          (void)fprintf(psrp_out,"\n%d threads restarted\n",t_cnt);
       (void)fflush(psrp_out);
    }

    return(PSRP_OK);
}



/*-------------------------------------------*/
/* Kill thread executing in this application */
/*-------------------------------------------*/

_PRIVATE int psrp_builtin_kill_thread(const int argc, const char *argv[])

{   pthread_t tid;

    if(strcmp("tkill",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc == 1 || argc == 2 && strcmp(argv[1],"-usage") == 0)
    {  (void)fprintf(psrp_out,"Usage: tkill [-usage] | [<thread name> | [all]]\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*-----------------------*/
    /* Kill specified thread */
    /*-----------------------*/

    if(strcmp(argv[1],"all") != 0)
    {  if((tid = pupsthread_tfuncname2tid(argv[1])) == (pthread_t)NULL)
       {  (void)fprintf(psrp_out,"Name \"%s\" is not associated with a running thread\n",argv[1]);
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }

       if(pupsthread_cancel(tid) == (-1))
       {  (void)fprintf(psrp_out,"Thread (%lx, name \"%s\") failed to terminate\n",(unsigned long int)tid,argv[1]);
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }
    
       (void)fprintf(psrp_out,"Thread (%lx, name \"%s\") terminated\n",(unsigned long int)tid,argv[1]);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*------------------*/
    /* Kill all threads */
    /*------------------*/

    else
    {  int i,
           t_cnt = 0;

       char tmpname[SSIZE] = "";

       (void)pthread_mutex_lock(&ttab_mutex);


       /*--------------------------------------------------*/
       /* Kill all active threads (other than root thread) */
       /*--------------------------------------------------*/

       for(i=0; i<MAX_THREADS; ++i)
       {  if(ttab[i].tid != (pthread_t *)NULL)
          {  tid = ttab[i].tid;
             (void)strlcpy(tmpname,ttab[i].tfuncname,SSIZE);
             if(pupsthread_cancel(tid) == (-1))
             {  (void)fprintf(psrp_out,"Thread (%lx, name \"%s\") failed to terminate\n",(unsigned long int)tid,tmpname);
                (void)fflush(psrp_out);
             }
             else
             {  (void)fprintf(psrp_out,"Thread (%lx, name \"%s\") terminated\n",(unsigned long int)tid,tmpname);
                (void)fflush(psrp_out);

                ++t_cnt;
             }
          }
       }

       (void)pthread_mutex_unlock(&ttab_mutex);


       if(t_cnt == 0)
          (void)fprintf(psrp_out,"\nno threads terminated\n");
       else
          (void)fprintf(psrp_out,"\n%d threads terminated\n",t_cnt);
       (void)fflush(psrp_out);
    }

    return(PSRP_OK);
}
#endif /* PTHREAD_SUPPORT */




/*------------------------------------------------*/
/* Show /proc filesystem entries for this process */
/*------------------------------------------------*/

_PRIVATE int psrp_builtin_show_procstatus(const int argc, const char *argv[])

{   char appl_procstatus[SSIZE] = "",
         line[SSIZE]            = "";

    FILE *proc_stream = (FILE *)NULL;

    if(strcmp("pstat",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc != 1)
    {  (void)fprintf(psrp_out,"\nusage: procstatus\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    (void)snprintf(appl_procstatus,SSIZE,"/proc/%d/status",appl_pid);

    proc_stream = pups_fopen(appl_procstatus,"r",DEAD);

    (void)fprintf(psrp_out,"\n");
    (void)fflush(psrp_out);

    do {   (void)pups_fgets(line,SSIZE,proc_stream);

           if(feof(proc_stream) == 0)
           {  (void)fprintf(psrp_out,"%s\n",line);
              (void)fflush(psrp_out);
           }
       } while(feof(proc_stream) == 0);
      
    (void)pups_fclose(proc_stream);
    (void)fprintf(psrp_out,"\n");
    (void)fflush(psrp_out);

    return(PSRP_OK); 
}





/*-----------------------------------------------*/
/* Show link file locks held by this application */
/*-----------------------------------------------*/

_PRIVATE int psrp_builtin_show_link_file_locks(const int argc, const char *argv[])

{   if(strcmp("lflstat",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    (void)pups_show_link_file_locks(psrp_out);
    return(PSRP_OK);
}




/*------------------------------------------------------------------------------------------
    Show flock locks held by this application ...
------------------------------------------------------------------------------------------*/

_PRIVATE int psrp_builtin_show_flock_locks(const int argc, const char *argv[])

{   if(strcmp("flstat",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    (void)pups_show_flock_locks(psrp_out);
    return(PSRP_OK);
}




/*-----------------------------------------------------------------*/
/* Show non-default signal handlers installed for this application */
/*-----------------------------------------------------------------*/

_PRIVATE int psrp_builtin_show_sigstatus(const int argc, const char *argv[])

{   if(strcmp("sigstat",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);


    /*------------------------------------------*/
    /* Show signal status for individual signal */
    /*------------------------------------------*/

    if(argc > 1)
    {  int i;

       for(i=1; i<argc; ++i)
       {  if(pups_show_siglstatus(pups_signametosigno(argv[i]),psrp_out) == (-1))
          {  (void)fprintf(psrp_out,"Not a valid signal (%s)\n",argv[i]);
             (void)fflush(psrp_out);
          }
       }
    }


    /*---------------------------------------*/
    /* Show (abridged) status of all signals */
    /*---------------------------------------*/

    else
       pups_show_sigstatus(psrp_out);

    return(PSRP_OK);
}




/*-----------------------------------------------------------*/
/* Show signal mask and signals pending for this application */
/*-----------------------------------------------------------*/

_PRIVATE int psrp_builtin_show_sigmaskstatus(const int argc, const char *argv[])

{   if(strcmp("maskstat",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    pups_show_sigmaskstatus(psrp_out);
    return(PSRP_OK);
}




/*--------------------------------------------------*.
/* Show file descriptors opened by this application */
/*--------------------------------------------------*/

_PRIVATE int psrp_builtin_show_open_fdescriptors(const int argc, const char *argv[])

{   if(strcmp("fstat",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    pups_show_open_fdescriptors(psrp_out);
    return(PSRP_OK);
}




/*-------------------------------------------*/
/* Show (active) children of current process */
/*-------------------------------------------*/

_PRIVATE int psrp_builtin_show_children(const int argc, const char *argv[])

{   if(strcmp("cstat",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    pups_show_children(psrp_out);
    return(PSRP_OK);
}



/*-------------------------------------------------------------------------------*/
/* Show (fast) caches curently mapped into the address space of this application */
/*-------------------------------------------------------------------------------*/

_PRIVATE int psrp_builtin_show_caches(const int argc, const char *argv[])

{   if(strcmp("cachestat",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc > 1)
    {  (void)fprintf(psrp_out,"usage: cashestat <cache>\n");
       (void)fflush(psrp_out);
       return(PSRP_OK);
    }

    if(argc == 2)
    {  int c_index; 

       c_index = cache_name2index(argv[1]);
       if(cache_display_statistics(FALSE,psrp_out,c_index) == (-1))
       {  (void)fprintf(psrp_out,"\ncache \"%s\" is not mapped into addrees space of \"%s\"\n",argv[1],appl_name);
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }
    }
    else
       (void)cache_display(FALSE,psrp_out);

    return(PSRP_OK);
}




#ifdef PERSISTENT_HEAP_SUPPORT
/*---------------------------------------------------------------------------*/
/* Show persistent heaps attached to this application - if request is of the */
/* form hstat <heap> show the object map of that heap                        */
/*---------------------------------------------------------------------------*/

_PRIVATE int psrp_builtin_show_persistent_heaps(const int argc, const char *argv[])

{   if(strcmp("hstat",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);


    /*--------------------*/
    /* Parse command line */
    /*--------------------*/

    if(argc > 3 || (argc == 2 && strcmp(argv[1],"-usage") == 0))
    {  (void)fprintf(psrp_out,"usage: hstat [<hid>]\n");
       (void)fflush(psrp_out);
       return(PSRP_OK);
    }


    /*---------------------------------------------*/
    /* Display details of specific persistent heap */
    /*---------------------------------------------*/

    if(argc >= 2)
    {  int  h_index;
       char heapname[SSIZE] = "";


       /*--------------*/
       /* Sanity check */
       /*--------------*/

       if(sscanf(argv[1],"%d",&h_index) != 1 || h_index < 0)
       {  
          if((h_index = msm_name2index(argv[1])) == (-1))
          {  (void)fprintf(psrp_out,"invalid heap specification (%s)\n",argv[1]);
             (void)fflush(psrp_out);

             return(PSRP_OK);
          }
       }


       /*------------------------------------*/
       /* Display objects on persistent heap */
       /*------------------------------------*/

       if(argc == 2)
       {  if(msm_show_mapped_objects(h_index,psrp_out) == (-1))
          {  (void)fprintf(psrp_out,"invalid heap specification (%s)\n",argv[1]);
             (void)fflush(psrp_out);

             return(PSRP_OK);
          }
       } 


       /*------------------------------------*/
       /* Display details of specific object */
       /* on peristent heap                  */
       /*------------------------------------*/

       else
       {  int o_index;

          if(sscanf(argv[2],"%d",&o_index) != 1 ||  o_index < 0)
          {  if((o_index = msm_map_objectname2index(h_index,argv[2])) == (-1))
             {  (void)fprintf(psrp_out,"\nInvalid object specification (%s)\n",argv[2]);
                (void)fflush(psrp_out);

                return(PSRP_OK);
              }
          }

          if(msm_show_mapped_object(h_index,o_index,psrp_out) == (-1))
          {  (void)fprintf(psrp_out,"\nInvalid object specification (%s)\n",argv[2]);
             (void)fflush(psrp_out);

             return(PSRP_OK);
          }
       } 
    }


    /*--------------------------------------------------*/
    /* Display details of all attached persistent heaps */
    /*--------------------------------------------------*/

    else
       psrp_show_persistent_heaps(psrp_out);

    return(PSRP_OK);
}
#endif /* PERSISTENT_HEAP_SUPPORT */





/*-------------------------------------------------------------------*/
/* Show slaved interation client channels opened by this application */
/*-------------------------------------------------------------------*/

_PRIVATE int psrp_builtin_show_open_sics(const int argc, const char *argv[])

{   if(strcmp("sicstat",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    psrp_show_open_sics(psrp_out);
    return(PSRP_OK);
}


 

/*-----------------------------------------------------*/
/* Show (PUPS) entrance functions bound to application */
/*-----------------------------------------------------*/

_PRIVATE int psrp_builtin_pups_show_entrance_f(const int argc, const char *argv[])

{   if(strcmp("atentrance",argv[0]) != 0) 
       return(PSRP_DISPATCH_ERROR); 

    pups_show_entrance_f(psrp_out);
    return(PSRP_OK);
}




/*-------------------------------------------------*/
/* Show (PUPS) exit functions bound to application */
/*-------------------------------------------------*/

_PRIVATE int psrp_builtin_pups_show_exit_f(const int argc, const char *argv[])

{   if(strcmp("atexit",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    pups_show_exit_f(psrp_out);
    return(PSRP_OK);
}



/*-------------------------------------------------*/
/* Show (PUPS) exit functions bound to application */
/*-------------------------------------------------*/

_PRIVATE int psrp_builtin_pups_show_abort_f(const int argc, const char *argv[])

{   if(strcmp("atabort",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    pups_show_abort_f(psrp_out);
    psrp_show_on_abort_callback_f(psrp_out);

    return(PSRP_OK);
}




/*-----------------------------------------------------------------*/
/* Show (PUPS) virtual interval timers associated with application */
/*-----------------------------------------------------------------*/

_PRIVATE int psrp_builtin_pups_show_vitimers(const int argc, const char *argv[])

{   if(strcmp("vitstat",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    pups_show_vitimers(psrp_out);
    return(PSRP_OK);
}




/*----------------------------------*/
/* Show tracked heap memory objects */
/*----------------------------------*/

_PRIVATE int psrp_builtin_show_htobjects(const int argc, const char *argv[])

{   int objects;

    if(strcmp("hostat",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc < 2)
    {  (void)fprintf(psrp_out,"Usage: hostat <Largest N objects | list of named objects>\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(sscanf(argv[1],"%d",&objects) == 1)
       pups_tshowobjects(psrp_out,objects);
    else
    {  int i;

       if(argc >= 2)
       {  for(i=1; i<argc; ++i)
          {  int cnt          = 0;
             _BOOLEAN is_name = FALSE;
             void     *ptr    = (void *)NULL;


             /*--------------------------------*/ 
             /* Search matab by (partial) name */ 
             /*--------------------------------*/ 

             (void)pups_tpartnametoptr("RESET");
             while((ptr = pups_tpartnametoptr(argv[i])) != (void *)NULL)
             {    is_name = TRUE;

                  pups_tshowobject(psrp_out,ptr);
                  ++cnt;
             }


             /*-----------------------------------*/
             /* Not a name - search matab by type */
             /*-----------------------------------*/

             if(is_name == FALSE)
             {  (void)pups_ttypetoptr("RESET");
                while((ptr = pups_ttypetoptr(argv[i])) != (void *)NULL)
                {    pups_tshowobject(psrp_out,ptr);
                     ++cnt;
                }
             }

             if(cnt == 0)
             {  (void)fprintf(psrp_out,"\n%s is not a tracked heap object\n\n",argv[i]);
                (void)fflush(psrp_out);
             }
          }
       }
    }

    return(PSRP_OK);
}





/*--------------------------------------------*/
/* Destroy psrp object (releasing its memory) */
/*--------------------------------------------*/

_PUBLIC void psrp_destroy_object(const unsigned int slot_index)

{   

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_destroy_object] attempt by non root thread to perform PUPS/P3 PSRP operation");

    (void)psrp_ifree_tag_list(slot_index); 
}




/*-------------------------------------*/
/* Send error message string to client */
/*-------------------------------------*/

_PUBLIC void psrp_error(const char *error_string)

{   
    if(error_string == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return;
    }

    (void)fprintf(psrp_out,"[psrp ERROR] %s\n",error_string);
    (void)fflush(psrp_out);

    pups_set_errno(OK);
}




/*-----------------------------------------------------------------------------------------*/
/* Do a name to pid conversion if user has given the name of the PSRP server process to be */
/* contacted                                                                               */                                                         
/*-----------------------------------------------------------------------------------------*/

_PUBLIC int psrp_pname_to_pid(const char *process_name)

{   int  pid,
         cnt        = 0,
         target_pid = PSRP_TERMINATED;

    char cmd_line[SSIZE]    = "",
         procpidpath[SSIZE] = "";

    FILE          *stream    = (FILE *)NULL;
    DIR           *dirp      = (DIR *)NULL;
    struct dirent *next_item = (struct dirent *)NULL;

    if(process_name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if((dirp = opendir("/proc")) == (DIR *)NULL)
    {  pups_set_errno(ENOTDIR);
       return(-1);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("[psrp_pname_to_pid] attempt by non root thread to perform PUPS/P3 PSRP operation");


    /*----------------------------*/
    /* Search for (named) process */
    /*----------------------------*/

    pups_set_errno(OK);
    while((next_item = readdir(dirp)) != (struct dirent *)NULL)
    {    if(sscanf(next_item->d_name,"%d",&pid) == 1) 
         {  (void)snprintf(procpidpath,SSIZE,"/proc/%d/cmdline",pid);
            if ((stream = fopen(procpidpath,"r")) != (FILE *)NULL)
            {  
               (void)strlcpy(cmd_line,"",SSIZE);
               (void)fgets(cmd_line,SSIZE,stream);
               (void)fclose(stream);

               if(strncmp(cmd_line,process_name,strlen(process_name)) == 0)
               {  ++cnt;

                  if(cnt > 1)
                  {  (void)closedir(dirp);
                      pups_set_errno(ESRCH);

                      return(PSRP_DUPLICATE_PROCESS_NAME);
                  }

                  target_pid = pid;
               }
            }
         }
    }

    (void)closedir(dirp);

    pups_set_errno(OK);
    return(target_pid);
}

            
               
            
                                        

/*----------------------------------------------------------------------------------------*/
/* Do a pid to name conversion if user has given the pid of the PSRP server process to be */
/* contacted                                                                              */
/*----------------------------------------------------------------------------------------*/

_PUBLIC _BOOLEAN psrp_pid_to_pname(const int pid, char *process_name) 

{   FILE *stream = (FILE *)NULL;

    char cmd_line[SSIZE]    = "",
         procpidpath[SSIZE] = "";

    if(process_name == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("[psrp_pid_to_pname] attempt by non root thread to perform PUPS/P3 PSRP operation");


    /*-------------------------*/
    /* Does the process exist? */
    /*-------------------------*/

    (void)snprintf(procpidpath,SSIZE,"/proc/%d/cmdline",pid);
    if((stream = fopen(procpidpath,"r")) == (FILE *)NULL)
    {  pups_set_errno(ESRCH);
       return(FALSE);
    }


    /*-------------------------------------------*/
    /* Ok -- process exists -- lets get its name */
    /*-------------------------------------------*/

    (void)fgets(cmd_line,SSIZE,stream);
    (void)fclose(stream);
    
    if(sscanf(cmd_line,"%s",process_name) != 1)
       (void)strlcpy(process_name,"<unknown>",SSIZE);

    pups_set_errno(OK);
    return(TRUE);
} 




/*---------------------------------------------------------------------------*/
/* Get pid from psrp channel name (given process name running on given host) */
/*---------------------------------------------------------------------------*/

_PUBLIC int psrp_channelname_to_pid(const char *patchboard_path,  // Patchboard containing channel information
                                    char       *process_name,     // Name of process for which corresponding pid required
                                    char       *process_host)     // Name of host running process

{   int  i,
         entry_cnt,
         d_cnt           = 0,
         old_target_pid  = PSRP_TERMINATED,
         target_pid      = PSRP_TERMINATED,
         target_uid      = (-1);
         
    char strdum[SSIZE]               = "",
         tmpstr[SSIZE]               = "",
         channel_process_name[SSIZE] = "",
         **entries                   = (char **)NULL;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_channelname_to_pid] attempt by non root thread to perform PUPS/P3 PSRP operation");


    if(patchboard_path == (const char *)NULL    ||
       process_name    == (const char *)NULL    ||
       process_host    == (const char *)NULL     )
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    (void)strlcpy(tmpstr,process_name,SSIZE);
    (void)strlcat(tmpstr,"#",SSIZE);
    if((entries = pups_get_directory_entries(patchboard_path, 
                                             tmpstr, 
                                             &d_cnt, 
                                             &entry_cnt)) == (char **)NULL)
    {   pups_set_errno(ESRCH);
        return(-1);
    }


    /*-----------------------------------------------------------------*/
    /* Check that we don't have duplicate processes with the same name */
    /*-----------------------------------------------------------------*/

    for(i=0; i<d_cnt; ++i)
    {  (void)mchrep(' ',"#",entries[i]);

       if(strin(entries[i],"psrp") == TRUE && strin(entries[i],"in") == TRUE)
       {  int tmp_pid;

          (void)sscanf(entries[i],"%s%s%s%s%s%d%d",strdum,channel_process_name,process_host,strdum,strdum,&tmp_pid,&target_uid);

          if(target_uid == getuid())
          {  if(i > 0                                          &&
                old_target_pid > 0                             &&
                strcmp(process_name,channel_process_name) == 0 &&
                tmp_pid != old_target_pid                       )
             {

                /*----------------------------------------------------*/ 
                /* Clear memory allocated by directory search routine */
                /*----------------------------------------------------*/ 

                for(i=0; i<d_cnt; ++i)
                   (void)pups_free((void *)entries[i]);
                (void)pups_free((void *)entries);

                pups_set_errno(EEXIST);
                return(PSRP_DUPLICATE_PROCESS_NAME);
             }
             else
             {  target_pid     = tmp_pid;
                old_target_pid = target_pid;
             }
          }
       }
    }


    /*----------------------------------------------------*/
    /* Clear memory allocated by directory search routine */
    /*----------------------------------------------------*/

    for(i=0; i<d_cnt; ++i)
       (void)pups_free((void *)entries[i]);
    (void)pups_free((void *)entries);


    /*----------------------------------------------------------*/
    /* Is the channel actually associated with a live processs? */
    /*----------------------------------------------------------*/

    if(target_pid > 0 && kill(target_pid,SIGALIVE) == (-1))
    {  pups_set_errno(EEXIST);
       return(PSRP_TERMINATED);
    }
 
    pups_set_errno(OK);
    return(target_pid);
}




/*----------------------------------------------------------*/
/* Get psrp channel name from psrp channel name (given pid) */
/*----------------------------------------------------------*/

_PUBLIC _BOOLEAN psrp_pid_to_channelname(const char *patchboard_path,  // Patchboard containing channel information
                                         const int               pid,  // Pid for which corresponding channelname is required
                                         char          *process_name,  // Name of process corresponding to pid
                                         char          *process_host)  // Name of host running process corresponding to pid

{  int  i,
        uid,
        entry_cnt,
        d_cnt = 0;

   char strdum[SSIZE]  = "",
        pid_str[SSIZE] = "",
        **entries      = (char **)NULL;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_pid_to_channelname] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(patchboard_path == (char *)NULL    ||
       pid             <  0               || 
       process_name    == (char *)NULL    ||
       process_host    == (char *)NULL     )
    {  pups_set_errno(EINVAL);
       return(FALSE);
    }


    /*-----------------------------------------*/
    /* Check to see if process actually exists */
    /*-----------------------------------------*/

    if(kill(pid,SIGALIVE) == (-1))
    {  pups_set_errno(EEXIST);
       return(FALSE);
    }


    /*------------------------------------------------*/
    /* Open stream and run ps command to get pid list */
    /*------------------------------------------------*/

    (void)snprintf(pid_str,SSIZE,"%d",pid);
    if((entries = pups_get_directory_entries(patchboard_path, 
                                             pid_str, 
                                             &d_cnt, 
                                             &entry_cnt)) == (char **)NULL)
    {   pups_set_errno(ESRCH);
        return(FALSE);
    }


    /*--------------------------------------------------------------*/
    /* values <= 0 imply that their is no channel for specified pid */
    /*--------------------------------------------------------------*/

    if(d_cnt > 0)
    {  for(i=0; i<d_cnt; ++i)    
       {  if(strin(entries[i],"psrp") == TRUE)
          {  int tmp_pid;

             (void)mchrep(' ',"#",entries[i]);
             (void)sscanf(entries[i],"%s%s%s%s%s%d%d",strdum,process_name,process_host,strdum,strdum,&tmp_pid,&uid);


             /*----------------------------------------------*/
             /* Check to see if we actually own this process */
             /*----------------------------------------------*/

             if(uid != getuid())
             {  pups_set_errno(EACCES);
                return(FALSE);
             }

             if(pid == tmp_pid)
             {

                /*----------------------------------------------------*/
                /* Clear memory allocated by directory search routine */
                /*----------------------------------------------------*/

                for(i=0; i<d_cnt; ++i)
                   (void)pups_free((void *)entries[i]);
                (void)pups_free((void *)entries);

                pups_set_errno(OK);
                return(TRUE);
             }
          }
       }
    }


    /*----------------------------------------------------*/
    /* Clear memory allocated by directory search routine */
    /*----------------------------------------------------*/

    for(i=0; i<d_cnt; ++i)
       (void)pups_free((void *)entries[i]);
    (void)pups_free((void *)entries);

    (void)strlcpy(process_name,"nochan",SSIZE);
    (void)strlcpy(process_host,"nohost",SSIZE);

    pups_set_errno(ESRCH);
    return(FALSE);
}




/*---------------------------------------------------*/
/* Initialise slaved interation client channel table */
/*---------------------------------------------------*/

_PRIVATE void psrp_initsic(void)

{   int  i;

    for(i=0; i<PSRP_MAX_SIC_CHANNELS; ++i)
    {  channel[i].in_name    = (char *)NULL;
       channel[i].out_name   = (char *)NULL;
       channel[i].host_name  = (char *)NULL;
       channel[i].ssh_port   = (char *)NULL;
       channel[i].pen        = (char *)NULL;
       channel[i].index      = FREE;
       channel[i].in_stream  = (FILE *)NULL;
       channel[i].out_stream = (FILE *)NULL;
    }

    if(appl_verbose == TRUE)
    {  strdate(date);

       (void)fprintf(stderr,"%s %s (%d@%s:%s): PSRP slaved interaction clients (%d channels available)\n",
                                       date,appl_name,appl_pid,appl_host,appl_owner,PSRP_MAX_SIC_CHANNELS);
       (void)fflush(stderr);
    }
}




#ifdef PERSISTENT_HEAP_SUPPORT
/*----------------------------------------------------*/
/* Show persistent heaps attached to this PSRP server */
/*----------------------------------------------------*/

_PUBLIC void psrp_show_persistent_heaps(const FILE *stream)

{   int i,
        heaps = 0;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_show_persistemt_heaps] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(stream == (FILE *)NULL)
    {  pups_set_errno(EINVAL);
       return;
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */


    /*---------------------------------------*/
    /* Check to see that we have initialised */
    /* persistent heaps                      */
    /*---------------------------------------*/

    if(msm_init == FALSE)
    {  (void)fprintf(stream,"\n\n    Persistent heaps not initialised\n\n");
       (void)fflush(stream);

       pups_set_errno(EINVAL);
       return;
    }

    (void)fprintf(stream,"\n\n    Persistent heaps attached\n");
    (void)fprintf(stream,"    =========================\n\n");
    (void)fflush(stream);


    /*-----------------------------------------------------------*/
    /* Display abridged details of each attached persistent heap */
    /*-----------------------------------------------------------*/

    for(i=0; i<appl_max_pheaps; ++i)
    {

       #ifdef PSRPLIB_DEBUG
       (void)fprintf(stderr,"HTABLE: %d name: (%s)\n",i,htable[i].name);
       (void)fflush(stderr);
       #endif /* PSRPLIB_DEBUG */

       if(strcmp(htable[i].name,"") != 0)
       {  

          (void)fprintf(stream,"    %04d: \"%-32s\"  mapped at %016lx virtual (size %016lx bytes)\n",i,
                                                                                        htable[i].name,
                                                                     (unsigned long int)htable[i].addr,
                                                                     htable[i].edata - htable[i].sdata);
          (void)fflush(stream);

          ++heaps;
       }
    }


    /*---------------------------------------------*/
    /* Display number of attached persistent heaps */
    /*---------------------------------------------*/

    if(heaps == 0)
    {  (void)fprintf(stream,"    no persistent heaps mapped (%04d slots free)\n\n",appl_max_pheaps,appl_max_pheaps);
       (void)fflush(stream);
    }
    else if(heaps == 1)
       (void)fprintf(stream,"\n\n    %04d persistent heap mapped into process address space (%04d slots free)\n\n",1,appl_max_pheaps,appl_max_pheaps - 1);
    else if(heaps > 1)
       (void)fprintf(stream,"\n\n    %04d persistent heaps mapped into process address space (%04d slots free)\n\n",heaps,appl_max_pheaps,appl_max_pheaps - heaps);

    (void)fflush(stream);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&htab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
}
#endif /* PERSISTENT_HEAP_SUPPORT */




/*------------------------------------------------------*/
/* Show PSRP slaved client channels open on this server */
/*------------------------------------------------------*/

_PUBLIC void psrp_show_open_sics(const FILE *stream)

{   int i,
        sics = 0;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_show_open_sics] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(stream == (FILE *)NULL)
    {  pups_set_errno(EINVAL);
       return;
    }

    (void)fprintf(stream,"\n\n     Slaved interation client channels (SICs) free\n");
    (void)fprintf(stream,"     =============================================\n\n");

    for(i=0; i<PSRP_MAX_SIC_CHANNELS; ++i)
    {  if(channel[i].index != FREE)
       {  char type[SSIZE] = "";

          if(channel[i].type == SIC_FIFO)
             (void)strlcpy(type,"[duplex FIFO]",SSIZE);
          else
             (void)strlcpy(type,"[ssh tunnel]",SSIZE); 

          #ifdef SSH_SUPPORT
          (void)fprintf(stream,"    %04d: %-32s (on host %-48s via %-48s) %-16s (ssh port: %s)\n",i,
                                                                                     channel[i].pen,
                                                                                channel[i].out_name,
                                                                               channel[i].host_name,
                                                                                               type,
                                                                                channel[i].ssh_port);
          #else
          (void)fprintf(stream,"    %04d: %-32s (on host %-48s via %-48s) %-16s (ssh port: %s)\n",i,
                                                                                     channel[i].pen,
                                                                                channel[i].out_name,
                                                                               channel[i].host_name,
                                                                                               type);
          #endif /* SSH_SUPPPORT */

          (void)fflush(stream);
          ++sics;
       }
    }

    if(sics == 0)
       (void)fprintf(stream,"\n\n     no sics in use (%04d slots free)\n\n",PSRP_MAX_SIC_CHANNELS);
    else if(sics == 1)
       (void)fprintf(stream,"\n\n     %04d sic in use (%04d slots free)\n\n", 1,PSRP_MAX_SIC_CHANNELS - 1);
    else
       (void)fprintf(stream,"    %04d sics in use (%04d slots free)\n\n",sics,PSRP_MAX_SIC_CHANNELS - sics);

    (void)fflush(stream);

    pups_set_errno(OK);
}

                                                        




/*----------------------------------------------*/
/* Send a single command to PSRP server process */
/*----------------------------------------------*/

#ifdef SSH_SUPPORT
_PUBLIC psrp_channel_type *psrp_create_slaved_interaction_client(const char  *host_name,  // Host to create client on 
                                                                 const char  *ssh_port,   // Port on client
                                                                 const char  *psrp_pen)   // PEN of client
#else
_PUBLIC psrp_channel_type *psrp_create_slaved_interaction_client(const char  *psrp_pen)   // PEN of client
#endif /* SSH_SUPPORT */
{   int  i,
         ssh_portNo                   = (-1), 
         sic_index,
         scp,
         ft_index,
         get_items;

    char eff_psrp_pen[SSIZE]          = "psrp",
         sic_channel_in_name[SSIZE]   = "",
         sic_channel_out_name[SSIZE]  = "",
         psrp_command[SSIZE]          = "",
         psrp_pathname[SSIZE]         = "",
         pwd_file_name[SSIZE]         = "",
         slaved_psrp_client_ok[SSIZE] = "";

    _BOOLEAN local_client             = TRUE,
             channel_in_use           = FALSE,
             channel_allocated        = FALSE;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_create_slaved_interaction_client] attempt by non root thread to perform PUPS/P3 PSRP operation");


    /*----------------------------------*/
    /* Get name of slaved psrp instance */
    /*----------------------------------*/

    if(psrp_pen != (char *)NULL)
       (void)strlcpy(eff_psrp_pen,psrp_pen,SSIZE);


    #ifdef SSH_SUPPORT
    /*--------------------------*/
    /* Check port specification */
    /*--------------------------*/

    if(ssh_port                  != (char *)NULL  && 
       strcmp(ssh_port,"")       != 0             && 
       strcmp(ssh_port,"notset") != 0              )
    {  if(sscanf(ssh_port,"%d",&ssh_portNo) != 1 || ssh_portNo <= 0)
       {  pups_set_errno(EINVAL);
          return((psrp_channel_type *)NULL);
       }
    }
    #endif /* SSH_SUPPORT */


    /*-----------------------------------------------------------------------------------*/
    /* We have not yet created this channel so find a free slot in the channel table and */
    /* allocate it.                                                                      */
    /*-----------------------------------------------------------------------------------*/

    for(i=0; i<PSRP_MAX_SIC_CHANNELS; ++i)
    {  if(channel[i].index == FREE)
       {

          /*------------------------------*/
          /* SIC channel can be allocated */
          /*------------------------------*/

          sic_index         = i;
          channel_allocated = TRUE;
          break;
       }
    }


    /*-------------------------------------------------*/
    /* SIC channel names for input and output channels */
    /*-------------------------------------------------*/

    (void)snprintf(sic_channel_in_name,SSIZE, "%s/psrp#%s#%s#fifo#in#%d#%d#sic#%d" ,appl_fifo_dir,appl_name,appl_host,appl_pid,appl_uid,sic_index); 
    (void)snprintf(sic_channel_out_name,SSIZE,"%s/psrp#%s#%s#fifo#out#%d#%d#sic#%d",appl_fifo_dir,appl_name,appl_host,appl_pid,appl_uid,sic_index);


    /*-------------------------*/
    /* All SIC channels in use */
    /*-------------------------*/

    if(channel_allocated == FALSE)
    {  pups_set_errno(ENFILE);
       return((psrp_channel_type *)NULL);
    }


    /*----------------------------------------------------------*/
    /* Create FIFO channels for local slaved interaction client */
    /*----------------------------------------------------------*/

    if(appl_verbose == TRUE)
    {  (void)strdate(date);

       if(host_name != (char *)NULL)
          (void)fprintf(stderr,"%s %s (%d@%s:%s): creating remote SIC channel [to host %s] (via slaved client %s)\n",
                                                 date,appl_name,appl_pid,appl_host,appl_owner,host_name,eff_psrp_pen);
       else
          (void)fprintf(stderr,"%s %s (%d@%s:%s): creating local SIC channel (via slaved client %s)\n",
                                             date,appl_name,appl_pid,appl_host,appl_owner,eff_psrp_pen);
       (void)fflush(stderr);
    }


    /*---------------------------------------------------*/
    /* Fill in (common) fields of SIC channel descriptor */
    /*---------------------------------------------------*/

    channel[sic_index].index   = sic_index;

    if(channel[sic_index].in_name == (char *)NULL)
       channel[sic_index].in_name = (char *)pups_malloc(SSIZE);
    (void)strlcpy(channel[sic_index].in_name, sic_channel_in_name,SSIZE);

    if(channel[sic_index].out_name == (char *)NULL)
       channel[sic_index].out_name  = (char *)pups_malloc(SSIZE);
    (void)strlcpy(channel[sic_index].out_name,sic_channel_out_name,SSIZE);

    if(channel[sic_index].host_name == (char *)NULL)
       channel[sic_index].host_name = (char *)pups_malloc(SSIZE);

    if(host_name == (char *)NULL)
    {  (void)strlcpy(channel[sic_index].host_name,appl_host,SSIZE);
        channel[sic_index].remote = FALSE;
    }
    else
    {  (void)strlcpy(channel[sic_index].host_name,host_name,SSIZE);
        channel[sic_index].remote = TRUE;
    }



    /*----------------------*/
    /* Non default ssh port */
    /* (e.g. for container) */
    /*----------------------*/

    if(ssh_portNo >  0)
    {  if(channel[sic_index].ssh_port == (char *)NULL)
          channel[sic_index].ssh_port = (char *)pups_malloc(SSIZE); 
       (void)strlcpy(channel[sic_index].ssh_port,ssh_port,SSIZE);
    }

    if(channel[sic_index].pen  == (char *)NULL)
       channel[sic_index].pen  = (char *)pups_malloc(SSIZE);
    (void)strlcpy(channel[sic_index].pen,eff_psrp_pen,SSIZE);


    /*----------------------------------------------*/
    /* Create physical channel (as named FIFO pair) */
    /*----------------------------------------------*/

    (void)mkfifo(sic_channel_in_name,0600);
    channel[sic_index].in_stream  = pups_fopen(sic_channel_in_name, "w",LIVE);
    (void)empty_fifo(fileno(channel[sic_index].in_stream));

    (void)mkfifo(sic_channel_out_name,0600);
    channel[sic_index].out_stream = pups_fopen(sic_channel_out_name,"r",LIVE);
    (void)empty_fifo(fileno(channel[sic_index].out_stream));


    /*-----------------------------------------------------------------*/
    /* Give the slave channel FIFO's the same level of protection as   */
    /* a PSRP channel -- e.g. homeostatic protection cannot be removed */
    /*-----------------------------------------------------------------*/

    ft_index = pups_get_ftab_index(fileno(channel[sic_index].in_stream));
    ftab[ft_index].psrp = TRUE;

    ft_index = pups_get_ftab_index(fileno(channel[sic_index].out_stream));
    ftab[ft_index].psrp = TRUE;


    /*----------------------------------------*/
    /* Start remote slaved interaction client */ 
    /*----------------------------------------*/

    if(strccpy(psrp_pathname,pups_search_path("PATH","psrp")) == (char *)NULL)
    {  if(appl_verbose == TRUE)
       {  (void)strdate(date);


          /*--------------------------*/
          /* Cannot find psrp command */
          /*--------------------------*/

          if(host_name != (char *)NULL)
             (void)fprintf(stderr,"%s %s (%d@%s:%s): failed to create remote (ssh tunnel) SIC channel [to host %s] (cannot find psrp client in path)\n",
                                                                                                 date,appl_name,appl_pid,appl_host,appl_owner,host_name);
          else
             (void)fprintf(stderr,"%s %s (%d@%s:%s): failed to create local SIC channel (cannot find psrp client in path)\n",
                                                                                date,appl_name,appl_pid,appl_host,appl_owner);
          (void)fflush(stderr);
       }

       psrp_clear_channel_slot(&channel[sic_index]);

       pups_set_errno(EEXIST);
       return((psrp_channel_type *)NULL);
    }


    /*--------------------------------------------------------------------*/
    /* Build slaved client command (noting whether it is local or remote) */
    /*--------------------------------------------------------------------*/


    /*----------------------------------------*/
    /* Slaved PSRP client is running remotely */
    /*----------------------------------------*/

    if(host_name                     != (char *)NULL  && 
       strcmp(host_name,"localhost") != 0             && 
       strcmp(host_name,"notset")    != 0             &&
       strcmp(host_name,"")          != 0              )
    {  (void)snprintf(psrp_command,SSIZE,"%s -slaved -on %s -from %s %s %d -pen %s -noprompt",
                                                                         psrp_pathname,
                                                                             host_name,
                                                                             appl_host,
                                                                             appl_name,
                                                                              appl_pid,
                                                                          eff_psrp_pen);
       local_client = FALSE;
    }

    /*---------------------------------------*/
    /* Slaved PSRP client is running locally */
    /*---------------------------------------*/

    else
       (void)snprintf(psrp_command,SSIZE,"%s -slaved -pen %s -noprompt",
                                                   psrp_pathname,
                                                    eff_psrp_pen);


    /*---------------------------------------------*/
    /* Start (local) slaved instane of psrp client */
    /*---------------------------------------------*/

    if(local_client == TRUE)
    { 

       /*----------------------------------------------------*/
       /* Child side of fork                                 */
       /* Associate stdio (for slaved ssh) with PSRP channel */
       /*----------------------------------------------------*/

       if((scp = pups_fork(FALSE,FALSE)) == 0)
       {

          /*--------------------------------------*/
          /* Connect slaved client to SIC channel */
          /*--------------------------------------*/

          (void)fclose(stdin);
          stdin = fopen(sic_channel_in_name,  "r");

          (void)fclose(stdout);
          stdout = fopen(sic_channel_out_name,"w");

          (void)fclose(stderr);
          stderr = fopen(sic_channel_out_name,"w");


          /*----------------------------------*/
          /* Start local slaved psrp instance */
          /*----------------------------------*/

          (void)execlp("sh","sh","-c",psrp_command,(char *)NULL);


          /*---------------------------------------------------------*/
          /* We should not get here -- if we do an error has occured */
          /*---------------------------------------------------------*/

          _exit(255);
       }


       /*---------------------*/
       /* Parent side of fork */
       /*---------------------*/

       else if(scp == (-1))
       {  

          /*---------------------------------------*/
          /* Failed to create slaved psrp instance */
          /*---------------------------------------*/

          (void)psrp_destroy_slaved_interaction_client(&channel[sic_index],TRUE);

          if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@%s:%s): failed to create local (duplex FIFO) SIC channel (exec failed)\n",
                                                                          date,appl_name,appl_pid,appl_host,appl_owner);
             (void)fflush(stderr);
          }

          psrp_clear_channel_slot(&channel[sic_index]);

          pups_set_errno(ENOEXEC);
          return((psrp_channel_type *)NULL);
       }


       /*----------------------------------------------------*/
       /* Fill in remaining fields of SIC channel descriptor */
       /*----------------------------------------------------*/

       channel[sic_index].type     = SIC_FIFO;
       channel[sic_index].remote   = FALSE;
       channel[sic_index].scp      = scp; 
    }


    /*---------------------------------------------*/
    /* Start remote slaved instance of psrp client */
    /*---------------------------------------------*/

    #ifdef SSH_SUPPORT
    else
    {  int nb,
           ret,
           trys                        = 0;

       char absolute_pathname[SSIZE]     = "";


       /*------------------------------------------------------*/
       /* ssh needs an absolute pathname if we have a relative */
       /* pathname -- make it absolute                         */
       /*------------------------------------------------------*/

       if(psrp_pathname[0] != '/')
       {  char cwd[SSIZE] = "";

          (void)getcwd(cwd,SSIZE);


          /*-----------------------------------*/
          /* Pathname is of the form ./relpath */
          /*-----------------------------------*/

          if(psrp_pathname[0] == '.')
             (void)snprintf(absolute_pathname,SSIZE,"%s/%s",cwd,&psrp_command[2]);
          else
             (void)snprintf(absolute_pathname,SSIZE,"%s/%s",cwd,psrp_command);

          (void)strlcpy(psrp_command,absolute_pathname,SSIZE);
       }

       ssh_sic_index = index;

       #ifdef PSRPLIB_DEBUG
       (void)fprintf(stderr,"REMOTE PSRP COMMAND: \"%s\"\n",psrp_command);
       (void)fflush(stderr);
       #endif /* PSRPLIB_DEBUG */


       /*----------------------------------------------------*/
       /* Child side of fork                                 */
       /* Associate stdio (for slaved ssh) with PSRP channel */
       /*----------------------------------------------------*/

       if((scp = pups_fork(FALSE,FALSE)) == 0)
       {  char sshPortOpt[SSIZE] = "";

          if(ssh_portNo > 0)
             (void)snprintf(sshPortOpt,SSIZE,"-p %d",ssh_portNo);
          else
             (void)snprintf(sshPortOpt,SSIZE,"-p 22");

          (void)fclose(stdin);
          stdin = fopen(sic_channel_in_name,  "r");

          (void)fclose(stdout);
          stdout = fopen(sic_channel_out_name,"w");

          (void)fclose(stderr);
          stderr = fopen(sic_channel_out_name,"w");


          /*--------------------------------------*/
          /* We are not using passwords. You will */
          /* need to generate a public/private    */
          /* keyset for this to work              */
          /*--------------------------------------*/


          /*-----------------------------------*/
          /* Start remote slaved psrp instance */
          /*-----------------------------------*/

          if(ssh_compression == TRUE)
             (void)execlp("ssh","ssh",sshPortOpt,"-oPasswordAuthentication=no","-C","-l",    ssh_remote_uname,host_name,psrp_command,(char *)NULL);
          else
             (void)execlp("ssh","ssh",sshPortOpt,"-oPasswordAuthentication=no","-l",         ssh_remote_uname,host_name,psrp_command,(char *)NULL);


          /*---------------------------------------------------------*/
          /* We should not get here -- if we do an error has occured */
          /*---------------------------------------------------------*/

          _exit(255);
       }


       /*---------------------*/
       /* Parent side of fork */
       /*---------------------*/

       else if(scp == (-1))
       {  if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@%s:%s): failed to create remote (ssh tunnel) SIC channel [to host %s] (could not exec ssh)\n",
                                                                                    date,appl_name,appl_pid,appl_host,appl_owner,host_name);
             (void)fflush(stderr);
          }

          psrp_clear_channel_slot(&channel[sic_index]);

          pups_set_errno(ENOEXEC);
          return((psrp_channel_type *)NULL);
       }
 

       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): PSRP servers (slaved) ssh using (ssh tunnel) SIC channel (%s)\n",
                                                  date,appl_name,appl_pid,appl_host,appl_owner,sic_channel_in_name);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): now talking to PSRP server \"%s\"'s slaved ssh client\n\n",
                                                      date,appl_name,appl_pid,appl_host,appl_owner,appl_name);
          (void)fflush(stderr);
       }


       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): finished talking to PSRP server \"%s\"'s slaved ssh client (authentication successful)\n",
                                                                                     date,appl_name,appl_pid,appl_host,appl_owner,appl_name);
          (void)fflush(stderr);
       }


       /*----------------------------------------------------*/
       /* Fill in remaining fields of SIC channel descriptor */
       /*----------------------------------------------------*/

       channel[sic_index].type   = SIC_SSH_TUNNEL;
       channel[sic_index].remote = TRUE;
       channel[sic_index].scp    = scp;


       /*-------------------*/   
       /* Clear SIC channel */
       /*-------------------*/   

       (void)empty_fifo(fileno(channel[sic_index].in_stream));
       (void)empty_fifo(fileno(channel[sic_index].out_stream));

       scp           = (-1);
       ssh_sic_index = (-1);
    } 
    #else
    {  if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): remote SIC channels not supported (no ssh support)\n",
                                                           date,appl_name,appl_pid,appl_host,appl_owner);
          (void)fflush(stderr);
       }

       psrp_clear_channel_slot(&channel[index]);

       pups_set_errno(EINVAL);
       return((psrp_channel_type *)NULL);
    }       
    #endif /* SSH_SUPPORT */

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       if(host_name == (char *)NULL)
          (void)fprintf(stderr,"%s %s (%d@%s:%s): local (duplex FIFO) SIC channel created (via slaved client %s)\n",
                                                          date,appl_name,appl_pid,appl_host,appl_owner,eff_psrp_pen);
       #ifdef SSH_SUPPORT
       else
          (void)fprintf(stderr,"%s %s (%d@%s:%s): remote (ssh tunnel) SIC channel [to host %s] created (via slaved client %s)\n",
                                                             date,appl_name,appl_pid,appl_host,appl_owner,host_name,eff_psrp_pen);
       #endif /* SSH_SUPPORT */

       (void)fflush(stderr);
    }


    /*---------------------------------------------------*/
    /* Check that we have out PSRP server up and running */
    /*---------------------------------------------------*/

    (void)fgets(slaved_psrp_client_ok,SSIZE,channel[sic_index].out_stream);

    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,"GOT \"%s\"\n",slaved_psrp_client_ok);
    (void)fflush(stderr);
    #endif /* PSRPLIB_DEBUG */

    if(strncmp(slaved_psrp_client_ok,"PSRP",4) == 0)
    {  if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): slaved PSRP server \"%s\" has started\n",
                                 date,appl_name,appl_pid,appl_host,appl_owner,eff_psrp_pen);
          (void)fflush(stderr);
       }


       #ifdef PSRPLIB_DEBUG
       int cnt = 0;
       while(1)
       {   (void)fputs("test\n",channel[sic_index].in_stream);
           (void)fflush(channel[index].in_stream);

           (void)psrp_read_sic(&channel[sic_index],slaved_psrp_client_ok);
           (void)pups_sleep(1);

           (void)fprintf(stderr,"GOT: %s (cnt: %d)\n**********\n\n",slaved_psrp_client_ok,cnt++);
           (void)fflush(stderr);
       }
       #endif /* PSRPLIB_DEBUG */

       pups_set_errno(OK);
       return(&channel[sic_index]);
    }


    /*---------------------------------*/
    /* Problem with slaved psrp server */
    /*---------------------------------*/

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): slaved PSRP server \"%s\" has failed to start\n",
                                      date,appl_name,appl_pid,appl_host,appl_owner,eff_psrp_pen);
       (void)fflush(stderr);
    }

    psrp_clear_channel_slot(&channel[sic_index]);
       
    pups_set_errno(EINVAL);
    return((psrp_channel_type *)NULL);
}




/*---------------------------------------------------------*/
/* Send a request to slaved psrp client and wait for reply */
/*---------------------------------------------------------*/

_PUBLIC char *psrp_slaved_client_transaction(const _BOOLEAN          do_reply, // Log reply
                                             const psrp_channel_type     *sic, // SIC channel
                                             const char              *request) // Request to send

{   int ret,
        size = 0;

    _BOOLEAN looper;

    char tmp_str[512]   = "",    
         *reply         = (char *)NULL;;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_slaved_client_transaction] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(sic == (const psrp_channel_type *)NULL || request == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return((char *)NULL);
    }


    /*--------------*/
    /* Send request */
    /*--------------*/

    if((ret = psrp_write_sic(sic,request)) < 0)
    {  if(ret == (-2) && appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"\n%s %s (%d@%s:%s): attempted SIC transaction with self\n\n",
                                                date,appl_name,appl_pid,appl_host,appl_owner);
          (void)fflush(stderr);
       }

       pups_set_errno(EINVAL);
       return((char *)NULL);
    }


    /*------------*/
    /* Read reply */
    /*------------*/

    do {    (void)psrp_set_current_sic(sic); 
            if((looper = psrp_read_sic(sic,tmp_str)) == PSRP_MORE)
            {  
               if(do_reply == TRUE)
               {  size += strlen(tmp_str);
                  reply = (char *)realloc((void *)reply,size);
                  (void)strlcat(reply,tmp_str,SSIZE);
               }
            }

            #ifdef PSRPLIB_DEBUG
            (void)fprintf(stderr,"PSRPLIB TRANSACTION GOT: \"%s\"\n",tmp_str);
            (void)fflush(stderr);
            #endif /* PSRPLIB_DEBUG */

            (void)psrp_unset_current_sic();
       } while(looper == PSRP_MORE);

    pups_set_errno(OK);
    return(reply);
}

 


/*------------------------------------------------*/
/* Routine to clear a SIC channel descriptor slot */
/*------------------------------------------------*/

_PRIVATE void psrp_clear_channel_slot(psrp_channel_type *channel)

{   channel->in_stream   = channel->out_stream = (FILE *)NULL;

    if(channel->in_name != (char *)NULL)
      channel->in_name = (char *)pups_free((void *)channel->in_name);

    if(channel->out_name != (char *)NULL)
       channel->out_name = (char *)pups_free((void *)channel->out_name);

    if(channel->host_name != (char *)NULL)
       channel->host_name   = (char *)pups_free((void *)channel->host_name);

    if(channel->ssh_port  != (char *)NULL)
       channel->ssh_port = (char *)pups_free((void *)channel->ssh_port);

    if(channel->pen != (char *)NULL)
       channel->pen = (char *)pups_free((void *)channel->pen);

    channel->index       = FREE;
    channel->scp         = 0;
    channel->type        = 0;
    channel->remote      = FALSE;
}




/*-----------------------------------------------------------*/
/* Routine to close a link to slaved PSRP interaction client */
/*-----------------------------------------------------------*/

_PUBLIC psrp_channel_type *psrp_destroy_slaved_interaction_client(const psrp_channel_type        *channel,   // SIC to destroy
                                                                  const _BOOLEAN           channel_client)   // Force slaved client termination

{   int  i;
    char reply[SSIZE] = "";


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_destroy_slaved_interaction_client] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(channel == (const psrp_channel_type *)NULL)
    {  pups_set_errno(EINVAL);
       return((psrp_channel_type *)NULL);
    }

    if(appl_verbose == TRUE)
    {  (void)strdate(date);

       #ifdef SSH_SUPPORT
       if(channel->remote == TRUE)
       {  (void)fprintf(stderr,"%s %s (%d@%s:%s): remote (ssh tunnel) SIC [to PSRP client %s@%s] destroyed\n",
                                 date,appl_name,appl_pid,appl_host,appl_owner,channel->pen,channel->host_name);
       }
       else
       #endif /* SSH_SUPPORT */
          (void)fprintf(stderr,"%s %s (%d@%s:%s): local (duplex FIFO) SIC [to PSRP client %s] destroyed\n",
                                                 date,appl_name,appl_pid,appl_host,appl_owner,channel->pen);

       (void)fflush(stderr);
    }


    /*--------------------------------*/
    /* Destroy the interaction client */ 
    /*--------------------------------*/

    if(channel_client == TRUE)
       (void)psrp_write_sic(channel,"quit\n");

    (void)pups_fclose(channel->in_stream); 
    (void)pups_fclose(channel->out_stream); 
    (void)unlink(channel->in_name); 
    (void)unlink(channel->out_name); 


    /*-------------------------------*/
    /* Clear channel descriptor slot */
    /*-------------------------------*/

    psrp_clear_channel_slot(channel);


    /*-------------------------------------------------*/
    /* If we have run a suid root SIC we might need to */
    /* re-create /dev/tty                              */
    /*-------------------------------------------------*/

    if(access("/dev/tty",F_OK | R_OK | W_OK) == (-1))
    {  if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): /dev/tty lost - recreating (mktty)\n",
                                           date,appl_name,appl_pid,appl_host,appl_owner);
          (void)fflush(stderr);
       }

       (void)pups_system("mktty",shell,PUPS_ERROR_EXIT,(int *)NULL);
    }

    pups_set_errno(OK);
    return((psrp_channel_type *)NULL);
}




/*----------------------------------------------------------------*/
/* Support routine for stream I/O required by PSRP task functions */
/*----------------------------------------------------------------*/

_PUBLIC int psrp_assign_stdio(const FILE  *psrp_out,   // PSRP output channel
                              const int       *argc,   // Number of arguments
                              const char    *argv[],   // Argument vector
                              int           *in_des,   // Input descriptor
                              int          *out_des,   // Output descriptor
                              int          *err_des)   // Error/status logging descriptor

{    char in_name[SSIZE]  = "",
          out_name[SSIZE] = "",
          err_name[SSIZE] = ""; 


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_assign_stdio] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(psrp_out == (const FILE *)NULL     ||
       argc     == (const int *) NULL     ||
       argv     == (const char **)NULL    ||
       in_des   == (const int *)  NULL    ||
       out_des  == (const int *)  NULL    ||
       err_des  == (const int *)  NULL     )
    {  pups_set_errno(EINVAL);
       return(PSRP_STDIO_ERROR);
    }

    if((ptr = pups_locate(&init,"in",argc,argv,0)) != NOT_FOUND    ||
       (ptr = pups_locate(&init,"i", argc,argv,0)) != NOT_FOUND     )
    {  if(strccpy(in_name,pups_str_dec(&ptr,argc,args)) == (char *)NULL)
       {  (void)fprintf(psrp_out,"assign_psrp_stdio: expecting input file name");
          (void)fflush(psrp_out);

          pups_set_errno(EINVAL);
          return(PSRP_STDIO_ERROR);
       }

       if((*in_des = open(in_name,2)) == (-1))
       {  (void)fprintf(psrp_out,"assign_psrp_stdio: (input) stream %s could not be opened\n");
          (void)fflush(psrp_out);

          pups_set_errno(EINVAL);
          return(PSRP_STDIO_ERROR);
       }
    }
    else
       *in_des = open("/dev/null",2);   

    if((ptr = pups_locate(&init,"out",argc,argv,0)) != NOT_FOUND    ||
       (ptr = pups_locate(&init,"o"  ,argc,argv,0)) != NOT_FOUND     )
    {  if(strccpy(out_name,pups_str_dec(&ptr,argc,args)) == (char *)NULL)
       {  (void)fprintf(psrp_out,"assign_psrp_stdio: expecting output file name");
          (void)fflush(psrp_out);

          pups_set_errno(EINVAL);
          return(PSRP_STDIO_ERROR);
       }

       if((*out_des = open(out_name,2)) == (-1))
       {  (void)fprintf(psrp_out,"assign_psrp_stdio: (output) stream %s could not be opened\n");
          (void)fflush(psrp_out);

          pups_set_errno(EINVAL);
          return(PSRP_STDIO_ERROR);
       }
    }
    else
       *out_des = open("/dev/null",2);   

    if((ptr = pups_locate(&init,"err",argc,argv,0)) != NOT_FOUND    ||
       (ptr = pups_locate(&init,"e"  ,argc,argv,0))  != NOT_FOUND    )
    {  if(strccpy(err_name,pups_str_dec(&ptr,argc,args)) == (char *)NULL)
       {  (void)fprintf(psrp_out,"assign_psrp_stdio: expecting error (log) file name");
           (void)fflush(psrp_out);

          pups_set_errno(EINVAL);
          return(PSRP_STDIO_ERROR);
       }
 
       if((*err_des = open(err_name,2)) == (-1))
       {  (void)fprintf(psrp_out,"assign_psrp_stdio: (error) stream %s could not be opened\n");
          (void)fflush(psrp_out);

          pups_set_errno(EINVAL);
          return(PSRP_STDIO_ERROR);
       }
    }
    else
       *err_des = open("/dev/null",2);   

    pups_set_errno(OK); 
    return(PSRP_STDIO_ASSIGNED);
}



/*-------------------------------------------------------*/
/* Assign a set of input, output and error (log) streams */
/*-------------------------------------------------------*/

_PUBLIC int psrp_assign_fstdio(const FILE    *psrp_out, // PSRP output channel
                               const int         *argc, // Number of arguments
                               const char      *argv[], // Argument vector
                               FILE         *in_stream, // Input stream
                               FILE        *out_stream, // Output stream
                               FILE        *err_stream) // Error/logging stream

{   int in,
        out,
        err,
        ret; 


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_assign_fstdio] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(psrp_out    == (const FILE *) NULL    ||
       argc        == (const int *)  NULL    ||
       argv        == (const char **)NULL    ||
       in_stream   == (const FILE *) NULL    ||
       out_stream  == (const FILE *) NULL    ||
       err_stream  == (const FILE *) NULL     )
    {  pups_set_errno(EINVAL);
       return(PSRP_STDIO_ERROR);
    }

    ret = psrp_assign_stdio(psrp_out,argc,argv,&in,&out,&err);

    if(ret != PSRP_STDIO_ERROR)
    {  in_stream  = fdopen(in,"r"),
       out_stream = fdopen(out,"w"),
       err_stream = fdopen(err,"w");
    }

    return(ret);
}





/*---------------------------------------*/
/* Wait for data on nominated descriptor */
/*---------------------------------------*/

_PUBLIC int psrp_wait_for_data(const int fdes)

{   fd_set readfds,
           excepfds;

    struct timeval timeout;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_wait_for_data] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(fdes < 0 || fdes >= appl_max_files)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    timeout.tv_sec  = 0;
    timeout.tv_usec = 100;

    FD_ZERO(&readfds);
    FD_ZERO(&excepfds);
    FD_SET(fdes,&readfds);
    FD_SET(fdes,&excepfds);


    /*-----------------------------------------------------------*/
    /* Wait for data to become available on nominated descriptor */
    /*-----------------------------------------------------------*/

    do {   while(select(1,&readfds,(fd_set *)NULL,(fd_set *)NULL,&timeout) < 0);


           /*-------------------------*/
           /* Invalid file descriptor */
           /*-------------------------*/
 
           if(errno == EBADF)
              return(-1);

           timeout.tv_sec  = 0;
           timeout.tv_usec = 100;

           if(FD_ISSET(0,&readfds))
           {  pups_set_errno(OK);
              return(0);
           }

           FD_SET(0,&readfds);
           FD_SET(1,&readfds);
        } while(TRUE);

    pups_set_errno(OK);
    return(0);
}




/*-----------------------------------------*/
/* Search tag list for the first free slot */
/*-----------------------------------------*/

_PUBLIC int psrp_get_tag_index(const unsigned int tag_index)

{   int i;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_get_tag_index] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(tag_index > psrp_object_list_used)
    {  pups_set_errno(EINVAL);
       return(PSRP_DISPATCH_ERROR);
    }

    if(psrp_object_list[tag_index].aliases == psrp_object_list[tag_index].aliases_allocated)
    {  psrp_object_list[tag_index].aliases_allocated += PSRP_ALLOCATION_QUANTUM;
       psrp_object_list[tag_index].object_tag         = (char **)pups_realloc((void *)psrp_object_list[tag_index].object_tag,
                                                                              psrp_object_list[tag_index].aliases_allocated*sizeof(char **));

       ++psrp_object_list[tag_index].aliases;
       for(i=psrp_object_list[tag_index].aliases; i<psrp_object_list[tag_index].aliases_allocated; ++i)
          psrp_object_list[tag_index].object_tag[i] = (char *)NULL;

       pups_set_errno(OK);
       return(psrp_object_list[tag_index].aliases - 1);
    }

    for(i=0; i<psrp_object_list[tag_index].aliases_allocated; ++i)
       if(psrp_object_list[tag_index].object_tag[i] == (char *)NULL)
       {  ++psrp_object_list[tag_index].aliases;

          pups_set_errno(OK);
          return(i);
       }

    pups_set_errno(ESRCH);
    return(PSRP_OK);
}


 
 
/*---------------------------------------*/
/* Free resources allocated to tag index */
/*---------------------------------------*/

_PUBLIC int psrp_ifree_tag_list(const unsigned int tag_index)

{   int i;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_ifree_tag_list] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(tag_index > psrp_object_list_used)
    {  pups_set_errno(EINVAL);
       return(PSRP_DISPATCH_ERROR);
    }

    if(psrp_object_list[tag_index].object_handle != (void *)NULL    &&
       psrp_object_list[tag_index].object_tag    != (void *)NULL     )
    {   for(i=0; i<psrp_object_list[i].aliases_allocated; ++i)
        {   if(psrp_object_list[tag_index].object_tag[i] != (char *)NULL)
               (void)pups_free((void *)psrp_object_list[tag_index].object_tag[i]);
        }
 
        psrp_object_list[tag_index].object_tag = (char **)NULL; 

        pups_set_errno(OK);
        return(0);
    }

    pups_set_errno(ESRCH);
    return(-1);
}





/*--------------------------*/
/* Search tag list by index */
/*--------------------------*/

_PUBLIC int psrp_isearch_tag_list(const char *object_tag, const unsigned int tag_index)

{   int i;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_isearch_tag_list] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(object_tag == (const char *)NULL || tag_index > psrp_object_list_used)
    {  pups_set_errno(EINVAL);
       return(PSRP_DISPATCH_ERROR);
    }

    for(i=0; i<psrp_object_list[tag_index].aliases_allocated; ++i)
    {  if(psrp_object_list[tag_index].object_tag[i] != (char *)NULL       &&
          strcmp(psrp_object_list[tag_index].object_tag[i],object_tag) == 0)
       {  pups_set_errno(OK);
          return(i);
       }
    }

    pups_set_errno(ESRCH);
    return(PSRP_DISPATCH_ERROR);
}




/*-----------------------------------*/
/* Create an alias for a PSRP object */
/*-----------------------------------*/

_PUBLIC int psrp_alias(const char *name, const char *alias)

{   int i,
        j,
        alias_index;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_alias] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(name == (const char *)NULL || alias == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(PSRP_DISPATCH_ERROR);
    }

    /*-------------------------------------------*/
    /* Search dispatch table for object to alias */
    /*-------------------------------------------*/

    for(i=0; i<psrp_object_list_used; ++i)
    {  if(psrp_object_list[i].object_handle != NULL)
       {

          /*----------------------------------------------------------------*/
          /* Search for PSRP object tagged with target name (to be aliased) */ 
          /*----------------------------------------------------------------*/

          if(psrp_isearch_tag_list(name,i) >= 0) 
          {

             /*---------------------------------*/
             /* Check that this alias is unique */
             /*---------------------------------*/

             if(psrp_isearch_tag_list(alias,i) >= 0)
             {  pups_set_errno(ESRCH);
                return(PSRP_DISPATCH_ERROR);
             }
             else
             {  alias_index = psrp_get_tag_index(i);

                if(psrp_object_list[i].object_tag[alias_index] == (char *)NULL)
                   psrp_object_list[i].object_tag[alias_index] = (char *)pups_malloc(SSIZE);
                (void)strlcpy(psrp_object_list[i].object_tag[alias_index],alias,SSIZE);

                if(appl_verbose == TRUE)
                {  (void)fprintf(stderr,"%s %s (%d@%s:%s): %s aliased to %s\n",
                                                                          date,
                                                                     appl_name,
                                                                      appl_pid,
                                                                     appl_host,
                                                                    appl_owner,
                                             psrp_object_list[i].object_tag[0],
                                                                         alias);
                   (void)fflush(stderr);
                }

                pups_set_errno(OK);
                return(PSRP_OK);
             }
          }
        }   
     }

     pups_set_errno(ESRCH);
     return(PSRP_DISPATCH_ERROR);
}





/*--------------------------------------------*/
/* Remove alias associated with a PSRP object */
/*--------------------------------------------*/
 
_PUBLIC int psrp_unalias(const char *alias, const char *base_tag_name)
 
{   int i,
        j,
        alias_index;
 

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_unalias] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(base_tag_name == (const char *)NULL || alias == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(PSRP_DISPATCH_ERROR);
    }


    /*---------------------------------------------*/
    /* Search dispatch table for object to unalias */
    /*---------------------------------------------*/

    for(i=0; i<psrp_object_list_used; ++i)
    {   if(psrp_object_list[i].object_handle != NULL)
        {  if((alias_index = psrp_isearch_tag_list(alias,i)) >= 0)
           {

              /*----------------------------------------------------------*/
              /* Check that this is not the root alias (the real name) of */
              /* this object. We cannot remove this as it is used to map  */
              /* and unmap the object if it a DLL object                  */
              /*----------------------------------------------------------*/

              if(alias_index == 0)
              {  pups_set_errno(EPERM);
                 return(PSRP_TAG_ERROR);
              }

              if(appl_verbose == TRUE) 
              {  (void)fprintf(stderr,"%s %s (%d@%s:%s): %s unliased from %s\n",
                                                                           date,
                                                                      appl_name,
                                                                       appl_pid,
                                                                      appl_host,
                                                                     appl_owner,
                                                                          alias,
                                              psrp_object_list[i].object_tag[0]);

                 (void)fflush(stderr);
              }

              (void)pups_free((void *)psrp_object_list[i].object_tag[alias_index]);
              psrp_object_list[i].object_tag[alias_index] = (char *)NULL;

              (void)strlcpy(base_tag_name,psrp_object_list[i].object_tag[0],SSIZE);
              --psrp_object_list[i].aliases;

              pups_set_errno(OK); 
              return(PSRP_OK);
           }
        }
    }
      
    pups_set_errno(ESRCH); 
    return(PSRP_DISPATCH_ERROR);
}




/*------------------------------------------------------------------*/
/* Routine to display tag names associated with a given PSRP object */
/*------------------------------------------------------------------*/
 
_PUBLIC int psrp_show_aliases(const char *basename)
 
{   int i,
        j,
        alias_index;
 

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_show_aliases] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(basename == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(PSRP_DISPATCH_ERROR);
    }

    for(i=0; i<psrp_object_list_used; ++i)
    {  if(psrp_object_list[i].object_handle != (void *)NULL)
       {  alias_index = psrp_isearch_tag_list(basename,i);
 
         if(alias_index >= 0)
         {  (void)fprintf(psrp_out,"\n\n    Aliases list containing object tag \"%s\"\n\n",basename); 
            for(j=0; j<psrp_object_list[i].aliases_allocated; ++j)
               if(psrp_object_list[i].object_tag[j] != (char *)NULL)
               {  if(j == 0)
                     (void)fprintf(psrp_out,"%04d:    \"%-32s\" (root tag of PSRP object at %016lx virtual)\n",j,
                                                                               psrp_object_list[i].object_tag[j],
                                                            (unsigned long int)psrp_object_list[i].object_handle);
                  else
                     (void)fprintf(psrp_out,"%04d:    \"%-32s\"\n",j,psrp_object_list[i].object_tag[j]);
               }
                 
            (void)fprintf(psrp_out,"\n\n");
            (void)fflush(psrp_out);
 
            pups_set_errno(OK);
            return(PSRP_OK);
         }
       }
    }
 
    (void)fprintf(psrp_out,"psrp_show_aliases: \"%s\" is not a tag name of any loaded PSRP object\n",basename);       
    (void)fflush(psrp_out);

    pups_set_errno(ESRCH); 
    return(PSRP_DISPATCH_ERROR);
}




/*---------------------------------------------------------*/
/* Routine to check if a channel has a client locked on it */
/*---------------------------------------------------------*/

_PUBLIC _BOOLEAN psrp_channel_locked(const FILE *stream, const char *channel_name)

{   struct stat buf;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_channel_locked] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(channel_name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }
  
    if(stat(channel_name,&buf) == (-1))
    {  pups_set_errno(ENFILE);
       return(-1);
    }

    pups_set_errno(OK);

    if(buf.st_nlink > 1)
       return(TRUE);

    return(FALSE);
}





/*----------------------------------------------------------------------*/
/* Attach a dynamic databag to the dispatch list of the current process */
/*----------------------------------------------------------------------*/
 
_PRIVATE int psrp_builtin_attach_dbag(const int argc, const char *argv[])
 
{   char databag_file_name[SSIZE] = "",
         bag_tag[SSIZE]           = "";

    int h_mode; 

    if(strcmp(argv[0],"bag") != 0)
       return(PSRP_DISPATCH_ERROR);

    if(!(psrp_bind_status & PSRP_DYNAMIC_DATABAG))
    {  if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): permission denied (attach dynamic databag)\n\n",
                                                     date,appl_name,appl_pid,appl_host,appl_owner);
          (void)fflush(stderr);
       }

       (void)fprintf(psrp_out,"\nPermision denied (attach dynamic databag)\n\n");
       (void)fflush(psrp_out);
       return(PSRP_OK);
    }
 
    if(argc < 2 || argc > 4)
    {  (void)fprintf(psrp_out,"usage: bag <PSRP dispatch name> [<databag file name>] [LIVE | DEAD]\n");
       (void)fflush(psrp_out);
       return(PSRP_OK);
    }

    if(argc == 2)
    {  if(strcmp(argv[1],"LIVE") == 0 || strcmp(argv[1],"DEAD") == 0) 
       {  (void)fprintf(psrp_out,"usage: bag <PSRP dispatch name> [<databag file name>] [LIVE | DEAD]\n");
          (void)fflush(psrp_out);
          return(PSRP_OK);
       }

       (void)strlcpy(bag_tag,argv[1],SSIZE);
       (void)strlcpy(databag_file_name,argv[1],SSIZE);
    } 
    else if(argc == 3)
    {  (void)strlcpy(bag_tag,argv[1],SSIZE);

       if(strcmp(argv[2],"LIVE") == 0)
       {  h_mode = LIVE;
          (void)strlcpy(databag_file_name,argv[1],SSIZE);
       }
       else if(strcmp(argv[2],"DEAD") == 0)
       {  h_mode = DEAD;
          (void)strlcpy(databag_file_name,argv[1],SSIZE);
       }
       else
          (void)strlcpy(databag_file_name,argv[1],SSIZE);
    }
    else if(argc == 4)
    {  (void)strlcpy(bag_tag,argv[1],SSIZE);
       (void)strlcpy(databag_file_name,argv[2],SSIZE);
       if(strcmp(argv[3],"LIVE") == 0)
          h_mode = LIVE;
       else if(strcmp(argv[3],"DEAD") == 0)
          h_mode = DEAD;
       else
       {  (void)fprintf(psrp_out,"usage: bag <PSRP dispatch name> [<databag file name>] [LIVE | DEAD]\n");
          (void)fflush(psrp_out);
           return(PSRP_OK);
       }

       (void)strlcpy(databag_file_name,argv[2],SSIZE);
    }
 
    if(psrp_bind_status & PSRP_DYNAMIC_DATABAG)
    {  if(psrp_attach_dynamic_databag(TRUE,
                                      bag_tag,
                                      databag_file_name,
                                                 h_mode) == PSRP_DISPATCH_ERROR)
       {
 
           /*--------------------------------------------------------------------*/
           /* If we could not find an action matching the clients tag report the */
           /* error back to the client.                                          */
           /*--------------------------------------------------------------------*/
 
           (void)fprintf(psrp_out,"\nfailed to attach dynamic databag \"%s\" (from %s) to PSRP handler action list\n\n",
                                                                                               argv[1],databag_file_name);
           (void)fflush(psrp_out);
       }
       else
       {  (void)fprintf(psrp_out,"\ndynamic databag \"%s\" bound to PSRP handler action list\n\n");
          (void)fflush(psrp_out);
 
          (void)strdate(date);
          if(appl_verbose == TRUE)
          {  (void)fprintf(stderr,"%s %s (%d@%s:%s): dynamic databag \"%s\" attached to PSRP handler (imported from file %s)\n",
                                                         date,appl_name,appl_pid,appl_host,appl_owner,argv[1],databag_file_name);
             (void)fflush(stderr);
          }
       }   
    }   

    return(PSRP_OK);
}




#ifdef PERSISTENT_HEAP_SUPPORT
/*-----------------------------------------------------------------------*/
/* Attach a dynamic function to the dispatch list of the current process */
/*-----------------------------------------------------------------------*/


_PRIVATE int psrp_builtin_attach_persistent_heap(const int argc, const char *argv[])

{   char heap_file_name[SSIZE] = "",
         heap_tag[SSIZE]       = "";

    int h_mode   = DEAD;
 
    if(strcmp(argv[0],"heap") != 0)
       return(PSRP_DISPATCH_ERROR);
 
    if(!(psrp_bind_status & PSRP_PERSISTENT_HEAP))
    {  int  h_mode;

       (void)strdate(date);
       (void)fprintf(psrp_out,"Permision denied (attach persistent heap)\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(argc < 2 || argc > 4)
    {  (void)fprintf(psrp_out,"usage: heap <PSRP dispatch name> !<persistent heap file name>! [LIVE | DEAD]\n");
       (void)fflush(psrp_out); 
       return(PSRP_OK);
    }

    if(argc == 2)
    {  if(strcmp(argv[1],"LIVE") == 0 || strcmp(argv[1],"DEAD") == 0)
       {  (void)fprintf(psrp_out,"usage: heap <PSRP dispatch name> !map mode! [<persistent heap file name>] [LIVE | DEAD]\n");
          (void)fflush(psrp_out);
          return(PSRP_OK);
       }

       (void)strlcpy(heap_tag,argv[1],SSIZE);
       (void)strlcpy(heap_file_name,argv[1],SSIZE);
    }    
    else if(argc == 3)
    {  (void)strlcpy(heap_tag,argv[1],SSIZE);
       (void)strlcpy(heap_file_name,argv[2],SSIZE);
    }
    else if(argc == 4)
    {  (void)strlcpy(heap_tag,argv[1],SSIZE);
       (void)strlcpy(heap_file_name,argv[3],SSIZE);

       if(strcmp(argv[4],"LIVE") == 0)
          h_mode = LIVE; 
       else if(strcmp(argv[4],"DEAD") == 0)
          h_mode = DEAD;
       else 
       {  (void)fprintf(psrp_out,"usage: heap <PSRP dispatch name> !map mode! [<persistent heap file name>] [LIVE | DEAD]\n");
          (void)fflush(psrp_out);
          return(PSRP_OK);
       }
    }
 
    (void)psrp_attach_persistent_heap(TRUE,
                                      heap_tag,
                                      heap_file_name,
				      h_mode);

    return(PSRP_OK);
}

#endif /* PERSISTENT_HEAP_SUPPORT */

 


#ifdef DLL_SUPPORT
/*-----------------------------------------------------------------------*/
/* Attach a dynamic function to the dispatch list of the current process */
/*-----------------------------------------------------------------------*/

_PRIVATE int psrp_builtin_attach_dynamic_function(const int argc, const char *argv[])

{   char object_tag[SSIZE]      = "",
         object_tag_DLL[SSIZE] = "";

    if(strcmp(argv[0],"dll") != 0)
       return(PSRP_DISPATCH_ERROR);

    if(!(psrp_bind_status & PSRP_DYNAMIC_FUNCTION))
    {  if(appl_verbose == TRUE)
       {  char date[SSIZE] = "";

          (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s): permission denied (attach dynamic function)\n",
                                                            date,appl_name,appl_pid,appl_host);
          (void)fflush(stderr);
       }

       (void)fprintf(stderr,"\nPermission denied (attach dynamic function)\n\n");
       (void)fflush(stderr); 

       return(PSRP_OK);
    }

    if(argc < 2 || argc > 3)
    {  (void)fprintf(psrp_out,"usage: dll <PSRP dispatch name> [<object DLL>]\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(argc == 3)
    {  (void)strlcpy(object_tag,argv[1],SSIZE);
       (void)strlcpy(object_tag_DLL,argv[2],SSIZE);
    }
    else
       (void)strlcpy(object_tag_DLL,argv[1],SSIZE);

    if(access(object_tag_DLL,F_OK | R_OK | W_OK) == (-1))
    {  (void)fprintf(psrp_out,"cannot find DLL \"%s\"\n",object_tag,object_tag_DLL);
       (void)fflush(psrp_out);
    }
    else if(psrp_attach_dynamic_function(TRUE,
                                         object_tag,
                                         object_tag_DLL) == (-1))
    {  (void)fprintf(psrp_out,"error attaching \"%s\" (from DLL \"%s\")\n",object_tag,object_tag_DLL);
       (void)fflush(psrp_out);
    }
    else
       (void)fprintf(psrp_out,"dynamic function \"%s\" attached (from DLL \"%s\")\n",
                                                            object_tag,object_tag_DLL);
    return(PSRP_OK);
}

#endif /* DLL_SUPPORT */




/*----------------------------------------------------------------------------*/
/* Start the next segment of a multisegment server - If host is non null then */
/* start the new segment on that host and call it name. If we have a          */
/* checkpoint file restart using the checkpoint stored in that file           */
/*----------------------------------------------------------------------------*/

#ifdef SSH_SUPPORT
_PUBLIC int psrp_new_segment(const char *name,             // Name of process
                             const char *host_name,        // Name of new host
                             const char *ssh_port,         // Ssh port number to use 
                             const char *ckpt_file_name)   // CRIU checkpoint associated with process
#else
_PUBLIC int psrp_new_segment(const char *name,             // Name of process
                             const char *host_name,        // Name of new host
                             const char *ckpt_file_name)   // CRIU checkpoint associated with process
#endif /* SSH_SUPPORT */
{   int i,
        ssh_pid;

    sigset_t set;

    char line[SSIZE]               = "",
         command_line[SSIZE]       = "",
         cmd_tail[SSIZE]           = "",
         p_name[SSIZE]             = "",
         autoload_file_name[SSIZE] = "",
         exec_pathname[SSIZE]      = "",
         ckpt_restart[SSIZE]       = "",
         errstr[SSIZE]             = "";


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_new_segment] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }
 

    /*----------------------------------------------------------------------------*/
    /* Make sure that we ignore any yields from (multiple) clients until we have  */
    /* finished segmentation.                                                     */
    /*----------------------------------------------------------------------------*/

    in_psrp_new_segment = TRUE;


    /*--------------------------------------------------------------------------*/
    /* Save dispatch table so any aliases defined by this segment are passed to */
    /* the next.                                                                */
    /*--------------------------------------------------------------------------*/
 
    if(appl_psrp_save == TRUE)
    {  

       /*----------------------------------------------------------------*/
       /* Save the names of those objects which are to be automatically  */
       /* loaded at the next invocation of this server                   */
       /*----------------------------------------------------------------*/

       if(getuid() != 0)
          (void)snprintf(autoload_file_name,SSIZE,"%s/.%s.psrp",appl_home,name);
       else
          (void)snprintf(autoload_file_name,SSIZE,"/%s.psrp",name);

       if(appl_verbose == TRUE)
       {  strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): saving PSRP dispatch table (to [default] PSRP resource file %s)\n",
                                                     date,appl_name,appl_pid,appl_host,appl_owner,autoload_file_name);
          (void)fflush(stderr);
       }

       psrp_save_dispatch_table(autoload_file_name);
    }


    /*-----------------------------------------------------------------------------*/
    /* Make sure that we don't catch any signals until the new segment of the PSRP */
    /* server is up and running.                                                   */
    /*-----------------------------------------------------------------------------*/


    /*-----------------------------------------------------*/
    /* Don't raise SIGALRM during the segmentation process */
    /*-----------------------------------------------------*/

    (void)pups_malarm(0);
    (void)pups_sighandle(SIGALRM,(char *)NULL,SIG_IGN,(sigset_t *)NULL);

    if(host_name != (char *)NULL)
    {  (void)pups_sighandle(SIGINIT,  (char *)NULL, SIG_IGN, (sigset_t *)NULL);
       (void)pups_sighandle(SIGCHAN,  (char *)NULL, SIG_IGN, (sigset_t *)NULL);
       (void)pups_sighandle(SIGPSRP,  (char *)NULL, SIG_IGN, (sigset_t *)NULL);
       (void)pups_sighandle(SIGALIVE, (char *)NULL, SIG_IGN, (sigset_t *)NULL);
       (void)pups_sighandle(SIGCHLD,  (char *)NULL, SIG_IGN, (sigset_t *)NULL);
       (void)pups_sighandle(SIGINT,   (char *)NULL, SIG_IGN, (sigset_t *)NULL);
       (void)pups_sighandle(SIGCLIENT,(char *)NULL, SIG_IGN, (sigset_t *)NULL);


       /*----------------------------------------------------------*/
       /* Make sure all PUPS signals are serviced (while ignored)  */
       /* this stops spurious signals when the next segment starts */
       /*----------------------------------------------------------*/

       (void)sigemptyset(&set);
       (void)sigaddset(&set,SIGINIT);
       (void)sigaddset(&set,SIGCHAN);
       (void)sigaddset(&set,SIGPSRP);
       (void)sigaddset(&set,SIGALIVE);
       (void)sigaddset(&set,SIGALRM);
       (void)sigaddset(&set,SIGINT);
       (void)sigaddset(&set,SIGCHLD);
       (void)sigaddset(&set,SIGCLIENT);

       (void)pups_sigprocmask(SIG_UNBLOCK,&set,(sigset_t *)NULL);
    }


    /*--------------------------------------------------------*/
    /* Build command and overlay current process using execls */ 
    /*--------------------------------------------------------*/

    pups_argtline(cmd_tail);


    /*------------------------------------------------------*/
    /* If the user has specified a name for the new segment */
    /* make sure that it answers to it!                     */
    /*------------------------------------------------------*/

    if(name != (char *)NULL)
       (void)strlcpy(p_name,name,SSIZE);
    else
       (void)strlcpy(p_name,appl_name,SSIZE);


    /*-------------------------*/
    /* Expand command pathname */
    /*-------------------------*/

    if(appl_bin_name != (char *)NULL && strcmp(appl_bin_name,"") != 0)
    {  char stripped_bin_name[SSIZE] = "";


       /*---------------------------------------*/
       /* Need to strip path leading to binname */
       /*---------------------------------------*/

       if((char *)rindex(appl_bin_name,'/') != (char *)NULL)
       {  char tmpstr[SSIZE] = "";

          (void)strlcpy(tmpstr,(char *)rindex(appl_bin_name,'/'),SSIZE);
          (void)strlcpy(stripped_bin_name,&tmpstr[1],SSIZE);
       }
       else
          (void)strlcpy(stripped_bin_name,appl_bin_name,SSIZE); 

       if(strccpy(exec_pathname,pups_search_path("PATH",stripped_bin_name)) == (char *)NULL)
       {  pups_set_errno(ENOENT);
          return(-1);
       }
    }
    else 
    {  if(strccpy(exec_pathname,pups_search_path("PATH",appl_name)) == (char *)NULL)
       {  pups_set_errno(ENOENT);
          return(-1);
       }
    }

    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,"PSRPLIB APPL BIN NAME: %s\n",appl_bin_name);
    (void)fflush(stderr);
    #endif /* "PSRPLIB_DEBUG */


    /*-------------------------------------*/
    /* We must have absolute command paths */
    /*-------------------------------------*/

    if(exec_pathname[0] == '.' && exec_pathname[1] == '/')
    {  char cwd[SSIZE] = "";

       (void)getcwd(cwd,SSIZE);
       (void)strlcat(cwd,&exec_pathname[1],SSIZE);
       (void)strlcpy(exec_pathname,cwd,SSIZE);
    }
    else if(exec_pathname[0] == '.' && exec_pathname[1] == '.')
    {  char uwd[SSIZE] = "",
            cwd[SSIZE] = "";

       (void)getcwd(cwd,SSIZE); 
       (void)chdir("..");
       (void)getcwd(uwd,SSIZE);
       (void)strlcat(uwd,&exec_pathname[2],SSIZE);
       (void)strlcpy(exec_pathname,uwd,SSIZE);
       (void)chdir(cwd);
    }


    #ifdef SSH_SUPPORT
    /*---------------------------------------*/
    /* Is the new instance on a remote node? */
    /*---------------------------------------*/

    if(host_name != (char *)NULL && strcmp(host_name,"localhost") != 0)
    {

       /*-------------------------------------*/
       /* Build command to start new instance */
       /*-------------------------------------*/

       (void)snprintf(command_line,SSIZE,"%s -pen %s %s -psrp_segment %d %s >& /dev/null",exec_pathname, 
                                                                                          p_name,
                                                                                    ckpt_restart,
                                                                                  ++psrp_seg_cnt,
                                                                                        cmd_tail);



       /*---------------------*/
       /* Child side of fork */
       /*---------------------*/

       if((ssh_pid = pups_fork(FALSE,FALSE)) == 0)
       {  char sshPortOpt[SSIZE] = "";


          /*-------------*/
          /* Close files */
          /*-------------*/

          (void)pups_closeall();

          (void)close(0);
          (void)close(1);
          (void)close(2);


          /*---------------------------------*/
          /* Are we using non-standard port? */
          /*---------------------------------*/

          if(ssh_port != (char *)NULL && strcmp(ssh_port,"") != 0)
             (void)snprintf(sshPortOpt,SSIZE,"-p %s",ssh_port);
          else
             (void)snprintf(sshPortOpt,SSIZE,"-p %s",ssh_remote_port);


          /*--------------------------------------*/
          /* We are not using passwords. You will */
          /* need to generate a public/private    */
          /* keyset for this to work              */
          /*--------------------------------------*/

          if(ssh_compression == TRUE)
             (void)execlp("ssh","ssh",sshPortOpt,"-C","-l",ssh_remote_uname,host_name,command_line,(char *)NULL);
          else
             (void)execlp("ssh","ssh",sshPortOpt,"-l",     ssh_remote_uname,host_name,command_line,(char *)NULL);


          /*---------------------------------------------------------*/
          /* We should not get here -- if we do an error has occured */
          /*---------------------------------------------------------*/

          _exit(255);
       }

       /*---------------------*/
       /* Parent side of fork */
       /*---------------------*/
 
       else 
       {  int status = 0;


          /*-----------------------------------------------------*/
          /* If the remote end of the ssh connection is still    */
          /* running after 5 seconds  we can assume that command */
          /* is successfully running on remote server            */
          /*-----------------------------------------------------*/

          (void)pups_sleep(1);

          #ifdef PSRPLIB_DEBUG
          (void)fprintf(stderr,"PSRPLIB SSH EXEC WAITPID: %d\n",waitpid(ssh_pid,&status, WNOHANG));
          (void)fflush(stderr);
          #endif /* PSRPLIB_DEBUG */


          if(waitpid(ssh_pid,&status, WNOHANG) == (-1))
          {  

             #ifdef PSRPLIB_DEBUG
             (void)fprintf(stderr,"PSRPLIB SSH EXEC FAILED\n");
             (void)fflush(stderr);
             #endif /* PSRPLIB_DEBUG */

             if(appl_verbose == TRUE)
             {  (void)strdate(date);
                (void)fprintf(stderr,"%s %s (%d@%s:%s): failed to create remote (ssh tunnel) channel [to host %s] (could not exec ssh)\n",
                                                                                   date,appl_name,appl_pid,appl_host,appl_owner,host_name);
                (void)fflush(stderr);
             }

             (void)pups_malarm(1);
             in_psrp_new_segment = FALSE;
             pups_set_errno(ENOEXEC);

             return(-1);
          }
          else
          {
             #ifdef PSRPLIB_DEBUG
             (void)fprintf(stderr,"PSRPLIB SSH EXEC GOOD\n");
             (void)fflush(stderr);
             #endif /* PSRPLIB_DEBUG */


             /*----------------------------------*/
             /* Delay is a kludge - we have      */
             /* to make sure command has started */
             /* at remote end before we stop ssh */
             /*----------------------------------*/

             (void)pups_sleep(segmentation_delay);


             /*----------------------------------*/
             /* Kill hanging instance of ssh     */
             /* on local host                    */
             /*----------------------------------*/

             (void)kill(ssh_pid,SIGTERM);
          }

          /*----------------------------------------------*/
          /* Tell clients that a server change is pending */
          /*----------------------------------------------*/

          (void)psrp_reactivate_clients();
       }


       /*-------------------------------------------------------*/
       /* If we are segmenting parent effectively must delete   */
       /* PSRP channels and then create a regular file (whose   */
       /* name is the same as the input channel) which contains */
       /* a "trail" to the new location of the server           */
       /*-------------------------------------------------------*/


       #ifdef PSRPLIB_DEBUG
       (void)fprintf(stderr,"PSRPLIB TERMINATE CURRENT INSTANCE: %d\n",terminate_current_instance);
       (void)fflush(stderr);
       #endif /* PSRPLIB_DEBUG */

       if(terminate_current_instance == TRUE)
       {  char trailfile_name[SSIZE] = "";


           #ifdef PSRPLIB_DEBUG
           (void)fprintf(stderr,"PSRPLIB CREATING TRAILFILE\n");
           (void)fflush(stderr);
           #endif /* PSRPLIB_DEBUG */


          /*---------------------------------------------------------------*/
          /* Make sure that remote invocation can read PSRP initialisation */
          /* file (assuming it can see the same filesystem as exec host)   */
          /*---------------------------------------------------------------*/

          (void)strlcpy(appl_name,p_name,SSIZE);
          (void)snprintf(trailfile_name,SSIZE,"%s.trail",channel_name_out);
          (void)psrp_create_trailfile(trailfile_name,p_name,host_name,ssh_remote_port);


           /*--------------------------------------------------*/
           /* Parent exits leaving trail to its migrated child */
           /*--------------------------------------------------*/

           pups_exit(0);
       }
    }
    #endif /* SSH_SUPPORT */

    else 
    {  

       /*-------------------------------------*/
       /* Build command to start new instance */
       /*-------------------------------------*/

       if(n_clients > 0)
       {

          /*------------------------------------------------------------------------------*/
          /* If we have attached client(s) -- we must arrange for them to be re-awakened  */
          /* once our child segment is ready to deal with them                            */
          /*------------------------------------------------------------------------------*/

          (void)snprintf(command_line,SSIZE,"%s -pen %s %s -psrp_segment %d -psrp_reactivate_client %d %s",
                                                                                      exec_pathname,
                                                                                             p_name,
                                                                                       ckpt_restart,
                                                                                     ++psrp_seg_cnt,
                                                                                          chlockdes,
                                                                                           cmd_tail);
       }
       else
       {

          /*---------------------*/
          /* No attached clients */
          /*---------------------*/

          (void)snprintf(command_line,SSIZE,"%s -pen %s %s -psrp_segment %d",
	                                                exec_pathname,
					                       p_name,
					                 ckpt_restart,
						       ++psrp_seg_cnt);
       }
 

       /*--------------------------------------------------*/
       /* Local segmentation (parent is overlaid by child) */ 
       /*--------------------------------------------------*/

       if(terminate_current_instance == TRUE)
       {  

          /*-------------------------------------------------*/
          /* Make sure PSRP FIFOS are clean for new instance */
          /*-------------------------------------------------*/

          if(psrp_in != (FILE *)NULL)
             (void)empty_fifo(fileno(psrp_in));

          if(psrp_out != (FILE *)NULL)
             (void)empty_fifo(fileno(psrp_out));


          /*-------------------------------------*/
          /* Overlay new segment on local process*/
          /*-------------------------------------*/

          (void)pups_closeall();


          /*----------------------------------*/
          /* PSRP server is changing its name */
          /*----------------------------------*/

          if(strcmp(appl_name,p_name) != 0 && appl_default_chname == TRUE)
          {

             /*----------------------------------------------------*/
             /* If we are changing name we need to rename old PSRP */
             /* channels and lockposts and tell clients that a     */
             /* server change is pending                           */
             /*----------------------------------------------------*/

             if(psrp_rename_channel(p_name) == (-1))
                return(-1);
          }


          /*---------------------------------*/
          /* PSRP server is keeping its name */
          /*---------------------------------*/

          else
          {

            /*----------------------------------------------*/
            /* Tell clients that a server change is pending */
            /*----------------------------------------------*/

            (void)psrp_reactivate_clients();
          }

          #ifdef PSRPLIB_DEBUG
          (void)fprintf(stderr,""PSRPLIB EXECLS: %s\n",command_line);
          (void)fflush(stderr);
          #endif /* PSRPLIB_DEBUG */


          /*-------------------------------------*/
          /* We should not return from this call */
          /*-------------------------------------*/

          (void)pups_execls(command_line);
 
          (void)snprintf(errstr,SSIZE,"[psrp_new_segment] exec failure (%s)\n",command_line);
          pups_error(errstr);
       }
       else
       {  int child_pid = (-1);


          /*-------------------------------------*/
          /* Build command to start new instance */
          /*-------------------------------------*/

          (void)snprintf(command_line,SSIZE,"%s -pen %s %s -psrp_segment %d",
                                                        exec_pathname,
                                                               p_name,
                                                         ckpt_restart,
                                                       ++psrp_seg_cnt);

          #ifdef PSRPLIB_DEBUG
          (void)fprintf(stderr,"PSRPLIB NEW INSTANCE COMAMND LINE is: %s\n",command_line);
          (void)fflush(stderr);
          #endif /* PSRPLIB_DEBUG */


          /*---------------------------------------*/
          /* Local fork (parent is not terminated) */ 
          /*---------------------------------------*/

          if((child_pid = pups_fork(FALSE,FALSE)) == 0)
          {

             /*---------------------------------------*/
             /* Overlay new segment on local process. */
             /* and run in new session                */
             /*---------------------------------------*/

             (void)fclose(stdin);
             stdin = fopen("/dev/tty","r");

             (void)fclose(stdout);
             stdout = fopen("/dev/tty","w");

             (void)fclose(stderr);
             stderr = fopen("/dev/tty","w");

             (void)pups_closeall();
             (void)setsid();


             /*-------------------------------------*/
             /* We should not return from this call */
             /*-------------------------------------*/

             (void)pups_execls(command_line);

             (void)snprintf(errstr,SSIZE,"[psrp_new_segment] exec failure (%s)\n",command_line);
             pups_error(errstr);
          }

          if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@%s:%s): new instance \"%s\" (%d@%s:%s) created\n",
                           date,appl_name,appl_pid,appl_host,appl_owner,name,child_pid,appl_host,appl_owner);
             (void)fflush(stderr);
          }

          (void)pups_malarm(1);
          in_psrp_new_segment = FALSE;

          pups_set_errno(OK);
          return(0);
       }
    }

    (void)pups_malarm(1);
    in_psrp_new_segment = FALSE;


    /*-----------------------------------------------*/
    /* No error if current instance is still running */
    /*-----------------------------------------------*/

    if(terminate_current_instance == FALSE)
       return(0);
    else
       pups_set_errno(ENOEXEC);

    return(-1);
}




/*-----------------------------------------------*/
/* Send end of operation (return code) to client */
/* this is the psrp service return code          */
/*-----------------------------------------------*/

_PRIVATE void psrp_endop(char *op_tag)

{   

    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,"PSRPLIB ENDOP BEGIN %s\n",op_tag);
    (void)fflush(stderr);
    #endif /* PSRPLIB_DEBUG */

    if(op_tag == (char *)NULL)
       (void)fprintf(psrp_out,"EOP\n");
    else
       (void)fprintf(psrp_out,"EOP %s\n",op_tag); 

    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,""PSRPLIB ENDOP FLUSH %s\n",op_tag);
    (void)fflush(stderr);
    #endif /* PSRPLIB_DEBUG */

    (void)fflush(psrp_out);

    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,""PSRPLIB ENDOP: %s written\n",op_tag);
    (void)fflush(stderr);
    #endif /* DEBUG */

}




/*----------------------------------------------*/
/* Return number of clients connected to server */
/*----------------------------------------------*/

_PRIVATE int psrp_connected_clients(void)

{   int i,
        cnt = 0;


    /*----------------------------*/
    /* Search for client in table */
    /*----------------------------*/

    if(n_clients > 0)
    {  for(i=0; i<MAX_CLIENTS; ++i)
          if(psrp_client_pid[i] != (-1))
             ++cnt;
    }

    return(cnt);
}




/*---------------------------------------------------------------------------*/
/* Get current client index - if the current client pid is in the channel    */
/* table, return the corresponding index, if it isn't, and the channel table */
/* is not full return a new index, otherwise return error (ENOCH)            */
/*---------------------------------------------------------------------------*/

_PRIVATE int psrp_get_client_slot(int pid)

{   int i;


    /*----------------------------*/
    /* Search for client in table */
    /*----------------------------*/

    if(n_clients > 0)
    {  for(i=0; i<MAX_CLIENTS; ++i)
          if(psrp_client_pid[i] == pid)
             return(i);
    }


    /*---------------------------*/ 
    /* Find unused slot in table */
    /*---------------------------*/ 

    for(i=0; i<MAX_CLIENTS; ++i)
    {  if(psrp_client_pid[i] == (-1))
       {  ++n_clients;
          return(i);
       }
    }

    /*---------------------------*/
    /* Table full - return error */
    /*---------------------------*/

    return(ENOCH);
}




/*--------------------------------------------*/
/* Clear current client entry in client table */
/*--------------------------------------------*/

_PRIVATE int psrp_clear_client_slot(int slot_index)

{  if(slot_index < 0 || slot_index > MAX_CLIENTS)
      return(-1);

   psrp_client_exitf[slot_index] = (void *)NULL;
   psrp_client_pid[slot_index]   = (-1);
   req_r_cnt[slot_index]         = 0;
   
   (void)strlcpy(psrp_client_name[slot_index],    "none",SSIZE);
   (void)strlcpy(psrp_client_efname[slot_index],  "none",SSIZE);
   (void)strlcpy(psrp_client_host[slot_index],    "none",SSIZE);
   (void)strlcpy(old_request_str[slot_index],     "",SSIZE);
   (void)strlcpy(psrp_remote_hostpath[slot_index],"notset",SSIZE);

   --n_clients;
   return(0);
}




/*---------------------------------------*/
/* Show clients connected to this server */
/*---------------------------------------*/

_PRIVATE int psrp_show_clients(void)

{   int i;

    (void)fprintf(psrp_out,"\n\n    Clients attached to %s (%d@%s)\n\n",appl_name,appl_pid,appl_host);
    (void)fflush(psrp_out);

    for(i=0; i<MAX_CLIENTS; ++i)
    {  if(psrp_client_pid[i] > 0)
       {  (void)fprintf(psrp_out,"%d: %s (%d@%s)",i,psrp_client_name[i],psrp_client_pid[i],psrp_client_host[i]);

          if(psrp_client_pid[i] == psrp_client_pid[c_client])
             (void)fprintf(psrp_out,"    [*** me, your client]");

          if(strcmp(psrp_remote_hostpath[i],"notset") != 0)
             (void)fprintf(psrp_out,"    [from %s]",psrp_remote_hostpath[i]);

          if((void *)psrp_client_exitf[i] != (void *)NULL)
             (void)fprintf(psrp_out,"    (exit function \"%032s\" installed at %016lx virtual)",
                                                                          psrp_client_efname[i],
                                                        (unsigned long int)psrp_client_exitf[i]);
    
          (void)fprintf(psrp_out,"\n");
          (void)fflush(psrp_out);
        }
    }

    (void)fprintf(psrp_out,"\n\n    Total of %d client(s) currently attached to server\n\n",n_clients);
    (void)fflush(psrp_out);

    return(0);
}



/*----------------------------------------------------*/
/* Read reply over slaved interaction clients channel */
/*----------------------------------------------------*/

_PUBLIC int psrp_read_sic(const psrp_channel_type *sic, char *reply)

{   

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,"IN READ SIC\n");
    (void)fflush(stderr);
    #endif /* PSRPLIB_DEBUG */

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_read_sic] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(reply == (char *)NULL || sic == (const psrp_channel_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,"CAN READ SIC\n");
    (void)fflush(stderr);
    #endif /* PSRPLIB_DEBUG */


    /*---------------------------------------------------*/
    /* Buffer in which reply is accumulated MUST be NULL */
    /* as we are going to dynamically allocate it. The   */
    /* caller is responsible for freeing up the memory   */
    /* used                                              */
    /*---------------------------------------------------*/


    /*-----------------------------------------------*/
    /* Read data from channel until we get an EOT or */
    /* other error condition                         */
    /*-----------------------------------------------*/

    (void)fgets(reply,255,sic->out_stream);

    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,"READ SIC (reply is \"%s\")\n",reply);
    (void)fflush(stderr);
    #endif /* PSRPLIB_DEBUG */


    /*---------------------------------------------*/
    /* Check for error conditions                  */
    /* EOG - end of current PSRP transaction group */
    /*---------------------------------------------*/

    if(strncmp(reply,"EOT",3) == 0)
    {  pups_set_errno(OK);
       return(PSRP_EOT);
    }


    /*-------------------------------------------------*/
    /* CST - remote peer has terminated (unexpectedly) */
    /*-------------------------------------------------*/

    if(strncmp(reply,"CST",3) == 0)
    {  pups_set_errno(EEXIST);
       return(PSRP_TERMINATED);
    }

    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,"READ SIC: OK\n");
    (void)fflush(stderr);
    #endif /* PSRPLIB_DEBUG */

    pups_set_errno(OK);
    return(PSRP_MORE);
}





/*------------------------------------------------------*/
/* Send request over slaved interaction clients channel */
/*------------------------------------------------------*/

_PUBLIC int psrp_write_sic(const psrp_channel_type *sic, const char *request)

{   char toxic_1[SSIZE] = "",
         toxic_2[SSIZE] = "",
         toxic_3[SSIZE] = "";


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_write_sic] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(sic == (const psrp_channel_type *)NULL || request == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*----------------------------*/
    /* Check that channel is open */
    /*----------------------------*/

    (void)snprintf(toxic_1,SSIZE,"%s",                    appl_name);
    (void)snprintf(toxic_2,SSIZE,"%s@localhost",appl_name,appl_host);
    (void)snprintf(toxic_2,SSIZE,"%s@%s",       appl_name,appl_host);


    /*--------------------------------------------------------*/
    /* Self interaction via SIC is very toxic -- don't do it! */
    /*--------------------------------------------------------*/

    if(strin(request,toxic_1) == TRUE || strin(request,toxic_2) == TRUE)
    {  int  ret;
       char reply[SSIZE] = "";

       (void)fputs("end\n",sic->in_stream);
       (void)fflush(sic->in_stream);


       /*------------*/
       /* Read reply */
       /*------------*/

       do {    if((ret = psrp_read_sic(sic,reply)) == PSRP_MORE)
               {  (void)fputs(reply,psrp_out);
                  (void)fflush(psrp_out);
               }
          } while(ret == PSRP_MORE);

       pups_set_errno(EINVAL);
       return(-2);
    }


    /*--------------*/
    /* Send request */
    /*--------------*/

    if(strin(request,"\n") == FALSE)
       (void)strlcat(request,"\n",SSIZE);

    (void)fputs(request,sic->in_stream);
    (void)fflush(sic->in_stream);

    pups_set_errno(OK);
    return(0);
}





/*--------------------------------------*/
/* Set current PSRP interaction channel */
/*--------------------------------------*/

_PUBLIC int psrp_set_current_sic(const psrp_channel_type *sic)

{
    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_set_current_sic] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(sic == (const psrp_channel_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    psrp_current_sic = sic;

    pups_set_errno(OK);
    return(0);
}




/*----------------------------------------*/
/* Unset current PSRP interaction channel */
/*----------------------------------------*/

_PUBLIC void psrp_unset_current_sic(void)

{

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_unset_current_sic] attempt by non root thread to perform PUPS/P3 PSRP operation");

    psrp_current_sic = (psrp_channel_type *)NULL;
}




/*-----------------------------*/
/* Send break to remote server */
/*-----------------------------*/

_PUBLIC void psrp_int_sic(const psrp_channel_type *sic)

{   char pidname[SSIZE] = "";


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("attempt by non root thread to perform PUPS/P3 PSRP operation (psrp_int_sic)");

    if(sic == (const psrp_channel_type *)NULL)
    {  pups_set_errno(EINVAL);
       return;
    }

    (void)snprintf(pidname,SSIZE,"%d",sic->scp);
    (void)pups_rkill(sic->host_name,
                     sic->ssh_port,
                     appl_owner,
                     pidname,
                     SIGINT);

    pups_set_errno(OK);
}




/*--------------------------------------------*/
/* Install an exit function for a PSRP client */
/*--------------------------------------------*/

_PUBLIC int psrp_set_client_exitf(const unsigned int chid, const char *fname, int (*func)(int))

{

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_set_client_exitf] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(chid > MAX_CLIENTS || fname == (const char *)NULL || (void *)func == (void *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    (void)strlcpy(psrp_client_efname[chid],fname,SSIZE);
    psrp_client_exitf[chid] = (void *)func;

    pups_set_errno(OK);
    return(0);
}




/*----------------------------------------------*/
/* Deinstall an exit function for a PSRP client */ 
/*----------------------------------------------*/

_PUBLIC int psrp_reset_client_exitf(const unsigned int chid)

{

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_reset_client_exitf] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(chid > MAX_CLIENTS)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    (void)strlcpy(psrp_client_efname[chid],"",SSIZE);
    psrp_client_exitf[chid] = (void *)NULL;

    pups_set_errno(OK);
    return(0);
}




/*------------------------------------------------*/
/* Overlay the current process with a new comamnd */
/*------------------------------------------------*/

_PRIVATE int psrp_builtin_overlay_server_process(const int argc, const char *argv[])

{   int  i;

    char overlay_command_name[SSIZE] = "",
         overlay_command_tail[SSIZE] = "";

    if(strcmp(argv[0],"overlay") != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc < 2)
    {  (void)fprintf(psrp_out,"usage: overlay <command to overlay>\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*--------------------------*/
    /* Build command to overlay */
    /*--------------------------*/

    (void)strlcpy(overlay_command_tail,"",SSIZE);
    for(i=1; i<argc; ++i)
    {   if(i == 1)
           (void)strlcpy(overlay_command_name,argv[1],SSIZE);
        else
        {  (void)strlcat(overlay_command_tail,argv[i],SSIZE);

           if(i < argc - 1)
              (void)strlcat(overlay_command_tail," ",SSIZE);
        }
    }

    (void)psrp_overlay_server_process(FALSE,overlay_command_name,overlay_command_tail);


    /*---------------------------*/
    /* We should not get here if */
    /* overlay is successful     */
    /*---------------------------*/

    (void)fprintf(psrp_out,"\nfailed to overlay PSRP server (%s) [with command %s]\n\n",appl_name,overlay_command_name);
    (void)fflush(psrp_out);

    return(PSRP_OK);
}




/*------------------------------------------------*/
/* Overlay the current process with a new command */
/*------------------------------------------------*/

_PRIVATE int psrp_builtin_overfork_server_process(const int argc, const char *argv[])

{   int  i,
         ret;

    char overlay_command_name[SSIZE] = "",
         overlay_command_tail[SSIZE] = "";

    if(strcmp(argv[0],"overfork") != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc < 2)
    {  (void)fprintf(psrp_out,"\nusage: overfork <command to fork>\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*---------------------------*/
    /* Build command to overfork */
    /*---------------------------*/

    (void)strlcpy(overlay_command_tail,"",SSIZE);
    for(i=1; i<argc; ++i)
    {   if(i == 1)
           (void)strlcpy(overlay_command_name,argv[1],SSIZE);
        else
        {  (void)strlcat(overlay_command_tail,argv[i],SSIZE);

           if(i < argc - 1)
              (void)strlcat(overlay_command_tail," ",SSIZE);
        }
    }

    if(psrp_overlay_server_process(TRUE,overlay_command_name,overlay_command_tail) < 0)
       (void)fprintf(psrp_out,"\noverfork  PSRP server (%s) [with command %s] failed - resuming execution\n\n",
                                                                                 appl_name,overlay_command_name);
    else
       (void)fprintf(psrp_out,"\noverfork  PSRP server (%s) [with command %s] finished - resuming execution\n\n",
                                                                                   appl_name,overlay_command_name);
    (void)fflush(psrp_out);

    return(PSRP_OK);
}




/*-----------------------------------------------*/
/* Overlay server process with specified command */
/*-----------------------------------------------*/

_PUBLIC int psrp_overlay_server_process(const _BOOLEAN over_fork, const char *command_name, const char *command_tail)

{   char command_path[SSIZE]      = "",
         command_tail_path[SSIZE] = "";


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_overlay_server_process] attempt by non root thread to perform PUPS/P3 PSRP operation");


    if(command_name == (const char *)NULL || command_tail == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*------------------------------------------------------*/
    /* This command should not return (if it is successful) */
    /*------------------------------------------------------*/

    if(strccpy(command_path,pups_search_path("PATH",command_name)) != (char *)NULL)
    {  if(command_tail != (char *)NULL)
          (void)snprintf(command_tail_path,SSIZE,"%s %s",command_path,command_tail);
       else
          (void)strlcpy(command_tail_path,command_path,SSIZE);


       /*---------------------------------------------*/
       /* Make sure this operation is not interrupted */
       /* by PUPS signal handlers                     */
       /*---------------------------------------------*/

       (void)pups_malarm(0);
       (void)pupshold(PSRP_SIGS);

       if(over_fork == TRUE)
       {  int status;

          char channel_wait_in[SSIZE]  = "",
               channel_wait_out[SSIZE] = "";

          overforking = TRUE;
          if((overforked_child_pid = pups_fork(FALSE,FALSE)) == 0)
          {  (void)pups_execls(command_tail_path);

             if(appl_verbose == TRUE)
             {  (void)strdate(date);
                (void)fprintf(stderr,"%s %s (%d@%s:%s): failed to overfork command (%s) - resuming execution\n",
                                                      date,appl_name,appl_pid,appl_host,appl_owner,command_name);
                (void)fflush(stderr);
             }
             exit(255);
          }

          (void)snprintf(channel_wait_in,SSIZE,"%s.wait",channel_name_in);
          (void)rename(channel_name_in,channel_wait_in);
          (void)snprintf(channel_wait_out,SSIZE,"%s.wait",channel_name_out);
          (void)rename(channel_name_out,channel_wait_out);

          while(kill(overforked_child_pid,SIGALIVE) != (-1))
          {    (void)pups_usleep(100);
               (void)pupswaitpid(FALSE,overforked_child_pid,&status);
          }

          overforking          = FALSE;
          overforked_child_pid = (-1);

          (void)rename(channel_wait_in, channel_name_in);
          (void)rename(channel_wait_out,channel_name_out);

          if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@%s:%s): overforked command (%s) has terminated - resuming execution\n",
                                                          date,appl_name,appl_pid,appl_host,appl_owner,command_name);
             (void)fflush(stderr);
          }
       }
       else if(command_tail_path[0] != ' ')
       {  

          /*-----------------*/
          /* Reply to client */
          /*-----------------*/

          (void)fprintf(psrp_out,"EOT %s\n",psrp_c_code);
          (void)fflush(psrp_out);


          /*------------------------------------------------------*/
          /* Tell client that we have finished processing SIGPSRP */
          /*------------------------------------------------------*/

          psrp_endop("psrp");       


          /*----------------------------------------*/
          /* Close channel to existing PSRP command */
          /*----------------------------------------*/

          pups_exit(PUPS_DEFER_EXIT); 


          /*----------------------------------------*/
          /* Overlay new command on current process */
          /*----------------------------------------*/

          (void)pups_execls(command_tail_path);


          /*----------------------------------------------------------*/
          /* We should not get here -- if overlay has been successful */
          /*----------------------------------------------------------*/

          (void)pups_malarm(1);
          pupsrelse(PSRP_SIGS);
          pups_set_errno(ENOEXEC);
          return(-1);
       }
       else
       {  (void)pups_malarm(1);
          pupsrelse(PSRP_SIGS);

          pups_set_errno(ENOEXEC);
          return(-1);
       }

       (void)pups_malarm(1);
       pupsrelse(PSRP_SIGS);
    }

    pups_set_errno(OK);
    return(0);
}




/*-----------------------------*/
/* Set size of PUPS file table */
/*-----------------------------*/


_PRIVATE int psrp_builtin_extend_ftab(const int argc, const char *argv[])

{   int i,
        n_files;

    if(strcmp(argv[0],"ftab") != 0)
       return(PSRP_DISPATCH_ERROR);
   
    if(argc > 2)
    {  (void)fprintf(psrp_out,"\nusage: files [<maximum number of PUPS file table slots>]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(argc == 1)
    {  (void)fprintf(psrp_out,"\n%d PUPS file table slots available for PSRP server \"%s\"\n\n",
                                                                        appl_max_files,appl_name);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"shrink") != 0 && sscanf(argv[1],"%d",&n_files) != 1)
    {  (void)fprintf(psrp_out,"\nExpecting maximum number of files (in PUPS file table)\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&ftab_mutex);
    #endif /* PTHREAD_MUTEX */

    if(n_files < appl_max_files || strcmp(argv[1],"shrink") == 0)
    {

       /*---------------------------------------------*/
       /* Find the high water mark for the file table */
       /*---------------------------------------------*/

       for(i=appl_max_files-1; i>0; --i)
       {  if(ftab[i].fdes != (-1))
          {  int j;


             /*-----------------------------------------------------*/
             /* We have found the high water mark return all memory */
             /* above it to this processes free pool                */
             /*-----------------------------------------------------*/

             for(j=i; j<appl_max_files; ++j)
                (void)pups_clear_ftab_slot(TRUE,j);
             appl_max_files = i + 1;
             ftab = (ftab_type *)pups_realloc((void *)ftab,appl_max_files*sizeof(ftab_type));

             (void)fprintf(psrp_out,"\n%d PUPS file table slots now allocated for PSRP server \"%s\"\n\n",
                                                                                  appl_max_files,appl_name); 
             (void)fflush(psrp_out);

             #ifdef PTHREAD_SUPPORT
             (void)pthread_mutex_unlock(&ftab_mutex);
             #endif /* PTHREAD_MUTEX */

             return(PSRP_OK);
          }
       }

       (void)fprintf(psrp_out,"\n%d PUPS file table slots allocated for PSRP server (unchanged)\"%s\"\n\n",
                                                                                   appl_max_files,appl_name); 
       (void)fflush(psrp_out);


       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&ftab_mutex);
       #endif /* PTHREAD_MUTEX */

       return(PSRP_OK);
    }

    ftab           = (ftab_type *)pups_realloc((void *)ftab,n_files*sizeof(ftab_type));
    for(i=appl_max_files; i<n_files; ++i)
        (void)pups_clear_ftab_slot(FALSE,i);
    appl_max_files = n_files;


    (void)fprintf(psrp_out,"\n%d PUPS file table slots now allocated for PSRP server \"%s\"\n\n",
                                                                         appl_max_files,appl_name);
    (void)fflush(psrp_out); 


    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&ftab_mutex);
    #endif /* PTHREAD_MUTEX */

    return(PSRP_OK);
}




/*--------------------------------------*/
/* Set size of PUPS virtual timer table */
/*--------------------------------------*/

_PRIVATE int psrp_builtin_extend_vitab(const int argc, const char *argv[])

{   int i,
        n_vtimers;

    if(strcmp(argv[0],"vitab") != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc > 2)
    {  (void)fprintf(psrp_out,"\nusage: vitab [<maximum number of PUPS virtual timer table slots>]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(argc == 1)
    {  (void)fprintf(psrp_out,"\n%d PUPS virtual timer table slots available for PSRP server \"%s\"\n\n",
                                                                               appl_max_vtimers,appl_name);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"shrink") != 0 && sscanf(argv[1],"%d",&n_vtimers) != 1)
    {  (void)fprintf(psrp_out,"\nExpecting maximum number of timers (in PUPS virtual timer table)\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(n_vtimers < appl_max_vtimers || strcmp(argv[1],"shrink") == 0)
    {

       /*------------------------------------------------------*/
       /* Find the high water mark for the virtual timer table */
       /*------------------------------------------------------*/

       for(i=appl_max_vtimers-1; i>0; --i)
       {  if(vttab[i].interval_time != (-1))
          {  int j;


             /*-----------------------------------------------------*/
             /* We have found the high water mark return all memory */
             /* above it to this processes free pool                */
             /*-----------------------------------------------------*/

             for(j=i; j<appl_max_vtimers; ++j)
                (void)pups_clear_vitimer(TRUE,j);
             appl_max_vtimers = i + 1;
             vttab = (vttab_type *)pups_realloc((void *)vttab,appl_max_vtimers*sizeof(vttab_type));

             (void)fprintf(psrp_out,"\n%d PUPS virtual timer table slots now allocated for PSRP server \"%s\"\n\n",
                                                                                         appl_max_vtimers,appl_name);
             (void)fflush(psrp_out);

             return(PSRP_OK);
          }
       }

       (void)fprintf(psrp_out,"\n%d PUPS virtual timer table slots allocated for PSRP server (unchanged) \"%s\"\n\n",
                                                                                           appl_max_vtimers,appl_name);
       (void)fflush(psrp_out);
       return(PSRP_OK);
    }

    vttab = (vttab_type *)pups_realloc((void *)vttab,n_vtimers*sizeof(vttab_type));
    for(i=appl_max_vtimers; i<n_vtimers; ++i)
        (void)pups_clear_vitimer(FALSE,i);
    appl_max_vtimers = n_vtimers;

    (void)fprintf(psrp_out,"\n%d PUPS virtual timer table slots now allocated for PSRP server \"%s\"\n\n",
                                                                                appl_max_vtimers,appl_name);
    (void)fflush(psrp_out);
    return(PSRP_OK);
}




/*------------------------------*/
/* Set size of PUPS child table */
/*------------------------------*/

_PRIVATE int psrp_builtin_extend_chtab(const int argc, const char *argv[])

{   int i,
        n_children;

    if(strcmp(argv[0],"chtab") != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc > 2)
    {  (void)fprintf(psrp_out,"\nusage: chtab [<maximum number of PUPS child table slots>]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(argc == 1)
    {  (void)fprintf(psrp_out,"\n%d PUPS child table slots available for PSRP server \"%s\"\n\n",
                                                                         appl_max_child,appl_name);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"shrink") != 0 && sscanf(argv[1],"%d",&n_children) != 1)
    {  (void)fprintf(psrp_out,"\nExpecting maximum number of children (in PUPS child table)\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&chtab_mutex);
    #endif /* PTHREAD_SUPPORT */

    if(n_children < appl_max_child || strcmp(argv[1],"shrink") == 0)
    {

       /*---------------------------------------------*/
       /* Find the high water mark for the file table */
       /*---------------------------------------------*/

       for(i=appl_max_child-1; i>0; --i)
       {  if(chtab[i].pid != (-1))
          {  int j; 


             /*-----------------------------------------------------*/
             /* We have found the high water mark return all memory */
             /* above it to this processes free pool                */
             /*-----------------------------------------------------*/

             for(j=i; j<appl_max_child; ++j)
                (void)pups_clear_chtab_slot(TRUE,j);
             appl_max_child = i + 1;
             chtab = (chtab_type *)pups_realloc((void *)chtab,appl_max_child*sizeof(chtab_type));

             (void)fprintf(psrp_out,"\n%d PUPS child table slots now allocated for PSRP server \"%s\"\n\n",
                                                                                   appl_max_child,appl_name);
             (void)fflush(psrp_out);

             #ifdef PTHREAD_SUPPORT
             (void)pthread_mutex_unlock(&chtab_mutex);
             #endif /* PTHREAD_SUPPORT */

             return(PSRP_OK);
          }
       }

       (void)fprintf(psrp_out,"\n%d PUPS child table slots allocated for PSRP server (unchanged) \"%s\"\n\n",
                                                                                     appl_max_child,appl_name);
       (void)fflush(psrp_out);

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&chtab_mutex);
       #endif /* PTHREAD_SUPPORT */

       return(PSRP_OK);
    }

    chtab = (chtab_type *)pups_realloc((void *)chtab,n_children*sizeof(chtab_type));
    for(i=appl_max_files; i<n_children; ++i)
        (void)pups_clear_chtab_slot(FALSE,i);
    appl_max_child = n_children;

    (void)fprintf(psrp_out,"\n%d PUPS child table slots now allocated for PSRP server \"%s\"\n\n",
                                                                          appl_max_child,appl_name);
    (void)fflush(psrp_out);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&chtab_mutex);
    #endif /* PTHREAD_SUPPORT */

    return(PSRP_OK);
}




#ifdef PERSISTENT_HEAP_SUPPORT
/*-----------------------------------*/
/* Extend PUPS persistent heap table */
/*-----------------------------------*/

_PRIVATE int psrp_builtin_extend_htab(const int argc, const char *argv[])

{   int n_pheaps;

    if(strcmp(argv[1],"htab") != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc > 2)
    {  (void)fprintf(psrp_out,"\nusage: htab [<maximum number of PUPS persistent heap table slots>]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(argc == 1)
    {  (void)fprintf(psrp_out,"\n%d PUPS persistent heap table slots available for PSRP server \"%s\"\n\n",
                                                                                 appl_max_pheaps,appl_name);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(sscanf(argv[1],"%d",&n_pheaps) != 1)
    {  (void)fprintf(psrp_out,"\nExpecting maximum number of heaps (in PUPS persistent heap table)\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(n_pheaps < appl_max_pheaps)
    {  (void)fprintf(psrp_out,"\nCannot shrink PUPS persistent heap table\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    msm_extend(appl_max_pheaps,n_pheaps);
    appl_max_pheaps = n_pheaps;

    (void)fprintf(psrp_out,"\n%d PUPS persistent heap table slots now allocated for PSRP server \"%s\"\n\n",
                                                                                  appl_max_pheaps,appl_name);
    (void)fflush(psrp_out);

    return(PSRP_OK);
}
#endif /* PERSISTENT_HEAP_SUPPORT */




#ifdef DLL_SUPPORT
/*---------------------------------*/
/* Extend PUPS orifice [DLL] table */
/*---------------------------------*/

_PRIVATE int psrp_builtin_extend_ortab(const int argc, const char *argv[])

{   int i,
        n_orifices;

    if(strcmp(argv[0],"ortab") != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc > 2)
    {  (void)fprintf(psrp_out,"\nusage: ortab [<maximum number of PUPS orifice [DLL] table slots>]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(argc == 1)
    {  (void)fprintf(psrp_out,"\n%d PUPS orifice [DLL] table slots available for PSRP server \"%s\"\n\n",
                                                                              appl_max_orifices,appl_name);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"shrink") != 0 && sscanf(argv[1],"%d",&n_orifices) != 1)
    {  (void)fprintf(psrp_out,"\nExpecting maximum number of orifices (in PUPS orifice [DLL] table)\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(n_orifices < appl_max_orifices || strcmp(argv[1],"shrink") == 0)
    {

       /*---------------------------------------------*/
       /* Find the high water mark for the file table */
       /*---------------------------------------------*/

       for(i=appl_max_orifices-1; i>0; --i)
       {  if(ortab[i].orifice_handle == (void *)NULL)
          {  int j;


             /*-----------------------------------------------------*/
             /* We have found the high water mark return all memory */
             /* above it to this processes free pool                */
             /*-----------------------------------------------------*/

             for(j=i; j<appl_max_orifices; ++j)
                 (void)clear_ortab_slot(TRUE,j);
             appl_max_orifices = i + 1;
             ortab = (ortab_type *)pups_realloc((void *)ortab,appl_max_orifices*sizeof(ortab_type));

             (void)fprintf(psrp_out,"\n%d PUPS orifice [DLL] table slots now allocated for PSRP server \"%s\"\n\n",
                                                                                        appl_max_orifices,appl_name);
             (void)fflush(psrp_out);
             return(PSRP_OK);
          }
       }

       (void)fprintf(psrp_out,"\n%d PUPS orifice [DLL] table slots allocated for PSRP server (unchanged) \"%s\"\n\n",
                                                                                          appl_max_orifices,appl_name);
       (void)fflush(psrp_out);
       return(PSRP_OK);
    }

    ortab           = (ortab_type *)pups_realloc((void *)ortab,n_orifices*sizeof(ortab_type));
    for(i=appl_max_orifices; i<n_orifices; ++i)
        clear_ortab_slot(FALSE,i);
    appl_max_orifices = n_orifices;

    (void)fprintf(psrp_out,"\n%d PUPS orifice [DLL] slots now allocated for PSRP server \"%s\"\n\n",
                                                                         appl_max_orifices,appl_name);
    (void)fflush(psrp_out);
    return(PSRP_OK);
}
#endif /* DLL_SUPPORT */




/*-----------------------------------------------------*/
/* Check process circadian activity (crontab) schedule */
/*-----------------------------------------------------*/

_PRIVATE void psrp_crontab_checkschedule(char *args)

{   int i,
        local_s;

    time_t t,
           tdum;

    struct tm *local_time = (struct tm *)NULL;

    for(i=0; i<MAX_CRON_SLOTS; ++i)
    {  if(crontab[i].from != (-1) && crontab[i].to != (-1)) 
       {  t = time(&tdum);
          local_time = localtime(&t);


          /*---------------------*/
          /* Should we be awake? */
          /*---------------------*/

          if(crontab[i].overnight == TRUE)
             local_s = local_time->tm_hour*3600  + local_time->tm_min*60    + local_time->tm_sec;
          else
             local_s = local_time->tm_mday*86400 + local_time->tm_hour*3600 + local_time->tm_min*60 + local_time->tm_sec;

          if(crontab[i].from >= local_s && crontab[i].to < local_s)
          {  if(appl_verbose == TRUE)
             {  (void)strdate(date);
                (void)fprintf(stderr,"%s %s ($d@%s:%s): scheduling crontab event %d (%s)\n",
                        date,appl_name,appl_pid,appl_host,appl_owner,i,crontab[i].fname);
                (void)fflush(stderr);
             }


             /*----------------------------------------------------------*/
             /* If the entry in the crontab has no computational payload */
             /* simply sleep for the requisite period                    */
             /*----------------------------------------------------------*/

             if(crontab[i].func == (void *)NULL)
             {  do {   (void)pups_usleep(1000000);

                       t = time(&tdum);
                       local_time = localtime(&t);


                       /*---------------------*/
                       /* Should we be awake? */
                       /*---------------------*/

                       if(crontab[i].overnight == TRUE)
                          local_s = local_time->tm_hour*3600  + local_time->tm_min*60    + local_time->tm_sec;
                       else
                          local_s = local_time->tm_mday*86400 + local_time->tm_hour*3600 + local_time->tm_min*60 + local_time->tm_sec;
                   } while(crontab[i].from >= local_s && crontab[i].to < local_s);
             }
             else
             {  

                /*--------------------------------------------------*/
                /* Launch computational payload  - note we pass the */
                /* payload function its crontab slot so it can stop */
                /*--------------------------------------------------*/

                (*crontab[i].func)(i);              
             }

             if(appl_verbose == TRUE)
             {  (void)strdate(date);
                (void)fprintf(stderr,"%s %s ($d@%s:%s): crontab event %d (%s) finished\n",
                        date,appl_name,appl_pid,appl_host,appl_owner,i,crontab[i].fname);
                (void)fflush(stderr);
             }
          }
       }
    }
}




/*-----------------------------------------------*/
/* Enter a scheduled activity into crontab table */
/*-----------------------------------------------*/

_PUBLIC int psrp_crontab_schedule(const char *from, const char *to, const char *fname, const void *func)

{   int i,
        to_h,
        to_m,
        to_d,
        from_h,
        from_m,
        from_d,
        to_s,
        from_s,
        ctab_index;

    _BOOLEAN do_from_now = FALSE;

    time_t t,
           tdum;

    struct tm *local_time = (struct tm *)NULL;

    char sfrom[SSIZE] = "",
         sto[SSIZE]   = "";


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_crontab_schedule] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(from  == (const char *)NULL  ||
       to    == (const char *)NULL  ||
       fname == (const char *)NULL  ||
       func  == (const void *)NULL   )
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*------------------------------------------------*/
    /* Find a free slot in the crontab data structure */
    /*------------------------------------------------*/

    for(i=0; i<MAX_CRON_SLOTS; ++i)
       if(strcmp(crontab[i].fname,"notset") == 0)
       {  ctab_index = i;
          goto cronslot_found;
       }

    pups_set_errno(ENOMEM);
    return(-1);

cronslot_found:


    /*---------------------------------------------------------*/
    /* We have a free crontab slot -- add schedule information */
    /* to it.                                                  */
    /*---------------------------------------------------------*/

    for(i=0; i<strlen(from); ++i)
       if(from[i] == ':')
          sfrom[i] = ' ';
       else
          sfrom[i] = from[i];

    if(strcmp(sfrom,"now") == 0)
    {  t = time(&tdum);
       local_time = localtime(&t);

       from_d = local_time->tm_mday;
       from_h = local_time->tm_hour;
       from_m = local_time->tm_min;

       do_from_now = TRUE;
    }
    else if(sscanf(sfrom,"%d%d%d",&from_d,&from_h,&from_m) != 3)
    {  if(sscanf(sfrom,"%d%d",&from_h,&from_m) != 2)
       {  pups_set_errno(EINVAL);
          return(-1);
       }

       from_d = 0;
    }    

    if(from_h < 0 || from_h > 24 || from_m < 0 || from_m > 60)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    for(i=0; i<strlen(to); ++i)
       if(to[i] == ':')
          sto[i] = ' ';
       else
          sto[i] = to[i];

    if(strcmp(to,"forever") == 0)
       to_s = INT_MAX;
    else if(sscanf(sto,"%d%d%d",&to_d,&to_h,&to_m) != 3)
    {  if(sscanf(sto,"%d%d",&to_h,&to_m) != 2)
       {  pups_set_errno(EINVAL);
          return(-1);
       }

       if(do_from_now == TRUE)
          to_d = local_time->tm_mday;
       else
          to_d = 0;
    }

    if(to_h < 0 || to_h > 24 || to_m < 0 || to_m > 60)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if(from_d == 0 && to_d != 0 || from_d != 0 && to_d == 0)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if(from_h > to_h)
    {  t = time(&tdum);
       local_time = localtime(&t);
       from_d = local_time->tm_mday;
       to_d   = from_d + 1;

       crontab[ctab_index].overnight = TRUE;
    }
    else
       crontab[ctab_index].overnight = FALSE;

    from_s = from_d*86400 + from_h*3600 + from_m*60;
    to_s   = to_d* 86400  + to_h*3600   + to_m*60;


    /*------------------------------------------------------*/
    /* We can finally add the time data to the crontab slot */
    /*------------------------------------------------------*/

    crontab[ctab_index].from  = from_s;
    crontab[ctab_index].to    = to_s;
    crontab[ctab_index].func  = func;
    (void)strlcpy(crontab[ctab_index].fname,fname,SSIZE);
    (void)strlcpy(crontab[ctab_index].fromdate,from,SSIZE);
    (void)strlcpy(crontab[ctab_index].todate  ,to,SSIZE);

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       if(crontab[ctab_index].func  == (void *)NULL)
          (void)fprintf(stderr,"%s %s (%d@%s:%s): [crontab slot %d] inactivity scheduled between %s and %s\n\n",
                                                date,appl_name,appl_pid,appl_host,appl_owner,ctab_index,from,to);
       else
          (void)fprintf(stderr,"%s %s (%d@%s:%s): [crontab slot %d]  \"%-32s\" (at %016lx virtual) scheduled between %s and %s\n\n",
                                      date,appl_name,appl_pid,appl_host,appl_owner,ctab_index,fname,(unsigned long int)func,from,to);

       (void)fflush(stderr);
    }

    pups_set_errno(OK);
    return(0);
}




/*---------------------------*/
/* Unschedule a crontab slot */
/*---------------------------*/

_PUBLIC int psrp_crontab_unschedule(const unsigned int ctab_index)

{   int i;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_crontab_unschedule] mattempt by non root thread to perform PUPS/P3 PSRP operation");

    if(ctab_index >= MAX_CRON_SLOTS)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if(crontab[ctab_index].from != (-1) && crontab[ctab_index].to != (-1))
    {  crontab[ctab_index].to        = (-1);
       crontab[ctab_index].from      = (-1);
       crontab[ctab_index].overnight = FALSE;
       crontab[ctab_index].func      = (void *)NULL;

       (void)strlcpy(crontab[ctab_index].fname   ,"notset",SSIZE);
       (void)strlcpy(crontab[ctab_index].fromdate,"notset",SSIZE);
       (void)strlcpy(crontab[ctab_index].todate  ,"notset",SSIZE);

       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): crontab slot [%d] cleared\n",
                      date,appl_name,appl_pid,appl_host,appl_owner,ctab_index);
          (void)fflush(stderr);
       }

       pups_set_errno(OK);
       return(-1);
    }

    pups_set_errno(EINVAL);
    return(-1);
}




/*--------------------*/
/* Initialise crontab */
/*--------------------*/

_PUBLIC void psrp_crontab_init(void)

{   int i;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_crontab_init] attempt by non root thread to perform PUPS/P3 PSRP operation");


    /*-----------------------------------*/
    /* Initialise crontab data structure */
    /*-----------------------------------*/

    for(i=0; i<MAX_CRON_SLOTS; ++i)
    {  crontab[i].from      = (-1);
       crontab[i].to        = (-1);
       crontab[i].overnight = FALSE;
       crontab[i].func      = (void *)NULL;
 
       (void)strlcpy(crontab[i].fname,   "notset",SSIZE);
       (void)strlcpy(crontab[i].fromdate,"notset",SSIZE);
       (void)strlcpy(crontab[i].todate,  "notset",SSIZE);
    }


    /*-----------------------------------------------------------*/
    /* Add croncheck to the list of virtual interval timer tasks */
    /*-----------------------------------------------------------*/

    (void)pups_setvitimer("cron_homeostat",1,VT_CONTINUOUS,10,NULL,(void *)psrp_crontab_checkschedule);

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): PUPS/P3 cron service started\n",
                           date,appl_name,appl_pid,appl_host,appl_owner);
       (void)fflush(stderr);
    }

    pups_set_errno(OK);
}




/*--------------------------------------------------*/
/* Display schedule of activities for this function */
/*--------------------------------------------------*/

_PUBLIC int psrp_show_crontab(const FILE *stream)

{   int i,
        cront = 0;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_show_crontab] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(stream == (const FILE *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    (void)fprintf(stream,"\n    Crontab schedule\n");
    (void)fprintf(stream,"    ================\n\n");
    (void)fflush(stream);

    for(i=0; i<MAX_CRON_SLOTS; ++i)
    {  if(crontab[i].from > 0)
       {  if(crontab[i].func != (void *)NULL)
             (void)fprintf(stream,"    %04d: (payload \"%-32s\" at %016lx virtual) start: %-32s, stop %-32s\n",
                                                                                                             i,
                                                                                              crontab[i].fname,
                                                                            (unsigned long int)crontab[i].func,
                                                                                           crontab[i].fromdate,
                                                                                            crontab[i].todate);
          else
             (void)fprintf(stream,"    %04d: (inactivity) start: %-32s, stop %-32s\n",
                                                                                    i,
                                                                  crontab[i].fromdate,
                                                                    crontab[i].todate);

          (void)fflush(stream);

          ++cront;
       }
    }

    if(cront == 0)
       (void)fprintf(stream,"\n\n    No crontab tasks scheduled (%04d slots free)\n\n",MAX_CRON_SLOTS);
    else if(cront == 1)
       (void)fprintf(stream,"\n\n    %04d crontab task scheduled (%04d slots free)\n\n",1,MAX_CRON_SLOTS - 1);
    else
       (void)fprintf(stream,"    %04d crontab task scheduled (%04d slots free)\n\n",cront,MAX_CRON_SLOTS - cront);

    (void)fflush(stream);

    pups_set_errno(OK);
    return(0);
}




/*-------------------------------------*/
/* Add scheduling slot to PSRP crontab */
/*-------------------------------------*/

_PRIVATE int psrp_builtin_crontab_schedule(const int argc, const char *argv[])

{   if(strcmp("schedule",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc < 3)
    {  (void)fprintf(psrp_out,"\nusage: schedule !from! !to! [payload computation]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(argc > 3)
    {  int  slot_index;
       void *func = (void *)NULL;

       slot_index = psrp_find_action_slot_index(argv[3]);
       if(slot_index != (-1)                                                   &&
          psrp_object_list[slot_index].object_type == PSRP_DYNAMIC_FUNCTION    ||
          psrp_object_list[slot_index].object_type == PSRP_STATIC_FUNCTION      )
          psrp_crontab_schedule(argv[1],argv[2],argv[3],psrp_object_list[slot_index].object_handle);
       else
       {  (void)fprintf(psrp_out,"\nobject \"%s\" is not executable\n\n");
          (void)fflush(psrp_out);
       }
    }
    else
       psrp_crontab_schedule(argv[1],argv[2],"notset",(void *)NULL);

    return(PSRP_OK);
}




/*------------------------------------------*/
/* Remove scheduling slot from PSRP crontab */
/*------------------------------------------*/

_PRIVATE int psrp_builtin_crontab_unschedule(const int argc, const char *argv[])

{   int ctab_index;

    if(strcmp("unschedule",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc != 2)
    {  (void)fprintf(psrp_out,"\nusage: unschedule !crontab slot index!\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(sscanf(argv[1],"%d",&ctab_index) != 1)
    {  (void)fprintf(psrp_out,"\ncrontab index must be an integer (range 0 to %d)\n",MAX_CRON_SLOTS - 1);
       (void)fflush(psrp_out);
    }

    psrp_crontab_unschedule(ctab_index);
    return(PSRP_OK);
}




/*----------------------*/
/* Display PSRP crontab */
/*----------------------*/

_PRIVATE int psrp_builtin_show_crontab(const int argc, const char *argv[])

{   if(strcmp("cronstat",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc != 1)
    {  (void)fprintf(psrp_out,"\nusage: cronstat\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    psrp_show_crontab(psrp_out);
    return(PSRP_OK);
}



/*-------------------------------------------------*/
/* Set PSRP function condition (error/status) code */
/*-------------------------------------------------*/

_PUBLIC void psrp_set_c_code(const char *c_code)

{
    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_set_c_code] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(c_code == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return;
    }

    (void)strlcpy(psrp_c_code,c_code,SSIZE);

    pups_set_errno(OK);
}




/*--------------------------------------------------------*/
/* Toggle PSRP server background standard I/O autodetach  */
/*--------------------------------------------------------*/

_PRIVATE int psrp_builtin_set_nodetach(const int argc, const char *argv[])

{   if(strcmp("nodetach",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc == 1)
    {  if(appl_nodetach == TRUE)
          (void)fprintf(psrp_out,"\nstandard I/O (for PSRP server) attached when in background\n\n");
       else
          (void)fprintf(psrp_out,"\nstandard I/O (for PSRP server) detached when in background\n\n");
       (void)fflush(psrp_out);
       return(PSRP_OK);
    }

    if(strcmp(argv[1],"help") == 0 || strcmp(argv[1],"usage") == 0)
    {  (void)fprintf(psrp_out,"\n\nUsage  nodetach [help | usage] | [on | off]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"on") == 0)
    {  appl_nodetach = TRUE;

       (void)fprintf(psrp_out,"\nstandard I/O will remain attached when PSRP server is in background\n\n");
       (void)fflush(psrp_out);
    }
    else if(strcmp(argv[1],"off") == 0)
    {  appl_nodetach = FALSE;

       (void)fprintf(psrp_out,"\nstandard I/O will be detached when PSRP server is in background\n\n");
       (void)fflush(psrp_out);
    }
    else
    {  (void)fprintf(psrp_out,"\n\nUsage  nodetach [help | usage] | [on | off]\n\n");
       (void)fflush(psrp_out);
    }

    return(PSRP_OK);
}




/*---------------------------*/
/* Display process resources */
/*---------------------------*/

_PRIVATE int psrp_builtin_show_rusage(const int argc, const char *argv[])

{   struct rusage buf;
    struct rlimit rlim;

    if(strcmp("rusage",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc > 1)
    {  (void)fprintf(psrp_out,"\n\nUsage: rusage help | usage]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    (void)getrusage(RUSAGE_SELF,&buf);

    (void)fprintf(psrp_out,"\n\nCurrent resource usage (for PSRP server \"%s\")\n\n",appl_name);
    (void)fprintf(psrp_out,"User time                    : %9.4F minutes\n", (FTYPE)(buf.ru_utime.tv_sec)/60.0);
    (void)fprintf(psrp_out,"System time                  : %9.4F minutes\n", ((FTYPE)buf.ru_stime.tv_sec)/60.0);
    (void)fprintf(psrp_out,"Max resident size            : %9.4F Mbytes\n",  ((FTYPE)buf.ru_maxrss)/1000000.0);
    (void)fprintf(psrp_out,"Integral shared memory size  : %9.4F Mbytes\n",  ((FTYPE)buf.ru_ixrss)/1000000.0);
    (void)fprintf(psrp_out,"Integral unshared data size  : %9.4F Mbytes\n",  ((FTYPE)buf.ru_idrss)/1000000.0);
    (void)fprintf(psrp_out,"Integral unshared stack size : %9.4F Mbytes\n\n",((FTYPE)buf.ru_isrss)/1000000.0);
    (void)fflush(psrp_out);

    (void)fprintf(psrp_out,"\n\nCurrent resource limits (for PSRP server \"%s\")\n\n",appl_name);

    (void)getrlimit(RLIMIT_CPU,&rlim);
    if(rlim.rlim_cur == RLIM_INFINITY)
       (void)fprintf(psrp_out,"CPU time                     : unlimited\n");
    else
       (void)fprintf(psrp_out,"CPU time                     : %9.4F minutes\n",(FTYPE)(rlim.rlim_cur)/60.0);

    (void)getrlimit(RLIMIT_CORE,&rlim);
    if(rlim.rlim_cur == RLIM_INFINITY)
       (void)fprintf(psrp_out,"Core size                    : unlimited\n");
    else
       (void)fprintf(psrp_out,"Core size                    : %9.4F Mbytes\n",(FTYPE)(rlim.rlim_cur)/1000000.0);

    
    (void)getrlimit(RLIMIT_DATA,&rlim);
    if(rlim.rlim_cur == RLIM_INFINITY)
       (void)fprintf(psrp_out,"Data segment size            : unlimited\n");
    else
       (void)fprintf(psrp_out,"Data segment size            : %9.4F Mbytes\n",(FTYPE)(rlim.rlim_cur)/1000000.0);

    
    (void)getrlimit(RLIMIT_STACK,&rlim);
    if(rlim.rlim_cur == RLIM_INFINITY)
       (void)fprintf(psrp_out,"Stack size                   : unlimited\n");
    else
       (void)fprintf(psrp_out,"Stack size                   : %9.4F Mbytes\n",(FTYPE)(rlim.rlim_cur)/1000000.0);

    (void)getrlimit(RLIMIT_RSS,&rlim);
    if(rlim.rlim_cur == RLIM_INFINITY)
       (void)fprintf(psrp_out,"Resident set size            : unlimited\n");
    else
       (void)fprintf(psrp_out,"Resident set size            : %9.4F Mbytes\n",(FTYPE)(rlim.rlim_cur)/1000000.0);

    (void)getrlimit(RLIMIT_AS,&rlim);
    if(rlim.rlim_cur == RLIM_INFINITY)
       (void)fprintf(psrp_out,"Virtual memory size          : unlimited\n");
    else
       (void)fprintf(psrp_out,"Virtual memory size          : %9.4F Mbytes\n",(FTYPE)(rlim.rlim_cur)/1000000.0);
       
    (void)getrlimit(RLIMIT_FSIZE,&rlim);
    if(rlim.rlim_cur == RLIM_INFINITY)
       (void)fprintf(psrp_out,"Max file size                : unlimited\n");
    else
       (void)fprintf(psrp_out,"Max file size                : %9.4F Mbytes\n",(FTYPE)(rlim.rlim_cur)/1000000.0);
    
      
    (void)getrlimit(RLIMIT_NOFILE,&rlim);
    if(rlim.rlim_cur == RLIM_INFINITY)
       (void)fprintf(psrp_out,"Max simultaneously open files: %d\n\n",RLIMIT_NOFILE);
    else
       (void)fprintf(psrp_out,"Max simultaneously open files: %d\n\n",rlim.rlim_cur);
    
       
    return(PSRP_OK);
}




/*-----------------------------------*/
/* Dynamically set process resources */
/*-----------------------------------*/

_PRIVATE int psrp_builtin_set_rlimit(const int argc, const char *argv[])

{   int    cnt = 1;
    struct rlimit buf;

    if(strcmp("rset",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc == 1 || strcmp("help",argv[1]) == 0 || strcmp("usage",argv[1]) == 0)
    {  (void)fprintf(psrp_out,"\n\nUsage: rset  [help | usage]\n");
       (void)fprintf(psrp_out,"             cpu      [<time minutes>    | unlimited]\n");
       (void)fprintf(psrp_out,"             core     [<size Mbytes>     | unlimited]\n");
       (void)fprintf(psrp_out,"             stack    [<size Mbytes>     | unlimited]\n");
       (void)fprintf(psrp_out,"             data     [<size Mbytes>     | unlimited]\n");
       (void)fprintf(psrp_out,"             rss      [<size Mbytes>     | unlimited]\n");
       (void)fprintf(psrp_out,"             vmsize   [<size Mbytes>     | unlimited]\n");
       (void)fprintf(psrp_out,"             fsize    [<size Mbytes>     | unlimited]\n");
       (void)fprintf(psrp_out,"             nfiles   [<number of files> | unlimited]\n");
       (void)fprintf(psrp_out,"             nproc    [<max children>    | unlimited]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }
 

    /*------------------------------------------------*/ 
    /* Set maximum amoput of CPU this process can use */
    /*------------------------------------------------*/ 

    if(strcmp(argv[cnt],"cpu") == 0)
    {  int cpu_time;

       ++cnt;
       (void)getrlimit(RLIMIT_CORE,&buf);
       if(strcmp("unlimited",argv[cnt]) == 0)
       {  buf.rlim_cur = RLIM_INFINITY;
          (void)setrlimit(RLIMIT_CPU,&buf);

          if(buf.rlim_max == RLIM_INFINITY)
             (void)fprintf(psrp_out,"\n(maximum) CPU time is unlimited\n\n");
          else
             (void)fprintf(psrp_out,"\n(maximum) CPU time is set to hard limit (%d)\n",buf.rlim_max);
          (void)fflush(psrp_out);

          ++cnt;
       }
       else if(sscanf(argv[cnt],"%d",&cpu_time) != 1)
       {  (void)fprintf(psrp_out,"\nexpecting CPU time (in minutes)\n\n");
          (void)fflush(psrp_out);

          ++cnt;
       }
       else
       {  if(buf.rlim_max != RLIM_INFINITY && cpu_time > buf.rlim_max)
          {  (void)fprintf(psrp_out,"\nhard limit for CPU time is: %d\n",buf.rlim_max);
             (void)fflush(psrp_out);

             ++cnt;
          }
          else
          {  (void)fprintf(psrp_out,"\n(maximum) CPU time set to: %d minutes\n\n",cpu_time);
             (void)fflush(psrp_out);

             buf.rlim_cur = 60*cpu_time;
             if(setrlimit(RLIMIT_CPU,&buf) == (-1))
             {  (void)fprintf(psrp_out,"\nfailed to set maximum CPU time\n\n");
                (void)fflush(psrp_out);
             }
          }
          ++cnt;
       }
    }


    /*-----------------------------------------------------*/
    /* Set maximum size of core dump file for this process */
    /*-----------------------------------------------------*/

    if(strcmp(argv[cnt],"core") == 0)
    {  int core_size;

       ++cnt;
       (void)getrlimit(RLIMIT_CORE,&buf);
       if(strcmp("unlimited",argv[cnt]) == 0)
       {  buf.rlim_cur = buf.rlim_max;
          (void)setrlimit(RLIMIT_CORE,&buf);

          if(buf.rlim_max == RLIM_INFINITY)
             (void)fprintf(psrp_out,"\n(maximum) core dump size is unlimited\n\n");
          else
             (void)fprintf(psrp_out,"\n(maximum) core dump size is set to hard limit (%d Mbytes)\n\n",buf.rlim_max/1000000);
          (void)fflush(psrp_out);

          ++cnt;
       }
       else if(sscanf(argv[cnt],"%d",&core_size) != 1)
       {  (void)fprintf(psrp_out,"\nexpecting core dump size (in Mbytes)\n\n");
          (void)fflush(psrp_out);

          ++cnt;
       }
       else
       {  if(buf.rlim_max != RLIM_INFINITY && 1000000*core_size > buf.rlim_max)
          {  (void)fprintf(psrp_out,"\nhard limit for core dump size is: %d\n",buf.rlim_max/1000);
             (void)fflush(psrp_out);

             ++cnt;
          }
          else
          {  (void)fprintf(psrp_out,"\n(maximum) core dump size set to: %d Mbytes\n\n",core_size);
             (void)fflush(psrp_out);

             buf.rlim_cur = 1000000*core_size;
             if(setrlimit(RLIMIT_CORE,&buf) == (-1))
             {  (void)fprintf(psrp_out,"\nfailed to set maximum core dump size\n\n");
                (void)fflush(psrp_out);
             }

             ++cnt;
          }
       }
    }


    /*-----------------------------------------*/
    /* Set maximum stack size for this process */
    /*-----------------------------------------*/

    if(strcmp(argv[cnt],"stack") == 0)
    {  int stack_size;
      
       ++cnt; 
       (void)getrlimit(RLIMIT_STACK,&buf);
       if(strcmp("unlimited",argv[cnt]) == 0)
       {  buf.rlim_cur = buf.rlim_max;
          (void)setrlimit(RLIMIT_STACK,&buf); 
         
          if(buf.rlim_max == RLIM_INFINITY)
             (void)fprintf(psrp_out,"\n(maximum) stack size is unlimited\n\n");
          else 
             (void)fprintf(psrp_out,"\n(maximum) stack size is set to hard limit (%d Mbytes)\n\n",buf.rlim_max/1000000);
          (void)fflush(psrp_out);
       
          ++cnt;
       }  
       else if(sscanf(argv[cnt],"%d",&stack_size) != 1)
       {  (void)fprintf(psrp_out,"\nexpecting stack size (in Mbytes)\n\n");
          (void)fflush(psrp_out);
       
          ++cnt;
       }
       else
       {  if(buf.rlim_max != RLIM_INFINITY && 1000000*stack_size > buf.rlim_max)
          {  (void)fprintf(psrp_out,"\nhard limit for stack size is: %d Mbytes\n",buf.rlim_max/1000000);
             (void)fflush(psrp_out);

             ++cnt;
          }
          else
          {  (void)fprintf(psrp_out,"\n(maximum) stack size set to: %d Kbytes\n\n",stack_size);
             (void)fflush(psrp_out);

             buf.rlim_cur = 1000000*stack_size;
             if(setrlimit(RLIMIT_STACK,&buf) == (-1))
             {  (void)fprintf(psrp_out,"\nfailed to set maximum stack size\n\n");
                (void)fflush(psrp_out);
             }

             ++cnt;
          }
       }
    }


    /*------------------------------------------------*/
    /* Set maximum data segment size for this process */
    /*------------------------------------------------*/

    if(strcmp(argv[cnt],"data") == 0)
    {  int data_size;

       ++cnt;
       (void)getrlimit(RLIMIT_DATA,&buf);
       if(strcmp("unlimited",argv[cnt]) == 0)
       {  buf.rlim_cur = buf.rlim_max;
          (void)setrlimit(RLIMIT_DATA,&buf);

          if(buf.rlim_max == RLIM_INFINITY)
             (void)fprintf(psrp_out,"\n(maximum) data segement size set is unlimited\n\n");
          else
             (void)fprintf(psrp_out,"\n(maximum) data segement size set to hard limit (%d Mbytes)\n\n",buf.rlim_max/1000000);
          (void)fflush(psrp_out);

          ++cnt;
       }  
       else if(sscanf(argv[cnt],"%d",&data_size) != 1)
       {  (void)fprintf(psrp_out,"\nexpecting data segment size (in Mbytes)\n\n");
          (void)fflush(psrp_out);

          ++cnt;
       }
       else
       {  if(1000000*data_size > buf.rlim_max)
          {  (void)fprintf(psrp_out,"\nhard limit for data segment size is %d\n",buf.rlim_max/1000000);
             (void)fflush(psrp_out);

             ++cnt;
          }
          else
          {  (void)fprintf(psrp_out,"\n(maximum) data segment size set to %d Mbytes\n\n",data_size);
             (void)fflush(psrp_out);

             buf.rlim_cur = 1000000*data_size;
             if(setrlimit(RLIMIT_DATA,&buf) == (-1))
             {  (void)fprintf(psrp_out,"\nfailed to set maximum data segment size\n\n");
                (void)fflush(psrp_out);
             }
 
             ++cnt;
          }
       }
    }


    /*-------------------------------------------------*/
    /* Set mamximum resident set size for this process */
    /*-------------------------------------------------*/

    if(strcmp(argv[cnt],"rss") == 0)
    {  int rss_size;

       ++cnt;
       (void)getrlimit(RLIMIT_RSS,&buf);
       if(strcmp("unlimited",argv[cnt]) == 0)
       {  buf.rlim_cur = buf.rlim_max;
          (void)setrlimit(RLIMIT_RSS,&buf);

          if(buf.rlim_max == RLIM_INFINITY)
             (void)fprintf(stderr,"\n(maximum) resident set size is unlimited\n\n");
          else 
             (void)fprintf(psrp_out,"\n(maximum) resident set size is set to hard limit (%d Mbytes)\n\n",buf.rlim_max/1000000);
          (void)fflush(psrp_out);

          ++cnt;
       }  
       else if(sscanf(argv[cnt],"%d",&rss_size) != 1)
       {  (void)fprintf(psrp_out,"\nexpecting resident set size (in Mbytes)\n\n");
          (void)fflush(psrp_out);

          ++cnt;
       }
       else
       {  if(buf.rlim_max != RLIM_INFINITY && 1000000*rss_size > buf.rlim_max)
          {  (void)fprintf(psrp_out,"\nhard limit for resident set size is: %d\n",buf.rlim_max/1000000);
             (void)fflush(psrp_out);

             ++cnt;
          }
          else
          {  (void)fprintf(psrp_out,"\n(maximum) resident set size set to: %d Kbytes\n\n",rss_size);
             (void)fflush(psrp_out);

             buf.rlim_cur = 1000000*rss_size;
             if(setrlimit(RLIMIT_RSS,&buf) == (-1))
             {  (void)fprintf(psrp_out,"\nfailed to set maximum resident set size\n\n");
                (void)fflush(psrp_out);
             }

             ++cnt;
          }
       }
    }


    /*--------------------------------------------------------------*/
    /* Set maximum virutal memory (address space) which can be used */
    /* by this process                                              */
    /*--------------------------------------------------------------*/

    if(strcmp(argv[cnt],"vmsize") == 0)
    {  int vm_size;

       ++cnt;
       (void)getrlimit(RLIMIT_AS,&buf);
       if(strcmp("unlimited",argv[cnt]) == 0)
       {  buf.rlim_cur = buf.rlim_max;
          (void)setrlimit(RLIMIT_AS,&buf);

          if(buf.rlim_max == RLIM_INFINITY)
             (void)fprintf(stderr,"\n(maximum) virtual memory size is unlimited\n\n");
          else
             (void)fprintf(psrp_out,"\n(maximum) virtual memory size is set to hard limit (%04d Mbytes)\n\n",buf.rlim_max/1000000);
          (void)fflush(psrp_out);

          ++cnt;
       } 
       else if(sscanf(argv[cnt],"%d",&vm_size) != 1)
       {  (void)fprintf(psrp_out,"\nexpecting virtual memory size (in Mbytes)\n\n");
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }
       else
       {  if(buf.rlim_max != RLIM_INFINITY && 1000000*vm_size > buf.rlim_max)
          {  (void)fprintf(psrp_out,"\nhard limit for virtual memory size is: %04d Mbytes\n",buf.rlim_max/1000000);
             (void)fflush(psrp_out);

             ++cnt;
          }
          else
          {  (void)fprintf(psrp_out,"\n(maximum) virtual memory size set to: %04d Mbytes\n\n",vm_size);
             (void)fflush(psrp_out);

             buf.rlim_cur = 1000000*vm_size;
             if(setrlimit(RLIMIT_AS,&buf) == (-1))
             {  (void)fprintf(psrp_out,"\nfailed to set maximum virtual memory size\n\n");
                (void)fflush(psrp_out);
             }

             ++cnt;
          }
       }
    }


    /*------------------------------------------------*/
    /* Set maximum (write) file size for this process */
    /*------------------------------------------------*/

    if(strcmp(argv[cnt],"fsize") == 0)
    {  int file_size;

       ++cnt;
       (void)getrlimit(RLIMIT_FSIZE,&buf);
       if(strcmp("unlimited",argv[cnt]) == 0)
       {  buf.rlim_cur = buf.rlim_max;
          (void)setrlimit(RLIMIT_FSIZE,&buf);

          if(buf.rlim_max == RLIM_INFINITY)
             (void)fprintf(psrp_out,"\n(maximum) file size is unlimited\n\n");
          else
             (void)fprintf(psrp_out,"\n(maximum) file size is set to hard limit (%d Mbytes)\n\n",buf.rlim_max/1000000);
          (void)fflush(psrp_out);

          ++cnt;
       }  
       else if(sscanf(argv[cnt],"%d",&file_size) != 1)
       {  (void)fprintf(psrp_out,"\nexpecting file segment size (in Mbytes)\n\n");
          (void)fflush(psrp_out);

          ++cnt;
       }
       else
       {  if(buf.rlim_max != RLIM_INFINITY && 1000000*file_size > buf.rlim_max)
          {  (void)fprintf(psrp_out,"\nhard limit for maximum file size is: %d Mbytes\n",buf.rlim_max/1000000);
             (void)fflush(psrp_out);

             ++cnt;
          }
          else
          {  (void)fprintf(psrp_out,"\n(maximum) data segment size set to: %d Mbytes\n\n",file_size);
             (void)fflush(psrp_out);

             buf.rlim_cur = 10000000*file_size;
             (void)setrlimit(RLIMIT_FSIZE,&buf);

             ++cnt;
          }
       }
    }


    /*-----------------------------------------------------------------------------*/
    /* Set maximum number of files which this process can have open simultaneously */
    /*-----------------------------------------------------------------------------*/

    if(strcmp(argv[cnt],"nfiles") == 0)
    {  int n_files;

       if(strcmp("unlimited",argv[cnt]) == 0)
       {  buf.rlim_cur = buf.rlim_max;
          (void)setrlimit(RLIMIT_NOFILE,&buf);

          if(buf.rlim_max == RLIM_INFINITY)
             (void)fprintf(psrp_out,"\n(maximum) number of simultaneously open files is unlimited\n\n");
          else
             (void)fprintf(psrp_out,"\n(maximum) number of simultaneously open files is set to hard limit (%d)\n\n",buf.rlim_max);
          (void)fflush(psrp_out);

          ++cnt;
       }  
       else if(sscanf(argv[cnt],"%d",&n_files) != 1)
       {  (void)fprintf(psrp_out,"\nexpecting maximum number of simultaneously open files\n\n");
          (void)fflush(psrp_out);

          ++cnt;
       }
       else
       {  if(buf.rlim_max != RLIM_INFINITY && n_files > buf.rlim_max)
          {  (void)fprintf(psrp_out,"\nhard limit for maximum number of files simultaneously open is: %d\n",buf.rlim_max);
             (void)fflush(psrp_out);

             ++cnt;
          }
          else
          {  (void)fprintf(psrp_out,"\nnumber of files simultaneously open is set to: %d\n\n",n_files);
             (void)fflush(psrp_out);

             buf.rlim_cur = n_files;
             (void)setrlimit(RLIMIT_NOFILE,&buf);

             ++cnt;
          }
       }
    }


    /*--------------------------------------------------------*/
    /* Set maximum number of children this process can create */
    /*--------------------------------------------------------*/

    if(strcmp(argv[cnt],"nproc") == 0)
    {  int n_children;

       if(strcmp("unlimited",argv[cnt]) == 0)
       {  buf.rlim_cur = buf.rlim_max;
          (void)setrlimit(RLIMIT_NPROC,&buf);

          if(buf.rlim_max == RLIM_INFINITY)
             (void)fprintf(psrp_out,"\n(maximum) number of children is unlimited\n\n");
          else
             (void)fprintf(psrp_out,"\n(maximum) number of children is set to hard limit (%d)\n\n",buf.rlim_max);
          (void)fflush(psrp_out);

          ++cnt;
       }
       else if(sscanf(argv[cnt],"%d",&n_children) != 1)
       {  (void)fprintf(psrp_out,"\nexpecting maximum number of children\n\n");
          (void)fflush(psrp_out);

          ++cnt;
       }
       else
       {  if(buf.rlim_max != RLIM_INFINITY && n_children > buf.rlim_max)
          {  (void)fprintf(psrp_out,"\nhard limit for maximum number of files simultaneously open is: %d\n",buf.rlim_max);
             (void)fflush(psrp_out);

             ++cnt;
          }
          else
          {  (void)fprintf(psrp_out,"\nnumber of files simultaneously open is set to: %d\n\n",n_children);
             (void)fflush(psrp_out);

             buf.rlim_cur = n_children;
             (void)setrlimit(RLIMIT_NPROC,&buf);

             ++cnt;
          }
       }
    }

    if(cnt == 0)
    {  (void)fprintf(psrp_out,"\nsyntax error\n\n");
       (void)fflush(psrp_out);
    }

    return(PSRP_OK);
}



/*----------------------------------------------------------*/
/* Dynamically set process exit (if effective parent exits) */
/*----------------------------------------------------------*/

_PRIVATE int psrp_builtin_set_pexit(const int argc, const char *argv[])

{   if(strcmp("pexit",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(strcmp(argv[1],"help") == 0 || strcmp(argv[1],"usage") == 0)
    {  (void)fprintf(psrp_out,"\n\nUsage  pexit [help | usage]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): PSRP server will exit if its effective parent is terminated\n)",
                                                                  date,appl_name,appl_pid,appl_host,appl_owner);
       (void)fflush(stderr);
    }

    (void)fprintf(psrp_out,"\nPSRP server will exit if its effective parent is terminated\n\n");
    (void)fflush(psrp_out);

    appl_ppid_exit = TRUE;
    (void)pups_setvitimer("default_parent_hoemostat",1,VT_CONTINUOUS,1,NULL,(void *)&pups_default_parent_homeostat);

    return(PSRP_OK);
}




/*------------------------------------------------------------*/
/* Dynamically unset process exit (if effective parent exits) */
/*------------------------------------------------------------*/

_PRIVATE int psrp_builtin_set_unpexit(const int argc, const char *argv[])

{   if(strcmp("unpexit",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(strcmp(argv[1],"help") == 0 || strcmp(argv[1],"usage") == 0)
    {  (void)fprintf(psrp_out,"\n\nUsage  pexit [help | usage]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): PSRP server will not exit if its effecive parent is terminated\n)",
                                                                     date,appl_name,appl_pid,appl_host,appl_owner);
       (void)fflush(stderr);
    }

    (void)fprintf(psrp_out,"\nPSRP server will not exit if its effecive parent is terminated\n\n");
    (void)fflush(psrp_out);

    appl_ppid_exit = FALSE;
    (void)pups_clearvitimer("default_parent_hoemostat");
}            




/*-----------------------------------------------*/
/* Dynamically set PSRP server migration context */
/*-----------------------------------------------*/

_PRIVATE int psrp_builtin_set_rooted(const int argc, const char *argv[])

{   if(strcmp("rooted",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(strcmp(argv[1],"help") == 0 || strcmp(argv[1],"usage") == 0)
    {  (void)fprintf(psrp_out,"\n\nUsage  rooted [help | usage]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): PSRP server is rooted (cannot migrate\n)",
                                            date,appl_name,appl_pid,appl_host,appl_owner);
       (void)fflush(stderr);
    }

    (void)fprintf(psrp_out,"\nPSRP server is rooted (cannot migrate)\n\n");
    (void)fflush(psrp_out);
                 
    appl_rooted = TRUE;
}




/*--------------------------------------------------------*/
/* Dynamically unset PSRP server system migration context */
/*--------------------------------------------------------*/

_PRIVATE int psrp_builtin_set_unrooted(const int argc, const char *argv[])

{   if(strcmp("unrooted",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(strcmp(argv[1],"help") == 0 || strcmp(argv[1],"usage") == 0)
    {  (void)fprintf(psrp_out,"\n\nUsage  unrooted [help | usage]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    appl_rooted = FALSE;

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): PSRP server is not rooted (can migrate\n)",
                                             date,appl_name,appl_pid,appl_host,appl_owner);
       (void)fflush(stderr);
    }

    (void)fprintf(psrp_out,"\nPSRP server is not rooted (can migrate)\n\n");
    (void)fflush(psrp_out);

    return(PSRP_OK);
}
                   



/*------------------------------------------------*/
/* Dynamically set PSRP server (effective) parent */
/*------------------------------------------------*/

_PRIVATE int psrp_builtin_set_parent(const int argc, const char *argv[])

{   int  i_tmp;
    char pname[SSIZE] = "";

    if(strcmp("parent",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc == 1)
    {  if(psrp_pid_to_pname(appl_ppid,pname) == TRUE)
       {  (void)fprintf(psrp_out,"\neffective parent is %d (%s@%s)\n\n",appl_ppid,pname,appl_host);
          (void)fflush(psrp_out);
       }
       else
       {  (void)fprintf(psrp_out,"\neffective parent \"%s\" does not exist\n\n",pname);
          (void)fflush(psrp_out);
       }

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"help") == 0 || strcmp(argv[1],"usage") == 0)
    {  (void)fprintf(psrp_out,"\n\nUsage  parent [help | usage] [<effective parent name | effective parent PID>> [exit]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(sscanf(argv[1],"%d",&i_tmp) != 1)
    {  (void)strlcpy(pname,argv[1],SSIZE);

       if((i_tmp = psrp_pname_to_pid(pname)) < 0)
       {  if(i_tmp == PSRP_DUPLICATE_PROCESS_NAME)
             (void)fprintf(psrp_out,"\nambiguous parent process name specified (\"%s\")\n\n",pname);
          else
             (void)fprintf(psrp_out,"\neffective parent \"%s\" does not exist\n\n",pname);
          (void)fflush(psrp_out);
          return(PSRP_OK);
       }
    }
    else
    {  if(psrp_pid_to_pname(i_tmp,pname) == FALSE)
       {  (void)fprintf(psrp_out,"\neffective parent \"%s\" does not exist\n\n",pname);
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }
    }

    appl_ppid = i_tmp;

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): effective parent is %d (%s@%s)\n",
               date,appl_name,appl_pid,appl_host,appl_owner,appl_ppid,pname,appl_host);
       (void)fflush(stderr);
    }             

    (void)fprintf(psrp_out,"\neffective parent is now %d (%s@%s)\n",appl_ppid,pname,appl_host);
    (void)fflush(psrp_out);

    if(argc == 3)
    {  if(strcmp(argv[2],"exit") == 0)
       {  if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@%s:%s): PSRP server will exit if its effective parent is terminated\n)",
                                                                        date,appl_name,appl_pid,appl_host,appl_owner);
             (void)fflush(stderr);
           }

           (void)fprintf(psrp_out,"\nPSRP server will exit if its effective parent is terminated\n\n");
           (void)fflush(psrp_out);

           appl_ppid_exit = TRUE;
           (void)pups_setvitimer("default_parent_hoemostat",1,VT_CONTINUOUS,1,NULL,(void *)&pups_default_parent_homeostat);
        }
    }
    else
    {  (void)fprintf(psrp_out,"\n");
       (void)fflush(psrp_out);
    }
          
    return(PSRP_OK);
}




/*-------------------------------------------------------*/
/* Dynamically set PSRP server current working directory */
/*-------------------------------------------------------*/

_PRIVATE int psrp_builtin_set_cwd(const int argc, const char *argv[])

{   char cwd[SSIZE]     = "",
         cwdpath[SSIZE] = "",
         relpath[SSIZE] = "";

    _BOOLEAN default_cwd = FALSE;

    if(strcmp("cwd",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc == 1)
    {  (void)fprintf(psrp_out,"\ncurrent working directory is \"%s\"\n\n",appl_cwd);
       (void)fflush(psrp_out);
    }
 
    if(strcmp(argv[1],"help") == 0 || strcmp(argv[1],"usage") == 0)
    {  (void)fprintf(psrp_out,"\n\nUsage  cwd [help | usage] [<current working directory:%s> | default]\n\n",appl_cwd);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*-------------------------------------------*/
    /* Get the name of the new working directory */
    /*-------------------------------------------*/

    (void)strlcpy(cwd,argv[1],SSIZE);

    if(strcmp(cwd,"default") == 0)
    {  (void)getcwd(appl_cwd,SSIZE);
       default_cwd = TRUE;
    }

    /*----------------------------------*/
    /* Check to see if path is absolute */
    /*----------------------------------*/

    else if(cwd[0] == '/')
       (void)strlcpy(cwdpath,cwd,SSIZE);
    else if(cwd[0] == '.')
    {  if(cwd[1] == '.')
       {  (void)fprintf(psrp_out,"std_init: cannot have \"..\" in cwd path");
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }

       (void)getcwd(relpath,SSIZE);
       (void)snprintf(cwdpath,SSIZE,"%s/%s",relpath,&cwd[2]);
    }
    else
    {  (void)getcwd(relpath,SSIZE);
       (void)snprintf(cwdpath,SSIZE,"%s/%s",relpath,cwd);
    }

    if(chdir(cwdpath) != (-1))
    {  (void)strlcpy(appl_cwd,cwdpath,SSIZE);

       if(default_cwd == TRUE)
          (void)fprintf(psrp_out,"\nset current working directory to \"%s\" [default]\n\n",appl_cwd);
       else
          (void)fprintf(psrp_out,"\nset current working directory to \"%s\"\n\n",appl_cwd);
       (void)fflush(psrp_out);
    }
    else
    {  (void)fprintf(psrp_out,"\ncurrent working directory is \"%s\" (could not change it to \"%s\")\n\n",appl_cwd,cwdpath);
       (void)fflush(psrp_out);
    }

    return(PSRP_OK);
}


    
/*-------------------------------------------------------------*/
/* Dynamically set number of times psrp function is (re)-tryed */ 
/*-------------------------------------------------------------*/

_PRIVATE int psrp_builtin_set_trys(const int argc, const char *argv[])

{   int i;

    if(strcmp("strys",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(strcmp(argv[2],"help") == 0 || strcmp(argv[2],"usage") == 0)
    {  (void)fprintf(psrp_out,"\n\nUsage  strys [help | usage] [<trys:%d>]\n\n",MAX_TRYS);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(argc == 1)
    {  (void)fprintf(psrp_out,"\nPSRP server process \"%s\" will currently retry an operation %d times (before aborting it)\n\n",appl_name,max_trys);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }
    else if(argc == 2)
    {  int itmp;

       if(sscanf(argv[1],"%d",&itmp) != 1 || itmp < 0)
       {  (void)fprintf(psrp_out,"\n\nnumber of tries must be a positive integer\n\n");
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }

       max_trys = itmp;
    }
    else
    {  (void)fprintf(psrp_out,"\n\nUsage  strys [help | usage] [<trys:%d>]\n\n",MAX_TRYS);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }        

    (void)fprintf(psrp_out,"\nPSRP server process \"%s\" will now retry an operation %d times (before aborting it)\n\n",appl_name,max_trys);
    (void)fflush(psrp_out);

    return(PSRP_OK);
}




/*---------------------------------------------------------------*/
/* Dynamically protect a file or files) from accidental deletion */
/*---------------------------------------------------------------*/

_PRIVATE int psrp_builtin_file_live(const int argc, const char *argv[])

{   int i,
        f_index,
        slot_index;

    if(strcmp("live",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc == 1 || strcmp(argv[2],"help") == 0 || strcmp(argv[2],"usage") == 0)
    {  (void)fprintf(psrp_out,"\n\nUsage live [help | usage] [<filename | slotindex> ...]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*---------------------------------------------------------------*/
    /* Set up homeostats for all files in list which are not already */
    /* homeostatically protected                                     */
    /*---------------------------------------------------------------*/

    for(i=1; i<argc; ++i)
    {

       /*-------------------------------*/
       /* Do we have a slot identifier? */
       /*-------------------------------*/

       if(sscanf(argv[i],"%d",&slot_index) == 1)
       {  if(slot_index < 0 || slot_index > appl_max_files)
          {  (void)fprintf(stderr,"\nindex %d is not avalid slot id (must lie between 0 and %d)\n",
                                                                         slot_index,appl_max_files);
             (void)fflush(stderr);
          }
       }
       else
       { 

          /*-----------------------------------------------------------*/
          /* We have a file name -- get corresponding file table index */
          /*-----------------------------------------------------------*/

          if(access(argv[i],F_OK | R_OK | W_OK) == (-1))
          {  (void)fprintf(stderr,"\nfile \"%s\" does not exist\n",argv[i]);
             f_index = (-1);
          }
          else if((f_index = pups_get_ftab_index_by_name(argv[i])) == (-1))
             (void)fprintf(stderr,"\nfile \"%s\" is not open\n",argv[i]);
          (void)fflush(stderr);
       }


       /*--------------------------------------------------*/   
       /* If we have a non zero ftab index start homeostat */
       /*--------------------------------------------------*/   

       if(f_index != (-1))
       {  char homeostat_name[SSIZE] = "";



          /*-------------------*/
          /* Homeostat running */
          /*-------------------*/

          if(ftab[f_index].homeostatic > 1)
             (void)fprintf(psrp_out,"file \"%s\" is already homeostatically protected\n",ftab[f_index].fname);


          /*-----------------------*/
          /* Homeostat not running */
          /*-----------------------*/

          else      
          {  (void)pups_fd_alive(ftab[f_index].fdes,homeostat_name,&pups_default_fd_homeostat);

             (void)fprintf   (psrp_out,"file \"%s\" is now hoemostatically protected (default_file_homeostat)\n",ftab[f_index].fname);
             (void)fflush    (psrp_out);
          }
       }
    }

    return(PSRP_OK);
}





/*-----------------------------------------------------------------*/
/* Dynamically unprotect a file or files) from accidental deletion */ 
/*-----------------------------------------------------------------*/

_PRIVATE int psrp_builtin_file_dead(const int argc, const char *argv[])

{   int i,
        f_index;

    if(strcmp("dead",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc == 1 || strcmp(argv[2],"help") == 0 || strcmp(argv[2],"usage") == 0)
    {  (void)fprintf(psrp_out,"\n\nUsage dead [help | usage] [<filename | slotindex> ...]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*-------------------------*/
    /* Unprotect files in list */
    /*-------------------------*/

    for(i=1; i<argc; ++i)
    {

       /*-------------------------------*/
       /* Do we have a slot identifier? */
       /*-------------------------------*/

       if(sscanf(argv[i],"%d",&f_index) == 1)
       {  if(f_index < 0 || f_index > appl_max_files)
          {  (void)fprintf(stderr,"\nindex %d is not a valid slot identifier (must lie between 0 and %d)\n",
                                                                                     f_index,appl_max_files);
             (void)fflush(stderr);
          }
       }
       else
       {  

          /*-----------------------------------------------------------*/
          /* We have a file name -- get corresponding file table index */
          /*-----------------------------------------------------------*/

          if(access(argv[i],F_OK | R_OK | W_OK) == (-1))
          {  (void)fprintf(stderr,"\nfile \"%s\" does not exist\n",argv[i]);
             f_index = (-1);
          }
          else if((f_index = pups_get_ftab_index_by_name(argv[i])) == (-1))
             (void)fprintf(stderr,"\nfile \"%s\" is not currently open\n",argv[i]);
          (void)fflush(stderr);
       }


       /*-------------------------------------------------*/
       /* If we have a non zero ftab index stop homeostat */
       /*-------------------------------------------------*/

       if(f_index != (-1))
       {  char homeostat_name[SSIZE] = "";

          if(ftab[f_index].homeostatic > 1)
             (void)fprintf(psrp_out,"file \"%s\" is already unprotected\n");
          else
          {  (void)snprintf(homeostat_name,SSIZE,"%s:default_file_homeostat",ftab[f_index].fname,argv[1]);
             (void)pups_fd_dead(ftab[f_index].fdes);

             (void)fprintf(psrp_out,"file \"%s\" is now unprotected\n",argv[i]);
             (void)fflush(psrp_out);
          }
       }
    }

    return(PSRP_OK);
}




/*----------------------------------------------------------------------------------------*/
/* Fork a PUPS/PSRP server -- this routine will build -- all open files etc are inherited */
/* along with any homeostats which the parent may have been running                       */
/*----------------------------------------------------------------------------------------*/

_PUBLIC int psrp_fork(const char     *childname,   // Name of forked child
                      const _BOOLEAN obituary)     // Produce report (of what went wrong) if TRUE

{   int  i,
         ret,
         cnt = 0;

    sigset_t set;

    char new_channel_name_in[SSIZE]  = "",
         new_channel_name_out[SSIZE] = "";


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("attempt by non root thread to perform PUPS/P3 PSRP operation (psrp_fork)");

    if(childname == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if(in_chan_handler == TRUE || in_psrp_handler == TRUE)
       return(-1);


    /*--------------------------------------------------------------------------*/
    /* Signals are not inherited -- so if we have signals pending simply return */
    /*--------------------------------------------------------------------------*/

    (void)sigemptyset(&set);
    (void)sigpending(&set);

    for(i=1; i<MAX_SIGS; ++i)
    {  if(sigismember(&set,i))
       {

          #ifdef PSRPLIB_DEBUG
          (void)fprintf(stderr,"PSRPLIB SIGNALS PENDING\n");
          (void)fflush(stderr);
          #endif /* PSRPLIB_DEBUG */

          return(0);
       }       
    }


    /*--------------------------------------------*/
    /* First fork() to creat a child process.     */
    /* Non-zero return code indicates that we are */
    /* the parent process                         */
    /*--------------------------------------------*/

    if((ret = pups_fork(FALSE,obituary)) != 0)
       return(ret);


    /*-------------------------------------------------------*/
    /* We are the child. We need to restart our homeostats   */
    /* (pending signals are not inherited) and build ourself */
    /* a communications channel for PSRP interactions        */
    /*-------------------------------------------------------*/


    /*------------------------------------------------*/
    /* Dissassociate child from parents PSRP channels */
    /* they are not inherited                         */
    /*------------------------------------------------*/

    for(i=3; i<appl_max_files; ++i)
    {  if(ftab[i].fdes != (-1) && strncmp(ftab[i].fname,"psrp",4) == 0)
       {  (void)fclose(ftab[i].stream);

           if(cnt ++ > 2)
             goto done;
       }
    }

done:

    (void)strlcpy(appl_name,childname,SSIZE);

    /*-----------------------------------------------------*/
    /* Get (child) PID and update PUPS process information */
    /*-----------------------------------------------------*/

    appl_pid = getpid();


    /*---------------------------------------*/
    /* Create standard I/O for this process. */
    /*---------------------------------------*/

    (void)pups_free((void *)ftab[0].fname);
    ftab[0].fname = (char *)pups_malloc(SSIZE);
    if(isatty(0) == 1)
    {  ftab[0].named = TRUE;
       (void)snprintf(ftab[0].fname,SSIZE,"%s/pups.%s.%s.stdin.pst.%d:%d", appl_fifo_dir,appl_name,appl_host,appl_pid,appl_uid);
       (void)symlink("/dev/tty",ftab[0].fname); 
    }
    else
    {  ftab[0].named = FALSE;
       (void)snprintf(ftab[0].fname,SSIZE,"<redirected.0>");
    }

    (void)pups_free((void *)ftab[1].fname);
    ftab[1].fname = (char *)pups_malloc(SSIZE);
    if(isatty(1) == 1)
    {  ftab[1].named = TRUE;
       (void)snprintf(ftab[1].fname,SSIZE,"%s/pups.%s.%s.stdout.pst.%d:%d",appl_fifo_dir,appl_name,appl_host,appl_pid,appl_uid);
       (void)symlink("/dev/tty",ftab[1].fname); 
    }
    else
    {  ftab[1].named = FALSE;
       (void)snprintf(ftab[1].fname,SSIZE,"<redirected.1>");
    }

    (void)pups_free((void *)ftab[2].fname);
    ftab[2].fname = (char *)pups_malloc(SSIZE);
    if(isatty(2) == 1)
    {  ftab[2].named = TRUE;
       (void)snprintf(ftab[2].fname,SSIZE,"%s/pups.%s.%s.stderr.pst.%d:%d",appl_fifo_dir,appl_name,appl_host,appl_pid,appl_uid);
       (void)symlink("/dev/tty",ftab[2].fname); 
    }
    else
    {  ftab[2].named = FALSE;
       (void)snprintf(ftab[2].fname,SSIZE,"<redirected.2>");
    }
                                      

    /*-------------------------------------------------*/
    /* Create communication channels for this process. */
    /*-------------------------------------------------*/

    (void)snprintf(channel_name_out,SSIZE,"%s/psrp#%s#fifo#out#%d#%d",appl_fifo_dir,appl_ch_name,appl_pid,appl_uid);
    (void)snprintf(channel_name_in,SSIZE, "%s/psrp#%s#fifo#in#%d#%d", appl_fifo_dir,appl_ch_name,appl_pid,appl_uid);
    (void)mkfifo(channel_name_in, 0600);
    (void)mkfifo(channel_name_out,0600);


    /*---------------------------------------------------------------*/
    /* Make sure that we delete the communications channel when this */
    /* process exits.                                                */
    /*---------------------------------------------------------------*/

    if(appl_verbose == TRUE)
    {  strdate(date);
       (void)fprintf(stderr,
               "%s %s (%d@%s:%s): PSRP (protocol %5.2F) handler initialised on channel %s/psrp:%s:%s:fifo:IO:%d\n",
                                                                                                              date,
                                                                                                         appl_name,
                                                                                                          appl_pid,
                                                                                                         appl_host,
                                                                                                        appl_owner,
                                                                                             PSRP_PROTOCOL_VERSION,
                                                                                                     appl_fifo_dir,
                                                                                                         appl_name,
                                                                                                         appl_host,
                                                                                                          appl_pid);
       (void)fflush(stderr);
    }

    if(psrp_bind_status & PSRP_HOMEOSTATIC_STREAMS)
    {  if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): Streams: PSRP streams are homeostatic\n",
                                              date,appl_name,appl_pid,appl_host,appl_owner);
       }

       (void)pups_setvitimer("psrp_homeostat",1,VT_CONTINUOUS,1,NULL,(void *)psrp_homeostat);
    }    

    (void)pups_vitrestart();

    pups_set_errno(OK);
    return(ret);
}




/*-------------------------------------------------------------------------*/
/* Create a trailfile and associated delayed destructor processs (lyosome) */
/*-------------------------------------------------------------------------*/

_PRIVATE void psrp_create_trailfile(char *trail_file_name,
                                    char *pen,
                                    char *host,
                                    char *host_port)

{   char lyosome_command[SSIZE] = "";
    FILE *stream = (FILE *)NULL;


    /*----------------------*/
    /* Create the trailfile */
    /*----------------------*/

    if(access(trail_file_name,F_OK | R_OK | W_OK) == (-1))
       (void)pups_creat(trail_file_name,0666);


    /*-------------------------------------------*/
    /* Write migration information to trail file */
    /*-------------------------------------------*/

    stream = fopen(trail_file_name,"w");

    (void)fprintf(stream,"#--------------------------------------------------------------------\n");
    (void)fprintf(stream,"#    PSRP migration trail file version 1.02\n");
    (void)fprintf(stream,"#    (C) M.A. O'Neill, Tumbling Dice 2022\n");
    (void)fprintf(stream,"#---------------------------------------------------------------------\n\n");
    (void)fflush(stream);
    (void)fprintf(stream,"%s %s %s\n\n",pen,host,host_port);
    (void)fclose(stream);


    /*-------------------------------------------------------------*/
    /* Start lyosome process which destroys trail file after delay */
    /*-------------------------------------------------------------*/

    (void)snprintf(lyosome_command,SSIZE,"lyosome -lifetime 15 %s&",trail_file_name);
    (void)system(lyosome_command);
}




/*------------------*/
/* Read a trailfile */
/*------------------*/

_PUBLIC _BOOLEAN psrp_read_trailfile(const char *trail_file_name, // Name of trailfile
                                     char       *pen,             // Name of process on new host
                                     char       *host,            // Name of new host
                                     char       *host_port)       // Port for (ssh) access to new host

{   char line[SSIZE] = "";
    FILE *stream = (FILE *)NULL;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_read_trailfile] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(trail_file_name == (const char *)NULL  ||
       pen             == (const char *)NULL  ||
       host            == (const char *)NULL   )
    {  pups_set_errno(EINVAL);
       return(-1);
    }


#ifdef PSRPLIB_DEBUG
    while(access(trail_file_name,F_OK | R_OK | W_OK) == (-1))
    {    (void)fprintf(stderr,"PSRPLIB TFW: %s\n",trail_file_name);
         (void)fflush(stderr);

         (void)pups_sleep(1);
    }
#endif /* PSRPLIB_DEBUG */


    /*--------------------------------------------*/
    /* Read migration information from trail file */
    /*--------------------------------------------*/

    if(access(trail_file_name,F_OK | R_OK | W_OK) == (-1))
    {  (void)pups_set_errno(EEXIST);
       return(FALSE);
    }

    stream = pups_fopen(trail_file_name,"r",LIVE);

    (void)pups_fgets(line,SSIZE,stream);
    (void)pups_fgets(line,SSIZE,stream);
    (void)pups_fgets(line,SSIZE,stream);
    (void)pups_fgets(line,SSIZE,stream);
    (void)pups_fgets(line,SSIZE,stream);
    (void)pups_fgets(line,SSIZE,stream);

    (void)sscanf(line,"%s%s%s",pen,host,host_port);
    (void)pups_fclose(stream);

    (void)pups_set_errno(OK);
    return(TRUE);
}




#ifdef PSRP_AUTHENTICATE
/*---------------------------------------------------*/
/* Set authentication token for (secure) PSRP server */
/*---------------------------------------------------*/

_PRIVATE int psrp_builtin_set_secure(const int argc, const char *argv[])

{    if(strcmp("secure",argv[0]) != 0)
        return(PSRP_DISPATCH_ERROR);

     (void)strlcpy(appl_password,argv[1],SSIZE);

     if(strcmp(appl_password,"notset") != 0)
     {  appl_secure = TRUE;
        (void)fprintf(psrp_out,"\nPSRP server \"%s\" has accepted new authentication token\n\n",appl_name);
     }
     else
     {  appl_secure = FALSE;
        (void)fprintf(psrp_out,"\nPSRP server \"%s\" authentication revoked (open access)\n\n",appl_name);
     }

     (void)fflush(psrp_out);

     if(appl_verbose == TRUE)
     {  (void)strdate(date);

        if(appl_secure == TRUE)
           (void)fprintf(stderr,"%s %s (%d@%s:%s): server authentication token updated\n",
                                             date,appl_name,appl_pid,appl_host,appl_owner);
        else
           (void)fprintf(stderr,"%s %s (%d@%s:%s): server authentication revoked (open access)\n",
                                                     date,appl_name,appl_pid,appl_host,appl_owner);

        (void)fflush(stderr);
    }

    return(PSRP_OK);
}

#endif /* PSRP_AUTHENTICATE */




#ifdef BUBBLE_MEMORY_SUPPORT
/*--------------------------------------------------------------------------*/
/* Set mallopt parameters. At present only one option mimimum memory bubble */
/* (utilisation) threshold is supported                                     */
/*--------------------------------------------------------------------------*/

_PRIVATE int psrp_builtin_set_mbubble_utilisation_threshold(const int argc, const char *argv[])

{   int mbubble_utilisation;

    if(strcmp("mset",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc >2 || (ptr = pups_locate(&init,"usage",&argc,args,0)) != NOT_FOUND)
    {  (void)fprintf(psrp_out,"\nusage: mset [-usage] | <minimum bubble utilisation %%>\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(argc == 1)
    {  (void)fprintf(psrp_out,"\nMemory bubble utilisation threshold is currently %3d %%\n",mallopt(MUNMAP_THRESHOLD,(-1.0)));
       (void)fprintf(psrp_out,"If utilisation falls below this threshold bubble will be unmapped (from process address space)\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(sscanf(argv[1],"%d",&mbubble_utilisation) == 1)
    {  (void)fprintf(psrp_out,"\nMemory bubble utilisation threshold is now %3d %%\n\n",mbubble_utilisation);
       (void)fflush(psrp_out);

       (void)mallopt(MUNMAP_THRESHOLD,mbubble_utilisation);
    }
    else
    {  (void)fprintf(psrp_out,"\nexpecting memory bubble utilisation threshold parameter\n\n");
       (void)fflush(psrp_out);
    }

    return(PSRP_OK);
}




/*------------------------*/
/* Show malloc statistics */
/*------------------------*/

_PRIVATE int psrp_builtin_show_malloc_stats(const int argc, const char *argv[])

{    if(strcmp("mstat",argv[0]) != 0)
        return(PSRP_DISPATCH_ERROR);

     jmalloc_usage(FALSE,psrp_out);
     return(PSRP_OK);
}

#endif /* BUBBLE_MEMORY_SUPPORT */



#ifdef CRIU_SUPPORT
/*------------------------------------*/
/* Enable/disable (Criu) state saving */
/*------------------------------------*/

_PRIVATE int psrp_builtin_ssave(const int argc, const char *argv[])

{   int i,
        itmp;

    char tmpstr[SSIZE] = "";

    if(strcmp("ssave",argv[0]) != 0)
        return(PSRP_DISPATCH_ERROR);


    /*------------------------------------*/
    /* No arguments - report ssave status */
    /*------------------------------------*/

    if(argc == 1)
    {  if(appl_ssave == FALSE)
       {  (void)fprintf(psrp_out,"\nstate saving is disabled\n\n");
          (void)fflush(psrp_out);
       }
       else
       {  (void)fprintf(psrp_out,"\nstate saving is enabled (poll time: %d seconds, %d state saves, criu directory: \"%s\")\n\n",
                                                                                                                  appl_poll_time,
                                                                                                                     appl_ssaves,
                                                                                                                   appl_criu_dir);
          (void)fflush(psrp_out);
       }

       return(PSRP_OK);
    }


    /*--------------------*/
    /* Parse command tail */
    /*--------------------*/

    (void)pups_clear_cmd_tail();
    for(i=0; i<argc; ++i)
    {

       /*---------------*/
       /* Display usage */
       /*---------------*/

       if(pups_locate(&init,"usage",&argc,argv,0) != NOT_FOUND)
       {  (void)fprintf(psrp_out,"\nusage: ssave [-t <poll_time:%d>]\n",appl_poll_time);
          (void)fprintf(psrp_out,"             [-enable | -disable | -cd <criu directory:/tmp>]\n\n");
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }


       /*---------------------*/
       /* Enable state saving */
       /*---------------------*/

       if(pups_locate(&init,"enable",&argc,argv,0) != NOT_FOUND)
       {  if(appl_ssave == TRUE)
          {  (void)fprintf(psrp_out,"\nstate saving already enabled\n\n");
             (void)fflush(psrp_out);

             return(PSRP_OK);
          }
          else
             appl_ssave = TRUE;

          (void)fprintf(psrp_out,"\nstate saving enabled (poll time: %d seconds, criu directory: \"%s\")\n",
                                                                                             appl_poll_time,
                                                                                              appl_criu_dir);
          (void)fflush(psrp_out);
       }


       /*----------------------*/
       /* Disable state saving */
       /*----------------------*/

       if(pups_locate(&init,"disable",&argc,argv,0) != NOT_FOUND)
       {  if(appl_ssave == FALSE)
          {  (void)fprintf(psrp_out,"\nstate saving already disabled\n\n");
             (void)fflush(psrp_out);

             return(PSRP_OK);
          }
          else
             appl_ssave = FALSE;

          (void)fprintf(psrp_out,"\nstate saving disabled\n\n");
          (void)fflush(psrp_out);
       }


       /*-------------------------------------------------*/
       /* Enable state saving with user defined poll time */
       /*-------------------------------------------------*/

       else if((ptr = pups_locate(&init,"t",&argc,argv,0)) != NOT_FOUND)
       {  
          if((itmp = pups_i_dec(&ptr,argc,args)) == (int)INVALID_ARG)
          {  (void)fprintf(psrp_out,"\nexpecting poll time in seconds (integer >= 0)\n\n");
             (void)fflush(psrp_out);

             return(PSRP_OK);
          }
          else if(itmp > 0)
          {  appl_ssave     = TRUE;
             appl_poll_time = itmp;

             (void)fprintf(psrp_out,"\nstate saving enabled (poll time: %d seconds, criu directory: \"%s\")\n",
                                                                                                appl_poll_time,
                                                                                                 appl_criu_dir);
             (void)fflush(psrp_out);
          }
          else if(itmp <= 0)
          {  appl_ssave = FALSE;

             (void)fprintf(psrp_out,"\nstate saving disabled\n\n");
             (void)fflush(psrp_out);
          }
       }


       /*------------------------------------------------*/
       /* Set Criu directory - used to store checkpoints */
       /*------------------------------------------------*/

       if((ptr = pups_locate(&init,"cd",&argc,argv,0)) != NOT_FOUND)
       {  if(strccpy(tmpstr,pups_str_dec(ptr,argc,args)) == (char *)INVALID_ARG)
          {  (void)fprintf(psrp_out,"\nexpecting name of criu directory\n\n");
             (void)fflush(psrp_out);

             return(PSRP_OK);
          }

          if(access(tmpstr,F_OK) == (-1))
          {  (void)snprintf(errstr,SSIZE,"[psrp_builtin_ssave] criu directory \"%s\" not found");
             pups_error(errstr);
          }
          else
             (void)strlcpy(appl_criu_dir,tmpstr,SSIZE);
       }
    }


    /*------------------------------------------------------*/
    /* If any arguments remain unparsed - complain and stop */
    /*------------------------------------------------------*/

    pups_t_arg_errs(argd,argv);

    return(PSRP_OK);
}

#endif /* CRIU_SUPPORT */




/*------------------------------------------------*/
/* Set vitimer quantum for PSRP handler subsystem */
/*------------------------------------------------*/

_PRIVATE int psrp_builtin_set_vitimer_quantum(const int argc, const char *argv[])

{   int itmp;

    if(strcmp("quantum",argv[0]) != 0)
       return(PSRP_DISPATCH_ERROR);

    if(argc >2 || (ptr = pups_locate(&init,"usage",&argc,args,0)) != NOT_FOUND)
    {  (void)fprintf(psrp_out,"\nusage: qunatum [-usage] | <quantum | default>\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(argc == 1)
    {  (void)fprintf(psrp_out,"\nPSRP (vitimer) quantum is %d milliseconds\n\n",vitimer_quantum);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(sscanf(argv[1],"%d",&itmp) == 1)
    {  if(itmp < 1000 || itmp > 1000000)
       {  (void)fprintf(psrp_out,"\nPSRP (vitimer) quantum must lie between 1000 and 1000000 milliseconds\n\n");
          (void)fflush(psrp_out); 

          return(PSRP_OK);
       }

       vitimer_quantum = itmp;
       (void)pups_malarm(vitimer_quantum);

       (void)fprintf(psrp_out,"\nPSRP (vitimer) quantum is now %d %%\n\n",vitimer_quantum);
       (void)fflush(psrp_out);
    }
    else if(strcmp(argv[1],"default") == 0)
    {  vitimer_quantum = VITIMER_QUANTUM;
       (void)pups_malarm(vitimer_quantum);

       (void)fprintf(psrp_out,"\nPSRP (vitimer) quantum is now %d %% (default)\n\n",vitimer_quantum);
       (void)fflush(psrp_out);
    }
    else
    {  (void)fprintf(psrp_out,"\nexpecting PSRP (vitimer) quantum parameter\n\n");
       (void)fflush(psrp_out);
    }

    return(PSRP_OK);
}





#ifdef MAIL_SUPPORT
/*----------------------------------------------*/
/* Extract return address from an incoming mail */
/*----------------------------------------------*/

_PRIVATE int extract_return_address(char *from_line, char *from_address)

{   int i,
        cnt = 0;

    _BOOLEAN copy_chars = FALSE;

    for(i=0; i<strlen(from_line); ++i)
    {  if(from_line[i] == '<')
          copy_chars = TRUE;
       else if(copy_chars == TRUE && from_line[i] != '>')
       {  from_address[cnt] = from_line[i];
          ++cnt;
       }

       if(from_line[i] == '>')
       {  from_address[cnt + 1] = '\0';
          return(0);
       }
    }

    return(-1);
}




/*----------------------------------------------------------------------*/
/* Read a message from the PSRP process inbox extracting any date which */
/* is MIME encapsulated. This is a wrapper wqhich calls the mhstore     */
/* function                                                             */
/*----------------------------------------------------------------------*/

_PUBLIC int psrp_mime_storeparts(const char *msg_file, const char *type)

{   char inc_command[SSIZE]      = "",
         line[SSIZE]             = "",
         typestr[SSIZE]          = "",
         address[SSIZE]          = "",
         mhn_command[SSIZE]      = "",
         mhstore_command[SSIZE]  = "";

    int   size;
    _BYTE buf[512] = "";
    FILE *pstream  = (FILE *)NULL;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_mime_storeparts] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(msg_file == (const char *)NULL || type == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*---------------------------------------------------------*/
    /* First read (mime) encapsulated message from maildrop    */
    /* and put it into the default inbox for the process which */
    /* is running this application                             */
    /*---------------------------------------------------------*/

    (void)snprintf(inc_command,SSIZE,"inc +%s | grep \"no mail\"",appl_mdir);
    if((pstream = popen(inc_command,"r")) == (FILE *)NULL)
       pups_error("[psrp_mime_storeparts] failed to open stream to \"inc\" command");

    (void)fscanf(pstream,"%s",line);
    (void)pclose(pstream);

    size = strlen(line);
    if(size > 2)
    {  pups_set_errno(EEXIST);
       return(-1);
    }


    /*--------------------------------------------*/
    /* Is next message addressed to this process? */
    /*--------------------------------------------*/

    (void)snprintf(mhn_command,SSIZE,"bash -c \"mhn +%s %s | grep Subject\"",appl_mdir,msg_file);
    if((pstream = popen(mhn_command,"r")) == (FILE *)NULL)
       pups_error("[psrp_mime_storeparts] failed to open stream to \"mhn\" command (address)");

    (void)fgets(line,SSIZE,pstream);
    (void)pclose(pstream);

    (void)snprintf(address,SSIZE,"%s@%s",appl_name,appl_host);
    if(strin(line,address) == TRUE)
    {  

       /*---------------------------------------------------------*/
       /* Yes someone has sent us a message! Lets find out who it */
       /* is and read what they have sent to us.                  */
       /*---------------------------------------------------------*/


       /*----------------------------------------------------------------*/
       /* Extract parts of (MIME encapsulated) message into directory    */
       /* If we have specified a particular type add an appropriate flag */
       /* to the mhstore command.                                        */
       /*----------------------------------------------------------------*/

       if(type != (char *)NULL || strcmp(type,"all") != 0)
          (void)snprintf(typestr,SSIZE,"-type %s ",type);


       /*-----------------------------------------------------------------*/  
       /* Extract parts of (MIME) message (into specified MIME directory) */ 
       /*-----------------------------------------------------------------*/  

       (void)snprintf(mhstore_command,SSIZE,"cd %s; mhstore +%s %s %s >& /dev/null",appl_mime_dir,appl_mdir,typestr,msg_file);
       (void)system(mhstore_command);

       pups_set_errno(OK);
       return(0);
    }

    pups_set_errno(EINVAL);
    return(-1);
}




/*----------------------------------------------------------------------*/
/* Extract text (first part) from MIME message. This is assumed to be a */
/* PSRP command -- if it is not return error condition to caller        */
/*----------------------------------------------------------------------*/

_PUBLIC int psrp_msg2requestlist(const char *msg_file, const char *psrp_requestlist)

{   _BOOLEAN looper   = TRUE;

    int server_pid,
        line_cnt     = 0,
        command_cnt  = 0;

    char msg_textpath[SSIZE] = "",
         next_request[SSIZE] = "",
         server_name[SSIZE]  = "";

    FILE *stream = (FILE *)NULL;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_msg2requestlist] attempt by non root thread to perform PUPS/P3 PSRP operation");


    /*------------------------------------------------------------------*/
    /* If we cannot access text return to caller (with error condition) */
    /*------------------------------------------------------------------*/

    if(msg_file == (const char *)NULL || psrp_requestlist == (const char *)NULL || access(msg_textpath,F_OK | R_OK | W_OK) == (-1))
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    /*-------------------------------------------*/
    /* Build path to text part of [MIME] message */
    /*-------------------------------------------*/

    (void)snprintf(msg_textpath,SSIZE,"%s/%s.1.txt",appl_mdir,msg_file);
    stream = pups_fopen(msg_textpath,"r",LIVE);


    /*---------------------------------------------------------*/
    /* First item is our process name -- if mail is not for us */
    /* delete it from local mailbox and return                 */
    /*---------------------------------------------------------*/

    if(strip_comment(stream,&line_cnt,server_name) == FALSE)
    {  (void)pups_fclose(stream);

       pups_set_errno(OK);
       return(-1);
    }       


    /*--------------------*/
    /* Build request list */
    /*--------------------*/

    do {    if(feof(stream) == 0)
            {  if(strip_comment(stream,&line_cnt,next_request) == FALSE)
               {  (void)pups_fclose(stream);


                  /*-------------------------------------------------*/
                  /* We must have read at least one command to avoid */
                  /* error condition                                 */
                  /*-------------------------------------------------*/

                  if(command_cnt == 0)
                  {  pups_set_errno(EINVAL);
                     return(-1);   
                  }

                  pups_set_errno(OK);
                  return(0);
               }
                                                                           
               (void)strlcat(psrp_requestlist,next_request,SSIZE);
               (void)strlcat(psrp_requestlist," ",SSIZE);

               if(command_cnt > 0 && strcmp(next_request,"none") == 0)
               {  pups_set_errno(EINVAL);
                  return(-1);
               }

               ++command_cnt;
            }
            else
               looper = FALSE;

       } while(looper == TRUE);

    (void)pups_fclose(stream);

    pups_set_errno(OK);
    return(0);
}




/*--------------------------------------------*/
/* Delete MIME message (at end of processing) */
/*--------------------------------------------*/

_PUBLIC int psrp_mime_delete(const char *msg_file)

{   int  ret;
    char msg_pathname[SSIZE] = "";


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_mime_delete] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(msg_file == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    (void)snprintf(msg_pathname,SSIZE,"%s/%s",appl_mdir,msg_file);
    ret = unlink(msg_pathname);

    pups_set_errno(OK);
    return(ret);
}




/*---------------------------------------------*/
/* Sends one file as MIME encapsulated message */
/*---------------------------------------------*/

_PUBLIC void psrp_mime_mail(const char *to,        // Recipient
                            const char *filetype,  // Type of (MIME) file
                            const char *filepath)  // Location of file

{   char    subject[SSIZE] = "";
    mc_type partlist;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_mime_mail] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(to       == (const char *)NULL  ||
       filetype == (const char *)NULL  ||
       filepath == (const char *)NULL   )
    {  pups_set_errno(EINVAL);
       return;
    }

    (void)snprintf(subject,SSIZE,"Automatic reply [%s@%s]",appl_name,appl_host);
    (void)strlcpy(partlist.type,filetype,SSIZE);
    (void)strlcpy(partlist.path,filepath,SSIZE);
    (void)psrp_mime_encapsulate(to,(char *)NULL,subject,1,&partlist);

    pups_set_errno(OK);
}




/*------------------------------------------------------*/
/* Send set of files and send as multipart MIME message */
/*------------------------------------------------------*/

_PUBLIC int psrp_mime_encapsulate(const char    *recipient, // Recipient (of MIME message)
                                  const char    *cc_list,   // CC list recipients
                                  const char    *subject,   // Subject (of message)
                                  const int     n_files,    // Number of MIME files
                                  const mc_type *partlist)  // Mime partlist

{   int i;

    char mcf_filename[SSIZE] = "",
         mail_script[SSIZE]  = "";

    FILE *stream = (FILE *)NULL;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_mime_encapsulate] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(recipient == (const char *)   NULL  ||
       cc_list   == (const char *)   NULL  ||
       subject   == (const char *)   NULL  ||
       partlist  == (const mc_type *)NULL   )
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*------------------------*/
    /* Build sendfiles script */
    /*------------------------*/

    if(recipient == (char *)NULL || partlist == (mc_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*-----------------------------------------------------*/
    /* If there is no subject send standard subject banner */
    /*-----------------------------------------------------*/

    if(subject == (char *)NULL)
       (void)snprintf(subject,SSIZE,"Automatic reply from PSRP server %s (%d@%s:%s)",appl_name,appl_pid,appl_host,appl_owner);


    /*-------------------------------------------*/
    /* Build MIME composition draft for filelist */
    /*-------------------------------------------*/

    (void)snprintf(mcf_filename,SSIZE,"/tmp/mcf.%d.%s.tmp",appl_pid,appl_host);
    (void)pups_creat(mcf_filename,0600);
    stream = fopen(mcf_filename,"w");

    if(cc_list != (char *)NULL)
       (void)fprintf(stream,"Cc: %s\n",cc_list);

    if(subject != (char *)NULL)
       (void)fprintf(stream,"Subject: %s\n",subject);

    (void)fprintf(stream,"-------\n");
    (void)fflush(stream);

    for(i=0; i<n_files; ++i)
    {

       /*----------------------------------------------------*/
       /* Check that the next file component actually exists */
       /*----------------------------------------------------*/

       if(access(partlist[i].path,F_OK | R_OK) == (-1))
       {  pups_set_errno(EEXIST);
          (void)fclose(stream);
          (void)unlink(mcf_filename);

          return(-1);
       }
       (void)fprintf(stream,"#%s %s\n",(char *)partlist[i].type,(char *)partlist[i].path);
       (void)fflush(stream);

    }
    (void)fclose(stream);


    /*--------------------------------------------------------------*/
    /* We now have the sendfiles_script built so lets mail the MIME */
    /* message!                                                     */
    /*--------------------------------------------------------------*/

    (void)snprintf(mail_script,SSIZE,"cat %s | mhbuild - | sendmail %s",mcf_filename,recipient);
    (void)system(mail_script);

    (void)unlink(mcf_filename);

    pups_set_errno(OK);
    return(0);
}




/*-------------------*/
/* Get reply address */
/*-------------------*/

_PUBLIC int psrp_get_replyto(const char *msg_file, char *replymailpath)

{   char mhn_command[SSIZE] = "",
         line[SSIZE]        = "";

    _BOOLEAN looper   = TRUE;
    FILE     *pstream = (FILE *)NULL;

    if(msg_file == (const char *)NULL || replymailpath == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_get_replyto] attempt by non root thread to perform PUPS/P3 PSRP operation");


    /*------------------------------------*/
    /* Does message have a replyto field? */
    /*------------------------------------*/

    (void)snprintf(mhn_command,SSIZE,"bash -c \"mhn +%s %s | grep \"Replyto:\"\"",appl_mh_folder,msg_file);
    if((pstream = popen(mhn_command,"r")) == (FILE *)NULL) 
    {  pups_set_errno(ENOEXEC);
       return(-1);
    }

    (void)fgets(line,SSIZE,pstream);
    (void)pclose(pstream);

    if(strin(line,"Replyto") == TRUE)
    {  (void)extract_return_address(line,replymailpath);
       return(0);
    }


    /*-----------------------------------*/
    /* Use from address as reply address */
    /*-----------------------------------*/

    (void)snprintf(mhn_command,SSIZE,"bash -c \"mhn +%s %s | grep \"From:\"\"",appl_mh_folder,msg_file);
    if((pstream = popen(mhn_command,"r")) == (FILE *)NULL)
    {  pups_set_errno(ENOEXEC);
       return(-1);
    }

    (void)fgets(line,SSIZE,pstream);
    (void)pclose(pstream);

    if(strin(line,"From") == TRUE)
    {  (void)extract_return_address(line,replymailpath);

       pups_set_errno(OK);
       return(0);
    }

    pups_set_errno(EINVAL);
    return(-1);
}




/*--------------------*/
/* Save PSRP channels */
/*--------------------*/

_PRIVATE FILE *psrp_saved_in  = (FILE *)NULL;
_PRIVATE FILE *psrp_saved_out = (FILE *)NULL;

_PRIVATE void psrp_save_channels(void)

{    psrp_saved_in  = psrp_in;
     psrp_saved_out = psrp_out;
}




/*-----------------------*/
/* Restore PSRP channels */
/*-----------------------*/

_PRIVATE void psrp_restore_channels(void)

{    psrp_in  = psrp_saved_in;
     psrp_out = psrp_saved_out;
}        




/*-------------------------------------------------------------------------*/
/* Mail homeostat (checks PSRP process mailbox for mail). If mail is found */
/* any PSRP commands are extracted from it and passed to the PSRP command  */
/* parser                                                                  */
/*-------------------------------------------------------------------------*/

_PUBLIC int psrp_mail_homeostat(void *t_info, const char *args)

{   int msg_number;

    char mailbox_lock[SSIZE]     = "",
         reply_cmd[SSIZE]        = "",
         strmsg_number[SSIZE]    = "",
         psrp_requestlist[SSIZE] = "",
         subject[SSIZE]          = "";

    FILE          *reply_stream  = (FILE *)NULL;
    struct dirent *next_item     = (struct dirent *)NULL;

    _IMMORTAL DIR *dirp          = (DIR *) NULL;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_mail_homeostat] attempt by non root thread to perform PUPS/P3 PSRP operation");


    /*-------------------------------------------------------*/
    /* PSRP command can only be processed from one interface */
    /* at a time                                             */
    /*-------------------------------------------------------*/

    if(in_psrp_handler == TRUE)
       return(-1);
    else
    {

       /*---------------------------------------------*/
       /* Block PSRP requests while mail is processed */
       /*---------------------------------------------*/

       (void)psrp_ignore_requests();
       (void)psrp_save_channels();
    }


    /*---------------------------*/
    /* Open PSRP process mailbox */
    /*---------------------------*/

    if(dirp == (DIR *)NULL)
    {  if((dirp = opendir(appl_mdir)) == (DIR *)NULL)
         pups_error("[psrp_mail_homeostat] failed to open process mailbox");
    }


    /*--------------------------------*/
    /* Loop over all files in mailbox */
    /*--------------------------------*/

    while((next_item = readdir(dirp)) != (struct dirent *)NULL)
    {    

         /*------------------------*/
         /* Is this a raw message? */
         /*------------------------*/

         if(snprintf(strmsg_number,SSIZE,"%d",next_item->d_name) == 1)
         {

            /*------------------------*/
            /* First lock the mailbox */
            /*------------------------*/

            (void)snprintf(strmsg_number,SSIZE,"%d",msg_number);

            if(access(strmsg_number,F_OK | R_OK) != (-1))
            {  (void)snprintf(mailbox_lock,SSIZE,"lock.%d.tmp",msg_number);
               while(pups_creat(mailbox_lock,0600) == (-1))
               {     (void)psrp_restore_channels();
                     (void)psrp_accept_requests();

                     return(-1);
               }


               /*-------------------------------------------------------------------*/
               /* We have the lock but someone else has already processed the mail! */
               /*-------------------------------------------------------------------*/

               if(access(strmsg_number,F_OK | R_OK) == (-1))
               {  
                  /*----------------*/
                  /* Unlock mailbox */
                  /*----------------*/

                  (void)unlink(mailbox_lock);
               }


               /*--------------------------------------------------------------*/
               /* We will produce some kind of reply -- give it a subject line */
               /*--------------------------------------------------------------*/

               (void)snprintf(subject,SSIZE,"Reply from PSRP server %s[%d]@%s",appl_name,appl_pid,appl_host);


               /*----------------------------------------------------------------------*/
               /* Check for mail (decoding and MIME encapsulated messages we may find) */
               /*----------------------------------------------------------------------*/

               if(psrp_mime_storeparts(strmsg_number,appl_mime_type) == 0)
               {  char reply[SSIZE] = "";


                  /*-------------------------------------------------------*/
                  /* Mail for us (extract PUPS/PSRP commandlist from text) */  
                  /*-------------------------------------------------------*/

                  if(psrp_msg2requestlist(strmsg_number,psrp_requestlist) == (-1))
                  {

                     /*-----------------------------------------------------*/
                     /* Cannot understand mail -- tell sender about problem */
                     /*-----------------------------------------------------*/

                     if(psrp_get_replyto(strmsg_number,appl_replyto) == (-1))
                     {  if(appl_verbose == TRUE)
                        {  (void)strdate(date);
                           (void)fprintf(stderr,"%s %s(%d@%s:%s): problem with mail \"replyto\" address\n",
                                                              date,appl_name,appl_pid,appl_host,appl_owner);
                           (void)fflush(stderr);
                        }
                     }
                     else
                     {  (void)snprintf(reply_cmd,SSIZE,"mail -s %s s",appl_replyto,subject);
                        (void)snprintf(reply,SSIZE,"PSRP server %s[%d]@%s could not understand you!\n",appl_name,appl_pid,appl_host);


                        /*-------------*/
                        /* Mail sender */
                        /*-------------*/

                        if((reply_stream = popen(reply_cmd,"w")) != (FILE *)NULL)
                        {  (void)fputs(reply,reply_stream);
                           (void)pclose(reply_stream); 
                        }

                        if(appl_verbose == TRUE)
                        {  (void)strdate(date);
                           (void)fprintf(stderr,"%s %s(%d@%s:%s): mail request from \"%s\" rejected (message not understood)\n",
                                                                      date,appl_name,appl_pid,appl_host,appl_owner,appl_replyto);
                           (void)fflush(stderr);
                        }
                     }
                  }
                  else
                  {  char tmpfile[SSIZE]       = "",
                          reply_command[SSIZE] = "";


                     /*-------------------------------------*/
                     /* Secure server must be authenticated */
                     /*-------------------------------------*/

                     if(strcmp(appl_password,"notset") != 0)
                     {  if(strncmp(psrp_requestlist,"password",8) != 0)
                        {  char reply_cmd[SSIZE] = "";

                           (void)snprintf(reply_cmd,SSIZE,"mail -s %s s",appl_replyto,subject);
                           (void)snprintf(reply,SSIZE,"PSRP server %s[%d]@%s is secure (and requires authentication)\n",appl_name,appl_pid,appl_host);


                           /*-------------*/
                           /* Mail sender */
                           /*-------------*/

                           if((reply_stream = popen(reply_cmd,"w")) != (FILE *)NULL)
                           {  (void)fputs(reply,reply_stream);
                              (void)pclose(reply_stream);
                           }

                           if(appl_verbose == TRUE)
                           {  (void)strdate(date);
                              (void)fprintf(stderr,"%s %s(%d@%s:%s): mail request from \"%s\" rejected (not authenticated)\n",
                                                              date,appl_name,appl_pid,appl_host,appl_owner,appl_replyto);
                              (void)fflush(stderr);
                           }
                        }
                     }

                     if(appl_verbose == TRUE)
                     {  (void)strdate(date);
                        (void)fprintf(stderr,"%s %s(%d@%s:%s): processing incoming mail from \"%s\"\n",
                                             date,appl_name,appl_pid,appl_host,appl_owner,appl_replyto);
                        (void)fflush(stderr);
                     }

                     if(psrp_get_replyto(strmsg_number,appl_replyto) == (-1))
                     {  if(appl_verbose == TRUE)
                        {  (void)strdate(date);
                           (void)fprintf(stderr,"%s %s(%d@%s:%s): problem with mail \"replyto\" address\n",
                                                              date,appl_name,appl_pid,appl_host,appl_owner);
                           (void)fflush(stderr);
                        }
                     }

                     if(strcmp(psrp_requestlist,"none") != 0)
                     {

                        /*-----------------------------------------*/
                        /* Send commandlist to PSRP command parser */
                        /*-----------------------------------------*/

                        (void)snprintf(tmpfile,SSIZE,"%s.reply.%d.tmp",strmsg_number,appl_pid);
                        psrp_out = pups_fopen(tmpfile,"w",LIVE);
                        (void)psrp_parse_request(psrp_requestlist,MAIL_FACE);
                        (void)pups_fclose(psrp_out);


                        /*----------------------*/
                        /* Send reply to sender */
                        /*----------------------*/

                        (void)snprintf(reply_cmd,SSIZE,"cat %s | mail -s %s s",tmpfile,appl_replyto,subject);
                        (void)system(reply_command);
                     }


                     /*---------------------------------------------------*/
                     /* Application specific mail handler which runs when */
                     /* mail has been received.                           */
                     /*---------------------------------------------------*/

                     if((void *)appl_mail_handler != (void *)NULL)
                        (void)(*appl_mail_handler)(appl_mdir);
                  }


                  /*-------------------------------*/
                  /* Remove message (from MH drop) */
                  /*-------------------------------*/

                  (void)psrp_mime_delete(strmsg_number);


                  /*----------------*/
                  /* Unlock mailbox */
                  /*----------------*/

                  (void)unlink(mailbox_lock);
               } 
            }
        }
    }


    /*----------------------------------*/
    /* We have finished processing mail */
    /*----------------------------------*/

    (void)closedir(dirp);
    dirp = (DIR *)NULL;

    (void)psrp_restore_channels();
    (void)psrp_accept_requests();
}
#endif /* MAIL_SUPPORT */




/*--------------------------------------------------------------------------*/
/* Rename existing PSRP channels (this function expect all appl builtins to */
/* have been modified appropriately before it is called)                    */
/*--------------------------------------------------------------------------*/

_PUBLIC int psrp_rename_channel(const char *psrp_name)

{   char new_channel_name_in[SSIZE]   = "",
         new_channel_name_out[SSIZE]  = "",
         new_stdin_pst_name[SSIZE]    = "",
         new_stdout_pst_name[SSIZE]   = "",
         new_stderr_pst_name[SSIZE]   = ""; 


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_rename_channel] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(psrp_name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    (void)snprintf(appl_ch_name,SSIZE,"%s#%s",psrp_name,appl_host);

    (void)snprintf(new_channel_name_out,SSIZE,"%s/psrp#%s#fifo#out#%d#%d",appl_fifo_dir,appl_ch_name,appl_pid,appl_uid);
    (void)snprintf(new_channel_name_in,SSIZE, "%s/psrp#%s#fifo#in#%d#%d", appl_fifo_dir,appl_ch_name,appl_pid,appl_uid);

    if(isatty(0) == 1)
      (void)snprintf(new_stdin_pst_name,SSIZE,  "%s/pups.%s.%s.stdin.pst.%d:%d", appl_fifo_dir,psrp_name,appl_host,appl_pid,appl_uid);

    if(isatty(1) == 1)
      (void)snprintf(new_stdout_pst_name,SSIZE,"%s/pups.%s.%s.stdout.pst.%d:%d", appl_fifo_dir,psrp_name,appl_host,appl_pid,appl_uid);

    if(isatty(2) == 1)
      (void)snprintf(new_stderr_pst_name,SSIZE, "%s/pups.%s.%s.stderr.pst.%d:%d",appl_fifo_dir,psrp_name,appl_host,appl_pid,appl_uid);


    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,"PSRPLIB NEW CHIN:       %s\n",new_channel_name_in);
    (void)fprintf(stderr,"PSRPLIB NEW CHOUT:      %s\n",new_channel_name_out);
    (void)fprintf(stderr,"PSRPLIB NEW PST_STDIN:  %s\n",new_stdin_pst_name);
    (void)fprintf(stderr,"PSRPLIB NEW PST_STDOUT: %s\n",new_stdout_pst_name);
    (void)fprintf(stderr,"PSRPLIB NEW PST_STDERR  %s\n",new_stderr_pst_name);
    #endif /* PSRPLIB_DEBUG */


    /*-----------------------------------------------------*/
    /* Check to see if (renamed) channel is already in use */
    /* if it is abort the renaming operation               */
    /*-----------------------------------------------------*/

    if(access(new_channel_name_in,F_OK)  == 0 ||
       access(new_channel_name_out,F_OK) == 0 ||
       access(new_stdin_pst_name,F_OK)   == 0 ||
       access(new_stdout_pst_name,F_OK)  == 0 ||
       access(new_stderr_pst_name,F_OK)  == 0)
       return(-1);

    /*----------------------------------------------*/
    /* Tell clients that a server change is pending */
    /*----------------------------------------------*/

    (void)psrp_reactivate_clients();

    if(rename(channel_name_in, new_channel_name_in) == (-1))
    {  (void)strlcpy(channel_name_in, new_channel_name_in,SSIZE);
       return(-1);
    }

    if(rename(channel_name_out,new_channel_name_out) == (-1))
    {  (void)strlcpy(channel_name_out,new_channel_name_in,SSIZE);
       return(-1);
    }

    if(strcmp(new_stdin_pst_name,"") != 0)
    {  if(rename(ftab[0].fname,new_stdin_pst_name) == (-1))
          return(-1);

       (void)strlcpy(ftab[0].fname,new_stdin_pst_name,SSIZE);
    }

    if(strcmp(new_stdout_pst_name,"") != 0)
    {  if(rename(ftab[1].fname,new_stdout_pst_name) == (-1))
          return(-1);

       (void)strlcpy(ftab[1].fname,new_stdout_pst_name,SSIZE);
    }

    if(strcmp(new_stderr_pst_name,"") != 0)
    {  if(rename(ftab[2].fname,new_stderr_pst_name) == (-1))
          return(-1);

       (void)strlcpy(ftab[2].fname,new_stderr_pst_name,SSIZE);
    }

    pups_set_errno(OK);
    return(0);
}





/*------------------------------*/
/* Reactivate connected clients */
/*------------------------------*/

_PRIVATE int psrp_reactivate_clients(void)

{   int i;

    for(i=0; i<MAX_CLIENTS; ++i)
    {  if(psrp_client_pid[i] != (-1))
       {  if(kill(psrp_client_pid[i],SIGCLIENT) == (-1))
          {  if(appl_verbose == TRUE)
             {  (void)strdate(date);
                (void)fprintf(stderr,"%s %s (%d@%s:%s): failed to reactivate client %d\n",
                                           date,appl_name,appl_pid,appl_host,appl_owner,i);
                (void)fflush(stderr);
             }
          }
          else
          {  if(appl_verbose == TRUE)
             {  (void)strdate(date);
                (void)fprintf(stderr,"%s %s (%d@%s:%s): client %d reactivated\n",
                                  date,appl_name,appl_pid,appl_host,appl_owner,i);
                (void)fflush(stderr);
             }
          }
       }
    }

    return(0);
}




#ifdef SSH_SUPPORT
/*----------------------------------------------*/
/* Routine to start server on given target host */
/*----------------------------------------------*/

_PUBLIC int psrp_remote_start(const char *hostname, const _BOOLEAN bg, const int argc, const char *argv[])

{   int i,
        child_pid       = (-1);

    char cmdline[SSIZE] = "",
         ssh_cmd[SSIZE] = "";

    _BOOLEAN vector_found  = FALSE;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       pups_error("[psrp_remote_start] attempt by non root thread to perform PUPS/P3 PSRP operation");

    if(hostname == (const char *) NULL  || 
       argv     == (const char **)NULL   )
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*------------------------------*/
    /* Build command to be executed */
    /*------------------------------*/

    (void)strlcpy(cmdline,appl_bin_name,SSIZE);
    (void)strlcat(cmdline," ",SSIZE);

    for(i=0; i<argc+1; ++i)
    {  

       /*---------------------------------*/
       /* We need to ignore to -on <host> */
       /* command line item               */
       /*---------------------------------*/

       if(strcmp(argv[i],"-on") == 0)
          ++i;
       else
       { (void)strlcat(cmdline,argv[i],SSIZE);
         (void)strlcat(cmdline," ",SSIZE);
       }
    }


    /*----------------------------------------------*/
    /* Build command to be remotely executed        */
    /* note that we can use password authentication */
    /* as we are pushing server from command line   */
    /*----------------------------------------------*/

    #ifdef PSRP_DEBUG
    (void)fprintf(stderr,"REMOTE CMDLINE: %s\n",cmdline);
    (void)fflush(stderr);
    #endif /* PSRP_DEBUG */

    if(strcmp(ssh_remote_port,"") == 0)
       (void)snprintf(ssh_cmd,SSIZE,"exec ssh -n -p 22 %s@%s %s",appl_owner,hostname,cmdline);
    else
       (void)snprintf(ssh_cmd,SSIZE,"exec ssh -n -p %s %s@%s %s",ssh_remote_port,appl_owner,hostname,cmdline);


    /*------------------------------------------------------*/
    /* Is PSRP server running in background on remote host? */
    /*------------------------------------------------------*/

    if(bg == TRUE)
       (void)strlcat(ssh_cmd," >& /dev/null &",SSIZE);

    if(appl_verbose == TRUE)
    {  (void)strdate(date);

       if(bg == TRUE)
          (void)fprintf(stderr,"%s %s (%d@%s:%s): starting on host \"%s\" (in background)\n",
                                       date,appl_name,appl_pid,appl_host,appl_owner,hostname);
       else
          (void)fprintf(stderr,"%s %s (%d@%s:%s): starting on host \"%s\" (in foreground)\n",
                                       date,appl_name,appl_pid,appl_host,appl_owner,hostname);
       (void)fflush(stderr);
    }


    /*----------------------------------*/
    /* Parent side of fork              */
    /* Parent has to run ssh command    */
    /* so that it remains in foreground */
    /*----------------------------------*/

    if((child_pid = fork()) != 0)
    {  

       /*----------------*/
       /* Remote command */
       /*----------------*/

       (void)execlp("bash","bash","-c",ssh_cmd,(char *)0);


       /*------------------------*/
       /* We should not get here */
       /*------------------------*/

       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): failed to start on host \"%s\"\n",
                              date,appl_name,appl_pid,appl_host,appl_owner,hostname);
          (void)fflush(stderr);
       }
    }


    /*---------------------*/
    /* Child side of fork  */
    /* Remove PSRP channel */
    /* on launch-host      */
    /*---------------------*/
 
    else
       (void)pups_exit(0);
}
#endif /* SSH_SUPPORT */



/*-----------------------------------------*/
/* Process a set of (slaved) PSRP requests */
/*-----------------------------------------*/

#ifdef SSH_SUPPORT
_PUBLIC char **psrp_process_sic_transaction_list(const char  *hostname, // Host running remote server (and slaved client)
                                                 const char  *ssh_port, // Port for ssh connection to remote host
                                                 const int  n_requests, // Number of request to be sent to remote server
                                                 const char  *requests) // Request list
#else
_PUBLIC char **psrp_process_sic_transaction_list(const char *hostname,  // Host running remote server (and slaved client)
						 const int  n_requests, // Number of request to be sent to remote server
                 				 const       *requests) // Request list
#endif /* SSH_SUPPORT */

{   int  i,
         ret,
         size,
         eff_n_requests;

    _BOOLEAN looper           = FALSE,
             ignore_replys    = FALSE;

    char sic_name[SSIZE]      = "",
         next_request[SSIZE]  = "",
         *reply               = (char *)NULL,
         **replys             = (char **)NULL;

    psrp_channel_type *sic    = (psrp_channel_type *)NULL;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
      pups_error("[psrp_process_sic_transaction_list] attempt by non root thread to perform PUPS/P3 PSRP operation");

    #ifdef PSRPLIB_DEBUG
    (void)fprintf(stderr,"[psrp_process_sic_transaction_list] HOST:         \"%s\"\n",hostname);
    (void)fprintf(stderr,"[psrp_process_sic_transaction_list] REQUESTS:     %d\n",    n_requests);
    (void)fprintf(stderr,"[psrp_process_sic_transaction_list] REQUEST LIST: \"%s\"\n",requests);
    (void)fflush(stderr);
    #endif /* PSRPLIB_DEBUG */

    if(n_requests == 0 || requests == (const char *)NULL) 
    {  pups_set_errno(EINVAL);
       return((char **)NULL);
    }


    /*-----------------------------*/
    /* Are we going to log replys? */
    /*-----------------------------*/

    if(n_requests < 0)
    {  eff_n_requests *= (-1);
       ignore_replys   =  TRUE;
    }


    /*--------------------------------------------*/
    /* Create SIC channels for slaved interaction */
    /*--------------------------------------------*/


    /*----------------------------*/
    /* Identifier for SIC channel */
    /*----------------------------*/

    (void)snprintf(sic_name,SSIZE,"%s[sic]",appl_name);


    /*--------------*/
    /* SIC is local */
    /*--------------*/

    if(hostname == (const char *)NULL)
    {  

       #ifdef SSH_SUPPORT
       if((sic = psrp_create_slaved_interaction_client((char *)NULL,"","pslave")) == (psrp_channel_type *)NULL)
       #else
       if((sic = psrp_create_slaved_interaction_client("pslave")) == (psrp_channel_type *)NULL)
       #endif /* SSH_SUPPORT */

       {  if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@%s:%s): SIC creation failed\n",date,appl_name,appl_pid,appl_host,appl_owner);
             (void)fflush(stderr);
          }

          #ifdef PSRPLIB_DEBUG
          (void)fprintf(stderr,"[psrp_process_sic_transaction_list] local SIC_CREATE: failed\n");
          (void)fflush(stderr);
          #endif /* PSRPLIB_DEBUG */

          return((char **)NULL);
       }
    }


    #ifdef SSH_SUPPORT
    /*---------------*/
    /* SIC is remote */
    /*---------------*/

    else
    {  char eff_ssh_port[SSIZE] = "";


       /*------------------------*/
       /* Is ssh port specified? */
       /*------------------------*/

       if(ssh_port == (const char *)NULL)
          (void)strlcpy(eff_ssh_port,"22",SSIZE);
       else
          (void)strlcpy(eff_ssh_port,ssh_port,SSIZE);

       if((sic = psrp_create_slaved_interaction_client(hostname,eff_ssh_port,"pslave")) == (psrp_channel_type *)NULL)
       {  if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@%s:%s): SIC creation failed\n",date,appl_name,appl_pid,appl_host,appl_owner);
             (void)fflush(stderr);
          }

          #ifdef PSRPLIB_DEBUG
          (void)fprintf(stderr,"[psrp_process_sic_transaction_list] remote SIC_CREATE: failed\n");
          (void)fflush(stderr);
          #endif /* PSRPLIB_DEBUG */

          return((char **)NULL);
       }
    }
    #endif /* SSH_SUPPORT */


    /*---------------------------*/
    /* Create buffer for replies */
    /*---------------------------*/

    replys = (char **)calloc(eff_n_requests,sizeof(char *));


    /*----------------------------------*/
    /* Send requests and collect replys */
    /*----------------------------------*/

    (void)strext(' ',(char *)NULL,(char *)NULL);
    for(i=0; i<eff_n_requests; ++i)
    {   looper = strext(';',next_request,requests);

        #ifdef PSRPLIB_DEBUG
        (void)fprintf(stderr,"REQUEST %d: \"%s\"\n",i,next_request);
        (void)fflush(stderr);
        #endif /* PSRPLIB_DEBUG */

        if(looper != FALSE)
        {  if(ignore_replys == FALSE)
           {  if((reply = psrp_slaved_client_transaction(TRUE,sic,next_request)) != (char *)NULL) 
                 replys[i] = reply;

              #ifdef PSRPLIB_DEBUG
              (void)fprintf(stderr,"REPLY %d: \"%s\"\n",i,reply);
              (void)fflush(stderr);
              #endif /* PSRPLIB_DEBUG */

           }
           else
              (void)psrp_slaved_client_transaction(FALSE,sic, next_request);
        }
    }

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): SIC transaction done\n",date,appl_name,appl_pid,appl_host,appl_owner);
       (void)fflush(stderr);
    }

    (void) psrp_destroy_slaved_interaction_client(sic,TRUE);

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): SIC channel closed\n",date,appl_name,appl_pid,appl_host,appl_owner);
       (void)fflush(stderr);
    }

    pups_set_errno(OK);
    return(replys);
}




/*-----------------------------------------------*/
/* Set up (exec) search paths. This assumes that */
/* exec is invoking a bash (Bourne again) shell  */
/*-----------------------------------------------*/

_PUBLIC int psrp_exec_env(const char *remote_env)

{   char appl_cwd[SSIZE]  = "";

    if(remote_env == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }
    else
       pups_set_errno(OK);


    /*-----------------------*/
    /* Get current directory */
    /*-----------------------*/

    (void)getcwd(appl_cwd,SSIZE);


    /*--------------------*/
    /* Exec shell is bash */
    /*--------------------*/

    #ifdef PSRP_DEBUG
    (void)snprintf(remote_env,SSIZE,"source $HOME/.psrp_bash_profile >& /dev/null; echo $PATH; cd %s >& /dev/null",appl_cwd);
    #else
    (void)snprintf(remote_env,SSIZE,"source $HOME/.psrp_bash_profile >& /dev/null; cd %s >& /dev/null",appl_cwd);
    #endif /* PSRP_DEBUG */

    return(0);
}




/*-------------------------------*/
/* Enable PSRP abort status flag */
/*-------------------------------*/

_PUBLIC int psrp_enable_abrtflag(void)

{   psrp_abrtflag_enable = TRUE;

    pups_set_errno(OK);
    return(0);
}




/*--------------------------------*/
/* Disable PSRP abort status flag */
/*--------------------------------*/

_PUBLIC int psrp_disable_abrtflag(void)

{   psrp_abrtflag_enable = FALSE;

    pups_set_errno(OK);
    return(0);
}




/*-----------------------*/
/* Clear PSRP abort flag */
/*-----------------------*/

_PUBLIC int psrp_clear_abrtflag(void)
{   psrp_abrtflag = FALSE;

    pups_set_errno(OK);
    return(0);
}




/*-----------------------*/
/* Get PSRP abort status */
/*-----------------------*/

_PUBLIC _BOOLEAN psrp_get_abrtflag(void)

{   pups_set_errno(OK); 
    return(psrp_abrtflag);
}




/*----------------------------------------------------*/
/* Enable/disable PSRP client interrupt (via SIGABRT) */
/*----------------------------------------------------*/

_PUBLIC int psrp_critical(const _BOOLEAN critical)

{

    /*------------------------*/
    /* Enter critcial section */
    /*------------------------*/

    if(critical == TRUE) 

       /*------------------------------------------------*/
       /* Tell current client server is non interuptable */
       /*------------------------------------------------*/

       (void)kill(psrp_client_pid[c_client],SIGCRITICAL);


    /*-----------------------*/
    /* Exit critical section */
    /*-----------------------*/

    else


       /*--------------------------------------------*/
       /* Tell current client server is interuptable */
       /*--------------------------------------------*/

       (void)kill(psrp_client_pid[c_client],SIGCRITICAL);

    pups_set_errno(OK);
    return(0);
}




/*----------------------------------------------*/
/* Block/unblock client interrupt (via SIGABRT) */
/*----------------------------------------------*/

_PUBLIC int psrp_client_block(const _BOOLEAN block)

{   sigset_t block_set;

    (void)sigemptyset(&block_set);
    (void)sigaddset  (&block_set,SIGABRT);

    if(block == TRUE)
       (void)sigprocmask(SIG_BLOCK,  &block_set,(sigset_t *)NULL);
    else
       (void)sigprocmask(SIG_UNBLOCK,&block_set,(sigset_t *)NULL);

    pups_set_errno(OK);
    return(0);
}



/*-----------------------------------------*/
/* Is PEN (porcess execution name) unique? */
/*-----------------------------------------*/
 
_PUBLIC void psrp_pen_unique(void)

{
    if(psrp_pname_to_pid(appl_name) == PSRP_DUPLICATE_PROCESS_NAME)
    {  (void)strlcpy(errstr,"PEN (%s) is not unique",SSIZE);
       pups_error(errstr);
    }
}
