/*------------------------------------------------------------------
     Purpose: Upcase a string.

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.00 
     Dated:   30th Agust 2019 
     e-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>



/*-------------------*/
/* Version of upcase */
/*-------------------*/

#define UPCASE_VERSION    "2.00"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE             2048 


/*------------------*/
/* Main entry point */
/*------------------*/

int main(int argc, char *argv[])

{   int  i;
    char upcased[SSIZE] = "";

    if(argc != 2)
    {  (void)fprintf(stderr,"\nupcase version %s, (C) Tumbling Dice 2002-2019 (built %s %s)\n\n",UPCASE_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"UPCASE is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"UPCASE comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fprintf(stderr,"Usage: upcase <string>\n");
       (void)fflush(stderr);

       exit(-1);
    }

    for(i=0; i<strlen(argv[1]); ++i)
       upcased[i] = toupper(argv[1][i]);

    upcased[i] = '\0';
    (void)fprintf(stdout,"%s",upcased);
    (void)fflush(stdout);

    exit(0);
}
