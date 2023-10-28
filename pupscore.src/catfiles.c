/*------------------------------------------------------------------
    Purpose:  Catenate pair of files

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.00 
    Dated:   4th January 2023
    e-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------*/


#include <xtypes.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


/*---------------------*/
/* Version of catfiles */
/*---------------------*/

#define CATFILES_VERSION    "2.00"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE               2048 




/*------------------*/
/* main entry point */
/*------------------*/

_PUBLIC int main(int argc, char *argv[])

{   FILE *stream_1 = (FILE *)NULL,
	 *stream_2 = (FILE *)NULL;

    char line[SSIZE] = "";

    if(argc < 3)
    {  (void)fprintf(stderr,"\ncatfiles version %s, (C) Tumbling Dice 2003-2023 (built %s %s)\n\n",CATFILES_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"CATFILES is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"CATFILES comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fprintf(stderr,"\nUsage: catfiles <appendor file name> <appendee file name>\n\n");
       (void)fflush(stderr);

       exit(255);
    }


    /*----------------------------------------------------*/
    /* Check that we can read appendee and appendor files */
    /*----------------------------------------------------*/

    if(access(argv[1],F_OK  | R_OK | W_OK) == (-1))
    {  (void)fprintf(stderr,"catfiles: cannot find (appendor) file \"%s\"\n",argv[1]);
       (void)fflush(stderr);

       exit(255);
    }

    if(access(argv[2],F_OK  | R_OK | W_OK) == (-1))
    {  (void)fprintf(stderr,"catfiles: cannot find (appendee) file \"%s\"\n",argv[2]);
       (void)fflush(stderr);

       exit(255);
    }

    if((stream_1 = fopen(argv[1],"a")) == (FILE *)NULL)
    {  (void)fprintf(stderr,"catfiles: cannot open (appendee) file \"%s\"\n",argv[2]);
       (void)fflush(stderr);

       exit(255);
    }

    if((stream_2 = fopen(argv[2],"r")) == (FILE *)NULL)
    {  (void)fprintf(stderr,"catfiles: cannot open (appendor) file \"%s\"\n",argv[2]);
       (void)fflush(stderr);

       exit(255);
    }

    do {   (void)fgets(line,SSIZE,stream_2);
	   
	   if(feof(stream_2) == 0)
	   {  (void)fputs(line,stream_1);
	      (void)(void)fflush(stream_1);
	   }
       } while(feof(stream_2) == 0);

    (void)fclose(stream_1);
    (void)fclose(stream_2);

    exit(0);
}
