/*--------------------------------------------------------
    Purpose: Return suffix of string of form prefix.suffix 

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.02 
    Dated:   11th December 2024
    E-mail:  mao@tumblingdice.co.uk
--------------------------------------------------------*/


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

#define SUFFIX_VERSION  "2.00"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE           2048


/*------------------*/
/* main entry point */
/*------------------*/

_PUBLIC  int32_t main(int32_t argc, char *argv[])

{   uint32_t i,
             j,
             pos           = 0,
             cnt           = 0;

    char     suffix[SSIZE] = "";


    /*--------------------*/
    /* Parse command line */
    /*--------------------*/

    if(argc != 2                                          ||
       strncmp(argv[1],"-help", strlen(argv[1])) == 0     ||
       strncmp(argv[1],"--help",strlen(argv[1])) == 0      )
    {  (void)fprintf(stderr,"\nsuffix version %s, (C) Tumbling Dice 2000-2024 (gcc %s: built %s %s)\n\n",SUFFIX_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fprintf(stderr,"SUFFIX is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"SUFFIX comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nusage: suffix <string>\n\n");
       (void)fflush(stderr);

       exit(255);
    }

    for(i=0; i<strlen(argv[1]); ++i)
    {  if(argv[1][i] == '.')
          pos = i;
    }

    if(pos > 0)
    {  for(j=pos+1; j<strlen(argv[1]); ++j)
          suffix[cnt++] = argv[1][j];
       suffix[cnt] = '\0';

       (void)fprintf(stdout,"%s\n",suffix);
       (void)fflush(stdout);

       exit(0);
     }

     (void)fprintf(stdout,"%s\n",argv[1]);
     (void)fflush(stdout);

     exit(0);
}
