/*----------------------------------------------------------------------------- 
    Header for the standard utilities library.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 7.20
    Dated:   26th October 2023
    E-Mail:  mao@tumblingdice.co.uk
------------------------------------------------------------------------------*/


#ifndef UTILS_H
#define UTILS_H


/*------------------*/
/* Keep c2man happy */
/*------------------*/

#include <c2man.h>

#ifndef __C2MAN__
#include <signal.h>
#include <setjmp.h>
#include <time.h>
#include <termios.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <setjmp.h>


#ifdef _OPENMP
#include <omp.h>
#endif /* _OPENMP */

#endif /* __C2MAN__ */


/***********/
/* Version */
/***********/

#define UTILIB_VERSION              "7.20"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE                       2048



/*-------------------------------*/
/* Floating point representation */
/*-------------------------------*/

#include <ftype.h>


#ifdef SECURE
#include <sed_securicor.h>
#endif /* SECURE */


/*------------------------*/
/* Default Criu poll time */
/* (between state saves)  */
/*------------------------*/


#define DEFAULT_CRIU_POLL_TIME      60


/*-----------------------------------*/
/* Default software watchdog timeout */
/*-----------------------------------*/

#define DEFAULT_SOFTDOG_TIMEOUT     60



/*----------------------------------------------------------*/
/* Default resource allocations (for an application to run) */
/*----------------------------------------------------------*/

#define BOGOMIP_SCALING              10000.0
#define PUPS_DEFAULT_MIN_CPU         2.0 
#define PUPS_DEFAULT_MIN_MEM         10000
#define PUPS_DEFAULT_RESOURCE_WAIT   3600
#define PUPS_IGNORE_MIN_MEM          (-1)


/*-------------------*/
/* Checkpooint flags */
/*-------------------*/

#define CKPT_INCREMENTAL    (1 << 0)
#define CKPT_EXIT           (1 << 1) 
#define CKPT_VERBOSE        (1 << 2) 
#define CKPT_WRAP_FILES     (1 << 3) 
#define CKPT_CHILDREN       (1 << 4) 


/*------------------------*/
/* Checkpoint return type */
/*------------------------*/

#define CKPT_DONE           (1 << 0)
#define CKPT_RESTART        (1 << 1)


/*-----------------*/
/* File copy flags */
/*-----------------*/

#define PUPS_FILE_COPY      (1 << 0) 
#define PUPS_FILE_MOVE      (1 << 1) 
#define COPY_BUF_SIZE       32768 



/*------------------------------------*/
/* File transfer modes (for pups_scp) */
/*------------------------------------*/

#define FROMC        1  // Copy from remote 
#define FROMX        2  // Transfer from remote
#define TOC          3  // Copy to remote 
#define TOX          4  // Transfer to remote 


/*---------------------------------*/
/* Support for compressing streams */
/*---------------------------------*/

#ifdef ZLIB_SUPPORT
#include <zlib.h>
#define  gzFILE gzFile
#endif /* SUPPORT_ZLIB */


/*------------------------------------------------------------*/
/* Quantum for PUPS virtual interval timers (in microseconds) */
/*------------------------------------------------------------*/

#define VITIMER_QUANTUM    1000


/*------------------------------------------------*/
/* Backtrack exit code (from SIGSEGV backtracker) */
/*------------------------------------------------*/

#define PUPS_BACKTRACK     (-9999)


/*----------------------*/
/* Link file lock types */
/*----------------------*/

#define LOCK               1  
#define EXLOCK             1  
#define WRLOCK             2
#define RDLOCK             4



/*----------------------------*/
/* pups_lockf lock operations */
/*----------------------------*/

#define PUPS_UNLOCK        0
#define PUPS_RDLOCK        (1 << 0)
#define PUPS_TRY_RDLOCK    (1 << 1) 
#define PUPS_WRLOCK        (1 << 2) 
#define PUPS_TRY_WRLOCK    (1 << 3) 


/*--------------------------------*/
/* Wait forever (to acquire lock) */
/*--------------------------------*/

#define WAIT_FOREVER       (-1)


/*----------------------------------------------------*/
/* Try lock (but don't wait if it cannot be acquired) */
/*----------------------------------------------------*/

#define TRYLOCK            0


#ifdef BUBBLE_MEMORY_SUPPORT

#define pups_main bubble_target
_EXTERN _BOOLEAN checkpointing;

#else
#define pups_main main
#endif /* BUBBLE_MEMORY_SUPPORT */


/*--------------------------------------------------*/
/* Maximum number of (concurrent) locks per process */
/*--------------------------------------------------*/

#define PUPS_MAX_LOCKS   512 


/*-------------------------------------------*/
/* Maximum number of flock locks per process */
/*-------------------------------------------*/

#define PUPS_MAX_FLOCKS  512 



/*-----------------------------------------------*/
/* Defer process exit when pups_exit() is called */
/*-----------------------------------------------*/

#define PUPS_DEFER_EXIT   9999


/*------------------------------------------*/
/* Offset used in PUPS file protection mode */
/*------------------------------------------*/

#define PUPS_PROT_OFFSET  (-1000)


/*---------------*/
/* Load averages */
/*---------------*/

#define LOAD_AVERAGE_1     1
#define LOAD_AVERAGE_2     2
#define LOAD_AVERAGE_3     3


/*----------------------------------*/
/* Error code to return if no error */
/*----------------------------------*/

#undef  OK
#define OK                 0


/*-------------------------------------*/
/* Define homeostasis revocation flags */
/*-------------------------------------*/

#define PUPS_HPROT_NOREVOKE  0
#define PUP_HPROT_REVOKE     1




/*-----------------*/
/* Version control */
/*-----------------*/

#define NO_VERSION_CONTROL (-1)


/*--------------------------*/
/* Memory allocator options */
/*--------------------------*/

#define MALLOC_DUMB         0  // Ordinary xmalloc semantics
#define MALLOC_HOMEOSTATIC  1  // Homeostatic xmalloc semantics


/*--------------------------------------------------------------*/
/* These definitions are used by initftab() for stdio homeostat */
/* initialisation                                               */
/* -------------------------------------------------------------*/

#define STDIO_DEAD          1  // Use standard fork call
#define STDIO_LIVE          2  // Use virtual fork call


/*-----------------------------------------*/
/* Signals to be held by pupshold function */
/*-----------------------------------------*/

#define ALL_PUPS_SIGS       1
#define PSRP_SIGS           2


/*--------------------------------*/
/* File system object lock states */
/*--------------------------------*/

#define GETLOCK             (1 << 1) 
#define TSTLOCK             (1 << 2) 


/*--------------------------*/
/* Dynamic homeostat states */
/*--------------------------*/

#define FILE_RELOCATE       (1 << 0)
#define HOST_RENAME         (1 << 1) 
#define LOST_FILE_LOCATE    (1 << 2)


/*--------------------------*/
/* Extended file open flags */
/*--------------------------*/

#ifdef SUPPORT_MOPI
#define O_MOPI    65535
#endif /* SUPPORT_MOPI */

#define O_DUMMY             32768 
#define O_LIVE              16384


/*-----------------------------------------*/
/* Return codes for PUPS statkill function */
/*-----------------------------------------*/

#define PUPS_EXEC           0
#define PUPS_TERMINATED     (-1)
#define PUPS_STOPPED        (-2)


/*-----------------------------------------*/
/* Homeostatic (file system) object states */
/*-----------------------------------------*/

#define DEAD                (1 << 1) 
#define LIVE                (1 << 2) 


