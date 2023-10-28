/*----------------------------------------------------------------------------
     Purpose: Saves command state (via Criu) 

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.01 
     Dated:   24th May 2023
     E-mail:  mao@tumblingdice.co.uk
-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <bsd/string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>



/*-------------*/
/* Version tag */
/*-------------*/

#define SSAVETAG    20


/*---------*/
/* Version */
/*---------*/

#define SSAVE_VERSION        "2.01"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE                2048 


/*------------------*/
/* Type definitions */
/*------------------*/

#define _PUBLIC
#define _PRIVATE             static
#define _BOOLEAN             unsigned char


/*---------------*/
/* Boolean types */
/*---------------*/

#define FALSE                0
#define TRUE                 255


/*--------------------*/
/* Checkpoint signals */
/*--------------------*/

#define SIGCHECK             SIGRTMIN + 11
#define SIGRESTART           SIGRTMIN + 12


/*----------------------------------*/
/* Default time between state saves */
/*----------------------------------*/

#define DEFAULT_POLL_TIME    4 




/*------------------------------------------------*/
/* Variables which are global to this application */
/*------------------------------------------------*/

_PRIVATE int  child_pid       = (-1);
_PRIVATE char buildStr[SSIZE] = "";




/*---------------------------------------*/
/* Signal handler for SIGINT and SIGTERM */
/*---------------------------------------*/

_PRIVATE int term_handler(int signum)

{   (void)fprintf(stderr,"ssave: aborted\n\n");
    (void)fflush(stderr);

    (void)kill(child_pid,SIGTERM);
    exit(-2);
}




/*------------------*/
/* Main entry point */
/*------------------*/

_PUBLIC int main(int argc, char *argv[])

