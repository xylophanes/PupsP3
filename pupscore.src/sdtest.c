/*----------------------------------
     Purpose: Test/debug for softdog

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 1.02
     Dated:   11th December 2024
     E-mail:  mao@tumblingdice.co.uk
----------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>


/*---------*/
/* Defines */
/*---------*/

#define SDTEST_VERSION  "1.02"
#define SSIZE           2048


/*------------------*/
/* Global variables */
/*------------------*/

static pid_t pid = (-1);


/*-------------------------*/
/* Terminate process group */
/*-------------------------*/

static int32_t term_handler(const int32_t signum)

{    (void)killpg(pid,SIGTERM);
     exit(255);
}


/*------------------------------------*/
/* Simulate problem. Stop feeding     */
/* watchdog and wait to be terminated */
/*------------------------------------*/

static  int32_t wait_for_termination(const  int32_t signum)
{
    (void)fprintf(stderr,"    sdtest: **** waiting for termination\n");
    (void)fflush(stderr);

    while (1)
          (void)usleep(10000);
}


/*------------------*/
/* Main entry point */
/*------------------*/

int32_t main(int32_t argc, char *argv[])

{   pid_t softdog_pid = (-1);

 
    /*--------*/
    /* Banner */
    /*--------*/

    (void)fprintf(stderr,"\nsoftdog tester version %s, (C) Tumbling Dice, 2024 (gcc %s: built %s %s)\n\n",SDTEST_VERSION,__VERSION__,__TIME__,__DATE__);

    (void)fprintf(stderr,"LYOSOME is free software, covered by the GNU General Public License, and you are\n");
    (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
    (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
    (void)fprintf(stderr,"LYOSOME comes with ABSOLUTELY NO WARRANTY\n\n");
 
    (void)fflush(stderr);


    /*--------------------------*/
    /* Pid of this test process */
    /*--------------------------*/

    pid = getpid();
    (void)fprintf(stderr,"    sdtest: pid is %d\n",pid);
    (void)fflush(stderr);


    /*----------------------*/
    /* Process group leader */
    /*----------------------*/

    (void)setsid();


    /*-----------------*/
    /* Signal handlers */
    /*-----------------*/

    (void)signal(SIGTERM, (void *)&term_handler);
    (void)signal(SIGUSR1, (void *)&wait_for_termination);


    /*---------------*/
    /* Start softdog */
    /*---------------*/


    if ((softdog_pid = fork()) == 0)
    {  unsigned char pidstr[SSIZE]  = "";

         
       /*-------*/
       /* Child */
       /*-------*/

       (void)fprintf(stderr,"    sdtest: starting softdog\n");
       (void)fflush(stderr);

       (void)snprintf(pidstr,SSIZE,"%d",pid);
       (void)execlp("softdog","softdog","-verbose","-monitorpid",pidstr,"-timeout","10", (char *)NULL);


       /*---------------------*/
       /* Should not get here */
       /*---------------------*/

       (void)fprintf(stderr,"    sdtest: unpexpected child exit\n");
       (void)fflush(stderr);

       _exit(255);
    } 


    /*--------------------------------*/
    /* Keep feeding specified softdog */
    /* process every 10 milliseconds  */
    /*--------------------------------*/

    (void)fprintf(stderr,"    sdtest: softdog pid is %d\n",softdog_pid);
    (void)fflush(stderr);

    while (1)
    {   (void)kill(softdog_pid,SIGCONT);
 
        (void)fprintf(stderr,"\n    sdtest: kick softdog %d\n\n",softdog_pid);
        (void)fflush(stderr);

        (void)sleep(1);
    }

    exit(0);
}
