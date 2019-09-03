/*-------------------------------------------------------------------------
    Purpose: Test to see if input is integer

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 1.03
    Dated:   23rd January 2018
    E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/*---------*/
/* Version */
/*---------*/

#define ISINT_VERSION    1.03


/*------------------*/
/* Main entry point */
/*------------------*/

int main(int argc, char *argv[])

{   int  idum;
    char strdum[256] = "";

    if(argc != 2)
    {  (void)fprintf(stderr,"\nisint version %s, (C) Tumbling Dice 2000-2018 (built %s %s)\n\n",ISINT_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"ISINT is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"ISINT comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nUsage: isint <string>\n\n");
       (void)fflush(stderr);

       exit(-1);
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
