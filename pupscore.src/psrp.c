/*--------------------------------------------------------------
    Purpose: User communication interface to PSRP server process

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 22.11
    Dated:   3rd January 2025 
    E-mail:  mao@tumblingdice.co.uk
--------------------------------------------------------------*/
/*-------------------------------------------------------------*/
/* If we have OpenMP support then we also have pthread support */
/* (OpenMP is built on pthreads for Linux distributions).      */
/*-------------------------------------------------------------*/

#if defined(_OPENMP) && !defined(PTHREAD_SUPPORT)
#define PTHREAD_SUPPORT
#endif /* PTHREAD_SUPPORT */


#ifdef HAVE_CURSES
#include <curses.h>
#endif /* HAVE_CURSES */

#include <errno.h>
#include <locale.h>
#include <me.h>
#include <utils.h>
#include <netlib.h>
#include <psrp.h>
#include <dll.h>

#ifdef PERSISTENT_HEAP_SUPPORT
#include <pheap.h>
#endif /* PERSISTENT_HEAP_SUPPORT */

#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <bsd/string.h>
#include <setjmp.h>
#include <errno.h>
#include <pwd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <utmp.h>
#include <termios.h>
#include <vstamp.h>
#include <dirent.h>


/*********************************/
/* Floating point representation */
/*********************************/

#include <ftype.h>

#ifdef HAVE_READLINE
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#endif /* HAVE_READLINE */


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


/*---------*/
/* Version */
/*---------*/

#define PSRP_VERSION          "22.09"


/*---------------------------------------------*/
/* Defines which are local to this application */
/*---------------------------------------------*/


#define PSRP_MAX_HISTORY      1024
#define PML_VERSION           1.03
#define PSRP_LOCAL_ATOMIC     1
#define PSRP_LOCAL_AGGREGATE  2
#define PSRP_REMOTE_REQUEST   3
#define PSRP_REQUEST_ERROR    (-1)
#define MAX_PSRP_MACROS       SSIZE 
#define MAX_PSRP_MACRO_FILES  32
#define MAX_MACRO_ARGS        32
#define MAX_MACRO_TAGS        128 
#define MAX_CLASH_TAGS        32 
#define MAX_LABELS            128
#define EXPAND_MACRO_STREXT   0 
#define MAX_MACRO_DEPTH       N_STREXT_STRINGS
#define EXIT_ON_INTR          0
#define RESTART_INTR_REQ      1
#define RESTART_INTR_MACRO    2
#define DEFAULT_PAGER         "less -r"
#define DEAD_SERVERS          (2 << 0)
#define VECTORED_SERVERS      (2 << 1)
#define NOWNED_SERVERS        (2 << 2)
#define BUSY_SERVERS          (2 << 3)
#define IDLE_SERVERS          (2 << 4)


/*--------------------------------*/
/* Definition of (PML) macro type */
/*--------------------------------*/

                                                          /*----------------------------------------*/
typedef struct {    uint32_t cnt;                         /* Number of name tags actually in use    */
                    char     mdf[SSIZE];                  /* Macro definition file exporting macro  */
                    char     tag[MAX_MACRO_TAGS][SSIZE];  /* Array of macro name tags               */
                    char     body[SSIZE];                 /* Body of macro                          */
               } macro_type;                              /*----------------------------------------*/



/*----------------------------------------------*/
/* Get application information for slot manager */
/*----------------------------------------------*/
/*---------------------------*/
/* Slot information function */
/*---------------------------*/

_PRIVATE void psrp_slot(int32_t level)
{   (void)fprintf(stdout,"int app psrp %s: [ANSI C]\n",PSRP_VERSION);

    if(level > 1)
    {  (void)fprintf(stdout,"(C) 1994-2025 Tumbling Dice\n");
       (void)fprintf(stdout,"Author: M.A. O'Neill\n");
       (void)fprintf(stdout,"PSRP (protocol %5.2F) communication shell (gcc %s: built %s %s)\n",PSRP_PROTOCOL_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fflush(stdout);
    }

    (void)fprintf(stdout,"\n");
    (void)fflush(stdout);
}



/*----------------------------*/
/* usage information function */
/*----------------------------*/

_PRIVATE void psrp_usage(void)

{   (void)fprintf(stdout,"[-startmsg:FALSE]\n"); 
    (void)fprintf(stdout,"[-spid <pid of PSRP server process>]\n");
    (void)fprintf(stdout,"[-sname <name of PSRP server process>\n");
    (void)fprintf(stdout,"[-wait:FALSE]\n");
    (void)fprintf(stdout,"\n[-noprompt:FALSE]\n");
    (void)fprintf(stdout,"[-log:FALSE]\n");
    (void)fprintf(stdout,"[-trys <attempts to connect to server PSRP channel>]]\n\n");
    (void)fprintf(stdout,"[-quiet:FALSE]\n");
    (void)fprintf(stdout,"[-squiet:FALSE]\n");
    (void)fprintf(stdout,"[-c <PSRP request to be executed>]\n");
    (void)fprintf(stdout,"[-hard:FALSE]\n");
    (void)fprintf(stdout,"[-log:FALSE>]\n");
    (void)fprintf(stdout,"[-recursive:FALSE]\n");

    #ifdef PSRP_AUTHENTICATE
    (void)fprintf(stdout,"[-password <authentication token>]\n");
    (void)fprintf(stdout,"[-user <username>]\n");
    #endif /* PSRP_AUTHENTICATE */

    #ifdef SLAVED_COMMANDS
    (void)fprintf(stdout,"[-clist <clist file name>]\n");
    #endif /* SLAVED_COMMANDS */

    #ifdef SSH_SUPPORT
    (void)fprintf(stdout,"[-on <host> [-from <host> <client> <client pid]]\n");
    #endif /* SSH_SUPPORT */

    (void)fprintf(stdout,"[-slaved:FALSE]\n\n");
    (void)fprintf(stdout,"[>& <ASCII log file>]\n\n");
    (void)fprintf(stdout,"SIGNALS\n\n");
    (void)fprintf(stdout, "SIGPSRP, SIGINIT, SIGCHAN, SIGCLIENT: PSRP status request signals (protocol %5.2F)\n",PSRP_PROTOCOL_VERSION);

    #ifdef CRUI_SUPPORT
    (void)fprintf(stderr,"SIGCHECK SIGRESTART:      checkpoint and restart signals\n");
    #endif /* CRIU_SUPPORT */

    (void)fprintf(stdout,"SIGALIVE: check for presence of PSRP server process\n");
    (void)fflush(stdout);
}


#ifdef SLOT
_EXTERN int32_t max_slots                               = MAX_SLOTS;
_EXTERN int32_t max_use_slots                           = MAX_USE_SLOTS;
_EXTERN void    (* SLOT ) __attribute__ ((aligned(16))) = psrp_slot;
_EXTERN void    (* USE  ) __attribute__ ((aligned(16))) = psrp_usage;
#endif /* SLOT */



/*------------------------*/
/* Application build date */
/*------------------------*/

_EXTERN char appl_build_time[SSIZE] = __TIME__;
_EXTERN char appl_build_date[SSIZE] = __DATE__;


/*----------------------------------------*/
/* Functions imported by this application */
/*----------------------------------------*/
/*--------------------------------------------------*/
/* Variables which are imported by this application */
/*--------------------------------------------------*/

_IMPORT int32_t     pupshold_cnt;


/*-----------------*/
/* Local variables */
/*-----------------*/

_PRIVATE char       psrp_history[SSIZE]                  = "";
_PRIVATE char       trailfile_name[SSIZE]                = "";
_PRIVATE _BOOLEAN   psrp_recursive                       = FALSE;
_PRIVATE _BOOLEAN   window_resized                       = FALSE;
_PRIVATE _BOOLEAN   in_cb_readline                       = FALSE;
_PRIVATE int32_t    request_type                         = 0;
_PRIVATE char       datasink_cmd[SSIZE]                  = DEFAULT_PAGER;
_PRIVATE int32_t    from_tstp_handler                    = FALSE;
_PRIVATE int32_t    restricted_clist                     = FALSE;
_PRIVATE int32_t    clist_size                           = 0;
_PRIVATE int32_t    clist_alloc                          = 0;
_PRIVATE char       **clist                              = (char **)NULL;
_PRIVATE char       clist_f_name[SSIZE]                  = "";
_PRIVATE _BOOLEAN   exit_on_terminate                    = FALSE;
_PRIVATE _BOOLEAN   secure_operation                     = FALSE;
_PRIVATE _BOOLEAN   do_datasink                          = FALSE;
_PRIVATE _BOOLEAN   do_bg_datasink                       = FALSE;
_PRIVATE int32_t    child_pid                            = (-1);
_PRIVATE int32_t    from_client_pid;
_PRIVATE char       from_client_name[SSIZE]              = "";
_PRIVATE char       from_hostname[SSIZE]                 = "";
_PRIVATE sigjmp_buf command_loop_top;
_PRIVATE sigjmp_buf process_loop_top;
_PRIVATE char       macro_f_name[SSIZE]                  = "";
_PRIVATE char       remote_host_pathname[SSIZE]          = "notset";
_PRIVATE char       channel_name[SSIZE]                  = "";
_PRIVATE char       channel_out[SSIZE]                   = "";
_PRIVATE char       psrp_server[SSIZE]                   = "";
_PRIVATE char       new_psrp_server[SSIZE]               = "notset";
_PRIVATE char       psrp_host[SSIZE]                     = "";
_PRIVATE char       request_line[SSIZE]                  = "";
_PRIVATE char       last_request_line[SSIZE]             = "";
_PRIVATE char       psrp_c_code[SSIZE]                   = "";
_PRIVATE char       c_code[SSIZE]                        = "";
_PRIVATE char       last_macro[SSIZE]                    = "";
_PRIVATE char       username[SSIZE]                      = "";
_PRIVATE int32_t    max_trys                             = MAX_TRYS;
//_PRIVATE  int32_t        try_cnt                            = 8;
_PRIVATE int32_t    server_seg_cnt                       = 0;
_PRIVATE int32_t    n_m_files                            = 0;
_PRIVATE int32_t    rpt_cnt                              = 0;
_PRIVATE int32_t    r_cnt                                = 0;
_PRIVATE int32_t    seg_restart_action                   = EXIT_ON_INTR; 
_PRIVATE int32_t    rpt_period                           = PSRP_RPT_DEFAULT;
_PRIVATE int32_t    t_cnt                                = 0;
_PRIVATE int32_t    repeat_count                         = (-1);
_PRIVATE int32_t    n_macros                             = 0;

_PRIVATE macro_type macro[MAX_PSRP_MACROS];
 
_PRIVATE FILE       *client_in                           = (FILE *)NULL;
_PRIVATE FILE       *client_out                          = (FILE *)NULL;
_PRIVATE FILE       *stream                              = (FILE *)NULL;

_PRIVATE ino_t      out_inode;
_PRIVATE ino_t      current_out_inode;
_PRIVATE ino_t      in_inode;
_PRIVATE ino_t      current_in_inode;

_PRIVATE pid_t      server_pid                           = (-1);
_PRIVATE pid_t      slave_pid                            = (-1);
_PRIVATE pid_t      new_server_pid                       = (-1);

_PRIVATE des_t      slave_in                             = (-1);

_PRIVATE _BOOLEAN   close_psrp_channel                   = FALSE;
_PRIVATE _BOOLEAN   in_new_getinfo                       = FALSE;
_PRIVATE _BOOLEAN   wait_for_psrp_server_startup         = FALSE;
_PRIVATE _BOOLEAN   migrating_segmentation               = FALSE;
_PRIVATE _BOOLEAN   do_rsnr                              = FALSE;
_PRIVATE _BOOLEAN   cinit_done                           = FALSE;
_PRIVATE _BOOLEAN   in_repeat_command                    = FALSE;
_PRIVATE _BOOLEAN   priviliged                           = FALSE;
_PRIVATE _BOOLEAN   busy_retry_mode                      = FALSE;
_PRIVATE _BOOLEAN   psrp_hard_link                       = FALSE;
_PRIVATE _BOOLEAN   is_builtin                           = TRUE;
_PRIVATE _BOOLEAN   psrp_slaved                          = FALSE;
_PRIVATE _BOOLEAN   redirected_requests                  = FALSE;
_PRIVATE _BOOLEAN   is_remote                            = FALSE;
_PRIVATE _BOOLEAN   psrp_log                             = FALSE;
_PRIVATE _BOOLEAN   interactive_mode                     = TRUE;
_PRIVATE _BOOLEAN   is_aggregate                         = FALSE;
_PRIVATE _BOOLEAN   exec_shell                           = FALSE;
_PRIVATE _BOOLEAN   processing_command                   = FALSE;
_PRIVATE _BOOLEAN   pups_error_abort                     = FALSE;
_PRIVATE _BOOLEAN   in_client_handler                    = FALSE;
_PRIVATE _BOOLEAN   in_client_handler_int                = FALSE;
_PRIVATE _BOOLEAN   in_append_op                         = FALSE;
_PRIVATE _BOOLEAN   flycom                               = FALSE;
_PRIVATE _BOOLEAN   server_verbose                       = TRUE;
_PRIVATE _BOOLEAN   pel_appl_verbose                     = TRUE;
_PRIVATE _BOOLEAN   save_pel_appl_verbose                = TRUE;
_PRIVATE _BOOLEAN   macro_expansion                      = TRUE;
_PRIVATE _BOOLEAN   macros_expanded                      = FALSE;
_PRIVATE _BOOLEAN   show_body                            = FALSE;
_PRIVATE _BOOLEAN   rpt_curses_mode                      = FALSE;
_PRIVATE _BOOLEAN   in_prompt_loop                       = FALSE;
_PRIVATE _BOOLEAN   do_repeat[MAX_MACRO_DEPTH]           = { FALSE };
_PRIVATE _BOOLEAN   log_state_updated                    = FALSE;
_PRIVATE _BOOLEAN   prompt                               = TRUE;
_PRIVATE _BOOLEAN   have_access_lock                     = FALSE;
_PRIVATE _BOOLEAN   server_connected                     = FALSE;

_PRIVATE char mstack_f_name[MAX_PSRP_MACRO_FILES][SSIZE] = { [0 ... MAX_PSRP_MACRO_FILES-1] = {""}};


/*-----------------*/
/* Local functions */
/*-----------------*/

// Check for existence of trailfile and  get its name
_PRIVATE _BOOLEAN psrp_have_trailfile(void);

// Virtual maggot
_PROTOTYPE _PRIVATE int32_t psrp_virtual_maggot(const char *);

// Get (test) password
_PROTOTYPE _PRIVATE _BOOLEAN psrp_get_password(void);

#ifdef SLAVED_COMMANDS
// Read a restricted command list
_PROTOTYPE _PRIVATE int32_t builtin_read_clist(const _BOOLEAN, const char *);

// Is command permitted? 
_PROTOTYPE _PRIVATE _BOOLEAN command_permitted(const char *);
#endif /* SLAVED_COMMANDS */

// Set current working directory for PSRP client
_PROTOTYPE _PRIVATE _BOOLEAN psrp_builtin_set_lcwd(const char *);
  
// Follow a PSRP server onto a new node
_PROTOTYPE _PRIVATE int32_t psrp_migrate_client_to_server_host(void);

// Handler for SIGABRt
_PROTOTYPE _PRIVATE int32_t critical_handler(const int32_t);

// Handler for SIGWINCH
_PROTOTYPE _PRIVATE int32_t winch_handler(const int32_t);

// Handler for SIGTSTP
_PROTOTYPE _PRIVATE int32_t tstp_handler(const int32_t);

// Remove lockpost files
_PROTOTYPE _PRIVATE void psrp_remove_lockpost_files(void);

// Set authentication token for remote PSRP services
_PRIVATE _BOOLEAN builtin_set_password(const char *, const char *);

// Change PSRP client session owner
_PROTOTYPE _PRIVATE _BOOLEAN builtin_change_user(const char *);

// Set authentication token (for remote PSRP services)
_PROTOTYPE _PRIVATE _BOOLEAN psrp_set_password(const char *);

// Check password (for a given user)
_PROTOTYPE _PRIVATE _BOOLEAN psrp_check_password(const char *, const char *);

// Connect to PSRP client process on remote client
_PROTOTYPE _PRIVATE _BOOLEAN builtin_connect_remote_client(const char *);

// Show PUPS channels within nominated directory
_PROTOTYPE _PRIVATE void psrp_show_channels(const int32_t, const char *);

// Kill PSRP servers in nominated directory
_PROTOTYPE _PRIVATE void psrp_kill_servers(const char *, const char *);

// Restore system status (after longjmp)
_PROTOTYPE _PRIVATE void restore_system_status(const int32_t);

// Initialise do_repeat array
_PROTOTYPE _PRIVATE void initialise_repeaters(void);

// Clear screen
_PROTOTYPE _PRIVATE void cls(void);

// Process a repeated command
_PROTOTYPE _PRIVATE _BOOLEAN psrp_repeat_command(const int32_t, const char *);

// Start curses screen handling
_PROTOTYPE _PRIVATE void cinit(void);

// Stop curses screen handling
_PROTOTYPE _PRIVATE void cend(void);

// Clear screen
_PROTOTYPE _PRIVATE void cls(void);

// Print header for repeated command
_PROTOTYPE _PRIVATE void rpthdr(const char *);

// Find label (in macro body)
_PROTOTYPE _PRIVATE _BOOLEAN req_find(const uint32_t, const char *, const char *);

// Load a macro definition file
_PROTOTYPE _PRIVATE void builtin_load_macro_definitions(const _BOOLEAN, const _BOOLEAN, const char *);

// Edit a macro definition file
_PROTOTYPE _PRIVATE void builtin_edit_macro_definitions(const _BOOLEAN, const _BOOLEAN, const char *);

// Expand recursive macro definitions
_PROTOTYPE _PRIVATE _BOOLEAN expand_macros_recursive(const char *, char *);

// Substitute macro arguments
_PROTOTYPE _PRIVATE _BOOLEAN substitute_arguments(char *, const uint32_t, const char [MAX_MACRO_ARGS][SSIZE]);

// Expand macro definitions
_PROTOTYPE _PRIVATE _BOOLEAN expand_macros(const char *, char *);

// Update the macro stack
_PROTOTYPE _PRIVATE void update_macro_stack(const _BOOLEAN, const _BOOLEAN, const char *);

// Update macro definitions file
_PROTOTYPE _PRIVATE void edit_macro_definitions(const _BOOLEAN, const _BOOLEAN, const char *);

// Check macro tags (for uniqueness)
_PROTOTYPE _PRIVATE _BOOLEAN check_macro_tags(const uint32_t, const char *);

// Catentate macro to macro file
_PROTOTYPE _PRIVATE void catenate_macro(const char *, const char *, const char *);

// Purge (delete) currently loaded macro definitions
_PROTOTYPE _PRIVATE void purge_macros(const char *, const _BOOLEAN, const _BOOLEAN);

// Load macro definition file
_PROTOTYPE _PRIVATE int32_t load_macro_definitions(const _BOOLEAN, const _BOOLEAN, const char *);

// Show loaded macros
_PROTOTYPE _PRIVATE void show_macro_tags(void);

// Wait for end of operation
_PROTOTYPE _PRIVATE void psrp_waitfor_endop(const char *);

// Interact with nominated PSRP server process
_PROTOTYPE _PRIVATE void psrp_command_loop(void);

// Handler for SIGTSTP
_PROTOTYPE _PRIVATE int32_t tstp_handler(const int32_t);

// Handler for SIGPSRP
_PROTOTYPE _PRIVATE int32_t client_handler(const int32_t);

// Handler for SIGCHAN
_PROTOTYPE _PRIVATE int32_t chan_client_handler(const int32_t);

// Handler for SIGALIVE (to server)
_PROTOTYPE _PRIVATE void server_alive_handler(vttab_type *, char *);

// Send a single command to the PSRP server process
_PROTOTYPE _PRIVATE _BOOLEAN psrp_process_command(char *);

// Remove junk prior to exiting
_PROTOTYPE _PRIVATE void remove_junk(void);

// Open a PSRP server process
_PROTOTYPE _PRIVATE _BOOLEAN psrp_open_server(void);

// Close a PSRP server process
_PROTOTYPE _PRIVATE int32_t psrp_close_server(const _BOOLEAN, const _BOOLEAN);

// Builtin to catenate last request to macro definition file
_PROTOTYPE _PRIVATE void builtin_catenate_macro(char *);

// Builtin to open a copnnection to a new PSRP server
_PROTOTYPE _PRIVATE void builtin_open_psrp_server(const char *);


// Display help on builtin client and server commands
_PROTOTYPE _PRIVATE void builtin_psrp_help(void);

// Handler for SIGINT when PSRP server process is in a critical (blocking) state
_PROTOTYPE _PRIVATE int32_t psrp_critical_int_handler(const int32_t);

// Handler for SIGINT propagates SIGINT to PSRP server process
_PROTOTYPE _PRIVATE int32_t psrp_int_handler(const int32_t);

// Expand a request of the form server@host: <command list>
_PROTOTYPE _PRIVATE int32_t expand_request(char *, char *, _BOOLEAN *, char *);

// Test for repeated command
_PROTOTYPE _PRIVATE void initialise_repeaters(void);

// Grab PSRP channel (to talk to server)
_PROTOTYPE _PRIVATE int32_t psrp_grab_channel(void);

// Yield PSRP channel (to other connected client)
_PROTOTYPE _PRIVATE void psrp_yield_channel(void);


/*-------------------*/
/* Software I.D. tag */
/*-------------------*/

#define VTAG  8182
extern  int32_t appl_vtag = VTAG;




/*--------------------*/
/* Handler for SIGHUP */
/*--------------------*/

_PRIVATE int32_t hup_handler(const int32_t signum)

{   (void)pups_exit(255);
}



#ifdef PSRP_DEBUG
testiface(void)

{   char line[SSIZE] = "";

    (void)fprintf(stdout,"PSRP\n");
    (void)fflush(stdout);
    (void)scanf("%s",line);
    (void)fprintf(stdout,"PSRP got (%s)\n",line);
    (void)fprintf(stdout,"EOT\n");
    (void)fflush(stdout);

    exit(0);
}
#endif /* DEBUG */




/*--------------------------------------------------*/
/* PSRP communications client pups_main entry point */
/*--------------------------------------------------*/

_PUBLIC int32_t pups_main(int32_t argc, char *argv[])

{   char psrp_request[SSIZE]    = "",
         pathname[SSIZE]        = "",
         psrp_parameters[SSIZE] = "",
         remote_host[SSIZE]     = "";

    sigset_t client_set;


    /*------------------------------------------*/
    /* Get standard items form the command tail */
    /*------------------------------------------*/

    pups_std_init(TRUE,
                  &argc,
                  PSRP_VERSION,
                  "M.A. O'Neill",
                  "psrp",
                  "2025",
                  (void *)argv);


    #ifdef HAVE_READLINE
    /*-------------------------------*/
    /* Get (readline) history if any */
    /*-------------------------------*/

    (void)snprintf(psrp_history,SSIZE,"%s/.%s.history",appl_home,appl_name);
    if(access(psrp_history,F_OK | R_OK) != (-1))
       (void)read_history(psrp_history);
    #endif /* HAVE_READLINE */


    /*--------------------*/
    /* Handler for SIGHUP */
    /*--------------------*/

    (void)signal(SIGHUP,(void *)&hup_handler);


    /*--------------------------------------------------------------------------*/
    /* Get password to authenticate connection to PSRP services on remote hosts */
    /*--------------------------------------------------------------------------*/

    if(pups_locate(&init,"startmsg",&argc,args,0) != NOT_FOUND)
    {  (void)fprintf(stderr,"\n    PSRP client version %s, [build %6d] (gcc: %s: build %s %s)\n",PSRP_VERSION,appl_vtag,__VERSION__,__TIME__,__DATE__);
       (void)fprintf(stderr,"    (C) M.A. O'Neill, Tumbling Dice 2025\n\n");
       (void)fflush(stderr);
    }


    #ifdef PSRP_AUTHENTICATE 
    if(pups_locate(&init,"password",&argc,args,0) != NOT_FOUND)
    {  if(isatty(0) == 1)
          psrp_set_password(appl_password);
       else
          (void)scanf("%s",appl_password);
    }
    #ifdef SLAVED_COMMANDS_
    if((ptr = pups_locate(&init,"clist",&argc,args,0)) != NOT_FOUND)
    {  if(strccpy(clist_f_name,pups_str_dec(&ptr,&argc,args)) == (char *)NULL)
          pups_error("[psrp] expecting restricted command list name"); 
       (void)builtin_read_clist(FALSE,clist_f_name);
    }
    #endif /* SLAVED_COMMANDS */
    #endif /* PSRP_AUTHENTICATE */



    /*-----------------------------------------------------------*/
    /* Hard link - psrp client stays attached to stopped servers */
    /*-----------------------------------------------------------*/

    if(pups_locate(&init,"hard",&argc,args,0) != NOT_FOUND)
    {  psrp_hard_link = TRUE;

       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stdout,"%sWARNING%s %s %s (%d@%s): PSRP client is hard linked (client will remain attached to stopped server)\n",
                                                                                                                          boldOn,boldOff,
                                                                                            date,appl_name,appl_pid,appl_host,appl_owner);
          (void)fflush(stdout);
       }
    }


    /*---------------------------------------------------------------*/
    /* Wait for target PSRP server to start (and then connect to it) */
    /*---------------------------------------------------------------*/

    if(pups_locate(&init,"wait",&argc,args,0) != NOT_FOUND)  
    {  wait_for_psrp_server_startup = TRUE;

       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stdout,"%sWARNING%s %s %s (%d@%s): PSRP client wait mode (client will wait for target server to start and then attach to it))\n",
                                                                                                                                          boldOn,boldOff,
                                                                                                            date,appl_name,appl_pid,appl_host,appl_owner);
          (void)fflush(stdout);
       }
    }


    /*------------------------------------------------*/
    /* Exit when (attached) server terminates if TRUE */
    /*------------------------------------------------*/

    if(pups_locate(&init,"exit",&argc,args,0) != NOT_FOUND)  
    {  exit_on_terminate = TRUE;

       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stdout,"%sWARNING%s %s %s (%d@%s): PSRP client exit mode (client will exit when attached server terminates))\n",
                                                                                                                         boldOn,boldOff,
                                                                                           date,appl_name,appl_pid,appl_host,appl_owner);
          (void)fflush(stdout);
       }
    }


    /*----------------------*/
    /* Change effective UID */
    /*----------------------*/

    if((ptr = pups_locate(&init,"user",&argc,args,0)) != NOT_FOUND)
    {  struct passwd *pwent = (struct passwd *)NULL;

       if(geteuid() != 0)
          pups_error("[psrp] need to set UID root to change session owner");

       if(strccpy(appl_owner,pups_str_dec(&ptr,&argc,args)) == (char *)NULL)
          pups_error("[psrp] expecting effective user name");

       if(psrp_check_password(appl_owner,shell) == FALSE)
          pups_error("[psrp] failed to change effective user");
    }


    #ifdef SSH_SUPPORT
    /*---------------------------------------------------*/
    /* Run on remote host (via terminal services daemon) */
    /*---------------------------------------------------*/

    (void)strlcpy(from_hostname,appl_host,SSIZE);
    (void)strlcpy(from_client_name,appl_name,SSIZE);
    from_client_pid = appl_pid;


    /*----------------------------------------------*/
    /* If we are a remote client, where are we from */
    /*----------------------------------------------*/

    if((ptr = pups_locate(&init,"from",&argc,args,0)) != NOT_FOUND)
    {  int32_t in;

       is_remote = TRUE;


       /*---------------------------------*/
       /* Get the name of the remote host */
       /*---------------------------------*/

       if(strccpy(from_hostname,pups_str_dec(&ptr,&argc,args)) == (char *)NULL)
          pups_error("[psrp] expecting name of (remote) client's host");


       /*-----------------------------------*/
       /* Get the name of the remote client */
       /*-----------------------------------*/

       if(strccpy(from_client_name,pups_str_dec(&ptr,&argc,args)) == (char *)NULL)
          pups_error("[psrp] expecting name of (remote) client");


       /*----------------------------------*/
       /* Get the pid of the remote client */
       /*----------------------------------*/

       if((from_client_pid = pups_i_dec(&ptr,&argc,args)) == (int32_t)INVALID_ARG || from_client_pid < 0)
          pups_error("[psrp] expecting name of (remote) client pid");


       /*---------------------------*/
       /* Build path to remote host */
       /*---------------------------*/

       (void)snprintf(remote_host_pathname,SSIZE,"%s[%d]@%s:%s",from_client_name,from_client_pid,from_hostname,appl_owner);

       #ifdef PSRP_DEBUG 
       (void)fprintf(stderr,"REMOTE HOST PATHNAME: \"%s\"\n",remote_host_pathname);
       (void)fflush(stderr);
       #endif /* PSRP_DEBUG */

    }
    #endif /* SSH_SUPPORT */


    /*----------------------------------------------------------------------*/
    /* Slave mode - propagate PSRP protocol control words to enslaving peer */
    /*----------------------------------------------------------------------*/

    if((ptr = pups_locate(&init,"slaved",&argc,args,0)) != NOT_FOUND)
    {  psrp_slaved = TRUE;

       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stdout,"%sWARNING%s %s %s (%d@%s): PSRP client is running in slaved mode (PSRP protocol propagation enabled)\n",
                                                                                                                         boldOn,boldOff,
                                                                                           date,appl_name,appl_pid,appl_host,appl_owner);
          (void)fflush(stdout);
       }
    } 


    /*-----------------------------------*/
    /* Mark this PSRP instance recursive */
    /*-----------------------------------*/

    if(pups_locate(&init,"recursive",&argc,args,0) != NOT_FOUND)
    {  psrp_recursive = TRUE;

      if(appl_verbose == TRUE)
      {  (void)strdate(date);
         (void)fprintf(stdout,"%sWARNING%s %s %s (%d@%s): PSRP client is running in recursive mode (processing aggregate command)\n",
                                                                                                                      boldOn,boldOff,
	                                                                                date,appl_name,appl_pid,appl_host,appl_owner);
         (void)fflush(stdout);
      }
    }


    /*---------------------------------------------------------------------------------------*/
    /* If we have a .psrprc file - re-direct stdin to it so we can execute any psrp commands */
    /* in it before we start accepting commands from the terminal                            */
    /*---------------------------------------------------------------------------------------*/

    (void)snprintf(pathname,SSIZE,"%s/.psrprc",getenv("HOME"));
    if(access(pathname,F_OK | R_OK | W_OK) != (-1))
    {  if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stdout,"%sWARNING%s %s %s (%d@%s:%s): Reading commands from \"%s\" (prior to interactive comamnd input)\n",
                                                                                                                    boldOn,boldOff,
                                                                             date,appl_name,appl_pid,appl_host,appl_owner,pathname);
          (void)fflush(stdout);
       }


       /*--------------------------------------------------------------------*/
       /* If this client is attached to a slave channel we need to duplicate */
       /* descriptor before we re-drirect stdin                              */
       /*--------------------------------------------------------------------*/

       if(isatty(0) == 0)
          slave_in = dup(0);

       (void)fclose(stdin);

       stdin               = fopen(pathname,"r");
       redirected_requests = TRUE;
    }
    else
    {  struct stat buf;
       (void)fstat(0,&buf);


       /*-------------------------------------------------------------*/
       /* If we have a regular file requests are being redirected via */
       /* stdin -- take appropriate action                            */ 
       /*-------------------------------------------------------------*/

       if(S_ISREG(buf.st_mode))
       {  if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stdout,"%sWARNING%s %s %s (%d@%s:%s):    Reading commands from redirected \"stdin\" (prior to interactive command input)\n",
                                                                                                                                        boldOn,boldOff,
                                                                                                 date,appl_name,appl_pid,appl_host,appl_owner,pathname);
             (void)fflush(stdout);
          }

          redirected_requests = TRUE;
       }
    }


    /*---------------------------------------------*/
    /* Initialise handler and pups_error functions */
    /*---------------------------------------------*/

    (void)strlcpy(psrp_c_code,"ok",SSIZE);
    (void)sigfillset(&client_set);

    (void)pups_sighandle(SIGCLIENT,  "client_handler",     (void *)client_handler,      (sigset_t *)NULL);
    (void)pups_sighandle(SIGCHAN,    "chan_client_handler",(void *)chan_client_handler, (sigset_t *)NULL);
    (void)pups_sighandle(SIGTSTP,    "tstp_handler",       (void *)tstp_handler,        (sigset_t *)NULL);
    (void)pups_sighandle(SIGCRITICAL,"critical_handler",   (void *)critical_handler,    (sigset_t *)NULL);

    (void)strlcpy((char *)psrp_server,"unnamed",SSIZE);


    /*--------------------*/
    /* Select prompt mode */
    /*--------------------*/

    if(pups_locate(&init,"noprompt",&argc,args,0) != NOT_FOUND)
    {  prompt = FALSE;

       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stdout,"%s %s (%d@%s:%s): psrp prompt mode disabled\n",
                                  date,appl_name,appl_pid,appl_host,appl_owner);
          (void)fflush(stdout);
       }


       /*--------------------------------*/
       /* Tell the world we have started */
       /*--------------------------------*/

       (void)fprintf(stdout,"PSRP\n");
       (void)fflush(stdout);
    }


    /*------------------------*/
    /* Get PSRP server by pid */
    /*------------------------*/

    do_rsnr = FALSE;
    if((ptr = pups_locate(&init,"spid",&argc,args,0)) != NOT_FOUND)
    {  if((server_pid = pups_i_dec(&ptr,&argc,args)) == (int32_t)INVALID_ARG || server_pid < 0)
          pups_error("[psrp] expecting pid of PSRP server");


       /*-----------------------------------------*/
       /* Check to see if this is a valid process */
       /*-----------------------------------------*/

       if(psrp_pid_to_channelname(appl_fifo_dir,server_pid,psrp_server,psrp_host) == FALSE)
          pups_error("[psrp] channel to target PSRP process does not exist");


       /*--------------------------------------*/
       /* Is the resolved process name unique? */
       /*--------------------------------------*/

       if(psrp_channelname_to_pid(appl_fifo_dir,psrp_server,psrp_host) < 0)
       {  do_rsnr = TRUE;

          if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stdout,"\n%sERROR%s %s %s(%d@%s:%s): reverse server name resolution (cannot track PSRP channel dynamically)\n\n",
                                                                                                                             boldOn,boldOff,
                                                                                               date,appl_name,appl_pid,appl_host,appl_owner);
             (void)fflush(stdout);
          }
       }
    }
    else
    {

       /*-------------------------*/
       /* Get PSRP server by name */ 
       /*-------------------------*/

       if((ptr = pups_locate(&init,"sname",&argc,args,0)) != NOT_FOUND)
       {  int32_t  idum;
          uint32_t try_cnt = 0;

          if(strccpy(psrp_server,pups_str_dec(&ptr,&argc,args)) == (char *)NULL)
             pups_error("[psrp] expecting name of PSRP server process");
 
          if(sscanf(psrp_server,"%d",&idum) != 0)
             pups_error("[psrp] expecting name of PSRP server process");

          while(try_cnt < max_trys)
          {   server_pid = psrp_channelname_to_pid(appl_fifo_dir,psrp_server,psrp_host);


              /*-----------------------*/              
              /* Do we own the server? */
              /*-----------------------*/

              if(pups_i_own(server_pid) == FALSE)
              {  (void)snprintf(errstr,SSIZE,"%sERROR%S [psrp] current user \"%s\" does not own PSRP server \"%s\" (cannot connect)\n\n",boldOn,boldOff,appl_owner,psrp_server);
                 pups_error(errstr);
              }
                

              if(server_pid !=  PSRP_DUPLICATE_PROCESS_NAME    &&
                 server_pid !=  PSRP_TERMINATED                 )
                 goto process_found;

              if(wait_for_psrp_server_startup == FALSE)
              {  if(server_pid == PSRP_TERMINATED)
                 {  if(pel_appl_verbose == TRUE)
                    {  if(is_remote == TRUE &&  interactive_mode == FALSE)
                       {  if(do_rsnr == TRUE)
                             (void)fprintf(stdout,"\n%sERROR%s cannot access process \"%d@%s\" as PSRP server\n\n",boldOn,boldOff,server_pid,appl_host);
                          else
                             (void)fprintf(stdout,"\n%sERROR%s cannot access process \"%s@%s\" as PSRP server\n\n",boldOn,boldOff,psrp_server,appl_host);
                       }
                       else
                       {  if(do_rsnr == TRUE)
                             (void)fprintf(stdout,"\n%sERROR%s cannot access process %d as PSRP server\n\n",boldOn,boldOff,server_pid);
                          else
                             (void)fprintf(stdout,"\n%sERROR%s cannot access process \"%s\" as PSRP server\n\n",boldOn,boldOff,psrp_server);
                       }

                       (void)fflush(stdout);
                    }

                    pups_exit(255);
                 }
              }

              if(try_cnt == max_trys && server_pid == PSRP_TERMINATED)
                 pups_error("[psrp] channel to target PSRP process does not exist");
     
              if(server_pid == PSRP_DUPLICATE_PROCESS_NAME)
                 pups_error("[psrp] target PSRP process name is not unique");

              ++try_cnt;


              /*----------------------------------------*/
              /* Wait for migrated PSRP server to start */
              /*----------------------------------------*/

              if(try_cnt == 1)
              {  if(appl_verbose == TRUE)
                 {  (void)strdate(date);
                    (void)fprintf(stdout,"%sWARNING%s %s %s (%d@%s:%s): waiting for PSRP server \"%s@%s\" to start\n",
                                                                                                       boldOn,boldOff,
                                                   date,appl_name,appl_pid,appl_host,appl_owner,psrp_server,appl_host);
                    (void)fflush(stdout);
                 }
              }

              (void)pups_sleep(TRY_TIMEOUT);
          }


          /*----------------------------------*/
          /* Migrated PSRP server has started */
          /*----------------------------------*/

          if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stdout,"%s %s (%d@%s:%s): PSRP server \"%s@%s\" started\n",
                           date,appl_name,appl_pid,appl_host,appl_owner,psrp_server,appl_host);
             (void)fflush(stdout);
          }


          
          if(strcmp(psrp_c_code,"ok") != 0)
             pups_exit(255);
          pups_exit(0);

process_found:

          if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stdout,"%s %s (%d@%s:%s): PSRP server is %s (%d@%s)\n",
                            date,appl_name,appl_pid,appl_host,appl_owner,psrp_server,server_pid,appl_host);
             (void)fflush(stdout);
          }
       }
       else
          (void)strlcpy(psrp_server,"none",SSIZE);
    }


    /*---------------------------------------------*/
    /* Do not produce output from builtin commands */
    /* (when in non-interactive mode)              */
    /*---------------------------------------------*/

    if(pups_locate(&init,"quiet",&argc,args,0) != NOT_FOUND)
       pel_appl_verbose = FALSE;


    /*---------------------------------------------------*/
    /* Do not produce output from (psrp) server commands */
    /* (when in non-interactive mode)                    */
    /*---------------------------------------------------*/

    if(pups_locate(&init,"squiet",&argc,args,0) != NOT_FOUND)
       server_verbose = FALSE;


    /*-----------------------------------------*/
    /* Execute command in non-interactive mode */
    /*-----------------------------------------*/

    if((ptr = pups_locate(&init,"c",&argc,args,0)) != NOT_FOUND)
    {  if(strccpy(psrp_request,pups_str_dec(&ptr,&argc,args)) == (char *)NULL)
          pups_error("[psrp] expecting PSRP request");                                             

       interactive_mode = FALSE;
       pups_error_abort = TRUE;

       #ifdef PSRP_DEBUG
       (void)fprintf(stderr,"NON INTERACTIVE CMD: \"%s\"\n",psrp_request);
       (void)fflush(stderr);
       #endif /* PSRP_DEBUG */

    }


    /*-----------------------------*/
    /* Turn transaction logging on */
    /*-----------------------------*/

    if(pups_locate(&init,"log",&argc,args,0) != NOT_FOUND)
    {  psrp_log          = TRUE;
       log_state_updated = TRUE;

       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stdout,"%s %s (%d@%s:%s): Transaction logging enabled\n",
                                    date,appl_name,appl_pid,appl_host,appl_owner);
          (void)fflush(stdout);
       }
    }


    /*------------------------------------------------------*/
    /* If any arguments remain unparsed - complain and stop */
    /*------------------------------------------------------*/

    pups_t_arg_errs(argd,(void *)args);


    /*----------------*/
    /* Handle SIGPIPE */
    /*----------------*/

    (void)pups_sighandle(SIGPIPE,"psrp_int_handler",(void *)psrp_int_handler,(sigset_t *)NULL);


    /*-------------------------------------------------------------*/
    /* Current client is alway 1 for client end of PSRP connection */
    /*-------------------------------------------------------------*/

    c_client = 9999;


    /*-------------------------------------------------------------*/
    /* Create a communication channel to the target server process */
    /*-------------------------------------------------------------*/

    if(strcmp(psrp_server,"none") != 0 && (server_connected = psrp_open_server()) == FALSE)
       (void)strlcpy(psrp_server,"none",SSIZE);


    /*--------------------------------------------*/
    /* Start interacting with PSRP server process */
    /*--------------------------------------------*/

    (void)pupsighold(SIGCLIENT,TRUE);
    if(interactive_mode == TRUE)
       psrp_command_loop();
    else
    {  if(server_pid != (-1) && strin(psrp_request,"close") == FALSE) 
          (void)strlcat(psrp_request,"; close\n",SSIZE);
       else
          (void)strlcat(psrp_request,"\n",SSIZE);

       (void)pups_sighandle(SIGINT,"psrp_int_handler",(void *)&psrp_int_handler, (sigset_t *)NULL);
       (void)psrp_process_command(psrp_request);
    }
 
    if(psrp_log == TRUE)
    {  (void)fprintf(stdout,"\npsrp(protocol %5.2F): closing communication channel (%d transactions)\n\n",
                                                                              PSRP_PROTOCOL_VERSION,t_cnt);
       (void)fflush(stdout);
    }

    remove_junk();

    if(strcmp(psrp_c_code,"ok") != 0)
       pups_exit(255);
    pups_exit(0);
}



/*------------------------------*/
/* Callback version of readline */
/* which is signal safe         */
/*------------------------------*/

_PRIVATE _BOOLEAN newline      = FALSE;
_PRIVATE char     *cb_retline  = (char *)NULL;


/*----------------------------------------*/
/* Callback function called for each line */
/* when <return> is typed                 */
/*----------------------------------------*/
 
_PRIVATE void cb_linehandler (char *readline_buffer)
{
    rl_callback_handler_remove();

    if(readline_buffer != (char *)NULL)
    {  if(strcmp(readline_buffer,"quit") != 0   &&
          strcmp(readline_buffer,"exit") != 0   &&
          strcmp(readline_buffer,"bye" ) != 0   &&
	  strcmp(readline_buffer,""    ) != 0    )
       {  (void)history_truncate_file(psrp_history,PSRP_MAX_HISTORY);
          add_history (readline_buffer);
       }

       cb_retline = (char *)malloc(SSIZE);
       (void)strlcpy(cb_retline,readline_buffer,SSIZE);
       newline = TRUE;
    }

    (void)free(readline_buffer);
}




/*------------------------------------------*/
/* Version of readline which is signal safe */
/*------------------------------------------*/

_PRIVATE char *cb_readline(const char *prompt)
{
    sigset_t set,
             old_set;


    /*------------------------------------*/
    /* Make sure handler for SIGINT knows */
    /* we are in cb_readline()            */
    /*------------------------------------*/

    in_cb_readline = TRUE;


    /*-----------------------------------------*/
    /* Make sure signals are handled correctly */
    /*-----------------------------------------*/

    (void)sigfillset(&set);
    (void)sigdelset(&set, SIGINT);
    (void)sigdelset(&set, SIGTERM);
    (void)sigdelset(&set, SIGWINCH);


    /*-----------------------------------------*/
    /* Set the default locale values according */
    /* to environment variables                */
    /*-----------------------------------------*/

    (void)setlocale (LC_ALL, "");


    /*-----------------------*/
    /* Install line handler  */
    /*-----------------------*/

    rl_callback_handler_install(prompt, cb_linehandler);


    /*------------*/
    /* Event loop */
    /*------------*/

    newline = FALSE;
    (void)fflush(stdin);

    while(TRUE)
    {

        /*---------------*/
        /* Block signals */
        /*---------------*/

        (void)sigprocmask(SIG_BLOCK,&set, &old_set);


        /*-----------------------------------------------*/
        /* Handle SIGWINCH event (terminal size changed) */
        /*-----------------------------------------------*/

        if(window_resized == TRUE)
  	{  window_resized = FALSE;
           rl_resize_terminal();
        }


        /*---------*/
        /* Newline */
        /*---------*/

        if (newline == TRUE)
        {

           /*---------------------------*/
           /* Reset terminal parameters */
           /*---------------------------*/

           (void)sigprocmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);
           rl_callback_handler_remove();
           in_cb_readline = FALSE;
           return(cb_retline);
        }


        /*---------------------*/
        /* Read next character */
        /*---------------------*/

        if(pups_fd_has_data(fileno(stdin)) == TRUE)
           rl_callback_read_char();


        /*-----------------*/
        /* Restore signals */
        /*-----------------*/

        else
        {  (void)sigprocmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);

	   // MAO - was DELAY
           (void)pups_usleep(MEDIUM_DELAY);
        }
    }


    /*---------------------------*/
    /* Reset terminal parameters */
    /*---------------------------*/

    (void)sigprocmask(SIG_SETMASK,&old_set,(sigset_t *)NULL);
    rl_callback_handler_remove();
    in_cb_readline = FALSE;
    return((char *)NULL);
}




/*-----------------------------------------------------------------------------------*/
/* Routine to establish a PSRP connection with a remote server process and open up a */
/* dialogue with it                                                                  */
/*-----------------------------------------------------------------------------------*/

_PRIVATE void psrp_command_loop(void)

{   char promptc,
         expanded_request_line[SSIZE] = "";

    _BOOLEAN looper  = TRUE;


    /*----------------------------------------*/
    /* Process interrupt requests (from user) */
    /*----------------------------------------*/

    if(interactive_mode == TRUE)
       (void)pups_sighandle(SIGINT,"psrp_int_handler",(void *)&psrp_int_handler, (sigset_t *)NULL);

toploop:

    do {   int32_t ret,
                   chars,
                   state,
                   restore_signum;

           do {

                  /*-----------------------------------------*/
                  /* Re-entry point after SIGINT raised      */
                  /* Sometimes SIGCLIENT also re-enters here */
                  /* if segmentation restart action is set   */
                  /* appropriately                           */
                  /*-----------------------------------------*/

                  if(interactive_mode == TRUE)
                  {  (void)pupshold(ALL_PUPS_SIGS);
                     (void)pups_set_jump_vector();
                     (void)pups_sighandle(SIGINT,"psrp_int_handler",(void *)&psrp_int_handler, (sigset_t *)NULL);

                  }

                  if((restore_signum = sigsetjmp(command_loop_top,1)) > 0) 
                  {  
                     #ifdef PSRP_DEBUG
                     (void)fprintf(stderr,"IN RESTORE 1\n");
                     (void)fflush(stderr);
                     #endif /* PSRP_DEBUG */

                     if(pupshold_cnt > 0)
                       (void)pupsrelse(ALL_PUPS_SIGS);

                     restore_system_status(restore_signum); 


                     /*--------------------------------------------------------*/
                     /* Re-enable processing of interrupt requests (from user) */
                     /*--------------------------------------------------------*/

                     if(interactive_mode == TRUE)
                       (void)pups_sighandle(SIGINT,"psrp_int_handler",(void *)&psrp_int_handler, (sigset_t *)NULL);


                     /*-------------------------------------------------------------*/
                     /* This section of code is entered if we have an active server */
                     /* and it is not in the process of segmenting                  */
                     /*-------------------------------------------------------------*/

                     if(server_pid != (-1) && in_client_handler == FALSE) 
                     {  char buf[SSIZE] = "";

                        psrp_log           = FALSE;
                        processing_command = FALSE;
                        r_cnt              = 0;
                        initialise_repeaters();


                        /*-------------------------------------------------------------*/
                        /* Termination of commands of the form server[@host]: command  */
                        /* But note we ONLY terminate if vectoring to command_loop_top */
                        /* was the result of a SIGINT rather than a segmentation event */
                        /* This status is signified by in_client_handler == FALSE      */
                        /*-------------------------------------------------------------*/

                        if(flycom == TRUE)
                        {  
                           #ifdef PSRP_DEBUG
                           (void)fprintf(stdout,"SCONN:%d  FLYCOM: %d\n",server_connected,flycom);
                           (void)fflush(stdout); 
                           #endif /* PSRP_DEBUG */

                           flycom = FALSE;
                        }
                     }
                     else if(in_client_handler == TRUE && seg_restart_action != EXIT_ON_INTR)
                     {

                        /*-----------------------------------------------------*/
                        /* If this is a segmentation event restart any request */
                        /* interrupted by the segmentation                     */
                        /*-----------------------------------------------------*/

                        in_client_handler  = FALSE;
                        processing_command = FALSE;
                        r_cnt              = 0;
                        initialise_repeaters();

                        goto process_command;
                     }
                     else if(close_psrp_channel == TRUE)
                     {  close_psrp_channel = FALSE;


                        /*-----------------------------------------------------*/
                        /* Stop homeostatic monitoring of connection to server */
                        /*-----------------------------------------------------*/

                        (void)pups_clearvitimer("server_alive_handler");

                        client_in  = pups_fclose(client_in);
                        client_out = pups_fclose(client_out);
                     }
                  }
                  else 
                     (void)pupsrelse(ALL_PUPS_SIGS);

                  flycom             = FALSE;
                  in_client_handler  = FALSE;
                  processing_command = FALSE;


                  /*-----------------------*/
                  /* Get request from user */
                  /*-----------------------*/

prompt:           (void)pups_sighandle(SIGWINCH,"winch_handler",(void *)winch_handler,(sigset_t *)NULL); 


                  /*-----------------------------------------------------*/
                  /* Display # prompt (if PSRP connection is priviliged) */
                  /*-----------------------------------------------------*/

                  if(priviliged == TRUE)
                     promptc = '#';
                  else
                     promptc = '>';

                  (void)strlcpy(request_line,"",SSIZE);
                  (void)strlcpy(expanded_request_line,"",SSIZE);

                  in_prompt_loop = TRUE;

                  if(prompt == TRUE)
                  {  char prompt_str[SSIZE] = "",
                          remote[SSIZE]     = "";

                     if(is_remote == TRUE)
                        (void)strlcpy(remote,"remote ",SSIZE);
                     else
                        (void)strlcpy(remote,"",SSIZE);

                     if(redirected_requests != TRUE)
                     {  if(strcmp(psrp_server,"none") == 0)
                        {  promptc    = '>';
                           priviliged = FALSE;
                           (void)snprintf(prompt_str,SSIZE,"%s%s (no server open@%s:%s)> ",remote,appl_name,appl_host,appl_owner);
                        }
                        else
                        {  

                           /*-----------------------------------------------------*/
                           /* Display # prompt (if PSRP connection is priviliged) */
                           /*-----------------------------------------------------*/

                           if(priviliged == TRUE)
                              promptc = '#';
                           else
                              promptc = '>';

                           (void)snprintf(prompt_str,SSIZE,"%s%s %s (%d@%s:%s)%c ",remote,appl_name,psrp_server,server_pid,appl_host,appl_owner,promptc);
                        }
                     }


                     /*------------------------------------------*/
                     /* We MUST ensure that stdin is blocking    */
                     /* irrespective of whether it is a terminal */
                     /* FIFO or socket                           */
                     /*------------------------------------------*/

                     (void)fcntl(0,F_SETFL,fcntl(0,F_GETFL,0) & ~O_NONBLOCK);


                     /*-------------------*/
                     /* SIGCLIENT enabled */
                     /*-------------------*/

                     if(from_tstp_handler == FALSE)
                     {     pupsigrelse(SIGCLIENT);
                           (void)write(1,"\n",1);
                     }
                     else
                        from_tstp_handler = TRUE;

                     #ifdef HAVE_READLINE
                     if(isatty(0) == 1)
                     {  char history_line[SSIZE] = "",
                             *retline            = (char *)NULL;

                        retline = cb_readline(prompt_str);

                        if(retline != (char *)NULL && strempty(retline) == FALSE)
                           (void)strlcpy(request_line,retline,SSIZE);
                        (void)free((void *)retline);
                     }   
                     else
                     {   if(redirected_requests != TRUE)
                            (void)write(1,prompt_str,strlen(prompt_str));

                         (void)fgets(request_line,SSIZE,stdin);
                     }

                     #else
                     (void)fprintf(stdout,"%s",prompt_str);
                     (void)fflush(stdout);

                     (void)fgets(request_line,SSIZE,stdin);
                     #endif /* HAVE_READLINE */

                     (void)pupsighold(SIGCLIENT,TRUE);


                     /*-------------------------------------------------------*/
                     /* If we have exhausted data on stdin (from psrprc file) */
                     /* re-open stdin on /dev/tty for further interactive     */
                     /* input                                                 */
                     /*-------------------------------------------------------*/

                     if(feof(stdin) != 0)
                     {  FILE *stdin_stream = (FILE *)NULL; 


                        /*----------------------------------------------------------*/
                        /* If this client is slaved by a server we need to reattach */
                        /* the server stdin (rather than /dev/tty)                  */ 
                        /*----------------------------------------------------------*/

                        if(slave_in != (-1))
                        {  (void)dup2(slave_in,0);
                           (void)close(slave_in);

                           stdin_stream = stdin;
                           stdin_stream = fdopen(0,"r");
                        }
                        else
                        {  (void)fclose(stdin);
                           stdin = fopen("/dev/tty","r");
                        }

                        redirected_requests = FALSE;
                     }
                  }
                  else
                     (void)fgets(request_line,SSIZE,stdin);

                  in_prompt_loop = FALSE;


                  /*------------------------------------------------------------*/
                  /* Stop application wasting CPU time (when connected to FIFO) */
                  /*------------------------------------------------------------*/

                  (void)pups_usleep(DELAY);

              } while(request_line[0] == '\0' || request_line[0] == '#' || request_line[0] == '\n');

process_command:

              looper = psrp_process_command(request_line);
              (void)strlcpy(last_request_line,request_line,SSIZE);

        }  while(looper == TRUE);
}



/*--------------------------------------*/
/* Process next (compound) PSRP command */
/*--------------------------------------*/

_PRIVATE _BOOLEAN psrp_process_command(char *request_line)

{   uint32_t    i,
                try_cnt          = 0,
                req_cnt          = 0;

    char strdum[SSIZE]            = "",
         tmp_str[SSIZE]           = "",
         request[SSIZE]           = "",
         reply[SSIZE]             = "",
         target[SSIZE]            = "",
         server_password[SSIZE]   = "",
         psrp_string[SSIZE]       = "",
         psrp_server_info[SSIZE]  = "",
         toxic_1[SSIZE]           = "",
         toxic_2[SSIZE]           = "",
         toxic_3[SSIZE]           = "";

    _BOOLEAN ret,
             looper,
             process_new,
             server_password_set = FALSE;

    int32_t  status               = 0,
             dcnt                 = 0,
             restore_signum,
             isysc                = 0;

    FILE     *pstream             = (FILE *)NULL;
    struct   sigaction action;


    /*-----------------------------------------*/
    /* Autoclean patchboard directory removing */
    /* stale items                             */
    /*-----------------------------------------*/

    (void)psrp_virtual_maggot("/tmp");


    /*-------------------------------------------------------------------*/
    /* Possibly because we have a non-tty connection each line of input  */
    /* to remote server has a carriage return added to it - strip it off */
    /*-------------------------------------------------------------------*/

    if(is_remote == TRUE)
    {  if((int32_t)request_line[strlen(request_line) - 1] == 13)
          request_line[strlen(request_line) - 1] = '\0';
    }

    /*--------------------------*/
    /* Increase recursion count */
    /*--------------------------*/

    ++r_cnt;
    if(r_cnt == 1)
       initialise_repeaters();


    /*-------------------------------------*/
    /* Test macros are not nested too deep */
    /*-------------------------------------*/

    if(r_cnt == MAX_MACRO_DEPTH)
    {  if(pel_appl_verbose == TRUE)
       {  (void)fprintf(stdout,"\n%sERROR%s PML syntax pups_error: macros stacked too deep (max stack depth %d)\n\n",boldOn,boldOff,MAX_MACRO_DEPTH);
          (void)fflush(stdout);
       } 

       (void)strlcpy(psrp_c_code,"msderr",SSIZE);
       return(FALSE);
    }


    /*----------------------------------------------*/
    /* Strip any trailing white spaces from request */
    /*----------------------------------------------*/

    (void)strail(request_line,' ');

    (void)strlcpy(request,"",SSIZE);
    (void)strlcpy(reply,"",SSIZE);

    (void)m_strext(r_cnt,(unsigned char)MSTREXT_RESET_ALL,(char *)NULL,(char *)NULL);
    do {    looper = m_strext(r_cnt,';',tmp_str,request_line);
            process_new = TRUE;



            /*-----------------------------------------------*/
            /* Re-entry point after SIGCLIENT raised if user */
            /* has specified that interrupted request should */
            /* be restarted                                  */
            /*-----------------------------------------------*/

            (void)pups_set_jump_vector();
            if((restore_signum = sigsetjmp(process_loop_top,1)) > 0)
            {
                #ifdef PSRP_DEBUG
                (void)fprintf(stderr,"IN RESTORE 2\n");
                (void)fflush(stderr);
                #endif /* PSRP_DEBUG */

               restore_system_status(restore_signum);
            }

            if(looper == FALSE)
            {  --r_cnt;

               (void)pups_reset_jump_vector();
               return(TRUE);
            }

            (void)strlcpy(request,strpch(' ',tmp_str),SSIZE);
            (void)strail(request,' ');


            /*---------------------------------------------------*/
            /* Set automatic request retry mode (if server busy) */
            /*---------------------------------------------------*/

            if(strncmp(request,"retry",5) == 0)
            {  char strdum[SSIZE] = "",
                    mode[SSIZE]   = "";

               if(sscanf(request,"%s%s",strdum,mode) != 2)
               {  if(busy_retry_mode == TRUE)
                  {  if(pel_appl_verbose == TRUE)
                     {  (void)fprintf(stdout,"\nautomatic request retry mode currently enabled\n");
                        (void)fprintf(stdout,"will make %d attempts to pass current request to server (at 1000 millisecond intervals)\n\n",
                                                                                                                                  max_trys);
                     }
                  }

                  else if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"\nautomatic request retry mode currently disabled\n");
                     (void)fprintf(stdout,"will abort current request (if server currently busy)\n\n");
                  }
                  (void)fflush(stdout);

                  (void)strlcpy(psrp_c_code,"ok",SSIZE);
                  goto next_request;
               }

               if(strcmp(mode,"on") != 0 && strcmp(mode,"off") != 0)
               {  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"\nusage: retry [on | off]\n\n");
                     (void)fflush(stdout);

                     (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                     return(TRUE);
                  }

                  (void)strlcpy(psrp_c_code,"ok",SSIZE);
                  goto next_request;
               }

               else if(strcmp(mode,"on") == 0)
               {  busy_retry_mode = TRUE;

                  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"\nautomatic request retry mode enabled\n");
                     (void)fprintf(stdout,"will make %d attempts to pass current request to server (at 1000 millisecond intervals)\n\n",
                                                                                                                               max_trys);
                  }
               }
               else
               {  busy_retry_mode = FALSE;

                  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"\nautomatic request retry mode disabled\n");
                     (void)fprintf(stdout,"will abort current request (if server currently busy)\n\n");
                  }
               }
               (void)fflush(stdout);

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }


            /*--------------------------------------------------------------------------------------------*/
            /* PML label - used as a re-entry marker for the resume statement, "end" is a dummy statement */
            /* which marks the end of an expanded macro                                                   */
            /*--------------------------------------------------------------------------------------------*/

            if(request[0] == '%' || strncmp(request,"end",3) == 0)
            {  (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }


            /*------------------------------------------------------------------------*/
            /* PML pups_error handler - invokes target command (which may be a macro) */
            /* if psrp_req_pups_error is TRUE                                         */
            /*------------------------------------------------------------------------*/

            if(strncmp(request,"if",2) == 0)
            {  

               /*-----------------------------------------------------------*/
               /* Format of if command is: if <condition> <macro>|<command> */
               /* therefore if expects 2 arguments                          */
               /*-----------------------------------------------------------*/

               if(sscanf(request,"%s %s %s",strdum,c_code,target) != 3) 
               {  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"\n%sERROR%s PML syntax pups_error: \"if\" requires condition and command/macro target\n\n",boldOn,boldOff);
                     (void)fflush(stdout);
                  }

                  --r_cnt;

                  (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                  return(TRUE);
               }

               if(strcmp(psrp_c_code,c_code) == 0)
               {  

                  /*----------------------------------------------------------------*/
                  /* Condition code MUST be cleared before we process any recursive */
                  /* command or they will enter the pups_error handler erroneously  */ 
                  /*----------------------------------------------------------------*/

                  (void)strlcpy(psrp_c_code,"ok",SSIZE);
                  (void)strlcpy(request,target,SSIZE);
               }
               else
                  goto next_request;
           }

           if(strncmp(request,"exit",4) == 0    ||
              strncmp(request,"bye",3)  == 0    ||
              strncmp(request,"quit",4) == 0     )
           {  --r_cnt;


              #ifdef HAVE_READLINE
              /*--------------*/
              /* Save history */
              /*--------------*/

              (void)write_history(psrp_history);
              #endif /* HAVE_READLINE */

              (void)alarm(0);
              return(FALSE);
           }


           /*------------------------------------------------------------*/                                 
           /* If we are not trapping pups_errors and one has been raised */
           /* clear the pups_error and exit                              */
           /*------------------------------------------------------------*/                                 

           if(strcmp(request,"perror") != 0)
           {  if(strcmp(psrp_c_code,"ok") != 0)
              {   (void)strlcpy(psrp_c_code,"ok",SSIZE);

                  if(req_cnt > 0)
                  {  --r_cnt;
                    return(TRUE);
                  }
              }
           }


           /*----------------------------------------*/
           /* End - wind to (inserted) end statemant */
           /*----------------------------------------*/

            if(strncmp(request,"end",3) == 0)
            {  if(sscanf(request,"%s %s",tmp_str,tmp_str) > 1)
               {  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"\n%sERROR%s PML syntax pups_error: \"end\" does not have arguments\n\n",boldOn,boldOff);
                     (void)fflush(stdout);
                  }

                  --r_cnt;

                  (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                  return(TRUE);
               }
               else
               {  --r_cnt;
                  return(TRUE);
               }
            }

            if(strncmp(request,"pups_errorabort",10) == 0)
            {  if(sscanf(request,"%s%s",tmp_str,tmp_str) != 2)
               {  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"\nusage: pups_errorabort on|off\n\n");
                     (void)fflush(stdout);
                  }

                  (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                  return(TRUE);
               }
               else if(strcmp(tmp_str,"on") != 0 && strcmp(tmp_str,"off") != 0)
               {   if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"\nusage: pups_errorabort on|off\n\n");
                     (void)fflush(stdout);
                  }

                  (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                  return(TRUE);
               }

               if(strcmp(tmp_str,"on") == 0)
                  pups_error_abort = TRUE;
               else
                  pups_error_abort = FALSE;

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }


            /*-----------------------------------------*/
            /*  Pause - wait for user to type <return> */
            /*-----------------------------------------*/

            if(strncmp(request,"pause",5) == 0)
            {  if(sscanf(request,"%s %s",tmp_str,tmp_str) > 1)
               {  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"\nusage: pause\n\n");
                     (void)fflush(stdout);
                  }

                  --r_cnt;

                  (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                  goto next_request;
               }
               else
               {  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"press <return> to continue\n");
                     (void)fflush(stdout);
                  }
                  (void)fgets(tmp_str,SSIZE,stdin);
               }

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }


            /*----------------------------------*/
            /* Abort - return to command prompt */
            /*----------------------------------*/

            if(strncmp(request,"abort",5) == 0)
            {  if(sscanf(request,"%s %s",tmp_str,tmp_str) > 1)
               {  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"\nusage: abort\n\n");
                     (void)fflush(stdout);
                  }

                  --r_cnt;

                  (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                  goto next_request;
               }
               else
                  pups_sigvector(SIGALRM, &command_loop_top);
            }
                 

            /*------------------------------------------------*/
            /* Raise - raise psrp_c_code pups_error condition */
            /*------------------------------------------------*/

            if(strncmp(request,"raise",5) == 0)
            {  if(sscanf(request,"%s %s",tmp_str,psrp_c_code) != 2)
               {  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"\nusage: raise <condition code>\n\n");
                     (void)fflush(stdout);
                  }

                  --r_cnt;

                  (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                  goto next_request;
               }

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }


            /*-----------------------------*/
            /* Resume transaction at label */
            /*-----------------------------*/

            if(strncmp(request,"resume",6) == 0)
            {  char label[SSIZE] = ""; 

               if(sscanf(request,"%s %s",tmp_str,label) != 2)
               {  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"\nusage: resume <label>\n\n");
                     (void)fflush(stdout);
                  }

                  --r_cnt;

                  (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                  return(TRUE);
               }
               else
               {  if(label[0] != '%')
                  {  if(pel_appl_verbose == TRUE)
                     {  (void)fprintf(stdout,"\n%sERROR%s PML syntax pups_error: label for \"resume\" (%s) malformed\n\n",boldOn,boldOff,label);
                        (void)fflush(stdout);
                     }

                     --r_cnt;

                     (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                     return(TRUE);
                  } 
                  else
                  {  req_find(r_cnt,label,request_line);
                     (void)strlcpy(psrp_c_code,"ok",SSIZE);
                     goto next_request;
                  }
               }
           }


           /*------------------------------------------------------------------------*/
           /* Is this to be interpreted as a command (e.g. no attempt is to be made  */
           /* to expand it) -- this feature enables us to overload names (macros and */
           /* commands can have the same name without clashing)                      */
           /*------------------------------------------------------------------------*/

           if(strncmp(request,"atomic",6) == 0)
           {  int32_t  start_index = 0;

              while(request[start_index] != '\n' && request[start_index] != '\0' && request[start_index] != ' ')
                    ++start_index;

              if(request[start_index] == '\n' || request[start_index] == '\0')
              {  if(pel_appl_verbose == TRUE)
                 {  (void)fprintf(stdout,"\nusage: atomic <pdf>\n\n");
                    (void)fflush(stdout);

                    (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                    return(TRUE);
                 }

                 (void)strlcpy(psrp_c_code,"ok",SSIZE);
                 goto next_request;
              }

              macro_expansion = FALSE;
              (void)strlcpy(tmp_str,&request[start_index+1],SSIZE);
              (void)strlcpy(request,tmp_str,SSIZE);
           }


           /*------------------------------------------*/
           /* Set repeat period (for repeated command) */
           /*------------------------------------------*/

           if(strncmp(request,"rperiod",7) == 0)
           {  char    tmpstr[SSIZE]  = "";
              int32_t tmp_rpt_period = (-1);

              if(sscanf(request,"%s %s",tmpstr,tmpstr) != 2)
              {  if(pel_appl_verbose == TRUE)
                 {  (void)fprintf(stdout,"\nusage: rperiod <repeat period in seconds> | default\n\n");
                    (void)fflush(stdout);
                 }

                 (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                 goto next_request;
              }

              if(strcmp(tmpstr,"default") != 0 && (sscanf(tmpstr,"%d",&tmp_rpt_period) != 1 || tmp_rpt_period <= 0))
              {  if(pel_appl_verbose == TRUE)
                 {  (void)fprintf(stdout,"\nusage: rperiod <repeat period in seconds> | default\n\n");
                    (void)fflush(stdout);
                 }

                 (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                 goto next_request;
              }
              else if(strcmp(tmpstr,"default") == 0)
                 tmp_rpt_period = PSRP_RPT_DEFAULT;
              else if(tmp_rpt_period == (-1))
              {  if(pel_appl_verbose == TRUE)
                 {  (void)fprintf(stdout,"\nusage: rperiod <repeat period in seconds> | default\n\n");
                    (void)fflush(stdout);
                 }

                 (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                 goto next_request;
              }
 
              rpt_period = tmp_rpt_period;

              if(pel_appl_verbose == TRUE)
              {  if(rpt_period == PSRP_RPT_DEFAULT)
                    (void)fprintf(stdout,"\ninterval for \"repeat\" directive is %d seconds (default)\n\n",rpt_period);
                 else
                    (void)fprintf(stdout,"\ninterval for \"repeat\" directive is %d seconds\n\n",rpt_period);

                 (void)fflush(stdout);
              }

              (void)strlcpy(psrp_c_code,"ok",SSIZE);
              goto next_request;
           }
             

           /*----------------------------*/                 
           /* Is this a repeated request */
           /*----------------------------*/                 

           if(strncmp(request,"repeat",6) == 0)
           {  char tmp_str[SSIZE] = "";

              if(strcmp(request,"repeat") == 0)
              {  if(pel_appl_verbose == TRUE)
                 {  (void)fprintf(stdout,"\nusage: repeat [cnt] <PDF | inbuilt>\n\n");
                    (void)fflush(stdout);
                 }

                 --r_cnt;
                 (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                 return(TRUE);
              }

              do_repeat[r_cnt] = TRUE;
              (void)strlcpy(tmp_str,&request[ch_index(request,' ') + 1],SSIZE);

              if(sscanf(tmp_str,"%d",&repeat_count) == 1) 
              {  if(repeat_count <= 0)
                 {  if(pel_appl_verbose == TRUE)
                    {  (void)fprintf(stdout,"\n%sERROR%s PML syntax pups_error: illegal \"repeat\" modifier count to PSRP request\n\n",boldOn,boldOff);
                       (void)fflush(stdout);
                    }

                    --r_cnt;
                    (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                    return(TRUE);
                 }

                 (void)strlcpy(request,&tmp_str[ch_index(tmp_str,' ') + 1],SSIZE);
             }
             else
                (void)strlcpy(request,tmp_str,SSIZE);

           }
           else
              do_repeat[r_cnt] = FALSE;


           /*-----------------------------------*/
           /* Show body (if command is a macro) */
           /*-----------------------------------*/

           if(strncmp(request,"body",4) == 0)
           {  char tmp_str[SSIZE] = "";

              if(strcmp(request,"body") == 0)
              {  if(pel_appl_verbose == TRUE)
                 {  (void)fprintf(stdout,"\nusage: body <PML macro>\n\n");
                    (void)fflush(stdout);
                 }

                 --r_cnt;
                 (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                 goto next_request;
              }

              show_body = TRUE;
              (void)strlcpy(request,(char *)index((char *)request,' '),SSIZE);

           }
           else
              show_body = FALSE;


            /*-------------------*/
            /* Expand any macros */
            /*-------------------*/

            macros_expanded = FALSE;
            if(macro_expansion == TRUE)
            {  if(expand_macros_recursive(request,tmp_str) == FALSE)
               {  if(pel_appl_verbose == TRUE)
                   {  (void)fprintf(stdout,"\n%sERROR%s PML syntax pups_error: macro \"%s\" not found\n\n",boldOn,boldOff,request);
                      (void)fflush(stdout);
                   }

                   --r_cnt;
                   (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                   goto next_request;
                }
            }
            else
               macro_expansion = TRUE;


            /*------------------------------------------------------------*/
            /* If our request is a macro it may have been expanded into a */
            /* large aggregate of requests - process these recursively    */
            /*------------------------------------------------------------*/

            if(macros_expanded == TRUE)
            { 

               /*------------------------------------------------------------*/
               /* Body just prints out the body of the current macro - used  */
               /* for debugging purposes                                     */
               /*------------------------------------------------------------*/

               if(show_body == TRUE)
               {  (void)fprintf(stdout,request);
                  (void)fflush(stdout);

                  (void)strlcpy(psrp_c_code,"ok",SSIZE);
                  goto next_request;
               } 


               /*----------------------------------------------------*/
               /* If a repeat modifier has been detected, repeat the */
               /* macro                                              */
               /*----------------------------------------------------*/
 
               if(do_repeat[r_cnt] == TRUE)
               {  if(psrp_repeat_command(repeat_count,request) == FALSE)
                  {  --r_cnt;
                     return(TRUE);
                  }
               }
               else
               {  if(psrp_process_command(request) == FALSE)
                  {  --r_cnt;
                     return(TRUE);
                  }
               }               

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }

            #ifdef SSH_SUPPORT
            if(strncmp(request,"connect",7) == 0)
            {  if(builtin_connect_remote_client(request) == FALSE)
               {  (void)fprintf(stdout,"\n%sERROR%s failed to connect remote client\n\n",boldOn,boldOff);
                  (void)fflush(stdout);
               }
                
               (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
               goto next_request;
            }
            #endif /* SSH_SUPPORT */


            /*---------------------------------------------------------------------------------*/
            /* Process aggregates - that is commands of the form server[@host]: <command-list> */
            /*---------------------------------------------------------------------------------*/

            (void)strlcpy(request,tmp_str,SSIZE);

            if((request_type = expand_request(tmp_str,request,&flycom,psrp_host)) == PSRP_REQUEST_ERROR)
            {  (void)strlcpy(psrp_c_code,"psynerr",SSIZE); 
                goto next_request;
            }


            /*------------------------------------------------------------*/
            /* Request is local - but has been expanded into an aggregate */
            /* command                                                    */
            /*------------------------------------------------------------*/

            if(request_type == PSRP_LOCAL_AGGREGATE)
            {  des_t child_pipe[2]         = { [0 ... 1] = (-1) };

               char  strdum[SSIZE]         = "",
                     psrp_cmd[SSIZE]       = "",
                     server_str[SSIZE]     = "",
                     server_pid_str[SSIZE] = "";



               /*-------------------------------------------------------------*/
               /* An open server cannot be referenced in an aggregate command */
               /*-------------------------------------------------------------*/

               #ifdef PSRP_DEBUG
               (void)fprintf(stderr,"AGGREGATE CMD (request: %s current server: %s, pid %d)\n",request,psrp_server,server_pid);
               (void)fflush(stderr);
               #endif /* PSRP_DEBUG */


               (void)snprintf(server_pid_str,SSIZE,"%d",server_pid);
               (void)sscanf(request,"%s%s",strdum,server_str);
               if(strin(server_str,psrp_server) == TRUE || strin(server_str,server_pid_str) == TRUE)
               {  
                  if(strin(server_str,server_pid_str) == TRUE)
                     (void)fprintf(stdout,"\n%sERROR%s instance of PSRP server \"%s\" is open (and cannot be referenced within aggregate command)\n",boldOn,boldOff,psrp_server);
                  else
                     (void)fprintf(stdout,"\n%sERROR%s PSRP server \"%s\" is open (and cannot be referenced within aggregate command)\n",boldOn,boldOff,psrp_server);
                  (void)fflush(stdout);

                  (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                  goto next_request;
               }

 
               /*------------------------*/
               /* Do we have a password? */
               /*------------------------*/

               if(strcmp(appl_password,"notset") != 0)
               {  char tmpstr[SSIZE] = "";

                  (void)pipe(child_pipe);
                  (void)snprintf(tmpstr,SSIZE,"%s\n",appl_password);
                  (void)write(child_pipe[1],tmpstr,strlen(tmpstr));
                  (void)close(child_pipe[1]);
               }


               /*--------------------------------------*/
               /* Get pathname for psrp client command */
               /*--------------------------------------*/

               if(strccpy(psrp_cmd,pups_search_path("PATH",appl_bin_name)) == (-1))
                  pups_error("[psrp] failed to resolve ben path for psrp client");


               /*------------------------------------------------*/
               /* Fork off child process which will actually run */
               /* local aggregate command                        */
               /*------------------------------------------------*/


               #ifdef PSRP_DEBUG
               (void)fprintf(stderr,"PSRP COMMAND: \"%s\"\n",psrp_cmd);
               (void)fflush(stderr);
               #endif /* PSRP_DEBUG */


               /*--------------------*/
               /* Child side of fork */
               /*--------------------*/

               if((child_pid = pups_fork(FALSE,FALSE)) == 0)
               { 
                  if(isatty(0) == 1)
                  {  if(strcmp(appl_password,"notset") != 0)
                     {  (void)close(0);
                         (void)dup(child_pipe[0]);
                         (void)close(child_pipe[0]);

                         (void)execlp(psrp_cmd,appl_name,"-recursive","-password","-c",request,(char *)NULL);
                     }
                     else
                         (void)execlp(psrp_cmd,appl_name,"-recursive",            "-c",request,(char *)NULL);
                  }


                  /*--------------------------------------------------------------*/
                  /* We should not get here -- if we do an pups_error has occured */
                  /*--------------------------------------------------------------*/

                  pups_error("[psrp_process_command] failed to execute local slaved PSRP server (for aggregate command)");
                  exit(255);
               }


               /*---------------------*/
               /* Parent side of fork */
               /*---------------------*/

               else
               {

                  /*-----------------------------------------------------------*/
                  /* We need to be able to handle SIGINT in case the aggregate */
                  /* command is interrupted                                    */
                  /*-----------------------------------------------------------*/

                  if(interactive_mode == TRUE)
                     (void)pups_sighandle(SIGINT,"psrp_int_handler",(void *)&psrp_int_handler, (sigset_t *)NULL);


                  /*-------------------------------------------------*/
                  /* Has the aggregate command actually executed OK? */
                  /*-------------------------------------------------*/

                  (void)pupswaitpid(FALSE,child_pid,&status);
                  ret = WEXITSTATUS(status);


                  /*-----------------------------------*/
                  /* Restore default action for SIGINT */
                  /*-----------------------------------*/

                  if(interactive_mode == TRUE)
                     (void)pups_sighandle(SIGINT,"ignore",SIG_IGN, (sigset_t *)NULL);


                  /*-----------------------------------------------------*/
                  /* For some reason ssh gives an exit code of 255 when  */
                  /* PSRP server is terminated. This is a kludge, but it */
                  /* fixes the (unwanted) error message                  */
                  /*-----------------------------------------------------*/

                  if(ret != 0 && strin(request,"terminate") == FALSE && strin(request,"kill") == FALSE)
                  {  (void)fprintf(stdout,"\n%sERROR%s failed to execute command (on local slaved PSRP server)\n",boldOn,boldOff);
                     (void)fflush(stdout);

                     (void)strlcpy(psrp_c_code,"prexerr",SSIZE);
                     goto next_request;
                  }              
               }
                  
               goto next_request;
            }

            #ifdef SSH_SUPPORT
            /*---------------------------------*/
            /* Command to run on remote server */
            /*---------------------------------*/

            else if(request_type == PSRP_REMOTE_REQUEST)
            {  char ssh_remote_command[SSIZE] = "",
                    ssh_remote_env[SSIZE]     = "";

                int32_t ret,
                        child_pipe[2]         = { [0 ... 1] = (-1) };


              (void)psrp_exec_env(ssh_remote_env);
              (void)mchrep('\0',"\n",request);
              (void)snprintf(ssh_remote_command,SSIZE,"bash -c '%s; %s'",ssh_remote_env,request);


              #ifdef PSRP_DEBUG
              (void)fprintf(stderr,"REMOTE CMD: (%s)\n",ssh_remote_command);
              (void)fflush(stderr);
              #endif /* PSRP_DEBUG */


               /*------------------------*/
               /* Do we have a password? */
               /*------------------------*/

               if(strcmp(appl_password,"notset") != 0) 
               {  char tmpstr[SSIZE] = "";

                  (void)pipe(child_pipe);
                  (void)snprintf(tmpstr,SSIZE,"%s\n",appl_password);
                  (void)write(child_pipe[1],tmpstr,strlen(tmpstr));
                         (void)close(child_pipe[1]);
               }


               /*--------------------*/
               /* Child side of fork */
               /*--------------------*/

               if((child_pid = pups_fork(FALSE,FALSE)) == 0)
               {  

                  /*---------------------*/
                  /* Terminal connection */
                  /*---------------------*/

                  if(isatty(0) == 1)
                  {  if(ssh_compression == TRUE)
                        (void)execlp("ssh","ssh","-q","-C","-t","-l",ssh_remote_uname,psrp_host,ssh_remote_command,(char *)NULL);
                     else
                        (void)execlp("ssh","ssh","-q","-t","-l",     ssh_remote_uname,psrp_host,ssh_remote_command,(char *)NULL);
                  }


                  /*------------------------*/
                  /* No terminal connection */
                  /*------------------------*/

                  else
                  {  if(ssh_compression == TRUE)
                        (void)execlp("ssh","ssh","-q","-C","-l",ssh_remote_uname,psrp_host,ssh_remote_command,(char *)NULL);
                     else
                        (void)execlp("ssh","ssh","-q","-l",     ssh_remote_uname,psrp_host,ssh_remote_command,(char *)NULL);
                  }


                  /*--------------------------------------------------------------*/
                  /* We should not get here -- if we do an pups_error has occured */
                  /*--------------------------------------------------------------*/

                  pups_error("[psrp_process_command] failed to build ssh channel to remote host");
                  exit(255);
               }


               /*---------------------*/
               /* Parent side of fork */
               /*---------------------*/

               else

               {

                  /*--------------------------------------------------------*/
                  /* We need to be able to handle SIGINT in case the remote */
                  /* command is interrupted                                 */
                  /*--------------------------------------------------------*/

                  if(interactive_mode == TRUE)
                     (void)pups_sighandle(SIGINT,"psrp_int_handler",(void *)&psrp_int_handler, (sigset_t *)NULL);


                  /*----------------------------------------------------*/
                  /* Is the server actually running on the remote host? */
                  /*----------------------------------------------------*/

                  (void)pupswaitpid(FALSE,child_pid,&status);
                  ret = WEXITSTATUS(status);


                  /*-----------------------------------*/
                  /* Restore default action for SIGINT */
                  /*-----------------------------------*/

                  if(interactive_mode == TRUE)
                     (void)pups_sighandle(SIGINT,"ignore",SIG_IGN, (sigset_t *)NULL);

                  if(ret < 0)
                  {  (void)fprintf(stdout,"\n%sERROR%s failed to execute ssh command (on remote host \"%s\")\n",boldOn,boldOff,psrp_host);
                     (void)fflush(stdout);
                     
                     (void)strlcpy(psrp_c_code,"prexerr",SSIZE);
                     goto next_request;
                  }
               }

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }                                   
            #else
            else if(request_type == PSRP_REMOTE_REQUEST)
            {  if(pel_appl_verbose == TRUE)
               {  (void)strdate(date);
                  (void)fprintf(stdout,"%sERROR%s %s %s (%d@%s:%s): remote requests not supported(no socket support)\n",
                                                                                                         boldOn,boldOff,
                                                                           date,appl_name,appl_pid,appl_host,appl_owner);
                  (void)fflush(stdout);
               }

               (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
               goto next_request;
            }
            #endif /* SSH_SUPPORT */


            /*---------------------------------*/
            /* Repeat loop for atomic commands */
            /*---------------------------------*/

            if(do_repeat[r_cnt] == TRUE)
            {  if(psrp_repeat_command(repeat_count,request) == FALSE)
               {  --r_cnt;
                  return(TRUE);
               }

               (void)strlcpy(psrp_c_code,"ok",SSIZE); 
               goto next_request;
            }


            #ifdef SLAVED_COMMANDS
            /*-----------------------*/
            /* Send command to shell */
            /*-----------------------*/

            if(request[0] == '!')
            {  if(strcmp(shell,"false") == 0)
               {  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"\n%sERROR%s Interactive shells are not permitted from this PSRP client (client is slaved)\n\n",boldOn,boldOff);
                     (void)fflush(stdout);
                  }

                  (void)strlcpy(psrp_c_code,"psherr",SSIZE);
                  goto next_request;
               }

               if(strlen(request) < 2)
               {
                  if(isatty(0) == 1 && restricted_clist == FALSE)
                  {  exec_shell = TRUE;

                     #ifdef NO_INTERACTIVE_SHELLS
                     if(pel_appl_verbose == TRUE)
                     {  (void)fprintf(stdout,"\n%sERROR%s Interactive shells are not permitted from this PSRP client (no interactive shells allowed)\n\n",boldOn,boldOff);
                        (void)fflush(stdout);
                     }

                     (void)strlcpy(psrp_c_code,"psherr",SSIZE);
                     goto next_request;
                     #else  /* NO_INTERACTIVE_SHELLS */
                     (void)pups_system(shell,shell,PUPS_WAIT_FOR_CHILD | PUPS_ERROR_EXIT,(int32_t *)NULL);
                     #endif /* NO_INTERACTIVE_SHELLS */
                  }
                  else
                  {  if(pel_appl_verbose == TRUE)
                     {  (void)fprintf(stdout,"\n%sERROR%S Interactive remote shells are not supported for non pty (xrshd) connections (not running in terminal)\n\n",boldOn,boldOff);
                        (void)fflush(stdout);
                     }

                     (void)strlcpy(psrp_c_code,"psherr",SSIZE);
                     goto next_request;
                  }
               }
               else
               {  

                  /*--------------------------------------------------------------------*/
                  /* If we have a restricted command set filter the input to the shell. */
                  /* Only those commands which match the filter are permitted.          */
                  /*--------------------------------------------------------------------*/

                  if(restricted_clist == TRUE)
                  {  if(command_permitted(&request[1]) == FALSE)
                     {  if(pel_appl_verbose == TRUE)
                        {  (void)fprintf(stdout,"\n%sERROR%s command \"%s\" is not permitted (shell commands restricted)\n\n",boldOn,boldOff,&request[1]);
                           (void)fflush(stdout);
                        }

                        (void)strlcpy(psrp_c_code,"psherr",SSIZE);
                        goto next_request;
                     }
                  }


                  #ifdef NO_INTERACTIVE_SHELLS
                  /*------------------------------------------------------------*/
                  /* Has an interactive shell sneaked in through the back door? */
                  /*------------------------------------------------------------*/

                  if(strin(request,"sh") == TRUE && sscanf(request,"%s%s") == 1)
                  {  if(pel_appl_verbose == TRUE)
                     {  (void)fprintf(stdout,"\n%sERROR%s Interactive shells are not permitted from this PSRP client (back door interative shells not allowed)\n\n",boldOn,boldOff);
                        (void)fflush(stdout);
                     }
                  
                     (void)strlcpy(psrp_c_code,"psherr",SSIZE);
                     goto next_request;
                  }
                  #endif /* NO_INTERACTIVE_SHELLS */


                  /*-----------------------------------------------------------------------*/
                  /* If we have an '&' detach this command from the client's process group */
                  /* so keyboard signals sent to the client don't effect it                */
                  /*-----------------------------------------------------------------------*/

                  if(strin(request,"&") == TRUE)
                     (void)pups_system(&request[1],shell,PUPS_ERROR_EXIT | PUPS_STREAMS_DETACHED | PUPS_NEW_SESSION,(int32_t *)NULL);
                  else
                     (void)pups_system(&request[1],shell,PUPS_WAIT_FOR_CHILD | PUPS_ERROR_EXIT,(int32_t *)NULL);

                  exec_shell = FALSE;

                 (void)strlcpy(psrp_c_code,"ok",SSIZE); 
                 goto next_request;
             }
           }
           #endif /* SLAVED_COMMANDS */


            /*-----------------------*/
            /* PSRP builtin commands */
            /*-----------------------*/

            if(strncmp(request,"lcwd",4) == 0)
            {  char cwd[SSIZE] = "";

               if(sscanf(request,"%s%s",cwd,cwd) == 1)
                  (void)psrp_builtin_set_lcwd((char *)NULL);
               else
                  (void)psrp_builtin_set_lcwd(cwd);

               (void)strlcpy(psrp_c_code,"ok",SSIZE); 
               goto next_request;
            }

            if(strncmp(request,"wait",4) == 0)
            {  wait_for_psrp_server_startup = TRUE;

               if(pel_appl_verbose == TRUE)
               {  (void)fprintf(stdout,"\n%sWARNING%s PSRP client will wait for server startup (and then connect to it)\n\n",boldOn,boldOff);
                  (void)fflush(stdout);
               }

               (void)strlcpy(psrp_c_code,"ok",SSIZE); 
               goto next_request;
            }

            if(strncmp(request,"exit",4) == 0)
            {  exit_on_terminate = TRUE;

               if(pel_appl_verbose == TRUE)
               {  (void)fprintf(stdout,"\n%sWARNING%S PSRP client will exit (if attached PSRP server terminates)\n\n",boldOn,boldOff);
                  (void)fflush(stdout);
               }

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }

            if(strncmp(request,"nowait",4) == 0)
            {  wait_for_psrp_server_startup = FALSE;

               if(pel_appl_verbose == TRUE)
               {  (void)fprintf(stdout,"\n%sWARNING%S PSRP client will abort connection attempt (if server is not running)\n\n",boldOn,boldOff);
                  (void)fflush(stdout);
               }

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }     

            if(strin(request,"pager") == TRUE)
            {  uint32_t args;

               char     strdum[SSIZE] = "",
                        tmpstr[SSIZE] = "";

               args = sscanf(request,"%s %s",strdum,tmpstr);


               /*----------------------------------------------------*/
               /* Display usage information if requested or required */
               /*----------------------------------------------------*/
   
               if(args == 1                                                                ||
                  args == 2                                                                &&
                  (strcmp(datasink_cmd,"help") == 0 || strcmp(datasink_cmd,"usage") == 0)  &&
                  pel_appl_verbose == TRUE                                                  )
               {  (void)fprintf(stdout,"usage: pager [help | usage] | [on | off | default | <pager command:%s>]\n",DEFAULT_PAGER);
                  (void)fflush(stdout);

                  (void)strlcpy(psrp_c_code,"ok",SSIZE);
                  goto next_request;
               }


               /*---------------*/
               /* Disable pager */
               /*---------------*/

               else if(strcmp(tmpstr,"off") == 0)
               {  do_datasink = FALSE;
                  (void)pups_sighandle(SIGPIPE,"psrp_int_handler",(void *)psrp_int_handler,(sigset_t *)NULL);

                  (void)fprintf(stdout,"\npaging mode disabled\n\n");
                  (void)fflush(stdout);
               }


               /*------------------------------*/
               /* Use current or default pager */
               /*------------------------------*/

               else if(strcmp(tmpstr,"on") == 0 || strcmp(tmpstr,"default") == 0)
               {  do_datasink = TRUE;
                  (void)pups_sighandle(SIGPIPE,"ignore",SIG_IGN,(sigset_t *)NULL);
                  (void)pups_sighandle(SIGINT, "ignore",SIG_IGN,(sigset_t *)NULL);

                  (void)fprintf(stdout,"\npaging mode enabled (pager is \"%s\")\n\n",datasink_cmd);
                  (void)fflush(stdout);
               }


               /*-------------------*/
               /* Set pager command */
               /*-------------------*/
               /*---------*/
               /* Default */
               /*---------*/

               else if(strcmp(tmpstr,"default") == 0)
               {  do_datasink = TRUE;
                  (void)pups_sighandle(SIGPIPE,"ignore",SIG_IGN,(sigset_t *)NULL);
                  (void)pups_sighandle(SIGINT, "ignore",SIG_IGN,(sigset_t *)NULL);
                  (void)strlcpy(datasink_cmd,DEFAULT_PAGER,SSIZE);

                  (void)fprintf(stdout,"\npaging mode enabled (pager is \"%s\") [default]\n\n",datasink_cmd);
                  (void)fflush(stdout);
               }


               /*------*/
               /* Less */
               /*------*/

               else if(strcmp(tmpstr,"less") == 0)
               {  do_datasink = TRUE;
                  (void)pups_sighandle(SIGPIPE,"ignore",SIG_IGN,(sigset_t *)NULL);
                  (void)pups_sighandle(SIGINT, "ignore",SIG_IGN,(sigset_t *)NULL);
                  (void)strlcpy(datasink_cmd,"less",SSIZE);

                  (void)fprintf(stdout,"\npaging mode enabled (pager is \"%s\")\n\n",datasink_cmd);
                  (void)fflush(stdout);
               }


               /*------*/
               /* More */
               /*------*/

               else if(strcmp(tmpstr,"more") == 0)
               {  do_datasink = TRUE;
                  (void)pups_sighandle(SIGPIPE,"ignore",SIG_IGN,(sigset_t *)NULL);
                  (void)pups_sighandle(SIGINT, "ignore",SIG_IGN,(sigset_t *)NULL);
                  (void)strlcpy(datasink_cmd,"more",SSIZE);

                  (void)fprintf(stdout,"\npaging mode enabled (pager is \"%s\")\n\n",datasink_cmd);
                  (void)fflush(stdout);
               }


               /*-------*/
               /* usage */
               /*-------*/

               else
               {  

                  /*-----------------------*/
                  /* Display usage (error) */
                  /*-----------------------*/

                  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"usage: pager help | usage] | [on | off | default | <pager command:%s>]\n",DEFAULT_PAGER);
                     (void)fflush(stdout);
                  }

                  (void)strlcpy(psrp_c_code,"ok",SSIZE);
                  goto next_request;
               }

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }

            if(strin(request,"sink") == TRUE)
            {  uint32_t args;

               char     strdum[SSIZE]   = "",
                        tmpstr_1[SSIZE] = "",
                        tmpstr_2[SSIZE] = "";

               args = sscanf(request,"%s%s%s",strdum,tmpstr_1,tmpstr_2);
               if(args == 2 && (strcmp(datasink_cmd,"help") == 0 || strcmp(datasink_cmd,"usage") == 0) && pel_appl_verbose == TRUE)
               {  (void)fprintf(stdout,"usage: sink <datasink command> [bg]\n");
                  (void)fflush(stdout);

                  (void)strlcpy(psrp_c_code,"ok",SSIZE);
                  goto next_request;
               }
               else if(args == 2)
               {  (void)strlcpy(datasink_cmd,tmpstr_1,SSIZE);

                  if(args == 3 && strcmp(tmpstr_2,"bg") == 0)
                     do_bg_datasink = TRUE; 
                  else
                  {  (void)fprintf(stdout,"usage: sink <datasink command> [bg]\n");
                     (void)fflush(stdout);

                     (void)strlcpy(psrp_c_code,"ok",SSIZE);
                     goto next_request;
                  }
               }
               else if(args != 2)
               {  (void)fprintf(stdout,"usage: sink <datasink command> [bg]\n");
                  (void)fflush(stdout);

                  (void)strlcpy(psrp_c_code,"ok",SSIZE);
                  goto next_request;
               }

               if(do_datasink == TRUE)
                  do_datasink = FALSE;
               else
                  do_datasink = TRUE;

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }
 
            if(strin(request,"clean") == TRUE)
            {  uint32_t args,
                        n_deleted        = 0;

               char     strdum[SSIZE]    = "",
                        clean_cmd[SSIZE] = "",
                        directory[SSIZE] = "/tmp";

               args = sscanf(request,"%s %s",strdum,directory);

               if(strcmp(strdum,"clean") == 0)
               {  if(args > 2)
                  {  if(pel_appl_verbose == TRUE)
                     {  (void)fprintf(stdout,"usage: clean <PSRP channel directory>\n");
                        (void)fflush(stdout);
                     }

                     (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                     goto next_request;
                  }

                  if(args == 1)
                     n_deleted = psrp_virtual_maggot("/tmp");
                  else
                     n_deleted = psrp_virtual_maggot(directory);

                  if(pel_appl_verbose == TRUE)
                  {  if(n_deleted == 0)
                        (void)fprintf(stdout,"\nPSRP channel directory \"%s\" is clean (no stale items found)\n\n",directory);
                     else
                        (void)fprintf(stdout,"\nPSRP channel directory \"%s\" cleaned (%d stale items found)\n\n",directory,n_deleted);
                     (void)fflush(stdout);
                  }

                  (void)strlcpy(psrp_c_code,"ok",SSIZE);
                  goto next_request;
               }
            }

            if(strncmp(request,"cinit",5) == 0)
            {  rpt_curses_mode = TRUE;

               if(pel_appl_verbose == TRUE)
               {  (void)fprintf(stdout,"\nCurses mode enabled (for repeated commands)\n\n");
                  (void)fflush(stdout);
               }

               (void)strlcpy(psrp_c_code,"ok",SSIZE); 
               goto next_request;
            }

            if(strncmp(request,"cend",4) == 0)
            {  rpt_curses_mode = FALSE;

               if(pel_appl_verbose == TRUE)
               {  (void)fprintf(stdout,"\nCurses mode disabled (for repeated commands)\n\n");
                  (void)fflush(stdout);
               }

               (void)strlcpy(psrp_c_code,"ok",SSIZE); 
               goto next_request;
            }

            if(strncmp(request,"sleep",5) == 0)
            {  time_t delay;

               if(sscanf(request,"%s%d",tmp_str,&delay) != 2)
               {  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"\nusage: sleep <seconds>\n\n");
                     (void)fflush(stdout);
                  }

                  (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                  goto next_request;
               }
               else
                  pups_sleep(delay);

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }

            if(strncmp(request,"cls",3) == 0)
            {  cls();
               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }

            if(strncmp(request,"quiet",5) == 0)
            {  pel_appl_verbose      = FALSE;
               save_pel_appl_verbose = FALSE;
               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }

            if(strncmp(request,"squiet",5) == 0)
            {  server_verbose      = FALSE;
               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }

            if(strncmp(request,"noisy",5) == 0)
	    {  pel_appl_verbose      = TRUE;
               save_pel_appl_verbose = TRUE;
               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }

            if(strncmp(request,"snoisy",5) == 0)
	    {  server_verbose        = TRUE;
               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }

            if(strncmp(request,"perror",6) == 0 && pel_appl_verbose == TRUE)
            {  (void)fprintf(stdout,"pups_error code: %s",psrp_c_code);

               if(pups_error_abort == TRUE)
                  (void)fprintf(stdout," (pups_error abort %sON%s)\n",boldOn,boldOff);
               else
                  (void)fprintf(stdout," (pups_error abort %sOFF%s)\n",boldOn,boldOff);

               (void)fflush(stdout);

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }

            if(strncmp(request,"version",7) == 0)
            {  if(pel_appl_verbose == TRUE)
               {  _BOOLEAN is_vmtype       = FALSE;
		       
		  char     machtype[SSIZE] = "",
                           vmtype  [SSIZE] = "",
                           ostype  [SSIZE] = "",
                           hostname[SSIZE] = "";

                  (void)fprintf(stdout,"\nPSRP  interaction client %s (PSRP protocol %5.2F, PML version %5.2F)\n",
                                                                   PSRP_VERSION,PSRP_PROTOCOL_VERSION,PML_VERSION);

                  (void)fflush(stdout);


                  /*------------------------------------------*/
                  /* Use uname to get system information      */
                  /* environment variables are not realiable  */
                  /*------------------------------------------*/

                  (void)pups_sysinfo(hostname,ostype,(char *)NULL,machtype);


                  /*-------------------------------------------------------*/
                  /* Check to see if we are running in virtual environment */
                  /*-------------------------------------------------------*/

                  is_vmtype= pups_os_is_virtual(vmtype);
		  if(is_vmtype == TRUE)
                     (void)fprintf(stdout,"\n    Running in %s %s %s host (%s)\n",        machtype,ostype,vmtype,hostname);
                  else if(is_vmtype == FALSE)
                     (void)fprintf(stdout,"\n    Running on %s %s bare-metal host (%s)\n",machtype,ostype,hostname);
		  else
                     (void)fprintf(stdout,"\n    Running on %s %s unknown host\n",        machtype,ostype,hostname);           
                  (void)fflush(stdout);


		  /*----------------------------*/
		  /* Communication channel type */
		  /*----------------------------*/

                  (void)fprintf(stdout,"    Using named pipes (FIFO's) for PSRP channels\n\n");


		  /*-------------*/
		  /* String size */
		  /*-------------*/

		  (void)fprintf(stdout,"    String size [SSIZE] is %04d bytes\n\n",SSIZE);


		  /*-------------------------------*/
		  /* Floating point representation */
		  /*-------------------------------*/

                  #ifdef FLOAT
                  (void)fprintf(stdout,"    Floating point representation is single precision (%d bytes)\n\n",sizeof(FTYPE));
                  #else
                  (void)fprintf(stdout,"    Floating point representation is double precision (%d bytes)\n\n",sizeof(FTYPE));
                  #endif /* FLOAT */

                  (void)fflush(stdout);


		  /*---------------*/
		  /* OpenMP status */
		  /*---------------*/

                  #ifndef SINGLE_THREAD
                  #ifdef _OPENMP
                  (void)fprintf(stdout,"    %d parallel (OMP) threads available\n\n",omp_get_max_threads());
                  (void)fflush(stdout);
                  #endif /* _OPENMP       */
                  #else
                  (void)fprintf(stdout,"    OMP is disabled\n\n");
                  (void)fflush(stdout);
                  #endif /* SINGLE_THREAD */


		  /*---------------------------*/
		  /* PUPS/P3 supported modules */
		  /*---------------------------*/

                  #ifdef SHADOW_SUPPORT
                  (void)fprintf(stdout,"    [with shadow support                  ]\n");
                  #endif /* SHADOW_SUPPORT */

                  #ifdef NIS_SUPPORT
                  (void)fprintf(stdout,"    [with NIS support                     ]\n");
                  #endif /* NIS_SUPPORT */

                  #ifdef SSH_SUPPORT
                  (void)fprintf(stdout,"    [with ssh support                     ]\n");
                  #endif /* SSH_SUPPORT */

                  #ifdef DLL_SUPPORT
                  (void)fprintf(stdout,"    [with DLL support                     ]\n");
                  #endif /* DLL_SUPPORT */

                  #ifdef PTHREAD_SUPPORT
                  (void)fprintf(stdout,"    [with thread support                  ]\n");
                  #endif /* PTHREAD_SUPPORT */

                  #ifdef PERSISTENT_HEAP_SUPPORT
                  (void)fprintf(stdout,"    [with persistent heap support         ]\n");
                  #endif /* PERSISTENT_HEAP_SUPPORT */

                  #ifdef BUBBLE_MEMORY_SUPPORT
                  (void)fprintf(stdout,"    [with dynamic bubble memory support   ]\n");
                  #else
                  (void)fprintf(stdout,"    [with gnu malloc support              ]\n");
                  #endif /* BUBBLE_MEMORY_SUPPORT */

                  #ifdef CRIU_SUPPORT
                  (void)fprintf(stderr,"    [with state saving support            ]\n");
                  #endif /* CRIU_SUPPORT */

                  #ifndef NO_INTERACTIVE_SHELLS 
                  (void)fprintf(stdout,"    [with interactive shells              ]\n");
                  #endif /* NO_INTERACTIVE_SHELLS */

                  #ifdef SLAVED_COMMANDS
                  (void)fprintf(stdout,"    [with slaved commands                 ]\n");
                  #endif /* SLAVED_COMMANDS */

                  (void)fprintf(stdout,"\nCopyright (C), Tumbling Dice, 1994-2025 (gcc %s: built %s %s)\n\n",__VERSION__,__TIME__,__DATE__);
                  (void)fprintf(stdout,"PSRP is free software, covered by the GNU General Public License, and you are\n");
                  (void)fprintf(stdout,"welcome to change it and/or distribute copies of it under certain conditions.\n");
                  (void)fprintf(stdout,"See the GPL and LGPL licences at www.gnu.org for further details\n");
                  (void)fprintf(stdout,"PSRP comes with ABSOLUTELY NO WARRANTY\n\n");
                  (void)fflush(stdout);

                  #ifdef SANITIZE
		  (void)fprintf(stdout,"debug mode: [gcc sanitizer flags: bounds,address,leak,null]\n");
                  #endif /* SANITIZE */

                  (void)strlcpy(psrp_c_code,"ok",SSIZE);
                  goto next_request;
               }
               else
               {  (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                  goto next_request;
               }
            }

            if(strncmp(request,"id",4) == 0)
            {  if(pel_appl_verbose == TRUE)
               {  if(strcmp(appl_ttyname,"") != 0)
                     (void)fprintf(stdout,"\n    Client owner \"%s\" (uid:%d, gid:%d) (Controlling tty is \"%s\")\n",appl_owner,appl_uid,appl_gid,appl_ttyname);
                  else
                     (void)fprintf(stdout,"\n    Client owner \"%s\" (uid:%d, gid:%d) (No controlling tty)\n",appl_owner,appl_uid,appl_gid);
                  (void)fflush(stdout);

                  #ifdef SSH_SUPPORT
                  if(is_remote == TRUE)
                  {  char vmtype[SSIZE] = "";


                     /*----------------------------------------*/
                     /* Are we running in virtual environment? */
                     /*----------------------------------------*/

                     if(pups_os_is_virtual(vmtype) == TRUE)
                        (void)fprintf(stdout,"    this client is in a remote %s instance [from %s]\n",vmtype,remote_host_pathname);
                     else
                        (void)fprintf(stdout,"    this client is remote [from %s]\n",remote_host_pathname);

                     if(ssh_compression == TRUE)
                        (void)fprintf(stdout,"    [connected via compressing secure shell (ssh) protocols] [via port %s]\n",ssh_remote_port);
                     else
                        (void)fprintf(stdout,"    [connected via secure shell (ssh) protocols] [via port %s]\n",            ssh_remote_port);
                     (void)fflush(stdout);
                  }
                  else
                  #endif /* SSH_SUPPORT */
                  {  char vmtype[SSIZE] = "";


                     /*----------------------------------------*/
                     /* Are we running in virtual environment? */
                     /*----------------------------------------*/

                     if(pups_os_is_virtual(vmtype) == TRUE)
                        (void)fprintf(stdout,"    this client is local to %s instance \"%s\"\n",vmtype,appl_host);
                     else
                        (void)fprintf(stdout,"    this client is local to host \"%s\"\n",appl_host);
                     (void)fflush(stdout);

                  }

                  if(do_rsnr == TRUE && strcmp(psrp_server,"none") != 0)
                  {  (void)fprintf(stdout,"    %sWARNING%s reverse name resolution (for  attached server \"%s\" [%d@%s]): cannot track PSRP channel dynamically\n",
                                                                                                                                                    boldOn,boldOff,
                                                                                                                                  psrp_server,server_pid,appl_host); 
                  }
 
                  (void)strlcpy(psrp_c_code,"ok",SSIZE);
                  goto next_request;
               }
               else
               {  (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                  goto next_request;
               }
            }

            #ifdef PSRP_AUTHENTICATE 
            if(strncmp(request,"user",4) == 0)
            {  builtin_change_user(request);

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }

            if(strncmp(request,"password",8) == 0)
            {  (void)builtin_set_password(request,appl_password);

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }

            if(strncmp(request,"secure",6) == 0)
            {  if(builtin_set_password(request,server_password) == FALSE)
               {  (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                  goto next_request;
               }

               (void)snprintf(request,SSIZE,"secure %s",server_password);
               server_password_set = TRUE;

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }
            #endif /* PSRP_AUTHENTICATE */

            #ifdef SLAVED_COMMANDS
            if(strncmp(request,"clist",5) == 0)
            {  if(strcmp(appl_password,"notset") == 0 || psrp_get_password() == TRUE)
                  builtin_read_clist(TRUE,request);

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }
            #endif /* SLAVED_COMMANDS */


            if(strin(request,"patchboard") == TRUE)
            {  int32_t args                         = 0;
               char    patchboard_directory[SSIZE]  = "";

               args = sscanf(request,"%s %s",strdum,patchboard_directory);


               /*------*/
               /* Help */
               /*------*/
 
               if(args == 1)
               {  (void)fprintf(stdout,"usage: patchboard [<patchboard directory:%s>]\n",appl_fifo_dir);
                  (void)fflush(stdout);

                  (void)strlcpy(psrp_c_code,"ok",SSIZE);
                  goto next_request;
               }


               /*----------------*/
               /* Set patchboard */
               /*----------------*/

               else
               {

                  /*--------------------*/
                  /* Default patchboard */
                  /*--------------------*/

                  if(strcmp(patchboard_directory,"default") == 0)
                     (void)strlcpy(appl_fifo_dir,"/tmp",SSIZE);


                  /*-------------------------*/
                  /* User defined patchboard */
                  /*-------------------------*/

                  else if(access(patchboard_directory,F_OK | R_OK | W_OK) == 0)
                  {  struct stat buf;


                     /*---------------------------------------------*/
                     /* Check patchboard name refers to a directory */
                     /*---------------------------------------------*/
                  
                     (void)stat(patchboard_directory,&buf);
                     if(S_ISDIR(buf.st_mode))
                     {  (void)strlcpy(appl_fifo_dir,patchboard_directory,SSIZE);

                        (void)strlcpy(psrp_c_code,"ok",SSIZE);
                        goto next_request;
                     }
                  }
               
                  (void)strlcpy(psrp_c_code,"pysnerr",SSIZE);
                  goto next_request;
               }
            }

 
            if(strin(request,"chanstat") == TRUE)
            {  uint32_t args                    = 0,
                        chanstat_flags          = 0;

               char     strdum[SSIZE]           = "",
                        chanstat_flagstr[SSIZE] = "";

               args = sscanf(request,"%s %s %s",strdum, chanstat_flagstr);


               /*--------------------------------*/
               /* Single argument case (default) */
               /* show servers we own            */
               /*--------------------------------*/

               if(args == 1)
                  chanstat_flags = BUSY_SERVERS | IDLE_SERVERS;


               /*--------------------*/
               /* Two argument cases */
               /*--------------------*/
               /*-------------------------*/
               /* Public patchboard with  */
               /* used defined flags      */
               /*-------------------------*/

               else if(args == 2)
               {

                  /*----------------*/
                  /* Default filter */
                  /*----------------*/

                  if(strcmp(chanstat_flagstr,"default") == 0) 
                     chanstat_flags = BUSY_SERVERS | IDLE_SERVERS;


                  /*-----------------*/
                  /* Show everything */
                  /*-----------------*/

                  else if(strcmp(chanstat_flagstr,"all") == 0)
                     chanstat_flags = DEAD_SERVERS | NOWNED_SERVERS | VECTORED_SERVERS | BUSY_SERVERS | IDLE_SERVERS;


                  /*-----------------------------*/
                  /* Set individual filter flags */
                  /*-----------------------------*/

                  else
                  {  if(strin(chanstat_flagstr,"dead") == TRUE)
                        chanstat_flags |= DEAD_SERVERS;
                
                     if(strin(chanstat_flagstr,"nowned") == TRUE)
                        chanstat_flags |= NOWNED_SERVERS;

                     if(strin(chanstat_flagstr,"vectored") == TRUE)
                        chanstat_flags |= NOWNED_SERVERS;

                     if(strin(chanstat_flagstr,"busy") == TRUE)
                        chanstat_flags |= BUSY_SERVERS;

                     if(strin(chanstat_flagstr,"idle") == TRUE)
                        chanstat_flags |= IDLE_SERVERS;
                  }
               }


               /*--------------------*/
               /* Too many arguments */
               /*--------------------*/

               else
               {  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"usage: chanstat [all|live|busy|idle] [<directory>]\n");
                     (void)fflush(stdout);
                  }
   
                  (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                  goto next_request;
               }

               psrp_show_channels(chanstat_flags,appl_fifo_dir);

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }


            if(strin(request,"killall") == TRUE)
            {  uint32_t args;

               char     strdum[SSIZE]    = "",
                        directory[SSIZE] = "",
                        partname[SSIZE]  = "";

               args = sscanf(request,"%s %s %s",strdum,directory,partname);

               if(strcmp(strdum,"killall") == 0)
               {  if(args == 1 || args > 3)
                  {  if(pel_appl_verbose == TRUE)
                     {  (void)fprintf(stdout,"usage: killall <directory> <PSRP server spec>\n");
                        (void)fflush(stdout);
                     }

                     (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                     goto next_request;
                  }
                  else if(args == 1)
                     (void)strlcpy(directory,appl_fifo_dir,SSIZE);

                  psrp_kill_servers(directory,partname);

                  if(strcmp(appl_fifo_dir,"all") == 0)
                  {  if(strcmp(appl_fifo_dir,"/tmp") != 0)
                        psrp_kill_servers("/tmp","all");
                  }
                  else if(args == 2)
                  {  if(strcmp(appl_fifo_dir,"/tmp") != 0)
                        psrp_kill_servers("/tmp",partname);
                  }
                  else
                     psrp_kill_servers(directory,partname);

                  (void)strlcpy(psrp_c_code,"ok",SSIZE);
                  goto next_request;
               }
            }

            #ifdef DLL_SUPPORT
            if(strin(request,"dllpstat") == TRUE)
            {  uint32_t args;
               char     dll_pathname[SSIZE] = "";

               args = sscanf(request,"%s %s",dll_pathname,dll_pathname);
               if(args != 2)
               {  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"usage: dllstat <DLL pathname>\n");
                     (void)fflush(stdout);
                  }

                  (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                  goto next_request;
               }

               (void)pups_show_dll_orifices(stdout,dll_pathname);
               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }
            #endif /* DLL_SUPPORT */

            if(strncmp(request,"trys",4) == 0)
            {  if(sscanf(request,"%s %d",strdum,&max_trys) < 2)
               {  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"\n%sERROR%s must make at least %d attempts to complete PSRP requests\n\n",boldOn,boldOff,max_trys);
                     (void)fflush(stdout);
                  }

                  (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                  goto next_request;
               }

               if(pel_appl_verbose == TRUE)
               {  (void)fprintf(stdout,"\n%sWARNING%S will make %d attempts to complete PSRP requests\n\n",boldOn,boldOff,max_trys);
                  (void)fflush(stdout);
               }

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }
 
            if(strncmp(request,"open",4) ==  0)
            {  

               /*------------------------------------------------------*/
               /* We cannot open ourself - abort any attempt to do so! */
               /*------------------------------------------------------*/

               (void)snprintf(toxic_1,SSIZE,"open %s",             appl_name);
               (void)snprintf(toxic_2,SSIZE,"open %s@localhost",   appl_name);
               (void)snprintf(toxic_3,SSIZE,"open %s@%s",appl_name,appl_host);

               if(strin(request,toxic_1) == TRUE || strin(request,toxic_2) == TRUE || strin(request,toxic_3) == TRUE)
               {  if(strin(request,appl_host) == TRUE)
                     (void)fprintf(stdout,"\n%sERROR%s attempt to self intract [%s] -- aborting!\n\n",boldOn,boldOff,toxic_3);
                  else if(strin(request,"localhost") == TRUE)
                     (void)fprintf(stdout,"\n%sERROR%s attempt to self interact [%s] -- aborting!\n\n",boldOn,boldOff,toxic_2);
                  else
                     (void)fprintf(stdout,"\n%sERROR%s attempt to self interact [%s] -- aborting!\n\n",boldOn,boldOff,toxic_1);
                  (void)fflush(stderr);

                  (void)strlcpy(psrp_c_code,"soperr",SSIZE); 
                  goto next_request;
               }


               /*------------------------------------*/
               /* If we currently have a server open */
               /* close it                           */ 
               /*------------------------------------*/

               if(server_connected == TRUE)
               {  char tmpstr[SSIZE] = "";


                  /*--------------------------------------------*/
                  /* If server is already open -- simply return */
                  /*--------------------------------------------*/

                  (void)sscanf(request,"%s%s",tmpstr,tmpstr);
                      
                  if(strcmp(psrp_server,tmpstr) == 0)
                  {  if(pel_appl_verbose == TRUE)
                     {  (void)fprintf(stdout,"\n%sERROR%s PSRP server \"%s@%s\" is already open\n\n",boldOn,boldOff,psrp_server,psrp_host);
                        (void)fflush(stdout);
                     }

                     (void)strlcpy(psrp_c_code,"soperr",SSIZE);
                     goto next_request;
                  }

                  if(psrp_close_server(FALSE,TRUE) == (-1))
                  {  (void)strlcpy(psrp_c_code,"closerr",SSIZE);
                     goto next_request;
                  }

                  (void)strlcpy(psrp_server,"none",SSIZE);
                  server_connected   = FALSE;
                  psrp_log           = FALSE;
               }

               builtin_open_psrp_server(request);
               goto next_request;
            }

            #ifdef SSH_SUPPORT
            if(strncmp(request,"compress",7) == 0)
            {  char tmpstr[SSIZE]               = "",
                    new_ssh_remote_uname[SSIZE] = "";

               ssh_compression = FALSE;
    
               if((ret = sscanf(request,"%s%s%s",tmpstr,new_ssh_remote_uname,tmpstr)) == 2)
               {  if(strcmp(new_ssh_remote_uname,"on") == 0)
                     ssh_compression = TRUE;
                  else if(strcmp(new_ssh_remote_uname,"off") == 0)
                     ssh_compression = FALSE;
                  else
                     (void)strlcpy(ssh_remote_uname,new_ssh_remote_uname,SSIZE);
               }
               else if(ret == 1)
                  (void)strlcpy(ssh_remote_uname,appl_owner,SSIZE);
               else if(ret == 3)
               {  if(strcmp(tmpstr,"on") == 0)
                     ssh_compression = TRUE;
                  else if(strcmp(new_ssh_remote_uname,"off") == 0)
                     ssh_compression = FALSE;
                  else
                  {  if(pel_appl_verbose == TRUE)
                     {  (void)fprintf(stdout,"%sERROR%s \nexpecting \"on\" or \"off\" (%s)\n\n",boldOn,boldOff,tmpstr);
                        (void)fflush(stdout); 
                     }

                     (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                     return(TRUE);
                  }

                  (void)strlcpy(ssh_remote_uname,new_ssh_remote_uname,SSIZE); 
               }

               if(pel_appl_verbose == TRUE)
               {  if(ssh_compression == TRUE)
                     (void)fprintf(stdout,"\nssh compression \"%son%s\" [remote userid is \"%s\"]\n\n", boldOn,boldOff,ssh_remote_uname);
                  else
                     (void)fprintf(stdout,"\nssh compression \"%soff%s\" [remote userid is \"%s\"]\n\n",boldOn,boldOff,ssh_remote_uname);

                  (void)fflush(stdout);
              }

              (void)strlcpy(psrp_c_code,"ok",SSIZE);
              goto next_request;
            }

            if(strncmp(request,"connect",7) == 0)
            {  builtin_connect_remote_client(request);
               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }
            #endif /* SSH_SUPPORT */

            if(strncmp(request,"close",5) == 0 && server_connected == TRUE)
            {  if(psrp_close_server(FALSE,TRUE) == (-1))
               {  (void)strlcpy(psrp_c_code,"closerr",SSIZE);
                  goto next_request;
               }

               (void)strlcpy(psrp_server,"none",SSIZE);
               server_connected   = FALSE;
               psrp_log           = FALSE;

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }

            if(strncmp(request,"kill",4) == 0 && server_connected == TRUE)
            { 

               /*---------------------------------------*/
               /* Stop homeostatic monitoring of server */
               /* before we kill it                     */
               /*---------------------------------------*/

               (void)pups_clearvitimer("server_alive_handler");


               /*-------------------------*/
               /* Try to kill PSRP server */
               /*-------------------------*/

               if(kill(server_pid,SIGTERM) == (-1))
               {

                  /*--------------------------------------------------*/
                  /* If we fail to kill it better keep monitoring it! */
                  /*--------------------------------------------------*/

                  (void)pups_setvitimer("server_alive_handler",DELAY,VT_CONTINUOUS,DELAY,NULL,(void *)server_alive_handler);
                  (void)strlcpy(psrp_c_code,"killerr",SSIZE);
               }


               /*------------------------------------*/
               /* Success - report on servers demise */
               /* and clean up broken connection     */
               /*------------------------------------*/

               else
               {  (void)fprintf(stdout,"\n%sWARNING%s target PSRP server process %s (%d@%s) has been killed (by active client)\n\n",
                                                                                                                     boldOn,boldOff,
                                                                                                   psrp_server,server_pid,psrp_host);
                  (void)fflush(stdout);

                  (void)strlcpy(psrp_server,"none",SSIZE); 
                  server_connected = FALSE;
                  server_pid       = (-1);
                  psrp_log         = FALSE;

                  (void)strlcpy(psrp_c_code,"ok",SSIZE);
               }

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }

            if(strncmp(request,"segaction",9) == 0)
            {  uint32_t args;

               args  = sscanf(request,"%s%s",tmp_str,tmp_str);
               if(args == 1 && pel_appl_verbose == TRUE)
               {  switch(seg_restart_action)
                  {     case RESTART_INTR_MACRO: (void)fprintf(stdout,"\nMacros restarted on server segmentation\n\n");
                                                 break;

                        case RESTART_INTR_REQ:   (void)fprintf(stdout,"\nStalled request restarted on server segmentation\n\n");
                                                 break;

                        case EXIT_ON_INTR:       (void)fprintf(stdout,"\nRequest processing interrupted on server segmentation\n\n");
                                                 break;

                        default:                 break;
                  }
                  (void)fflush(stdout);
               }

               else if(args == 2)
               {  if(strncmp(tmp_str,"macro",5) == 0)
                     seg_restart_action = RESTART_INTR_MACRO;
                  else if(strncmp(tmp_str,"request",7) == 0)
                     seg_restart_action = RESTART_INTR_REQ;
                  else if(strncmp(tmp_str,"exit",4) == 0)
                     seg_restart_action = EXIT_ON_INTR;
                  else
                  {  (void)fprintf(stdout,"\nusage: segaction macro|request|exit\n\n");
                     (void)fflush(stdout);

                     (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                     goto next_request;
                  }

                  if(pel_appl_verbose == TRUE)
                  {  switch(seg_restart_action)
                     {     case RESTART_INTR_MACRO: (void)fprintf(stdout,"\nMacros restarted on server segmentation\n\n");
                                                    break;

                           case RESTART_INTR_REQ:   (void)fprintf(stdout,"\nStalled request restarted on server segmentation\n\n");
                                                    break;

                           case EXIT_ON_INTR:       (void)fprintf(stdout,"\nRequest processing interrupted on server segmentation\n\n");
                                                    break;

                           default:                 break;
                      }
                      (void)fflush(stdout);
                  }
               }

               else if(args > 2)
               {  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"\nusage: segaction macro|request|exit\n\n");
                     (void)fflush(stdout);
                  }

                  (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                  goto next_request;
               }

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }

            if(strncmp(request,"chelp",5) == 0 && pel_appl_verbose == TRUE)
            {  builtin_psrp_help();
               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }


            /*-------------------------------------------*/
            /* Open macro editor (to define a new macro) */
            /*-------------------------------------------*/

            if(strncmp(request,"medit",5) == 0)
            {  builtin_edit_macro_definitions(FALSE,FALSE,request);
               (void)strlcpy(psrp_c_code,"ok",SSIZE); 
               goto next_request;
            }

            if(strncmp(request,"mcload",7) == 0)
            {  builtin_load_macro_definitions(TRUE, FALSE,request);
               (void)strlcpy(psrp_c_code,"ok",SSIZE); 
               goto next_request;
            }


            /*--------------------------------------------------*/
            /* Append macro definitions from file (non-default) */
            /*--------------------------------------------------*/

            if(strncmp(request,"mcappend",9) == 0)
            {  builtin_load_macro_definitions(TRUE, TRUE,request);
               (void)strlcpy(psrp_c_code,"ok",SSIZE); 
               goto next_request;
            }


            /*---------------------------------------*/
            /* Load definition file noto macro stack */
            /*---------------------------------------*/

            if(strncmp(request,"mload",5) == 0)
            {  builtin_load_macro_definitions(FALSE, FALSE,request);
               (void)strlcpy(psrp_c_code,"ok",SSIZE); 
               goto next_request;
            }


            /*------------------------------------------------------*/
            /* Append macro definition file to (loaded) macro stack */
            /*------------------------------------------------------*/

            if(strncmp(request,"mappend",7) == 0)
            {  builtin_load_macro_definitions(FALSE, TRUE,request);
               (void)strlcpy(psrp_c_code,"ok",SSIZE); 
               goto next_request;
            }


            /*---------------------------------------------------*/
            /* Erase the macro file if the user has requested it */
            /*---------------------------------------------------*/

            if(strncmp(request,"mpurge",6) == 0)
            {  uint32_t args;

               args = sscanf(request,"%s %s %s",tmp_str,macro_f_name,tmp_str);


               /*----------------------------*/
               /* User pups_error - display usage */
               /*----------------------------*/

               if(args < 2)
               {  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"\nusage: mpurge <macro file>|all [erase]\n\n");
                     (void)fflush(stdout);
                  }

                  (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                  goto next_request;
               }


               /*-------------------------------------------------------------------------*/
               /* Two parameter command - remove macro(s) but do not delete corresponding */
               /* definition file(s)                                                      */
               /*-------------------------------------------------------------------------*/
 
               else if(args == 2)
               {  if(strcmp(macro_f_name,"all") == 0)
                     purge_macros((char *)NULL,TRUE,FALSE);
                  else
                     purge_macros(macro_f_name,TRUE,FALSE);
               }


               /*--------------------------------------------------------------------*/
               /* Three parameter command - remove macro(s) and delete corresponding */
               /* definition files                                                   */
               /*--------------------------------------------------------------------*/

               else if(args == 3)
               {  if(strcmp(macro_f_name,"all") == 0 && strcmp(tmp_str,"erase") == 0)
                     purge_macros((char *)NULL,TRUE,TRUE);
                  else if(strcmp(tmp_str,"erase") == 0)
                     purge_macros(macro_f_name,TRUE,TRUE);
               }


               /*---------------------------------*/
               /* User pups_error - display usage */
               /*---------------------------------*/

               else
               {  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"\nusage: mpurge <macro file>|all [erase]\n\n");
                     (void)fflush(stdout);
                  }

                  (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                  goto next_request;
               }

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }

            if(strncmp(request,"macros",6) == 0)
            {  show_macro_tags();
               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }

            if(strncmp(request,"linktype",8) == 0)
            {  uint32_t args;
               char     linkstate[SSIZE] = "";

               args = sscanf(request,"%s%s",linkstate,linkstate);
               if(args > 2)
               {  (void)fprintf(stdout,"\nusage: linktype [<hard | soft>]\n\n");
                  (void)fflush(stdout);

                  (void)strlcpy(psrp_c_code,"satterr",SSIZE);
                  goto next_request;
               }

               else if(args == 1)
               {  if(pel_appl_verbose == TRUE && psrp_hard_link == TRUE)
                     (void)fprintf(stdout,"\n%sWARNING%s PSRP channel hard linked: PSRP client will remain connected to stopped server\n\n",boldOn,boldOff);
                  else if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"\n%sWARNING%s PSRP channel soft linked: PSRP client will auto disconnect if connected server stopped\n",boldOn,boldOff);
                     (void)fprintf(stdout,"                                 and will refuse to open channels to stopped servers\n\n");
                  }

                  (void)fflush(stdout);

                  (void)strlcpy(psrp_c_code,"ok",SSIZE);
                  goto next_request;
               }

               if(strcmp(linkstate,"hard") == 0)
               {  psrp_hard_link = TRUE;

                  if(pel_appl_verbose== TRUE)
                  {  (void)fprintf(stdout,"\n%sWARNING%s PSRP channel hard linked: PSRP client will remain connected to stopped server\n\n",boldOn,boldOff);
                     (void)fflush(stdout);
                  }
               }

               else
               {  if(strcmp(linkstate,"soft") == 0)
                  {  psrp_hard_link = FALSE;

                     if(pel_appl_verbose == TRUE)
                     {  (void)fprintf(stdout,"\n%sWARNING%s PSRP channel soft linked: PSRP client will auto disconnect if connected server stopped\n");
                        (void)fprintf(stdout,"                                and will refuse to open channels to stopped servers\n\n");
                     }
                  }
                  else
                  {  if(pel_appl_verbose == TRUE)
                     {  (void)fprintf(stdout,"\nusage: linktype <hard | soft>\n\n");
                        (void)fflush(stdout);
                     }

                     (void)strlcpy(psrp_c_code,"satterr",SSIZE);
                     goto next_request;
                  }
               }

               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }


            /*--------------------------------------------------------------------*/
            /* Commands from this point on assume that we have a server connected */
            /* and ready to process PSRP commands.                                */
            /*--------------------------------------------------------------------*/

            if(server_connected == FALSE)
            {  if(pel_appl_verbose == TRUE)
               {  if(secure_operation == FALSE)
                     (void)fprintf(stdout,"%sERROR%s no server attached - cannot process PSRP command (%s)\n\n",boldOn,boldOff,request);
                  else
                     (void)fprintf(stdout,"%sERROR%S no server attached - cannot complete (secure) operation (%s) on server\n\n",boldOn,boldOff,request);
                  (void)fflush(stdout);
               }

               secure_operation = FALSE;
               (void)strlcpy(psrp_c_code,"satterr",SSIZE);
               goto next_request;
            }

            if(strncmp(request,"log",3) == 0)                                              
            {  if(pel_appl_verbose == TRUE)
               {  (void)fprintf(stdout,"\nPSRP request logging enabled\n\n");
                  (void)fflush(stdout);
               }

               log_state_updated = TRUE;
               psrp_log          = TRUE;
            }           
    
            if(strncmp(request,"nolog",5) == 0)
            {  if(pel_appl_verbose == TRUE)
               {  (void)fprintf(stdout,"\nPSRP request logging disabled\n\n");
                  (void)fflush(stdout);
               }

               log_state_updated = TRUE;                                                   
               psrp_log          = FALSE;                      
            }

            if(strin(request,"segcnt") == TRUE && pel_appl_verbose == TRUE)
            {  if(server_seg_cnt == 0)
                  (void)fprintf(stdout,"server \"%s\" is not segmented\n",psrp_server);
               else
                  (void)fprintf(stdout,"server \"%s\" segment count of %d\n",psrp_server,
                                                                          server_seg_cnt);
               (void)fflush(stdout);
 
               (void)strlcpy(psrp_c_code,"ok",SSIZE);
               goto next_request;
            }


            /*-------------------------------------------------------------------*/
            /* If we are about to overlay we must stop monitoring current server */
            /*-------------------------------------------------------------------*/

            if(strncmp(request,"overlay",7) == 0)
            {  char request_tail[SSIZE] = "";

               if(strccpy(request_tail,strin2(request,"-pen")) != (char *)NULL)
                  (void)sscanf(request_tail,"%s",new_psrp_server);
               else
                  (void)sscanf(request,"%s %s",new_psrp_server,new_psrp_server);

               new_server_pid = server_pid;
            }       
 

            /*----------------------------------*/
            /* Retry command if server was busy */
            /*----------------------------------*/
            /*------------------------------------------------------*/
            /* We cannot open ourself - abort any attempt to do so! */
            /*------------------------------------------------------*/

            (void)snprintf(toxic_1,SSIZE,"open %s",             appl_name);
            (void)snprintf(toxic_2,SSIZE,"open %s@localhost",   appl_name);
            (void)snprintf(toxic_3,SSIZE,"open %s@%s",appl_name,appl_host);

            if(strin(request,toxic_1) == TRUE || strin(request,toxic_2) == TRUE || strin(request,toxic_3) == TRUE)
            {  if(strin(request,appl_host) == TRUE)
                  (void)fprintf(stdout,"\n%sERROR%s attempt to self interact [%s] -- aborting!\n\n",boldOn,boldOff,toxic_3);
               else if(strin(request,"localhost") == TRUE)
                  (void)fprintf(stdout,"\n%sERROR%s attempt to self interact [%s] -- aborting!\n\n",boldOn,boldOff,toxic_2);
               else
                  (void)fprintf(stdout,"\n%sERROR%s attempt to self interact [%s] -- aborting!\n\n",boldOn,boldOff,toxic_1);
               (void)fflush(stderr);

               (void)strlcpy(psrp_c_code,"psynerr",SSIZE); 
               goto next_request;
            }


busy_retry:

            try_cnt = 0;


            /*------------------------------------------------------------------*/
            /* Grab the PSRP channel so we have the servers (metaphorical) ear! */
            /*------------------------------------------------------------------*/

            if(psrp_grab_channel() == (-1))
            {  (void)strlcpy(psrp_c_code,"psynerr",SSIZE); 
               goto next_request;
            }


            /*---------------------------------------------------------------------*/
            /* Force pause if change of server while current command is processing */
            /*---------------------------------------------------------------------*/

            processing_command = TRUE;

            if(request[0] != '\0')
            {  int32_t ret;
               pid_t   current_server_pid;


               /*--------------------------------------*/
               /* Test to see if server process exists */
               /*--------------------------------------*/

               if(do_rsnr == FALSE)
                  current_server_pid = psrp_channelname_to_pid(appl_fifo_dir,psrp_server,psrp_host);
               else
                  current_server_pid = server_pid;

               ret = pups_statkill(server_pid,SIGCHAN);
               if(ret != PUPS_EXEC)
               {

                  /*--------------------------------------------------------------------------*/
                  /* Has the server really terminated -- or has it actually migrated?         */
                  /* If it has migrated  psrp_migrate_client_to_server_host() will not return */
                  /*--------------------------------------------------------------------------*/

                  if(psrp_migrate_client_to_server_host() == 0)
                  {

                     /*------------------------*/
                     /* It has been terminated */
                     /*------------------------*/

                     if(pel_appl_verbose == TRUE)
                     {  if(ret == PUPS_TERMINATED)
                        {  (void)fprintf(stdout,"\n%sWARNING%s target PSRP server process %s (%d@%s) has been terminated\n\n",
                                                                                                               boldOn,boldOff,
                                                                                             psrp_server,server_pid,psrp_host);
                          (void)fflush(stdout);
                        }

                        else if(ret == PUPS_STOPPED && psrp_hard_link == FALSE)
                        {  (void)fprintf(stdout,"\n%sWARNING%s target PSRP server process %s (%d@%s) has been stopped\n\n",
                                                                                                            boldOn,boldOff,
                                                                                          psrp_server,server_pid,psrp_host);
                           (void)fflush(stdout);
                        }
                     }
                  }


                  /*------------------------------------------------*/
                  /* Tell server to perform a disconnect on restart */
                  /* if it is stopped                               */
                  /*------------------------------------------------*/

                  if(ret == PUPS_STOPPED && psrp_hard_link == FALSE)
                     (void)kill(server_pid,SIGCLIENT);

                  (void)strlcpy(psrp_server,"none",SSIZE);
                  server_connected   = FALSE;
                  server_seg_cnt     = 0;
                  processing_command = FALSE;
                  (void)strlcpy(psrp_c_code,"stermerr",SSIZE);


                  /*-----------------------------------------------------*/
                  /* Stop homeostatic monitoring of connection to server */
                  /*-----------------------------------------------------*/

                 (void)pups_clearvitimer("server_alive_handler");


                  /*-----------------------------------*/
                  /* CLear curses mode (if nessessary) */
                  /*-----------------------------------*/

                  cend();

                  (void)strlcpy(psrp_c_code,"ptermerr",SSIZE);
                  goto next_request;
               }
               else
                  server_pid = current_server_pid;


               /*------------------------*/
               /*  Do logging if enabled */
               /*------------------------*/

               if(psrp_log == TRUE && log_state_updated == FALSE && pel_appl_verbose == TRUE)
               {  (void)fprintf(stdout,"\nServer process %s (%d@%s) datagram connection via (full-duplex) channel %s\n\n",
                                                                            psrp_server,server_pid,psrp_host,channel_name);
                  (void)fflush(stdout);
               }


               /*--------------------------------------*/
               /* Wait for server handshake - tells us */
               /* that SIGCHAN has been processed      */
               /*--------------------------------------*/

               psrp_waitfor_endop("EOP chan");


               /*-----------------------------------*/
               /* Tell server we want to talk to it */
               /*-----------------------------------*/

               (void)kill(server_pid,SIGPSRP);


               /*------------------------------------------*/
               /* Negotiate connection with server process */
               /*------------------------------------------*/

               is_builtin = FALSE;

               while(fgets(psrp_string,SSIZE,client_in) == (char *)NULL)
                    (void)pups_usleep(DELAY);


               /*--------------*/
               /* Lock channel */
               /*--------------*/

               (void)lockf(fileno(client_in),F_LOCK,0);


               /*------------------------*/
               /* Pass request to server */
               /*------------------------*/

               #ifdef PSRP_DEBUG
               (void)fprintf(stderr,"REQUEST OUT %s\n",request);
               (void)fflush(stderr);
               #endif /* PSRP_DEBUG */


               (void)fprintf(client_out,"%s\n",request);
               (void)fflush(client_out);


               /*------------------------------------*/
               /* Activate (datasink if appropriate) */
               /*------------------------------------*/

               if(do_datasink == TRUE)
               {  if((pstream = popen(datasink_cmd,"w")) == (FILE *)NULL)
                  {  (void)fprintf(stderr,"\n%sERROR%s failed to execute datasink command \"%s\"\n\n",boldOn,boldOff,datasink_cmd);
                     (void)fflush(stderr);

                     (void)kill(appl_pid,SIGINT); 
                  }

                  (void)pups_sighandle(SIGTSTP,"default",SIG_DFL,(sigset_t *)NULL);


                  /*--------------------------------------------*/
                  /* Send PSRP into the background if specified */
                  /*--------------------------------------------*/

                  if(do_bg_datasink == TRUE)
                     (void)kill(appl_pid,SIGTSTP);
              }

              do {

                     /*---------------------------------------*/
                     /* Get response from PSRP server process */
                     /*---------------------------------------*/

                     if (strcmp(request,"terminate") != 0) 
                     {  sigset_t set;

                        (void)strlcpy(reply,"",SSIZE); 
                        (void)fgets(reply,SSIZE,client_in);


                        /*----------------------------*/
                        /* make sure we can interrupt */
                        /*----------------------------*/

                        (void)sigemptyset(&set);
                        (void)sigaddset(&set,SIGINT);
                        (void)sigaddset(&set,SIGTERM);
                        (void)sigprocmask(SIG_UNBLOCK,&set,NULL);


                        if(do_repeat[r_cnt] == TRUE && strin(reply,"Illegal") == TRUE)
                        {  do_repeat[r_cnt] = FALSE;
                           (void)strlcpy(psrp_c_code,"sillerr",SSIZE);
                        }
                     }


                     /*----------------------------------------------*/
                     /* Has PSRP server terminated? If so detach and */
                     /* and return to prompt mode                    */
                     /*----------------------------------------------*/

                     else
                     {  

                        /*------------*/
                        /* Disconnect */
                        /*------------*/

                        (void)pups_clearvitimer("server_alive_handler");
                        if(pel_appl_verbose == TRUE)
                        {  (void)fprintf(stdout,"%sWARNING%s connection closed (server termination by active client)\n\n",boldOn,boldOff);
                           (void)fflush(stdout);
                        }

                        server_seg_cnt     = 0;
                        psrp_log           = FALSE;
                        server_connected   = FALSE;
                        (void)strlcpy(psrp_c_code,"scliterr",SSIZE);

                        (void)strlcpy(psrp_server,"none",SSIZE);


                        /*-----------------------------------------------------*/
                        /* If we are slaved propagate CST (server termination) */
                        /* flag to remote peer                                 */
                        /*-----------------------------------------------------*/

                        if(psrp_slaved == TRUE)
                        {  (void)fprintf(stdout,"CST\n");
                           (void)fflush(stdout);
                        }
                          
                        (void)strlcpy(psrp_c_code,"ok",SSIZE); 
                        goto next_request;
                     }


                     /*--------------------------------------------------------*/
                     /* EOT safail means we have a security violation -- abort */
                     /* connection to PSRP server.                             */
                     /*--------------------------------------------------------*/

                     if(strncmp(reply,"EOT safail",10) == 0)
                     {  if(psrp_close_server(FALSE,TRUE) == (-1))
                        {  (void)strlcpy(psrp_c_code,"closerr",SSIZE);
                           goto next_request;
                        }

                        (void)strlcpy(psrp_server,"none",SSIZE);
                        server_connected   = FALSE;
                        psrp_log           = FALSE;

                        (void)strlcpy(psrp_c_code,"safailerr",SSIZE);
                        goto next_request;
                     }


                     /*------------------------------*/
                     /* Write server reply to stdout */
                     /*------------------------------*/

                     if(strncmp(reply,"EOT",3) != 0 && log_state_updated == FALSE)
                     {  if(server_verbose == TRUE)
                        {
                           #ifdef HAVE_CURSES
                           if(rpt_curses_mode == TRUE && in_repeat_command == TRUE)
                           {  if(do_datasink == TRUE)
                              {  (void)fprintf(pstream,"%s",reply);
                                 (void)fflush(pstream);
                              }
                              else
                                 (void)printw("%s",reply);
                           }
                           else
                           #endif /* HAVE_CURSES */

                           {  if(do_datasink == TRUE)
                              {  (void)fprintf(pstream,"%s",reply);
                                 (void)fflush(pstream);
                              }
                              else
                              {  (void)fprintf(stdout,"%s",reply);
                                 (void)fflush(stdout);
                              }
                           }
                        }
                     }


                     /*------------------------------------------------------*/
                     /* If we are slaved propagate EOT (end of transmission) */
                     /* flag to remote peer                                  */
                     /*------------------------------------------------------*/

                     if(strncmp(reply,"EOT",3) == 0 && psrp_slaved == TRUE)
                     {  (void)fprintf(stdout,"EOT\n");
                        (void)fflush(stdout);
                     }

                 }  while(strncmp(reply,"EOT",3) != 0);


                 /*------------------------------------*/
                 /* If datasink is active terminate it */
                 /*------------------------------------*/

                 if(do_datasink == TRUE)
                 {  (void)pclose(pstream);
                    (void)pups_sighandle(SIGTSTP,"tstp_handler",(void *)tstp_handler, (sigset_t *)NULL);
                 }


                 /*-------------------------------------------*/
                 /* Extract PSRP service function return code */
                 /*-------------------------------------------*/

                 (void)sscanf(reply,"%s%s",tmp_str,psrp_c_code);

                 #ifdef PSRP_DEBUG
                 (void)fprintf(stdout,"PSRP CODE RETURNED: %s (%s)\n",psrp_c_code,reply);
                 (void)fflush(stdout);
                 #endif /* PSRP_DEBUG */

                 if(log_state_updated == TRUE)
                    log_state_updated = FALSE;
                 else
                 {  if(psrp_log == TRUE) 
                    {  (void)fprintf(stdout,"\nServer process %s (%d@%s) EOT sent\n\n",
                                          psrp_server,server_pid,psrp_host,psrp_string);
                       (void)fflush(stdout);
                    }
                 }

                 ++t_cnt;


                 /*----------------------------------------------------------*/
                 /* Wait for server to tell us that SIGPSRP has been handled */
                 /*----------------------------------------------------------*/

                 psrp_waitfor_endop("EOP psrp");


                 /*---------------*/
                 /* Clear channel */
                 /*---------------*/

                 (void)psrp_empty_fifo(fileno(client_in));
                 (void)psrp_empty_fifo(fileno(client_out));

                 processing_command = FALSE;


                 /*-------------------------------*/
                 /* Enable other attached clients */
                 /* to submit requests            */
                 /*-------------------------------*/

                 psrp_yield_channel();


                 /*-------------------------------------------------------*/
                 /* If our server has returned busy lets do it all again! */
                 /*-------------------------------------------------------*/

                 if(strcmp(psrp_c_code,"busy") == 0)
                 {  (void)fprintf(stdout,"psrp: retrying request (%s)\n",request);
                    (void)fflush(stdout);

                    if(busy_retry_mode == TRUE && try_cnt++ < max_trys)
                    {  (void)pups_usleep(DELAY);
                       goto busy_retry;
                    }
                 }


                 /*--------------------------------------------*/
                 /* Have we failed to get authorisation token? */
                 /*--------------------------------------------*/

                 if(strcmp(psrp_c_code,"autherr") == 0)
                    priviliged = FALSE;


                 /*----------------*/
                 /* Unlock channel */
                 /*----------------*/

                 (void)lockf(fileno(client_in),F_ULOCK,0);
             }


             /*------------------------------------------------------------------------*/
             /* Update local password (if last request updated server secure password) */
             /*------------------------------------------------------------------------*/

             if(server_password_set == TRUE)
             {  server_password_set = FALSE;
                (void)strlcpy(appl_password,server_password,SSIZE);
             }

next_request: ++req_cnt;

             if(psrp_slaved == TRUE)
             {  (void)fprintf(stdout,"EOT\n");
                (void)fflush(stdout);
             }

             if(pups_error_abort == TRUE && strcmp(psrp_c_code,"ok") != 0)
             {  if(interactive_mode == FALSE)
                   pups_exit(255);
                else
                   pups_sigvector(SIGALRM,&command_loop_top);
             }

       } while(looper == TRUE);

    (void)pups_reset_jump_vector(); 
    processing_command = FALSE;
    --r_cnt;

    return(TRUE);
}




/*-----------------------------------------------------*/
/* Handler for SIGCRITICAL (toggle handling of SIGINT) */ 
/*-----------------------------------------------------*/

_PRIVATE int32_t critical_handler(const int32_t signum)

{   _IMMORTAL _BOOLEAN toggle = FALSE;

    _IMMORTAL struct   sigaction ignore_sigint_action,
                                 prev_sigint_action; 

    /*---------------*/
    /* Ignore SIGINT */
    /*---------------*/

    if(toggle == FALSE)
    {  toggle                          = TRUE;
       ignore_sigint_action.sa_handler = (void *)psrp_critical_int_handler;
       ignore_sigint_action.sa_flags   = 0;
       (void)sigaction(SIGINT,&ignore_sigint_action,&prev_sigint_action);
    }


    /*---------------*/
    /* Handle SIGINT */
    /*---------------*/

    else
    {  toggle = FALSE;
       (void)sigaction(SIGINT,&prev_sigint_action,(struct sigaction *)NULL);
    }

    return(0);
}




/*---------------------*/
/* Handler for SIGTSTP */
/*---------------------*/

_PRIVATE int32_t tstp_handler(const int32_t signum)

{   (void)fprintf(stdout,"\n%sWARNING%s psrp client process does not support BSD style job control\n",boldOn,boldOff);
    (void)fflush(stdout);


    /*---------------------------------------*/
    /* Send SIGCONT to rest of process group */
    /*---------------------------------------*/

    (void)kill(0,SIGCONT);

    from_tstp_handler = TRUE;

    if(exec_shell == FALSE)
       pups_sigvector(SIGALRM, &command_loop_top);

    return(0);
}




/*------------------------------------------------------------------------------------*/
/* Handler for SIGPSRP - this is sent to suspend (and subsequently to reactivate) the */
/* client during segmented server exec operations                                     */
/*------------------------------------------------------------------------------------*/
 
_PRIVATE int32_t client_handler(const int32_t signum)

{   des_t old_client_indes  = (-1),
          old_client_outdes = (-1);

    char current_psrp_server[SSIZE] = "",
         pending_request[SSIZE]     = "";

    struct stat buf;
    sigset_t    client_set;

    cend();

    #ifdef PSRP_DEBUG 
    (void)fprintf(stdout,"CLIENT HANDLER\n");
    (void)fflush(stdout);
    #endif /* PSRP_DEBUG */


    /*---------------------------------------------*/
    /* Release signals once we have indicated that */
    /* client handler is active                    */
    /*---------------------------------------------*/

    (void)sigemptyset(&client_set);
    (void)sigaddset(&client_set,SIGTSTP);
    (void)sigaddset(&client_set,SIGINT); 
    (void)sigaddset(&client_set,SIGTERM);
    (void)sigaddset(&client_set,SIGALIVE);
    (void)sigaddset(&client_set,SIGALRM);
    (void)pups_sigprocmask(SIG_UNBLOCK,&client_set,(sigset_t *)NULL);


    /*---------------------------------------------------------*/
    /* If we have the channel to the server -- yield it -- we  */
    /* may not be the first to acquire the seglock, in which   */
    /* case its unfortunate possessor would hang!              */
    /*---------------------------------------------------------*/

    (void)psrp_yield_channel();


    /*--------------------------------------------------------*/
    /* CURSES silently un-installs signal handler for SIGALRM */
    /* re-install it before proceding                         */
    /*--------------------------------------------------------*/

    (void)pups_vitrestart();


    /*---------------------------------*/
    /* Save name of the current server */
    /*---------------------------------*/

    (void)strlcpy(current_psrp_server,psrp_server,SSIZE);


    if(pel_appl_verbose == TRUE && psrp_have_trailfile() == TRUE) 
    {  (void)strdate(date);
       (void)fprintf(stdout,"%sWARNING%s waiting for trailfile to migrated segment (%d) of PSRP  server \"%s\"\n",
                                                                                                   boldOn,boldOff,
                                                                                     server_seg_cnt+1,psrp_server);
       (void)fflush(stdout);
    }


    /*----------------------------------------------------------------------*/
    /* Wait to acquire seglock - when we have it re-connect to new segment  */
    /* unless the file has disappeared, in which case we need to find where */
    /* the new segment has migrated to before connecting to it              */
    /*----------------------------------------------------------------------*/

    while(pups_get_fd_lock(fileno(client_in),TSTLOCK) == FALSE)
          pups_sleep(DELAY);


    if(pel_appl_verbose == TRUE && psrp_have_trailfile() == TRUE) 
    {  (void)strdate(date);
       (void)fprintf(stdout,"trailfile to new segment (%d) for server \"%s\" ready\n",
                                                         server_seg_cnt+1,psrp_server);
       (void)fflush(stdout);
    }


    /*--------------------*/
    /* Server has crashed */
    /*--------------------*/

    if(strcmp(psrp_server,"none") == 0)
    {

       /*------------------------------*/
       /* Release channel segment lock */
       /*------------------------------*/

       (void)pups_release_fd_lock(fileno(client_in));
       (void)pups_sigvector(SIGALRM, &command_loop_top);
    }  


    /*---------------------------------------------------------*/
    /* On a heavily loaded machine we may drop the connection  */
    /* even if our target is a PSRP server, as it may not have */
    /* time to re-create its communication channel             */
    /*---------------------------------------------------------*/

    (void)pups_sleep(DELAY);


    /*--------------------------------------------------------------------*/
    /* We have to be a bit clever here. If we are not the client which    */
    /* initiated segmentation we need to look up the (possibly new) name  */
    /* of the server using the PID                                        */
    /* If we return FALSE here -- it means the entitiy we are waiting for */
    /* is not a PSRP server. Abort to command line closing PSRP channel   */
    /*--------------------------------------------------------------------*/

    (void)psrp_pid_to_channelname(appl_fifo_dir,server_pid,psrp_server,appl_host);
    if(strcmp(psrp_server,"nochan") == 0)
    {  if(pel_appl_verbose == TRUE)
       {  (void)fprintf(stdout,"%sERROR%S \"%s\" is a not a PSRP server (cannot autoconnect it)\n",boldOn,boldOff,psrp_server);
           (void)fflush(stdout);
       }

       client_in  = pups_fclose(client_in);
       client_out = pups_fclose(client_out);

       (void)strlcpy(psrp_server,"none",SSIZE);
       server_connected   = FALSE;
       server_pid         = 0;
       server_seg_cnt     = 0;
       processing_command = FALSE;
       (void)strlcpy(psrp_c_code,"iclerr",SSIZE);


       /*-----------------------------------------------------*/
       /* Stop homeostatic monitoring of connection to server */
       /*-----------------------------------------------------*/

       (void)pups_clearvitimer("server_alive_handler"); 

       (void)strlcpy(psrp_c_code,"autocerr",SSIZE);
       (void)pups_sigvector(SIGALRM, &command_loop_top);
    }

    if(pel_appl_verbose == TRUE)
    {  if(strcmp(current_psrp_server,psrp_server) != 0)
       {  if(strcmp(psrp_server,"notset") == 0)
             (void)fprintf(stdout,"%sWARNING%s PSRP server [%d] (name \"%s\") has terminated\n",boldOn,boldOff,server_pid,psrp_server);
          else
             (void)fprintf(stdout,"%sWARNING%s PSRP server [%d] has changed its name from \"%s\" to \"%s\"\n",server_pid,
                                                                                                          boldOn,boldOff,
                                                                                                     current_psrp_server,
                                                                                                             psrp_server);
          (void)fflush(stdout);
       }
    }

    if(strcmp(current_psrp_server,psrp_server) != 0)
    {  (void)snprintf(channel_name_out,SSIZE, "%s/psrp#%s#%s#fifo#in#%d#%d" ,appl_fifo_dir,psrp_server,psrp_host,server_pid,getuid());
       (void)snprintf(channel_name_in,SSIZE,  "%s/psrp#%s#%s#fifo#out#%d#%d",appl_fifo_dir,psrp_server,psrp_host,server_pid,getuid());
       (void)snprintf(channel_name,SSIZE,     "psrp#%s#%s#fifo#IO#%d#%d"    ,psrp_server,psrp_host,server_pid,getuid());
    }

    (void)pups_usleep(LONG_DELAY);


    #ifdef PSRP_DEBUG
    (void)fprintf(stdout,"GOT CHANNEL LOCK\n");
    (void)fflush(stdout);
    #endif /* PSRP_DEBUG */



    /*-------------------------*/
    /* Do we have a trailfile? */
    /*-------------------------*/

    if(psrp_have_trailfile() == TRUE)
    {

       /*------------------------------------------------------*/
       /* Check for any requests which are pending as a result */
       /* of the client waiting for the new server segment     */ 
       /*------------------------------------------------------*/

       (void)strlcpy(pending_request,"",SSIZE);
       if(pups_monitor(0,0,100) == READ_DATA)
       {  (void)read(0,pending_request,256);


          /*------------------------------------------------------*/
          /* If we have a pending close operation on the server   */
          /* simply indicate "no-connection" to server and vector */
          /* back to the command loop top                         */
          /*------------------------------------------------------*/

          if(strncmp(pending_request,"close",5) == 0    ||
             strncmp(pending_request,"bye",3)   == 0    ||
             strncmp(pending_request,"exit",4)  == 0    || 
             strncmp(pending_request,"quit",4)  == 0     )
          {  server_connected = FALSE;
             (void)psrp_close_server(TRUE,FALSE);
             (void)strlcpy(psrp_server,"none",SSIZE);
             server_connected   = FALSE;
             processing_command = FALSE;
             psrp_log           = FALSE;
             psrp_log           = FALSE;
             initialise_repeaters();

             (void)strlcpy(psrp_c_code,"scloscond",SSIZE);
             (void)pups_sigvector(SIGALRM, &command_loop_top);
          }
          else
          {  uint32_t i;


             /*------------------------------------------------*/
             /* Make sure that the pending request is serviced */
             /* when we vector back to the command loop top    */
             /*------------------------------------------------*/

             processing_command = TRUE;


             /*----------------------------------------------------------*/
             /* Check for a repeated command - if we get one we need to  */
             /* turn the repeat flag on as well                          */
             /*----------------------------------------------------------*/

             if(strncmp(pending_request,"repeat",6) == 0)
                do_repeat[r_cnt] = TRUE;


             /*----------------------------------------------------------*/
             /* If we have accumulated a lot of requests during the      */
             /* segmentation operation -- this is possible if the server */
             /* has migrated far away in cyber space, the linefeeds need */
             /* to be replaced with ';' demarcation characters           */
             /*----------------------------------------------------------*/

             for(i=pups_strlen(pending_request) - 2; i>=0; --i)
             {  if(pending_request[i] == '\n')
                   pending_request[i] = ';';
             }

             if(strcmp(request_line,"") != 0)
             {  (void)strlcat(request_line,"; ",SSIZE);
                (void)strlcat(request_line,pending_request,SSIZE);
             }
             else
                (void)strlcpy(request_line,pending_request,SSIZE);
          }
       }


       /*--------------------------------------------------------*/
       /* If we have a command of the form server: <command> the */
       /* leading server string should be stripped and (as the   */
       /* server is already open) and a close should be appended */ 
       /*--------------------------------------------------------*/

       if(strin(request_line,": ") == TRUE)
       {  char tmpstr[SSIZE] = "";

          (void)strpch(' ',request_line);
          (void)strfrm(request_line,':',1,tmpstr);
          (void)snprintf(request_line,SSIZE,"%s; close\n",tmpstr);
       }



       /*-----------------------------*/
       /* Reconnect to server process */
       /*-----------------------------*/

       (void)stat(channel_name_out,&buf);
       current_out_inode = buf.st_ino;

       (void)stat(channel_name_in,&buf);
       current_in_inode  = buf.st_ino;

       if(pel_appl_verbose == TRUE)
       {  (void)strdate(date);
          if(processing_command == TRUE)
             (void)fprintf(stdout,"\"%s\": server segment (%d) now ready (restarting interrupted PSRP transaction)\n",
                                                                                         psrp_server,server_seg_cnt+1);
          else
             (void)fprintf(stdout,"\"%s\": server segment (%d) now ready\n",
                                               psrp_server,server_seg_cnt+1);
 
          (void)fflush(stdout);

          (void)fprintf(stdout,"\n%sWARNING%s reconnecting PSRP server \"%s\" (%d@%s) [PSRP input channel ID changed]\n",
                                                                                                          boldOn,boldOff,
                                                                                        psrp_server,server_pid,psrp_host);
          (void)fflush(stdout);
       }

       in_inode  = current_in_inode;
       out_inode = current_out_inode;

       (void)cend();
       (void)psrp_close_server(TRUE,FALSE);
       server_connected  = FALSE;


       /*-----------------------------------------------------------*/
       /* Reconnect server (testing for possible connection errors) */
       /*-----------------------------------------------------------*/

       if((server_connected  = psrp_open_server()) == FALSE)
       {  (void)strdate(date);
          (void)fprintf(stdout,"%sERROR%s server segment (%d %s (%d@%s):%s):  reconnection failed\n",
                                                                                      boldOn,boldOff,
                                       date,appl_name,appl_pid,appl_host,appl_owner,server_seg_cnt+1);
          (void)fflush(stdout);

          (void)pups_release_fd_lock(old_client_indes);
          (void)close(old_client_indes);
          (void)pups_sigvector(SIGALRM, &command_loop_top);
       }
    }

    #ifdef PSRP_DEBUG
    (void)fprintf(stdout,"CLIENT HANDLER EXIT\n");
    (void)fflush(stdout);
    #endif /* DEBUG */


    /*--------------------------------------------------------------------*/
    /* Where we vector to here depends on whether we are going to restart */
    /* the precise request that was interrupted or the whole macro        */
    /*--------------------------------------------------------------------*/

    if(seg_restart_action == RESTART_INTR_REQ && strcmp(request_line,"") != 0)
       (void)pups_sigvector(SIGALRM, &process_loop_top);
    else
       (void)pups_sigvector(SIGALRM, &command_loop_top);

    in_client_handler = FALSE;
}





/*---------------------------------------------------------------------------------------*/
/* Handler for SIGCHAN -- this will be raised when another client (or other process) has */
/* terminated the PSRP server process                                                    */
/*---------------------------------------------------------------------------------------*/

_PRIVATE int32_t chan_client_handler(const int32_t signum)

{   

    /*----------------------------------------------------------------*/
    /* If the server has already disconnected by the time we get here */
    /* simply return to caller                                        */
    /*----------------------------------------------------------------*/

    if(server_connected == FALSE)
       return(0); 


    /*------------------------------------------------------------------------*/
    /* Has the server really terminated -- or has it actually migrated?       */
    /* If it has migrated  rp_migrate_client_to_server_host() will not return */
    /*------------------------------------------------------------------------*/

    if(psrp_migrate_client_to_server_host() == 0)
    {  if(pel_appl_verbose == TRUE)
       {  (void)fprintf(stdout,"\n%sERROR%s connection failed (PSRP server %s (%d@%s) has been terminated)\n\n",
                                                                                                 boldOn,boldOff,
                                                                               psrp_server,server_pid,psrp_host);
          (void)fflush(stdout);
       }
    }


    /*-----------------------------------------------------*/
    /* Stop homeostatic monitoring of connection to server */
    /*-----------------------------------------------------*/

    (void)pups_clearvitimer("server_alive_handler");


    /*-----------------------------*/
    /* Release channel access lock */
    /*-----------------------------*/

    (void)pups_release_fd_lock(fileno(client_out));

    cend();
    remove_junk();
    pups_exit(255);
}





/*------------------------------*/
/* Remove junk prior to exiting */
/*------------------------------*/

_PRIVATE void remove_junk(void)

{   uint32_t i;

    char pwd_file_name[SSIZE] = "",
         pwd_link_name[SSIZE] = "";

    if(client_in != (FILE *)NULL)
       (void)pups_fclose(client_in);

    if(client_out != (FILE *)NULL)
    {  if(have_access_lock == TRUE)
       {  (void)pups_release_fd_lock(fileno(client_out));
          have_access_lock = FALSE;
       }
       (void)pups_fclose(client_out);
    }

    (void)snprintf(pwd_link_name,SSIZE,"/tmp/authtok.%d.lnk.tmp",getpid());

    if(access(pwd_link_name,F_OK | R_OK | W_OK) == 0)
    {  struct stat stat_buf;

       (void)unlink(pwd_link_name);
       (void)snprintf(pwd_file_name,SSIZE,"/tmp/authtok.%d.tmp",appl_uid);
       (void)stat(pwd_file_name,&stat_buf);

       if(stat_buf.st_nlink == 1)
          (void)unlink(pwd_file_name);
    }

    if(clist_alloc > 0)
    {  for(i=0; i<clist_size; ++i)
          (void)pups_free((void *)clist[i]);
       (void)pups_free((void *)clist);
    }
}




/*-----------------------------*/
/* Open channel to PSRP server */
/*-----------------------------*/

_PRIVATE _BOOLEAN psrp_open_server(void)

{   int32_t trys                      = 0;

    char    full_channel_name[SSIZE]  = "",
            psrp_server_info[SSIZE]   = "",
            tmp_str[SSIZE]            = "";

    struct stat buf;


    /*--------------------------------------------*/
    /* If server is already open -- simply return */
    /*--------------------------------------------*/

    if(server_connected == TRUE)
    {  if(pel_appl_verbose == TRUE)
       {  (void)fprintf(stdout,"\n%sERROR%s PSRP server \"%s@%s\" is already open\n\n",boldOn,boldOff,psrp_server,psrp_host);
          (void)fflush(stdout);
       }

       (void)strlcpy(psrp_c_code,"soperr",SSIZE);
       return(TRUE);
    }


    /*----------------------------------*/
    /* Build PSRP channel name bindings */
    /*----------------------------------*/

    (void)snprintf(channel_name_out,SSIZE, "%s/psrp#%s#%s#fifo#in#%d#%d" ,appl_fifo_dir,psrp_server,psrp_host,server_pid,getuid());
    (void)snprintf(channel_name_in,SSIZE,  "%s/psrp#%s#%s#fifo#out#%d#%d",appl_fifo_dir,psrp_server,psrp_host,server_pid,getuid());
    (void)snprintf(channel_name,SSIZE,     "psrp#%s#%s#fifo#IO#%d#%d"    ,psrp_server,psrp_host,server_pid,getuid());


    /*---------------------------------------------------------------------------------*/
    /* We HAVE to try to open the PSRP channel ahead of actually telling the server we */
    /* wish to do so (or even knowing whether it will support PSRP protocols). This is */
    /* so we can use the network lock manager to handle locking for PSRP. Note we can  */
    /* only hold locks on an OPEN file object                                          */
    /*---------------------------------------------------------------------------------*/

    while((client_in = pups_fopen((char *)channel_name_in, "r+",DEAD)) == (FILE *)NULL)
    {     (void)pups_usleep(100);
          ++trys;

          /*------------------------------------*/
          /* Problem opening PSRP input channel */
          /*------------------------------------*/

          if(trys > max_trys)
          {  if(pel_appl_verbose == TRUE)
             {  (void)fprintf(stdout,"\n%sERROR%s %s (%d@%s) could not open PSRP input channel\n",
                                                                                   boldOn,boldOff,
                                                                                      psrp_server,
                                                                                       server_pid,
                                                                                        psrp_host,
                                                                                     channel_name);
                (void)fprintf(stdout,"%sERROR%S %s (%d@%s) connection failed\n\n",boldOn,boldOff,psrp_server,server_pid,psrp_host);
                (void)fflush(stdout);
             }

             (void)strlcpy(psrp_c_code,"soperr",SSIZE);
             return(FALSE);
          }
    }

    (void)stat(channel_name_in,&buf);
    in_inode = buf.st_ino;

    trys = 0;
    while((client_out = pups_fopen((char *)channel_name_out,"r+",DEAD)) == (FILE *)NULL)
    {     pups_usleep(100);
          ++trys;

          /*-------------------------------------*/
          /* Problem opening PSRP output channel */
          /*-------------------------------------*/

          if(trys > max_trys)
          {  (void)pups_fclose(client_in);

             if(pel_appl_verbose == TRUE)
             {  (void)fprintf(stdout,"\n%sERROR%s %s (%d@%s) could not open PSRP output channel\n",
                                                                                    boldOn,boldOff,
                                                                                       psrp_server,
                                                                                        server_pid,
                                                                                         psrp_host,
                                                                                      channel_name);
                (void)fprintf(stdout,"%sERROR%s %s (%d@%s) connection failed\n\n",boldOn,boldOff,psrp_server,server_pid,psrp_host);
                (void)fflush(stdout);
             }



             (void)strlcpy(psrp_c_code,"soperr",SSIZE);
             return(FALSE);
          }
    }


    /*-----------------------------------------------*/
    /* Check to see if PSRP server is currently busy */
    /* if we are soft linked to it                   */
    /*-----------------------------------------------*/
    /*-----------*/
    /* Soft link */
    /*-----------*/

    if(psrp_hard_link == FALSE)
    {  trys = 0;

       while(pups_get_fd_lock(fileno(client_out),TSTLOCK) != TRUE)
       {    pups_usleep(100);
            ++trys;

            /*-----------------------------*/
            /* Problem acquiring file lock */
            /*-----------------------------*/

             if(trys > max_trys)
             {  (void)pups_fclose(client_in);
                (void)pups_fclose(client_out);
   
                if(pel_appl_verbose == TRUE)
                {  (void)fprintf(stdout,"\n%sERROR%s %s (%d@%s) busy\n",
                                                         boldOn,boldOff,
                                                            psrp_server,
                                                             server_pid,
                                                              psrp_host,
                                                           channel_name);
                   (void)fprintf(stdout,"%sERROR%s %s (%d@%s) connection failed\n\n",boldOn,boldOff,psrp_server,server_pid,psrp_host);
                   (void)fflush(stdout);
                }
   
                (void)strlcpy(psrp_c_code,"soperr",SSIZE);
                return(FALSE);
             }
        }
    }


    /*-----------*/
    /* Hard link */
    /*-----------*/

    else
       pups_get_fd_lock(fileno(client_out),GETLOCK);


    have_access_lock = TRUE;

    (void)stat(channel_name_out,&buf);
    out_inode = buf.st_ino;


    /*---------------------------------------------------------------------------*/
    /* Check that client will support PSRP protocol and that channels are intact */
    /*---------------------------------------------------------------------------*/

    if(client_in == (FILE *)NULL && client_out == (FILE *)NULL)
    {  if(pel_appl_verbose == TRUE)
       {  (void)fprintf(stdout,"\n%sERROR%s %s (%d@%s) does not support Process Status Request Protocol (via %s)\n",
                                                                                                     boldOn,boldOff,
                                                                                                        psrp_server,
                                                                                                         server_pid,
                                                                                                          psrp_host,
                                                                                                       channel_name);
          (void)fprintf(stdout,"%sERROR%S %s (%d@%s) connection failed\n\n",boldOn,boldOff,psrp_server,server_pid,psrp_host);
          (void)fflush(stdout);
       }

       (void)strlcpy(psrp_server,"none",SSIZE);
       (void)strlcpy(psrp_c_code,"soperr",SSIZE);
       return(FALSE);
    }
    else
    {  if(client_in == (FILE *)NULL || client_out == (FILE *)NULL)
       {  if(pel_appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stdout,"\n%sERROR%s %s %s (%d@%s:%s) broken PSRP channel\n",
                                                                        boldOn,boldOff,
                                                                                  date,
                                                                           psrp_server,
                                                                            server_pid,
                                                                             psrp_host,
                                                                            appl_owner);
             (void)fprintf(stdout,"%sERROR%s %s %s (%d@%s:%s) connection failed\n",boldOn,boldOff,date,psrp_server,server_pid,psrp_host,appl_owner);
             (void)fflush(stdout);
          }

          (void)strlcpy(psrp_server,"none",SSIZE);
          (void)strlcpy(psrp_c_code,"cbrokerr",SSIZE);
          return(FALSE);
       }
    }


    /*---------------------------------------------*/
    /* Tell server we wish to open a channel to it */
    /*---------------------------------------------*/

    if(do_rsnr == FALSE)
       server_pid = psrp_channelname_to_pid(appl_fifo_dir,psrp_server,psrp_host);

    if(pups_i_own(server_pid) == FALSE)
    {  (void)pups_release_fd_lock(fileno(client_out));

       (void)fprintf(stdout,"\n%sERROR%S user \"%s\" does not own PSRP server \%s\" (cannot connect)\n\n",boldOn,boldOff,appl_owner,psrp_server);
       (void)fflush(stdout);

       return(FALSE);
    }

    if(pups_statkill(server_pid,SIGINIT) == PUPS_STOPPED)
    {  if(pel_appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stdout,"\n%sERROR%s %s %s (%d@%s:%s): stopped\n",
                                                          boldOn,boldOff,
                                                                    date,
                                                             psrp_server,
                                                              server_pid,
                                                               psrp_host,
                                                              appl_owner);
          (void)fprintf(stdout,"%sERROR%s %s %s (%d@%s:%s) connection failed\n",boldOn,boldOff,date,psrp_server,server_pid,psrp_host,appl_owner);
          (void)fflush(stdout);
       }

       (void)strlcpy(psrp_server,"none",SSIZE);
       (void)strlcpy(psrp_c_code,"csstoperr",SSIZE);
       return(FALSE);
    }

    if(pel_appl_verbose == TRUE)
    {  (void)fprintf(stdout,"\npsrp(protocol %5.2F): opening PSRP communication channel to %s (%d@%s)\n\n",
                                                    PSRP_PROTOCOL_VERSION,psrp_server,server_pid,psrp_host);
       (void)fflush(stdout);
    }


    /*------------------------------------------------------*/
    /* Start homeostatic monitoring of connection to server */
    /*------------------------------------------------------*/

    (void)pups_setvitimer("server_alive_handler",DELAY,VT_CONTINUOUS,DELAY,NULL,(void *)server_alive_handler);


    /*--------------------------------------------------------*/
    /* Tell server that this is an OPEN operation on the PSRP */
    /* channel. Send password to authenticate remote access.  */
    /*--------------------------------------------------------*/

    (void)fprintf(client_out,"OPEN %d %s\n",appl_pid,appl_password);
    (void)fflush(client_out);
    (void)fgets(tmp_str,SSIZE,client_in);

    #ifdef PSRP_DEBUG
    (void)fprintf(stderr,"SERVER INFO: %s\n",tmp_str);
    (void)fflush(stderr);
    #endif /* PSRP_DEBUG */

    (void)sscanf(tmp_str,"%d",&server_seg_cnt);


    /*---------------------------------------------------------*/
    /* If we are a remote client send full client path (rather */
    /* than client name) to server                             */
    /*---------------------------------------------------------*/

    if(is_remote == TRUE)
       (void)fprintf(client_out,"%s %d %s %s\n",appl_name,appl_pid,appl_host,remote_host_pathname);
    else
       (void)fprintf(client_out,"%s %d %s\n",appl_name,appl_pid,appl_host);

    (void)fflush(client_out);


    /*--------------------------------------------------------*/
    /* Wait for server to handshake to tell us SIGCHAN (GRAB) */
    /* is processed                                           */
    /*--------------------------------------------------------*/

    psrp_waitfor_endop("EOP grope");


    /*-----------------------------*/
    /* Release channel access lock */
    /*-----------------------------*/

    (void)pups_release_fd_lock(fileno(client_out)); 
    have_access_lock = FALSE;

    (void)strlcpy(psrp_c_code,"ok",SSIZE);
    return(TRUE);
}




/*------------------------------*/
/* Close channel to PSRP server */
/*------------------------------*/

_PRIVATE int32_t psrp_close_server(const _BOOLEAN force_hard_link, const _BOOLEAN tell_server)

{

    /*-------------------------*/
    /* Get channel access lock */
    /*-------------------------*/

    if(have_access_lock == FALSE)
    {  int32_t trys = 0;


       /*-----------*/
       /* Soft link */
       /*-----------*/

       if(psrp_hard_link == FALSE && force_hard_link == FALSE)
       {
          while(pups_get_fd_lock(fileno(client_out),TSTLOCK) != TRUE)
          {    pups_usleep(100);
               ++trys;

               /*-----------------------------*/
               /* Problem acquiring file lock */
               /*-----------------------------*/

                if(trys > max_trys)
                {  if(pel_appl_verbose == TRUE)
                   {  (void)fprintf(stdout,"\n%sERROR%s %s (%d@%s) busy\n",
                                                            boldOn,boldOff,
                                                               psrp_server,
                                                                server_pid,
                                                                 psrp_host,
                                                              channel_name);
                      (void)fprintf(stdout,"%sERROR%s %s (%d@%s) request failed\n\n",boldOn,boldOff,psrp_server,server_pid,psrp_host);
                      (void)fflush(stdout);
                   }

                   (void)strlcpy(psrp_c_code,"soperr",SSIZE);
                   return(-1);
                }
           }
       }

       /*-----------*/
       /* Hard link */
       /*-----------*/

       else
          (void)pups_get_fd_lock(fileno(client_out),GETLOCK);
    }


    (void)fflush(client_in);
    (void)fflush(client_out);

    (void)psrp_empty_fifo(fileno(client_in));
    (void)psrp_empty_fifo(fileno(client_out));

    if(tell_server == TRUE)
    {  (void)kill(server_pid,SIGINIT);

       errno = 0;
       (void)fprintf(client_out,"CLOSE %d\n",appl_pid);
       (void)fflush(client_out);


       /*---------------------------------------------------------------------*/
       /* Wait for server handshake indicating (processing of SIGINIT (CLOSE) */
       /* completed                                                           */
       /*---------------------------------------------------------------------*/

       psrp_waitfor_endop("EOP close");
    }


    /*-----------------------------*/
    /* Release channel access lock */
    /*-----------------------------*/

    (void)pups_release_fd_lock(fileno(client_out));
    have_access_lock = FALSE;

    (void)ftruncate(fileno(client_in), 0);
    (void)ftruncate(fileno(client_out),0);

    client_in =  pups_fclose(client_in);
    client_out = pups_fclose(client_out);


    /*-----------------------------------------------------*/
    /* Stop homeostatic monitoring of connection to server */
    /*-----------------------------------------------------*/

    (void)pups_clearvitimer("server_alive_handler");

    return(0);
}




/*-------------------------------------------------------------------*/
/* Builtin command to open a connection to a new PSRP server process */
/* (and close the connection to the current server if any)           */
/*-------------------------------------------------------------------*/

_PRIVATE void builtin_open_psrp_server(const char *request)

{   int32_t  ret,
             trys;

    pid_t    tmp_pid                = PSRP_TERMINATED;

    char     tmp_str[SSIZE]         = "",
             tmp_str2[SSIZE]        = "",
             server[SSIZE]          = "",
             psrp_parameters[SSIZE] = "",
             rhost[SSIZE]           = "",
             channel_name[SSIZE]    = "";

     _BOOLEAN exists = TRUE;

     if(request == (char *)NULL)
     {  (void)fprintf(stdout,"\n%sERROR%s expecting target PSRP server name\n\n",boldOn,boldOff);
        (void)fflush(stdout);

        (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
        return;
     }

     (void)strlcpy(psrp_c_code,"ok",SSIZE);
     (void)strext('\0',(char *)NULL,(char *)NULL);
     (void)strext(' ',tmp_str,request);

     if(strext(' ',tmp_str,request) != END_STRING)
     {  (void)fprintf(stdout,"\n%sERROR%s expecting server/host name\n\n",boldOn,boldOff);
        (void)fflush(stdout);

        (void)strlcpy(psrp_c_code,"psnynerr",SSIZE);
        return;
     }

     if(ret == PSRP_DUPLICATE_PROCESS_NAME)
     {  (void)fprintf(stderr,"\n%sERROR%s vector channel name \"%s\" not unique\n\n",boldOn,boldOff,tmp_str);
        (void)fflush(stderr);

        (void)strlcpy(psrp_c_code,"sexerr",SSIZE);
        return;
     }
     else if(ret == TRUE)
        return;


     /*-------------------------------------------------------*/
     /* Check to see if we are going to try to talk to a PSRP */
     /* server on a remote host                               */
     /*-------------------------------------------------------*/

     if(strin(tmp_str,"@") == TRUE)
     {  uint32_t i = 0;

        #ifndef SSH_SUPPORT
        (void)strdate(date);
        (void)fprintf(stdout,"\n%sERROR%s %s %s (%d@%s:%s): remote open not supported (no ssh support)\n\n",
                                                                                             boldOn,boldOff,
                                                               date,appl_name,appl_pid,appl_host,appl_owner);

        (void)fflush(stdout);


        /*-------------*/
        /* Host not up */
        /*-------------*/

        (void)strlcpy(psrp_c_code,"nsuperr",SSIZE);
        return;
        #else /* SSH_SUPPORT */


        (void)mchrep(' ',":@",tmp_str);
        (void)sscanf(tmp_str,"%s%s",server,rhost);


        while(tmp_str[i] != '@')
        {     server[i] = tmp_str[i];
              ++i;
        }

        server[i++] = '\0';
        (void)strpch(' ',server);


        /*----------------------------------------------------------------------------*/
        /* Get name of the host which is currently running the server we wish to talk */
        /* to                                                                         */
        /*----------------------------------------------------------------------------*/

        (void)strlcpy(rhost,&tmp_str[i],SSIZE);
        (void)strpch(' ',rhost);


        /*----------------------------------------------------------------------------*/
        /* If we are going to open a server which is implicitly running somewhere     */
        /* on the local cluster we need to find out where it is and then to subsitute */
        /* the name of that host                                                      */ 
        /*----------------------------------------------------------------------------*/

        {  char    ssh_command[SSIZE]    = "",
                   ssh_remote_env[SSIZE] = "",
                   sshPortOpt[SSIZE]     = "";

           int32_t ret,
                   status;


           /*-----------------------------------*/
           /* Set up environment on remote host */
           /*-----------------------------------*/

           (void)psrp_exec_env(ssh_remote_env);


           /*-----------------------------*/
           /* Access remote server by PID */
           /*-----------------------------*/

           if(sscanf(server,"%d",&server_pid) == 1)
              (void)snprintf(psrp_parameters,SSIZE,"bash -c '%s; psrp -spid  %d -on %s -from %s %s %d'",ssh_remote_env,server_pid,rhost,appl_host,appl_name,appl_pid);


           /*------------------------------*/
           /* Access remote server by name */
           /*------------------------------*/

           else
              (void)snprintf(psrp_parameters,SSIZE,"bash -c '%s; psrp -sname %s -on %s -from %s %s %d'",ssh_remote_env,server,    rhost,appl_host,appl_name,appl_pid);


           /*--------------------*/
           /* Child side of fork */
           /*--------------------*/
           /*------------------------------------------------------------*/
           /* Are we using a non standard port - we may be if the remote */
           /* end of this connection is living in a container            */
           /*------------------------------------------------------------*/

           (void)snprintf(sshPortOpt,SSIZE,"-p %s",ssh_remote_port);
           if((child_pid = pups_fork(FALSE,FALSE)) == 0)
           {

              /*-------------------------------*/
              /* Terminal connection to server */
              /*-------------------------------*/

              if(isatty(0) == 1)
              {  if(ssh_compression == TRUE)
                    (void)execlp("ssh","ssh",sshPortOpt,"-q","-C","-t","-l",ssh_remote_uname,rhost,psrp_parameters,(char *)NULL);
                 else
                    (void)execlp("ssh","ssh",sshPortOpt,"-q","-t","-l",     ssh_remote_uname,rhost,psrp_parameters,(char *)NULL);
              }


              /*-----------------------------------*/
              /* Non terminal connection to server */
              /*-----------------------------------*/

              else
              {  if(ssh_compression == TRUE)
                    (void)execlp("ssh","ssh",sshPortOpt,"-q","-C","-l",ssh_remote_uname,rhost,psrp_parameters,(char *)NULL);
                 else
                    (void)execlp("ssh","ssh",sshPortOpt,"-q","-l",     ssh_remote_uname,rhost,psrp_parameters,(char *)NULL);
              }


              /*--------------------------------------------------------------*/
              /* We should not get here -- if we do an pups_error has occured */
              /*--------------------------------------------------------------*/

              pups_error("[builtin_open_psrp_server] failed to open ssh channel to remote host");
              exit(255);
           }
           else
           {

              /*---------------------*/
              /* Parent side of fork */
              /*---------------------*/

              /*----------------------------------------------------*/
              /* Is the server actually running on the remote host? */
              /*----------------------------------------------------*/

              (void)pupswaitpid(FALSE,child_pid,&status);
              ret = WEXITSTATUS(status);

              if(ret != 0)
              {  (void)fprintf(stdout,"\n%sERROR%s failed to open remote PSRP server \"%s\" on host \"%s\"\n",boldOn,boldOff,server,rhost);
                 (void)fflush(stdout);

                 (void)strlcpy(psrp_c_code,"sloerr",SSIZE);
              }

              return;

           }
        }
        #endif /* SSH_SUPPORT */

     }
     else
     {

        /*----------------------------------------------*/
        /* Open request has the form open <PSRP server> */
        /* e.g. server is on the local host             */
        /*----------------------------------------------*/

        (void)strlcpy(server,tmp_str,SSIZE);
     }


     trys             = 0;
     server_connected = FALSE;

     while(server_connected == FALSE)
     {    do_rsnr = FALSE;

          if(sscanf(server,"%d",&tmp_pid) != 1)
             tmp_pid = psrp_channelname_to_pid(appl_fifo_dir,server,tmp_str2);
          else
          {  exists  = psrp_pid_to_channelname(appl_fifo_dir,tmp_pid,server,tmp_str2);
             if(psrp_channelname_to_pid(appl_fifo_dir,server,psrp_host) < 0)
             {  do_rsnr = TRUE;

                if(pel_appl_verbose == TRUE)
                {  (void)fprintf(stdout,"\n%sWARNING%s reverse server name resolution mode (cannot track PSRP channel dynamically)\n\n",boldOn,boldOff);
                   (void)fflush(stdout);
                }
             }
          }

          if(tmp_pid != PSRP_DUPLICATE_PROCESS_NAME    &&
             tmp_pid != PSRP_TERMINATED                &&
             exists  == TRUE                            )
          {  (void)strlcpy(psrp_server,server,SSIZE);
             (void)strlcpy(psrp_host,tmp_str2,SSIZE);
             server_pid = tmp_pid;

             if((server_connected = psrp_open_server()) == FALSE)
             {  (void)strlcpy(psrp_server,"none",SSIZE);
                (void)strlcpy(psrp_host,"none",SSIZE);
                server_pid = (-1);

                (void)strlcpy(psrp_c_code,"sexerr",SSIZE);
                return;
             }

             psrp_log         = FALSE;
             return;
          }
          else
          {  if(wait_for_psrp_server_startup == FALSE)
             {  if(tmp_pid == PSRP_TERMINATED || exists == FALSE)
                {  if(pel_appl_verbose == TRUE)
                   {  if(is_remote == TRUE &&  interactive_mode == FALSE)
                         (void)fprintf(stdout,"\n%sERROR%s cannot access process \"%s@%s\" as PSRP server\n\n",boldOn,boldOff,server,appl_host);
                      else
                         (void)fprintf(stdout,"\n%sERROR%s cannot access process \"%s\" as PSRP server\n\n",boldOn,boldOff,server);
                  
                      (void)fflush(stdout);
                   }

                   (void)strlcpy(psrp_c_code,"sexerr",SSIZE);
                   return;
                }                 
             }

             if(exists == FALSE || tmp_pid == PSRP_TERMINATED)
             {  if(trys == max_trys)
                {  if(pel_appl_verbose == TRUE)
                   {  (void)fprintf(stdout,"\n%sERROR%s channel to target process (%s@%s) does not exist\n\n",boldOn,boldOff,server,appl_host);
                      (void)fflush(stdout);
                   }

                   (void)strlcpy(psrp_c_code,"sexerr",SSIZE);
                   return;
                }
                else
                {  ++trys;

                   if(trys == 1 && pel_appl_verbose == TRUE)
                   {  (void)strdate(date);
                      (void)fprintf(stdout,"\n%sWARNING%s waiting for PSRP server %s@%s to start\n\n",boldOn,boldOff,server,appl_host);
                      (void)fflush(stdout);
                   }

                   (void)pups_sleep(TRY_TIMEOUT);
                }
             }

             if(tmp_pid == PSRP_DUPLICATE_PROCESS_NAME)
             {  if(pel_appl_verbose == TRUE)
                {  (void)fprintf(stdout,"\n%sERROR%s target PSRP process name is not unique\n\n",boldOn,boldOff);
                   (void)fflush(stdout);
                }

                (void)strlcpy(psrp_c_code,"sunerr",SSIZE);
                return;
             }
         }
     }

     if(pel_appl_verbose == TRUE)
     {  (void)strdate(date);
         (void)fprintf(stdout,"\nPSRP server %s%s@%s%s has started\n\n",boldOn,server,appl_host,boldOff);
         (void)fflush(stdout);
     }
}




/*-----------------------------------------------------------------------*/
/* Display help on buitlin commands supported by the psrp client process */
/*-----------------------------------------------------------------------*/

_PRIVATE void builtin_psrp_help(void)

{    FILE *pstream = (FILE *)NULL;

     pstream = popen(datasink_cmd,"w"); 

     (void)fprintf(pstream,"\n\n    PRSP process interaction client version %s (gcc %s: built %s %s)\n\n",PSRP_VERSION,__VERSION__,__TIME__,__DATE__);
     (void)fprintf(pstream,"    %sMacro control statements%s\n"   ,boldOn,boldOff);   
     (void)fprintf(pstream,"    %s========================%s\n\n" ,boldOn,boldOff);
     (void)fprintf(pstream,"    if      <cond> <cmd>         : execute <cmd> if <cond> TRUE\n");
     (void)fprintf(pstream,"    %%<label>                     : target label for resume\n");
     (void)fprintf(pstream,"    resume %%<label>              : resume macro execution at %%<label>\n");
     (void)fprintf(pstream,"    pups_errorabort on|off       : PML script aborted if on and pups_error code for last command != \"ok\"\n");
     (void)fprintf(pstream,"    sleep   <secs>               : Delay PML script execution for <secs> second\n");
     (void)fprintf(pstream,"    end                          : end macro currently executing\n");
     (void)fprintf(pstream,"    abort                        : abort current PML script\n");
     (void)fprintf(pstream,"    atomic  <cmd>                : do not try to expand <cmd> as macro\n");
     (void)fprintf(pstream,"    body    <cmd>                : show body (if <cmd> is a macro)\n");
     (void)fprintf(pstream,"    repeat  <cnt> <command>      : repeat command <cnt> times\n");
     (void)fprintf(pstream,"    rperiod <seconds>            : set repeat command repeat interval (in seconds)\n");
     (void)fprintf(pstream,"    repeat  <command>            : repeat command infinitely\n");
     (void)fprintf(pstream,"    raise   <cond>               : raise condition <cond> (pups_mainly used for testing)\n");
     (void)fprintf(pstream,"\n\n    %sBuiltin PSRP macro commands%s\n",boldOn,boldOff);   
     (void)fprintf(pstream,"    %s===========================%s\n\n"  ,boldOn,boldOff);
     (void)fprintf(pstream,"    medit                        : update PML (PSRP macro) definition file,  <file>\n");
     (void)fprintf(pstream,"    mcload   <file>              : load macro file overwriting macro stack (with syntax check)\n");
     (void)fprintf(pstream,"    mcappend <file>              : append macro file to macro stack (with syntax check)\n");
     (void)fprintf(pstream,"    mload    <file>              : load macro file overwriting macro stack)\n");
     (void)fprintf(pstream,"    mappend  <file>              : append macro file to macro stack\n");
     (void)fprintf(pstream,"    mpurge   all|<file>          : delete all PML macros or those in <file>\n");
     (void)fprintf(pstream,"    macros                       : show tags for all loaded PML macros\n");



     (void)fprintf(pstream,"\n\n    %sBuiltin PSRP commands%s\n"  ,boldOn,boldOff);   
     (void)fprintf(pstream,"    %s=====================%s\n\n"    ,boldOn,boldOff);
     (void)fprintf(pstream,"    %sHelp and display settings%s\n"  ,boldOn,boldOff);
     (void)fprintf(pstream,"    %s=========================%s\n\n", boldOn,boldOff);
     (void)fprintf(pstream,"    version                      : version of this PSRP interaction client\n");
     (void)fprintf(pstream,"    chelp                        : display help on builtin commands for client\n");
     (void)fprintf(pstream,"    pager    on|off|<pager>      : Set output pager\n");
     (void)fprintf(pstream,"    quiet                        : do not display output from builtin PSRP client commands\n");
     (void)fprintf(pstream,"    squiet                       : do not display output from PSRP server dispatch functions\n");
     (void)fprintf(pstream,"    noisy                        : display output from builtin PSRP client commands\n");
     (void)fprintf(pstream,"    snoisy                       : display output from builtin PSRP server dispatch functions\n");
     (void)fprintf(pstream,"    perror                       : print pups_error handler status\n");
     (void)fprintf(pstream,"    id                           : print owner, uid, gid and controlling tty for this [psrp] process\n");

     if(server_connected == TRUE)
     {  (void)fprintf(pstream,"    log                          : switch PSRP protocol transaction logging on\n");
        (void)fprintf(pstream,"    nolog                        : switch PSRP protocol transaction logging on\n");
     }

     (void)fprintf(pstream,"\n\n    %sCurses settings%s\n", boldOn,boldOff);
     (void)fprintf(pstream,"    %s===============%s\n\n"  , boldOn,boldOff);
     (void)fprintf(pstream,"    cinit                        : enter curses mode (for repeated commands)\n");
     (void)fprintf(pstream,"    cend                         : exit curses mode (for repeated commands)\n");
     (void)fprintf(pstream,"    cls                          : clear screen\n");

     (void)fprintf(pstream,"\n\n    %sPSRP client action settings%s\n",boldOn,boldOff);
     (void)fprintf(pstream,"    %s===========================%s\n\n"  ,boldOn,boldOff);
     (void)fprintf(pstream,"    open       <server>[@<host>] : open PSRP server process <server> [on <host>]\n");
     (void)fprintf(pstream,"    close                        : close PSRP server process\n");
     (void)fprintf(pstream,"    kill                         : kill PSRP server process immediately\n");
     (void)fprintf(pstream,"    trys       <num trys>        : set number of attempts to open PSRP server\n");
     (void)fprintf(pstream,"    retry      <on | off>        : enable or disable automatic request repetition (if server is busy)\n");
     (void)fprintf(pstream,"    wait                         : PSRP client will wait for server to start (and then connect to it)\n");
     (void)fprintf(pstream,"    nowait                       : PSRP client will abort connection attempt (if server not running)\n");
     (void)fprintf(pstream,"    exit                         : PSRP client will exit (if attached PSRP server terminates)\n");
     (void)fprintf(pstream,"    lcwd       <path>            : set current working directory for PSRP client\n");
     (void)fprintf(pstream,"    sink       [<dsink cmd>] [bg]: redirect PSRP client output to datasink command [in background]\n");
     (void)fprintf(pstream,"    clean      <directory>       : clean stale PSRP channels from PSRP channel (patchboard) directory\n");
     (void)fprintf(pstream,"    patchboard <directory>       : set patchboard <directory>\n");
     (void)fprintf(pstream,"    chanstat   [flags]           : show PSRP channels matching flags ([all] | [default] | [dead|nowner|vectored|busy|idle]\n");
     (void)fprintf(pstream,"    dllpstat   <DLL pathname>    : show orifices exported by <DLL pathname>\n");
     (void)fprintf(pstream,"    linktype                     : show type of PSRP channel link\n");
     (void)fprintf(pstream,"    linktype   <hard | soft>     : set type of PSRP channel link (soft link disconnects if server stopped)\n");
     (void)fprintf(pstream,"    killall    <directory> <spec>: Kill all PSRP servers in <directory> matching <spec>\n");
     (void)fprintf(pstream,"    segaction  <action>          : specify/display request processing action on server segmenation\n");
     (void)fprintf(pstream,"    segcnt                       : display number of segments (for segmented server)\n");
     (void)fprintf(pstream,"    quit | exit | bye            : terminate psrp client\n");

     (void)fprintf(pstream,"\n\n    %sBuiltin PSRP security commands%s\n", boldOn,boldOff);   
     (void)fprintf(pstream,"    %s==============================%s\n\n"  , boldOn,boldOff);
     #ifdef PSRP_AUTHENTICATE 
     (void)fprintf(pstream,"    user      <username>         : change session owner to <username>\n");
     (void)fprintf(pstream,"    password                     : set PSRP services authentication token\n");
     (void)fprintf(pstream,"    secure                       : set server side PSRP services authentication token\n");
     #endif /* PSRP_AUTHENTICATE */

     #ifdef SLAVED_COMMANDS
     (void)fprintf(pstream,"    clist     <clist file>       : read list of commands which can be executed by slaved shells\n");
     #endif /* SLAVED_COMMANDS */

     (void)fprintf(pstream,"\n\n    %sBuiltin PSRP remote server commands%s\n", boldOn,boldOff);   
     (void)fprintf(pstream,"    %s===================================%s\n\n"  , boldOn,boldOff);
     #ifdef SSH_SUPPORT
     (void)fprintf(pstream,"    compress  [on | off]         : switch ssh compression on or off\n");
     (void)fprintf(pstream,"    connect   [<user>@]<host>    : connect to remote PSRP client on <host> [under UID <user>]\n");
     #endif /* SSH_SUPPORT */

     (void)fprintf(pstream,"    <srv>: <req>                 : send <req> to <srv>\n");

     #ifdef SSH_SUPPORT
     (void)fprintf(pstream,"    <srv>@<host>: <req>          : send <req> to <srv> running on <host>\n");
     (void)fprintf(pstream,"                                 : <host>=? means search local cluster for <srv>\n");
     (void)fprintf(pstream,"                                 : and then send request if <srv> is uniquely pups_located\n");
     #endif /* SSH_SUPPORT */

     (void)fprintf(pstream,"\n\n    %sPass command to shell%s\n", boldOn,boldOff);
     (void)fprintf(pstream,"    %s=====================%s\n\n"  , boldOn,boldOff);

     (void)fprintf(pstream,"    !<command>                   : send command to users default shell\n");

     (void)fprintf(pstream,"\n\n    %sMultiple command processing%s\n", boldOn,boldOff);   
     (void)fprintf(pstream,"    %s===========================%s\n\n"  , boldOn,boldOff);
     (void)fprintf(pstream,"    c1; c2; c3                   : process multiple requests       \n");
     (void)fprintf(pstream,"    c \"a1 a2\"                    : glob argument \"a1 a2\" to a1a2 \n\n");
     (void)fflush(pstream);

     (void)pclose(pstream);
}



/*------------------------------------------------------------------*/
/* Handler for SIGINT when server is in a critical (blocking) state */
/*------------------------------------------------------------------*/

_PRIVATE int32_t psrp_critical_int_handler(const int32_t signum)

{   (void)fprintf(stdout,"\n%s*** server \"%s\" is not interruptable%s\n",boldOn,psrp_server,boldOff);
    (void)fflush(stdout);

    return(0);
}




/*---------------------------------------------------------------------------------------*/
/* Handler for SIGINT - propagate to client - note that we have the PSRP channel if this */
/* handler is not blocked                                                                */
/*---------------------------------------------------------------------------------------*/

_IMPORT  int32_t pupsighold_cnt[];
_PRIVATE int32_t psrp_int_handler(const int32_t signum)

{    char junk[SSIZE]    = "",
          signame[SSIZE] = "";


     /*------*/
     /* Exit */
     /*------*/

     if(in_cb_readline == TRUE && in_client_handler == FALSE)
     {  (void)fprintf(stdout,"\n\n");
        (void)fflush(stdout);

        pups_exit(255);
     }


     /*----------------*/
     /* Unlock channel */
     /*----------------*/

     (void)lockf(fileno(client_in),F_ULOCK,0);


     /*----------------------------------------------*/
     /* Aggregate command which has been interrupted */
     /*----------------------------------------------*/

     if(request_type == PSRP_LOCAL_AGGREGATE)
        (void)pups_sigvector(SIGALRM, &command_loop_top);


     /*-------------------------------------------*/
     /* Remote command which has been interrupted */
     /*-------------------------------------------*/

     if(request_type == PSRP_REMOTE_REQUEST)
        (void)pups_sigvector(SIGALRM, &command_loop_top);


     /*-------------------------------------------------------------------*/
     /* If we don't have the channel lock, we cannot interrupt the server */
     /* process as it is dealing with someone else's request              */
     /*-------------------------------------------------------------------*/

     if(pups_get_fd_lock(fileno(client_in),TSTLOCK) == FALSE)
        (void)pups_sigvector(SIGALRM, &command_loop_top);


     /*---------------------------------------------------------------*/
     /* Sometimes this handler can be called as the client_handler    */
     /* exits - make sure that in_client_handler is FALSE (this means */
     /* command_loop_top will behave properly despite this interrupt) */
     /*---------------------------------------------------------------*/

     if(in_client_handler == TRUE)
     {  in_client_handler     = FALSE;
        in_client_handler_int = TRUE;
        processing_command    = FALSE;
        initialise_repeaters();

        #ifdef PSRP_DEBUG
        (void)fprintf(stderr,"CHALT IN INT_HANDLER\n");
        (void)fflush(stderr);
        #endif /* PSRP_DEBUG */

        return(0);
     }

     #ifdef PSRP_DEBUG
     (void)fprintf(stderr,"%d: SIGINT (child %d)\n",appl_pid,child_pid);
     (void)fflush(stderr);
     #endif /* PSRP_DEBUG */

     cend();
     in_repeat_command = FALSE;
     pel_appl_verbose  = save_pel_appl_verbose;


     /*-----------------------------------------------*/
     /* This fixes a bug which causes the PSRP client */
     /* to report a hold/relse count mismatch         */
     /*-----------------------------------------------*/

     if(pupsighold_cnt[SIGCLIENT] == 0)
        pupsighold_cnt[SIGCLIENT] = 1;


     /*--------------------------------------------------------------*/
     /* Wait for child to exit before continuing -- this means       */
     /* children which change tty modes have a chance to restore     */
     /* them (so psrp's terminal handler continues to run correctly) */
     /*--------------------------------------------------------------*/

     if(child_pid != (-1))
     {  int32_t status;

        (void)kill(child_pid,SIGINT);
        (void)pupswaitpid(FALSE,child_pid,&status);

        #ifdef PSRP_DEBUG
        (void)fprintf(stderr,"%d: SIGINT (child %d exited)\n",appl_pid,child_pid);
        (void)fflush(stderr);
        #endif /* PSRP_DEBUG */

        child_pid = (-1);
     }

     #ifdef PSRP_DEBUG
     (void)fprintf(stderr,"\n\ninterrupted (%d)\n\n",appl_pid);
     (void)fflush(stderr);
     #endif /* PSRP_DEBUG */


     /*-----------------------------------------------------------------------*/
     /* We cannot interrupt a server if it is interacting with another client */
     /*-----------------------------------------------------------------------*/

     if(server_connected == FALSE || in_prompt_loop == TRUE || is_builtin == TRUE)
     {  if(interactive_mode == TRUE)
        {  (void)pups_sigvector(SIGALRM, &command_loop_top);
           (void)fprintf(stdout,"\n\ninteractive (%d)\n\n",appl_pid);
           (void)fflush(stdout);
        }
        else
        {  (void)remove_junk();

           if(strcmp(psrp_c_code,"ok") != 0)
              (void)pups_exit(255);

           (void)pups_exit(0);
        }
     }


     /*----------------------------------------------*/
     /* Tell server that we have had a SIGINT raised */
     /* which it must process as a SIGABRT           */
     /* Pass the details of the interrupting client  */
     /* to the server -- this permits thread control */
     /* in the case of multithreaded servers         */
     /*----------------------------------------------*/

     if(server_pid != (-1))
     {

        #ifdef PSRP_DEBUG
        (void)fprintf(stderr,"ABRT sent (to server %d)\n",server_pid);
        (void)fflush(stderr);
        #endif /* PSRP_DEBUG */

        (void)kill(server_pid,SIGABRT);
     }
    

     /*-----------------------*/
     /* None interactive mode */
     /*-----------------------*/
 
     if(interactive_mode == FALSE)
     {
        #ifdef PSRP_DEBUG
        (void)fprintf(stderr,"ABRT sent (to server %d) wait for abrt\n",server_pid);
        (void)fflush(stderr);
        #endif /* PSRP_DEBUG */


        /*--------------------------------------------------------*/
        /* PID of this process (so server knows who caused abort) */
        /*--------------------------------------------------------*/

        (void)fprintf(client_out,"ABRT %d\n",appl_pid);
        (void)fflush(client_out);


        /*---------------------------------------------------------*/
        /* Wait for server to inform us that it has finished abort */
        /* processing.                                             */
        /*---------------------------------------------------------*/

        (void)psrp_waitfor_endop("EOP abrt");
        (void)psrp_empty_fifo(fileno(client_in));
        (void)psrp_empty_fifo(fileno(client_out));
        (void)psrp_close_server(TRUE,TRUE);
        (void)remove_junk();

        #ifdef PSRP_DEBUG
        (void)fprintf(stderr,"ABRT sent (to server %d) got abrt\n",server_pid);
        (void)fflush(stderr);
        #endif /* PSRP_DEBUG */
        
        if(strcmp(psrp_c_code,"ok") != 0)
          (void)pups_exit(255);

        (void)pups_exit(0);
     }


     /*-----------------------------------------*/
     /* If we are processing a command abort it */
     /*-----------------------------------------*/

     processing_command = FALSE;


     /*--------------------------------------------------*/ 
     /* If we were executing a repeated command abort it */
     /*--------------------------------------------------*/ 

     initialise_repeaters();

     (void)fprintf(client_out,"%d\n",appl_pid);
     (void)fflush(client_out);
     (void)psrp_waitfor_endop("EOP abrt");

     if(flycom == FALSE)
        (void)psrp_yield_channel();

     --r_cnt;


done:

     /*----------------------------------------*/
     /* If interrupt generated by SIGPIPE exit */
     /*----------------------------------------*/

     if(signum == SIGPIPE)
        pups_exit(255);

     (void)pups_sigvector(SIGALRM, &command_loop_top);
}





/*----------------------------------------*/
/* Handler for broken channel (to server) */
/*----------------------------------------*/

_PRIVATE void server_alive_handler(vttab_type *tinfo, char *args)

{    int32_t  ret;

     ino_t    current_in_inode,
              current_out_inode;

     pid_t    current_server_pid;

     struct   stat buf;

     sigset_t set,
              o_set;

     char     wait[SSIZE] = "";

     (void)sigfillset(&set);
     (void)pups_sigprocmask(SIG_SETMASK,&set,&o_set);


     /*------------------------------------------------------------*/   
     /* If server is migrating to another host terminate homeostat */
     /*------------------------------------------------------------*/   

     if(appl_rooted == FALSE && migrating_segmentation == TRUE)
     {

        /*-----------------------------------------------------*/
        /* Stop homeostatic monitoring of connection to server */
        /*-----------------------------------------------------*/

        (void)pups_clearvitimer("server_alive_handler");

        if(pel_appl_verbose == TRUE)
        {  (void)fprintf(stdout,"\n\n%sWARNING%s PSRP server \"%s\" is migrating to another host (terminating server alive homeostat)\n\n",boldOn,boldOff,psrp_server);
           (void)fflush(stdout);
        }

        return;
     }


     /*--------------------------------------------------------*/  
     /* If we have a SIGCLIENT pending, service it immediately */
     /*--------------------------------------------------------*/  

     (void)sigpending(&set);
     if(sigismember(&set,SIGCLIENT))
        pups_signalpause(SIGCLIENT);

     (void)stat(channel_name_out,&buf);
     current_out_inode = buf.st_ino;

     (void)stat(channel_name_in,&buf);
     current_in_inode  = buf.st_ino;


     /*------------------------------------------------------*/
     /* Check to see if server is in a fit state to interact */
     /* with client                                          */
     /*------------------------------------------------------*/

     if(in_client_handler == FALSE)
     {  if(do_rsnr == FALSE)
        {  if((current_server_pid = psrp_channelname_to_pid(appl_fifo_dir,psrp_server,psrp_host)) == PSRP_DUPLICATE_PROCESS_NAME)
           {  do_rsnr          = TRUE;

              if(pel_appl_verbose == TRUE)
              {  (void)fprintf(stdout,"\n\n%sWARNING%s \"%s\" is no longer unique (switching to implicit reverse server name resolution mode)\n\n",boldOn,boldOff,psrp_server);
                 (void)fflush(stdout);
              }

              current_server_pid = server_pid;
              (void)pups_sigvector(SIGALRM, &command_loop_top);
           }
        } 
        else
        {  

           /*-------------------------------------------*/
           /* check to see if this server is now unique */
           /*-------------------------------------------*/

           if((current_server_pid = psrp_channelname_to_pid(appl_fifo_dir,psrp_server,psrp_host)) > 0)
           {  do_rsnr = FALSE;

              if(pel_appl_verbose == TRUE)
              {  (void)fprintf(stdout,"\n\n%sWARNING%s \"%s\" is now unique (switching to implicit server PID resolution mode)\n\n",boldOn,boldOff,psrp_server);
                 (void)fflush(stdout);
              }

              (void)pups_sigvector(SIGALRM, &command_loop_top);
           }
           else
              current_server_pid = server_pid;
        }
     }


     /*--------------------------------------*/
     /* Check to see if PSRP server is alive */
     /*--------------------------------------*/

     ret = pups_statkill(server_pid,SIGALIVE); 
     if(ret == PUPS_TERMINATED || (ret == PUPS_STOPPED && psrp_hard_link == FALSE))
     {  cend();

        if(ret == PUPS_TERMINATED)
        {  

           /*-------------------------------------------------------------------------*/
           /* Has the server really terminated -- or has it actually migrated?        */
           /* If it has migrated psrp_migrate_client_to_server_host() will not return */
           /*-------------------------------------------------------------------------*/

           psrp_migrate_client_to_server_host();
               
           if(pel_appl_verbose == TRUE)     
           {  (void)fprintf(stdout,"\n\n%sWARNING%s target PSRP server process %s (%d@%s) has been terminated\n\n",
                                                                                                    boldOn,boldOff,
                                                                                  psrp_server,server_pid,psrp_host);
              (void)fflush(stdout);
           }
        }
        else if(ret == PUPS_STOPPED)
        {  if(pel_appl_verbose == TRUE)
           {  (void)fprintf(stdout,"\n\n%sWARNING%s target PSRP server process %s (%d@%s) has been stopped\n\n",
                                                                                                 boldOn,boldOff,
                                                                               psrp_server,server_pid,psrp_host);
              (void)fflush(stdout);
           }
        }

        close_psrp_channel = TRUE;


        /*-----------------------------------------------*/
        /* Tell server to perform a disconnect on retart */
        /* if it is stopped                              */
        /*-----------------------------------------------*/

        if(ret == PUPS_STOPPED)
           kill(server_pid,SIGCLIENT);

        (void)strlcpy(psrp_server,"none",SSIZE);
        server_connected   = FALSE;
        server_pid         = (-1);
        server_seg_cnt     = 0;
        processing_command = FALSE;
        (void)strlcpy(psrp_c_code,"stermerr",SSIZE);


        /*-----------------------------------------------------*/
        /* Stop homeostatic monitoring of connection to server */
        /*-----------------------------------------------------*/

        (void)pups_clearvitimer("server_alive_handler");


        /*---------------------------------------------*/
        /* If we are running non-interactively -- exit */
        /*---------------------------------------------*/

        if(prompt == FALSE)
        {  if(strcmp(psrp_c_code,"ok") != 0)
             (void)pups_exit(255);
           pups_exit(0);
        }

        (void)pups_sigvector(SIGALRM, &command_loop_top);
     }
     else
        current_server_pid = server_pid;


     /*-------------------------------------------*/
     /* Check for server in (overfork) wait state */
     /*-------------------------------------------*/

     (void)snprintf(wait,SSIZE,"%s.wait",channel_name_in);
     if(access(wait,F_OK | R_OK | W_OK) == 0 && psrp_hard_link == FALSE)
     {  if(pel_appl_verbose == TRUE)
        {  (void)fprintf(stdout,"\n%sWARNING%s target process %s (%d@%s) has been overforked\n\n",boldOn,boldOff,psrp_server,server_pid,psrp_host);
           (void)fflush(stdout);
        }

        close_psrp_channel = TRUE;


        /*-----------------------------------------------*/
        /* Tell server to perform a disconnect on retart */
        /* if it is stopped                              */
        /*-----------------------------------------------*/

        if(ret == PUPS_STOPPED)
           kill(server_pid,SIGCLIENT);

        (void)strlcpy(psrp_server,"none",SSIZE);
        server_connected   = FALSE;
        server_pid         = 0;
        server_seg_cnt     = 0;
        processing_command = FALSE;
        (void)strlcpy(psrp_c_code,"sovfmerr",SSIZE);


        /*-----------------------------------------------------*/
        /* Stop homeostatic monitoring of connection to server */
        /*-----------------------------------------------------*/

        (void)pups_clearvitimer("server_alive_handler");


        /*---------------------------------------------*/
        /* If we are running non-interactively -- exit */
        /*---------------------------------------------*/

        if(prompt == FALSE)
        {  if(strcmp(psrp_c_code,"ok") != 0)
             (void)pups_exit(255);
           pups_exit(0);
        }

        (void)pups_sigvector(SIGALRM, &command_loop_top);
     }

     if(in_client_handler == FALSE)
     {  _BOOLEAN reconnected             = FALSE,
                 overlaying              = FALSE;

        char     wait_channelname[SSIZE] = "";

        _IMMORTAL int32_t cr_cnt = 0;


        /*----------------------------------------------------*/
        /* Check to see if client I/P channel lost - if it is */
        /* wait for the server to provide a new channel and   */
        /* connect to it                                      */ 
        /*----------------------------------------------------*/

        (void)stat(channel_name_in,&buf);
        current_in_inode = buf.st_ino;

        (void)snprintf(wait_channelname,SSIZE,"%s.wait",channel_name_in);
        if(access(wait_channelname,F_OK | R_OK | W_OK) == (-1) && current_in_inode != in_inode)
        {  int32_t trys = 0;


           /*-------------------------------------------------------------*/
           /* If we have just passed an overlay command to the new server */
           /* we must update the PSRP channel name                        */
           /*-------------------------------------------------------------*/

           if(strcmp(new_psrp_server,"notset") != 0)
           {  if(strcmp(new_psrp_server,"notset") != 0)
                 (void)strlcpy(psrp_server,new_psrp_server,SSIZE);

              (void)strlcpy(new_psrp_server,"notset",SSIZE);
              if(new_server_pid != (-1))
                 current_server_pid = server_pid = new_server_pid;

              (void)snprintf(channel_name_out,SSIZE, "%s/psrp#%s#%s#fifo#in#%d#%d" ,appl_fifo_dir,psrp_server,psrp_host,server_pid,getuid());
              (void)snprintf(channel_name_in,SSIZE,  "%s/psrp#%s#%s#fifo#out#%d#%d",appl_fifo_dir,psrp_server,psrp_host,server_pid,getuid());
              (void)snprintf(channel_name,SSIZE,     "psrp#%s#%s#fifo#IO#%d#%d"    ,psrp_server,psrp_host,server_pid,getuid());

              overlaying = TRUE;
           }


           /*--------------------------------------*/
           /* Wait for server to re-create channel */
           /*--------------------------------------*/

           trys = 0;
           while(access(channel_name_in,F_OK | R_OK | W_OK) == (-1))
           {   _BOOLEAN ret = FALSE;


               /*-----------------------------------------------------------------------*/
               /* If PSRP server has not re-created its channels in a second we will    */
               /* still drop the connection -- this has implications if the PSRP server */
               /* is running on a heavily loaded machine                                */
               /*-----------------------------------------------------------------------*/

               (void)pups_sleep(1);
               (void)psrp_pid_to_channelname(appl_fifo_dir,server_pid,new_psrp_server,appl_host);

               if(strcmp(new_psrp_server,psrp_server) != 0)
               {  if(strcmp(new_psrp_server,"nochan") == 0)
                  {  if(pel_appl_verbose == TRUE)  
                     {  (void)fprintf(stdout,"\n%sERROR%s \"%s\" is not a PSRP server (cannot autoconnect it)\n",boldOn,boldOff,psrp_server);
                        (void)fflush(stdout);
                     }

                     close_psrp_channel = TRUE;

                     (void)strlcpy(psrp_server,"none",SSIZE);
                     server_connected   = FALSE;
                     server_pid         = (-1);
                     server_seg_cnt     = 0;
                     processing_command = FALSE;
                     (void)strlcpy(psrp_c_code,"iclerr",SSIZE);


                     /*-----------------------------------------------------*/
                     /* Stop homeostatic monitoring of connection to server */
                     /*-----------------------------------------------------*/

                     (void)pups_clearvitimer("server_alive_handler");
                                                                       
                     (void)strlcpy(psrp_c_code,"autocerr",SSIZE);
                     (void)pups_sigvector(SIGALRM, &command_loop_top);
                  }
                  else
                  {  pups_sleep(1);
                     ++trys;

                  }

                  if(strcmp(new_psrp_server,"nohost") != 0)
                  {  if(in_new_getinfo == FALSE && pel_appl_verbose == TRUE)
                     {  (void)fprintf(stdout,"\n%sWARNING%s PSRP server has changed its name from \"%s\" to \%s\"\n",boldOn,boldOff,psrp_server,new_psrp_server);
                        (void)fflush(stdout);
                     }

                     (void)strlcpy(psrp_server,new_psrp_server,SSIZE);
                     (void)strlcpy(new_psrp_server,"notset",SSIZE);
                     (void)snprintf(channel_name_out,SSIZE, "%s/psrp#%s#%s#fifo#in#%d#%d" ,appl_fifo_dir,psrp_server,psrp_host,server_pid,getuid());
                     (void)snprintf(channel_name_in,SSIZE,  "%s/psrp#%s#%s#fifo#out#%d#%d",appl_fifo_dir,psrp_server,psrp_host,server_pid,getuid());
                     (void)snprintf(channel_name,SSIZE,     "psrp#%s#%s#fifo#IO#%d#%d"    ,psrp_server,psrp_host,server_pid,getuid());
                  }

                  overlaying = TRUE;
               }

               if(trys > max_trys)
               {  cend();

                  if(pel_appl_verbose == TRUE)
                  {  (void)strdate(date);
                     (void)fprintf(stdout,"\n%sERROR%s no client input channel to %s (%d@%s) [not a PSRP server?]\n\n",
                                                                                                        boldOn,boldOff,
                                                                                      psrp_server,server_pid,psrp_host);
                     (void)fflush(stdout);
                  }

                  close_psrp_channel = TRUE;

                  (void)strlcpy(psrp_server,"none",SSIZE);
                  server_connected   = FALSE;
                  server_pid         = 0;
                  server_seg_cnt     = 0;
                  processing_command = FALSE;
                  (void)strlcpy(psrp_c_code,"iclerr",SSIZE);


                  /*-----------------------------------------------------*/
                  /* Stop homeostatic monitoring of connection to server */
                  /*-----------------------------------------------------*/

                  (void)pups_clearvitimer("server_alive_handler");


                  /*---------------------------------------------*/
                  /* If we are running non-interactively -- exit */
                  /*---------------------------------------------*/

                  if(prompt == FALSE)
                  {  if(strcmp(psrp_c_code,"ok") != 0)
                        (void)pups_exit(255);
                     pups_exit(0);
                  }

                  (void)pups_sigvector(SIGALRM, &command_loop_top);
               }

               ++trys;

               if(access(channel_name_in,F_OK | R_OK | W_OK) == (-1))
                  (void)pups_usleep(LONG_DELAY);
           }
          
           if(pel_appl_verbose == TRUE)
           {  if(cr_cnt == 0)
              {  (void)fprintf(stdout,"\n");
                 (void)fflush(stdout);
              } 

              (void)strdate(date);
              if(pel_appl_verbose == TRUE)
              {  if(overlaying == FALSE)
                    (void)fprintf(stdout,"\n%sWARNING%s reconnecting PSRP server \"%s\" (%d@%s) [PSRP input channel ID changed]\n\n",
                                                                                                                      boldOn,boldOff,
                                                                                                    psrp_server,server_pid,psrp_host);
                 else
                    (void)fprintf(stdout,"\n%sWARNING%s autoconnecting overlayed PSRP server \"%s\" (%d@%s) [PSRP input channel]\n",boldOn,boldOff,psrp_server,server_pid,psrp_host);

                 (void)fflush(stdout);
              }

              ++cr_cnt;
              if(cr_cnt == 2)
              {  (void)fprintf(stdout,"\n");
                 (void)fflush(stdout);

                 cr_cnt = 0;
              }
           }

           (void)strlcpy(new_psrp_server,"notset",SSIZE);

           client_in = pups_fclose(client_in);
           client_in = pups_fopen(channel_name_in,"r+",DEAD);
           (void)stat(channel_name_in,&buf);
           in_inode    = buf.st_ino;
           reconnected = TRUE;
        }


        /*----------------------------------------------------*/
        /* Check to see if client O/P channel lost - if it is */
        /* wait for the server to provide a new channel and   */
        /* connect to it                                      */
        /*----------------------------------------------------*/

        (void)stat(channel_name_out,&buf);
        current_out_inode  = buf.st_ino;

        (void)snprintf(wait_channelname,SSIZE,"%s.wait",channel_name_out);
        if(access(wait_channelname,F_OK | R_OK | W_OK) == (-1)  && current_out_inode != out_inode)
        {  int32_t trys = 0;


           /*--------------------------------------*/
           /* Wait for server to re-create channel */
           /*--------------------------------------*/

           while(access(channel_name_out,F_OK | R_OK | W_OK) == (-1));
           {   if(trys > max_trys)
               {  cend();

                  if(pel_appl_verbose == TRUE)
                  {  (void)fprintf(stdout,"\n%sERROR%s no client output channel to %s (%d@%s) [not a PSRP server?]\n\n",
                                                                                                         boldOn,boldOff,
                                                                                       psrp_server,server_pid,psrp_host);
                     (void)fflush(stdout);
                  }

                  close_psrp_channel = TRUE;

                  (void)strlcpy(psrp_server,"none",SSIZE);
                  server_connected   = FALSE;
                  server_pid         = 0;
                  server_seg_cnt     = 0;
                  processing_command = FALSE;
                  (void)strlcpy(psrp_c_code,"oclerr",SSIZE);


                  /*-----------------------------------------------------*/
                  /* Stop homeostatic monitoring of connection to server */
                  /*-----------------------------------------------------*/

                  (void)pups_clearvitimer("server_alive_handler");


                  /*---------------------------------------------*/
                  /* If we are running non-interactively -- exit */
                  /*---------------------------------------------*/

                  if(prompt == FALSE)
                  {  if(strcmp(psrp_c_code,"ok") != 0)
                        pups_exit(255);
                     pups_exit(0);
                  }

                  (void)pups_sigvector(SIGALRM, &command_loop_top);
               }

               ++trys;

               if(access(channel_name_out,F_OK | R_OK | W_OK) == (-1))
                  (void)pups_usleep(LONG_DELAY);
           }

           server_pid = current_server_pid;
          
           if(pel_appl_verbose == TRUE)
           {  if(cr_cnt == 0)
              {  (void)fprintf(stdout,"\n");
                 (void)fflush(stdout);
              }

              (void)strdate(date);
              if(pel_appl_verbose == TRUE)
              {  if(overlaying == FALSE)
                    (void)fprintf(stdout,"\n%sWARNING%s reconnecting PSRP server \"%s\" (%d@%s) [PSRP output channel ID changed]\n\n",
                                                                                                                       boldOn,boldOff,
                                                                                                     psrp_server,server_pid,psrp_host);
                 else
                    (void)fprintf(stdout,"\n%sWARNING%s autoconnecting overlayed PSRP server \"%s\" (%d@%s) [PSRP output channel]\n",
                                                                                                                      boldOn,boldOff,
                                                                                                    psrp_server,server_pid,psrp_host);
                 (void)fflush(stdout);
              }
           
              ++cr_cnt;
              if(cr_cnt == 2)
              {  (void)fprintf(stdout,"\n");
                 (void)fflush(stdout);

                 cr_cnt = 0;
              }
           }

           client_out = pups_fclose(client_out);
           client_out = pups_fopen(channel_name_out,"r+",DEAD);
           (void)stat(channel_name_out,&buf);
           out_inode   = buf.st_ino;
           reconnected = TRUE;
        }

        if(reconnected == TRUE)
           (void)pups_sigvector(SIGALRM, &command_loop_top);
     }
}




/*---------------------------------------------------------*/
/* Expand requests of the form server@host: <command list> */
/*---------------------------------------------------------*/
/*---------------------------------------*/
/* Return first non-whitespace character */
/*---------------------------------------*/

_PRIVATE char first_non_whitespace(const size_t from, char *s)

{   size_t i;

    if(from >= strlen(s))
       return('\0');

    for(i=from; i<strlen(s); ++i)
    {  if(s[i] != ' ')
          return(s[i]);
    }

    return('\0');
}


/*---------------------------------------*/
/* Expand aggregated and remote requests */
/*---------------------------------------*/

_PRIVATE  int32_t expand_request(char *request, char *expanded_request, _BOOLEAN *flycom, char *rhost)

{    size_t   i,
              cnt             = 0;

      int32_t at_pos          = (-1),
              sc_pos          = (-1),
              n_ats           = 0,
              n_semicolons    = 0,
              request_type    = PSRP_LOCAL_ATOMIC;

    char      tmpstr[SSIZE]   = "",
              rserver[SSIZE]  = "",
              reply[SSIZE]    = ""; 

    _BOOLEAN  glob            = FALSE,
              scope_op_found  = FALSE,
              psuedoserver    = FALSE;


    /*-------------------------------------*/
    /* If this is a shell command -- abort */
    /*-------------------------------------*/

    if(first_non_whitespace(0,request) == '!')
       return(request_type);


    /*-------------------------------------------------------*/
    /* Open is not expanded here. This enables us to process */
    /* remote open commands of the form open <server>@<host> */
    /*-------------------------------------------------------*/

    if(strin(request,"open") == TRUE)
    {  

       /*----------------------------------------------*/
       /* Check for syntax pups_errors in open request */
       /*----------------------------------------------*/

       if(first_non_whitespace(4,request) == '@')
       {  if(pel_appl_verbose == TRUE)
          {  (void)fprintf(stdout,"\n%sERROR%s PML syntax pups_error (open request has form server@host: <command list>)\n\n",boldOn,boldOff);
             (void)fflush(stdout);
          }

          (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
          return(PSRP_REQUEST_ERROR);
       }

       (void)strlcpy(expanded_request,request,SSIZE);
       return(request_type);
    }
    else
    {  if(first_non_whitespace(0,request) == '@')
          psuedoserver = TRUE;
    }


    /*--------------------------------------*/
    /* Check request for syntax pups_errors */
    /*--------------------------------------*/

    for(i=0; i<pups_strlen(request); ++i)
    {  if(request[i] == ':' && request[i+1] != ' ' && isdigit(request[i+1]) == 0)
       {  if(pel_appl_verbose == TRUE)
          {  (void)fprintf(stdout,"\n%sERROR%s PML syntax pups_error (expanded request has form server@host: <command list>)\n\n",boldOn,boldOff);
             (void)fflush(stdout);
          }

          (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
          return(PSRP_REQUEST_ERROR);
       }


       /*---------------------------------*/
       /* Check to see if we need to glob */
       /*---------------------------------*/

       if(request[i] == '"')
       {

          /*-----------------------------------------*/
          /* Glob material enclosed in double quotes */
          /*-----------------------------------------*/

          if(strin(request,"!")        == FALSE    &&
             strin(request,"overfork") == FALSE    &&
             strin(request,"overlay")  == FALSE     )
          {  if(glob == TRUE)
                glob = FALSE;
             else
                glob = TRUE;
          }

          /*------------------------------------------------------------*/
          /* Do not glob if we are going to submit this line to another */
          /* command processor                                          */
          /*------------------------------------------------------------*/

          else
             glob = FALSE;
       }
       else
       {  if(strncmp(&request[i],"::",2) == 0)
          {  if(scope_op_found == FALSE)
             {  tmpstr[cnt++]   =  request[i];
                i               += 1;
                scope_op_found  =  TRUE;
             }
          }
          else
          {  if(request[i] == ':' && isdigit(request[i+1]) == 0)
             {  sc_pos = i;
                ++n_semicolons;
             }
          }

          if(request[i] == '@')
          { at_pos = i;
            ++n_ats;
          }

          if(glob == FALSE)
             tmpstr[cnt++] = request[i];
          else
          {  if(request[i] != ' ')
                tmpstr[cnt++] = request[i];
          }
       }
    }

    if(n_semicolons > 1 || (n_semicolons > 0 && scope_op_found == TRUE))
    {  if(pel_appl_verbose == TRUE)
       {  (void)fprintf(stdout,"\n%sERROR%s PML syntax pups_error (expanded request has form server@host: <command list>)\n\n",boldOn,boldOff);
          (void)fflush(stdout);
       }

       (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
       return(PSRP_REQUEST_ERROR);
    }

    if(n_ats > 1)
    {  if(pel_appl_verbose == TRUE)
       {  (void)fprintf(stdout,"\n%sERROR%s PML syntax pups_error (expanded request has form server@host: <command list>)\n\n",boldOn,boldOff);
          (void)fflush(stdout);
       }

       return(PSRP_REQUEST_ERROR);
    }

    if(sc_pos - at_pos < 2 && (sc_pos != (-1) && at_pos != (-1)))
    {  if(pel_appl_verbose == TRUE)
       {  (void)fprintf(stdout,"\n%sERROR%s PML syntax pups_error (expanded request has form server@host: <command list>)\n\n",boldOn,boldOff);
          (void)fflush(stdout);
       }

       (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
       return(PSRP_REQUEST_ERROR);
    }

    if(at_pos > sc_pos || sc_pos == 0 || (at_pos == 0 && psuedoserver == FALSE))
    {  if(pel_appl_verbose == TRUE)
       {  (void)fprintf(stdout,"\n%sERROR%s PML syntax pups_error (expanded request has form server@host: <command list>)\n\n",boldOn,boldOff);
          (void)fflush(stdout);
       }

       (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
       return(PSRP_REQUEST_ERROR);
    }

    (void)strlcpy(request,tmpstr,SSIZE);
    if(n_semicolons > 0 && strin(request,":") == TRUE && strin(request,"::") == FALSE)
    {  i = 0;


       /*--------------------------------*/
       /* Get name of remote PSRP server */
       /*--------------------------------*/

       if(psuedoserver == FALSE)
       {  while(request[i] != ':' && request[i] != '@')
          {     rserver[i] = request[i];
                ++i;
          }

          rserver[i]                        = '\0';
          request[pups_strlen(request) - 1] = '\0';

          if(strcmp(rserver,"psrp") == 0)
             psuedoserver = TRUE;
       }

       if(request[i] == '@')
       {  int32_t cnt = 0;

          ++i;
          while(request[i] != ':')
          {     rhost[cnt] = request[i];

                ++i;
                ++cnt;
          }
          rhost[cnt] = '\0';


          /*----------------------------------------------------*/
          /* Make sure we don't try to connect to the localhost */
          /* if the user has typed <srv>@<localhost>: <cmd>     */
          /*----------------------------------------------------*/

          if(strcmp(rhost,"localhost") == 0)
             request_type = PSRP_LOCAL_AGGREGATE;
          else
             request_type = PSRP_REMOTE_REQUEST;
       }
       else
          request_type = PSRP_LOCAL_AGGREGATE;

       if(request_type == PSRP_REMOTE_REQUEST)
       {  
          #ifdef SSH_SUPPORT
          char psrp_client[SSIZE]                 = "";

          if(strcmp(appl_password,"notset") == 0)
             (void)strlcpy(psrp_client,"psrp",SSIZE);
          else
             (void)snprintf(psrp_client,SSIZE,"psrp -password");


          /*----------------------------------------------------------------------------*/
          /* Translate command list demarcation for fly aggregate on remote PSRP server */
          /*----------------------------------------------------------------------------*/

          (void)mchrep(';',",",&request[i+1]);


          /*----------------------------------------------------------------*/
          /* Are we contacting a real server process at the remote host, or */
          /* are we simply asking the remote psrp client for a service?     */
          /*----------------------------------------------------------------*/

          if(psuedoserver == TRUE)
             (void)snprintf(expanded_request,SSIZE,"%s -on %s -c \"%s; bye\"\n",
                                     psrp_client,appl_host,&request[i+1]);
          else
             (void)snprintf(expanded_request,SSIZE,"%s -on %s -c \"open %s; %s; close; bye\"\n",
                                                    psrp_client,appl_host,rserver,&request[i+1]);
          #else
          (void)strdate(date);
          (void)fprintf(stdout,"%sERROR%s %s %s (%d@%s:%s): remote command expansion not supported (no ssh support)\n",
                                                                                                        boldOn,boldOff,
                                                                          date,appl_name,appl_pid,appl_host,appl_owner);

          (void)fflush(stdout);


          /*-------------*/
          /* Host not up */
          /*-------------*/

          (void)strlcpy(psrp_c_code,"nsuperr",SSIZE);
          return(PSRP_REQUEST_ERROR);
          #endif /* SSH_SUPPORT */ 
       }
       else
       {

          /*------------------------------------------------------------------------*/
          /* Translate command list demarcation fly aggragate on local PSRP  server */
          /*------------------------------------------------------------------------*/

          (void)mchrep(';',",",&request[i+1]);


          /*---------------------------------------*/
          /* Build command to be run on local host */
          /*---------------------------------------*/

          (void)snprintf(expanded_request,SSIZE,"open %s; %s; close\n",rserver,&request[i+1]);
       }

       if(*flycom == FALSE)
       {  if(strin(expanded_request,"open") == TRUE && strin(expanded_request,"close") == TRUE)
             *flycom = TRUE;
       }
    }
    else
       (void)strlcpy(expanded_request,request,SSIZE);

    return(request_type);
}




/*-------------------------------------------------------------------------------------------------*/
/* Routine to yeild PSRP connection to any other clients which may be attached to the same server. */
/* Clients compete for the PSRP lock -- the client which gains the lock can talk to the server     */
/*-------------------------------------------------------------------------------------------------*/

_PRIVATE void psrp_yield_channel(void)

{   

    /*----------------------------------------------------------------------------------*/
    /* Temporarily close connection and free lock to let other clients talk to server   */
    /* Don't tell server -- it doesn't care whose requests it is processing!            */
    /*----------------------------------------------------------------------------------*/

    have_access_lock = FALSE;
    (void)pups_release_fd_lock(fileno(client_out));
}




/*--------------------------------------*/
/* Routine to grab open PSRP connection */
/*--------------------------------------*/

_PRIVATE  int32_t psrp_grab_channel(void) 

{   int32_t  trys       = 0;
    char tmp_str[SSIZE] = "\0";

    #ifdef PSRP_DEBUG
    (void)fprintf(stdout,"IN PSRP GRAB CHANNEL\n");
    (void)fflush(stdout); 
    #endif /* PSRP_DEBUG */


    /*----------------------------------------------*/
    /* If we already have the channel simply return */
    /*----------------------------------------------*/

    if(have_access_lock == TRUE)
       return(0);

    #ifdef PSRP_DEBUG
    (void)fprintf(stdout,"GRABOP\n");
    (void)fflush(stdout); 
    #endif /* PSRP_DEBUG */


    /*----------------------------------------------------------------------------------*/
    /* Wait till we reaquire the PSRP channel lock -- when we do re-open our connection */
    /* to the server                                                                    */
    /*----------------------------------------------------------------------------------*/
    /*-----------*/
    /* Soft link */
    /*-----------*/

    if(psrp_hard_link == FALSE)
    {  while(pups_get_fd_lock(fileno(client_out),TSTLOCK) != TRUE)
       {    pups_usleep(100);
            ++trys;
       
            if(trys > max_trys)
            {  if(pel_appl_verbose == TRUE)
               {  (void)fprintf(stdout,"\n%sERROR%s %s (%d@%s) busy\n",
                                                        boldOn,boldOff,
                                                           psrp_server,
                                                            server_pid,
                                                             psrp_host,
                                                          channel_name);
                  (void)fprintf(stdout,"%sERROR%s %s (%d@%s) request failed\n\n",boldOn,boldOff,psrp_server,server_pid,psrp_host);
                  (void)fflush(stdout);
               }

               return(-1);
           }
       }
    }


    /*-----------*/
    /* Hard link */
    /*-----------*/

    else
       (void)pups_get_fd_lock(fileno(client_out),GETLOCK);

    #ifdef PSRP_DEBUG
    (void)fprintf(stdout,"GRABOP LOCKED\n");
    (void)fflush(stdout); 
    #endif /* PSRP_DEBUG */

    if(do_rsnr == FALSE)
       server_pid = psrp_channelname_to_pid(appl_fifo_dir,psrp_server,psrp_host);


    /*-------------------------------------------------------------------*/
    /* We now have lock - lets re-establish full communication to server */
    /*-------------------------------------------------------------------*/

    have_access_lock = TRUE;


    /*--------------------------------------------------------------------*/
    /* We need to update the server so that all PSRP signals are directed */
    /* to us                                                              */
    /*--------------------------------------------------------------------*/

    (void)kill(server_pid,SIGINIT);

    #ifdef PSRP_DEBUG
    (void)fprintf(stderr,"SIGNAL %d (SIGINIT) sent\n",SIGINIT);
    (void)fflush(stderr);
    #endif /* PSRP_DEBUG */

    (void)fprintf(client_out,"GRAB %d %s\n",appl_pid,appl_password);
    (void)fflush(client_out);

    #ifdef PSRP_DEBUG
    (void)fprintf(stderr,"GRAB OUT\n");
    (void)fflush(stderr);
    #endif /* PSRP_DEBUG */

    (void)sscanf(tmp_str,"%d",&server_seg_cnt);
    (void)fprintf(client_out,"%s %d %s %s\n",appl_name,appl_pid,appl_host,remote_host_pathname);
    (void)fflush(client_out);


    /*-----------------------------------------------------*/
    /* Wait for server handshake indicating SIGINIT (GRAB) */
    /* has been serviced                                   */            
    /*-----------------------------------------------------*/

    psrp_waitfor_endop("EOP grope");

    #ifdef PSRP_DEBUG
    (void)fprintf(stdout,"GRABOP DONE (seg cnt = %d)\n",server_seg_cnt);
    (void)fflush(stdout);
    #endif /* PSRP_DEBUG */

    return(0);
}




/*--------------------------------*/
/* Wait for end of operation code */
/*--------------------------------*/

_PRIVATE void psrp_waitfor_endop(const char *opcode)

{   char tmp_str[SSIZE] = "\0";

    do {   (void)strlcpy(tmp_str,"\0",SSIZE);
           (void)fgets(tmp_str,SSIZE,client_in);
       } while(strncmp(tmp_str,opcode,pups_strlen(opcode) - 1) != 0);  

    #ifdef PSRP_DEBUG
    (void)fprintf(stdout,"WFE got %s\n",tmp_str);
    (void)fflush(stdout);
    #endif /* PSRP_DEBUG */
}




/*--------------------------------------------------------------*/
/* Add a new entry to the macro stack and load all macros in it */
/*--------------------------------------------------------------*/

_PRIVATE void update_macro_stack(const _BOOLEAN check_mode, const _BOOLEAN append, const char *macro_f_name)

{   uint32_t i;


    /*----------------------------------------------*/
    /* If we are not appending - simpy purge macros */
    /* clear mdf stack and initialise with current  */
    /* macro                                        */
    /*----------------------------------------------*/

    if(append == FALSE)
    {  purge_macros((char *)NULL,FALSE,FALSE);
       load_macro_definitions(check_mode,FALSE,macro_f_name);
       (void)strlcpy(mstack_f_name[0],macro_f_name,SSIZE);
       ++n_m_files;

       return;
    }
    else
       in_append_op = TRUE;


    /*---------------------------------------------------------*/
    /* See if the macro file we have just referenced is in the */
    /* macro file stack - if it isn't add it                   */
    /*---------------------------------------------------------*/

    for(i=0; i<n_m_files; ++i)
    {  ssize_t cmp_cnt;

       if(pups_strlen(mstack_f_name[i]) > pups_strlen(macro_f_name))
          cmp_cnt = pups_strlen(mstack_f_name[i]);
       else
          cmp_cnt = pups_strlen(macro_f_name);

       if(strncmp(mstack_f_name[i],macro_f_name,cmp_cnt) == 0)
          goto in_m_f_stack;
    }


    /*------------------------------------------------------*/
    /* Do we need to add this definitions file to the stack */
    /* of loaded definitions files?                         */
    /*------------------------------------------------------*/

    if(n_m_files < MAX_PSRP_MACRO_FILES)
    {  (void)strlcpy(mstack_f_name[n_m_files],macro_f_name,SSIZE);
       ++n_m_files;
    }
    else
    {  if(pel_appl_verbose == TRUE)
       {  (void)fprintf(stdout,"\n%sERROR%s too many macro definition files (maximum of %d permitted)\n\n",
                                                                                            boldOn,boldOff,
                                                                                      MAX_PSRP_MACRO_FILES);
          (void)fflush(stdout);
       }

       return;
    }


in_m_f_stack:


    /*---------------------------------------------------*/
    /* Re-load macro file stack after loading new member */
    /*---------------------------------------------------*/

    for(i=0; i<n_m_files; ++i)
    {  if(i == 0)
          (void)load_macro_definitions(check_mode,FALSE,mstack_f_name[i]);
       else
          (void)load_macro_definitions(check_mode,TRUE,mstack_f_name[i]);
    }

    in_append_op = FALSE;
}



/*-------------------------------*/
/* Define a new macro definition */
/*-------------------------------*/

_PRIVATE void edit_macro_definitions(const _BOOLEAN load, const _BOOLEAN append, const char *macro_f_name)

{   char edit_macro_command[SSIZE] = "",
         editor[SSIZE]             = "",
         username[SSIZE]           = "";


    /*------------------------------------------------------*/
    /* Editor needs a tty connection -- in general we       */
    /* cannot get such a connection if we have used rsh etc */
    /* to start this instance of the PSRP client -- check   */
    /* that we have a tty connection -- if not exit         */
    /*------------------------------------------------------*/

    if(isatty(1) == 0)
    {  (void)fprintf(stdout,"\n%sERROR%s not a tty connection -- cannot edit macro definition file\n\n",boldOn,boldOff);
       (void)fflush(stdout);

       return;
    }


    /*----------------------------------*/
    /* Get file editor -- default is vi */
    /*----------------------------------*/

    if((char *)getenv("EDITOR") == (char *)NULL)
       (void)strlcpy(editor,"vi",SSIZE);
    else
       (void)strlcpy(editor,(char *)getenv("EDITOR"),SSIZE);

    if(strcmp(macro_f_name,"default") == 0)
    {  

       /*--------------*/
       /* Get username */
       /*--------------*/

       if((char *)getenv("HOME") == (char *)NULL)
          (void)strlcpy(username,".",SSIZE);
       else
          (void)snprintf(username,SSIZE,"%s/.",(char *)getenv("HOME"));

       (void)snprintf(macro_f_name,SSIZE,"%s%s.mdf",username,appl_name);
    }


    /*--------------------------------------------------*/
    /* If we have no macro defintions file -- create it */
    /*--------------------------------------------------*/

    if(access(macro_f_name,F_OK | R_OK | W_OK) == (-1))
       (void)pups_creat(macro_f_name,0600);


    /*------------------------------------------*/
    /* Edit the macro file for this PSRP client */
    /*------------------------------------------*/

    (void)snprintf(edit_macro_command,SSIZE,"bash -c \"%s %s\"",editor,macro_f_name);
    (void)system(edit_macro_command);

    if(load == TRUE)
       update_macro_stack(TRUE,append,macro_f_name);
}




/*-----------------------------------------*/
/*  Load macro definitions from macro file */
/*                                         */
/*  Each macro is of the form:             */
/*                                         */
/*  tag_1                                  */
/*  tag_2                                  */
/*  tag_3                                  */
/*  {   <macro command definition>         */
/*  }                                      */
/*-----------------------------------------*/

_PRIVATE int32_t load_macro_definitions(const _BOOLEAN check_mode, const _BOOLEAN append, const char *macro_f_name)

{   char  next_line[SSIZE] = "",
          tmp_str[SSIZE]   = "",
          root_tag[SSIZE]  = "";

    FILE *macro_stream = (FILE *)NULL;

    ssize_t i,
            start_str,
            end_str,
            start                      = 0;

    uint32_t    macro_cnt,
                 n_labels              = 0,
                 head_cnt              = 0,
                 line_cnt              = 0;

    _BOOLEAN in_body                   = FALSE,
             begin_body_transcription  = FALSE,
             macro_head                = TRUE,
             has_head                  = FALSE,
             has_body                  = FALSE,
             max_tag_warning           = FALSE,
             is_comment                = FALSE,
             macro_decoded             = TRUE;

    if(access(macro_f_name,F_OK | R_OK | W_OK) == (-1))
    {  if(pel_appl_verbose == TRUE)
       {  (void)fprintf(stdout,"\n%sERROR%s PML macro file \"%s\" does not exist\n\n",boldOn,boldOff,macro_f_name);
          (void)fflush(stdout);

          (void)strlcpy(psrp_c_code,"cmnserr",SSIZE);
          return(-1);
       }
    }

    macro_stream = pups_fopen(macro_f_name,"r",LIVE);

    if(pel_appl_verbose == TRUE)
    {  (void)fprintf(stdout,"PML (Process Enquiry Language) version 1.00\n");
       if(check_mode == TRUE)
          (void)fprintf(stdout,"Checking macro file \"%s\"\n\n",macro_f_name);
       else
          (void)fprintf(stdout,"Processing macro file \"%s\"\n\n",macro_f_name);

       (void)fflush(stdout);
    }


    /*------------------------*/
    /* Initialise macro store */
    /*------------------------*/

    if(check_mode == FALSE)
    {  if(append == FALSE)
          purge_macros((char *)NULL,FALSE,FALSE);

       macro_cnt = n_macros;
    }
    else
       macro_cnt = 0;

    do {

           /*------------------------------------*/ 
           /* Skip emtpy lines and comment lines */
           /*------------------------------------*/ 


read_next_line:

           do {

                   /*-----------------------------------------*/
                   /* Check for end of macro definitions file */ 
                   /*-----------------------------------------*/

                   if(feof(macro_stream)) 
                   {  if(macro_decoded == FALSE)
                      {  if(pel_appl_verbose)
                         {  if(line_cnt > 1)
                               (void)fprintf(stdout,"%sERROR%s %s %d: premature end of file\n",boldOn,boldOff,macro_f_name,line_cnt);
                            else
                               (void)fprintf(stdout,"%sERROR%s %s: empty file\n",boldOn,boldOff,macro_f_name);

                            (void)fflush(stdout);
                         }
                      }


                      /*--------------------------------------*/
                      /* Check for non-unique macro tags      */
                      /* If we have a name clash purge loaded */
                      /* macros and exit                      */
                      /*--------------------------------------*/

                      if(check_macro_tags(macro_cnt,macro_f_name) == FALSE)
                      {  if(check_mode == FALSE)
                            purge_macros(macro_f_name,TRUE,FALSE);

                         (void)strlcpy(psrp_c_code,"cmnserr",SSIZE);
                         return(-1);
                      }

                      (void)pups_fclose(macro_stream);

                      if(check_mode == TRUE)
                      {  if(pel_appl_verbose == TRUE)
                         {  (void)fprintf(stdout,"\n%d macro definition(s) checked\n\n",macro_cnt);
                            (void)fflush(stdout);
                         }
                      }
                      else
                      {  if(pel_appl_verbose == TRUE)
                         {  (void)fprintf(stdout,"\n%d macro definition(s) loaded\n\n",macro_cnt);
                            (void)fflush(stdout);
                         }

                         n_macros = macro_cnt;
                      }

                      return(0);
                   }

                   (void)fgets(next_line,SSIZE,macro_stream); 

                   while(next_line[start++] == ' ');
                   is_comment = FALSE;
                   start      = 0;


                   /*-----------------*/
                   /* Ignore comments */
                   /*-----------------*/

                   if(next_line[start] == '#')
                      is_comment = TRUE; 
                   else 
                   {

                      /*-------------------------*/
                      /* Strip trailing comments */
                      /*-------------------------*/

                      while(next_line[start] != '#' && next_line[start] != '\n' && next_line[start] != '\0')
                            ++start;

                      if(next_line[start] == '#')
                         next_line[start] = '\0';
                   }

                   ++line_cnt;
              } while(is_comment == TRUE || strcmp(next_line,"\n") == 0);


           /*-------------------------------------------------------*/
           /* Decode macro tags -- these are the names by which the */
           /* macro is known to the client/user                     */
           /*-------------------------------------------------------*/

           macro_decoded            = FALSE;

           if(strin(next_line,"atomic") == TRUE && in_body == FALSE)
           {  if(pel_appl_verbose == TRUE)
              {  (void)fprintf(stdout,"\n%sERROR%s %s %d: PML syntax pups_error - \"atomic\" is a reserved word\n\n",boldOn,boldOff,macro_f_name,line_cnt);
                 (void)fflush(stdout);
              }

              if(check_mode == FALSE)
                 purge_macros(macro_f_name,TRUE,FALSE);

              (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
              return(-1);
           }

           if(strin(next_line,"{") == TRUE)
           {  if(strchcnt('{',next_line) > 1)
              {  if(pel_appl_verbose == TRUE)
                 {  (void)fprintf(stdout,"\n%sERROR%s %s %d: PML syntax pups_error - \"{\" occurs multiply in macro %s\n\n",boldOn,boldOff,macro_f_name,line_cnt,root_tag);
                    (void)fflush(stdout);
                 }

                 if(check_mode == FALSE)
                    purge_macros(macro_f_name,TRUE,FALSE);

                 (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                 return(-1);
              }

              if(in_body == TRUE)
              {  if(pel_appl_verbose == TRUE)
                 {  (void)fprintf(stdout,"\n%sERROR%s %s %d: PML syntax pups_error - \"{\" found embedded in body of macro %s\n\n",boldOn,boldOff,macro_f_name,line_cnt,root_tag);
                    (void)fflush(stdout);
                 }

                 if(check_mode == FALSE)
                    purge_macros(macro_f_name,TRUE,FALSE);

                 (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                 return(-1);
              }

              if(has_head == FALSE)
              {  if(pel_appl_verbose == TRUE)
                 {  (void)fprintf(stdout,"\n%sERROR%s %s %d: PML syntax pups_error - macro has no head\n\n",boldOn,boldOff,macro_f_name,line_cnt);
                    (void)fflush(stdout);
                 }

                 if(check_mode == FALSE)
                    purge_macros(macro_f_name,TRUE,FALSE);
                 (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                 return(-1);
              }

              in_body                  = TRUE;
              begin_body_transcription = TRUE;

              if(pups_strlen(next_line) < 2)
                 goto read_next_line;
              else
              {  (void)strlcpy(tmp_str,strpch('{',next_line),SSIZE);
                 (void)strlcpy(next_line,tmp_str,SSIZE);
              }
           }
           else if(strin(next_line,"}") == TRUE)
           {  if(has_body == FALSE)
              {  if(pel_appl_verbose == TRUE)
                 {  (void)fprintf(stdout,"\n%sERROR%s %s %d: PML syntax pups_error - macro %s has no body\n\n",boldOn,boldOff,macro_f_name,line_cnt,root_tag);
                    (void)fflush(stdout);
                 }

                 if(check_mode == FALSE)
                    purge_macros(macro_f_name,TRUE,FALSE);

                 (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                 return(-1);
              }

              if(pups_strlen(next_line) > 4)
              {  if(pel_appl_verbose == TRUE)
                 {  (void)fprintf(stdout,"\n%sERROR%s %s %d: PML syntax pups_error - \"}\" found embedded in body of %s\n\n",boldOn,boldOff,macro_f_name,line_cnt,root_tag);
                    (void)fflush(stdout);
                 }

                 if(check_mode == FALSE)
                    purge_macros(macro_f_name,TRUE,FALSE);

                 (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                 return(-1);
              }

              if(in_body == FALSE)
              {  if(pel_appl_verbose == TRUE)
                 {  (void)fprintf(stdout,"\n%sERROR%s %s %d: PML syntax pups_error - \"}\" found embedded in head of %s\n\n",boldOn,boldOff,macro_f_name,line_cnt,root_tag);
                    (void)fflush(stdout);
                 }

                 if(check_mode == FALSE)
                    purge_macros(macro_f_name,TRUE,FALSE);

                 (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                 return(-1);
              }

              macro_decoded = TRUE;
              macro_head    = TRUE;
              has_head      = FALSE;
              in_body       = FALSE;
              ++macro_cnt;
              head_cnt      = 0;
              if(macro_cnt == MAX_PSRP_MACROS)
              {  if(pel_appl_verbose == TRUE)
                 {  (void)fprintf(stdout,"\n%sERROR%s PML syntax pups_error: cannot load any more macros (maximum of %d macros permitted)\n\n",boldOn,boldOff,MAX_PSRP_MACROS);
                    (void)fflush(stdout);
                 }

                 if(check_mode == FALSE)
                    purge_macros(macro_f_name,TRUE,FALSE);

                 (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                 return(-1);
              }

              goto read_next_line;
           }


           /*----------------------------------*/
           /* Strip leading spaces from string */
           /*----------------------------------*/

           for(i=0; i<pups_strlen(next_line); ++i)
           {  if(next_line[i] != ' ' && next_line[i] != '{')
              {  start_str = i;
                 goto sl_spaces_done;
              }
          }

sl_spaces_done:


           /*------------------------------------------------*/
           /* Strip trailing spaces and linefeed from string */
           /*------------------------------------------------*/

           for(i=pups_strlen(next_line) - 1; i>= 0; --i)
              if(next_line[i] != '\n' && next_line[i] != ' ' && (int32_t)next_line[i] != 0)
              {  end_str = i;
                 goto st_spaces_done;
              }


st_spaces_done:

           next_line[end_str + 1] = '\0';


           /*------------------------------------------------*/
           /* If all we have left is a new-line -- ignore it */
           /*------------------------------------------------*/

           if(strcmp(next_line,"\n") != 0)
           {

           (void)strlcpy(tmp_str,(char *)&next_line[start_str],SSIZE);
           (void)strlcpy(next_line,tmp_str,SSIZE);


           /*---------------------------------------------------*/
           /* If we are in the macro body simply copy it to the */
           /* body area of the PSRP client macro table          */
           /*---------------------------------------------------*/

           if(in_body == TRUE)
           {  char label[MAX_LABELS][SSIZE] = { [0 ... MAX_LABELS-1] = {""}}; 


              /*----------------------------------------------*/
              /* If this is a label - check that it is unique */
              /*----------------------------------------------*/

              if(next_line[0] == '%')
              {  uint32_t j; 


                 /*--------------------*/
                 /* Check label syntax */
                 /*--------------------*/

                 if(sscanf(next_line,"%s%s",tmp_str,tmp_str) > 1)
                 {  (void)fprintf(stdout,"\n%sERROR%s %s %d: PML syntax pups_error - malformed label \"%s\"\n\n",boldOn,boldOff,macro_f_name,line_cnt,next_line);
                    (void)fflush(stdout);

                    if(check_mode == FALSE)
                       purge_macros(macro_f_name,TRUE,FALSE);

                    (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                    return(-1);
                 }


                 /*----------------------------------------------------*/
                 /* Make sure that we have space to process this label */
                 /*----------------------------------------------------*/

                 if(n_labels == MAX_LABELS)
                 {  (void)fprintf(stdout,"\n%sERROR%s %s %d: PML syntax pups_error - too many labels (max %d)\n\n",boldOn,boldOff,macro_f_name,line_cnt,MAX_LABELS);
                    (void)fflush(stdout);

                    if(check_mode == FALSE)
                       purge_macros(macro_f_name,TRUE,FALSE);

                    (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                    return(-1);
                 }

                 for(j=0; j<n_labels; ++j)
                 {  ssize_t cmp_cnt;

                    if(pups_strlen(next_line) > pups_strlen(label[j]))
                       cmp_cnt = pups_strlen(next_line);
                    else
                       cmp_cnt = pups_strlen(label[j]);

                    if(strncmp(label[j],next_line,cmp_cnt) == 0)
                    {  (void)fprintf(stdout,"\n%sERROR%s %s %d: PML syntax pups_error - label \"%s\" is not unique\n\n",boldOn,boldOff,macro_f_name,line_cnt,label[j]);
                       (void)fflush(stdout);

                       if(check_mode == FALSE)
                          purge_macros(macro_f_name,TRUE,FALSE);

                       (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                       return(-1);
                    } 
                 }

                 (void)strlcpy(label[n_labels++],next_line,SSIZE);
                 ++n_labels;
              }

              if(begin_body_transcription == TRUE)
              {  if(check_mode == FALSE)
                 {  begin_body_transcription = FALSE;
                    (void)strlcpy(macro[macro_cnt].body,next_line,SSIZE);
                 }

                 has_body = TRUE;
              }
              else if(check_mode == FALSE)
              {  (void)strlcat(macro[macro_cnt].body,"; ",SSIZE);
                 (void)strlcat(macro[macro_cnt].body,next_line,SSIZE);
              }
           } 
           else
           {  char strdum1[SSIZE] = "",
                   strdum2[SSIZE] = "";


              /*--------------------------------------------------*/
              /* Check that we only have one synonym per line in  */
              /* the head of the macro                            */
              /*--------------------------------------------------*/

              if(sscanf(next_line,"%s %s",strdum1,strdum2) > 1 && strdum2[0] != '#')
              {  if(pel_appl_verbose == TRUE)
                 {  (void)fprintf(stdout,"\n%sERROR%s %s %d: PML syntax pups_error - head has more than one synonym on line (%s, %s ...)\n\n",
                                                                                                                               boldOn,boldOff,
                                                                                                        macro_f_name,line_cnt,strdum1,strdum2);
                    (void)fflush(stdout);
                 }

                 if(check_mode == FALSE)
                    purge_macros(macro_f_name,TRUE,FALSE);

                 (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                 return(-1);
              }
              (void)strlcpy(next_line,strdum1,SSIZE);

              if(macro_head == TRUE)
              {  if(check_mode == FALSE)
                 {  if(pel_appl_verbose == TRUE)
                    {  (void)fprintf(stdout,"\n%s %-4d: loading PML macro (root tag %s)\n\n",macro_f_name,line_cnt,next_line);
                       (void)fflush(stdout);
                    }
                 }
                 else
                 {  if(pel_appl_verbose == TRUE)
                    {  (void)fprintf(stdout,"\n%s %-4d: PML macro (root tag %s) OK\n\n",macro_f_name,line_cnt,next_line);
                       (void)fflush(stdout);
                    }
                 }
              
                 macro_head = FALSE;
              }


              /*--------------------------------------------------*/
              /* We are in the head of the macro -- copy alias to */
              /* next free slot in alias table                    */
              /*--------------------------------------------------*/

              if(head_cnt < MAX_MACRO_TAGS)
              {  if(check_mode == FALSE)
                 {  if(has_head == FALSE)
                    {  (void)strlcpy(root_tag,next_line,SSIZE);
                       (void)strlcpy(macro[macro_cnt].mdf,macro_f_name,SSIZE);
                    }

                    macro[macro_cnt].cnt = head_cnt + 1;
                    (void)strlcpy(macro[macro_cnt].tag[head_cnt],next_line,SSIZE);
                 }

                 ++head_cnt;
              }
              else
              {  if(max_tag_warning == FALSE)
                 {  max_tag_warning = TRUE;

                    if(pel_appl_verbose == TRUE)
                    {  (void)fprintf(stdout,"\n%sERROR%s %s %d: PML syntax pups_error - head of %s has too many tags\n\n",boldOn,boldOff,macro_f_name,line_cnt,root_tag);
                       (void)fflush(stdout);
                    }

                    if(check_mode == FALSE)
                       purge_macros(macro_f_name,TRUE,FALSE);

                    (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                    return(-1);
                 }
              }

              has_head = TRUE;
           }
           }

       } while(TRUE);
}




/*------------------------------------------------------------------------------------------*/
/* Substitute macro arguments - PML uses a simple scheme $1 - $8 identify arguments 1 to 8. */
/* Each argument is substituted for the appropriate item in the argument list               */
/*------------------------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN substitute_arguments(char *macro_body, const uint32_t    n_args, const char argument_list[MAX_MACRO_ARGS][SSIZE])

{   uint32_t i,
             index,
             cnt                           = 0;

    char     dummy[SSIZE]                  = "",
             substituted_macro_body[SSIZE] = "";


    /*----------------------------------------------------------*/
    /* Make sure that we have correct number of dummy variables */
    /* in macro body                                            */
    /*----------------------------------------------------------*/

    for(i=0; i<n_args; ++i)
    {  (void)snprintf(dummy,SSIZE,"$%d",i+1);
       if(strin(macro_body,dummy) == FALSE)
          return(FALSE);
    }


    /*---------------------------------------------------------------------*/
    /* We have the correct number of arguments - so lets substitute them   */
    /* NOTE: we have assumed that we are never likely to have a body which */
    /* occupies more than 95% of the space assigned to it                  */
    /*---------------------------------------------------------------------*/

    (void)strlcpy(substituted_macro_body,"",SSIZE);
    for(i=0; i<pups_strlen(macro_body); ++i)
    {  if(cnt > SSIZE)
          pups_error("[substitute_arguments]  macro body too big");

       if(macro_body[i] == '$')
       {  (void)sscanf(&macro_body[i],"$%d",&index);
          (void)strlcat(substituted_macro_body,argument_list[index-1],SSIZE);
          cnt += pups_strlen(argument_list[index-1]);
          i   += 2; 
       }
       else
       {  substituted_macro_body[cnt++] = macro_body[i];
          substituted_macro_body[cnt+1] = '\0';
       }
    }

    (void)strlcpy(macro_body,substituted_macro_body,SSIZE);
    return(TRUE);
}




/*---------------------------------------*/
/* Expand any macros in the request_line */
/*---------------------------------------*/

_PRIVATE _BOOLEAN expand_macros(const char *request_line, char *expanded_request_line)

{   ssize_t  i,
             j,
             cnt                                  = 0,
             start                                = 0;

    uint32_t n_args                               = 0;

    _BOOLEAN looper,
             is_macro                             = FALSE;

    char     tmp_str[SSIZE]                       = "",
             next_item[SSIZE]                     = "",
             argument_list[MAX_MACRO_ARGS][SSIZE] = { [0 ... MAX_MACRO_ARGS-1] = {""}};


    /*---------------------------------*/
    /* Initialise sub-string extractor */
    /*---------------------------------*/

    (void)m_strext(EXPAND_MACRO_STREXT,'\2',(char *)NULL,(char *)NULL);


    /*-----------------------*/
    /* Build expanded macros */
    /*-----------------------*/

    (void)strlcpy(expanded_request_line,"",SSIZE);
    do {

          /*-------------------------------------*/
          /* Is request line a compound command? */
          /*-------------------------------------*/

          if(strin(request_line,";") == TRUE)

             /*-----------------------------*/
             /* Process as compound command */
             /*-----------------------------*/

             looper = m_strext(EXPAND_MACRO_STREXT,';',tmp_str,request_line);
          else
          {

             /*---------------------------*/
             /* Process as atomic command */
             /*---------------------------*/

             looper = FALSE;
             (void)strlcpy(tmp_str,request_line,SSIZE);
          }

          i   = 0;
          cnt = 0;


          /*----------------*/
          /* Get macro name */
          /*----------------*/

          while(tmp_str[i] == ' ')
                ++i;

          while(tmp_str[i] != ' ' && tmp_str[i] != '\n' && tmp_str[i] != '\0')
          {     next_item[cnt++] = tmp_str[i];
                ++i;
          }

          next_item[cnt] = '\0';


          /*-------------------------------------------------------------------------*/
          /* "if", "atomic" and "resume" statements should be left alone as they are */
          /* dealt with in psrp_process_command                                      */
          /* Anything with a ! in it is by definition not a macro                    */
          /*-------------------------------------------------------------------------*/

          if(strcmp(next_item,"if") == 0 || strcmp(next_item,"atomic") == 0 || strcmp(next_item,"resume") == 0 || next_item[0] == '!')
          {  (void)strlcpy(next_item,tmp_str,SSIZE);
             goto reserved_word_statement;
          }
  
          if(tmp_str[i] != '\n' && tmp_str[i] != '\0')
          {  

             /*--------------------------*/
             /* Expand the argument list */
             /*--------------------------*/

             n_args = 0;
             do { 

                     /*-----------------------*/
                     /* Move to next argument */
                     /*-----------------------*/

                     while(tmp_str[i] == ' ' && tmp_str[i] != '\n' && tmp_str[i] != '\0')
                           ++i;

                     if(tmp_str[i] != '\n' && tmp_str[i] != '\0')
                     {  cnt = 0;


                        /*----------------------------------------------------------------*/
                        /* Read next argument (globbing between parentheses if necessary) */
                        /*----------------------------------------------------------------*/

                        if(tmp_str[i] == '"')
                        {  
                           argument_list[n_args][cnt++] = tmp_str[i++];
                           while(tmp_str[i] != '"' && tmp_str[i] != '\n' && tmp_str[i] != '\0')
                           {     argument_list[n_args][cnt++] = tmp_str[i]; 
                                 ++i;
                           }


                           /*---------------------------------------------------------------*/
                           /* No delimiter (end of input string) - flag pups_error and exit */
                           /*---------------------------------------------------------------*/

                           if(tmp_str[i] == '\0' || tmp_str[i] == '\n')
                           {  if(pel_appl_verbose == TRUE)
                              {  (void)fprintf(stdout,"\n%sERROR%s PML syntax pups_error: closing \" delimiter not found (%s)\n\n",boldOn,boldOff,last_macro);
                                 (void)fflush(stdout);
                              }

                              (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                              return(FALSE);
                           }
                           argument_list[n_args][cnt++] = tmp_str[i++];
                        }
                        else
                        {  while(tmp_str[i] != ' ' && tmp_str[i] != '\n' && tmp_str[i] != '\0')
                           {     argument_list[n_args][cnt++] = tmp_str[i];
                                 ++i;
                           }
                        }


                        /*-------------------------*/
                        /* Terminate next argument */
                        /*-------------------------*/

                        argument_list[n_args][cnt] = '\0';
                        ++n_args;


                       /*---------------------------------------------*/
                       /* Check that we don't have too many arguments */
                       /*---------------------------------------------*/

                       if(n_args > MAX_MACRO_ARGS)
                       {  if(pel_appl_verbose == TRUE)
                          {  (void)fprintf(stdout,"\n%sERROR%s PML syntax pups_error: PML macros may have up to %d arguments\n\n",boldOn,boldOff,MAX_MACRO_ARGS);
                             (void)fflush(stdout);
                          }

                          (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
                          return(FALSE);
                       }
                   }

                } while(tmp_str[i] != '\n' && tmp_str[i] != '\0');
          }


          /*-------------------------*/
          /* Expand (possible) macro */
          /*-------------------------*/

          (void)strail(next_item,' ');
          is_macro = FALSE;
          for(i=0; i<n_macros; ++i)
          {  for(j=0; j<macro[i].cnt; ++j)
             {  int32_t cmp_cnt;

                if(pups_strlen(macro[i].tag[j]) > pups_strlen(next_item)) 
                   cmp_cnt = pups_strlen(macro[i].tag[j]);
                else
                   cmp_cnt = pups_strlen(next_item);
 
                if(strncmp(macro[i].tag[j],next_item,cmp_cnt) == 0)
                { 

                   /*-------------------*/
                   /* Get body of macro */ 
                   /*-------------------*/

                   (void)strlcpy(next_item,macro[i].body,SSIZE);


                   /*----------------------------------------------------*/
                   /* If the macro has any arguments substitute them now */
                   /*----------------------------------------------------*/

                   if(n_args > 0)
                   {  if(substitute_arguments(next_item,n_args,argument_list) == FALSE)
                      {  if(pel_appl_verbose == TRUE)
                         {  (void)fprintf(stdout,"\n%sERROR%s PML syntax pups_error: macro argument substitution failed\n\n",boldOn,boldOff);
                            (void)fflush(stdout);
                         }

                         (void)strlcpy(psrp_c_code,"psuberr",SSIZE);
                         return(FALSE);
                      } 
                   }
                   else
                   {

                      /*--------------------------------------------------------------------*/
                      /* Macro function has arguments but we have failed to substitute then */
                      /*--------------------------------------------------------------------*/

                      if(strin(next_item,"$") == TRUE)
                      {  if(pel_appl_verbose == TRUE)
                         {  (void)fprintf(stdout,"\n%sERROR%s PML syntax pups_error: macro has unsubstituted arguments\n\n",boldOn,boldOff);
                            (void)fflush(stdout);
                         }

                         (void)strlcpy(psrp_c_code,"psuberr",SSIZE);
                         return(FALSE);
                      }
                   }

                   (void)strlcpy(last_macro,macro[i].tag[j],SSIZE);
                   is_macro = TRUE;
                   goto macro_expanded;
                }
              }
           }

reserved_word_statement:


           /*-------------------------------------------------------------------------*/
           /* If this is not a macro we need whole input buffer (including arguments) */
           /*-------------------------------------------------------------------------*/

           while(tmp_str[start] == ' ')
                 ++start;
   
           (void)strlcpy(next_item,&tmp_str[start],SSIZE);


macro_expanded: 

           (void)strlcat(expanded_request_line,next_item,SSIZE);

           if(looper == TRUE)
              (void)strlcat(expanded_request_line,"; ",SSIZE); 

       } while(looper == TRUE);

    if(is_macro == TRUE)
       (void)strlcat(expanded_request_line,"; end",SSIZE);

    return(TRUE);
}





/*--------------------*/
/* Show loaded macros */
/*--------------------*/

_PRIVATE void purge_macros(const char *macro_f_name, const _BOOLEAN appl_verbose, const _BOOLEAN erase_macro_files)

{   uint32_t i,
             j;

    if(n_macros == 0)
    {  if(pel_appl_verbose == TRUE)
       {  (void)fprintf(stdout,"\nno macros loaded\n\n");
          (void)fflush(stdout);
       }

       return;
    }

    for(i=0; i<n_macros; ++i)
    {  for(j=0; j<macro[i].cnt; ++j)
          (void)strlcpy(macro[i].tag[j],"",SSIZE);

       (void)strlcpy(macro[i].body,"",SSIZE);
       macro[i].cnt = 0;
    }

    n_macros = 0;

    if(macro_f_name == (char *)NULL)
    {  if(pel_appl_verbose == TRUE)
       {  (void)fprintf(stdout,"\nall macros removed\n\n");
          (void)fprintf(stdout,"\n\n");
       }

       if(erase_macro_files == TRUE)
       {  for(i=0; i<n_m_files; ++i)
             (void)unlink(mstack_f_name[i]);

          if(pel_appl_verbose == TRUE)
          {  (void)fprintf(stdout," (all loaded macro files deleted)");
             (void)fflush(stdout);
          }
       }

       if(pel_appl_verbose == TRUE)
       {  (void)fprintf(stdout,"\n\n");
          (void)fflush(stdout);
       }


       /*-----------------------------------------------------------*/
       /* If we are called by an append operation - retain names of */
       /* macro definition files, as they will be required for      */
       /* append-reload operation                                   */
       /*-----------------------------------------------------------*/

       if(in_append_op == FALSE)
       {  for(i=0; i<n_m_files; ++i)
             (void)strlcpy(mstack_f_name[i],"",SSIZE);
          n_m_files = 0;
       }
    }
    else 
    {  if(pel_appl_verbose == TRUE)
       {  (void)fprintf(stdout,"\nall macros from macro file %s removed\n\n",macro_f_name);
          (void)fprintf(stdout,"\n\n");
       }


       /*-----------------------------------------------------------------------*/
       /* Reload all macro files (except the one we have just asked to delete)  */
       /* This will be quite inefficient in macros which delete multipled files */
       /*-----------------------------------------------------------------------*/

       for(i=0; i<n_m_files; ++i)
          if(strcmp(mstack_f_name[i],macro_f_name) == 0)
          {  if(erase_macro_files == TRUE)
             {  if(pel_appl_verbose == TRUE)
                {  (void)fprintf(stdout," (macro file %s deleted)",macro_f_name);
                   (void)fflush(stdout);
                }

                (void)unlink(macro_f_name);  
             }

             for(j=i; j<n_m_files-1; ++j)
                (void)strlcpy(mstack_f_name[j],mstack_f_name[j+1],SSIZE);

             --n_m_files; 


             /*----------------------------------*/
             /* Reload all remaining macro files */
             /*----------------------------------*/

             for(j=0; j<n_m_files; ++j)
                if(j == 0)
                   (void)load_macro_definitions(FALSE,FALSE,mstack_f_name[i]);
                else
                   (void)load_macro_definitions(FALSE,TRUE,mstack_f_name[i]);

             if(pel_appl_verbose == TRUE)
             {  (void)fprintf(stdout,"\n\n");
                (void)fflush(stdout);
             }

             return;
          }
     }
}




/*-------------------------------------------------*/
/* Check macro tags to ensure that they are unique */
/*-------------------------------------------------*/

_PRIVATE _BOOLEAN check_macro_tags(const uint32_t    n_macros, const char *macro_f_name)

{   uint32_t i,
             j,
             k,
             l,
             m,
             eff_n_tags,
             n_clashes    = 0;

    _BOOLEAN ret          = TRUE;

    char clash_tags[MAX_CLASH_TAGS][SSIZE] = { [0 ... MAX_CLASH_TAGS-1] = {""}};

    for(i=0; i<n_macros; ++i)
    {  for(j=i+1; j<n_macros; ++j)
       {  if(macro[i].cnt > macro[j].cnt)
             eff_n_tags = macro[i].cnt;
          else
             eff_n_tags = macro[j].cnt;

          for(k=0; k<eff_n_tags; ++k)
          {  for(l=k; l<eff_n_tags; ++l)
             {  if(strcmp(macro[i].tag[k],macro[j].tag[l]) == 0)
                {  for(m=0; m<n_clashes; ++m)
                      if(strcmp(clash_tags[m],macro[i].tag[k]) == 0)
                         goto already_found;

                   if(n_clashes > MAX_CLASH_TAGS)
                   {  if(pel_appl_verbose == TRUE)
                      {  (void)fprintf(stdout,"%sERROR%s %s: too many tags are non unique\n",boldOn,boldOff,macro_f_name);
                         (void)fflush(stdout);
                      }

                      (void)strlcpy(psrp_c_code,"mtuerr",SSIZE);
                      return(FALSE);
                   }
                   else
                   {  (void)strlcpy(clash_tags[n_clashes],macro[i].tag[k],SSIZE);
                      ++n_clashes;
                   }


already_found:     ret = FALSE;
                }
             }
          }
       }
    }

    for(i=0; i<n_clashes; ++i)
    {  if(pel_appl_verbose == TRUE)
       {  (void)fprintf(stdout,"%sERROR%s %s: \"%s\" is not unique\n",boldOn,boldOff,macro_f_name,clash_tags[i]);
          (void)fflush(stdout);
       }

       (void)strlcpy(psrp_c_code,"mtuerr",SSIZE);
    }

    return(ret);
}




/*----------------------------------------------------*/
/* Add the current macro to the end of the macro file */
/*----------------------------------------------------*/

_PRIVATE void catenate_macro(const char *name, const char *body, const char *macro_f_name)

{   FILE *macro_stream = (FILE *)NULL;


    /*------------------------------------*/
    /* If file does not exist - create it */
    /*------------------------------------*/

    if(access(macro_f_name,F_OK | R_OK | W_OK) == (-1))
       (void)pups_creat(macro_f_name,0600);


    /*------------------------------------------*/
    /* Build new macro and add it to macro file */
    /*------------------------------------------*/

    macro_stream = pups_fopen(macro_f_name,"a",LIVE);

    (void)strdate(date);
    (void)fprintf(macro_stream,"\n");
    (void)fprintf(macro_stream,"# %s: Macro %s catenated from PSRP client\n",macro_f_name,name);
    (void)fprintf(macro_stream,"%s\n",name);
    (void)fprintf(macro_stream,"{\n");
    (void)fprintf(macro_stream,"%s\n",body);
    (void)fprintf(macro_stream,"}\n");
    (void)fprintf(macro_stream,"\n");
    (void)fflush(macro_stream);


    /*-----------------------------------------------------*/
    /* Re-read macro file (to make catenated macro active) */
    /*-----------------------------------------------------*/

    (void)pups_fclose(macro_stream);
}




/*--------------------*/
/* Show loaded macros */
/* -------------------*/


_PRIVATE void show_macro_tags(void)

{   uint32_t i,
             j;

    (void)fprintf(stdout,"\n%d macro(s) currently loaded\n\n",n_macros);
    (void)fflush(stdout);

    for(i=0; i<n_macros; ++i)
    {  (void)fprintf(stdout,"macro %d (mdf %s):",i,macro[i].mdf);

       for(j=0; j<macro[i].cnt; ++j)
          if(j == 0)
             (void)fprintf(stdout," [root tag] %s;",macro[i].tag[j]);
          else
             (void)fprintf(stdout," %s;",macro[i].tag[j]);

       (void)fprintf(stdout,"\n");
       (void)fflush(stdout);
    } 

    (void)fprintf(stdout,"\n\n");
    (void)fflush(stdout);
}




/*----------------------------*/
/* Catenate macro definitions */
/*----------------------------*/

_PRIVATE void builtin_catenate_macro(char *request)

{   int32_t i,
            ret,
            cnt,
            m_body_bra,
            m_body_ket,
            start,
            end,
            body_size;

   char     tmp_str[SSIZE]  = "",
            tag_name[SSIZE] = "";


   /*--------------------------------------------------------------------*/
   /* Before we do anything else we must extract the macro body (if any) */
   /* specified on the command line                                      */
   /*--------------------------------------------------------------------*/

   m_body_bra = strchcnt('{',request);
   m_body_ket = strchcnt('}',request);


   /*---------------------------------------------------------------------*/
   /* Check that the macro body is enclosed by bra and ket ( { <body> } ) */
   /*---------------------------------------------------------------------*/

   if(m_body_bra > 1)
   {  if(pel_appl_verbose == TRUE)
      {  (void)fprintf(stdout,"\n%sERROR%s PML syntax pups_error - to many \"{\" symbols in inline macro definition\n\n",boldOn,boldOff);
         (void)fflush(stdout);
      }

      (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
      return;
   }   
   else if(m_body_ket > 1)
   {  if(pel_appl_verbose == TRUE)
      {  (void)fprintf(stdout,"\n%sERROR%s PML syntax pups_error - to many \"}\" symbols in inline macro definition\n\n",boldOn,boldOff);
         (void)fflush(stdout);
      }

      (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
      return;
   }
   else if((m_body_bra == 1 && m_body_ket == 0) || (m_body_bra == 0 && m_body_ket == 1))
   {  if(pel_appl_verbose == TRUE)
      {  (void)fprintf(stdout,"\n%sERROR%s PML syntax pups_error - inline macro body is not closed\n\n",boldOn,boldOff);
         (void)fflush(stdout);
      }

      (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
      return; 
   }     


   /*---------------------------------*/
   /* Extract macro body and store it */
   /*---------------------------------*/

   start     = ch_pos(request,'{') + 1;
   end       = ch_pos(request,'}');
   body_size = end - start;

   cnt = 0;
   for(i=start; i<end; ++i)
      last_request_line[cnt++] = request[i];

   last_request_line[cnt] = '\0';

   --start;
   ++end;
   for(i=end; i<pups_strlen(request_line); ++i)
      request[start++] = request[end++];
 
   ret = sscanf(request,"%s %s %s",tmp_str,tag_name,macro_f_name,tmp_str);


   /*---------------------------------------*/
   /* Less than two parameters - user pups_error */
   /*---------------------------------------*/

   if(ret < 2)
   {  (void)fprintf(stdout,"\nusage: mcat <tagname> [<mdf file>] [body]\n\n");
      (void)fflush(stdout);

      return;
   }


   /*--------------------------------------------------------------------*/
   /* Two parameter command - add macro to default macro definition file */
   /* for this application                                               */
   /*--------------------------------------------------------------------*/

   else if(ret == 2)
   {   char username[SSIZE] = "";


       /*--------------*/
       /* Get username */
       /*--------------*/

       if((char *)getenv("HOME") == (char *)NULL)
          (void)strlcpy(username,".",SSIZE);
       else
          (void)snprintf(username,SSIZE,"%s/.",(char *)getenv("HOME"));

       (void)snprintf(macro_f_name,SSIZE,"%s%s.mdf",username,appl_name);
       catenate_macro(tag_name,last_request_line,macro_f_name);
    }


    /*-------------------------------------------------------------*/
    /* Three parameter command - add macro definition to name file */
    /*-------------------------------------------------------------*/

    else if(ret == 3)
       catenate_macro(tag_name,last_request_line,macro_f_name);
}




/*------------------------*/
/* Load macro definitions */
/*------------------------*/

_PRIVATE void builtin_load_macro_definitions(const _BOOLEAN check_mode, const _BOOLEAN append, const char *request)

{

    /*-----------------------------------------------*/
    /* Have we defined a macro file, or are we going */
    /* to use the default file?                      */
    /*-----------------------------------------------*/

    if(sscanf(request,"%s %s",macro_f_name,macro_f_name) == 2)
       update_macro_stack(check_mode,append,macro_f_name);
    else
    {   char username[SSIZE] = "";


        /*--------------*/
        /* Get username */
        /*--------------*/

        if((char *)getenv("HOME") == (char *)NULL)
           (void)strlcpy(username,".",SSIZE);
        else
           (void)snprintf(username,SSIZE,"%s/.",(char *)getenv("HOME"));
        (void)snprintf(macro_f_name,SSIZE,"%s%s.mdf",username,appl_name);
        update_macro_stack(check_mode,append,macro_f_name);
     }
}




/*------------------------*/
/* Edit macro definitions */
/*------------------------*/

_PRIVATE void builtin_edit_macro_definitions(const _BOOLEAN load, const _BOOLEAN append, const char *request)

{

    /*-----------------------------------------------*/
    /* Have we defined a macro file, or are we going */
    /* to use the default file?                      */
    /*-----------------------------------------------*/

    if(sscanf(request,"%s %s",macro_f_name,macro_f_name) == 2)
       edit_macro_definitions(load,append,macro_f_name);
    else
    {  (void)strlcpy(macro_f_name,"default",SSIZE);
       edit_macro_definitions(load,append,macro_f_name);
    }
}




/*------------------------------------*/
/* Expand recursive macro definitions */
/*------------------------------------*/

_PRIVATE _BOOLEAN expand_macros_recursive(const char *request_line, char *expanded_request_line)

{   _BOOLEAN looper = TRUE;

    macros_expanded = FALSE;
    do {    if(expand_macros(request_line,expanded_request_line) == FALSE)
               return(FALSE);


            /*-------------------------------------------------------*/
            /* Lop off leading and trailing spaces before we attempt */
            /* any comparision                                       */
            /*-------------------------------------------------------*/

            (void)strail(expanded_request_line,' ');
            (void)strlcpy(expanded_request_line,strlead(expanded_request_line,' '),SSIZE);
            (void)strail(request_line,' ');
            (void)strlcpy(request_line,strlead(request_line,' '),SSIZE);

            if(strcmp(expanded_request_line,request_line) == 0)
               looper = FALSE;
            else
            {  (void)strlcpy(request_line,expanded_request_line,SSIZE);
               macros_expanded = TRUE;
            }

       } while(looper == TRUE);

    return(TRUE);
}




/*-----------------------------------*/
/* Move to request (in request line) */
/*-----------------------------------*/

_PRIVATE _BOOLEAN req_find(const uint32_t r_cnt, const char *label, const char *request_line)

{   size_t   s1,
             s2,
             cmp_size,
             cnt = 0;

    _BOOLEAN ret;
    char     next_item[SSIZE] = "";

    (void)m_strext(r_cnt,'\2',(char *)NULL,(char *)NULL);

    do {   ret = m_strext(r_cnt,';',next_item,request_line);

           s1 = pups_strlen(label);
           s2 = pups_strlen(next_item);

           (void)strlcpy(next_item,strlead(next_item,' '),SSIZE);
           if(pups_strlen(label) > pups_strlen(next_item))
              cmp_size = pups_strlen(label);
           else
              cmp_size = pups_strlen(next_item);

           if(ret == FALSE)
              return(FALSE);

           if(strncmp(next_item,label,cmp_size) == 0)
              return(TRUE);

           ++cnt;
       } while(TRUE);
}





/*--------------------------------------------------------------------------------------*/
/* Routine to repeat a command -- if the repeat count is negative, the loop is infinite */
/*--------------------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN psrp_repeat_command(const int32_t count, const char *request_line)


{   rpt_cnt               = 0; 
    in_repeat_command     = TRUE;


    /*-----------------------------------------------------*/
    /* Builtin commands cannot be supported in curses mode */
    /*-----------------------------------------------------*/

    if(server_pid == 0)
    {  (void)fprintf(stdout,"\n%sERROR%s cannot repeat builtin commands in curses mode\n\n",boldOn,boldOff);
       (void)fflush(stdout);

       return(FALSE);
    }


    #ifdef HAVE_CURSES
    if(rpt_curses_mode == TRUE)
    {  save_pel_appl_verbose = pel_appl_verbose;
       pel_appl_verbose      = FALSE;
    }

    if(rpt_curses_mode == TRUE && cinit_done == FALSE)
    {  cinit();
       cls();
    }
    #endif /* HAVE_CURSES */


    /*---------------------------------*/  
    /* Process an infinite repeat loop */
    /*---------------------------------*/

    if(count < 0)
    {

       while(TRUE)
       {

            #ifdef HAVE_CURSES
            if(rpt_curses_mode == TRUE)
               move(0,0);
            #endif /* HAVE_CURSES */

            rpthdr(request_line);
            if(psrp_process_command(request_line) == FALSE)
            {  
               #ifdef HAVE_CURSES
               if(rpt_curses_mode == TRUE)
               {  pel_appl_verbose  = save_pel_appl_verbose;
                  in_repeat_command = FALSE;
               }
               #endif /* HAVE_CURSES */

               return(FALSE);
            }

            #ifdef HAVE_CURSES
            if(rpt_curses_mode == TRUE)
               (void)refresh();
            #endif /* HAVE_CURSES */

            ++rpt_cnt;
            (void)pups_sleep(rpt_period);
        }
    }


    /*------------------------------*/
    /* Process a finite repeat loop */
    /*------------------------------*/

    else
    {  uint32_t i;

       for(i=0; i<count; ++i)
       {
           #ifdef HAVE_CURSES
           if(rpt_curses_mode == TRUE)
              move(0,0);
           #endif /* HAVE_CURSES */

           rpthdr(request_line);
           if(psrp_process_command(request_line) == FALSE)
           {

               #ifdef HAVE_CURSES
               if(rpt_curses_mode == TRUE)
               {  pel_appl_verbose  = save_pel_appl_verbose;
                  in_repeat_command = FALSE;
               }
               #endif /* HAVE_CURSES */

               return(FALSE);
           }

           #ifdef HAVE_CURSES
           if(rpt_curses_mode == TRUE)
              (void)refresh();
           #endif /* HAVE_CURSES */

           ++rpt_cnt;
           (void)pups_sleep(rpt_period);
       }
   }

   #ifdef HAVE_CURSES
   if(rpt_curses_mode == TRUE)
      cend();

   if(rpt_curses_mode == TRUE)
   {  in_repeat_command = FALSE;
      pel_appl_verbose  = save_pel_appl_verbose;
   }
   #endif /* HAVE_CURSES */

   return(TRUE);
}




/*----------------------*/
/* Set up curses system */
/*----------------------*/

_PRIVATE void cinit(void)

{

    #ifdef HAVE_CURSES
    /*---------------*/
    /* Set up curses */
    /*---------------*/

    if(isatty(1))
    {  (void)initscr();
       (void)echo();
       (void)keypad(stdscr,TRUE);
    }
    #else
    (void)strlcpy(psrp_c_code,"curserr",SSIZE);
    #endif /* HAVE_CURSES */

    cinit_done = TRUE;
}



/*--------------------*/
/* Stop curses system */
/*--------------------*/

_PRIVATE void cend(void)

{

    #ifdef HAVE_CURSES
    if(rpt_curses_mode == TRUE)
    {  endwin();
       cinit_done = FALSE;
    }
    #endif /* HAVE_CURSES */
}




/*----------------------------*/
/* Clear screen command (cls) */
/*----------------------------*/

_PRIVATE void cls(void)

{

    #ifdef HAVE_CURSES
    if(rpt_curses_mode == TRUE && cinit_done == TRUE)
    {  (void)clear();
       (void)refresh();
    }
    else
       (void)strlcpy(psrp_c_code,"curserr",SSIZE);
    #else
    (void)strlcpy(psrp_c_code,"curserr",SSIZE); 
    #endif /* HAVE_CURSES */
}




/*-----------------------------------*/
/* Print header for repeated command */
/*-----------------------------------*/

_PRIVATE void rpthdr(const char *request)

{
    #ifdef HAVE_CURSES
    if(rpt_curses_mode == TRUE)
    {  (void)printw("\n\n    Repeating request \"%s\" [rpt cnt is %d]\n",request,rpt_cnt);
       if(server_seg_cnt > 0)
          (void)printw("    Server is segmented [segment %d active]\n",server_seg_cnt);

       if(server_pid == 0)
          (void)printw("    Command is local [on PSRP client] (^C exits)\n\n");
       else
          (void)printw("    PSRP server is %s (%d@%s) (^C exits)\n\n",psrp_server,server_pid,psrp_host);

    }
    else
    #endif /* HAVE_CURSES */

    {   (void)fprintf(stdout,"\n\n    Repeating request \"%s\" [rpt cnt is %d]\n",request,rpt_cnt);
        if(server_seg_cnt > 0)
           (void)fprintf(stdout,"    Server is segmented [segment %d active]\n",server_seg_cnt);

        if(server_pid == 0)
           (void)fprintf(stdout,"    Command is local [on PSRP client] (^C exits)\n\n");
        else
           (void)fprintf(stdout,"    PSRP server is %s (%d@%s) (^C exits)\n\n",psrp_server,server_pid,psrp_host);
    }
}




/*----------------------------*/
/* Initialise do_repeat array */
/*----------------------------*/

_PRIVATE void initialise_repeaters(void)

{   uint32_t i;

    for(i=0; i<MAX_MACRO_DEPTH; ++i)
       do_repeat[i] = FALSE;
}




/*------------------------------------*/
/* Restore system (and signal) status */
/*------------------------------------*/

_PRIVATE void restore_system_status(const int32_t signum)

{    if(signum > 0 && signum < MAX_SIGS)
     {  sigset_t set;

        #ifdef PSRP_DEBUG
        (void)fprintf(stdout,"RESTORING %d\n",signum);
        (void)fflush(stdout);
        #endif /* PSRP_DEBUG */ 

        (void)sigemptyset(&set);
        (void)pups_sigprocmask(SIG_SETMASK,&set,(sigset_t *)NULL);

        (void)pups_vitrestart();
     }
     else
        pups_error("[restore_system_status] bad signal");


     /*---------------------*/
     /* Restore state flags */
     /*---------------------*/

     in_client_handler = FALSE;


     /*------------------------------------------------*/ 
     /* Not too sure why appl_pid needs to be reset    */
     /* could be a side effect of call to siglongjmp() */
     /* which got us here                              */
     /*------------------------------------------------*/ 

     appl_pid = getpid();
}




/*------------------------------------------------------------*/
/* Display the current PUPS connections on the client machine */
/*------------------------------------------------------------*/

_PRIVATE void psrp_show_channels(const int32_t flags, const char *directory)

{   uint32_t i,
             cnt = 0,
             entry_cnt,
             n_entries,
             schan;

    pid_t    pid;
    uid_t    uid;

    char    vector[SSIZE]      = "",
            strdum[SSIZE]      = "",
            application[SSIZE] = "",
            hostname[SSIZE]    = "",
            pathname[SSIZE]    = "",
            **entry_list       = (char **)NULL;

    struct  stat   buf;
    struct  passwd *pwent      = (struct passwd *)NULL;


    /*---------------------------------------------------*/
    /* Check that we are actually looking at a directory */
    /*---------------------------------------------------*/

    (void)stat(directory,&buf);
    if(!S_ISDIR(buf.st_mode))
    {  (void)fprintf(stdout,"%sERROR%s \"%s\" is not a directory\n",boldOn,boldOff,directory);
       (void)fflush(stdout);

       return;
    }

    entry_list = pups_get_directory_entries(directory,"psrp",&n_entries,&entry_cnt);
    for(i=0; i<n_entries; ++i)
    {  int32_t busy = 0;
       des_t   fdes = (-1);


       /*---------------------------*/
       /* Get the owner of resource */
       /*---------------------------*/

       (void)snprintf(pathname,SSIZE,"%s/%s",directory,entry_list[i]);
       (void)stat(pathname,&buf);
       pwent = pups_getpwuid(buf.st_uid);

       fdes = open(pathname,O_RDWR);
       busy = lockf(fdes,F_TEST,0);
       (void)close(fdes);

       mchrep(' ',"#",entry_list[i]);

       if(strin(entry_list[i],"psrp") == TRUE                                             &&
          (strin(entry_list[i],"fifo in") == TRUE || strin(entry_list[i],"vector") == TRUE))
       {  

          uint32_t j,
                   entries;

          char     procpidpath[SSIZE] = "",
                   binname[SSIZE]     = "";


          #ifdef PSRP_DEBUG
          (void)fprintf(stderr,"ENTRY LIST[%d]: \"%s\"\n",i,entry_list[i]);
          (void)fflush(stderr);
          #endif /* PSRP_DEBUG */

          entries = sscanf(entry_list[i],"%s%s%s%s%s%d%d%s%d",strdum,application,hostname,vector,strdum,&pid,&uid,strdum,&schan);

          #ifdef PSRP_DEBUG
          (void)fprintf(stderr,"ENTRIES: %d\n",entries);
          (void)fflush(stderr);
          #endif /* PSRP_DEBUG */

          if(entries == 7 || entries == 9)
          {  char   state[SSIZE] = "";


             /*--------------------------------*/
             /* test whether channel is active */
             /*--------------------------------*/

             errno     = 0;


             /*------------------------*/
             /* Non interactive states */
             /*------------------------*/

             if(strcmp(vector,"vector") == 0)
                (void)strlcpy(state,"vector",      SSIZE);
             else if(kill(pid,SIGALIVE) == (-1))
             {  if(errno == ESRCH)
                   (void)strlcpy(state,"dead",     SSIZE);
                else
                   (void)strlcpy(state,"not owner",SSIZE);
             }


             /*--------------------*/
             /* Interactive states */
             /*--------------------*/

             else
             {

                /*---------------------------------*/ 
                /* Channel is alive - is it busy ? */
                /*---------------------------------*/ 

                if(busy != 0)
                   (void)strlcpy(state,"busy",SSIZE);
                else
                   (void)strlcpy(state,"idle",SSIZE);
             }


             /*----------------*/
             /* Display filter */
             /*----------------*/
                                                                                      /*------------------------------*/
             if(!(flags & DEAD_SERVERS)       && strcmp(state,"dead")      == 0   ||  /* Exclude dead servers         */
                !(flags & VECTORED_SERVERS)   && strcmp(state,"vector")    == 0   ||  /* Exclude vectored servers     */
                !(flags & NOWNED_SERVERS)     && strcmp(state,"not owner") == 0   ||  /* Exclude servers we don't own */
                !(flags & BUSY_SERVERS)       && strcmp(state,"busy")      == 0   ||  /* Exlude busy servers          */
                !(flags & IDLE_SERVERS)       && strcmp(state,"idle")      == 0    )  /* Exclude idle servers         */
                                                                                      /*------------------------------*/

                continue;


             /*------------------------*/
             /* Display server details */
             /*------------------------*/

             else
             { 

                /*------------------------------------------------*/
                /* Display banner if this is first item displayed */
                /*------------------------------------------------*/

                if(cnt == 0)
                {  (void)strdate(date);
                   (void)fprintf(stdout,"\n%s: PSRP channels on %s (via %s)\n\n",date,appl_host,directory); 
                   (void)fprintf(stdout,"Application\t\towner\t\t\tstate\t\t\tpid\t\tbinname\n\n");
                   (void)fflush(stdout);
                } 

                (void)fprintf(stdout,"%-15s\t\t%-15s\t\t%-15s\t\t%d",application,pwent->pw_name,state,pid);

                if(strcmp(state,"vector") == 0)
                {  (void)strlcpy(binname,"****",SSIZE);
                   (void)fprintf(stdout,"\t\t%15s",binname);
                }
                else
                {

                   /*--------------------------------*/
                   /* Get name of binary (if we can) */
                   /*--------------------------------*/

                   for(j=0; j<SSIZE; ++j)
                      binname[j] = '\0';
 
                   (void)snprintf(procpidpath,SSIZE,"/proc/%d/exe",pid);
                   (void)readlink(procpidpath,binname,256);

                   for(j=SSIZE; j>=0; --j)
                   {  if(binname[j] == '/')
                         break;
                   }

                   (void)fprintf(stdout,"\t\t%-15s",&binname[j+1]);
                }


                /*--------------*/
                /* PSRP channel */
                /*--------------*/

                if(entries == 7)
                {

                   /*----------------------------------------*/
                   /* Are we connected to this PSRP server ? */
                   /*----------------------------------------*/

                   if(pid == server_pid)
                      (void)fprintf(stdout," [PSRP*]\n");
                   else
                      (void)fprintf(stdout," [PSRP]\n");
              
                }


                /*-------------*/
                /* SIC channel */
                /*-------------*/
            
                else
                {  

                   /*-------------------------------------------*/
                   /* Is this SIC channel associated with       */
                   /* the server we are currently connected to? */
                   /*-------------------------------------------*/
                
                   if(pid == server_pid)
                      (void)fprintf(stdout," [SIC*: %d]\n",schan);
                   else
                      (void)fprintf(stdout," [SIC: %d]\n",schan);
                }

                (void)fflush(stdout);
                ++cnt;
             }
          }
       }
    }

    if(cnt == 0)
    {  (void)fprintf(stdout,"%sERROR%s no PSRP channels (matching current display filter flags) mounted via %s\n",boldOn,boldOff,directory);
       (void)fflush(stdout);


       /*----------------------*/
       /* Clear the entry list */
       /*----------------------*/

       for(i=0; i<n_entries; ++i)
          (void)pups_free((void *)entry_list[i]);
       (void)pups_free((void *)entry_list);

       return;
    }


    (void)fprintf(stdout,"\n");
    (void)fflush (stdout);


    /*----------------------*/
    /* Clear the entry list */
    /*----------------------*/

    for(i=0; i<n_entries; ++i)
       (void)pups_free((void *)entry_list[i]);
    (void)pups_free((void *)entry_list);
}




/*--------------------------*/
/* Connect to remote client */
/*--------------------------*/

_PRIVATE _BOOLEAN builtin_connect_remote_client(const char *request)

{ 

    #ifndef SSH_SUPPORT
    (void)pups_set_errno(EINVAL);
    return(FALSE);
    #else

    char    cwd[SSIZE]               = "",
            strdum[SSIZE]            = "",
            ruser[SSIZE]             = "",
            rhost[SSIZE]             = "",
            psrp_parameters[SSIZE]   = "",
            ssh_command[SSIZE]       = "",
            ssh_remote_env[SSIZE]    = "";

    int32_t ret                      = 0,
            status                   = 0;


    if(request == (char *)NULL || sscanf(request,"%s %s",strdum,rhost) != 2)
    {  (void)fprintf(stdout,"%sERROR%s expecting remote host name\n",boldOn,boldOff);
       (void)fflush(stdout);

       (void)strlcpy(psrp_c_code,"psynerr",SSIZE);
       return(FALSE);
    }


    /*-----------------------------------*/
    /* Set up environment on remote host */
    /*-----------------------------------*/

    (void)psrp_exec_env(ssh_remote_env);
    (void)snprintf(psrp_parameters,SSIZE,"bash -c '%s; psrp -on %s -from %s %s %d'",ssh_remote_env,rhost,appl_host,appl_name,appl_pid);


    /*--------------------*/
    /* Child side of fork */
    /*--------------------*/

    if((child_pid = pups_fork(FALSE,FALSE)) == 0)
    {

       /*---------------------*/
       /* Terminal connection */
       /*---------------------*/

       if(isatty(0) == 1)
       {

           /*---------------------*/
           /* Overlay ssh command */
           /*---------------------*/

           if(ssh_compression == TRUE)
             (void)execlp("ssh","ssh","-q","-C","-t","-l",ssh_remote_uname,rhost,psrp_parameters,(char *)NULL);
          else
             (void)execlp("ssh","ssh","-q","-t","-l",     ssh_remote_uname,rhost,psrp_parameters,(char *)NULL);
       }


       /*-------------------------*/
       /* Non terminal connection */
       /*-------------------------*/

       else
       {

           /*---------------------*/
           /* Overlay ssh command */
           /*---------------------*/

           if(ssh_compression == TRUE)
             (void)execlp("ssh","ssh","-q","-C","-l",ssh_remote_uname,rhost,psrp_parameters,(char *)NULL);
          else
             (void)execlp("ssh","ssh","-q","-l",     ssh_remote_uname,rhost,psrp_parameters,(char *)NULL);
       }

       /*--------------------------------------------------------------*/
       /* We should not get here -- if we do an pups_error has occured */
       /*--------------------------------------------------------------*/

       pups_error("psrp: failed to build ssh channel to remote host");

       (void)strlcpy(psrp_c_code,"sexerr",SSIZE);
       exit(255);
    }


    /*---------------------*/
    /* Parent side of fork */
    /*---------------------*/

    else
    {

       /*---------------------------------------------*/
       /* Wait for child (checking that it exited OK) */
       /*---------------------------------------------*/

       (void)pupswaitpid(FALSE,child_pid,&status);
       ret = WEXITSTATUS(status);
    }

    if(ret != 0) 
       return(FALSE);
    
    return(TRUE);

    #endif /* SSH_SUPPORT */
}





/*---------------------------*/
/* Authenticate a user token */
/*---------------------------*/

_PRIVATE _BOOLEAN psrp_check_password(const char *username, const char *shell)

{   char password_banner[SSIZE]  = "",
         password[SSIZE]         = "",
         salt[2]                 = "",
         envstr[SSIZE]           = "",
         crypted_password[SSIZE] = "";

    uint32_t cnt                 = 0;
    struct   passwd *pwent       = (struct passwd *)NULL;

    if((pwent = pups_getpwnam(username)) == (struct passwd *)NULL)
    {  (void)fprintf(stdout,"%sERROR%s no such user \"%s\"\n",boldOn,boldOff,username);
       (void)fflush(stdout);

       return(FALSE);
    }

    do {     uint32_t verify_cnt      = 0;

             char     tmp_pw_1[SSIZE] = "",
                      tmp_pw_2[SSIZE] = "";

             if(interactive_mode == TRUE)
             {  do {   if(verify_cnt > 0)
                       {  (void)fprintf(stdout,"\nnot confirmed -- try again\n\n");
                          (void)fflush(stdout);
                       }

                       (void)snprintf(password_banner,SSIZE,"%s PSRP (protocol %5.2F) password: ",appl_host,PSRP_PROTOCOL_VERSION);
                       (void)strlcpy(tmp_pw_1,getpass(password_banner),SSIZE);

                       (void)snprintf(password_banner,SSIZE,"%s PSRP (protocol %5.2F) verify password:",appl_host,PSRP_PROTOCOL_VERSION);
                       (void)strlcpy(tmp_pw_2,getpass(password_banner),SSIZE);

                       ++verify_cnt;
                   } while(strcmp(tmp_pw_1,tmp_pw_2) != 0);

                (void)strlcpy(password,tmp_pw_1,SSIZE);
             }
             else
             {  (void)snprintf(password_banner,SSIZE,"%s PSRP (protocol %5.2F) password: ",appl_host,PSRP_PROTOCOL_VERSION);
                (void)strlcpy(appl_password,getpass(password_banner),SSIZE);
             }

            ++cnt;
            if(cnt >= max_trys)
               return(FALSE);
       } while(strlen(password) == 0);

    salt[0] = pwent->pw_passwd[0];
    salt[1] = pwent->pw_passwd[1];

    (void)strlcpy(crypted_password,(char *)crypt(password,salt),SSIZE);
    if(strcmp(pwent->pw_passwd,crypted_password) == 0)
    {  if(setreuid(pwent->pw_uid,(-1)) == (-1))
       {  (void)fprintf(stdout,"%sERROR%s permission denied (failed to set uid=%d for \"%s\")\n",boldOn,boldOff,pwent->pw_uid,username);
          (void)fflush(stdout);

          return(FALSE);
       }

       if(setregid(pwent->pw_gid,(-1)) == (-1))
       {  (void)fprintf(stdout,"%sERROR%s permission denied (failed to set gid=%d for \"%s\")\n",boldOn,boldOff,pwent->pw_gid,username);
          (void)fflush(stdout);

          return(FALSE);
       }


       /*------------------------------------------------------------------*/
       /* Are we a PSRP server access account (access to PSRP server only) */
       /*------------------------------------------------------------------*/

       (void)fprintf(stdout,"%sWARNING%s user \"%s\" is now owner of this PSRP session (uid=%d, gid=%d)\n",boldOn,boldOff,username,getuid(),getgid());
       (void)fflush(stdout);


       /*-----------------------*/
       /* Close any open server */
       /*-----------------------*/

       if(server_connected == TRUE)
       {  psrp_close_server(TRUE,TRUE);
          server_connected   = FALSE;
          psrp_log           = FALSE;
       }


       /*------------------------------*/
       /* Set up users preferred shell */
       /*------------------------------*/

       (void)snprintf(envstr,SSIZE,"SHELL=%s",pwent->pw_shell);
       (void)putenv(envstr);


       /*--------------------------*/
       /* Set users home directory */
       /*--------------------------*/

       (void)snprintf(envstr,SSIZE,"HOME=%s",pwent->pw_dir);
       (void)putenv(envstr); 


       /*--------------------------------------------*/
       /* Set application owner and (clear) password */
       /*--------------------------------------------*/

       (void)strlcpy(appl_owner,pwent->pw_name,SSIZE);
       (void)strlcpy(appl_password,password,SSIZE);

       (void)snprintf(envstr,SSIZE,"LOGNAME=%s",appl_owner);
       (void)putenv(envstr);

       (void)snprintf(envstr,SSIZE,"USER=%s",appl_owner);
       (void)putenv(envstr);


       #ifdef HAVE_LOGWTMP
       /*------------------*/
       /* Update wtmp file */
       /*------------------*/

       logwtmp(ttyname(0),appl_name,appl_host);
       #endif /* HAVE_LOGWTMP */

       return(TRUE);
    }

    (void)fprintf(stdout,"%sERROR%s permission denied (authentication failure for user \"%s\")\n",boldOn,boldOff,username);
    (void)fflush(stdout);

    return(FALSE);
}





/*------------------------*/
/* Change to a new userid */
/*------------------------*/

_PRIVATE _BOOLEAN builtin_change_user(const char *request)

{   char   strdum[SSIZE]   = "",
           username[SSIZE] = "";

    struct passwd *pwent   = (struct passwd *)NULL;

    if(geteuid() != 0)
    {  (void)fprintf(stdout,"%sERROR%s cannot change session owner unless client (psrp) has effective root UID\n",boldOn,boldOff);
       (void)fflush(stdout);

       return(FALSE);
    }

    if(sscanf(request,"%s%s",strdum,username) != 2)
    {  (void)fprintf(stdout,"\nusage: user <username>\n\n");
       (void)fflush(stdout);

       return(FALSE);
    }

    (void)fprintf(stdout,"\n%sWARNING%s %s PSRP (protocol %5.2F) changing PSRP session owner (from %s to %s)\n",
                                                                                                 boldOn,boldOff,
                                                            appl_host,PSRP_PROTOCOL_VERSION,appl_owner,username);
    (void)fflush(stdout);

    if(psrp_check_password(username,shell) == FALSE)
       return(FALSE);

    return(TRUE);
}




/*---------------------------------------------*/
/* Get current authentication token for client */
/*---------------------------------------------*/

_PRIVATE _BOOLEAN psrp_get_password(void)

{   uint32_t i;

    char     password_banner[SSIZE] = "",
             tmp_pw_1[SSIZE]        = "";

    for(i=0; i<max_trys; ++i)
    {  (void)snprintf(password_banner,SSIZE,"%s PSRP (protocol %5.2F) password: ",appl_host,PSRP_PROTOCOL_VERSION);
       (void)strlcpy(tmp_pw_1,getpass(password_banner),SSIZE);

       if(strcmp(tmp_pw_1,appl_password) == 0)
          return(TRUE);
    }

    (void)fprintf(stderr,"\n%sERROR%s too many trys\n\n",boldOn,boldOff);
    (void)fflush(stderr);

    return(FALSE);
}




/*-----------------------------------------------*/
/* Set current authentication token (for client) */
/*-----------------------------------------------*/

_PRIVATE _BOOLEAN psrp_set_password(const char *password)

{   char     password_banner[SSIZE]  = "",
             pwd_file_name[SSIZE]    = "";

    uint32_t cnt                     = 0;

    do {    uint32_t verify_cnt      = 0;

            char     tmp_pw_1[SSIZE] = "",
                     tmp_pw_2[SSIZE] = "";

            if(interactive_mode == TRUE)
            {  do {   if(verify_cnt > 0)
                      {  (void)fprintf(stdout,"\nnot confirmed -- try again\n\n");
                         (void)fflush(stdout);
                      }

                      (void)snprintf(password_banner,SSIZE,"%s PSRP (protocol %5.2F) new password: ",appl_host,PSRP_PROTOCOL_VERSION);
                      (void)strlcpy(tmp_pw_1,getpass(password_banner),SSIZE);

                      if(strcmp(tmp_pw_1,"") == 0)
                      {  (void)fprintf(stdout,"\naborted\n\n");
                         (void)fflush(stdout);

                         return(FALSE);
                      }

                      (void)snprintf(password_banner,SSIZE,"%s PSRP (protocol %5.2F) verify new password:",appl_host,PSRP_PROTOCOL_VERSION);
                      (void)strlcpy(tmp_pw_2,getpass(password_banner),SSIZE);

                      ++verify_cnt;
                  } while(strcmp(tmp_pw_1,tmp_pw_2) != 0);

               (void)strlcpy(password,tmp_pw_1,SSIZE);
            }
            else
            {  (void)snprintf(password_banner,SSIZE,"%s PSRP (protocol %5.2F) new password: ",appl_host,PSRP_PROTOCOL_VERSION);
               (void)strlcpy(password,getpass(password_banner),SSIZE);
            }

            ++cnt;

            if(cnt >= max_trys)
               return(FALSE);
       } while(strlen(password) == 0 && cnt < max_trys);


    /*--------------------------------------------------------*/
    /* Creat authentication file so other [PSRP] applications */
    /* can authenticate themselves without needing to ask the */
    /* client owner for a password                            */
    /*--------------------------------------------------------*/

    (void)snprintf(pwd_file_name,SSIZE,"/tmp/authtok.%d.tmp",appl_uid);
    if(access(pwd_file_name,F_OK | R_OK | W_OK) == (-1))
    {  FILE *pwd_stream        = (FILE *)NULL;
       char pwd_link_name[SSIZE] = "";

       (void)pups_creat(pwd_file_name,0600);

       (void)snprintf(pwd_link_name,SSIZE,"/tmp/authtok.%d.lnk.tmp",appl_pid);
       (void)link(pwd_file_name,pwd_link_name);

       pwd_stream = pups_fopen(pwd_file_name,"w",DEAD);

       (void)fprintf(pwd_stream,"%s\n",password);
       (void)fflush(pwd_stream);
       (void)pups_fclose(pwd_stream);
    }

    return(TRUE);
}




/*----------------------------------*/
/* Set current authentication token */
/*----------------------------------*/

_PRIVATE _BOOLEAN builtin_set_password(const char *request, const char *password)

{   _IMMORTAL uint32_t update_cnt = 0;

    char      strdum[SSIZE]       = "",
              tmpstr[SSIZE]       = "";


    /*---------------------------------*/
    /* Check request syntax is correct */
    /*---------------------------------*/

    if(sscanf(request,"%s%s",strdum,strdum) != 1)
    {  (void)fprintf(stdout,"\nusage: password\n\n");
       (void)fflush(stdout);

       return(FALSE);
    }

    if(strncmp(request,"secure",8) == 0)
    {  (void)strlcpy(tmpstr,"(server) ",SSIZE);
       secure_operation = TRUE;
    }

    (void)fprintf(stdout,"\n%s PSRP (protocol %5.2F) setting %sauthentication token for PSRP services\n",
                                                                  appl_host,PSRP_PROTOCOL_VERSION,tmpstr);
    (void)fflush(stdout);


    /*--------------------------------------------------------------*/ 
    /* If we have a current password we must authenticate ourselves */
    /* before we can change it.                                     */
    /*--------------------------------------------------------------*/ 

    if(strcmp(appl_password,"notset") != 0)
    {  if(psrp_get_password() == FALSE)
       {  (void)fprintf(stderr,"\nauthentication failure\n\n");
          (void)fflush(stderr);

          return(FALSE);
       } 
    }

    if(strncmp(request,"secure",8) == 0)
       (void)strlcpy(tmpstr,"(client side) ",SSIZE);

    if(psrp_set_password(password) == TRUE)
    {  if(update_cnt == 0)
          (void)fprintf(stdout,"\n%sauthentication token for PSRP services set\n\n",tmpstr);
       else
          (void)fprintf(stdout,"\n%sauthentication token for PSRP services updated\n\n",tmpstr);
       ++update_cnt;
    }
    else
      (void)fprintf(stdout,"\n%sauthentication token for remote PSRP not set\n\n",tmpstr); 

    (void)fflush(stdout);
    return(TRUE);
}




/*-----------------------*/
/* Remove lockpost files */
/*-----------------------*/

_PRIVATE void psrp_remove_lockpost_files(void)

{   uint32_t i;

    for(i=0; i<3; ++i)
    {

       /*-----------------------------------------------*/
       /* Remove this applictions' stdio lockpost files */
       /*-----------------------------------------------*/

       if(appl_verbose == TRUE)
       {  (void)fprintf(stdout,"    Removing lockpost file (%s%d) from patchboard directory %s\n",ftab[i].fname,i,appl_fifo_dir);
          (void)fflush(stdout);
       }

       (void)unlink(ftab[i].fname);
       (void)unlink(ftab[i].fshadow);
    }
}




/*-----------------*/
/* Handle SIGWINCH */
/*-----------------*/

_PRIVATE int32_t winch_handler(const int32_t signum)

{

    /*--------------------*/
    /* Raise resize event */
    /*--------------------*/

    window_resized = TRUE;


    /*--------------------------------------------------*/
    /* If we are relaying we need to propagate SIGWINCH */
    /* to slave (and its process group)                 */
    /*--------------------------------------------------*/

    if(slave_pid != (-1))
       (void)kill(slave_pid,SIGWINCH);


    #ifdef PSRP_DEBUG
    (void)fprintf(stdout,"*** window resized\n");
    (void)fflush(stdout);
    #endif /* PSRP_DEBUG */
}



/*---------------------------------------------*/
/* Migrate to host that server has migrated to */
/*---------------------------------------------*/

_PRIVATE int32_t psrp_migrate_client_to_server_host(void)

{   struct stat buf;

    char remote_server[SSIZE]       = "",
         remote_host[SSIZE]         = "",
         remote_host_port[SSIZE]    = "",
         remote_psrp_command[SSIZE] = "";


    #ifdef PSRP_DEBUG
    (void)fprintf(stderr,"MIGRATE\n");
    (void)fflush(stderr);
    #endif /* PSRP_DEBUG */


    /*-----------------------------------------------*/
    /* Do not follow server if psrp client recursive */
    /* as this means it is executing an aggregated   */
    /* migration request                             */
    /*-----------------------------------------------*/

    if(psrp_recursive == TRUE)
       return(PSRP_RECURSIVE);


    /*----------------------------------------------------------------*/
    /* Has target server migrated under us? If so there simply inform */
    /* user and abandon what we are doing                             */
    /*----------------------------------------------------------------*/

    #ifdef PSRP_DEBUG
    (void)fprintf(stderr,"READING TRAILFILE INTERACTIVE MODE: %d\n",interactive_mode);
    (void)fprintf(stderr,"TRAILFILE NAME: \"%s\"\n",trailfile_name);
    (void)fflush(stderr);
    #endif /* PSRP_DEBUG */

    if(interactive_mode == FALSE || psrp_have_trailfile() == FALSE /*|| strcmp(trailfile_name,"") == 0*/)
    {
       #ifdef PSRP_DEBUG
       (void)fprintf(stderr,"FAIL\n");
       (void)fprintf(stderr,"HAVE TRAILFILE: %d\n",psrp_have_trailfile());
       (void)fflush(stderr);
       #endif /* PSRP_DEBUG */

       return(PSRP_NOTRAIL);
    }

    #ifdef PSRP_DEBUG
    (void)fprintf(stderr,"READ TRAILFILE\n");
    (void)fflush(stderr);
    #endif /* PSRP_DEBUG */

    if(pel_appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stdout,"%sWARNING%s PSRP server \"%s\" has migrated to another host (reading trail data from \"%s\")\n",boldOn,boldOff,psrp_server,trailfile_name);
       (void)fflush(stdout);
    }


    /*---------------------------------------*/
    /* For now simply extract the trail data */
    /*---------------------------------------*/

    if(psrp_read_trailfile(trailfile_name,remote_server,remote_host,remote_host_port) == TRUE)
    {  if(pel_appl_verbose == TRUE)
       {  (void)fprintf(stdout,"%sWARNING%s PSRP server \"%s\" is now running as \"%s\" on host \"%s\" (port: %s) (client migrating to reconnect to it)\n",
                                                                                                                                            boldOn,boldOff,
                                                                                                    psrp_server,remote_server,remote_host,remote_host_port);
          (void)fflush(stdout);
       }


       /*-------------------------------------------------------*/
       /* Migrate onto the servers machine and reconnect to it  */
       /* note that we may arrive at the servers node before it */
       /* does so we may have to wait for it, Note if we are    */
       /* remote report basal PSRP client location as this is   */
       /* where the user is!                                    */
       /*-------------------------------------------------------*/


       /*----------------------------------*/
       /* PSRP client is does not have pen */
       /*----------------------------------*/

       if(strcmp(appl_name,appl_bin_name) == 0)
       {  


          if(is_remote == TRUE)
             (void)snprintf(remote_psrp_command,SSIZE,"%s -on %s -ssh_port %s -wait -sname %s -from %s %s %d",
                                                                                                appl_bin_name,
                                                                                                  remote_host,
                                                                                             remote_host_port,
                                                                                                remote_server,
                                                                                                from_hostname,
                                                                                             from_client_name,
                                                                                              from_client_pid);
          else
             (void)snprintf(remote_psrp_command,SSIZE,"%s -on %s -ssh_port %s -wait -sname %s -from %s %s %d",
                                                                                                appl_bin_name,
                                                                                                  remote_host,
                                                                                             remote_host_port,
                                                                                                remote_server,
                                                                                                    appl_host,
                                                                                                    appl_name,
                                                                                                     appl_pid);
       }


       /*---------------------*/
       /* PSRP client has pen */
       /*---------------------*/

       else
       {  if(is_remote == TRUE)
             (void)snprintf(remote_psrp_command,SSIZE,"%s -pen %s -on %s -ssh_port %s -wait -sname %s -from %s %s %d",
                                                                                                        appl_bin_name,
                                                                                                            appl_name, 
                                                                                                          remote_host,
                                                                                                     remote_host_port,
                                                                                                        remote_server,
                                                                                                        from_hostname,
                                                                                                            appl_name,
                                                                                                      from_client_pid);
          else
             (void)snprintf(remote_psrp_command,SSIZE,"%s -pen %s -on %s -ssh_port %s -wait -sname %s -from %s %s %d",
                                                                                                        appl_bin_name,
                                                                                                            appl_name,
                                                                                                          remote_host,
                                                                                                     remote_host_port,
                                                                                                        remote_server,
                                                                                                            appl_host,
                                                                                                            appl_name,
                                                                                                             appl_pid);
       }

       #ifdef PSRP_DEBUG
       (void)fprintf(stdout,"RCOMM: %s\n",remote_psrp_command);
       (void)fflush(stdout);
       #endif /* PSRP_DEBUG */

       if(WEXITSTATUS(system(remote_psrp_command)) >= 0)
       {

          /*---------------------------------------------*/
          /* Destroy stack of sockets to remote endpoint */
          /*---------------------------------------------*/

          if(is_remote == TRUE)
          {  if(strcmp(psrp_c_code,"ok") != 0)
                pups_exit(255);
             pups_exit(0);
          }
       }
       else
       {  (void)fprintf(stdout,"\n%sERROR%s failed to exec PSRP command (on migrated server \"%s\")\n",boldOn,boldOff,remote_server);
          (void)fflush(stdout);
       }
    }
    else
    {  (void)fprintf(stdout,"\n%sERROR%s failed to to find trail to migrated server \"%s\"\n",boldOn,boldOff,remote_server);
       (void)fflush(stdout);
    }


    /*--------------------------------------------------*/
    /* Remote PSRP connection has been closed -- resume */
    /* local PSRP client                                */
    /*--------------------------------------------------*/

    (void)strlcpy(psrp_server,"none",SSIZE);
    (void)psrp_close_server(TRUE,FALSE);

    server_connected   = FALSE;
    processing_command = FALSE;
    psrp_log           = FALSE;

    initialise_repeaters();
    (void)strlcpy(psrp_c_code,"scloscond",SSIZE);


    /*------------------------------------------*/
    /* Wait for further command input from user */
    /*------------------------------------------*/

    (void)pups_sigvector(SIGALRM, &command_loop_top);

    return(0);
}



/*-------------------------------*/
/* Set current working directory */
/*-------------------------------*/

_PRIVATE _BOOLEAN psrp_builtin_set_lcwd(const char *cwd)

{   char cwdpath[SSIZE] = "",
         relpath[SSIZE] = "";

    _BOOLEAN default_cwd = FALSE;

    if(cwd == (char *)NULL)
    {  (void)fprintf(stdout,"\ncurrent working directory is \"%s\"\n\n",appl_cwd,cwdpath);
       (void)fflush(stdout);
    }

    if(strcmp(cwd,"help") == 0 || strcmp(cwd,"usage") == 0)
    {  (void)fprintf(stdout,"\n\nusage  cwd [help | usage] [<current working directory:%s> | default]\n\n",appl_cwd);
       (void)fflush(stdout);

       return(FALSE);
    }

    if(strcmp(cwd,"default") == 0)
    {  (void)strlcpy(appl_cwd,(char *)getenv("PWD"),SSIZE);
       default_cwd = TRUE;
    }


    /*----------------------------------*/
    /* Check to see if path is absolute */
    /*----------------------------------*/

    else if(cwd[0] == '/')
       (void)strlcpy(cwdpath,cwd,SSIZE);
    else if(cwd[0] == '.')
    {  if(cwd[1] == '.')
       {  (void)fprintf(stdout,"%sERROR%s std_init: cannot have \"..\" in cwd path",boldOn,boldOff);
          (void)fflush(stdout);

          return(FALSE);
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
          (void)fprintf(stdout,"\nset current working directory to \"%s\" [default]\n\n",appl_cwd);
       else
          (void)fprintf(stdout,"\nset current working directory to \"%s\"\n\n",appl_cwd);
       (void)fflush(stdout);
    }
    else
    {  (void)fprintf(stdout,"\n%sWARNING%s current working directory is \"%s\" (could not change it to \"%s\")\n\n",boldOn,boldOff,appl_cwd,cwdpath);
       (void)fflush(stdout);
    }

    return(TRUE);
}



#ifdef SLAVED_COMMANDS
/*----------------------*/
/* Is command permitted */
/*----------------------*/

_PRIVATE _BOOLEAN command_permitted(const char *cmd)

{   uint32_t i;

    char     strdum[SSIZE]        = "",
             effective_cmd[SSIZE] = "";

    (void)sscanf(cmd,"%s%s",effective_cmd,strdum);
    for(i=0; i<clist_size; ++i)
    {  if(strcmp(clist[i],effective_cmd) == 0)
          return(TRUE);
    }

    return(FALSE);
}




/*------------------------------*/
/* Read restricted command list */
/*------------------------------*/

_PRIVATE  int32_t builtin_read_clist(const _BOOLEAN  interactive, const char *request)

{   FILE     *stream             = (FILE *)NULL,
             *stripped_stream    = (FILE *)NULL;

    uint32_t i;
    int32_t ret;

    char    strdum[SSIZE]       = "",
            tmp_f_name[SSIZE]   = "",
            clist_f_name[SSIZE] = "";

    if(interactive == TRUE)
       ret = sscanf(request,"%s%s",strdum,clist_f_name);
    else
       ret = sscanf(request,"%s",clist_f_name);

    if(ret > 1 && clist_alloc > 0)
    {  for(i=0; i<clist_alloc; ++i)
          (void)pups_free((char *)clist[i]);
       clist = (char **)pups_free((char *)clist);
       clist_size = clist_alloc = 0;
    }

    if(strcmp(clist_f_name,"none") == 0)
    {  restricted_clist = FALSE;

       (void)fprintf(stderr,"\n%sWARNING%s restricted command list revoked (any command can be executed by default shell)\n\n",boldOn,boldOff);
       (void)fflush(stderr);

       return(0);
    }
   
    if(interactive == TRUE && ret == 1)
    {  if(clist_size == 0)
       {  (void)fprintf(stderr,"\n%sERROR%s expecting restricted command list file name\n\n",boldOn,boldOff);
          (void)fflush(stderr);

          return(-1);
       }

       (void)fprintf(stderr,"\n%sRCL command list%s\n",boldOn,boldOff);
       (void)fprintf(stderr,"%s================%s\n\n",boldOn,boldOff);
       (void)fflush(stderr);

       for(i=0; i<clist_size; ++i)
       {  (void)fprintf(stderr,"%d: %32s\n",i,clist[i]);
          (void)fflush(stderr);
       }

       (void)fprintf(stderr,"\n\n");
       (void)fflush(stderr);

       return(-1);
    }

    if(access(clist_f_name,F_OK | R_OK | W_OK) == (-1))
    {  (void)fprintf(stderr,"\n%sERROR%s cannot find/access restricted command list file \"%s\"\n",boldOn,boldOff,clist_f_name);
       (void)fflush(stderr);

       return(-1);
    }


    /*------------------------------------------*/
    /* Strip any comment from command list file */
    /*------------------------------------------*/

    stream          = fopen(clist_f_name,"r");
    (void)snprintf(tmp_f_name,SSIZE,"%s.%d.%s.tmp",appl_name,appl_pid,appl_host);
    stripped_stream = pups_strp_commnts('#',stream, tmp_f_name);

    do {   if(clist_alloc == 0)
           {  clist_alloc = ALLOC_QUANTUM;
              clist       = (char **)pups_calloc(clist_alloc,sizeof(char **));
           }
           else if(clist_alloc == clist_size)
           {  clist_alloc += ALLOC_QUANTUM;
              clist = (char **)pups_realloc((void *)clist,clist_alloc*sizeof(char **));
           }


           /*-----------------------------*/
           /* Read next command from file */
           /*-----------------------------*/

           clist[clist_size] = (char *)pups_malloc(256*sizeof(char));
           (void)fscanf(stripped_stream,"%s",clist[clist_size]);
        
           ++clist_size;
       } while(feof(stripped_stream) == 0);

    --clist_size;
    (void)fclose(stripped_stream);
    (void)unlink(tmp_f_name);

    restricted_clist = TRUE;

    (void)fprintf(stderr,"\n%d commands read from rcl file \"%s\"\n\n",clist_size,clist_f_name);
    (void)fflush(stderr);

    return(0);
}
#endif /* SLAVED_COMMANDS */




/*------------------------------------------------------------*/
/* Display the current PUPS connections on the client machine */
/*------------------------------------------------------------*/

_PRIVATE void psrp_kill_servers(const char *directory, const char *part_name)

{   uint32_t i,
             entry_cnt,
             n_entries,
             cnt                = 0;

    pid_t    pid;

    char     strdum[SSIZE]      = "",
             application[SSIZE] = "",
             hostname[SSIZE]    = "",
             pathname[SSIZE]    = "",
             **entry_list       = (char **)NULL;

    struct stat   buf;
    struct passwd *pwent = (struct passwd *)NULL;


    /*---------------------------------------------------*/
    /* Check that we are actually looking at a directory */
    /*---------------------------------------------------*/

    (void)stat(directory,&buf);
    if(!S_ISDIR(buf.st_mode))
    {  (void)fprintf(stdout,"%sERROR%s \"%s\" is not a directory\n",boldOn,boldOff,directory);
       (void)fflush(stdout);

       return;
    }

    entry_list = pups_get_directory_entries(directory,"psrp",&n_entries,&entry_cnt);
    for(i=0; i<n_entries; ++i)
    {  

       /*---------------------------*/
       /* Get the owner of resource */
       /*---------------------------*/

       (void)snprintf(pathname,SSIZE,"%s/%s",directory,entry_list[i]);
       (void)stat(pathname,&buf);
       pwent = pups_getpwuid(buf.st_uid);

       (void)mchrep(' ',"#",entry_list[i]);

       if(strin(entry_list[i],"psrp") == TRUE && strin(entry_list[i],"fifo in") == TRUE)
       {

          uint32_t j;

          char     procpidpath[SSIZE] = "",
                   binname[SSIZE]     = "";

          if(sscanf(entry_list[i],"%s%s%s%s%s%d",strdum,application,hostname,strdum,strdum,&pid) == 6)
          {  char   state[SSIZE] = "";


             /*--------------------------------*/
             /* Get name of binary (if we can) */
             /*--------------------------------*/

             for(j=0; j<SSIZE; ++j)
                binname[j] = '\0';

             (void)snprintf(procpidpath,SSIZE,"/proc/%d/exe",pid);
             (void)readlink(procpidpath,binname,256);

             for(j=SSIZE; j>=0; --j)
             {  if(binname[j] == '/')
                   goto name_extracted;
             }

name_extracted:

             (void)fprintf(stdout,"%16s",&binname[j+1]);


             /*-----------------------------------------------------------*/
             /* Check that PSRP servers associated with this channel list */
             /* are local.                                                */
             /*-----------------------------------------------------------*/

             if(strcmp(hostname,appl_host) != 0)
             {  (void)fprintf(stdout,"\n%sWARNING%s PSRP channels on \"%s\" are not local to %s\n\n",boldOn,boldOff,directory,appl_host);
                (void)fflush(psrp_out);  


                /*----------------------*/
                /* Clear the entry list */
                /*----------------------*/

                for(i=0; i<n_entries; ++i)
                    (void)pups_free((void *)entry_list[i]);
                (void)pups_free((void *)entry_list);

                return;
             }

             if(strcmp(part_name,"all") == 0)
                (void)kill(pid,SIGTERM);
             else if(strin(application,part_name) == TRUE)
                (void)kill(pid,SIGTERM);

             if(cnt == 0)
                (void)fprintf(stdout,"\n");


             (void)fprintf(stdout,"PSRP server \"%s%s%s\" (bin name \"%s\") terminated\n",boldOn,application,boldOff,binname);
             (void)fflush(stdout);

             if(exit_on_terminate == TRUE)
                pups_exit(255);

             ++cnt;
          }
       }
    }

    if(cnt == 0)
    {  

       /*----------------------*/
       /* Clear the entry list */
       /*----------------------*/

       for(i=0; i<n_entries; ++i)
          (void)pups_free((void *)entry_list[i]);
       (void)pups_free((void *)entry_list);

       return;
    }

    (void)fprintf(stdout,"\n");
    (void)fflush(stdout);


    /*----------------------*/
    /* Clear the entry list */
    /*----------------------*/

    for(i=0; i<n_entries; ++i)
       (void)pups_free((void *)entry_list[i]);
    (void)pups_free((void *)entry_list);
}




/*-----------------------------------------------------------*/
/* Virtual maggot remove stale channels, lockposts and locks */
/*-----------------------------------------------------------*/

_PRIVATE  int32_t psrp_virtual_maggot(const char *dirname)


{   int32_t   ret              = 0;

    uint32_t  cnt              = 0,
              deleted          = 0;

    uid_t     owner            = 0;
    pid_t     lock_pid         = 0;

    DIR       *dirp            = (DIR *)NULL;

    char      tmpstr[SSIZE]    = "",
              next_item[SSIZE] = "";

    struct dirent *next_entry = (struct dirent *)NULL;

    if((dirp = opendir(dirname)) == (DIR *)NULL)
       return(-1);

    do {    next_entry = readdir(dirp);

            (void)strext(' ',(char *)NULL,(char *)NULL);


            if(next_entry != (struct dirent *)NULL           &&
               (strncmp(next_entry->d_name,"psrp",4) == 0    ||
                strncmp(next_entry->d_name,"pups",4) == 0    ||
                strncmp(next_entry->d_name, "lid",3) == 0    ||
                strin(next_entry->d_name,".tmp")     == TRUE ))
            {  struct stat buf;
               char   item_path[SSIZE] = "";

               (void)strlcpy(tmpstr,next_entry->d_name,SSIZE); 
               (void)mchrep(' ',"#:",tmpstr); 


               /*----------------------*/
               /* Do we own this file? */
               /*----------------------*/

               (void)snprintf(item_path,SSIZE,"%s/%s",dirname,next_entry->d_name);
               (void)lstat(item_path,&buf);

               if(getuid() == 0 || buf.st_uid == getuid())
               {

		  /*--------------------------------*/
		  /* Parse psrp object name for PID */
		  /*--------------------------------*/

                  cnt = 0;
                  do {    ret = strext(' ',next_item,tmpstr);


                          /*-----*/
                          /* PID */
                          /*-----*/

                          if(cnt == 0)
                          {  if(sscanf(next_item,"%d",&lock_pid) == 1)
		                ++cnt;
                          }


                          /*-------*/
                          /* Owner */
                          /*-------*/

                          else if(cnt == 1)
                          {  if(sscanf(next_item,"%d",&owner) == 1)
		                ++cnt;
                          }


			  /*----------------------------*/
			  /* Have process PID and owner */
			  /*----------------------------*/

			  if(cnt == 2)
                          {
                             #ifdef PSRP_DEBUG 
                             (void)fprintf(stderr,"OWNER %d  LOCK PID %d: ITEM \"%s\"\n",owner,lock_pid,item_path);
                             (void)fflush(stderr);
                             #endif /* PSRP_DEBUG */


                             /*----------------*/
                             /* Is owner live? */
                             /*----------------*/

                             if(kill(lock_pid,SIGALIVE) == (-1))
                             {  

                                ++deleted;
                                (void)unlink(item_path);

                                #ifdef PSRP_DEBUG 
                                (void)fprintf(stderr,"OWNER %d  LOCK PID %d: ITEM REMOVED \"%s\"\n",owner,lock_pid,item_path);
                                (void)fflush(stderr);
                                #endif /* PSRP_DEBUG */
                             }

                             break;
                          }

                     } while(ret != END_STRING);
               }
            }
        } while(next_entry != (struct dirent *)NULL);


    /*-------------------------------------*/
    /* Repair potential damage to /dev/tty */
    /*-------------------------------------*/

    if (getuid() == 0)
       (void)system("mktty");

    return(deleted);
}




/*----------------------------------------------------*/
/* Check for existence of trailfile and  get its name */
/*----------------------------------------------------*/

_PRIVATE _BOOLEAN psrp_have_trailfile(void)

{  
    (void)snprintf(trailfile_name,SSIZE,"%s.trail",channel_name_in);

    #ifdef PSRP_DEBUG
    (void)fprintf(stderr,"TRAILFILE: %s\n",trailfile_name);
    (void)fflush(stderr);
    #endif /* PSRP_DEBUG */

    if(access(trailfile_name,F_OK | R_OK) == (-1))
    {
       #ifdef PSRP_DEBUG
       (void)fprintf(stderr,"NO TRAILFILE\n");
       (void)fflush(stderr);
       #endif /* PSRP_DEBUG */

       return(FALSE);
    }

    #ifdef PSRP_DEBUG
    (void)fprintf(stderr,"HAVE TRAILFILE\n");
    (void)fflush(stderr);
    #endif /* PSRP_DEBUG */

    return(TRUE);
}
