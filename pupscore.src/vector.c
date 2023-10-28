/*-------------------------------------------------------------------------------------------------------
    Purpose: Vector (to given PSRP server). 

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.00 
    Dated:   4th January 2023
    E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------------------------------------------*/


#include <me.h>
#include <utils.h>
#include <netlib.h>
#include <psrp.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <hipl_hdr.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pheap.h>
#include <vstamp.h>
#include <casino.h>
#include <stdlib.h>


/*********************/
/* Version of vector */
/*********************/

#define VECTOR_VERSION    "2.00"


/*------------------------------------------------------------------------------------------------------
    Get application information for slot manager ...
------------------------------------------------------------------------------------------------------*/
/*---------------------------*/ 
/* Slot information function */
/*---------------------------*/ 

_PRIVATE void vector_slot(int level)
{   (void)fprintf(stderr,"int app vector %s: [ANSI C]\n",VECTOR_VERSION);
 
    if(level > 1)
    {  (void)fprintf(stderr,"(C) 2002-2023 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. ONeill\n");
       (void)fprintf(stderr,"PSRP remote server vector (built %s %s)\n\n",__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");

    (void)fflush(stderr);
}
 
 
 
 
/*----------------------------*/ 
/* Application usage function */
/*----------------------------*/ 

_PRIVATE void vector_usage(void)

{   (void)fprintf(stderr,"[-on <host>]\n");
    (void)fprintf(stderr,"[-server <PSRP server>]\n\n");
    (void)fprintf(stderr,"[>& <ASCII log file>]\n\n");

    (void)fprintf(stderr,"Signals\n\n");
    (void)fprintf(stderr,"SIGINIT SIGCHAN SIGPSRP: Process status [PSRP] request (protocol %4.2F)\n",PSRP_PROTOCOL_VERSION);
    (void)fprintf(stderr,"SIGCLIENT: tell client server is about to segment\n");

    #ifdef CRUI_SUPPORT
    (void)fprintf(stderr,"SIGCHECK SIGRESTART:      checkpoint and restart signals\n");
    #endif /* CRIU_SUPPORT */

    (void)fprintf(stderr,"SIGALIVE: check for existence of client on signal dispatch host\n");
    (void)fflush(stderr);
}


#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT)() __attribute__ ((aligned(16))) = vector_slot;
_EXTERN void (* USE )() __attribute__ ((aligned(16))) = vector_usage;
#endif /* SLOT */


/*-------------------------------------------------------------------------------------------------------
    Application build date ...
-------------------------------------------------------------------------------------------------------*/

_EXTERN char appl_build_time[SSIZE] = __TIME__;
_EXTERN char appl_build_date[SSIZE] = __DATE__;




/*-------------------------------------------------------------------------------------------------------
    Functions which are private to this application ...
-------------------------------------------------------------------------------------------------------*/

// Initialise link (to remote host) 
_PROTOTYPE _PRIVATE int psrp_link(int, char *[]);




/*-------------------------------------------------------------------------------------------------------
    Variables which are private to this module ...
-------------------------------------------------------------------------------------------------------*/

_PRIVATE char r_host[SSIZE]   = "",
              r_server[SSIZE] = "";


/*-------------------------------------------------------------------------------------------------------
    Software I.D. tag (used if CKPT support enabled to discard stale dynamic
    checkpoint files) ...
-------------------------------------------------------------------------------------------------------*/

#define VTAG  2154

extern int appl_vtag = VTAG;




/*--------------------------------------------------------------------------------------------------------
    Main - decode command tail then interpolate using parameters supplied by user ...
--------------------------------------------------------------------------------------------------------*/

_PUBLIC int pups_main(int argc, char *argv[])

{   

    /*---------------------------------------------------------------*/
    /* Do not allow PSRP clients to connect until we are initialised */
    /*---------------------------------------------------------------*/

    psrp_ignore_requests();


    /*------------------------------------------*/
    /* Get standard items form the command tail */
    /*------------------------------------------*/

    pups_std_init(TRUE,
                  &argc,
                  VECTOR_VERSION,
                  "M.A. O'Neill",
                  "(PSRP) vector",
                  "2023",
                  argv);


    /*-------------------------------------*/
    /* Get host running remote PSRP server */
    /*-------------------------------------*/

    if((ptr = pups_locate(&init,"on",&argc,args,0)) != NOT_FOUND)
    {   if(strccpy(r_host,pups_str_dec(&ptr,&argc,args)) == (char *)NULL)
           pups_error("vector: expecting remote host name");

        if(appl_verbose == TRUE)
        {  (void)strdate(date);
           (void)fprintf(stderr,"%s %s (%d@%s:%s): remote host is \"%s\"\n",
                   date,appl_name,appl_pid,appl_host,appl_owner,r_host);
           (void)fflush(stderr);
        }
    }
    else
       pups_error("vector: remote host must be specified");


    /*--------------------------------*/ 
    /* Get name of remote PSRP server */
    /*--------------------------------*/ 

    if((ptr = pups_locate(&init,"server",&argc,args,0)) != NOT_FOUND)
    {   if(strccpy(r_server,pups_str_dec(&ptr,&argc,args)) == (char *)NULL)
           pups_error("vector: expecting remote PSRP server name");

        if(appl_verbose == TRUE)
        {  (void)strdate(date);
           (void)fprintf(stderr,"%s %s (%d@%s:%s): remote PSRP server is \"%s\"\n",
                             date,appl_name,appl_pid,appl_host,appl_owner,r_server);
           (void)fflush(stderr);
        }
    }
    else
       pups_error("vector: remote PSRP server  must be specified");


    /*---------------------------------------*/
    /* Complain about any unparsed arguments */
    /*---------------------------------------*/

    pups_t_arg_errs(argd,args);


    /*-------------------------------------------*/
    /* Initialise PSRP function dispatch handler */ 
    /*-------------------------------------------*/

    psrp_init(PSRP_DYNAMIC_FUNCTION | PSRP_STATIC_DATABAG | PSRP_HOMEOSTATIC_STREAMS,NULL);


    /*-----------------------------------------------------------*/
    /* We must define static bindings BEFORE loading the default */
    /* dispatch table. In the case of static bindings, the only  */
    /* effect of loading a saved dispatch table is to (possibly) */
    /* add object aliases                                        */
    /*-----------------------------------------------------------*/

    (void)psrp_attach_static_function(stderr,"link",(void *)&psrp_link);
    (void)psrp_load_default_dispatch_table(stderr);


    /*----------------------------------------------------------*/
    /* Tell PSRP clients we are ready to service their requests */
    /*----------------------------------------------------------*/

    psrp_accept_requests();

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): starting relay to %s@%s\n",
                     date,appl_name,appl_pid,appl_host,appl_owner,r_server,r_host);
       (void)fflush(stderr);
    }

    while(TRUE)
         pups_usleep(100);


    /*--------------------------------------------------------------------------*/
    /* Exit from PUPS/PSRP application cleaning up any mess it may have created */
    /*--------------------------------------------------------------------------*/

    pups_exit(0);
}




/*-------------------------------------------------------------------------------------------------------------
    Initialise link (to remote host) ...
-------------------------------------------------------------------------------------------------------------*/

_PRIVATE int psrp_link(int argc, char *argv[])

{   int  status;
    char psrp_cmd[SSIZE] = "";

    (void)snprintf(psrp_cmd,SSIZE,"exec psrp -verbose -sname %s -on %s",r_server,r_host,channel_name_in,channel_name_out);

    #ifdef DEBUG
    (void)fprintf(stderr,"CMD: %s\n",psrp_cmd);
    (void)fflush(stderr);
    #endif /* DEBUG */


    /*---------------------------------------*/
    /* Run command examining its exit status */
    /*---------------------------------------*/

    status = system(psrp_cmd);
    if( WEXITSTATUS(status) == (-1))
      pups_error("remote host vector failed");

    return(PSRP_OK);
}
