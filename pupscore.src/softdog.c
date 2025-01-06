/*----------------------------------------
    Purpose: Per process software watchdog 

    Author:  Mark A. O'Neill
             Tumbling Dice
             Gosforth
             NE3 4RT 

    Version: 1.02 
    Date:    11th December 2024 
    Email:   mao@tumblingdice.co.uk
---------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <bsd/string.h>
#include <xtypes.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>


#define _XOPEN_SOURCE
#include <unistd.h>


/*---------*/
/* Defines */
/*---------*/

#define SOFTDOG_VERSION         "1.02" 
#define DEFAULT_SOFTDOG_TIMEOUT  60 


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE                    2048


/*----------------------*/
/* Signaction structure */
/*----------------------*/

_PRIVATE struct sigaction sighandle;


/*-------------------*/
/* Private Variables */
/*-------------------*/

                                                                   /*------------------------------------*/
_PRIVATE _BOOLEAN     do_verbose       = FALSE;                    /* Log status to stderr if TRUE       */
_PRIVATE uint32_t     monitor_pgrp     = (-1);                     /* Pgrp of monitored process group    */
_PRIVATE uint32_t     monitor_pid      = (-1);                     /* Pid of monitored process           */
_PRIVATE uint32_t     softdog_timeout  = DEFAULT_SOFTDOG_TIMEOUT;  /* Softfog timeout (seconds)          */
_PRIVATE unsigned char hostname[SSIZE] = "";                       /* Host running this softdog instance */
                                                                   /*------------------------------------*/


/*----------------*/
/* Signal handler */
/*----------------*/

_PRIVATE int32_t sighandler(const int32_t signum)

{   sigset_t pending_set;


    /*----------------------------------*/
    /* Which signal are we processing ? */
    /*----------------------------------*/

    switch(signum)
    {

          /*------------------------*/
          /* Stop software watchdog */
          /*------------------------*/

          case SIGINT:
          case SIGQUIT:
          case SIGTERM:   
		 
		          /*---------------*/ 
		          /* Close softdog */
		          /*---------------*/

		          (void)alarm(0);


			  /*-------------------------*/
                          /* Close hardware watchdog */
			  /*-------------------------*/
			  /*-------------------------------------------------------------------------*/
			  /* The 'V' value needs to be written into watchdog device file to indicate */
			  /* that we intend to close/stop the watchdog. Otherwise, debug message     */
			  /* watchdog timer closed unexpectedly' will be printed                     */
			  /*-------------------------------------------------------------------------*/

			  if (do_verbose == TRUE)
			  {  (void)fprintf(stderr,"\nsoftdog (%d@%s): aborted\n\n",getpid(),hostname);
			     (void)fflush(stderr);
			  }

                          exit(0);


          /*--------------------------*/
          /* Restart watchdog timeout */
          /*--------------------------*/

          case SIGCONT:   

                          /*----------*/
                          /* Log kick */
                          /*----------*/

                          if (do_verbose == TRUE)
                          {  (void)fprintf(stderr,"softdog (%d@%s): kicked\n",getpid(),hostname);
                             (void)fflush(stderr);
                          }


                          /*--------------------------------------------------*/
                          /* Clear any further kicks which might have accrued */
                          /* while dealing with this kick                     */
                          /*--------------------------------------------------*/

                          (void)sigpending(&pending_set);

                          if(sigismember(&pending_set,SIGCONT))
                          {  sighandle.sa_handler = SIG_IGN; 
                             (void)sigaction(SIGCONT,&sighandle, (struct sigaction *)NULL);

                             (void)sigemptyset(&pending_set);
                             (void)sigaddset(&pending_set,SIGCONT);
                             (void)sigsuspend(&pending_set);

                             sighandle.sa_handler = (void *)sighandler;
                             (void)sigaction(SIGCONT,&sighandle, (struct sigaction *)NULL);
                          }


                          /*---------------*/
			  /* Reset softdog */
                          /*---------------*/

                          (void)alarm(softdog_timeout);

                          break;


          /*----------------------------------*/
          /* Kill monitored processi and exit */
          /*----------------------------------*/

          case SIGALRM:
		      
                          if (do_verbose == TRUE)
                          {  if (monitor_pgrp == (-1))
                                (void)fprintf(stderr,"\nsoftdog (%d@%s): terminating monitored process\n",getpid(),hostname);
                             else
                                (void)fprintf(stderr,"\nsoftdog (%d@%s): terminating monitored process group\n",getpid(),hostname);

                             (void)fflush(stderr);
                          }


			  /*------------------------------*/ 
			  /* Send signal to process group */
			  /*------------------------------*/

			  if (monitor_pgrp != (-1))
			  {  

                             /*-------------------------------------------*/
                             /* Exit if monitored process group is killed */
                             /*-------------------------------------------*/

                             if (killpg(monitor_pgrp,SIGKILL) == 0)
                             {  if (do_verbose == TRUE)
                                {  (void)fprintf(stderr,"softdog (%d@%s): exit (monitored process group terminated)\n\n",getpid(),hostname);
                                   (void)fflush(stderr);
                                }

                                exit(0);
                             }
                          }


			  /*------------------------*/
			  /* Send signal to process */
			  /*------------------------*/

			  else
                          {

                             /*-------------------------------------*/
                             /* Exit if monitored process is killed */
                             /*-------------------------------------*/

			     if (kill(monitor_pid, SIGKILL) == 0)
                             {  if (do_verbose == TRUE)
                                {  (void)fprintf(stderr,"softdog (%d@%s): exit (moninitored process terminated)\n\n",getpid(),hostname);
                                   (void)fflush(stderr);
                                }
                                exit(0);
                             }
                          }

          /*---------*/
          /* Default */
          /*---------*/

          default:        break;
    }

    return(0);
}