/*-----------------------------------------------------------------------*/
/* Maximum number of attempts to re-connect to a resource which has been */
/* lost                                                                  */
/* ----------------------------------------------------------------------*/

#define TRY_TIMEOUT         1 
#define MAX_TRYS            4 


/*------------------------------------------------*/
/* Maximum number of open files (per application) */
/*------------------------------------------------*/

#define MAX_FILES           512 


/*---------------------------*/
/* Maximum number of signals */
/*---------------------------*/

#define MAX_SIGS            68 


/*------------------------------------------------------------------------------
    Maximum number of children ...
------------------------------------------------------------------------------*/

#define MAX_CHILDREN        512 


/*------------------------------------------------*/
/* Timer modes (for PUPS virtual interval timers) */
/*------------------------------------------------*/

#define MAX_VTIMERS         512 

#define VT_NONE             0
#define VT_ONESHOT          1
#define VT_CONTINUOUS       2


/*-----------------------------------------------------*/
/* Allocation quantum for dynamically allocated arrays */
/*-----------------------------------------------------*/

#define ALLOC_QUANTUM       64




/*-------------------------------------*/
/* Definition of signal parameter type */
/*-------------------------------------*/

typedef struct {   int  pid;                               // (Remote) pid to signal
                   int  signum;                            // Signal to be sent
                   int  r_pid;                             // PID of remote client
                   int  uid;                               // Id. of clients owner
                   char r_host[64];                        // Remote host name
                   char r_client[64];                      // Remote client name
               } sigparams_type;


/*----------------------------------*/
/* Definition of virtual timer type */
/*----------------------------------*/

typedef struct {   int  priority;                          // Priority of timer
                   char *name;                             // Name of payload func
                   int  mode;                              // Mode of timer
                   int  prescaler;                         // Interval prescaler
                   int  interval_time;                     // Length of interval
                   char *handler_args;                     // Handler argument string
                   void (*handler)(void *, char *);        // Handler function
               } vttab_type;



/*-------------------------------*/
/* Definition of file table type */
/*-------------------------------*/

typedef struct {   _BOOLEAN psrp;                          // Is this a PSRP stream?
                   _BOOLEAN pheap;                         // Is this a persistent heap?
                   _BOOLEAN creator;                       // TRUE if creator
                   _BOOLEAN named;                         // File (inode) is named
                   _BOOLEAN locked;                        // TRUE if file locked
                   _BOOLEAN mounted;                       // TRUE if on mounted fs
                   int      fdes;                          // File descriptor
                   int      id;                            // Entry id tag 
                   int      mode;                          // File access mode 
                   int      homeostatic;                   // Is this a live stream?
                   int      rd_pid;                        // Remote service daemon
                   int      fifo_pid;                      // Pipestream leaders PID
                   int      fs_blocks;                     // Min fs blocks for write
                   int      st_mode;                       // File protections
                   int      lost_cnt;                      // Loss count for file
                   FILE     *stream;                       // Stdio stream

#ifdef ZLIB_SUPPORT
                   gzFILE   *zstream;                      // Compression stream
#endif /* SUPPORT_ZLIB */

                   char     *hname;                        // Handler's name
                   char     *fname;                        // File name (if any)
                   char     *fs_name;                      // File system name
                   char     *fshadow;                      // File shadow
                   char     *rd_host;                      // RSD host (for rd_pid)
                   char     *rd_ssh_port;                  // RSD host ssh port (for rd_pid)
                   _BOOLEAN (*homeostat)(int, int);        // Dynamic fs. homeostat
                   void     (*handler)(void *, char *);    // Handler
               } ftab_type;


/*---------------------------------*/
/* Definition of signal table type */
/*---------------------------------*/

typedef struct {   unsigned long int haddr;                // Signal handler address
                   char              hname[SSIZE];         // Signal handler name
                   sigset_t          sa_mask;              // Sigs blocked in handler
                   int               sa_flags;             // Signal handler flags
               } sigtab_type;


/*------------------------------------------*/
/* Definition of dynamic memory object type */
/*------------------------------------------*/

typedef struct {  unsigned long int size;                  // Size of object (bytes)
                  int               n_el;                  // Number of elements
                  void              *ptr;                  // Pointer to object
                  char              *name;                 // Object name
                  char              *type;                 // Object type
               } matab_object_type;


/*-----------------------------------------*/
/* Definition of dynamic memory table type */
/*-----------------------------------------*/

typedef struct {  int               allocated;             // Objects in table
                   matab_object_type **object;             // Dynamic memory objects
               } matab_type;


/*---------------------------------*/
/* Definitions of child table type */
/*---------------------------------*/

typedef struct {   int      pid;                           // Child pid
                   _BOOLEAN obituary;                      // If TRUE log child death
                   char     *name;                         // Child name
               } chtab_type;


/*-------------------------*/
/* Seal pipestream on exit */
/*-------------------------*/

#define PIPESTREAM_DETACH   1


/*------------------------------------------------------*/
/* Error parameter definitions (for PUPS error handler) */
/*------------------------------------------------------*/

#define NONE                1
#define PRINT_ERROR_STRING  2
#define EXIT_ON_ERROR       4
#define CHILD_EXIT_ON_ERROR 8


/*---------------------------*/
/* Indicates a free resource */
/*---------------------------*/

#define FREE               (-9999)


/*-----------------------------------------------*/
/* Defines used by string manipulation functions */
/*-----------------------------------------------*/

#define N_STREXT_STRINGS   64
#define RESET_STREXT       (void)strext('*',(char *)NULL,(char *)NULL)
#define RESET_MSTREXT      1,(char *)NULL,(char *)NULL
#define END_STRING         (-9999) 
#define STREXT_ERROR       (-8888)
#define MSTREXT_RESET_ALL  1
#define MSTREXT_RESET_CUR  2


/*-------------------------------*/
/* Monitor function return codes */
/*-------------------------------*/

#define TIMED_OUT          0
#define READ_DATA          1
#define WRITE_DATA         2
#define EXCEPTION_RAISED   4 


/*---------------------------------------*/
/* Defines used by utilib error handlers */
/*---------------------------------------*/

#define PUPS_ERROR         (-1)
#define PUPS_OK            0
#define MAX_FUNCS          512 


/*-----------------------------------*/
/* Defines used by utilities library */
/*-----------------------------------*/

#define STRING_LENGTH      1024 
//#define LONG_LINE          1024
#define LONG_LONG_LINE     4096
#define NOT_FOUND          (-1)
#define INVALID_ARG        (-9999)
#define PI                 3.14159265
#define DEGRAD             0.01745329
#define RADDEG             57.29578778
#define ASSUMED_ZERO       1.0e-128



#ifdef __NOT_LIB_SOURCE__
/*--------------------------------*/
/* External variable declarations */
/*--------------------------------*/

_IMPORT jmp_buf   appl_resident_restart;  // Restart location (for memory resident application)

#ifdef SSH_SUPPORT
_EXPORT char     ssh_remote_port[];       // Remote (ssh) port
_EXPORT char     ssh_remote_uname[];      // Remote (ssh) username
_EXPORT _BOOLEAN ssh_compression;         // Use ssh compression
#endif /* SSH_SUPPORT */

_EXPORT _BOOLEAN    pups_abort_restart;   // TRUE if abort handler enabled
_EXPORT  sigjmp_buf pups_restart_buf;     // Abort restart buffer
_EXPORT _BOOLEAN    fs_write_blocked;     // TRUE if filesystem is full

