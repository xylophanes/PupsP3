/*-----------------------------------*/
/* Looper payload (for testing)      */
/* M.A. O'Neill, Tumbling Dice, 2019 */
/*-----------------------------------*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>



/*---------------*/
/* Check signals */
/*---------------*/

#define SIGCHECK     SIGRTMIN + 11
#define SIGRESTART   SIGRTMIN + 12




/*-------------------*/
/* Dummy system call */
/*-------------------*/

int alarm_handler(int signum)

{
    (void)fprintf(stderr,"ALARM\n");
    (void)fflush(stderr);

    (void)alarm(2);
    (void)signal(SIGALRM,(void *)alarm_handler);

}




/*--------------------------*/
/* Restart any system calls */
/*--------------------------*/

int cont_handler(void)

{   

    (void)fprintf(stderr,"CHECKPOINT\n");
    (void)fflush(stderr);

    (void)usleep(1000);


    (void)fprintf(stderr,"RESTART\n");
    (void)fflush(stderr);

    (void)alarm(2);
    return(0);
}



/*------------------*/
/* main entry point */
/*------------------*/

int main(int argc, char *argv[])

{  int iter = 0;


   /*-------------------*/
   /* Schedule an alarm */
   /*-------------------*/

   (void)alarm(2);


   /*----------------*/
   /* Handle SIGARLM */
   /*----------------*/

   (void)signal(SIGALRM,   (void *)alarm_handler);


   /*-----------------------------*/
   /* Handle (checkpoint) restart */
   /*-----------------------------*/

   (void)signal(SIGCONT,   (void *)cont_handler);
   (void)signal(SIGRESTART,(void *)cont_handler);


   /*-----------*/
   /* Main loop */
   /*-----------*/

   while(1)
   {    (void)fprintf(stderr,"iteration %d \n",iter++);
        (void)fflush(stderr);

        (void)sleep(1);
   }
}

