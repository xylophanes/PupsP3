/*-------------------------------------------------------------------------
    Purpose: header for PSRP support library.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 7.04 
    Dated:   29th December 2024 
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

#define PSRPLIB_VERSION      "7.04"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE                512 


/*-------------------------------*/
/* Floating point representation */
/*-------------------------------*/

#include <ftype.h>


/*-----------------*/
/* PSRP  interface */
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

#define PSRP_PROTOCOL_VERSION          10.00 
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

typedef struct {   char     type[SSIZE];        // MIME part type
                   char     path[SSIZE];        // Filepath of part
               } mc_type;

#endif /* MAIL_SUPPORT */


/*------------------------------*/
/* Types used by cron subsystem */
/*------------------------------*/

typedef struct {   time_t        from;                // Start of scheduled op.
                   time_t         to;                 // End of scheduled op.
                   _BOOLEAN       overnight;          // Op extends over midnight
                   void           (*func)(int32_t);   // Payload function
                   char           fname[SSIZE];       // Name of payload function
                   char           fromdate[SSIZE];    // Date string for start
                   char           todate[SSIZE];      // Date string for stop
               } psrp_crontab_type;


/*-----------------------------------*/
/* Types used by PSRP handler system */
/*-----------------------------------*/

typedef struct {    uint32_t       aliases_allocated;  // Allocated alias slots
		    uint32_t       aliases;            // Number of aliases
		    char           **object_tag;       // Names of PSRP object
                    char           *object_f_name;     // Object resource file
                    uint32_t       hid;                // Heap descriptor
		    int32_t        object_type;        // Type of PSRP object
		    size_t         object_size;        // Size of PSRP object
		    int32_t        object_state;       // State of PSRP object
		    void           *object_handle;     // Handle of PSRP object
	       } psrp_object_type;