#ifdef CRIU_SUPPORT
_EXPORT _BOOLEAN appl_criu_ssave;         // TRUE if (criu) state saving enabled
#endif /* CRIU_SUPPORT */

_EXPORT int sargc,                        // Number of arguments in command tail
            ptr,                          // Pointer to current option
            t_args,                       // Total args in generic command tail
            vitimer_quantum,              // Timer quantum (for PSRP timers)
            appl_fsa_mode,                // Filesystem access mode
            appl_t_args,                  // Total args in command tail
            appl_alloc_opt,               // Memory allocator options (for xmalloc etc)
            appl_max_files,               // Maximum number of ftab slots
            appl_max_child,               // Maximum number of child slots
            appl_max_pheaps,              // Maximum number of persistent heaps
            appl_max_vtimers,             // Maximum number of virtual timers
            appl_max_orifices,            // Maximum number of orifice [DLL] slots
            appl_vtag,                    // Application (compile) version tag
            base_arg,                     // Base argument for argfile ops
            appl_last_child,              // Last child process forked by xsystem
            appl_sid,                     // Session (process group) i.d. 
            appl_pid,                     // Process i.d.
            appl_ppid,                    // (Effective) parent PID
            appl_uid,                     // Process UID (user i.d.)
            appl_gid,                     // Process group i.d.
            appl_tty,                     // Process controlling terminal descriptor
            appl_remote_pid,              // Remote PID to relay signals to

#ifdef CRIU_SUPPORT
            appl_criu_ssaves,             // Number of times state has been saved
            appl_poll_time,               // Pol time for (Criu) state saving
#endif /* CRIU_SUPPORT */

#ifdef _OPENMP
            appl_omp_threads,             // Number of OMP threads
#endif /* _OPENMP */

#ifdef PTHREAD_SUPPORT
            appl_root_tid;                // LWP i.d. of root thread
            appl_root_thread,             // Root (initial) thread for process
#endif /* PTHREAD_SUPPORT */

            appl_nice_lvl,                // Process niceness
            appl_softdog_timeout,         // Software watchdog timeout
            appl_softdog_pid;             // Software watchdog process PID



_EXPORT _BOOLEAN appl_verbose,            // TRUE if verbose mode selected
                 appl_resident,           // Application is memory resident 
                 appl_enable_residence,   // Application can be memory resident
                 appl_etrap,              // TRUE if error trapping for gdb analysis
                 appl_proprietary,        // TRUE if application is proprietary

#ifdef MAIL_SUPPORT
                 appl_mailable,           // TRUE if mail can be sent to process
#endif /* MAIL_SUPPORT */

                 test_mode,               // TRUE if test mode
                 init,                    // TRUE if first option
                 appl_pg_leader,          // TRUE if process group leader
                 appl_nodetach,           // Don't detach stdio in background
                 appl_fgnd,               // TRUE if process in foreground process group
                 appl_snames_crypted,     // TRUE if shadow filenames crypted
                 appl_wait,               // TRUE if process waiting (SIGSTOP)
                 appl_psrp,               // TRUE if application PSRP enabled
                 appl_ppid_exit,          // If TRUE exit if parent terminates 
                 appl_rooted,             // IF TRUE system context cannot migrate
                 appl_psrp_load,          // If TRUE load PSRP resources at startup
                 appl_psrp_save,          // If TRUE save dispatch table at exit
                 appl_have_pen,           // TRUE if binname != exec name
                 appl_default_chname,     // TRUE if PSRP channel name default
		 appl_softdog_enabled,    // TRUE if software watchdog enabled
                 argd[];                  // Argument decode status flags

_EXPORT char *version,                    // Version of program
             *appl_owner,                 // Name of application owner
             *appl_password,              // Password of application owner
             *appl_name,                  // Application (process) name of program 
             *appl_logfile,               // Error/log file 
             *appl_bin_name,              // Application (binary) name of program
             *appl_remote_host,           // Remote host to relay signals to

#ifdef MAIL_SUPPORT
             *appl_mdir,                  // Application mailbox
             *appl_mh_folder,             // Application MH (MIME) folder
             *appl_mime_dir,              // Process MIME workspace
             *appl_mime_type,             // MIME message part type (to retreive)
             *appl_replyto,               // Applicationm reply to address (for mail)
#endif /* MAIL_SUPPORT */

             *appl_fifo_dir,              // Application default FIFO patchboard directory
             *appl_fifo_topdir,           // Directory containing application patchboards
             *appl_ch_name,               // Application PSRP channel name
             *appl_err,                   // Application error string
             *appl_hostpool,              // Application capabilities database
             *appl_ben_name,              // Application binary
             *appl_pam_name,              // PUPS authentication module name
             *appl_ttyname,               // Applications controlling terminal
             *appl_uprot_tag,             // Homeostasis (file) revocation tag
             *author,                     // Author of program
             *revdate,                    // Date of last modification
             *shell,                      // Application shell
	     *appl_home,                  // Effective home directory
             *appl_cwd,                   // Application current working directory
             *appl_cmd_str,               // Application command string
             *appl_host,                  // Host node running this application
             *appl_state,                 // State of this application
             *arg_f_name,                 // Argument file name
             *appl_tunnel_path,           // Tunnel path (binary for tunnel process
             *args[256],                  // Secondry argument vectors
             appl_argfifo[];              // Argument FIFO (for memory resident process)

#ifdef CRIU_SUPPORT
_EXPORT char appl_criu_ssave_dir[];       // Criu checkpoint directory for state saving
_EXPORT char appl_criu_dir[];             // Criu directory (holds migratable files and checkpoints) 
#endif /* CRIU_SUPPORT */

_EXPORT char date[SSIZE];                 // Date and time stamp
_EXPORT char errstr[SSIZE];               // Error string
_EXPORT char appl_machid[SSIZE];          // Unique machine (host) identifier
_EXPORT void *exit_function;              // Exit function to be called by SIGTERM


#ifdef PTHREAD_SUPPORT 
_EXPORT pthread_mutex_t pups_fork_mutex;  // Thread safe pups_fork mutex
_EXPORT pthread_mutex_t ftab_mutex;       // Thread safe file table mutex
_EXPORT pthread_mutex_t chtab_mutex;      // Thread safe child table mutex
#endif /* PTHREAD_SUPPORT */


_EXPORT _BOOLEAN    appl_kill_pg;
_EXPORT _BOOLEAN    appl_secure;
_EXPORT _BOOLEAN    pups_process_homeostat;
_EXPORT _BOOLEAN    ftab_extend;
_EXPORT ftab_type   *ftab;
_EXPORT chtab_type  *chtab;
_EXPORT vttab_type  *vttab;
_EXPORT sigtab_type sigtab[MAX_SIGS];
_EXPORT _BOOLEAN    default_fd_homeostat_action;
_EXPORT _BOOLEAN    ignore_pups_signals;
_EXPORT _BOOLEAN    pups_exit_entered;
_EXPORT _BOOLEAN    in_vt_handler;
_EXPORT _BOOLEAN    in_jmalloc;


#ifdef MAIL_SUPPORT
/*----------------------------*/
/* Application mail handler   */
/*----------------------------*/

_EXPORT int         (*appl_mail_handler)(char *);
#endif /* MAIL_SUPPORT */

#else /* External variables */
#   undef  _EXPORT
#   define _EXPORT
#endif /* __NOT_LIB_SOURCE__ */


/*------------------------------------------------------------------------------
    Prototype function definitions ...
------------------------------------------------------------------------------*/

