/*------------------------------------------
    Purpose: Test to see if input is integer

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.02 
    Dated:   10th Decemeber 2024
    E-mail:  mao@tumblingdice.co.uk
------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <xtypes.h>
#include <stdint.h>


/*---------*/
/* Defines */
/*---------*/
/*---------*/
/* Version */
/*---------*/

#define ISINT_VERSION  "2.02" 


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE          2048


/*------------------*/
/* Main entry point */
/*------------------*/

_PUBLIC  int32_t main(int argc, char *argv[])

{   int32_t idum;
    char    strdum[SSIZE] = "";


    /*--------------------*/
    /* Parse command line */
    /*--------------------*/

    if(argc != 2)
    {  (void)fprintf(stderr,"\nisint version %s, (C) Tumbling Dice 2000-2024 (gcc %s: built %s %s)\n\n",ISINT_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fprintf(stderr,"ISINT is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"ISINT comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nUsage: isint <string>\n\n");
       (void)fflush(stderr);

       exit(255);
    }

    
    if(sscanf(argv[1],"%d",&idum) != 1)
    {  (void)printf("FALSE\n");
       (void)fflush(stdout);
    }
    else
    {  (void)printf("TRUE\n");
       (void)fflush(stdout);
    }

    exit(0);
}