typedef struct {    char           in_name[SSIZE];     // Name of in stream
		    char           out_name[SSIZE];    // Name of out stream
		    char           host_name[SSIZE];   // Host for PSRP client
                    char           ssh_port[SSIZE];    // Host ssh port
		    char           pen[SSIZE];         // PSRP client PEN
		    FILE           *in_stream;         // In stream
		    FILE           *out_stream;        // Out stream
		    uint32_t       index;              // Index in channel table
                    pid_t          scp;                // Slaved client PID
                    int32_t        type;               // Channel type
                    _BOOLEAN       remote;             // TRUE if SIC remote
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
_EXPORT  int32_t psrp_client_pid[MAX_CLIENTS];         // PID of PSRP client process
_EXPORT  int32_t n_clients;                            // Number of connected clients
_EXPORT  int32_t c_client;                             // Currently active client
_EXPORT  int32_t psrp_seg_cnt;                         // Server segment counter
_EXPORT  int32_t psrp_instances;                       // Instance counter
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

// Register client side abort callback [root thread]
_PROTOTYPE _EXPORT int32_t psrp_register_on_abort_callback_f(const char *, const void *);

// Deregister client side abort callback [root thread]
_PROTOTYPE _EXPORT void psrp_deregister_on_abort_callback_f(void);

// Display client side abort callback function details [root thread]
_PROTOTYPE _EXPORT int32_t psrp_show_on_abort_callback_f(const FILE *);

// Callback function for client side abort
_PROTOTYPE _EXPORT _BOOLEAN (*on_abort_callback_f)(void);

// Ignore PSRP requests [root thread]
_PROTOTYPE _EXPORT void psrp_ignore_requests(void);

// Accept PSRP requests [root thread]
_PROTOTYPE _EXPORT void psrp_accept_requests(void);

// Initialise PSRP handler [root thread]
_PROTOTYPE _EXPORT void psrp_init(const int32_t, const int32_t (*)(const int32_t, const char *[]));

// Attach static function to PSRP handler [root thread]
_PROTOTYPE _EXPORT int32_t psrp_attach_static_function(const char *, const void *);

// Attach static databag to PSRP handler [root thread]
_PROTOTYPE _EXPORT int32_t psrp_attach_static_databag(const char *, const uint64_t, const _BYTE *);

// Attach dynamic databag to PSRP handler [root thread]
_PROTOTYPE _EXPORT int32_t psrp_attach_dynamic_databag(const _BOOLEAN, const char *, const char *, const int32_t);

// Overlay PSRP server process with specified command [root thread]
_PROTOTYPE _EXPORT int32_t psrp_overlay_server_process(const _BOOLEAN, const char *, const char *);


#ifdef SUPPORT_DLL
// Attach dynamic function to PSRP handler [root thread]
_PROTOTYPE _EXPORT int32_t psrp_attach_dynamic_function(const _BOOLEAN, const char *, const char *);
#endif /* SUPPORT_DLL */


#ifdef PERSISTENT_HEAP_SUPPORT
// Attach a persistent heap [root thread]
_PROTOTYPE _EXPORT int32_t  psrp_attach_persistent_heap(const _BOOLEAN, const char *, const char *, const int32_t);

// Show persistent heaps mapped  into process address space [root thread]
_PROTOTYPE _EXPORT void psrp_show_persistent_heaps(const FILE *);
#endif /* PERSISTENT_HEAP_SUPPORT */


// Detach object from PSRP handler list by name [root thread]
_PROTOTYPE _EXPORT int32_t psrp_detach_object_by_name(const char *);

// Detach object from PSRP handler list by handle [root thread]
_PROTOTYPE _EXPORT int32_t psrp_detach_object_by_handle(const void *);

// Show objects in PSRP dispatch table and  PSRP server status information [root thread]
_PROTOTYPE _EXPORT void psrp_show_object_list(void);

// Handle PSRP interrupt function request from remote client
_PROTOTYPE _EXPORT int32_t  int_handler(int);

// Load default dispatch table (at server startup) [root thead]
_PROTOTYPE _EXPORT _BOOLEAN psrp_load_default_dispatch_table(void);

// Load a dispatch table class (merging functions on server) [root thread]
_PROTOTYPE _EXPORT int32_t psrp_load_dispatch_table(const _BOOLEAN, const char *);

// Save a dispatch table  [root thread]
_PROTOTYPE _EXPORT int32_t psrp_save_dispatch_table(const char *);

// Handle broken channel
_PROTOTYPE _EXPORT int32_t psrp_chbrk_handler(const uint32_t, const uint32_t);

// Initialise PSRP environment [root thread]
_PROTOTYPE _EXPORT _BOOLEAN psrp_std_init(const int32_t, const char *[]);

// PSRP handler command line parser error handler [root thread]
_PROTOTYPE _EXPORT _BOOLEAN psrp_t_arg_errs(const int32_t, const _BOOLEAN *, const char *[]);

// send error message to client [root thread]
_PROTOTYPE _EXPORT void psrp_error(const char *);

// Get PSRP handler list entry by name [root thread]
_PROTOTYPE _EXPORT int32_t lookup_psrp_object_by_name(const char *);

// Get PSRP handler list entry by handle
_PROTOTYPE _EXPORT int32_t lookup_psrp_object_by_handle(const void *);

// Get handle of PSRP object from name [root thread]
_PROTOTYPE _EXPORT void *psrp_get_handle_from_name(const char *);

// Get name of PSRP object from handle [root thread]
_PROTOTYPE _EXPORT int32_t psrp_get_name_from_handle(const void *, char *);

// Get type of psrp object from name [root thread]
_PROTOTYPE _EXPORT int32_t psrp_get_type_from_name(const char *);

// Get type of psrp object from handle [root thread]
_PROTOTYPE _EXPORT int32_t psrp_get_type_from_handle(const void *);

// Remove communication channels when PSRP server process exist [root thread]
_PROTOTYPE _EXPORT void psrp_exit(void);

// Get process pid from process name [root thread]
_PROTOTYPE _EXPORT pid_t psrp_pname_to_pid(const char *);

// Get process name from pid [root thread]
_PROTOTYPE _EXPORT _BOOLEAN psrp_pid_to_pname(const pid_t, char *);

// Get pid from PSRP channel name [root thread]
_PROTOTYPE _EXPORT pid_t psrp_channelname_to_pid(const char *, const char *, const char *);

// Resolve process name from PSRP channel name [root thread]
_PROTOTYPE _EXPORT _BOOLEAN psrp_pid_to_channelname(const char *, const pid_t, char *, char *);

// Set current PSRP slaved client channel [root thread]
_PROTOTYPE _EXPORT int32_t psrp_set_current_sic(const psrp_channel_type *);

// Unset current slaved client channel [root thread]
_PROTOTYPE _EXPORT void psrp_unset_current_sic(void);

// Create connection to slaved PSRP client process [root thread]
#ifdef SSH_SUPPORT
_PROTOTYPE _EXPORT psrp_channel_type *psrp_create_slaved_interaction_client(const char *, const char *, const char *);
#else
_PROTOTYPE _EXPORT psrp_channel_type *psrp_create_slaved_interaction_client(const char *);
#endif /* SSH_SUPPORT */

// Send a request to slaved PSRP client and wait for reply [root thread]
_PROTOTYPE _EXPORT char *psrp_slaved_client_transaction(const _BOOLEAN, const psrp_channel_type *, const char *);

// Destroy slaved PSRP client connection [root thread]
_PROTOTYPE _EXPORT psrp_channel_type *psrp_destroy_slaved_interaction_client(const psrp_channel_type *, const _BOOLEAN);

// Assign stdio descriptors for PSRP server [root thread]
_PROTOTYPE _EXPORT int32_t psrp_assign_stdio(const FILE *, const int32_t *, const char *[], des_t *, des_t *, des_t *);

// Assign stdio streams for a PSRP task function [root thread]
_PROTOTYPE _EXPORT int32_t psrp_assign_fstdio(const FILE *, const int32_t *, const char *[], FILE *, FILE *, FILE *);

// Wait for data on nominated descriptor [root thread]
_PROTOTYPE _EXPORT int32_t psrp_wait_for_data(const des_t);

// Create alias for PSRP object [root thread]
_PROTOTYPE _EXPORT int32_t psrp_alias(const char *, const char *);

// Remove PSRP object alias [root thread]
_PROTOTYPE _EXPORT int32_t psrp_unalias(const char *, const char *);

// Display PSRP object aliases [root thread]
_PROTOTYPE _EXPORT int32_t psrp_show_aliases(const char *);

// Duplicate current process
_PROTOTYPE _EXPORT int32_t psrp_duplicate_instance(_BOOLEAN, char *);

// Free resources allocated to PSRP handler slot [root thread]
_PROTOTYPE _EXPORT int32_t psrp_ifree_tag_list(const uint32_t);

// Search PSRP handler list for the first free slot [root thread]
_PROTOTYPE _EXPORT int32_t psrp_get_tag_index(const uint32_t);

// Search PSRP handler list [root thread]
_PROTOTYPE _EXPORT int32_t psrp_isearch_tag_list(const char *, const uint32_t);

// Get the index of the next available slot in the dispatch table [root thread]
_PROTOTYPE _EXPORT int32_t psrp_get_action_slot_index(void);

// Destroy PSRP object by handler table index [root thread]
_PROTOTYPE _EXPORT void psrp_destroy_object(const uint32_t);

// Get state of PSRP object by tag [root thread]
_PROTOTYPE _EXPORT int32_t psrp_ostate(const char *);

// Check if named psrp channel is locked by client [root thread]
_PROTOTYPE _EXPORT _BOOLEAN psrp_channel_locked(const FILE *, const char *);

// Show open slaved client channels [root thread]
_PROTOTYPE _EXPORT void psrp_show_open_sics(const FILE *);

// Empty a FIFO [root thread]
_PROTOTYPE _EXPORT void psrp_empty_fifo(const des_t);

// Handler for client side of PSRP  interrupt mechanism
_PROTOTYPE _EXPORT int32_t psrp_client_int_handler(int32_t);

#ifdef SSH_SUPPORT
// Start new server segment [root thread]
_PROTOTYPE _EXPORT int32_t psrp_new_segment(const char *, const char *, const char *, const char *);
#else
_PROTOTYPE _EXPORT int32_t psrp_new_segment(const char *, const char *, const char *);
#endif /* SSH_SUPPORT */

// Generate duplicate instance a PSRP server [root thread]
_PROTOTYPE _EXPORT int32_t psrp_new_instance(const _BOOLEAN, const char *, const char *);

// Find host running specified server
_PROTOTYPE _EXPORT int32_t psrp_get_server_host(FILE *, char *, char [][SSIZE]);

// Prod a remote host (to see if it is alive)
_PROTOTYPE _EXPORT int32_t psrp_prod_host(FILE *, char *, char *, char *, int32_t, int32_t *);

// Find PSRP dispatch table slot entry [root thread]
_PROTOTYPE _EXPORT int32_t psrp_find_action_slot_index(const char *);

// Send request over slaved PSRP client channel [root thread]
_PROTOTYPE _EXPORT int32_t psrp_write_sic(const psrp_channel_type *, const char *);

// Read reply over slaved PSRP client channel [root thread]
_PROTOTYPE _EXPORT int32_t psrp_read_sic(const psrp_channel_type *, char *);

// Send  interrupt to slaved PSRP client [root thread]
_PROTOTYPE _EXPORT void psrp_int_sic(const psrp_channel_type *);

// Reset dispatch table [root thread]
_PROTOTYPE _EXPORT void psrp_reset_dispatch_table(void);

// Install exit function for PSRP client [root thread]
_PROTOTYPE _EXPORT int32_t psrp_set_client_exitf(const uint32_t, const char *,  int32_t (*)(int32_t));

// Remove exit function for PSRP client [root thread]
_PROTOTYPE _EXPORT int32_t psrp_reset_client_exitf(const uint32_t);

// Schedule activity in (PSRP server) crontab table [root thread]
_PROTOTYPE _EXPORT int32_t psrp_crontab_schedule(const char *, const char *, const char *, const void *);

// Unschedule  PSRP server (crontab) activity[root thread]
_PROTOTYPE _EXPORT int32_t psrp_crontab_unschedule(const uint32_t);

// Initialise (PSRP server) crontab [root thread]
_PROTOTYPE _EXPORT void psrp_crontab_init(void);

// Display scheduled (PSRP crontab) activities for this PSRP server [root thread]
_PROTOTYPE _EXPORT int32_t psrp_show_crontab(const FILE *);

// Set PSRP handler condition (error/status) code [root thread]
_PROTOTYPE _EXPORT void psrp_set_c_code(const char *);

// Fork a PUPS (server) process [root thread]
_PROTOTYPE _EXPORT pid_t psrp_fork(const char *, const _BOOLEAN);

// Read trail data from migrated PSRP channel [root thread]
_PROTOTYPE _EXPORT _BOOLEAN psrp_read_trailfile(const char *, char *, char *, char *);


#ifdef MAIL_SUPPORT
// Store components of a MIME encapsulated message [root thread]
_PROTOTYPE _EXPORT int32_t psrp_mime_storeparts(const char *, const char *);

// Extract PSRP requests from text segment of MIME encapsulated message [root thread]
_PROTOTYPE _EXPORT int32_t psrp_msg2requestlist(const char *, char *);

// Delete MIME encapsulated message [root thread]
_PROTOTYPE _EXPORT int32_t psrp_mime_delete(const char *);

// Send single file as MIME message [root thread]
_PROTOTYPE _EXPORT void psrp_mime_mail(const char *, const char *, const char *);

// Send set of files as multipart MIME message [root thread]
_PROTOTYPE _EXPORT int32_t psrp_mime_encapsulate(const char *, const char *, const char *, const int32_t, const mc_type *);

// Get replyto  address from MIME message header [root thread]
_PROTOTYPE _EXPORT int32_t psrp_get_replyto(const char *, char *);

// Mail homeostat (allow PSRP process to read mailbox and reply to  mail) [root thread]
_PROTOTYPE _EXPORT int32_t psrp_mail_homeostat(void *, const char *);
#endif /* MAIL_SUPPORT */


// Handler for client disconnect in response to server stop [root thread]
_PROTOTYPE _EXPORT int32_t psrp_cont_handler(const int32_t);

// Rename PSRP channel [root thread]
_PROTOTYPE _EXPORT int32_t psrp_rename_channel(const char *);

// Start PSRP server on target host [root thread]
_PROTOTYPE _EXPORT int32_t psrp_cmd_on_host(const char *, const _BOOLEAN, uint32_t, char *[]);

#ifdef SSH_SUPPORT
// Process a set of (slaved) PSRP requests  [root thread]
_PROTOTYPE _EXPORT char **psrp_process_sic_transaction_list(const char *, const char *, const int32_t, const char *);
#else
_PROTOTYPE _EXPORT char **psrp_process_sic_transaction_list(const char *, const int32_t, const char *);
#endif /* SSH_SUPPORT */

// Set (bash) enviroment for remote execution [root thread]
_PROTOTYPE _EXPORT int32_t psrp_exec_env(const char *);

// Enable PSRP abort status flag  [root thread]
_PROTOTYPE _EXPORT int32_t psrp_enable_abrtflag(void);

// Disable PSRP abort status flag [root thread]
_PROTOTYPE _EXPORT int32_t psrp_disable_abrtflag(void);

// Reset PSRP abort status flag 
_PROTOTYPE _EXPORT int32_t psrp_reset_abrtflag(void);

// Clear PSRP abort status flag [root thread]
_PROTOTYPE _EXPORT int32_t psrp_clear_abrtflag(void);

// Get PSRP abort status flag  [root thread]
_PROTOTYPE _EXPORT _BOOLEAN psrp_get_abrtflag(void);

// Enable/disable PSRP client interrupt in critical PSRP server operation [root thread]
_PROTOTYPE _EXPORT int32_t psrp_critical(const _BOOLEAN);

// Block/unblock client interrupt [root thread]
_PROTOTYPE _EXPORT int32_t psrp_client_block(const _BOOLEAN);

// Is PEN (process execution name) unique? [root thread]
_PROTOTYPE _EXPORT void psrp_pen_unique(void);


#ifdef _CPLUSPLUS
#   undef  _EXPORT
#   define _EXPORT extern "C"
#else
#   undef  _EXPORT                                                              
#   define _EXPORT extern                                   
#endif /* _CPLUSPLUS */                                                                         

#endif  /* PSRP_H */