// Thread aware sigprockmask()
_PROTOTYPE _EXPORT int pups_sigprocmask(int, const sigset_t *restrict, sigset_t *restrict);

// Get time (accurate to milliseconds)
_PROTOTYPE _EXPORT double millitime(void);

// Get node name and I.P. address associated with network interface
_PROTOTYPE _EXPORT int pups_get_ip_info(const char *, char *, char *);

// Forward indexing function
_PROTOTYPE _EXPORT long int ch_index(const char *, const char);

// Reverse indexing function
_PROTOTYPE _EXPORT long int rch_index(const char *, const char);

// Extended getpwnam routine (which searches NIS passwd map)
_PROTOTYPE _EXPORT struct passwd *pups_getpwnam(const char *);

// Extended getpwuid routine (which searches NIS passwd map)
_PROTOTYPE _EXPORT struct passwd *pups_getpwuid(const int);

// Set up support child prior to dangerous operation
_PROTOTYPE _EXPORT int pups_process_homeostat_enable(const _BOOLEAN);

// Cancel support child (at end of dangerous operation)
_PROTOTYPE _EXPORT void pups_process_hoemostat_disable(const int);

// Clear a virtual timer datastructure
_PROTOTYPE _EXPORT int pups_clear_vitimer(const _BOOLEAN, const unsigned int);

// Get file table index (associated with a file descriptor)
_PROTOTYPE _EXPORT int pups_get_ftab_index_from_fd(unsigned const int);

// Get file table index (associated with a stream)
_PROTOTYPE _EXPORT  int pups_get_ftab_index_from_stream(const FILE *);

// Restart virtual interval timers
_PROTOTYPE _EXPORT int pups_vitrestart(void);

// Find free file table index
_PROTOTYPE _EXPORT int pups_find_free_ftab_index(void);

#ifdef ZLIB_SUPPORT
// Get file table index (associated with a zstream)
_PUBLIC int pups_get_ftab_index_from_zstream(const gzFILE *);
#endif /* ZLIB_SUPPORT */


// Show number of link file locks concurrently held
_PROTOTYPE _PUBLIC int pups_show_link_file_locks(const FILE *);

// Apply file lock
_PROTOTYPE _EXPORT int pups_lockf(const int, const int, const unsigned long int);

// Test to see if file is on a mounted filesystem
_PROTOTYPE _EXPORT _BOOLEAN pups_get_fs_mountinfo(const char *, char *);

// Set system error number
_PROTOTYPE _EXPORT void pups_set_errno(int);

// Get system error number
_PROTOTYPE _EXPORT int pups_get_errno(void);

// Swap a pair of integers
_PROTOTYPE _EXPORT void iswap(int *, int *);

// Swap a pair of floats
_PROTOTYPE _EXPORT void fswap(FTYPE *, FTYPE *);

// Set error handler parameters
_PROTOTYPE _EXPORT void pups_seterror(const FILE *, const int, const int);

// Error handler
_PROTOTYPE _EXPORT int pups_error(const char *);

// Read standard items from command tail
_PROTOTYPE _EXPORT void pups_std_init(_BOOLEAN, int  *,char *,char *,char *,char *,char *[]);

// Load argument vector from file
_PROTOTYPE _EXPORT void pups_argfile(int, int *, const char *[], _BOOLEAN []);

// Generate effective command tail string from secondary argument vector
_PROTOTYPE _EXPORT void pups_argtline(char *);

// Copy command tail
_PROTOTYPE _EXPORT void pups_copytail(int *, const char *[], char *[]);

// Locate switch in command tail
_PROTOTYPE _EXPORT int pups_locate(_BOOLEAN *, const char *, int *, const char*[], int);

// Decode character item from command tail
_PROTOTYPE _EXPORT char pups_ch_dec(int *, int *, const char *[]);

// Decode integer from command tail
_PROTOTYPE _EXPORT int pups_i_dec(int *, int *, const char *[]);

// Decode float from command tail
_PROTOTYPE _EXPORT FTYPE pups_fp_dec(int *, int *, const char *[]);

// Decode string item from command tail
_PROTOTYPE _EXPORT char *pups_str_dec(int *, int *, const char *[]);



#ifdef SECURE
// Application security routine
_PROTOTYPE _EXPORT void pups_securicor(const char *);
#endif /* SECURE */


// Check if a terminal device has been redirected
_PROTOTYPE void pups_check_redirection(const des_t des);

// Convert character to integer
_PROTOTYPE _EXPORT int actoi(const char);

// Get absolute value of integer
_PROTOTYPE _EXPORT int iabs(int);

// Test for even integer
_PROTOTYPE _EXPORT _BOOLEAN ieven(const int);

// Test for odd integer
_PROTOTYPE _EXPORT _BOOLEAN iodd(const int);

// Get absolute value of integer
_PROTOTYPE _EXPORT int iodd(const int);

// Test for integer sign
_PROTOTYPE _EXPORT _BOOLEAN isign(const int);

// Find the maximum of a pair of integers
_PROTOTYPE _EXPORT int imax(const int, const int);

// Find the minimum of a pair of integers
_PROTOTYPE _EXPORT int imin(const int, const int);

// Get sign of character
_PROTOTYPE _EXPORT int chsign(const char);

// Pause/test routine
_PROTOTYPE _EXPORT int upause(const char *);

// Check for file and open it if it exists
_PROTOTYPE _EXPORT int pups_open(const char *, const int, int);

// Close file
_PROTOTYPE _EXPORT int pups_close(const int);

// Test if (named) file is living
_PROTOTYPE _EXPORT _BOOLEAN pups_isalive(const char *);

// Protect an (unopened) file
_PROTOTYPE _EXPORT int pups_protect(const char *, const char *, const void *);

// Unprotect an (unopened) file
_PROTOTYPE _EXPORT int pups_unprotect(const char *);

// Return the number of times a live file descriptor has been lost (and recreated)
_PROTOTYPE _EXPORT int pups_lost(const int);

// Test if file descriptor is living 
_PROTOTYPE _EXPORT _BOOLEAN pups_fd_islive(const int);

// Make dead file descriptor alive
_PROTOTYPE _EXPORT int pups_fd_alive(const int, const char *, const void *);

// Kill living file descriptor
_PROTOTYPE _EXPORT int pups_fd_dead(const int);

// CLose all open (ftab) file descriptors
_PROTOTYPE _EXPORT void pups_closeall(void);

// Display currently open files
_PROTOTYPE _EXPORT void pups_show_open_fdescriptors(const FILE *);

// Strip a comment from an input file
_PROTOTYPE _EXPORT _BOOLEAN strip_comment(const FILE *, int *, char *);

// Test for existence of substring
_PROTOTYPE _EXPORT _BOOLEAN strncmps(const char *, const char *);

// Strip an extension from a string
_PROTOTYPE _EXPORT _BOOLEAN strrext(const char *, char *, const char);

// Reverse strip an extension from a string
_PROTOTYPE _EXPORT _BOOLEAN strrextr(const char *, char *, const char);

// Copy string excluding characters
_PROTOTYPE _EXPORT size_t strexccpy(const char *, char *, const char *);

// Extract substring from string
_PROTOTYPE _EXPORT _BOOLEAN strext(const char, char *, const char *);

// Check if string 2 is a substring of string 1
_PROTOTYPE _EXPORT _BOOLEAN strin(const char *, const char *);


/*----------------------------------------------*/
/* Check if string 2 is a substring of string 1 */
/* returning tail of string 1                   */
/*----------------------------------------------*/

