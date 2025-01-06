/*--------------------------------------------------------------------
   Purpose: Place marker for monitoring applications in live pipelines 

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.02 
    Dated:   29th December 2024
    E-mail:  mao@tumblingdice.co.uk 
--------------------------------------------------------------------*/

#include <me.h>
#include <utils.h>
#include <signal.h>
#include <string.h>
#include <psrp.h>
#include <unistd.h>
#include <vstamp.h>
#include <bsd/bsd.h>


/*---------*/
/* Version */
/*---------*/

#define XCAT_VERSION    "2.02"


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




/*-----------------------------------------------------------------------------*/
/* Version ID (used by CKPT enabled binaries to discard stale checkpoint files */
/*-----------------------------------------------------------------------------*/

#include <vstamp.h>


/*----------------------------------------------*/
/* Get application information for slot manager */
/*----------------------------------------------*/
/*---------------------------*/
/* Slot information function */
/*---------------------------*/

_PRIVATE void xcat_slot(int level)
{   (void)fprintf(stderr,"int app xcat %s: [ANSI C]\n",XCAT_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(C) 2005-2024 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"Pipeline insertion marker for dynamic pipeline monitors (gcc %s: built %s %s)\n\n",__VERSION__,__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");
    (void)fflush(stderr);  
}
					 



/*----------------------------*/
/* Application usage function */
/*----------------------------*/

_PRIVATE void xcat_usage(void)

{   (void)fprintf(stderr,"xcat [< <input file> > <output file> >& <ASCII log file>]\n");

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
_EXTERN void (* SLOT)() __attribute__ ((aligned(16))) = xcat_slot;
_EXTERN void (* USE )() __attribute__ ((aligned(16))) = xcat_usage;
#endif /* SLOT */


/*------------------------*/
/* Application build date */
/*------------------------*/

_EXTERN char appl_build_time[SSIZE] = __TIME__;
_EXTERN char appl_build_date[SSIZE] = __DATE__;





/*---------*/
/* Defines */
/*---------*/

#define TO_STDOUT    1
#define TO_PSRP      2
#define TO_PTAP      4
#define TO_DATASINK  8




/*-----------------*/
/* Local variables */
/*-----------------*/

                                                       /*-------------------------------------*/
_PRIVATE _BOOLEAN data_flowing         = TRUE;         /* State of dataflow (stdin -> stdout) */
_PRIVATE _BOOLEAN root_instance        = TRUE;         /* State of dataflow (stdin -> stdout) */
_PRIVATE ssize_t  bytes_read;                          /* Number of bytes read from stdin     */
_PRIVATE char     line_of_input[SSIZE] = "";           /* Line buffer                         */
_PRIVATE char     mytmpfile[SSIZE]     = "";           /* Temporary file name                 */
_PRIVATE FILE     *mystream            = (FILE *)NULL; /* I/O stream                          */
                                                       /*-------------------------------------*/



/*---------------------------------------*/
/* Local prototype function declarations */
/*---------------------------------------*/

/* Report status of function to PSRP client */
_PROTOTYPE _PRIVATE  int32_t psrp_process_status(int32_t, char *[]);

/* Switch dataflow on/off */
_PROTOTYPE _PRIVATE  int32_t dataflow(int32_t, char *[]);




/*-----------------------*/
/* Report process status */
/*-----------------------*/

_PRIVATE  int32_t psrp_process_status(int32_t argc, char *argv[])

{    (void)fprintf(psrp_out,"\n    Pipleline dynamic insertion marker status\n");
     (void)fprintf(psrp_out,"    =========================================\n\n");
     (void)fflush(psrp_out); 

     #if defined(CRIU_SUPPORT)
     (void)fprintf(psrp_out,"    Binary is Criu enabled (checkpointable)\n");
     #endif /* CRIU_SUPPORT */

     if(data_flowing == TO_STDOUT)
        (void)fprintf(psrp_out,"    xcat (%d@%s): transferring data (from stdin to stdout)\n",appl_pid,appl_host);
     else if(data_flowing == TO_DATASINK)
        (void)fprintf(psrp_out,"    xcat (%d@%s): dataflow interrupted\n",appl_pid,appl_host);

     (void)fprintf(psrp_out,"\n\n");
     (void)fflush(psrp_out);

     return(PSRP_OK);
}




/*----------------------------------------*/
/* Switch dataflow to specified data sink */
/*----------------------------------------*/

_PRIVATE  int32_t dataflow(int32_t argc, char *argv[])

{   if(strcmp(argv[1],"help") == 0 || strcmp(argv[1],"usage") == 0)
    {  (void)fprintf(psrp_out,"usage: dataflow [stdout | psrp | ptap | off]\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(argc == 1)
    {  if(data_flowing == TO_DATASINK)
          (void)fprintf(psrp_out,"\n    dataflow: OFF\n");
       else if(data_flowing == TO_STDOUT)
          (void)fprintf(psrp_out,"\n    dataflow: standard output\n");
    }
    else if(strcmp(argv[1],"stdout")  == 0)
    {  data_flowing = TO_STDOUT;

       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): output redirected to standard output\n",
                                             date,appl_name,appl_pid,appl_host,appl_owner); 
          (void)fflush(stderr);
       }
    }
    else if(strcmp(argv[1],"psrp")  == 0 || strcmp(argv[1],"ptap") == 0)
    {  char xcat_command[SSIZE] = "";

       if(strcmp(argv[1],"psrp")  == 0)
          data_flowing = TO_PSRP;
       else
          data_flowing = TO_PTAP;

       if(appl_verbose == TRUE)
       {  (void)strdate(date);

          if(strcmp(argv[1],"psrp")  == 0)
             (void)fprintf(stderr,"%s %s (%d@%s:%s): output redirected to PSRP client\n",
                                            date,appl_name,appl_pid,appl_host,appl_owner); 
          else
             (void)fprintf(stderr,"%s %s (%d@%s:%s): output tapped to PSRP client\n",
                                        date,appl_name,appl_pid,appl_host,appl_owner); 
          (void)fflush(stderr);
       }


       /*-------------------------------------------------*/
       /* Client side abort callback for xcat application */
       /*-------------------------------------------------*/

       do {   bytes_read = read(0,line_of_input,SSIZE);

              if(bytes_read > 0)
              {  (void)fputs(line_of_input,psrp_out);
                 (void)fflush(psrp_out);


                 /*------------------------------------------------*/
                 /* If we are tapping standard output send data to */
                 /* standard output as well.                       */
                 /*------------------------------------------------*/

                 if(strcmp(argv[1],"ptap")  == 0 && root_instance == TRUE)
                    (void)write(1,line_of_input,bytes_read);
              }
          } while(bytes_read > 0);

       (void)fprintf(psrp_out,"\nEnd of file (on standard input)\n\n");
       (void)fflush(psrp_out);

       pups_exit(0);
    }
    else if(strcmp(argv[1],"off")  == 0)
    {  data_flowing = TO_DATASINK;

       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): output redirected to data sink\n",
                                       date,appl_name,appl_pid,appl_host,appl_owner); 
          (void)fflush(stderr);
       }
    }

    return(PSRP_OK);
}



