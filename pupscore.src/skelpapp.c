/*--------------------------------------------------------------------------------
    Purpose: @PURPOSE 

    Authors: @AUTHOR 
             @INSTITUTION

    Dated:   @DATE 
    Version: @VERSION 
    E-mail:  @EMAIL 
--------------------------------------------------------------------------------*/


#include <me.h>
#include <utils.h>
#include <netlib.h>
#include <psrp.h>
#include <hipl_hdr.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


/*------------------------*/
/* Version of application */
/*------------------------*/

#define VERSION "@VERSION"


#ifdef PERSISTENT_HEAP_SUPPORT
#include <pheap.h>
#endif /* PERSISTENT_HEAP_SUPPORT */

#ifdef BUBBLE_MEMORY_SUPPORT
#include <bubble.h>
#endif /* BUBBLE_MEMORY_SUPPORT */




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




/*----------------------------------------------*/
/* Get application information for slot manager */
/*----------------------------------------------*/
/*---------------------------*/ 
/* Slot information function */
/*---------------------------*/ 

_PRIVATE void @APPNAME_slot(int level)
{   (void)fprintf(stderr,"int @APPNAME %s: [ANSI C]\n",VERSION);
 
    if(level > 1)
    {  (void)fprintf(stderr,"(C) @DATE @INSTITUTION\n");
       (void)fprintf(stderr,"Author: @AUTHOR\n");
       (void)fprintf(stderr,"@APPDES (PUPS format)\n\n");
    }
    else
       (void)fprintf(stderr,"\n");

    (void)fflush(stderr);
}
 
 
 
 

/*----------------------------*/ 
/* Application usage function */
/*----------------------------*/ 

_PRIVATE void @APPNAME_usage(void)

{

    /*------------------------------------------------------------*/
    /* Put your usage information here - see for example embryo.c */
    /* to look at the format for this                             */
    /*------------------------------------------------------------*/


    /*------------------------------------------------------*/ 
    /* Standard blurb about the signals required to support */
    /* CKPT and PSRP protocols                              */ 
    /*------------------------------------------------------*/ 

    (void)fprintf(stderr,"\n\nSignals\n\n");
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
_EXTERN void (* SLOT)() __attribute__ ((aligned(16))) = @APPNAME_slot;
_EXTERN void (* USE )() __attribute__ ((aligned(16))) = @APPNAME_usage;
#endif /* SLOT */


/*------------------------*/
/* Application build date */
/*------------------------*/

_EXTERN char appl_build_time[SSIZE] = __TIME__;
_EXTERN char appl_build_date[SSIZE] = __DATE__;




/*-----------------------------------------------------------------*/
/* Public variables and function pointers used by this application */
/*-----------------------------------------------------------------*/




/*-------------------------------------------------*/
/* Functions which are private to this application */
/*-------------------------------------------------*/




/*--------------------------------------------*/
/* Variables which are private to this module */
/*--------------------------------------------*/




/*--------------------------------------------------*/
/* Software I.D. tag (used to discard stale dynamic */
/* checkpoint files and pesistent heaps)            */
/*--------------------------------------------------*/

#define VTAG     1
extern int appl_vtag = VTAG;




/*-------------------------------------------------------------------------------*/
/* Main - decode command tail then interpolate using parameters supplied by user */
/*-------------------------------------------------------------------------------*/

_PUBLIC int pups_main(int argc, char *argv[])

{

    #ifdef PSRP_ENABLED
    /*------------------------------------------------------------------------------*/
    /* Ignore any clients who may be trying to attach to us until we are intialised */
    /*------------------------------------------------------------------------------*/

    psrp_ignore_requests();
    #endif /* PSRP_ENABLED */




   /*------------------------------------------*/
   /* Get standard items from the command tail */
   /*------------------------------------------*/

    pups_std_init(TRUE,
                  &argc,
                  VERSION, 
                  "@AUTHOR",
                  "@APPNAME",
                  "@DATE",
                  argv);


    /*-----------------------------------------------*/
    /* Application specific arguments should go here */
    /*-----------------------------------------------*/


    /*---------------------------------------*/
    /* Complain about any unparsed arguments */
    /*---------------------------------------*/

    pups_t_arg_errs(argd,args);


    #ifdef PSRP_ENABLED
    /*---------------------------------------------------------*/
    /* Initialise the PSRP system here using psrp_init()       */
    /* Note any PSRP dynamic load option you do not want       */
    /* is simply excluded from the first set of OR'd args      */
    /* If you don't want homeostasis protection for the        */
    /* server messaging channels omit PSRP_HOMEOSTATIC_STREAMS */
    /*---------------------------------------------------------*/

    psrp_init(PSRP_STATIC_FUNCTION | PSRP_STATIC_DATABAG | PSRP_HOMEOSTATIC_STREAMS,NULL);


    /*-------------------------------------------------------------*/
    /* Load default dispatch table for this applications (loads    */
    /* dynamic objects which have been previously attached to this */
    /* application and remembers any aliases we may have had for   */
    /* dispatch table objects                                      */
    /*-------------------------------------------------------------*/

    (void)psrp_load_default_dispatch_table();


    /*--------------------------------------------------------------------------*/
    /* Once we have initialised the PSRP system, attached any dynamic objects   */
    /* and loaded any alias tables lets tell the world we are open for business */
    /*--------------------------------------------------------------------------*/

    psrp_accept_requests();
    #endif /* PSRP_ENABLED */


    /*----------------------------------------------------------------------------------------*/
    /* Any payload functions which attach or create objects like files and persistent heaps   */
    /* should register an exit function here, so that any temporary objects are destroyed     */
    /* by pups_exit()                                                                         */
    /*----------------------------------------------------------------------------------------*/


    /*----------------------------------------------------------------------------------*/
    /* Static payload of application here  - dynamic payloads are loaded using the PSRP */
    /* bind functions ...                                                               */
    /*----------------------------------------------------------------------------------*/


    /*-----------------------------------------------*/
    /* Exit nicely clearing up any mess we have made */
    /*-----------------------------------------------*/

    pups_exit(0);
}