_PROTOTYPE _EXPORT char *strin2(const char *, const char *);

// Check if string 2 is a substring of string 1 [with string 2 demarcation check]
_PROTOTYPE _EXPORT _BOOLEAN strintok(const char *, const char *, const char *);

// Check if string 3 is to right of string 2 within string 1
_PROTOTYPE _EXPORT _BOOLEAN stright(const char *, const char *, const char *);

// Replace string s3 with string s4 globally within string s1, returning as string s2
_PROTOTYPE _EXPORT _BOOLEAN strep(const char *, char *, const char *, const char *);

// Replace string with a character constant
_PROTOTYPE _EXPORT _BOOLEAN strepch(char *, const char *, const char);


/*-----------------------------------------------------------------------------------------*/
/* Replace string demarcated string s3 with string s4 globally within string s1, returning */
/* as string s2                                                                            */
/*-----------------------------------------------------------------------------------------*/

_PROTOTYPE _EXPORT _BOOLEAN streptok(const char *, char *, const char *, const char *, const char *);

// Truncate string at demarcation character (from string tail)
_PROTOTYPE _EXPORT _BOOLEAN strtrnc(char *, const char, int);

// Truncate string at demarcation character (from string head)
_PROTOTYPE _EXPORT _BOOLEAN strtrnch(char *, const char, int);

// Strip (nominated) trailing character string
_PROTOTYPE _EXPORT _BOOLEAN strail(char *, const char);

// Extract leaf from pathname
_PROTOTYPE _EXPORT _BOOLEAN strleaf(const char *, char *);

// Extract branch from pathname
_PROTOTYPE _EXPORT _BOOLEAN strbranch(const char *, char *);

// Strip trailing character string (which begins with digit)
_PROTOTYPE _EXPORT _BOOLEAN strdigit(char *);

// strip (nominated) leading characters from string
_PROTOTYPE _EXPORT char *strlead(char *, const char);

// Return substring which starts at first non-whitespace character
_PROTOTYPE _EXPORT char *strfirst(char *);

// Extract string
_PROTOTYPE _EXPORT _BOOLEAN strfrm(const char *, const char, int, char *);

// Count number of time character occurs in string
_PROTOTYPE _EXPORT int strchcnt(const char, const char *);

// Extract substring from string [multiple embedded comparisons]
_PROTOTYPE _EXPORT _BOOLEAN m_strext(const int, const char, char *, const char *);

// Routine to return the position of a character in a string
_PROTOTYPE _EXPORT long int ch_pos(const char *, const char);

// Routine to return the reverse position of a character in a string
_PROTOTYPE _EXPORT long int rch_pos(const char *, const char);

// Extract substring from string with multiple demarcation characters
_PROTOTYPE _EXPORT _BOOLEAN mdc_strext(const char *, int *, char *, const char *);

// Extract substring from string with multiple demarcation characters
_PROTOTYPE _EXPORT _BOOLEAN mdc_strext2(const char *, int *, int *, int *, char *, const char *);

// Look for occurence of string 1 inside string 2 returning index to string 1
_PROTOTYPE _BOOLEAN strinp(unsigned long int *, const char *, const char *);

// Strip numeric characters from a string
_PROTOTYPE _EXPORT _BOOLEAN strpdigit(const char *, char *);

// Strip character from string
_PROTOTYPE _EXPORT char *strpch(const char, char *);

// Replace characters in string with nominated character
_PROTOTYPE _EXPORT int mchrep(const char, const char *, char *);

// Copy string returning state of copied string pointer
_PROTOTYPE _EXPORT char *strccpy(char *, const char *);

// Test for empty string (contains only whitespace and control chars)
_PROTOTYPE _EXPORT _BOOLEAN strempty(const char *);

// Generate random alphanumeric string
_PROTOTYPE _EXPORT int strand(const size_t, char *);

// Find sign of float
_PROTOTYPE _EXPORT FTYPE fsign(const FTYPE);

// Round a floating point number to N significant figures
_PROTOTYPE _EXPORT FTYPE sigfig(const FTYPE, const unsigned int);

// Pascal compatable squaring routine
_PROTOTYPE _EXPORT FTYPE sqr(const FTYPE);

// Pascal compatable round routine
_PROTOTYPE _EXPORT int iround(const FTYPE);

// Get next free PUPS file table index
_PROTOTYPE _EXPORT int pups_find_ftab_index(void);

// Get next free PUPS file table index
_PROTOTYPE _EXPORT int pups_get_ftab_index(const int);

// Open file
_PROTOTYPE _EXPORT FILE *pups_fopen(char *, char *, int);

// Bind descriptor to stream
_PROTOTYPE _EXPORT FILE *pups_fdopen(const int, const char *);
 
// Close file
_PROTOTYPE _EXPORT FILE *pups_fclose(const FILE *);


#ifdef ZLIB_SUPPORT
// Open file associated with zstream
_PROTOTYPE _EXPORT gzFILE *pups_xgzopen(const char *, const char *, const int);

// Close file associated with zstream
_PROTOTYPE _EXPORT gzFILE *pups_gzclose(const gzFILE *);
#endif /* ZLIB_SUPPORT */


// Strip comments from file
_PROTOTYPE _EXPORT FILE *pups_strp_commnts(const char, const FILE *, char *);

// Print arguments which have not been parsed by application
_PROTOTYPE _EXPORT void pups_t_arg_errs(const _BOOLEAN *, const char *[]);

// Extended write - checks that N bytes are in fact read 
_PROTOTYPE _EXPORT int pups_write(const des_t, const _BYTE *, const psize_t);

// Extended write - checks that N bytes are in fact written
_PROTOTYPE _EXPORT int pups_read(const des_t, _BYTE *, const psize_t);

// Set memory allocator (extended) options
_PROTOTYPE _EXPORT void pups_set_alloc_opt(const int);

// Extended malloc - checks for allocation
_PROTOTYPE _EXPORT void *pups_malloc(const psize_t);

// Extended realloc - checks for allocation
_PROTOTYPE _EXPORT void *pups_realloc(void *, const psize_t);

// Extended calloc - checks for allocation
_PROTOTYPE _EXPORT void *pups_calloc(const pindex_t, const psize_t);

// Extended free - checks for unassigned pointer
_PROTOTYPE _EXPORT void *pups_free(const void *);

// Set process state
_PROTOTYPE _EXPORT void pups_set_state(const char *);

// Routine to get an application state vector
_PROTOTYPE _EXPORT void pups_get_state(const char *);

// Display an application state vector
_PROTOTYPE _EXPORT void pups_show_state(void);

// Register name of (file) creator
_PROTOTYPE _EXPORT int pups_creator(const int);

// Relinquish creator rights to file
_PROTOTYPE _EXPORT int pups_not_creator(const int);

// Generate command vector from string and then do execv */
_PROTOTYPE _EXPORT int pups_execls(const char *);

// Routine to allocate a two dimensional array
_PROTOTYPE _EXPORT void **pups_aalloc(const pindex_t, const pindex_t, const psize_t);

// Routine to free a two dimensional array
_PROTOTYPE _EXPORT void **pups_afree(pindex_t, void **);

// Routine to perform a heapsort
_PROTOTYPE _EXPORT void sort2(int, FTYPE [], FTYPE[]);

// Get entries in directory
_PROTOTYPE _EXPORT char **pups_get_directory_entries(const char *, const char *, int *, int *);

// Get (multikeyed) entries in directory
_PROTOTYPE _EXPORT char **pups_get_multikeyed_directory_entries(const char *, const int, const char **, int *, int *);

