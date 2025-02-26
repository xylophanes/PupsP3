/*---------------------------------
    Purpose: create regular file

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.03
    Dated:   10th October 2024
    E-mail:  mao@tumblingdice.co.uk
---------------------------------*/

#include <xtypes.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdint.h>


/*---------*/
/* Defines */
/*---------*/
/*-------------------*/
/* Version of mkfile */
/*-------------------*/

#define MKFILE_VERSION    "2.03"



/*------------------*/
/* Main entry point */
/*------------------*/

_PUBLIC int32_t main(int32_t argc, char *argv[])

{

    /*--------------------*/
    /* Parse command line */
    /*--------------------*/

    if(argc == 1 || strcmp(argv[1],"help") == 0 ||  strcmp(argv[1],"usage") == 0)
    {  (void)fprintf(stderr,"\nmkfile version %s, (C) Tumbling Dice 2002-2024 (gcc %s: built %s)\n\n",MKFILE_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fprintf(stderr,"MKFILE is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"MKFILE comes with ABSOLUTELY NO WARRANTY\n\n");


       (void)fprintf(stderr,"usage: mkfile <name> <(octal) permissions>\n\n");
       (void)fflush(stderr);

       exit(1);
    }


    /*-------------------------------------------------------------*/
    /* Do we have a user supplied set of (octal) file permissions? */
    /*-------------------------------------------------------------*/

    if(argc == 3)
    {  uint32_t perms;

       if(sscanf(argv[2],"%o",&perms) != 1)
       {  (void)fprintf(stderr,"mkfile: expecting (octal) file permissions\n");
          (void)fflush(stderr);

          exit(255);
       }

       if(access(argv[1],F_OK | R_OK | W_OK) != (-1))
       {  (void)fprintf(stderr,"mkfile: file \"%s\" already exists\n",argv[1]);
          (void)fflush(stderr);

          exit(255);
       }

       (void)close(creat(argv[1],0600));
       (void)chmod(argv[1],perms);
    }
    else
    {  if(access(argv[1],F_OK | R_OK | W_OK) != (-1))
       {  (void)fprintf(stderr,"mkfile: file \"%s\" already exists\n",argv[1]);
          (void)fflush(stderr);

          exit(255);
       }

       /* Default file permissions */
       (void)close(creat(argv[1],0600));
    }

    exit(0);
}