{   int i,
        argd                  = 1;

    time_t poll_time          = DEFAULT_POLL_TIME;

    _BOOLEAN do_verbose       = FALSE;

    char criu_check_f[SSIZE]    = "",
         reply[SSIZE]           = "",
         criu_cmd[SSIZE]        = "",
         payload_cmd[SSIZE]     = "",
         payload_sh_cmd[SSIZE]  = "",
         criu_dir[SSIZE]        = "/tmp",
         ssave_dir[SSIZE]       = "",
         ssave_buildpath[SSIZE] = "";

    FILE *stream              = (FILE *)NULL;


    /*------------------------------------*/
    /* Build details for this application */
    /*------------------------------------*/

    (void)snprintf(buildStr,SSIZE,"%s %d %s",getenv("MACHTYPE"),SSAVETAG,__DATE__);


    /*--------------------*/
    /* Parse command tail */
    /*--------------------*/

    if(argc == 1)
    {  (void)fprintf(stderr,"\n   ssave version %s (built %s %s)\n",SSAVE_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"   (C) Tumbling Dice, 2016-2023\n\n");
       (void)fprintf(stderr,"   Usage: ssave [-usage | -help] |\n");
       (void)fprintf(stderr,"                [-t <poll time:%d>]\n",DEFAULT_POLL_TIME);
       (void)fprintf(stderr,"                [-cd <criu directory:/tmp>]\n");
       (void)fprintf(stderr,"                !-p <payload command>!\n");
       (void)fprintf(stderr,"                [>& <error/status log>]\n\n");
       (void)fflush(stderr);

       exit(1);
    }
   
    for(i=1; i<argc; ++i)      
    {

       /*--------------------------*/
       /* Display help information */
       /*--------------------------*/

       if(strcmp(argv[i],"-usage") == 0 || strcmp(argv[i],"-help") == 0)
       {  (void)fprintf(stderr,"\n   ssave version %s (built %s %s)\n",SSAVE_VERSION,__TIME__,__DATE__);
          (void)fprintf(stderr,"   (C) Tumbling Dice, 2016-2023\n\n");
          (void)fprintf(stderr,"   Usage: ssave [-usage | -help] |\n");
          (void)fprintf(stderr,"                [-t <poll time:%d>]\n",DEFAULT_POLL_TIME);
          (void)fprintf(stderr,"                [-cd <criu directory:/tmp>\n");
          (void)fprintf(stderr,"                !-p <payload command>!\n");
          (void)fprintf(stderr,"                [>& <errori/status log>]\n\n");
          (void)fflush(stderr);

          exit(1);
       }


       /*--------------*/
       /* Verbose mode */
       /*--------------*/

       if(strcmp(argv[i],"-verbose") == 0)
       {  do_verbose = TRUE;
          ++argd;
       }


       /*-------------------------------*/
       /* Get time between check points */
       /*-------------------------------*/

       else if(strcmp(argv[i],"-t") == 0)
       {  if(i == argc - 1 || argv[i+1][0] == '-')
          {  if(do_verbose == TRUE)
             {  (void)fprintf(stderr,"ssave: expecting poll time in seconds (integer > 0)\n\n");
                (void)fflush(stderr);
             }

             exit(255);
          }


          /*--------------*/
          /* Sanity check */
          /*--------------*/

          else if((sscanf(argv[i+1],"%ld",&poll_time)) != 1 || poll_time <= 0)
          {  if(do_verbose == TRUE)
             {  (void)fprintf(stderr,"ssave: expecting poll time in seconds (integer > 0)\n\n");
                (void)fflush(stderr);
             }

             exit(255);
          }

          ++i;
          argd += 2;
       }


       /*---------------------*/
       /* Get payload command */
       /*---------------------*/

       else if(strcmp(argv[i],"-p") == 0)
       {  if(i == argc - 1 || argv[i+1][0] == '-')
          {  if(do_verbose == TRUE)
             {  (void)fprintf(stderr,"ssave: expecting name of payload command\n\n");
                (void)fflush(stderr);
             }

             exit(255);
          }
          else
             (void)strlcpy(payload_cmd,argv[i+1],SSIZE);

          ++i; 
          argd += 2;
       }


       /*----------------------------*/
       /* Get name of Criu directory */
       /*----------------------------*/

       else if(strcmp(argv[i],"-cd") == 0)
       {  if(i == argc - 1 || argv[i+1][0] == '-')
          {  if(do_verbose == TRUE)
             {  (void)fprintf(stderr,"ssave: expecting name of Criu directory\n\n");
                (void)fflush(stderr);
             }

             exit(255);
          }
          else
             (void)strlcpy(criu_dir,argv[i+1],SSIZE);

          ++i; 
          argd += 2;
       }
    }


    /*-------------------------------------------*/
    /* Check for unparsed command tail arguments */
    /*-------------------------------------------*/

    if(argd < argc)
    {  if(do_verbose == TRUE)
       {  (void)fprintf(stderr,"ssave: unparsed command tail arguments (parsed: %d, found: %d)\n\n",argc-1,argd-1);
          (void)fflush(stderr);
       }

       exit(255);
    }


    /*-----------------------------------------------*/
    /* Check that we have a working instance of Criu */
    /*-----------------------------------------------*/

    (void)snprintf(criu_check_f,SSIZE,"/tmp/criu.%d.check",getpid());
    (void)snprintf(criu_cmd,    SSIZE,"criu check >& %s",criu_check_f);
    (void)system(criu_cmd);

    if((stream = fopen(criu_check_f,"r")) == (FILE *)NULL)
    {  if(do_verbose == TRUE)
       {  (void)fprintf(stderr,"ssave: problem checking Criu command\n");
          (void)fflush(stderr);
       }
    }

    (void)fgets(reply,SSIZE,stream);
    (void)fclose(stream);
    (void)unlink(criu_check_f);


    #ifdef SSAVE_DEBUG
    (void)fprintf(stderr,"REPLY: %s\n",reply);
    (void)fflush(stderr);
    #endif /* SSAVE_DEBUG */

    if(strncmp(reply,"Looks good",10) != 0)
    {  if(do_verbose == TRUE)
       {  (void)fprintf(stderr,"ssave: no working Criu command (criu check failed)\n");
          (void)fflush(stderr);
       }

       exit(255);
    }


    /*------------------------------------*/
    /* We have a working instance of Criu */
    /*------------------------------------*/


    /*-----------------------------------------------------*/
    /* Ignore checkpoint signal so restarted applications  */
    /* can choose not to have a restart handler and not be */
    /* terminated                                          */
    /*-----------------------------------------------------*/

    (void)signal(SIGRESTART,SIG_IGN);


    /*-------------------------------------------*/
    /* Child side of fork run Criu to checkpoint */
    /* payload                                   */
    /*-------------------------------------------*/


    if((child_pid = fork()) == 0)
    {

       /*-------------------------*/
       /* Child should not return */
       /*-------------------------*/
  
       (void)snprintf(payload_sh_cmd,SSIZE,"exec %s",payload_cmd);
       (void)execlp("sh","sh","-c",payload_sh_cmd,(char *)NULL);

       if(do_verbose == TRUE)
       {  (void)fprintf(stderr,"ssave: problem running payload command\n");
          (void)fflush(stderr);
       }

       _exit(255);
    }


    /*-------------------------------------------*/
    /* Parent side of fork handles checkpointing */
    /*-------------------------------------------*/

    else
    {   struct timespec delay;


        /*----------------------------*/
        /* Handle termination signals */
        /*----------------------------*/

        (void)signal(SIGINT, (void *)term_handler);
        (void)signal(SIGTERM,(void *)term_handler);


        /*---------------------------------------*/
        /* Criu command - runs in background     */
        /* to ensure it is not part of the saved */
        /* process tree                          */ 
        /*---------------------------------------*/

        (void)snprintf(ssave_dir,SSIZE,"%s/criu.%d.ssave",criu_dir,child_pid);
        (void)snprintf(criu_cmd, SSIZE,"criu --leave-running --log-file /dev/null --link-remap --shell-job dump -D %s -t %d &",ssave_dir,getpid());

        #ifdef SSAVE_DEBUG
        (void)fprintf(stderr,"CRIU COMMAND:%s\n",criu_cmd);
        (void)fflush(stderr);
        #endif /* SSAVE_DEBUG */


        /*----------------------*/
        /* Checkpoint directory */
        /*----------------------*/

        if(mkdir(ssave_dir,0700) == (-1))
        {  if(do_verbose == TRUE)
           {  (void)fprintf(stderr,"ssave: could not create checkpoint directory\n\n");
              (void)fflush(stderr); 
           }

           exit(255);
        }


        /*--------------------*/
        /* Checkpointing loop */
        /*--------------------*/

        #ifdef SSAVE_DEBUG
        (void)fprintf(stderr,"CRIU LOOP1 head: %d %s\n",poll_time,ssave_dir);
        (void)fflush(stderr);
        #endif /* SSAVE_DEBUG */

        while(TRUE)
        {   int ret,
                status;


            #ifdef SSAVE_DEBUG
            (void)fprintf(stderr,"CRIU LOOP1\n");
            (void)fflush(stderr);
            #endif /* SSAVE_DEBUG */


            /*--------------------------*/
            /* Time between state saves */
            /*--------------------------*/

            delay.tv_sec  = poll_time;
            delay.tv_nsec = 0;
            ret = nanosleep(&delay,(struct timespec *)NULL);

            #ifdef SSAVE_DEBUG
            (void)fprintf(stderr,"CRIU LOOP2 ret: %d\n",ret);
            (void)fflush(stderr);
            #endif /* SSAVE_DEBUG */


            /*--------------------------------------------------*/
            /* Have we become child of init. If we are orphaned */
            /* and can stop caring for our (dead) parent        */
            /*--------------------------------------------------*/

            ret = waitpid(child_pid,&status,WNOHANG);

            if(ret == child_pid && WIFEXITED(status) == 1 && WIFSIGNALED(status) == 0) 
            {  char rm_cmd[SSIZE] = "";


               if(do_verbose == TRUE)
               {  (void)fprintf(stderr,"ssave: payload command has terminated successfully - exiting\n\n");
                  (void)fflush(stderr);
               }


               /*-------------------*/
               /* Remove checkpoint */
               /*-------------------*/

               (void)snprintf(rm_cmd,SSIZE,"rm -rf %s",ssave_dir);
               (void)system(rm_cmd);

               break;
            }


            /*----------------------*/
            /* Problem with payload */
            /*----------------------*/

            else if(ret == child_pid && WIFSIGNALED(status) == 1)
            {

               #ifdef SSAVE_DEBUG
               (void)fprintf(stderr,"EXIST STATUS: %d\n",WEXITSTATUS(status));
               (void)fflush(stderr);
               #endif /* SSAVE_DEBUG */

               if(do_verbose == TRUE)
               {  (void)fprintf(stderr,"ssave: payload command has terminated unexpectedly - exiting with checkpoint\n\n");
                  (void)fflush(stderr); 
               }

               exit(255);
            }              


            /*------------------------------------*/
            /* Put build date of this application */
            /* in checkpoint directory so restart */
            /* can see if it is stale             */
            /*------------------------------------*/

            (void)snprintf(ssave_buildpath,SSIZE,"%s/build",ssave_dir);
            if(access(ssave_buildpath,F_OK) == (-1))
               (void)close(creat(ssave_buildpath,0600));

            stream = fopen(ssave_buildpath,"w");
            (void)fprintf(stream,"%s\n",buildStr); 
            (void)fflush(stream);
            (void)fclose(stream);


            /*-----------------*/
            /* Take checkpoint */
            /*-----------------*/

            (void)system(criu_cmd);


            /*-------------------*/
            /* Dummy system call */
            /*-------------------*/

            delay.tv_sec  = (time_t)60;
            delay.tv_nsec = 0;
            (void)nanosleep(&delay,(struct timespec *)NULL);
        }
    }

    exit(0);
}