// Read call which is not interrupted by signal handling
_PROTOTYPE _EXPORT unsigned long int pups_sread(const int, char *, const unsigned long int);

// Write call which is not interrupted by signal handling
_PROTOTYPE _EXPORT unsigned long int pups_swrite(const int, const char *, const unsigned long int);

// Search a path for an item
_PROTOTYPE _EXPORT char *pups_search_path(const char *, const char *);

// Set process execution name
_PROTOTYPE _EXPORT void pups_set_pen(const char *[], const char *, const char *);

// Register process entrance function
_PROTOTYPE _EXPORT int pups_register_entrance_f(const char *, const void *, const char *);

// Deregister process entrance function
_PROTOTYPE _EXPORT int pups_deregister_entrance_f(const void *);

// Show entrance functions
_PROTOTYPE _EXPORT void pups_show_entrance_f(const FILE *);

// Register process exit function
_PROTOTYPE _EXPORT int pups_register_exit_f(const char *, const void *, const char *);

// Deregister process exit function
_PROTOTYPE _EXPORT int pups_deregister_exit_f(const void *);

// Show exit functions
_PROTOTYPE _EXPORT void pups_show_exit_f(const FILE *);

// Register process abort function
_PROTOTYPE _EXPORT int pups_register_abort_f(const char *, const void *, const char *);

// Show abort functions
_PROTOTYPE _EXPORT void pups_show_abort_f(const FILE *);

// Deregister process abort  function
_PROTOTYPE _EXPORT int pups_deregister_abort_f(const void *);

// PUPS process exit */
_PROTOTYPE _EXPORT int pups_exit(const int);

// Pipe read */
_PROTOTYPE _EXPORT unsigned long int pups_pipe_read(const int, void *, const unsigned long int);

// Block signal (for critical code sequence protection)
_PROTOTYPE _EXPORT int pupsighold(const int, const _BOOLEAN);

// Release blocked signals
_PROTOTYPE _EXPORT int pupsigrelse(const int);

// Block PUPS signals
_PROTOTYPE _EXPORT void pupshold(const int);

// Release all PUPS signals */
_PROTOTYPE _EXPORT void pupsrelse(const int);

// PUPS signal handler
_PROTOTYPE _EXPORT int pups_sighandle(const int, const char *, const void *, const sigset_t *);

// Suspend process until specified signal is recieved
_PROTOTYPE _EXPORT int pups_signalpause(const int);

// Check to see if specified signal is pending
_PUBLIC _BOOLEAN pups_signalpending(const int);

// PUPS signal handler status
_PROTOTYPE _EXPORT void pups_show_sigstatus(const FILE *);

// PUPS signal mask/signal pending status
_PROTOTYPE _EXPORT void pups_show_sigmaskstatus(const FILE *);

// Convert signal name to signal number
_PROTOTYPE _EXPORT int pups_signametosigno(const char *);

// Convert signal number to signal name
_PROTOTYPE _EXPORT char *pups_signotosigname(const int, char *);

// PUPS signal handler extended status
_PROTOTYPE _EXPORT int pups_show_siglstatus(const int, const FILE *);

// PUPS signal handler exit vectorer
_PROTOTYPE _EXPORT int pups_sigvector(const int, const sigjmp_buf *);

// Test to see if data is available on descriptor
_PROTOTYPE _EXPORT int pups_monitor(const int, const int, const int);

// Return date string
_PROTOTYPE _EXPORT void strdate(char *);

// Set up PUPS virtual interval timer
_PROTOTYPE _EXPORT int pups_setvitimer(const char *, const int, const int, const time_t, const char *, const void *);

// Clear PUPS virtual interval timer
_PROTOTYPE _EXPORT int pups_clearvitimer(const char *);

// Restart timer system
_PROTOTYPE _EXPORT _BOOLEAN vitmer_restart(void);

// Show PUPS virtual interval timer
_PROTOTYPE _EXPORT void pups_show_vitimers(const FILE *);

// Set homeostatic file state parameters
_PROTOTYPE _EXPORT int pups_set_fs_hsm_parameters(const int, const int, const char *);

// Homeostatic write check routine
_PUBLIC int pups_write_homeostat(const int, int (*)(int));

// Homeostat for stdio redirected to FIFO's
_PROTOTYPE _EXPORT int pups_default_fd_homeostat(void *, const char *);

// Monitor effective parent process
_PROTOTYPE _EXPORT void pups_default_parent_homeostat(void *, const char *);

// File system space homeostat
_PROTOTYPE _EXPORT _BOOLEAN pups_check_fs_space(const int);

// Attach homeostat to file table entry
_PROTOTYPE _EXPORT int pups_attach_homeostat(const int, const void *);

// Detach homeostat from file table entry
_PROTOTYPE _EXPORT int pups_detach_homeostat(const int);

// VT timer compatable sleep routine
_PROTOTYPE _EXPORT int pups_sleep(const int);

// Microsecond alarm function
_PROTOTYPE _EXPORT int pups_malarm(const unsigned long int);

// Get (named) file lock
_PROTOTYPE _EXPORT _BOOLEAN pups_get_lock(const char *, const int);

// Release (named) file lock
_PROTOTYPE _EXPORT _BOOLEAN pups_release_lock(const char *);

// Get PUPS file descriptor lock
_PROTOTYPE _EXPORT _BOOLEAN pups_get_fd_lock(const int, const int);

// Release PUPS file descriptor lock
_PROTOTYPE _EXPORT int pups_release_fd_lock(const int);

// Get file descriptor (from filename)
_PROTOTYPE _EXPORT int pups_fname2fdes(const char *);

// Get file stream (from filename)
_PROTOTYPE _EXPORT FILE *pups_fname2fstream(const char *);

// Get file table index (from file name)
_PROTOTYPE _EXPORT int pups_fname2index(const char *);

// Get file table index (from file descriptor)
_PROTOTYPE _EXPORT int pups_fdes2index(const int);

// Enable obituary (for child)
_PROTOTYPE _EXPORT int pups_pipestream_obituary_enable(const int);

// Disable obituary (for child)
_PROTOTYPE _EXPORT int pups_pipestream_obituary_disable(const int);

// Extended fork routine
_PROTOTYPE _EXPORT int pups_fork(const _BOOLEAN, const _BOOLEAN);

// Extended creat command
_PROTOTYPE _EXPORT int pups_creat(const char *, const int);

// Extended fchmod which updates ftab
_PROTOTYPE _EXPORT int pups_fchmod(const int, const int);

// Extended strlen command
_PROTOTYPE _EXPORT int pups_strlen(const char *);

// Tracked malloc routine
_PROTOTYPE _EXPORT void *pups_tmalloc(const unsigned long int, const char *, const char *);

// Tracked calloc routine
_PROTOTYPE _EXPORT void *pups_tcalloc(const int, const unsigned long int, const char *, const char *);

// Tracked realloc routine
_PROTOTYPE _EXPORT void *pups_trealloc(void *, const unsigned long int);

// Convert tracked object name to heap address
_PROTOTYPE _EXPORT void *pups_tnametoptr(const char *);

// Convert partial tracked object name to heap address
_PROTOTYPE _EXPORT void *pups_tpartnametoptr(const char *);

// Convert type to heap address
_PROTOTYPE _EXPORT void *pups_ttypetoptr(const char *);

// Tracked object free
_PROTOTYPE _EXPORT void *pups_tfree(const void *);

// Tracked dynamic array allocation
_PROTOTYPE _EXPORT void **pups_taalloc(const pindex_t, const pindex_t, const psize_t, const char *, const char *);

