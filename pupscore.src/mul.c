/*--------------------------------------------------------------------------------
    Purpose: divide function for shell scripts

    Author:  Mark A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle
             NE3 4RT
             Tyne and Wear

    Version: 1.00
    Dated:   23rd January 2018
    E-mail:  mao@tumblingdice.co.uk
---------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


/*---------*/
/* Version */
/*---------*/

#define MUL_VERSION    1.03


/*------------------*/
/* Main entry point */
/*------------------*/

int main(int argc, char *argv[])

{   int a,
        b,
        result;

    if(argc != 3)
    {  (void)fprintf(stderr,"\nmul version %s, (C) Tumbling Dice 2000-2018 (built %s %s)\n\n",MUL_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"MUL is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"MUL comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nUsage: mul <a> <b>\n\n");
       (void)fflush(stderr);

       exit(-1);
    }
    else if(sscanf(argv[1],"%d",&a) == 1 && sscanf(argv[2],"%d",&b) == 1)
    {  result = a * b;

       (void)fprintf(stdout,"%d",result);
       (void)fflush(stdout);
    }
    
    exit(0);
}
