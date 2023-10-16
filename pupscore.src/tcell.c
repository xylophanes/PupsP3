/*-------------------------------------------------------------------------------------------------------
    Purpose: Tcell process -- attacks and destroys any (parent) process which does not hold the
             correct licence. 

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.00
    Dated:   4th January 2022
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
#include <hash.h>
#include <stdlib.h>


/*---------------------*/
/* Application version */
/*---------------------*/

#define TCELL_VERSION    "2.00"


/*------------------------------------------------------------------------------------------------------
    Get application information for slot manager ...
------------------------------------------------------------------------------------------------------*/
/*---------------------------*/ 
/* Slot information function */
/*---------------------------*/ 

_PRIVATE void tcell_slot(int level)
{   (void)fprintf(stderr,"int dynamic app tcell %s: [ANSI C, PUPS MTD D]\n",TCELL_VERSION);
 
    if(level > 1)
    {  (void)fprintf(stderr,"(C) 2003-2022 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. ONeill\n");
       (void)fprintf(stderr,"Unlicensed process destructor (PUPS format) (built %s %s)\n\n",__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");

    (void)fflush(stderr);
}



/*----------------------------*/ 
/* Application usage function */
/*----------------------------*/ 

_PRIVATE void tcell_usage(void)

{   (void)fprintf(stderr,"tcell [>& <ASCII log file>]\n\n");
    (void)fprintf(stderr,"[>& <ASCII log file>]\n\n");

    (void)fprintf(stderr,"Signals\n\n");
    (void)fprintf(stderr,"SIGINIT SIGCHAN SIGPSRP: Process status [PSRP] request (protocol %4.2F)\n",PSRP_PROTOCOL_VERSION);
    (void)fprintf(stderr,"SICLIENT: tell client server is about to segment\n");

    #ifdef CRUI_SUPPORT
    (void)fprintf(stderr,"SIGCHECK SIGRESTART:      checkpoint and restart signals\n");
    #endif /* CRIU_SUPPORT */

    (void)fprintf(stderr,"SIGALIVE: check for existence of client on signal dispatch host\n");
    (void)fflush(stderr);
}


#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT)() __attribute__ ((aligned(16))) = tcell_slot;
_EXTERN void (* USE )() __attribute__ ((aligned(16))) = tcell_usage;
#endif /* SLOT */




/*-------------------------------------------------------------------------------------------------------
   Application build date ...
-------------------------------------------------------------------------------------------------------*/

_EXTERN char appl_build_time[SSIZE] = __TIME__;
_EXTERN char appl_build_date[SSIZE] = __DATE__;




/*-------------------------------------------------------------------------------------------------------
    Software I.D. tag (used if CKPT support enabled to discard stale dynamic
    checkpoint files) ...
-------------------------------------------------------------------------------------------------------*/

#define VTAG  4939
extern int appl_vtag = VTAG;




/*--------------------------------------------------------------------------------------------------------
    Exit function which terminates parent process (which has violated licence) ...
--------------------------------------------------------------------------------------------------------*/

_PRIVATE void violation(void)

{   (void)fprintf(stdout,"ERROR\n");
    (void)fflush(stdout);

    exit(255);
}



/*--------------------------------------------------------------------------------------------------------
    Main - decode command tail then interpolate using parameters supplied by user ...
--------------------------------------------------------------------------------------------------------*/

_PUBLIC int pups_main(int argc, char *argv[])

{   

    /*----------------------------------------------*/
    /* Register exit function (to terminate parent) */
    /*----------------------------------------------*/

    (void)pups_register_exit_f("violaton",(void *)&violation,(char *)NULL);


    /*---------------------------------------------------------------*/
    /* Do not allow PSRP clients to connect until we are initialised */
    /*---------------------------------------------------------------*/

    (void)psrp_ignore_requests();


    /*------------------------------------------*/
    /* Get standard items form the command tail */
    /*------------------------------------------*/
    /*------------------------------------------------------*/
    /* Error code 9999 will be emitted if securicor() fails */
    /* and this process (and its parent) will be terminated */
    /*------------------------------------------------------*/

    pups_std_init(TRUE,
                  &argc,
                  TCELL_VERSION,
                  "M.A. O'Neill",
                  "tcell",
                  "2022",
                  argv);


    /*--------------------------------------------*/
    /* We have license -- do not terminate parent */
    /*--------------------------------------------*/

    (void)pups_deregister_exit_f((void *)&violation);
    pups_exit(0);
}
