/*-----------------------------------*/
/* Check criu functionality          */
/* M.A. O'Neill, Tumbling Dice, 2019 */
/*-----------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <sched.h>
#include <time.h>


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE 2048 


/*------------------*/
/* Global variables */
/*------------------*/

char checkpoint_dir[SSIZE] = "";


/*---------------------------------------*/
/* Signal handler for SIGTERM and SIGINT */
/*---------------------------------------*/

int term_handler(int signum)

{   char rm_cmd[SSIZE] = "";

    (void)snprintf(rm_cmd,SSIZE,"rm -rf %s",checkpoint_dir);
    (void)system(rm_cmd);

    exit(0);
}


/*------------------------*/
/* Handle (regular) alarm */
/*------------------------*/

int alarm_handler(int signum)

{   (void)fprintf(stderr,"ALARM\n");
    (void)fflush(stderr);

    (void)alarm(1);
    return(0);
}




/*------------------*/
/* Main entry point */
/*------------------*/

int main(int argc, char *argv[])

{   int  pid,
         iter     = 0,
         dumpiter = 10;

    char criu_cmd[SSIZE] = "";


    /*-------------------------------*/
    /* Get iteration for (Criu) dump */
    /*-------------------------------*/

    if(argc > 2)
       exit(-1);
    else if(argc == 2)
    {  if(sscanf(argv[1],"%d",&dumpiter) != 1 || dumpiter <= 0)
          exit(-1);
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

    (void)alarm(1);
    while(1)
    {   (void)fprintf(stderr,"Iteration: %d\n",iter);
        (void)fflush(stderr);

        ++iter;


        /*-----------------*/
        /* Auto checkpoint */
        /*-----------------*/

        if(iter == dumpiter)
        {  struct timespec delay,
                           remainder; 

           dumpiter += 10; 


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

           (void)fprintf(stderr,"DUMPITER is now %d\n",dumpiter);
           (void)fflush(stderr);


           /*-------------------------------------------------*/
           /* Restart alarm (as it will expire before process */
           /* is restarted                                    */
           /*-------------------------------------------------*/

           (void)alarm(1);
        }

        sleep(1);
    }

    exit(0);
}
