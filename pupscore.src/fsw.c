/*------------------------------------------------------------------------------
    Purpose: Check status of file system before trying to write data to
             it.

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

    Version: 2.00 
    Dated:   30th August 2019 
    E-mail:  mao@tumblingdice.co.uk 
------------------------------------------------------------------------------*/

#include <me.h>
#include <utils.h>
#include <signal.h>
#include <string.h>
#include <psrp.h>
#include <unistd.h>
#include <vstamp.h>


/*----------------*/
/* Version of fsw */
/*----------------*/

#define FSW_VERSION    "2.00"

#ifdef BUBBLE_MEMORY_SUPPORT
#include <bubble.h>
#endif /* BUBBLE_MEMORY_SUPPORT */


/*----------------------------------------------------------------*/
/* Get signal mapping appropriate to OS and hardware architecture */
/*----------------------------------------------------------------*/

#define __DEFINE__
#if defined(I386) || defined(X86_64)
#include <sig.linux.x86.h>
#endif /* I386  || X86_64 */

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




/*------------------------------------------------------------------------------
    Version ID (used by CKPT enabled binaries to discard stale checkpoint
    files) ...
------------------------------------------------------------------------------*/

#include <vstamp.h>


/*------------------------------------------------------------------------------
    Get application information for slot manager ...
------------------------------------------------------------------------------*/
/*---------------------------*/
/* Slot information function */
/*---------------------------*/

