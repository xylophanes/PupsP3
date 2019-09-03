/*-----------------------------------------------------------------------------
    Purpose: Return prefix of string of form prefix.suffix. 

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Dated:   23rd January 2018
    Version: 1.05
    E-mail:  mao@tumblingdice.co.uk
-----------------------------------------------------------------------------*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/*-------------------*/
/* Version of prefix */
/*-------------------*/

#define PREFIX_VERSION    "1.05"



/*------------------*/
/* Main entry point */
/*------------------*/

int main(int argc, char *argv[])

{   int i,
        cnt = 0;

    char prefix[256] = "";

    if(argc != 2                                          ||
       strncmp(argv[1],"-help", strlen(argv[1])) == 0     ||
       strncmp(argv[1],"--help",strlen(argv[1])) == 0      )
    {  (void)fprintf(stderr,"\nprefix version %s, (C) Tumbling Dice 2000-2018 (built %s %s)\n\n",PREFIX_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"PREFIX is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"PREFIX comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nusage: prefix <string>\n\n");
       (void)fflush(stderr);

       exit(-1);
    }

    (void)strcpy(prefix,argv[i]); 
    for(i=strlen(argv[1]); i >= 0; --i)
    {  if(argv[1][i] == '.')
       {  (void)strcpy(prefix,argv[1]); 
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

