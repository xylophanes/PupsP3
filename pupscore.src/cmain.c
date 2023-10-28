/*-------------------------------------------------------------------------------------------------
    Purpose: main for a PUPS/P3 application 

    Author: M.A. O'Neill
            Tumbling Dice
            Gosforth
            Tyne and Wear
            NE3 4RT

    Version: 2.00 
    Dated:   4th January 2023
    E-mail:  mao@tumblingdice.co.uk 
-------------------------------------------------------------------------------------------------*/


#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <bubble.h>
#include <errno.h>
#include <xtypes.h>
#include <setjmp.h>


/*-------------------*/
/* Local definitions */
/*-------------------*/

#define CRC_SIZE 1024
#define SSIZE    2048 

#ifdef THREAD_SUPPORT
_PUBLIC pthread_t root_tid;
#endif /* THREAD_SUPPORT */


/*--------------------*/
/* Imported variables */
/*--------------------*/

_IMPORT _BOOLEAN initialised;


/*-------------------*/
/* Exported vaiables */
/*-------------------*/

_PUBLIC int     _bubble_lang_flag = 0;
_PUBLIC jmp_buf appl_resident_restart;


/*------------------*/
/* Main entry point */
/*------------------*/

_PUBLIC int main(int argc, char **argv, char **envp)
{    int   bin_des   = (-1);

     off_t pos,
           size;

     char  **eff_argv  = (char **)NULL,
           itag[SSIZE] = "";

     struct stat stat_buf;

     #ifdef THREAD_SUPPORT
     _root_tid = pthread_self();
     #endif /* THREAD_SUPPORT */

    initialised = TRUE;


    /*--------------------------------------------------*/
    /* Re-entry point for a memory resident application */
    /*--------------------------------------------------*/

    if(setjmp(appl_resident_restart) > 0)
    {

       /*--------------------------------*/
       /* Read argument vector from FIFO */
       /*--------------------------------*/


    }


    /*-----------------------------*/
    /* Non memory resident process */
    /*-----------------------------*/

    else

       /*----------------------*/
       /* Copy argument vector */
       /*----------------------*/

       eff_argv = argv;


    #ifdef BUBBLE_MEMORY_SUPPORT
    return bubble_target(argc, eff_argv, envp);
    #else
    return pups_main(argc, eff_argv, envp);
    #endif /* BUBBLE_MEMORY_SUPPORT */
}
