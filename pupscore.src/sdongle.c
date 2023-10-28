/*------------------------------------------------------------------
     Purpose: Generate soft dongle file (on random inode) 

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

#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <xtypes.h>


/*---------------------------------------------*/
/* Defines which are local to this application */
/*---------------------------------------------*/


/*--------------------*/
/* Version of sdongle */
/*--------------------*/

#define SDONGLE_VERSION "2.00"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE        2048 
#define MAX_SHUFFLES 1024 


/*------------------*/
/* Main entry point */
/*------------------*/

_PUBLIC int main(int argc, char *argv[])

{   int i,
        n_shuffles;

    char desname[SSIZE]          = "",
         dongle_file_name[SSIZE] = "softdongel.rin";

    struct stat stat_buf;

    if(argc < 2 || argc > 3 || strcmp(argv[1],"-usage") == 0 || strcmp(argv[1],"-help") == 0)
    {  (void)fprintf(stderr,"\nsdongle version %s, (C) Tumbling Dice 2003-2023 (built %s)\n\n",SDONGLE_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"SDONGLE is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"SDONGLE comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fprintf(stderr,"\nUsage: sdongle [-usage | -help] [<dongle file name>]\n\n");
       (void)fflush(stderr);

       exit(255);
    }


    /*------------------------------*/
    /* Seed random number generator */
    /*------------------------------*/

    (void)srand((long)getpid());


    /*--------------------------------------------------------*/
    /* Create N random files -- the last file (and its inode) */
    /* is retained for the soft dongle.                       */
    /*--------------------------------------------------------*/

    n_shuffles = rand() % MAX_SHUFFLES;

    if(argc > 1 && access(argv[1],F_OK | R_OK | W_OK) != (-1))
    {  (void)fprintf(stderr,"dongle: soft dongle file \"%s\" already exists\n",argv[1]);
       (void)fflush(stderr);

       exit(255);
    }
    else if(access(argv[1],F_OK | R_OK | W_OK) != (-1))
    {  (void)fprintf(stderr,"dongle: soft dongle file \"%s\" already exists\n",dongle_file_name);
       (void)fflush(stderr);

       exit(255);
    }


    /*-----------------*/
    /* Randomise inode */
    /*-----------------*/

    for(i=0; i<n_shuffles; ++i)
    {  (void)snprintf(desname,SSIZE,".uri.%d.%d.tmp",i,getpid()); 
       (void)close(creat(desname,0600));
    }


    /*---------------------------*/
    /* Create soft dongle dongle */
    /*---------------------------*/

    if(argc == 2)
    {  (void)close(creat(argv[1],0600)); 
       (void)stat(argv[1],&stat_buf);

       (void)fprintf(stderr,"\nsdongle: dongle file \"%s\" created\n",argv[1]);
       (void)fprintf(stderr,"sdongle: dongle file inode is: %x\n\n",stat_buf.st_ino);
       (void)fflush(stderr);
    }
    else
    {  (void)close(creat(dongle_file_name,0600)); 
       (void)stat(dongle_file_name,&stat_buf);

       (void)fprintf(stderr,"\nsdongle: dongle file \"%s\" created\n",dongle_file_name);
       (void)fprintf(stderr,"sdongle: dongle file inode is: %x\n\n",stat_buf.st_ino);
       (void)fflush(stderr);
    }


    /*------------------------*/
    /* Close termporary files */
    /*------------------------*/

    for(i=0; i<n_shuffles; ++i)
    {  (void)snprintf(desname,SSIZE,".uri.%d.%d.tmp",i,getpid());
       (void)unlink(desname);
    }

    exit(0);
}
