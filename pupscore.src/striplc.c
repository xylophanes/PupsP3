/*------------------------------------------------------
    Purpose: strips the last character from input string

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 1.06
    Dated:   2nd February 2024
    E-mail:  mao@tumblingdice.co.uk
-----------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>


/*---------*/
/* Defines */
/*---------*/
/*---------*/
/* Version */
/*---------*/

#define STRIPLC_VERSION    "1.06"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE              2048 


/*------------------*/
/* Main entry point */
/*------------------*/

int32_t main(int32_t argc, char *argv[])

{   char name[SSIZE] = "";


    /*--------------------*/
    /* Parse command line */
    /*--------------------*/
 
    if(argc > 1)
    {  (void)fprintf(stderr,"\nstriplc version %s, (C) Tumbling Dice 2003-2024 (gcc %s: built %s %s)\n\n",STRIPLC_VERSION,__VERSION__,__TIME__,__DATE__);
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
