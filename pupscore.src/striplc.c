/*-----------------------------------------------------------------------------------------
    Purpose: strips the last character from input string.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 1.05
    Dated:   4th January 2023
    E-mail:  mao@tumblingdice.co.uk
-----------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/*--------------------*/
/* Version of striplc */
/*--------------------*/

#define STRIPLC_VERSION    "1.05"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE              2048 


/*------------------*/
/* Main entry point */
/*------------------*/

int main(int argc, char *argv[])

{   char name[SSIZE] = "";
 
    if(argc > 1)
    {  (void)fprintf(stderr,"\nstriplc version %s, (C) Tumbling Dice 2003-2023 (built %s %s)\n\n",STRIPLC_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"STRIPLC is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"STRIPLC comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fprintf(stderr,"\nUsage: striplc [-usage | -help]\n\n");
       (void)fflush(stderr);

       exit(255);
    }

    (void)fgets(name,SSIZE,stdin);
    name[strlen(name) - 2] = '\0';
    (void)fputs(name,stdout);
    (void)fflush(stdout);

    exit(0);
}
