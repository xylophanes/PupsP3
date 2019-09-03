/*--------------------------------------------------------------------------------
    Purpose: Division function for shell scripts

    Author:  Mark A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle
             NE3 4RT
             Tyne and Wear

    Version: 1.02
    Dated:   23rd January 2018 
    E-mail:  mao@tumblingdice.co.uk
---------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


/*----------*/
/* Version */
/*---------*/

#define DIV_VERSION "1.02"


/*------------------*/
/* Main entry point */
/*------------------*/

int main(int argc, char *argv[])

{   int ret = 0,
         a,
         b,
         result;

    if(argc != 3                                          ||
       strncmp(argv[1],"-help", strlen(argv[1])) == 0     ||
       strncmp(argv[1],"--help",strlen(argv[1])) == 0      )
    {  (void)fprintf(stderr,"\ndiv version %s, (C) Tumbling Dice 2000-2018 (built %s %s)\n\n",DIV_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"DIV is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"DIV comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nusage: div <arg1> <arg2> [>& [<arg1> / <arg2>]\n\n");
       (void)fflush(stderr);

       exit(-1);
    }
    else if(argc == 3 && sscanf(argv[1],"%d",&a) == 1 && sscanf(argv[2],"%d",&b) == 1)
    {  if(a == 0 || b == 0)
          ret = (-1);
       else
       {  result = a / b;

          (void)fprintf(stdout,"%d",result);
          (void)fflush(stdout);
       }
    }
    
    exit(ret);
}
