/*----------------------------------------------------------------------------
     Purpose: Restarts checkpoint (via Criu) 

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.01 
     Dated:   24th May 2022
     E-mail:  mao@tumblingdice.co.uk
-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <bsd/string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>


/*-------------*/
/* Version tag */
/*-------------*/

#define SSAVETAG    20


/*---------------------*/
/* Version of restarit */
/*---------------------*/

#define RESTART_VERSION     "2.01"


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


/*--------------------*/
/* Checkpoint signals */
/*--------------------*/

#define SIGCHECK             SIGRTMIN + 11
#define SIGRESTART           SIGRTMIN + 12
#define DEFAULT_SIGNAL_DELAY 1


/*---------------*/
/* Boolean types */
/*---------------*/

#define FALSE                0
#define TRUE                 255


/*-----------------------------------------------*/
/* Variables which are local to this application */
/*-----------------------------------------------*/

char buildStr[SSIZE] = "";




/*------------------*/
/* Main entry point */
/*------------------*/

_PUBLIC int main(int argc, char *argv[])

{   int i,
        status,
        child_pid,
        argd                    = 1;

    unsigned int signal_delay   = DEFAULT_SIGNAL_DELAY;

    char ssave_dir[SSIZE]       = "",
         ssave_buildpath[SSIZE] = "",
         criu_cmd[SSIZE]        = "",
         ssaveBuildStr[SSIZE]   = "";

    _BOOLEAN do_verbose         = FALSE,
             do_detach          = FALSE;

    FILE *stream              = (FILE *)NULL;


    /*------------------------------------*/
    /* Build details for this application */
    /*------------------------------------*/

    (void)snprintf(buildStr,SSIZE,"%s %d %s",getenv("MACHTYPE"),SSAVETAG,__DATE__);


    /*--------------------*/
    /* Parse command tail */
    /*--------------------*/

    if(argc == 1)
    {  (void)fprintf(stderr,"\n   restart version %s (built %s %s)\n",RESTART_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"   (C) Tumbling Dice, 2016-2022\n\n");
       (void)fprintf(stderr,"   Usage: restart [-usage | -help] |\n");
       (void)fprintf(stderr,"                  [-detach]\n");
       (void)fprintf(stderr,"                  [-d <SIGRESTART signal delay secs:%d>]\n",DEFAULT_SIGNAL_DELAY);
       (void)fprintf(stderr,"                  !-c <checkpoint pathname>!\n");
       (void)fprintf(stderr,"                  [>& <error/status logfile>]\n\n");
       (void)fflush(stderr);

       exit(1);
    }

    for(i=1; i<argc; ++i)
    {

       /*--------------------------*/
       /* Display help information */
       /*--------------------------*/

       if(strcmp(argv[i],"-usage") == 0 || strcmp(argv[i],"-help") == 0)
       {  (void)fprintf(stderr,"\n   restart version %s (built %s)\n",RESTART_VERSION,__TIME__,__DATE__);
          (void)fprintf(stderr,"   (C) Tumbling Dice, 2016-2022\n\n");
          (void)fprintf(stderr,"   Usage: restart [-usage | -help] |\n");
          (void)fprintf(stderr,"                  [-detach]\n");
          (void)fprintf(stderr,"                  [-d <SIGRESTART signal delay secs:%d>]\n",DEFAULT_SIGNAL_DELAY);
          (void)fprintf(stderr,"                  !-c <checkpoint pathname>!\n");
          (void)fprintf(stderr,"                  [>& <error/status logfile>]\n\n");
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


       /*---------------------------*/
       /* Detach restarted process  */
       /* from controlling terminal */
       /*---------------------------*/

       if(strcmp(argv[i],"-detach") == 0)
       {  do_detach = TRUE;
          ++argd;
       }


       /*------------------*/
       /* Get signal delay */
       /*------------------*/

       else if(strcmp(argv[i],"-d") == 0)
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

          else if((sscanf(argv[i+1],"%ld",&signal_delay)) != 1 || signal_delay < 1)
          {  if(do_verbose == TRUE)
             {  (void)fprintf(stderr,"ssave: expecting (SIGRESTART) signal delay in seconds (integer > 0)\n\n");
                (void)fflush(stderr);
             }

             exit(255);
          }

          ++i;
          argd += 2;
       }


       /*--------------------------*/
       /* Get checkpoint to resume */
       /*--------------------------*/

       else if(strcmp(argv[i],"-c") == 0)
       {  if(i == argc - 1 || argv[i+1][0] == '-')
          {  if(do_verbose == TRUE)
             {  (void)fprintf(stderr,"restart: expecting checkpoint directory name\n\n");
                (void)fflush(stderr);
             }

             exit(255);
          }
          else
             (void)strlcpy(ssave_dir,argv[i+1],SSIZE);

          ++i;
          argd += 2;
       }
    }


    /*-------------------------------------------*/
    /* Check for unparsed command tail arguments */
    /*-------------------------------------------*/

    #ifdef RESTART_DEBUG
    (void)fprintf(stderr,"ARGC: %d ARGD: %d\n",argc,argd);
    (void)fflush(stderr);
    #endif /* RESTART_DEBUG */

    if(argd < argc)
    {  if(do_verbose == TRUE)
       {  (void)fprintf(stderr,"restart: unparsed command tail arguments (parsed: %d, found: %d)\n\n",argc-1,argd-1);
          (void)fflush(stderr);
       }

       exit(255);
    }


    /*------------------------*/
    /* Does checkpoint exist? */
    /*------------------------*/

    if(strcmp(ssave_dir,"") == 0)
    {  if(do_verbose == TRUE)
       {  (void)fprintf(stderr,"restart: checkpoint directory not specified\n\n");
          (void)fflush(stderr);
       }

       exit(255);
    }
    else if(access(ssave_dir,F_OK | R_OK) == (-1))
    {  if(do_verbose == TRUE)
       {  (void)fprintf(stderr,"restart: checkpoint directory \"%s\" not found\n\n",ssave_dir);
          (void)fflush(stderr);
       }

       exit(255);
    }


    /*----------------------*/
    /* Is checkpoint stale? */
    /*----------------------*/

    (void)snprintf(ssave_buildpath,SSIZE,"%s/build",ssave_dir);
    stream = fopen(ssave_buildpath,"r");
    (void)fgets(ssaveBuildStr,255,stream);
    (void)fclose(stream);

    #ifdef RESTART_DEBUG
    (void)fprintf(stderr,"buildStr: %s   ssaveBuildStr: %s\n",buildStr,ssaveBuildStr);
    (void)fflush(stderr);
    #endif /* RESTART_DEBUG */

    if(strncmp(buildStr,ssaveBuildStr,strlen(buildStr)) != 0)
    {  if(do_verbose == TRUE)
       {  (void)fprintf(stderr,"restart: checkpoint directory \"%s\" is stale\n\n",ssave_dir);
          (void)fflush(stderr);
       }

       exit(255);
    }



    /*------------------------------------------------------*/
    /* Detach criu from process once restart complete. This */
    /* is done if the checkpointed command was running in   */
    /* the background                                       */
    /*------------------------------------------------------*/

    if(do_detach == TRUE)
       (void)snprintf(criu_cmd,SSIZE,"criu --log-file /dev/null --shell-job restore -d -D %s",ssave_dir);


    /*---------------------------------------*/
    /* Run checkpointed command in foreground */
    /*---------------------------------------*/

    else 
       (void)snprintf(criu_cmd,SSIZE,"exec criu --log-file /dev/null --shell-job restore -D %s",ssave_dir);


    #ifdef RESTART_DEBUG
    (void)fprintf(stderr,"CRIU CMD: %s\n",criu_cmd);
    (void)fflush(stderr);
    #endif /* RESTART_DEBUG */


    /*--------------------------------------------*/
    /* Restart command must ignore the SIGRESTART */
    /* it sends to its own process group (which   */
    /* contains the restarted process tree)       */
    /*--------------------------------------------*/

    (void)signal(SIGRESTART,SIG_IGN);

    /*-----------------*/
    /* Restart command */
    /*-----------------*/

    if(fork() == 0)
    {  char signal_cmd[SSIZE] = "";


       /*-----------------------------------------*/
       /* Give Criu time to do its magic. This is */
       /* a horrible kludge. Wre should check for */
       /* reinstated member(s) of process tree    */
       /*-----------------------------------------*/

       (void)sleep(signal_delay);


       /*--------------------------------------------*/
       /* Send SIGRESTART to reinstated process tree */
       /*--------------------------------------------*/
 
       (void)kill(-getppid(),SIGRESTART);

       _exit(0);
    }
    else
       status = system(criu_cmd);


    /*----------------------------------------*/
    /* Checkpointed command has not restarted */
    /*----------------------------------------*/

    if(WEXITSTATUS(status) == (-1))
    {  if(do_verbose == TRUE)
       {  (void)fprintf(stderr,"restart: problem restarting checkpoint\n\n");
          (void)fflush(stderr);
       }
    }

    exit(255);
}
