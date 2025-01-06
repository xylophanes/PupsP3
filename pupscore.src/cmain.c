/*-----------------------------------------
    Purpose: main for a PUPS/P3 application 

    Author: M.A. O'Neill
            Tumbling Dice
            Gosforth
            Tyne and Wear
            NE3 4RT

    Version: 2.00 
    Dated:   10th December 2024
    E-mail:  mao@tumblingdice.co.uk 
-----------------------------------------*/


#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <bubble.h>
#include <errno.h>
#include <xtypes.h>
#include <setjmp.h>

#ifdef PTHREAD_SUPPORT
#include <pthread.h>
#endif /* PTHREAD_SUPPORT */


/*-------------------*/
/* Local definitions */
/*-------------------*/

#define CRC_SIZE 1024
#define SSIZE    2048 


/*--------------------*/
/* Imported functions */
/*--------------------*/

_IMPORT int32_t   pups_main(); 


/*--------------------*/
/* Imported variables */
/*--------------------*/

#ifdef PTHREAD_SUPPORT
_IMPORT pthread_t appl_root_tid;
#endif /* THREAD_SUPPORT */

#ifdef BUBBLE_MEMORY_SUPPORT
_IMPORT _BOOLEAN initialised;
#endif /* BUBBLE_MEMORY_SUPPORT */



/*--------------------*/
/* Exported variables */
/*--------------------*/

_PUBLIC int32_t _bubble_lang_flag = 0;
_PUBLIC jmp_buf appl_resident_restart;




/*------------------*/
/* Main entry point */
/*------------------*/

_PUBLIC int32_t main(int argc, char **argv, char **envp)
{   des_t bin_des   = (-1);

    off_t pos,
          size;

    char  **eff_argv  = (char **)NULL,
          itag[SSIZE] = "";

    struct stat stat_buf;

    #ifdef PTHREAD_SUPPORT
    appl_root_tid = pthread_self();
    #endif /* PTHREAD_SUPPORT */

    #ifdef BUBBLE_MEMORY_SUPPORT
    initialised = TRUE;
    #endif /* BUBBLE_MEMORY_SUPPORT */



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

    return pups_main(argc, eff_argv, envp); // was pups_main
}
