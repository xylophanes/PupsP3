/*------------------------------------------------------
    Purpose: Test to see if input is IEEE floating point 

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.03 
    Dated:   10th December 2024
    E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ftype.h>
#include <xtypes.h>
#include <stdint.h>


/*---------*/
/* Defines */
/*---------*/
/*---------*/
/* Version */
/*---------*/

#define ISFLOAT_VERSION    "2.03"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE              20248


/*------------------*/
/* Main entry point */
/*------------------*/

_PUBLIC int32_t main(int32_t argc, char *argv[])

{   FTYPE fdum;
    char  strdum[SSIZE] = "";


    /*--------------------*/
    /* Parse command line */
    /*--------------------*/

    if(argc != 2)
    {  (void)fprintf(stderr,"\nisfloat version %s, (C) Tumbling Dice 2000-2024 (gcc %s: built %s %s)\n\n",ISFLOAT_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fprintf(stderr,"ISFLOAT is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"ISFLOAT comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nUsage: isfloat <string>\n\n");
       (void)fflush(stderr);

       exit(255);
    }

    if(sscanf(argv[1],"%F",&fdum) != 1)
    {  (void)printf("FALSE");
       (void)fflush(stdout);
    }
    else
    {  (void)printf("TRUE");
       (void)fflush(stdout);
    }

    exit(0);
}
