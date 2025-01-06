/*------------------------------------------------------
     Purpose: Relay SIGHUP as SIGTERM (to payload child) 

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.02 
     Dated:   10th December 2024
     e-mail:  mao@tumblingdice.co.uk
-----------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <xtypes.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdint.h>


/*---------*/
/* Defines */
/*---------*/
/*-------------------*/
/* Version of hupter */
/*-------------------*/

#define HUPTER_VERSION    "2.02"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE             2048 


/*-----------------*/
/* Local variables */
/*-----------------*/

_PRIVATE  int32_t child_pid;
_PRIVATE  int32_t relay_signal = SIGTERM;



/*---------------------------------------*/
/* Relay -- send signal to process group */
/*---------------------------------------*/

_PRIVATE int32_t relay(const  int32_t signum)
{

    /*--------------------------------*/
    /* Relay the child process itself */ 
    /*--------------------------------*/

    (void)kill(child_pid,SIGINT);


    /*-------------------------------------*/
    /* Relay the payload (executed via sh) */ 
    /*-------------------------------------*/

    (void)kill(-(child_pid+1),SIGINT);
    exit (0);
}



/*------------------*/
/* Main entry point */
/*------------------*/

_PUBLIC  int32_t main(int argc, char *argv[])
{   char payload_cmd[SSIZE]  = "";

    int32_t  status,
             cmd_index       = 1;


    /*--------------------*/ 
    /* Parse command line */
    /*--------------------*/

    if(argc == 1 || (argc != 3 && (strcmp(argv[1],"-help") == 0 || strcmp(argv[1],"-help") == 0)))
    {  (void)fprintf(stderr,"\nhupter version %s, (C) Tumbling Dice 2003-2024 (gcc %s: built %s %s)\n\n",HUPTER_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fprintf(stderr,"HUPTER is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"HUPTER comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fprintf(stderr,"\nUsage: hupter [-usage | -help] | [-hup | -int | -term] [command]\n\n");
       (void)fflush(stderr);

       exit(255);
    }

    if(argc == 2)
    {  relay_signal = SIGTERM;
       cmd_index = 1;
    }
    else if(strcmp(argv[1],"-hup") == 0)
    {  relay_signal = SIGHUP;
       cmd_index = 2;
    }
    else if(strcmp(argv[1],"-int") == 0)
    {  relay_signal = SIGINT;
       cmd_index = 2;
    }
    else if(strcmp(argv[1],"-term") == 0)
    {  relay_signal = SIGTERM;
       cmd_index = 2;
    }
    else
    {  char hostname[SSIZE] = "";

       (void)gethostname(hostname,SSIZE);
       (void)fprintf(stderr,"\nhupter (%d@%s): expecting \"-hup\", \"-int\" or \"-term\"\n\n",getpid(),hostname);
       (void)fflush(stderr);

       exit(255);
    }


    /*------------------------------------*/
    /* Must run hupter in its own session */
    /*------------------------------------*/

    (void)setsid();


    /*-----------------------------------------------*/
    /* Signals to be relayed to payload (as SIGTERM) */
    /*-----------------------------------------------*/

    (void)signal(SIGHUP, (void *)&relay);
    (void)signal(SIGQUIT,(void *)&relay);
    (void)signal(SIGINT, (void *)&relay);
    (void)signal(SIGTERM,(void *)&relay);

    (void)snprintf(payload_cmd,SSIZE,"exec %s",argv[cmd_index]);
   
    if((child_pid = fork()) == 0)
    {  (void)system(payload_cmd);
       _exit(255);
    }


    /*------------------------*/
    /* Wait for child to exit */
    /*------------------------*/

    (void)waitpid(child_pid,&status,0);
    exit(0);
}
