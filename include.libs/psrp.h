/*-------------------------------------------------------------------------
    Purpose: header for PSRP support library.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 5.05
    Dated:   12th July 2023 
    E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------------*/


#ifndef PSRP_H
#define PSRP_H


/*------------------*/
/* Keep c2man happy */
/*------------------*/

#include <c2man.h>

#ifndef __C2MAN__
#include <stdio.h>

#ifdef THREAD_SUPPORT
#include <pthread.h>
#endif /* THREAD_SUPPORT */

#endif /* __C2MAN__ */


/***********/
/* Version */
/***********/

#define PSRPLIB_VERSION    "5.05"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE                 2048


/*-------------------------------*/
/* Floating point representation */
/*-------------------------------*/

#include <ftype.h>


/*-----------------*/
/* PSRP interfaces */
/*-----------------*/

                             /*-----------------------*/
#define HAVE_PSRP  1         /* Process is PSRP aware */
                             /*-----------------------*/

                             /*---------------------------------*/
#define UNIX_FACE (1 << 0)   /* PSRP command line options       */
#define PSRP_FACE (1 << 1)   /* PSRP requests via PSRP protocol */
#define MAIL_FACE (1 << 2)   /* PSRP requests via SMTP/MIME     */
#define WEB_FACE  (1 << 4)   /* PSRP requests via HTTP          */
                             /*---------------------------------*/


/*-----------------------------------*/
/* Defines used by crontab subsystem */
/*-----------------------------------*/

#define MAX_CRON_SLOTS             16


/*----------------------*/
/* fcntl non-block flag */
/*----------------------*/

#ifndef O_BLOCK
#define O_BLOCK 0
#endif /* O_BLOCK */


/*------------------------------------------------*/
/* Default period for psrp client repeat commands */
/*------------------------------------------------*/

#define PSRP_RPT_DEFAULT                1


/*-----------------------------------------------------*/
/* Segmentatation delay for migrating or forked server */
/* instance (seconds)                                  */
/*-----------------------------------------------------*/

#define SEGMENTATION_DELAY              5


/*-------------------------------------------*/
/* Max PSRP servers in cluster               */
/*-------------------------------------------*/

#define MAX_PSRP_SERVERS                1024


/*-------------------------------------------*/
/* Max PSRP channels on host                 */
/*-------------------------------------------*/

#define MAX_PSRP_CHANNELS               1024


/*-------------------------------------------*/
/* Number of clients on PSRP server          */
/*-------------------------------------------*/

#define MAX_CLIENTS                     32 
#define ENOCH                           (-9999)


/*-------------------------------------------*/
/* Number of channels to slaved PSRP clients */
/*-------------------------------------------*/

#define PSRP_MAX_SIC_CHANNELS           8 


/*---------------------------------------------*/
/* Types of channel supported by SIC subsystem */
/*---------------------------------------------*/

#define SIC_FIFO                        1
#define SIC_SSH_TUNNEL                  2


/*--------------------------------------*/
/* PSRP class file end of record marker */
/*--------------------------------------*/

#define PSRP_EOR_MARKER "!#### end_of_PSRP_record\n"


/*----------------------------*/
/* PSRP active channel states */
/*----------------------------*/

#define PSRP_NO_EVENT                  0
#define PSRP_CLIENT_TERMINATED         (1 << 0)
#define PSRP_IPC_LOST                  (1 << 1) 
#define PSRP_OPC_LOST                  (1 << 2) 
#define PSRP_LOCK_LOST                 (1 << 3) 
#define PSRP_RECURSIVE                 (1 << 4) 
#define PSRP_NOTRAIL                   (1 << 5) 


/*---------------------*/
/* PSRP system defines */
/*---------------------*/

#define PSRP_PROTOCOL_VERSION          9.02 
#define PSRP_BAG_TABLE_SIZE            128 
#define PSRP_DISPATCH_TABLE_SIZE       128 
#define PSRP_ALLOCATION_QUANTUM        128 
#define PSRP_OBJECT_UNKNOWN            (-9999)


