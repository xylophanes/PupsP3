/*-------------------------------------------------*/
/* Check criu functionality                        */
/* M.A. O'Neill, Tumbling Dice, 10th December 2024 */
/*-------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <sched.h>
#include <time.h>
#include <stdint.h>


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE               2048 


/*------------------*/
/* Global variables */
/*------------------*/

pid_t    pid                  = (-1);
int32_t ssave_delay           = 10;
char    checkpoint_dir[SSIZE] = "";




/*---------------------------------------*/
/* Signal handler for SIGTERM and SIGINT */
/*---------------------------------------*/

static int32_t term_handler(int32_t signum)

{   char rm_cmd[SSIZE] = "";

    (void)snprintf(rm_cmd,SSIZE,"rm -rf %s",checkpoint_dir);
    (void)system(rm_cmd);

    exit(0);
}




/*----------------------------------*/
/* (Alarm) handler for state saving */
/*----------------------------------*/

static int32_t alarm_handler(int32_t signum)

{   char   criu_cmd[SSIZE] = "";
    struct timespec delay;

    (void)fprintf(stderr,"STATE SAVE\n");
    (void)fflush(stderr);


    /*----------------------------------------*/
    /* Run criu command to checkpoint process */
    /*----------------------------------------*/

    (void)snprintf(criu_cmd,SSIZE,"criu --shell-job dump -D %s -t %d &",checkpoint_dir,pid);
    (void)system(criu_cmd);


    /*------------------------------------------*/
    /* This is a dummy system call - on restore */
    /* EINTR will be generated                  */
    /*------------------------------------------*/

    delay.tv_sec  = 60;
    delay.tv_nsec = 0;

    (void)nanosleep(&delay,(struct timespec *)NULL);

    (void)fprintf(stderr,"RESTART\n");
    (void)fflush(stderr);


    /*------------------*/
    /* Reschedule alarm */
    /* is restarted     */
    /*------------------*/

    (void)alarm(ssave_delay);
    return(0);
}




/*------------------*/
/* Main entry point */
/*------------------*/

int32_t main(int32_t argc, char *argv[])

{   uint32_t          iter  = 0;
    struct   timespec nsleep;


    /*-------------------------------*/
    /* Get iteration for (Criu) dump */
    /*-------------------------------*/

    if(argc > 2)
       exit(255);
    else if(argc == 2)
    {  if(sscanf(argv[1],"%d",&ssave_delay) != 1 || ssave_delay <= 0)
          exit(255);
    }

    pid = getpid();

    (void)snprintf(checkpoint_dir,SSIZE,"criu.%d.checkpoint",pid);
    (void)mkdir(checkpoint_dir,0700);


    /*------------------------*/
    /* Set up signal handlers */
    /*------------------------*/

    (void)signal(SIGINT, (void *)&term_handler);
    (void)signal(SIGTERM,(void *)&term_handler);
    (void)signal(SIGALRM,(void *)&alarm_handler);


    /*-----------*/
    /* Main loop */
    /*-----------*/

    (void)alarm(ssave_delay);
    while(1)
    {   (void)fprintf(stderr,"Iteration: %d\n",iter);
        (void)fflush(stderr);

        ++iter;

        nsleep.tv_sec  = 1;
        nsleep.tv_nsec = 0;
        (void)nanosleep(&nsleep,(struct timespec *)NULL);
    }

    exit(0);
}
