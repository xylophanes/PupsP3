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
    Dated:   4th January 2022
    Email:   mao@tumblingdice.co.uk
------------------------------------------------------------------------------*/

#include <me.h>
#include <utils.h>
#include <netlib.h>
#include <string.h>
#include <psrp.h>
#include <vstamp.h>
#include <stdlib.h>
#include <unistd.h>


/*-----------------*/
/* Version of xtee */
/*-----------------*/

#define XTEE_VERSION    "2.00"


#ifdef BUBBLE_MEMORY_SUPPORT
#include <bubble.h>
#endif /* BUBBLE_MEMORY_SUPPORT */




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

_PRIVATE void xtee_slot(int level)
{   (void)fprintf(stderr,"int app xtee %s: [ANSI C]\n",XTEE_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(C) 1999-2022 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"Extended tee filter (built %s %s)\n\n",__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");
    (void)fflush(stderr);  
}
					 



/*----------------------------*/
/* Application usage function */
/*----------------------------*/

_PRIVATE void xtee_usage(void)
{   (void)fprintf(stderr,"[-fifo:FALSE] | ");
    (void)fprintf(stderr,"[-pipe <command stream> [-detached:FALSE] | ");
    (void)fprintf(stderr,"[-tee_file <tee file name>]\n");
    (void)fprintf(stderr,"[-append:FALSE] |\n");
    (void)fprintf(stderr,"[-compress:FALSE]]\n");
    (void)fprintf(stderr,"[< <input file> 1> <output file> 2> <ASCII log file>]\n\n");

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
_EXTERN void (* SLOT)() __attribute__ ((aligned(16))) = xtee_slot;
_EXTERN void (* USE )() __attribute__ ((aligned(16))) = xtee_usage;
#endif /* SLOT */


/*------------------------------------------------------------------------------
    Application build date ...
------------------------------------------------------------------------------*/

_EXTERN char appl_build_time[SSIZE] = __TIME__;
_EXTERN char appl_build_date[SSIZE] = __DATE__;





/*------------------------------------------------------------------------------
    Private variables required by this application ...
------------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN tee_to_fifo         = FALSE;
_PRIVATE _BOOLEAN tee_to_pipestream   = FALSE;
_PRIVATE _BOOLEAN do_compress         = FALSE;
_PRIVATE _BOOLEAN detached            = FALSE;

                                                   /*-------------------------*/
_PRIVATE char tee_f_name[SSIZE]         = "";      /* Name of tee file        */
_PRIVATE char line_of_input[SSIZE]      = "";      /* Line buffer             */
_PRIVATE char compress_command[SSIZE]   = "";;     /* Compression command     */
_PRIVATE char pipestream_command[SSIZE] = "";      /* Compression pipestream  */
_PRIVATE int tee_file_des;                         /* Tee file descriptor     */
_PRIVATE int bytes_read;                           /* Bytes read (input)      */
                                                   /*-------------------------*/



/*------------------------------------------------------------------------------
    Report process status ...
------------------------------------------------------------------------------*/

_PRIVATE int psrp_process_status(int argc, char *argv[])

{    (void)fprintf(psrp_out,"\n    Extended tee filter status\n");
     (void)fprintf(psrp_out,"    =============================\n\n");
     (void)fflush(psrp_out);

     #ifdef CRIU_SUPPORT
     (void)fprintf(psrp_out,"    Binary is Criu enabled (checkpointable)\n");
     #endif /* CRIU_SUPPORT */

     if(tee_to_pipestream == TRUE)
        (void)fprintf(psrp_out,"    Teeing stdin to (embedded) pipestream (%s)\n",pipestream_command);
     else if(tee_to_fifo == TRUE)
        (void)fprintf(psrp_out,"    Teeing stdin to (named) FIFO (%s)\n",tee_f_name);
     else
     {  if(do_compress == TRUE)
           (void)fprintf(psrp_out,"    Teeing stdin (with compression) to regular file %s\n",tee_f_name);
        else
           (void)fprintf(psrp_out,"    Teeing stdin to regular file %s\n",tee_f_name);
     }
         
     (void)fprintf(psrp_out,"\n\n");
     (void)fflush(psrp_out);
 
     return(PSRP_OK);
}




/*------------------------------------------------------------------------------
    Software I.D. tag (used if CKPT support enabled to discard stale dynamic
    checkpoint files) ...
-------------------------------------------------------------------------------*/

#define VTAG  3686

extern int appl_vtag = VTAG;





/*------------------------------------------------------------------------------
    Main entry point to application ...
------------------------------------------------------------------------------*/

_PUBLIC int pups_main(int argc, char *argv[])

{   _BOOLEAN do_append = FALSE;


    /*--------------------------------------------*/
    /* Ignore PUPS/P3 requests while initialising */
    /*--------------------------------------------*/

    psrp_ignore_requests();


    /*--------------------*/
    /* Parse command tail */
    /*--------------------*/

    /* Do standard initialisations */
    pups_std_init(TRUE,
                  &argc,
                  XTEE_VERSION,
                  "M.A. O'Neill",
                  "xtee",
                  "2022",
                  argv);



    /*------------------------------*/
    /* Enable PUPS/P3 communication */
    /*------------------------------*/

    (void)psrp_init(PSRP_STATUS_ONLY  | PSRP_HOMEOSTATIC_STREAMS,&psrp_process_status);
    (void)psrp_load_default_dispatch_table();
    (void)psrp_accept_requests();
 
    if(isatty(0) == 1)
       pups_error("[xtee] no file attached to stdin");

    if(isatty(1) == 1)
       pups_error("[xtee] no file attached to stdout");


    /*----------------------------------------------------*/
    /* If pipe flag is set divert the O/P to a pipestream */
    /*----------------------------------------------------*/

    if((ptr = pups_locate(&init,"pipe",&argc,args,0)) != NOT_FOUND)
    {   if(strccpy(pipestream_command,pups_str_dec(&ptr,&argc,args)) == (char *)NULL)
           pups_error("[xtee] expecting command stream");

        tee_to_pipestream = TRUE;
        tee_file_des = pups_copen(pipestream_command,shell,1);

        if(appl_verbose == TRUE)
        {  (void)fprintf(stderr,"    Tee file is a command pipeline");

           if(detached == TRUE)
              (void)fprintf(stderr," (detached)\n");
           else
              (void)fprintf(stderr,"\n");
           (void)fflush(stderr);
        }
    }
    else
    {
       /*---------------------------------------------------------*/
       /* If fifo flag set then tee file is a System V named FIFO */
       /*---------------------------------------------------------*/

       if(pups_locate(&init,"fifo",&argc,args,0) != NOT_FOUND)
       {  tee_to_fifo = TRUE;
          if(appl_verbose == TRUE)
          {  (void)fprintf(stderr,"    Tee file is a named FIFO\n");
             (void)fflush(stderr);
          }
       }

       if(pups_locate(&init,"compress",&argc,args,0) != NOT_FOUND)
       {  if(tee_to_fifo == TRUE || tee_to_pipestream)
             pups_error("[xtee] cannot have both compress and pipe or fifo flags set");

          do_compress = TRUE;

          if(appl_verbose == TRUE)
          {  (void)fprintf(stderr,"    Compressing tee'd output\n");
             (void)fflush(stderr);
          }
       }


       /*-------------------*/
       /* Get tee file name */
       /*-------------------*/

       if((ptr = pups_locate(&init,"tee_file",&argc,args,0)) != NOT_FOUND)
       {  if(strccpy(tee_f_name,pups_str_dec(&ptr,&argc,args)) == (char *)INVALID_ARG)
              pups_error("[xtee] expecting tee file name");

          if(appl_verbose == TRUE)
          {  (void)fprintf(stderr,"    Tee file name is %s\n",tee_f_name);
             (void)fflush(stderr);
          }

          if(do_compress == TRUE)
          {  snprintf(compress_command,SSIZE,"xz > %s",tee_f_name);
             tee_file_des = pups_copen(compress_command,shell,1);
          }
          else
          {  if(tee_to_fifo == TRUE)
                (void)mkfifo(tee_f_name,2);
             else
                creat(tee_f_name,0644);


             /*-----------------------------------------------------------*/
             /* If we are teeing to a regular file are we appending data? */
             /*-----------------------------------------------------------*/

             if((ptr = pups_locate(&init,"append",&argc,args,0)) != NOT_FOUND)
             {  if(appl_verbose == TRUE)
                {  (void)fprintf(stderr,"    Append mode (for regular file \"%s\")\n",tee_f_name);
                   (void)fflush(stderr);
                }


                /*----------------------*/
                /* Eextend regular file */
                /*----------------------*/

                tee_file_des = pups_open(tee_f_name,1,LIVE);
		(void)lseek(tee_file_des,0,SEEK_END);
             }
             else

                /*------------------------*/
                /* Overwrite regular file */
                /*------------------------*/

                tee_file_des = pups_open(tee_f_name,1,LIVE);
          }
       }
       else
          pups_error("[xtee] no tee file specfied");


       /*-----------------------------------------------------------*/
       /* If we are teeing to a regular file are we appending data? */
       /*-----------------------------------------------------------*/

       if(tee_to_fifo == FALSE && tee_to_pipestream == FALSE && (ptr = pups_locate(&init,"append",&argc,args,0)) != NOT_FOUND)
       {  do_append = TRUE;

          if(appl_verbose == TRUE)
          {  (void)fprintf(stderr,"    Append mode (for regular file \"%s\")\n",tee_f_name);
             (void)fflush(stderr);
          }
       }
    }


    /*------------------------------------------------*/
    /* Complain about any unparsed command line items */
    /*------------------------------------------------*/

    pups_t_arg_errs(argd,args);

    /*--------------------------------------------------------------------------*/
    /* This is the pups_main part of the program - it takes data from stdin and */
    /* pushes it to stdout                                                      */
    /*--------------------------------------------------------------------------*/

     do {  if((bytes_read = read(0,line_of_input,SSIZE)) != 0)
           {  write(tee_file_des,line_of_input,bytes_read);
              write(1,line_of_input,bytes_read);
           }
        } while(bytes_read > 0); /* Was != 0 */

    if(tee_to_fifo == TRUE)
    {  (void)close(tee_file_des);    
       unlink(tee_f_name);
    }
    else
    {  if(tee_to_pipestream == TRUE)
       {  if(detached == TRUE)
          {  (void)close(tee_file_des);

             if(fork() != 0)
                pups_exit(0);

             close(0);
             close(1);
             close(2);
          } 
          else
             (void)close(tee_file_des);
       }
       else
          (void)close(tee_file_des);
    }

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): finished\n",date,appl_name,appl_pid,appl_host,appl_owner);
       (void)fflush(stderr);
    }

    pups_exit(0);
}
