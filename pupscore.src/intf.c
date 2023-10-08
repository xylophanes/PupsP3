/*--------------------------------------------------------------------------------
    Purpose: int to float conversion function for shell scripts

    Author:  Mark A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle
             NE3 4RT
             Tyne and Wear

    Version: 1.02
    Dated:   4th January 2022
    E-mail:  mao@tumblingdice.co.uk
---------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ftype.h>

#define INTF_VERSION  "1.02"

int main(int argc, char *argv[])

{   int a,
        ret = 0;

    if(argc != 2                                          ||
       strncmp(argv[1],"-help", strlen(argv[1])) == 0     ||
       strncmp(argv[1],"--help",strlen(argv[1])) == 0      )
    {  (void)fprintf(stderr,"\nintf version %s, (C) Tumbling Dice 2000-2022 (built %s %s)\n\n",INTF_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"INTF is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"INTF comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nusage: intf <arg1> [>& [(float)<arg1>]\n\n");
       (void)fflush(stderr);

       exit(255);
    }
    else if(sscanf(argv[1],"%d",&a))
    {  (void)fprintf(stdout,"%6.3f",(FTYPE)a);
       (void)fflush(stdout);
    }
    
    exit(ret);
}