/*---------------------------*/
/* PSRP object export states */
/*---------------------------*/

#define PSRP_PUBLIC                    0
#define PSRP_PRIVATE                   1


/*----------------------------------------*/
/* PSRP object export class update states */
/*----------------------------------------*/

#define PSRP_ADD_CLASS                 0
#define PSRP_REMOVE_CLASS              1


/*------------------------------------*/
/* PSRP library function return codes */
/*------------------------------------*/

#define PSRP_ERROR                     (-1)
#define PSRP_OK                        0   
#define PSRP_MORE                      (1 << 0)
#define PSRP_EOT                       (1 << 1) 
#define PSRP_EOG                       (1 << 2) 
#define PSRP_DISPATCH_ERROR            (-1)
#define PSRP_ITEM_LOCKED               (-2)
#define PSRP_TAG_ERROR                 (-3)
#define PSRP_LIST_ITEM_FOUND           (-4)
#define PSRP_EACCESS                   (-5) 
#define PSRP_CONNECT_ERROR             (-6)
#define PSRP_DUPLICATE_PROCESS_NAME    (-7)
#define PSRP_STDIO_ERROR               (-8)
#define PSRP_TRANSPORT_ERROR           (-9)
#define PSRP_TERMINATED                (-10)
#define PSRP_STDIO_ASSIGNED            (1 << 3) 
#define PSRP_RECONNECT_STDIO           (1 << 4) 
#define PSRP_REDIRECT_STDIO            (1 << 5) 


/*-----------------------------------------------------------------*/
/* Object types and states that the PSRP handler has to know about */
/*-----------------------------------------------------------------*/

#define PSRP_NONE                      0
#define PSRP_STATIC_FUNCTION           (1 << 0)
#define PSRP_DYNAMIC_FUNCTION          (1 << 1) 
#define PSRP_STATIC_DATABAG            (1 << 2) 
#define PSRP_DYNAMIC_DATABAG           (1 << 3)  
#define PSRP_STATUS_ONLY               (1 << 4) 
#define PSRP_STATICS_IMMORTAL          (1 << 5) 
#define PSRP_HOMEOSTATIC_STREAMS       (1 << 6) 
#define PSRP_PERSISTENT_HEAP           (1 << 7) 


/*-----------------------------------------------------------------*/
/* Error code for no remote process for PSRP pipestream operations */
/*-----------------------------------------------------------------*/

#define ENORPROC                       768


#ifdef MAIL_SUPPORT
/*------------------------------*/
/* Types used by mail subsystem */
/*------------------------------*/

typedef struct {   char     type[SSIZE];     // MIME part type
                   char     path[SSIZE];     // Filepath of part
               } mc_type;

#endif /* MAIL_SUPPORT */


/*------------------------------*/
/* Types used by cron subsystem */
/*------------------------------*/

typedef struct {   int      from;            // Start of scheduled op.
                   int      to;              // End of scheduled op.
                   _BOOLEAN overnight;       // Op extends over midnight
                   void     (*func)(int);    // Payload function
                   char     fname[SSIZE];    // Name of payload function
                   char     fromdate[SSIZE]; // Date string for start
                   char     todate[SSIZE];   // Date string for stop
               } psrp_crontab_type;


/*-----------------------------------*/
/* Types used by PSRP handler system */
/*-----------------------------------*/

typedef struct {    int  aliases_allocated;  // Allocated alias slots
		    int  aliases;            // Number of aliases
		    char **object_tag;       // Names of PSRP object
                    char *object_f_name;     // Object resource file
                    int  hid;                // Heap descriptor
		    int  object_type;        // Type of PSRP object
		    int  object_size;        // Size of PSRP object
		    int  object_state;       // State of PSRP object
		    void *object_handle;     // Handle of PSRP object
	       } psrp_object_type;