/*------------------*/          
/* Main entry point */
/*------------------*/

_PUBLIC int32_t main(int32_t argc, char *argv[])

{   sigset_t full_set,
             empty_set;

    uint32_t i,
             decoded = 0;


    /*----------*/
    /* Hostname */
    /*----------*/

    (void)gethostname(hostname,SSIZE);


    /*--------------------*/
    /* Parse command line */
    /*--------------------*/

    if (argc == 1 || strcmp(argv[1],"-usage") == 0 || strcmp(argv[1],"-help") == 0)
    {  (void)fprintf(stderr,"\nsoftdog version %s (gcc %s: built %s %s)\n\n",SOFTDOG_VERSION,__VERSION__,__TIME__,__DATE__);

       (void)fprintf(stderr,"SOFTDOG is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"SOFTDOG comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"usage: softdog [-usage] | [-help] |\n");
       (void)fprintf(stderr,"               [-verbose:FALSE]\n");
       (void)fprintf(stderr,"               !-monitorpid <pid>! | !monitorpg <pgrp>!\n");
       (void)fprintf(stderr,"               [-timeout <seconds:%04d>]\n\n",softdog_timeout);
       (void)fflush(stderr);

       exit(1);
    }

    for (i=1; i<argc; ++i)
    {

       /*--------------*/
       /* Verbose mode */
       /*--------------*/

       if (strcmp(argv[i],"-verbose") == 0)
       {  do_verbose = TRUE;
          ++decoded;
       }


       /*---------------------------------*/
       /* PGRP of monitored process group */
       /*---------------------------------*/

       if (strcmp(argv[i],"-monitorpg") == 0)
       {  if (i == argc - 1 || argv[i+1][0] == '-' || sscanf(argv[i+1],"%d",&monitor_pgrp) != 1 || monitor_pgrp < 0) 
          {  if (do_verbose == TRUE)
             {  (void)fprintf(stderr,"\nsoftdog (%d@%s): monitorpgrp [%s] must be a positive integer value\n\n",getpid(),hostname,argv[i+1]);
                (void)fflush(stderr);
             }

             exit(255);
          }

          ++i;
          decoded += 2;
       }


       /*--------------------------*/
       /* PID of monitored process */
       /*--------------------------*/

       if (monitor_pgrp == (-1) && strcmp(argv[i],"-monitorpid") == 0)
       {  if (i == argc - 1 || argv[i+1][0] == '-' || sscanf(argv[i+1],"%d",&monitor_pid) != 1 || monitor_pid < 0) 
          {  if (do_verbose == TRUE)
             {  (void)fprintf(stderr,"\nsoftdog (%d@%s): monitorpid [%s] must be a positive integer value\n\n",getpid(),hostname,argv[i+1]);
                (void)fflush(stderr);
             }

             exit(255);
          }

          ++i;
          decoded += 2;
       }


       /*-----------------*/
       /* Softdog timeout */
       /*-----------------*/

       if (strcmp(argv[i],"-timeout") == 0)
       {  if (i == argc - 1 || argv[i+1][0] == '-' || sscanf(argv[i+1],"%d",&softdog_timeout) != 1 || softdog_timeout < 0) 
          {  if (do_verbose == TRUE)
             {  (void)fprintf(stderr,"\nsoftdog (%d@%s): timeout must be a positive integer value\n\n",getpid(),hostname);
                (void)fflush(stderr);
             }

             exit(255);
          }

          ++i;
          decoded += 2;
       }
    }


    /*--------------------------------------------*/ 
    /* Check for unparsed command line parameters */
    /*--------------------------------------------*/ 
     
    if (decoded < argc - 1)
    {  if (do_verbose == TRUE)
       {  (void)fprintf(stderr,"\nsoftdog (%d@%s): command tail items unparsed (got %d, expected %d)\n\n",getpid(),hostname,decoded,argc - 1);
          (void)fflush(stderr);
       }

       exit(255);
    }


    /*--------*/
    /* Banner */
    /*--------*/

    if (do_verbose == TRUE)
    {  (void)fprintf(stderr,"\nsoftdog version %s (gcc %s: built %s %s)\n\n",SOFTDOG_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fflush(stderr);
    }

    /*-----------------------*/
    /* Setup signal handlers */
    /*-----------------------*/

    sigemptyset(&empty_set);
    sigfillset(&full_set);
    sighandle.sa_handler = (void *)sighandler;
    sighandle.sa_flags   = SA_RESTART;
    sighandle.sa_mask    = full_set;


    /*---------------------------------------------------*/
    /* Reset signal mask (as it is inherited from parent */
    /*---------------------------------------------------*/

    (void)sigprocmask(SIG_SETMASK,&empty_set,(sigset_t *)NULL);

    (void)sigaction(SIGINT, &sighandle, (struct sigaction *)NULL);
    (void)sigaction(SIGQUIT,&sighandle, (struct sigaction *)NULL);
    (void)sigaction(SIGTERM,&sighandle, (struct sigaction *)NULL);
    (void)sigaction(SIGALRM,&sighandle, (struct sigaction *)NULL);
    (void)sigaction(SIGCONT,&sighandle, (struct sigaction *)NULL);


    /*---------------*/
    /* Start softdog */
    /*---------------*/

    (void)alarm(softdog_timeout);


    /*---------------*/
    /* Watchdog loop */
    /*---------------*/

    while( TRUE )
	 (void)usleep(1000);


    /*-------------------*/
    /* We never get here */
    /*-------------------*/

    exit(0);
}
