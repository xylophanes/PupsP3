/*-------------------------------------------------------------------------
    Purpose: Set/update build date stamp on a PUPS application source. Used
             by applications to identify stale dynamic resources.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 1.06
    Dated:   12th December 2024
    E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <stdint.h>


/*---------*/
/* Defines */
/*---------*/
/*---------*/
/* Version */
/*---------*/

#define VSTAMP_VERSION    "1.06"




/*------------------*/
/* Main entry point */
/*------------------*/

int32_t main(int32_t argc, char *argv[])

{   time_t time_stamp = 0;
    FILE *stream    = (FILE *)NULL;


    /*--------------------*/
    /* Parse command line */
    /*--------------------*/

    if(argc > 1)
    {  (void)fprintf(stderr,"\nvstamp version %s, (C) Tumbling Dice 2000-2024 (gcc %s: built %s %s)\n\n",VSTAMP_VERSION,__VERSION__,__TIME__,__DATE__);
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
    (void)fprintf(stream,"/* (C) M.A. O'Neill, Tumbling Dice, 2024             */\n");
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

