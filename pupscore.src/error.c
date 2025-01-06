/*----------------------------------------------------------------------
     Purpose: Display error message associated with a given error number 

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 1.07
     Dated:   10th December 2024
     E-mail:  mao@tumblingdice.co.uk
---------------------------------------------------------------------*/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>


/*------------------*/
/* Version of error */
/*------------------*/

#define ERROR_VERSION    "1.07"


/*------------------*/
/* Main entry point */
/*------------------*/

int32_t main(int32_t argc, char *argv[])

{

    /*--------------------*/
    /* Parse command line */
    /*--------------------*/

    if(argc != 2)
    {  (void)fprintf(stderr,"\nError number to code tranlator version %s, (C) Tumbling Dice, 2002-2024 (gcc %s: built %s %s)\n",ERROR_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fprintf(stderr,"\nUsage: error <numeric error code>\n\n");
       (void)fprintf(stderr,"ERROR is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"ERROR comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fflush(stderr);
 
       exit(255);
    }

    (void)sscanf(argv[1],"%d",&errno);
    perror("error");

    exit(0);
} 
