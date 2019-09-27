/*---------------------------------------------------------------------------------------
    Purpose: tty homeostat - recreates /dev/tty if something has destroyed it

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.00 
    Dated:   30th August 2019 
    E-mail:  mao@tumblingdice.co.uk
---------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/sysmacros.h>


/*------------------*/
/* Version of mktty */
/*------------------*/

#define MKTTY_VERSION   "1.08"


/*-------------------*/
/* Local definitions */
/*-------------------*/

#define _PUBLIC
#define _BOOLEAN short int
#define FALSE 0
#define TRUE  255


/*------------------*/
/* Main entry point */
/*------------------*/

_PUBLIC int main(int argc, char *argv[])

{   _BOOLEAN verbose = FALSE;

    errno = 0;

    /*---------------*/
    /* Display usage */
    /*---------------*/

    if(argc == 2)
    {  if(strcmp(argv[1],"-verbose") == 0)
          verbose = TRUE;
       else
       {  (void)fprintf(stderr,"\nmktty version %s, (C) Tumbling Dice 2004-2019 (built %s)\n\n",MKTTY_VERSION,__TIME__,__DATE__);
          (void)fprintf(stderr,"MKTTY is free software, covered by the GNU General Public License, and you are\n");
          (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
          (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
          (void)fprintf(stderr,"MKTTY comes with ABSOLUTELY NO WARRANTY\n\n");

          (void)fprintf(stderr,"usage: mktty [-verbose:FALSE]\n\n");
          (void)fflush(stderr);

          exit(1);
       }
    }
    else if(argc > 2)
    {  (void)fprintf(stderr,"\nmktty version %s, (C) Tumbling Dice 2004-2018 (built %s %s)\n\n",MKTTY_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"MKTTY is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"MKTTY comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"usage: mktty [-verbose:FALSE]\n\n");
       (void)fflush(stderr);

       exit(1);
    }


    /*-----------------------------------------*/
    /* Check to see if /dev/tty already exists */
    /*-----------------------------------------*/

    if(access("/dev/tty",F_OK | R_OK | W_OK) == 0)
    {  if(verbose == TRUE)
       {  (void)fprintf(stderr,"mktty: \"/dev/tty\" already exists\n");
          (void)fflush(stderr);
       }

       exit(-1);
    }


    /*------------------------------------------*/ 
    /* /dev/tty has disappeared -- recreate it! */
    /*------------------------------------------*/ 

    (void)mknod("/dev/tty",S_IFCHR,makedev(5,0));
    (void)chmod("/dev/tty",0666);

    exit(0);
}

