/*------------------------------------------------------------------
    Purpose:  integer to float conversion function for shell scripts

    Author:  Mark A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle
             NE3 4RT
             Tyne and Wear

    Version: 1.03
    Dated:   19th February 2024
    E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ftype.h>
#include <stdint.h>


/*---------*/
/* Defines */
/*---------*/

#define INTF_VERSION  "1.03"


/*------------------*/
/* Main entry point */
/*------------------*/

int32_t main(int32_t argc, char *argv[])

{    int32_t a   = 0,
             ret = 0;


    /*--------------------*/
    /* Parse command line */
    /*--------------------*/

    if(argc != 2                                          ||
       strncmp(argv[1],"-help", strlen(argv[1])) == 0     ||
       strncmp(argv[1],"-usage",strlen(argv[1])) == 0      )
    {  (void)fprintf(stderr,"\nintf version %s, (C) Tumbling Dice 2000-2024 (built %s %s)\n\n",INTF_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fprintf(stderr,"INTF is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"INTF comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nusage:  int32_tf [-help] | [-usage] | <arg1> [>& [(float)<arg1>]\n\n");
       (void)fflush(stderr);

       exit(255);
    }
    else if(sscanf(argv[1],"%d",&a))
    {  (void)fprintf(stdout,"%7.3f",(FTYPE)a);
       (void)fflush(stdout);
    }
    
    exit(ret);
}
