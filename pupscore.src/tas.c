/*-------------------------------------------------------------
     Purpose: tests for a lock file in a directory which can be
              accessed by multiple processes.

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.02 
     Dated:   11th December 2024
     e-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------*/

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <bsd/string.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdint.h>


/*---------*/
/* Defines */
/*---------*/
/*---------*/
/* Version */
/*---------*/

#define TAS_VERSION            "2.02"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE                   2048 


/*-----------------*/
/* Local Variables */
/*-----------------*/

static int32_t  lock_pid      = (-1);
static int32_t  n_args        = (-1);
static char lock_name[SSIZE]  = "";




/*---------------------*/
/* Remove lock on exit */
/*---------------------*/

static int32_t exit_handler(const  int32_t signum)

{

    /*----------------------------------------------------*/
    /* Lock intelligently -- if we lose the process which */
    /* asked for the lock -- delete lock                  */
    /*----------------------------------------------------*/

    if(n_args == 3)
    {  while(1)
       {   if(kill(lock_pid,SIGCONT) == (-1))
           {  if(access(lock_name,F_OK | R_OK | W_OK) != (-1))
                 (void)unlink(lock_name);
           }
       }
   }

   exit(0);
}



/*------------------*/
/* Main entry point */
/*------------------*/

int32_t main(int32_t argc, char *argv[])

{   int32_t entered = 0;


    /*--------------------*/
    /* Parse command line */
    /*--------------------*/

    if(argc != 2 || strcmp(argv[1],"-usage") == 0 || strcmp(argv[1],"-help") == 0)
    {  (void)fprintf(stderr,"\ntas version %s, (C) Tumbling Dice 2002-2024 (gcc %s: built %s %s)\n\n",TAS_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fprintf(stderr,"TAS is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"TAS comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fprintf(stderr,"\nUsage: tas [-usage | -help] | <lock file name> [<lock PID>]\n\n");
       (void)fflush(stderr);

       exit(255);
    }

    (void)strlcpy(lock_name,argv[1],SSIZE);
    if(argc == 3)
    {  if(sscanf(argv[2],"%d",&lock_pid) != 1)
       {  (void)fprintf(stderr,"tas: expecting lock PID\n");
          (void)fflush(stderr);

          exit(255);
       }

       (void)signal(SIGTERM,(void *)&exit_handler);
       (void)signal(SIGINT, (void *)&exit_handler);
       (void)signal(SIGQUIT,(void *)&exit_handler);

       n_args = argc;
    }

    while(mkdir(lock_name,0700) == (-1))
    {    if(entered == 0)
         {  entered = 1;

            (void)fprintf(stderr,"\ntas: lock (%s) is in use\n\n",lock_name);
            (void)fflush(stderr);
         }

         sleep(1);
    }

    (void)fprintf(stderr,"\n    tas: got lock (%s)\n\n",lock_name);
    (void)fflush(stderr);


    /*----------------------------------------------------*/
    /* Lock  int32_telligently -- if we lose the process which */
    /* asked for the lock -- delete lock                  */
    /*----------------------------------------------------*/

    if(argc == 3)
    {  while(1)
       {   if(kill(lock_pid,SIGCONT) == (-1))
   	   {  if(access(lock_name,F_OK | R_OK | W_OK) != (-1))
	         (void)unlink(lock_name);
	
              exit(0);
	   }

	   (void)usleep(100);
       }
    }

    exit(0);
}
