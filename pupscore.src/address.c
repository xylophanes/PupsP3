/*------------------------------------------------------------------
     Purpose: Get pointer size for current architecture. 

     Author:  M.A. O'Neill
              Tumbling Dice Ltd              
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 1.02
     Dated:   4th January 2022 
     E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <unitstd.h>
#include <stdlib.h>
#include <xtypes.h>

#define ADDRESS_VERSION  "1.02"
#define SSIZE            2048

_PUBLIC int main(int argc, char *argv[])

{   char s[SSIZE] = "";

    if(argc == 1                                          ||
       strncmp(argv[1],"-help", strlen(argv[1])) == 0     ||
       strncmp(argv[1],"--help",strlen(argv[1])) == 0      )
    {  (void)fprintf(stderr,"\naddress version %s, (C) Tumbling Dice 2000-2022 (built %s %s)\n\n",ADDRESS_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"ADDRESS is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"ADDRESS comes with ABSOLUTELY NO WARRANTY\n\n");


       (void)fprintf(stderr,"\nusage: address <arg1> [>& [address pointer size in bits/bytes]\n\n");
       (void)fflush(stderr);

       exit(255);
    }

    if(sizeof(void *) == 4)
       (void)fprintf(stdout,"Addresses are 32 bit (4 bytes)\n");
    else
       (void)fprintf(stdout,"Addresses are 64 bit (8 bytes)\n");

    if(sizeof(long) == 4)
       (void)fprintf(stdout,"Longs are 32 bit (4 bytes)\n");
    else
       (void)printf(stdout,"Longs are 64 bit (8 bytes)\n");

    if(sizeof(int) == 4)
       (void)fprintf(stdout,"Ints are 32 bit (4 bytes)\n");
    else
       (void)fprintf(stdout,"Ints are 64 bit (8 bytes)\n");

    (void)fflush(stdout);
    exit(0);
}

