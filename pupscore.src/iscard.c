/*-------------------------------------------------------------------------
    Purpose: Test to see if input is cardinal (integer >= 0) 

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.00 
    Dated:   4th January 2023
    E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <xtypes.h>


/*---------*/
/* Version */
/*---------*/

#define ISCARD_VERSION  "2.00"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE           2048 


/*------------------*/
/* Main entry point */
/*------------------*/

_PUBLIC int main(int argc, char *argv[])

{   int idum;

    if(argc != 2)
    {  (void)fprintf(stderr,"\niscard version %s, (C) Tumbling Dice 2000-2023 (built %s %s)\n\n",ISCARD_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"ISCARD is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"ISCARD comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nUsage: iscard <string>\n\n");
       (void)fflush(stderr);

       exit(255);
    }

    
    if(sscanf(argv[1],"%d",&idum) != 1)
    {  (void)printf("FALSE");
       (void)fflush(stdout);
    }
    else if(idum < 0)
    {  (void)printf("FALSE");
       (void)fflush(stdout);
    }
    else
    {  (void)printf("TRUE");
       (void)fflush(stdout);
    }

    exit(0);
}