typedef struct {    char       *in_name;     // Name of in stream
		    char      *out_name;     // Name of out stream
		    char     *host_name;     // Host for PSRP client
                    char      *ssh_port;     // Host ssh port
		    char           *pen;     // PSRP client PEN
		    FILE     *in_stream;     // In stream
		    FILE    *out_stream;     // Out stream
		    int           index;     // Index in channel table
                    int             scp;     // Slaved client PID
                    int            type;     // Channel type
                    _BOOLEAN     remote;     // TRUE if SIC remote
		} psrp_channel_type;


/*-------------------------------------------------------------*/
/* Variables exported by the multithreaded DLL support library */
/*-------------------------------------------------------------*/

#ifdef __NOT_LIB_SOURCE__


/*-------------------------------------------*/
/* Variables exported by PSRP handler system */
/*-------------------------------------------*/

_EXPORT FILE     *psrp_in;                             // PSRP input channel (from client)
_EXPORT FILE     *psrp_out;                            // PSRP output channel (to client)
_EXPORT char     channel_name_in[];                    // PSRP input channel name
_EXPORT char     channel_name_out[];                   // PSRP ouput channel name
_EXPORT int      psrp_client_pid[MAX_CLIENTS];         // PID of PSRP client process
_EXPORT int      n_clients;                            // Number of connected clients
_EXPORT int      c_client;                             // Currently active client
_EXPORT int      psrp_seg_cnt;                         // Server segment counter
_EXPORT int      psrp_instances;                       // Instance counter
_EXPORT _BOOLEAN psrp_child_instance;                  // TRUE if child instance 
_EXPORT _BOOLEAN psrp_mode;                            // TRUE if server in psrp mode
_EXPORT _BOOLEAN in_psrp_new_segment;                  // TRUE if about to segment
_EXPORT _BOOLEAN in_psrp_handler;                      // TRUE if in PSRP handler
_EXPORT _BOOLEAN in_chan_handler;                      // TRUE if in CHAN handler
_EXPORT          psrp_channel_type *psrp_current_sic;  // Current SIC channel
_EXPORT          psrp_object_type  *psrp_object_list;  // List of attached PSRP objects

#else
#   undef  _EXPORT
#   define _EXPORT
#endif /* __NOT_LIB_SOURCE__ */


/*------------------------*/
/* PSRP handler functions */
/*------------------------*/

// Register client side abort callback
_PROTOTYPE _EXPORT int psrp_register_on_abort_callback_f(const char *, const void *);

// Deregister client side abort callback
_PROTOTYPE _EXPORT void psrp_deregister_on_abort_callback_f(void);

// Display client side abort callback function details
_PROTOTYPE _EXPORT int psrp_show_on_abort_callback_f(const FILE *);

// Callback function for client side abort
_PROTOTYPE _EXPORT _BOOLEAN (*on_abort_callback_f)(void);

// Ignore PSRP requests
_PROTOTYPE _EXPORT void psrp_ignore_requests(void);

// Accept PSRP requests
_PROTOTYPE _EXPORT void psrp_accept_requests(void);

// Initialise PSRP handler
_PROTOTYPE _EXPORT void psrp_init(const int, const int (*)(const int, const char *[]));

// Attach static function to PSRP handler
_PROTOTYPE _EXPORT int psrp_attach_static_function(const char *, const void *);

// Attach static databag to PSRP handler
_PROTOTYPE _EXPORT int psrp_attach_static_databag(const char *, const unsigned long int, const _BYTE *);

// Attach static databag to PSRP handler
_PROTOTYPE _EXPORT int psrp_attach_dynamic_databag(const _BOOLEAN, const char *, const char *, const int);

// Overlay PSRP server process with new command
_PROTOTYPE _EXPORT int psrp_overlay_server_process(const _BOOLEAN, const char *, const char *);


#ifdef SUPPORT_DLL

// Attach dynamic function to PSRP handler
_PROTOTYPE _EXPORT int psrp_attach_dynamic_function(const _BOOLEAN, const char *, const char *);

#endif /* SUPPORT_DLL */


#ifdef PERSISTENT_HEAP_SUPPORT

// Attach a persistent heap
_PROTOTYPE _EXPORT int  psrp_attach_persistent_heap(const _BOOLEAN, const char *, const char *, const int);