// Tracked dynamic array free
_PROTOTYPE _EXPORT void **pups_tafree(const void **);

// Display tracked object attributes
_PROTOTYPE _EXPORT int pups_tshowobject(const FILE *, const void *);

// Display tracked object table
_PROTOTYPE _EXPORT int pups_tshowobjects(const FILE *, int *);

// Extended fgets function
_PROTOTYPE _EXPORT char *pups_fgets(char *, const unsigned long int, const FILE *);

// PUPS wait function
_PROTOTYPE _EXPORT int pupswait(const _BOOLEAN, int *);

// PUPS waitpid function
_PROTOTYPE _EXPORT int pupswaitpid(const _BOOLEAN, const int, int *);

// Set child name associated with child process
_PROTOTYPE _EXPORT _BOOLEAN pups_set_child_name(const int, const char *);

// Clear a child table slot
_PROTOTYPE _EXPORT int pups_clear_chtab_slot(const _BOOLEAN, const unsigned int);

// Show active children
_PROTOTYPE _EXPORT void pups_show_children(const FILE *);


/*------------------------------------------------------------*/
/* Switch automatic child handling off (for a process we wish */
/* to wait for explicitily)                                   */
/*------------------------------------------------------------*/

_PROTOTYPE _EXPORT void pups_noauto_child(void);

// Re-enable automatic child handling
_PROTOTYPE _EXPORT void pups_auto_child(void);

// Is string a comment string? (first non-space character '#')
_PROTOTYPE _EXPORT _BOOLEAN strcomment(const char *);

// Set all characters in string to NULL ('\0')
_PROTOTYPE _EXPORT void strclr(char *);

// Save a PUPS sigtab entry
_PROTOTYPE _EXPORT void pups_sigtabsave(const int, sigtab_type *);

// Restore a PUPS sigtab entry
_PROTOTYPE _EXPORT void pups_sigtabrestore(const int, const sigtab_type *);

// Turn local echoing off (on tty)
_PROTOTYPE _EXPORT int pups_tty_echoing_off(FILE *, struct termios *);

// Turn local tty echoing on (on tty) */
_PROTOTYPE _EXPORT void pups_tty_echoing_on(FILE *, int, struct termios);

// Execute command (pipeline) locally */
_PROTOTYPE _EXPORT int pups_lexec(char *, char *, int);


/*---------------------------------------------------------------------*/
/* PUPS statkill -- pids which belong to terminated or stopped process */
/* return error                                                        */
/*---------------------------------------------------------------------*/

_PROTOTYPE _EXPORT int pups_statkill(int, int);


// Set file table id tag
_PROTOTYPE _EXPORT int set_ftab_id(const int, const int);

// Get file table index (from id tag)
_PROTOTYPE _EXPORT int pups_get_ftab_index_by_id(const int);

// Get file table index (from file name)
_PROTOTYPE _EXPORT int pups_get_ftab_index_by_name(const char *);

// Clear file table slot
_PROTOTYPE _EXPORT int pups_clear_ftab_slot(const _BOOLEAN, const int);

// Initialise child table
_PROTOTYPE _EXPORT void pups_init_child_table(const int);

// lseek() function which is PUPS signal safe
_PROTOTYPE _EXPORT int pups_lseek(int, unsigned long int, int);

// Get load average
_PROTOTYPE _EXPORT FTYPE pups_get_load_average(const int which_load_average);

// Test to see if fdes is asscoiated with a seekable device
_PROTOTYPE _EXPORT _BOOLEAN pups_is_seekable(const int);

// Relay data to a process slaved via a pair of FIFO's
_PROTOTYPE _EXPORT int pups_fifo_relay(const int, const int, const int);

// Set a read or write link file lock
_PROTOTYPE int pups_get_rdwr_link_file_lock(const int, const int, const char *);

// Get file lock [using link()]
_PROTOTYPE _EXPORT int pups_get_link_file_lock(const unsigned int, const char *);

// Release file lock [set using link()]
_PROTOTYPE _EXPORT int pups_release_link_file_lock(const char *);

// Wait for file to be unlinked before returning to caller
_PROTOTYPE _EXPORT int pups_unlink(const char *);

// Interaction free version of usleep()
_PROTOTYPE _EXPORT int pups_usleep(const unsigned long int);

// Check to see if current file is on CDFS
_PROTOTYPE _EXPORT _BOOLEAN pups_is_on_isofs(const char *);

// Check to see if current file is on CDFS
_PROTOTYPE _EXPORT _BOOLEAN pups_is_on_fisofs(const int);

// Check to see if current file is on NFS
_PROTOTYPE _EXPORT _BOOLEAN pups_is_on_nfs(const char *);

// Check to see if current file is on NFS
_PROTOTYPE _EXPORT _BOOLEAN pups_is_on_fnfs(const int);

// Copy file (by name)
_PROTOTYPE _EXPORT int pups_cpfile(const char *, const char *, int);

// Copy file (by descriptor)
_PROTOTYPE _EXPORT int pups_fcpfile(const int, const int);

// Check for host scanning (ps, top etc)
_PROTOTYPE _EXPORT _BOOLEAN pups_cmd_running(void);

// Do I own the current process?
_PROTOTYPE _EXPORT _BOOLEAN pups_i_own(const int);

// Enable abort restart (on reciept of SIGQUIT)
_PROTOTYPE _EXPORT int pups_restart_enable(void);

// Disable abort restart (on reciept of SIGQUIT)
_PROTOTYPE _EXPORT int pups_restart_disable(void);

// Compute psuedo 32 bit cyclic redundancy checksum
_PUBLIC int pups_crc_p32(const size_t, _BYTE *);

// Generate 64 bit cyclic redundancy checksum
_PUBLIC unsigned long int pups_crc_64(const size_t, _BYTE *);

// Extract CRC signature from file name
_PROTOTYPE _EXPORT unsigned long int pups_get_signature(const char *, const char);

// Sign a file (with its CRC checksum)
_PROTOTYPE _EXPORT int pups_sign(const unsigned long int, const char *, char *, const char);

// PUPS signal safe fprintf function
_PROTOTYPE _EXPORT int pupsfprintf(FILE *, char *, ...);

// Replace command tail tem at specified location
_PROTOTYPE _EXPORT _BOOLEAN pups_replace_cmd_tail_item(const char *, const char *);

// Insert item into the command tail
_PROTOTYPE _EXPORT int pups_insert_cmd_tail_item(const char *, const char *);

// Clear command tail
_PROTOTYPE _EXPORT int pups_clear_cmd_tail(void);

// Handler for backtracking (when segmentation violation occurs)
_PROTOTYPE _EXPORT int pups_segv_backtrack_handler(const int);

// Set a backtrack re-entry point (in process address space)
_PROTOTYPE _EXPORT _BOOLEAN pups_backtrack(const _BOOLEAN);

// Has file been updated?
_PROTOTYPE _EXPORT _BOOLEAN pups_file_updated(const char *);

// Is host reachable?
_PROTOTYPE _EXPORT _BOOLEAN pups_host_reachable(const char *);


// Set (fcntl) file lock (on descriptor)
_PROTOTYPE _EXPORT int pups_flock(const int, const int, const off_t, const off_t, const _BOOLEAN);

// Set (fcntl) file lock (on named file)
_PROTOTYPE _EXPORT int pups_flockfile(const char *, const int, const off_t, const off_t, const _BOOLEAN);

