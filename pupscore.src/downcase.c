/*----------------------------------
     Purpose: Downcase a string. 

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.02 
     Dated:   10th October 2024
     E-mail:  mao@tumblingdice.co.uk
----------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>


/*---------------------*/
/* Version of downcase */
/*---------------------*/

#define DOWNCASE_VERSION  "2.02"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE             2048


/*------------------*/
/* Main entry point */
/*------------------*/

int32_t main(int32_t argc, char *argv[])

{   ssize_t i;
    char    downcased[SSIZE] = "";


    /*--------------------*/
    /* Parse command line */
    /*--------------------*/

    if(argc != 2)
    {  (void)fprintf(stderr,"\ndowncase version %s, (C) Tumbling Dice 2002-2024 (gcc %s: built %s %s)\n\n",DOWNCASE_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fprintf(stderr,"DOWNCASE is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"DOWNCASE comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fprintf(stderr,"Usage: downcase <string>\n");
       (void)fflush(stderr);

       exit(255);
    }

    for(i=0; i<strlen(argv[1]); ++i)
       downcased[i] = tolower(argv[1][i]);

    downcased[i] = '\0';
    (void)fprintf(stdout,"%s",downcased);
    (void)fflush(stdout);

    exit(0);
}
