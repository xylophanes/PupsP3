/*----------------------------------
     Purpose: Upcase a string

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.02 
     Dated:   11th December 2024
     e-mail:  mao@tumblingdice.co.uk
----------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>


/*---------*/
/* Defines */
/*---------*/
/*---------*/
/* Version */
/*---------*/

#define UPCASE_VERSION    "2.02"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE             2048 


/*------------------*/
/* Main entry point */
/*------------------*/

int32_t main(int32_t argc, char *argv[])

{   uint32_t i;
    char     upcased[SSIZE] = "";


    /*--------------------*/
    /* Parse command line */
    /*--------------------*/

    if(argc != 2)
    {  (void)fprintf(stderr,"\nupcase version %s, (C) Tumbling Dice 2002-2024 (gcc %s: built %s %s)\n\n",UPCASE_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fprintf(stderr,"UPCASE is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"UPCASE comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fprintf(stderr,"Usage: upcase <string>\n");
       (void)fflush(stderr);

       exit(255);
    }

    for(i=0; i<strlen(argv[1]); ++i)
       upcased[i] = toupper(argv[1][i]);

    upcased[i] = '\0';
    (void)fprintf(stdout,"%s",upcased);
    (void)fflush(stdout);

    exit(0);
}