/*-------------*/
/* Remove junk */
/*-------------*/

_PRIVATE  int32_t remove_junk(void)

{    (void)pups_fclose(mystream);
     (void)unlink(mytmpfile);
}




/*-------------------*/
/* Software I.D. tag */
/*-------------------*/

#define VTAG  4323
extern  int32_t appl_vtag = VTAG;




/*------------------*/
/* Main entry point */
/*------------------*/

_PUBLIC  int32_t pups_main(int32_t argc, char *argv[])


{

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
                  XCAT_VERSION,
                  "M.A. O'Neill",
                  "xcat",
                  "2024",
                  (void *)argv);

    (void)psrp_init(PSRP_STATUS_ONLY  | PSRP_HOMEOSTATIC_STREAMS,(void *)&psrp_process_status);
    (void)psrp_attach_static_function("status",(void *)&psrp_process_status);
    (void)psrp_attach_static_function("flow",(void *)&dataflow);
    (void)psrp_load_default_dispatch_table();
    (void)psrp_accept_requests();

    if(root_instance == TRUE)
       (void)pups_register_exit_f("remove_junk",&remove_junk,(char *)NULL);


    /*------------------------------------------*/ 
    /* We must disable automatic child handling */
    /*------------------------------------------*/ 

    pups_noauto_child();


    /*-------------------------------------------*/
    /* Check command tail for unparsed arguments */
    /*-------------------------------------------*/

    pups_t_arg_errs(argd,(void *)args);

    if(isatty(0) == 1)
       pups_error("[xcat] no file attached to stdin");


    /*--------------------------------------------------------------------------*/
    /* This is the pups_main part of the program - it takes data from stdin and */
    /* pushes it to stdout                                                      */
    /*--------------------------------------------------------------------------*/

    do {   
           if(data_flowing == TO_PSRP || data_flowing == TO_PTAP)
           {  data_flowing = TO_STDOUT;

              if(appl_verbose == TRUE)
              {  (void)strdate(date);
                 (void)fprintf(stderr,"%s %s (%d@%s:%s): dataflow to standard output resumed\n",
                                                   date,appl_name,appl_pid,appl_host,appl_owner);
                 (void)fflush(stderr);
              }

              if(bytes_read > 0 && root_instance == TRUE)
                 (void)pups_write(1,line_of_input,bytes_read);
           }

           bytes_read = read(0,line_of_input,SSIZE);
           if(bytes_read > 0)
           {  if(data_flowing == TO_STDOUT)
                 (void)pups_write(1,line_of_input,bytes_read);
           }
       } while(bytes_read > 0);

    pups_exit(0);
}