// Show persistent heaps mapped into process address space
_PROTOTYPE _EXPORT void psrp_show_persistent_heaps(const FILE *);

#endif /* PERSISTENT_HEAP_SUPPORT */


// Detach named object from PSRP handler
_PROTOTYPE _EXPORT int psrp_detach_object_by_name(const char *);

// Detach object with known handle from PSRP handler
_PROTOTYPE _EXPORT int psrp_detach_object_by_handle(const void *);

// Show attached functions
_PROTOTYPE _EXPORT void psrp_show_object_list(void);

// Handle PSRP interrupt function request from remote client
_PROTOTYPE _EXPORT int int_handler(int);

// Load default dispatch table (at server startup)
_PROTOTYPE _EXPORT _BOOLEAN psrp_load_default_dispatch_table(void);

// Load a dispatch table class (merging functions on server)
_PROTOTYPE _EXPORT int psrp_load_dispatch_table(const _BOOLEAN, const char *);

// Save a dispatch table class
_PROTOTYPE _EXPORT int psrp_save_dispatch_table(const char *);

// Handle broken channel
_PROTOTYPE _EXPORT int psrp_chbrk_handler(const unsigned int, const int);

// Initialise PSRP object function command tail decoder
_PROTOTYPE _EXPORT _BOOLEAN psrp_std_init(const int, const char *[]);

// Check for junk on PSRP object function comamnd lines
_PROTOTYPE _EXPORT _BOOLEAN psrp_t_arg_errs(const int, const _BOOLEAN *, const char *[]);

// send error message to client
_PROTOTYPE _EXPORT void psrp_error(const char *);

// Look up item in dispatch table by name
_PROTOTYPE _EXPORT int lookup_psrp_object_by_name(const char *);

// Look up item in dispatch table by handle
_PROTOTYPE _EXPORT int lookup_psrp_object_by_handle(const void *);

// Get handle of PSRP object from name
_PROTOTYPE _EXPORT void *psrp_get_handle_from_name(const char *);

// Get name of PSRP object from handle
_PROTOTYPE _EXPORT int psrp_get_name_from_handle(const void *, char *);

// Get type of psrp object from name
_PROTOTYPE _EXPORT int psrp_get_type_from_name(const char *);

// Get type of psrp object from name
_PROTOTYPE _EXPORT int psrp_get_type_from_handle(const void *);

// Remove communication channel
_PROTOTYPE _EXPORT void psrp_exit(void);

// Resolve pid from process name
_PROTOTYPE _EXPORT int psrp_pname_to_pid(const char *);

// Resolve process name from process pid
_PROTOTYPE _EXPORT _BOOLEAN psrp_pid_to_pname(const int, char *);

// Resolve pid from PSRP channel name
_PROTOTYPE _EXPORT int psrp_channelname_to_pid(const char *, char *, char *);

// Resolve process name from PSRP channel name
_PROTOTYPE _EXPORT _BOOLEAN psrp_pid_to_channelname(const char *, const int, char *, char *);

// Set current SIC
_PROTOTYPE _EXPORT int psrp_set_current_sic(const psrp_channel_type *);

// Unset current SIC
_PROTOTYPE _EXPORT void psrp_unset_current_sic(void);

// Create a slaved psrp interaction client
#ifdef SSH_SUPPORT
_PROTOTYPE _EXPORT psrp_channel_type *psrp_create_slaved_interaction_client(const char *, const char *, const char *);
#else
_PROTOTYPE _EXPORT psrp_channel_type *psrp_create_slaved_interaction_client(const char *);
#endif /* SSH_SUPPORT */

// Carry out PSRP transaction via slaved client
_PROTOTYPE _EXPORT char *psrp_slaved_client_transaction(const _BOOLEAN, const psrp_channel_type *, const char *);

// Destroy psrp slaved interaction client
_PROTOTYPE _EXPORT psrp_channel_type *psrp_destroy_slaved_interaction_client(const psrp_channel_type *, const _BOOLEAN);