// Display flock locks currently held by caller process
_PROTOTYPE _EXPORT int pups_show_flock_locks(const FILE *);

// Substitute substring
_PROTOTYPE _EXPORT int strsub(const char *, const char *, const char *, char *);


#ifdef PSRP_AUTHENTICATE
// Authenticate user
_PROTOTYPE _EXPORT _BOOLEAN pups_checkuser(char *, char *);

// Authenticate secure service password
_PROTOTYPE _EXPORT _BOOLEAN pups_check_appl_password(char *);

// Authenticate a user
_PROTOTYPE _EXPORT _BOOLEAN pups_getpass(char *);

// Check that application has password set
_PROTOTYPE _EXPORT _BOOLEAN pups_check_pass_set(void);
#endif /* PSRP_ATHENTICATE */


// Get file type
_PROTOTYPE _EXPORT char *pups_get_file_type(const char *);

/* Convert string to lower case
_PROTOTYPE _EXPORT int downcase(char *);

// Convert string to upper case
_PROTOTYPE _EXPORT int upcase(char *);

// Enigma encrypt a string
_PROTOTYPE _EXPORT int ecryptstr(const int, const _BOOLEAN, const char *, char *);

// Get exporting NFS host name (from file path)
_PROTOTYPE _EXPORT int pups_get_nfs_host(const char *pathname, const char *host);

// Reflect endian ness of float
_PROTOTYPE _EXPORT float fconv(const FTYPE);

// Reflect endian ness of int
_PROTOTYPE _EXPORT int iconv(const int);

// Reflect endian ness of short
_PROTOTYPE _EXPORT short int sconv(const short int);

// Reflect endian ness of long
_PROTOTYPE _EXPORT long int lconv(const long int);

// Encrypted formatted print
_PROTOTYPE _EXPORT int efprintf(FILE *, const char *, ...);

// Encrypted formatted scan
_PROTOTYPE _EXPORT int efscanf(FILE *, const char *, ...);

// Encrypted formatted string scan
_PROTOTYPE _EXPORT int esscanf(char *cipher, const char *format, ...);

// Encrypted fputs
_PROTOTYPE _EXPORT int efputs(const char *, const FILE *);

// Encrypted fgets
_PROTOTYPE _EXPORT char *efgets(char *, const unsigned long int, const FILE *);

// Mark context of non local goto valid
_PROTOTYPE _EXPORT int pups_set_jump_vector(void);

// Mark context of non local goto invalid
_PROTOTYPE _EXPORT int pups_reset_jump_vector(void);

// TRUE if linux version (sub) string matches key
_PROTOTYPE _EXPORT _BOOLEAN pups_is_linuxversion(const char *);

// How many CPUS's does host have?
_PROTOTYPE _EXPORT int pups_cpus(FTYPE *, char *);

// Get resource loading
_PROTOTYPE _PUBLIC int pups_get_resource_loading(const _BOOLEAN, FTYPE *, int *);

// Have we got resources required?
_PROTOTYPE _EXPORT _BOOLEAN pups_have_resources(FTYPE, int);

// Transfer a file to/from a remote host
_PROTOTYPE _EXPORT int pups_scp(const char *, const char *, const char *);

// Is process PUPS/P3 aware?
_PROTOTYPE _EXPORT _BOOLEAN pups_aware(const int);

// Open extended pipestream
_PROTOTYPE _EXPORT int pups_popen(const char *, const char *, const int, int *);

// Open buffered extended pipestream
_PROTOTYPE _EXPORT FILE *pups_fpopen(const char *, const char *, const char *, int *);

// Close extended pipestream
_PROTOTYPE _EXPORT int pups_pclose(const int, const int);

// Close buffered extended pipestream
_PROTOTYPE _EXPORT int pups_fpclose(const FILE *, const int);

// Test to see if process is foreground
_PROTOTYPE _EXPORT _BOOLEAN pups_is_foreground(void);

// Test to see if process is background
_PROTOTYPE _EXPORT _BOOLEAN pups_is_background(void);

// Release all link file locks
_PROTOTYPE _EXPORT int pups_release_all_link_file_locks(void);

// Is command installed on system
_PROTOTYPE _EXPORT _BOOLEAN pups_cmd_installed(const char *);

// Get current CPU utilisation on host
_PROTOTYPE _EXPORT FTYPE pups_cpu_utilisation(void);

// Get prefix from string
_PROTOTYPE _EXPORT int pups_prefix(const char, const char *, char *);

// Get suffix from string
_PROTOTYPE _EXPORT int pups_suffix(const char, const char *, char *);

// Get file size
_PROTOTYPE _EXPORT unsigned long int pups_get_fsize(const char *);

// Descriptor associated with a pipe
_PROTOTYPE _EXPORT int isapipe(const int);

// Descriptor associated with a file
_PROTOTYPE _EXPORT int isafile(const int);

// Descriptor associated with a socket
_PROTOTYPE _EXPORT int isasock(const int);

// Descriptor associated with a (data) conduit (pipe, file or socket)
_PROTOTYPE _EXPORT int isaconduit(const int);

// Get system information for current host
_PROTOTYPE _EXPORT int pups_sysinfo(char *, char *, char *, char *);

// Am I running in virtual enviroment? 
_PROTOTYPE _EXPORT _BOOLEAN pups_os_is_virtual(char *);

// Is character first one in string? 
_PROTOTYPE _EXPORT _BOOLEAN ch_is_first(const char *, const char); 

// Is character last one in string? 
_PROTOTYPE _EXPORT _BOOLEAN ch_is_last(const char *, const char); 

// Is architecture big endian?
_PROTOTYPE _EXPORT _BOOLEAN pups_bigendian(void);


#ifdef CRIU_SUPPORT
// Enable (Criu) state saving
_PROTOTYPE _EXPORT int pups_ssave_enable(void);

// Disable (Criu) state saving
_PROTOTYPE _EXPORT int pups_ssave_disable(void);
#endif /* CRIU_SUPPORT */

// Set (round robin) scheduling priority
_PROTOTYPE _EXPORT int pups_set_rt_sched(const int);

// Switch to standard (time sharing) scheduler
_PROTOTYPE _EXPORT int pups_set_tslice_sched(void);

// Copy file
_PROTOTYPE _EXPORT int pups_copy_file(const int, const char *, const char *);


#ifdef BSD_FUNCTION_SUPPORT
// Open BSD strlcat function
_PROTOTYPE _EXPORT size_t strlcat(char *, const char *, size_t dsize);

// Open BSD strlcpy functiom
_PROTOTYPE _EXPORT size_t strlcpy(char *, const char *, size_t dsize);
#endif /* BSD_FUNCTION_SUPPORT */

// Can we run command? */
_PROTOTYPE _EXPORT _BOOLEAN pups_can_run_command(const char *);

// Enable (memory) residency
_PROTOTYPE _EXPORT void pups_enable_resident(void);

// Get size of directory in bytes
_PROTOTYPE _EXPORT off_t pups_dsize(const char *);

// Show software watchdog status
_PROTOTYPE _EXPORT int pups_show_softdogstatus(const FILE *);

// Start software watchdog
_PROTOTYPE _EXPORT int pups_start_softdog(const unsigned int);

// Stop software watchdog
_PROTOTYPE _EXPORT int pups_stop_softdog(void);


#ifdef _CPLUSPLUS
#   undef  _EXPORT
#   define _EXPORT extern "C"
#else
#   undef  _EXPORT
#   define _EXPORT extern
#endif /* _CPLUSPLUS */

#endif /* UTILS_H */
