/*--------------------------------------------------------
    Purpose: Return prefix of string of form prefix.suffix 

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.02 
    Dated:   11th October 2024
    E-mail:  mao@tumblingdice.co.uk
--------------------------------------------------------*/


#include <stdio.h>
#include <string.h>
#include <bsd/string.h>
#include <stdlib.h>
#include <xtypes.h>
#include <stdint.h>


/*---------*/
/* Defines */
/*---------*/
/*---------*/
/* Version */
/*---------*/

#define PREFIX_VERSION  "2.02"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE           2048




/*------------------*/
/* Main entry point */
/*------------------*/

int32_t main(int32_t argc, char *argv[])

{   uint32_t i,
             cnt       = 0;

    char prefix[SSIZE] = "";


    /*---------------------*/
    /* Parse comamnnd line */
    /*---------------------*/

    if(argc != 2                                          ||
       strncmp(argv[1],"-help", strlen(argv[1])) == 0     ||
       strncmp(argv[1],"--help",strlen(argv[1])) == 0      )
    {  (void)fprintf(stderr,"\nprefix version %s, (C) Tumbling Dice 2000-2024 (gcc %s: built %s %s)\n\n",PREFIX_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fprintf(stderr,"PREFIX is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"PREFIX comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nusage: prefix <string>\n\n");
       (void)fflush(stderr);

       exit(255);
    }

    (void)strlcpy(prefix,argv[i],SSIZE); 
    for(i=strlen(argv[1]); i >= 0; --i)
    {  if(argv[1][i] == '.')
       {  (void)strlcpy(prefix,argv[1],SSIZE); 
          prefix[i] = '\0';
          (void)fprintf(stdout,"%s\n",prefix);
          (void)fflush(stdout);

          exit(0);
       }
     }

     (void)fprintf(stdout,"%s\n",argv[1]);
     (void)fflush(stdout);

     exit(0);
}
