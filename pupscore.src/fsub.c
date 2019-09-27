/*--------------------------------------------------------------------------------
    Purpose: subtract function for shell scripts

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
#include <ftype.h>

#define FSUB_VERSION  "1.02"

int main(int argc, char *argv[])

{   int ret = 0;

    FTYPE a,
          b,
          result;

    if(argc != 3                                          ||
       strncmp(argv[1],"-help", strlen(argv[1])) == 0     ||
       strncmp(argv[1],"--help",strlen(argv[1])) == 0      )
    {  (void)fprintf(stderr,"\nfsub version %s, (C) Tumbling Dice 2000-2018 (built %s %s)\n\n",FSUB_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"FSUB is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"FSUB comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nusage: fsub <arg1> <arg2> [>& [<arg1> - <arg2>]\n\n");
       (void)fflush(stderr);

       exit(-1);
    }
    else if(sscanf(argv[1],"%F",&a) == 1 && sscanf(argv[2],"%F",&b) == 1)
    {  result = a - b;

       (void)fprintf(stdout,"%6.3f",result);
       (void)fflush(stdout);
    }
    
    exit(ret);
}