// Assign descriptors for a PSRP task function
_PROTOTYPE _EXPORT int psrp_assign_stdio(const FILE *, const int *, const char *[], int *, int *, int *);

// Assign streams for a PSRP task function
_PROTOTYPE _EXPORT int psrp_assign_fstdio(const FILE *, const int *, const char *[], FILE *, FILE *, FILE *);

// Wait for data on nominated descriptor
_PROTOTYPE _EXPORT int psrp_wait_for_data(const int);

// Provide an alias for a bound PSRP object
_PROTOTYPE _EXPORT int psrp_alias(const char *, const char *);

// Remove an alias for a bound PSRP object
_PROTOTYPE _EXPORT int psrp_unalias(const char *, const char *);

// Show alaises on object
_PROTOTYPE _EXPORT int psrp_show_aliases(const char *);

// Duplicate current process
_PROTOTYPE _EXPORT int psrp_duplicate_instance(_BOOLEAN, char *);

// Free resources allocated to tag index 
_PROTOTYPE _EXPORT int psrp_ifree_tag_list(const unsigned int);

// Search tag list for the first free slot
_PROTOTYPE _EXPORT int psrp_get_tag_index(const unsigned int);

// Search tag list
_PROTOTYPE _EXPORT int psrp_isearch_tag_list(const char *, const unsigned int);

// Get the index of the next available slot in the dispatch table
_PROTOTYPE _EXPORT int psrp_get_action_slot_index(void);

// Destroy PSRP object
_PROTOTYPE _EXPORT void psrp_destroy_object(const unsigned int);

// Get state of PSRP object
_PROTOTYPE _EXPORT int psrp_ostate(const char *);

// Check whether we have client locked on named channel
_PROTOTYPE _EXPORT _BOOLEAN psrp_channel_locked(const FILE *, const char *);

// Show open slaved interation client channels
_PROTOTYPE _EXPORT void psrp_show_open_sics(const FILE *);

// Empty a FIFO
_PROTOTYPE _EXPORT void empty_fifo(const int);

// Handler for client side of PSRP interrupt mechanism
_PROTOTYPE _EXPORT int psrp_client_int_handler(int);

// Start a new server segment
#ifdef SSH_SUPPORT
_PROTOTYPE _EXPORT int psrp_new_segment(const char *, const char *, const char *, const char *);
#else
_PROTOTYPE _EXPORT int psrp_new_segment(const char *, const char *, const char *);
#endif /* SSH_SUPPORT */

// Generate duplicate instance a PSRP server
_PROTOTYPE _EXPORT int psrp_new_instance(const _BOOLEAN, const char *, const char *);

// Find host running specified server
_PROTOTYPE _EXPORT int psrp_get_server_host(FILE *, char *, char [][SSIZE]);

// Prod a remote host (to see if it is alive)
_PROTOTYPE _EXPORT int psrp_prod_host(FILE *, char *, char *, char *, int, int *);

// Find PSRP dispatch table slot entry
_PROTOTYPE _EXPORT int psrp_find_action_slot_index(const char *);

// Send request over slaved client channel (peer-to-peer)
_PROTOTYPE _EXPORT int psrp_write_sic(const psrp_channel_type *, const char *);

// Read reply over slaved client channel (peer-to-peer)
_PROTOTYPE _EXPORT int psrp_read_sic(const psrp_channel_type *, char *);

// Send abort over slaved client channel (perr-to-peer)
_PROTOTYPE _EXPORT void psrp_int_sic(const psrp_channel_type *);

// Reset dispatch table
_PROTOTYPE _EXPORT void psrp_reset_dispatch_table(void);

// Install PSRP channel exit function
_PROTOTYPE _EXPORT int psrp_set_client_exitf(const unsigned int , const char *, int (*)(int));

// Deinstall PSRP channel exit function
_PROTOTYPE _EXPORT int psrp_reset_client_exitf(const unsigned int);

// Schedule a crontab operation
_PROTOTYPE _EXPORT int psrp_crontab_schedule(const char *, const char *, const char *, const void *);

