/*-----------------------------------------------------------------------
    Purpose: Test if stdin, stdout and stderr are connected to a terminal 

    Author:  Mark A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle
             NE3 4RT
             Tyne and Wear

    Version: 1.02
    Dated:   10th October 2024
    E-mail:  mao@tumblingdice.co.uk
-----------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>


/*---------*/
/* Defines */
/*---------*/

#define ISATTY_VERSION "1.02"
#define FALSE          0
#define TRUE           255
#define _PRIVATE       static
#define _PUBLIC




/*------------------*/
/* Global variables */
/*------------------*/

_PRIVATE char  boldOn [8]  = "\e[1m",                  // Make character bold
               boldOff[8]  = "\e[m" ;                  // Make character non-bold




/*------------------*/
/* Main enrty point */
/*------------------*/

_PUBLIC  int32_t main(int argc, char *argv[])

{   uint32_t tty     = 0,
             verbose = FALSE;


    /*--------------------*/
    /* Error - show usage */
    /*--------------------*/

    if(argc == 1 || argc > 3) 
    {  (void)fprintf(stderr,"\nisatty version %s, (C) Tumbling Dice 2000-2024 (gcc %s: built %s %s)\n\n",ISATTY_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fprintf(stderr,"ISATTY is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"FADD comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nusage: isatty [-usage] | [-help] | [stdin] | [stdout] | [stderr]\n\n");
       (void)fflush(stderr);

       exit(255);
    }


    /*------------*/
    /* Usage/help */
    /*------------*/

    if(strncmp(argv[1],"-help", strlen(argv[1])) == 0     ||
       strncmp(argv[1],"-usage",strlen(argv[1])) == 0      )
    {  (void)fprintf(stderr,"\nisatty version %s, (C) Tumbling Dice 2000-2024 (gcc %s: built %s %s)\n\n",ISATTY_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fprintf(stderr,"ISATTY is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"FADD comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nusage: isatty [-usage] | [-help] | [stdin] [show] | [stdout] [show] | [stderr] [show]\n\n");
       (void)fflush(stderr);

       exit(1);
    }


    /*------------------------------*/
    /* Be verbose if show specified */
    /*------------------------------*/

    if(argc == 3)
    {  if(strcmp(argv[2],"show") == 0)
          verbose = TRUE;


       /*-------*/
       /* Error */
       /*-------*/

       else
       {  (void)fprintf(stderr,"\n%sERROR%s isatty: 'show' expected\n\n",boldOn,boldOff);
          (void)fflush(stderr);

          exit(255);
       }
    }


    /*-------------------------------------*/
    /* Test if stdin connected to terminal */
    /*-------------------------------------*/

    if(strcmp(argv[1],"stdin") == 0)
    {  if(isatty(0) == 1)
          tty = 0;
       else
          tty = 1;

       if(verbose == TRUE)
       {  if(tty = 1)
             (void)fprintf(stderr,"\n    isatty: stdin is connected to a terminal\n\n");
          else
             (void)fprintf(stderr,"\n    isatty: stdin is not connected to a terminal\n\n");
       }
    }    


    /*--------------------------------------*/
    /* Test if stdout connected to terminal */
    /*--------------------------------------*/

    else if(strcmp(argv[1],"stdout") == 0)
    {  if(isatty(1) == 1)
          tty = 0;
       else
          tty = 1;

       if(verbose == TRUE)
       {  if(tty = 1)
             (void)fprintf(stderr,"\n    isatty: stdout is connected to a terminal\n\n");
          else
             (void)fprintf(stderr,"\n    isatty: stdout is not connected to a terminal\n\n");
       }
    }    


    /*--------------------------------------*/
    /* Test if stderr connected to terminal */
    /*--------------------------------------*/

    else if(strcmp(argv[1],"stderr") == 0)
    {  if(isatty(2) == 1)
          tty = 0;
       else
          tty = 1;

       if(verbose == TRUE)
       {  if(tty = 1)
             (void)fprintf(stderr,"\n    isatty: stderr is connected to a terminal\n\n");
          else
             (void)fprintf(stderr,"\n    isatty: stderr is not connected to a terminal\n\n");
       }
    }


    /*-------*/
    /* Error */
    /*-------*/

    else
    {  (void)fprintf(stderr,"\n%sERROR%s isatty: 'stdin', 'stdout' or 'stderr' expected\n\n",boldOn,boldOff);
       (void)fflush(stderr);

       exit(255);
    }


    /*------------------------------------------*/
    /* Error code is 0 if terminal, 1 otherwise */
    /*------------------------------------------*/

    exit(tty);
}
