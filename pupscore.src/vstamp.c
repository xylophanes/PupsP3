/*-----------------------------------------------------------------------------
    Purpose: Set/update build date stamp on a PUPS application source. Used
             by applications to identify stale dynamic resources.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 1.05
    Dated:   4th January 2023
    E-mail:  mao@tumblingdice.co.uk
-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/*-------------------*/
/* Version of vstamp */
/*-------------------*/

#define VSTAMP_VERSION    "1.05"




/*------------------*/
/* Main entry point */
/*------------------*/

int main(int argc, char *argv[])

{   int time_stamp = 0;
    FILE *stream   = (FILE *)NULL;

    if(argc > 1)
    {  (void)fprintf(stderr,"\nvstamp version %s, (C) Tumbling Dice 2000-2023 (built %s %s)\n\n",VSTAMP_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"VSTAMP is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"VSTAMP comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nUsage: vstamp (updates stamp.h)\n\n");
       (void)fflush(stderr);

       exit(0);
    }
    
    (void)creat("vstamp.h",0644);
    stream = fopen("vstamp.h","w");

    (void)time(&time_stamp);

    (void)fprintf(stream,"/*---------------------------------------------------*/\n");
    (void)fprintf(stream,"/* Time stamp file generated automatically by vstamp */\n");
    (void)fprintf(stream,"/* (C) M.A. O'Neill, Tumbling Dice, 2023             */\n");
    (void)fprintf(stream,"/* Do not edit                                       */\n");
    (void)fprintf(stream,"/*---------------------------------------------------*/\n");
    (void)fprintf(stream,"\n\n#ifdef TSTAMP_H\n#define TSTAMP_H\n\n");
    (void)fprintf(stream,"int appl_timestamp = %d;\n\n",time_stamp);
    (void)fprintf(stream,"#endif /* Time stamp */\n");
    (void)fflush(stream);

    (void)fprintf(stderr,"vstamp: time stamp header updated (%d)\n\n",time_stamp);
    (void)fflush(stderr);

    (void)fclose(stream);
    exit(0);
}