// Unschedule a crontab operation
_PROTOTYPE _EXPORT int psrp_crontab_unschedule(const unsigned int);

// Initialise crontab
_PROTOTYPE _EXPORT void psrp_crontab_init(void);

// Display crontab
_PROTOTYPE _EXPORT int psrp_show_crontab(const FILE *);

// Set dispatch function error/status code
_PROTOTYPE _EXPORT void psrp_set_c_code(const char *);

// Fork a PUPS (server) process
_PROTOTYPE _EXPORT int psrp_fork(const char *, const _BOOLEAN);

// Herpes algorithm
_PROTOTYPE _EXPORT int herpes_homeostat(void *, char *);

// Shy algorithm
_PROTOTYPE _EXPORT int shy_homeostat(void *, char *);

// Read trail data from migrated PSRP O/P channel
_PROTOTYPE _EXPORT _BOOLEAN psrp_read_trailfile(const char *, char *, char *, char *);


#ifdef MAIL_SUPPORT

// Store components of a MIME message
_PROTOTYPE _EXPORT int psrp_mime_storeparts(const char *, const char *);

// Extract PSRP requests from text segment of MIME message
_PROTOTYPE _EXPORT int psrp_msg2requestlist(const char *, const char *);

// Delete compomnents of MIME message
_PROTOTYPE _EXPORT int psrp_mime_delete(const char *);

// Send single file as MIME message 
_PROTOTYPE _EXPORT void psrp_mime_mail(const char *, const char *, const char *);

// Send files as multipart MIME message 
_PROTOTYPE _EXPORT int psrp_mime_encapsulate(const char *, const char *, const char *, const int, const mc_type *);

// Get replyto address from message header
_PROTOTYPE _EXPORT int psrp_get_replyto(const char *, char *);

// Mail homeostat (allow PSRP process to read its mailbox and reply to it mail!)
_PROTOTYPE _EXPORT int psrp_mail_homeostat(void *, const char *);

#endif /* MAIL_SUPPORT */

// Handler for SIGCONT */
_PROTOTYPE _EXPORT int psrp_cont_handler(const int);

// Rename an existing PSRP channel
_PROTOTYPE _EXPORT int psrp_rename_channel(const char *);

// Start PSRP server on remote host 
_PROTOTYPE _EXPORT int psrp_remote_start(const char *, const _BOOLEAN, const int, const char *[]);

// Send list of requests to remote peer (via enslaved PSRP client)
#ifdef SSH_SUPPORT
_PROTOTYPE _EXPORT char **psrp_process_sic_transaction_list(const char *, const char *, const int, const char *);
#else
_PROTOTYPE _EXPORT char **psrp_process_sic_transaction_list(const char *, const int, const char *);
#endif /* SSH_SUPPORT */

// Set up (exec) ennviroment
_PROTOTYPE _EXPORT int psrp_exec_env(const char *);

// Enable PSRP abort flag 
_PROTOTYPE _EXPORT int psrp_enable_abrtflag(void);

// Disable PSRP abort flag 
_PROTOTYPE _EXPORT int psrp_disable_abrtflag(void);

// Rest PSRP abort flag 
_PROTOTYPE _EXPORT int psrp_reset_abrtflag(void);

// Get PSRP abort flag 
_PROTOTYPE _EXPORT _BOOLEAN psrp_get_abrtflag(void);

// Enable/disable PSRP client interrupt (via SIGABRT)
_PROTOTYPE _EXPORT int psrp_critical(const _BOOLEAN);

// Block/unblock client interrupt (via SIGABRT) 
_PROTOTYPE _EXPORT int psrp_client_block(const _BOOLEAN);

// Is PEN (process execution name) unique? 
_PROTOTYPE _EXPORT void psrp_pen_unique(void);


#ifdef _CPLUSPLUS
#   undef  _EXPORT
#   define _EXPORT extern "C"
#else
#   undef  _EXPORT                                                              
#   define _EXPORT extern                                   
#endif /* _CPLUSPLUS */                                                                         

#endif  /* PSRP_H */