_PRIVATE void fsw_slot(int level)
{   (void)fprintf(stderr,"int app fsw %s: [ANSI C]\n",FSW_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(C) 2005-2019 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"File system status filter (built %s %s)\n\n",__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");
    (void)fflush(stderr);  
}
					 



/*----------------------------*/
/* Application usage function */
/*----------------------------*/

_PRIVATE void fsw_usage(void)
{   (void)fprintf(stderr,"[-fs_blocks <min fs blocks:128>]\n"); 
    (void)fprintf(stderr,"[-homeostatic <output file> [-guard:FALSE]]\n\n");
    (void)fprintf(stderr,"[< <input file> > <output file> >& <ASCII log file>]\n\n");

    (void)fprintf(stderr,"Signals\n\n");
    (void)fprintf(stderr,"SIGINIT SIGCHAN SIGPSRP: Process status [PSRP] request (protocol %2.2fF)\n",PSRP_PROTOCOL_VERSION);
    (void)fprintf(stderr,"SIGCLIENT: tell client server is about to segment\n");

    #ifdef CRUI_SUPPORT
    (void)fprintf(stderr,"SIGCHECK SIGRESTART:      checkpoint and restart signals\n");
    #endif /* CRIU_SUPPORT */

    (void)fprintf(stderr,"SIGALIVE: check for existence of client on signal dispatch host\n");
    (void)fflush(stderr);
}
		    
		    


#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT)() __attribute__ ((aligned(16))) = fsw_slot;
_EXTERN void (* USE )() __attribute__ ((aligned(16))) = fsw_usage;
#endif /* SLOT */



/*------------------------------------------------------------------------------
    Application build date ...
------------------------------------------------------------------------------*/

_EXTERN char appl_build_time[SSIZE] = __TIME__;
_EXTERN char appl_build_date[SSIZE] = __DATE__;





/*------------------------------------------------------------------------------
    Private variables required by this application ...
------------------------------------------------------------------------------*/

                                              /*---------------------------------------*/
_PRIVATE int fs_blocks               = 128;   /* Default minimum blocks in file system */
_PRIVATE char output_f_name[SSIZE];           /* O/P file homeostatically protected    */
_PRIVATE _BOOLEAN output_homeostasis = FALSE; /* Homeostatic if TRUE                   */
_PRIVATE _BOOLEAN guarding           = FALSE; /* TRUE if guarding a file resource      */
                                              /*---------------------------------------*/



/*------------------------------------------------------------------------------
    Local prototype function declarations ...
------------------------------------------------------------------------------*/

_PROTOTYPE _PRIVATE int psrp_process_status(int, char *[]);



/*------------------------------------------------------------------------------
    Report process status ...
------------------------------------------------------------------------------*/

_PRIVATE int psrp_process_status(int argc, char *argv[])

{    (void)fprintf(psrp_out,"\n    File system watcher/migrator status\n");
     (void)fprintf(psrp_out,"    ===================================\n\n");
     (void)fflush(psrp_out); 

     #ifdef CRIU_SUPPORT
     (void)fprintf(psrp_out,"    Binary is Criu enabled (checkpointable)\n");
     #endif /* CRIU_SUPPORT */

     if(output_homeostasis == TRUE)
     {  (void)fprintf(psrp_out, "    fsw (%d@%s): protecting output file (%s)\n",
                                          appl_pid,appl_host,output_f_name);
        (void)fflush(psrp_out);
     }

     if(fs_write_blocked == TRUE)
        (void)fprintf(psrp_out,
                "    fsw (%d@%s): waiting for space on fs (< %d blocks)\n",
                                              appl_pid,appl_host,fs_blocks);
     else
     {  if(guarding == TRUE)
           (void)fprintf(psrp_out,"    fsw (%d@%s): guarding file (%s)\n",appl_pid,appl_host,output_f_name);
        else
           (void)fprintf(psrp_out,"    fsw (%d@%s): writing data\n",appl_pid,appl_host);
     }

     (void)fprintf(psrp_out,"\n\n");
     (void)fflush(psrp_out);

     return(PSRP_OK);
}



               
/*------------------------------------------------------------------------------
    Software I.D. tag (used if CKPT support enabled to discard stale dynamic
    checkpoint files) ...
-------------------------------------------------------------------------------*/

#define VTAG  3337

extern int appl_vtag = VTAG;




/*------------------------------------------------------------------------------
    Main entry point ...
------------------------------------------------------------------------------*/

_PUBLIC int pups_main(int argc, char *argv[])

                                    /*---------------------------------*/
{   int      bytes_read;            /* Number of bytes read from stdin */
    char     line_of_input[SSIZE];  /* Line buffer                     */
    _BOOLEAN guard = FALSE;         /* True if we are guarding file    */
                                    /*---------------------------------*/


    /*--------------------------------*/
    /* Wait for PUPS/P3 to initialise */
    /*--------------------------------*/

    (void)psrp_ignore_requests();

    /*--------------------*/
    /* Parse command tail */
    /*--------------------*/


    /*-----------------------------*/
    /* Do standard initialisations */
    /*-----------------------------*/

    pups_std_init(TRUE,
                  &argc,
                  FSW_VERSION,
                  "M.A. O'Neill",
                           "fsw",
                          "2019",
                            argv);

    (void)psrp_init(PSRP_STATUS_ONLY  | PSRP_HOMEOSTATIC_STREAMS, &psrp_process_status);
    (void)psrp_load_default_dispatch_table();
    (void)psrp_accept_requests();


    /*------------------------------------------*/
    /* We must disable automatic child handling */
    /*------------------------------------------*/

    pups_noauto_child();


    /*-----------------------------------------------------------*/
    /* Get number of blocks at which the wait state is triggered */
    /*-----------------------------------------------------------*/

    if((ptr = pups_locate(&init,"fs_blocks",&argc,args,0)) != NOT_FOUND)
    {  if((fs_blocks = pups_i_dec(&ptr,&argc,args)) == INVALID_ARG)
          pups_error("[fsw] expecting wait state trigger block count");

       fs_blocks = iabs(fs_blocks);
    }


    /*---------------------*/
    /* Protect output file */
    /*---------------------*/

    if((ptr = pups_locate(&init,"homeostatic",&argc,args,0)) != NOT_FOUND)
    {  int  outdes = (-1);

       if(strccpy(output_f_name,pups_str_dec(&ptr,&argc,args)) == (char *)NULL)
          pups_error("[fsw] expecting name of output file to protect");


       /*----------------------------------------*/
       /* Check that output file actually exists */
       /*----------------------------------------*/

       if(access(output_f_name,F_OK | W_OK) == (-1))
       {  (void)fprintf(stderr,"fsw: output file (%s) does not exist/is not accessible\n",output_f_name);
          (void)fflush(stderr);

          pups_exit(-1);
       }


       /*-----------------------------------------------------------------*/
       /* Open dummy descriptor to protected file and start its homeostat */
       /*-----------------------------------------------------------------*/

       outdes             = pups_open(output_f_name,1,LIVE);
       (void)pups_creator(outdes);
       (void)pups_fd_alive(outdes,"default_fd_homeostat",pups_default_fd_homeostat);  
       output_homeostasis = TRUE;


       /*--------------------------------------------------*/
       /* Are we going to stand gaurd over this file after */
       /* data is written to it?                           */
       /*--------------------------------------------------*/

       if(pups_locate(&init,"guard",&argc,args,0) != NOT_FOUND)
          guard = TRUE;
    }


    /*-------------------------------------------*/
    /* Check command tail for unparsed arguments */
    /*-------------------------------------------*/

    pups_t_arg_errs(argd,args);

    if(isatty(0) == 1)
       pups_error("[fsw] no file attached to stdin");

    if(isatty(1) == 1)
       pups_error("[fsw] no file attached to stdout");

/*------------------------------------------------------------------------------
    This is the pups_main part of the program - it takes data from stdin and
    pushes it to stdout ...
------------------------------------------------------------------------------*/

    (void)pups_set_fs_hsm_parameters(1,fs_blocks,(char *)NULL); 
    do { 
           bytes_read = read(0,line_of_input,SSIZE);

           if(bytes_read > 0)
           {  (void)pups_write_homeostat(1,(void *)NULL);
              (void)write(1,line_of_input,bytes_read);
           }

       } while(bytes_read != 0);


    /*----------------------------------------------*/
    /* If we are guarding file persist indefinitely */
    /*----------------------------------------------*/

    if(guard == TRUE)
    {  guarding = TRUE;

      while(TRUE)
           (void)sched_yield();
    }

    pups_exit(0);
}
