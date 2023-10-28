/*------------------------------------------------------------------
     Purpose: Downcase a string. 

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.00 
     Dated:   4th January 2023
     E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/*---------------------*/
/* Version of downcase */
/*---------------------*/

#define DOWNCASE_VERSION    "2.00"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE             2048



int main(int argc, char *argv[])

{   int  i;
    char downcased[SSIZE] = "";

    if(argc != 2)
    {  (void)fprintf(stderr,"\ndowncase version %s, (C) Tumbling Dice 2002-2023 (built %s %s)\n\n",DOWNCASE_VERSION,__TIME__,__DATE__);
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
