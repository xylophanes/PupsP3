/*-----------------------------------------------------------------------------------------------------
     Purpose: creates a mouth (gob hole) into which data for payload application is cat'ed

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.03 
     Dated:   24th May 2023
     E-mail:  mao@tumblingdice.co.uk
-----------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <bsd/string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <xtypes.h>




/*------------------------------------------------------------------------------------------------------
    Defines which are local to this application ...
------------------------------------------------------------------------------------------------------*/
/*----------------*/
/* Version of gob */
/*----------------*/

#define GOB_VERSION   "2.03"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE         2048 

#ifndef FALSE 
#define FALSE 0 
#endif /* FALSE */

#ifndef TRUE
#define TRUE 255
#endif /* TRUE */




/*------------------------------------------------------------------------------------------------------
    Variables which are private to this application ...
------------------------------------------------------------------------------------------------------*/

static char hostname[SSIZE]      = "";
static char pen[SSIZE]           = "";
static char gob_fifo_name[SSIZE] = "";
static int  delete_on_exit       = FALSE;
static int  verbose              = FALSE;
static int  indes                = (-1);




/*---------------------------*/
/* Handle SIGTERM and SIGINT */
/*---------------------------*/

_PRIVATE int gob_exit(int signum)

{   if(delete_on_exit == TRUE)
    {  (void)unlink(gob_fifo_name);

       if(verbose == TRUE)
       {  (void)fprintf(stderr,"%s (%d@%s): exit (gob hole \"%s\" deleted)\n\n",pen,getpid(),hostname,gob_fifo_name);
          (void)fflush(stderr);
       }

       exit(0);
    }

    if(verbose == TRUE)
    {  (void)fprintf(stderr,"%s (%d@%s): exit\n\n",pen,getpid(),hostname);
       (void)fflush(stderr);
    }

    exit(0);
}




/*----------------*/
/* FIFO homeostat */ 
/*----------------*/

_PRIVATE int gob_fifo_homeostat(int signum)

{   int           bytes_read = (-1);
    unsigned char buf[SSIZE] = "";

    if(access(gob_fifo_name,F_OK | R_OK) == (-1))
    {  

       /*---------------------------------------------------------*/
       /* Suck contents of (stale) FIFO which has become nameless */
       /*---------------------------------------------------------*/

       (void)fcntl(indes,F_SETFL,O_NONBLOCK);
       bytes_read = read(indes,buf,SSIZE);

       if(bytes_read > 0)
       {  if(verbose == TRUE)
          {  (void)fprintf(stderr,"gob [%s] version %s (%d@%s) flushing stale gob hole [\"%s\"]\n\n",
                                                     pen,GOB_VERSION,getpid(),hostname,gob_fifo_name);
             (void)fflush(stderr);
          }

          (void)write(1,buf,bytes_read);
       }

       (void)close(indes);

       if(mknod(gob_fifo_name,0600 | S_IFIFO,0) == (-1))
       {  if(verbose == TRUE)
          {  (void)fprintf(stderr,"gob [%s] version %s (%d@%s) failed to creat gob hole \"%s\"]\n\n",
                                                     pen,GOB_VERSION,getpid(),hostname,gob_fifo_name);
             (void)fflush(stderr);
          }

          exit(255);
       }
       indes = open(gob_fifo_name,2);

       if(verbose == TRUE)
       {  (void)fprintf(stderr,"%s (%d@%s): gob hole \"%s\" lost - restoring\n",pen,getpid(),hostname,gob_fifo_name);
          (void)fflush(stderr);
       }
    }


    /*----------------------*/
    /* Reset signal handler */
    /*----------------------*/

    (void)signal(SIGALRM,(void *)&gob_fifo_homeostat);
    (void)alarm(1);

    return(0);
} 




/*----------------------------------*/
/* Main entry point for application */
/*----------------------------------*/

_PUBLIC int main(int argc, char *argv[])

{   int  i,
         bytes_read,
         decoded      = 0,
         end_of_data  = TRUE;

    char          newstr[SSIZE] = "";
    unsigned char buf[SSIZE]    = "";


    /*----------------------------*/
    /* Get process execution name */
    /*----------------------------*/

    (void)strncpy(pen,argv[0],SSIZE);


    /*----------------------*/
    /* Exit signal handlers */
    /*----------------------*/

    (void)signal(SIGTERM,(void *)&gob_exit);
    (void)signal(SIGINT, (void *)&gob_exit);
    (void)signal(SIGALRM,(void *)&gob_fifo_homeostat);
    (void)alarm(1);


    /*--------------*/ 
    /* Get hostname */
    /*--------------*/

    (void)gethostname(hostname,SSIZE);


    /*-------------------------------*/
    /* Decode command tail arguments */
    /*-------------------------------*/

    if(isatty(1) == 1)
    {  if(argc == 1 || strcmp(argv[1],"-usage") == 0 || strcmp(argv[1],"-help") == 0)
       {  (void)fprintf(stderr,"\ngob version %s, (C) Tumbling Dice 2005-2023 (built %s %s)\n\n",GOB_VERSION,__TIME__,__DATE__);
          (void)fprintf(stderr,"GOB is free software, covered by the GNU General Public License, and you are\n");
          (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
          (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
          (void)fprintf(stderr,"GOB comes with ABSOLUTELY NO WARRANTY\n\n");
          (void)fprintf(stderr,"\nUsage: gob [-usage | -help] | [-pen <execution name>] !-hole <gob hole FIFO name>!\n\n");
          (void)fflush(stderr);

          exit(255);
       }
    }

    for(i=1; i<argc; ++i)
    {  if(strcmp(argv[i],"-verbose") == 0)
          verbose = TRUE;
       else if(strcmp(argv[i],"-pen") == 0)
       {  if(fork() == 0)
          {  int  j,
                  cnt = 1;

             char args[32][SSIZE]  = { "" },
                  *argptr[32]      = { NULL };

             if(i == argc - 1 || argv[i+1][0] == '-')
             {  if(verbose == TRUE)
                {  (void)fprintf(stderr,"%s (%d@%s): expecting process execution name\n\n",pen,getpid(),hostname);
                   (void)fflush(stderr);
                }

                exit(255);
             }

             (void)strlcpy(args[0],argv[i+1],SSIZE);
             for(j=1; j<argc; ++j)
             {  if(j != i && j != i+1)
                {  (void)strlcpy(args[cnt],argv[j],SSIZE);
                   argptr[cnt] = (char *)&args[cnt];
                   ++cnt;
                }
             }

             (void)strlcpy(args[0],argv[i+1],SSIZE);

             (void)strlcpy(args[cnt],"\0",SSIZE);
             argptr[0]   = (char *)&args[0];

              if(verbose == TRUE)
              {  (void)fprintf(stderr,"%s (%d@%s): executing as \"%s\"\n\n",pen,getpid(),hostname,argptr[0]);
                 (void)fflush(stderr);
              }

              (void)execvp("gob",argptr);

              if(verbose == TRUE)
              {  (void)fprintf(stderr,"%s (%d@%s): failed to exec (gob) child process\n\n",pen,getpid(),hostname);
                 (void)fflush(stderr);
              }

              exit(255);
          }
          else
             _exit(255);
       }
       else if(strcmp(argv[i],"-hole") == 0)
       {  if(i == argc -1 || argv[i+1][0] == '-')
          {  if(verbose == TRUE)
             {  (void)fprintf(stderr,"%s (%d@%s): no gob hole FIFO specified\n\n",pen,getpid(),hostname);
                (void)fflush(stderr);
             }

             exit(255);
          }

          (void)strlcpy(gob_fifo_name,argv[i+1],SSIZE);
          ++i;
          decoded += 2;
       }
    }

    if(decoded < argc - 2)
    {   if(verbose == TRUE)
        {  (void)fprintf(stderr,"%s (%d@%s): unparsed command tail arguments\n\n",pen,getpid(),hostname);
           (void)fflush(stderr);
        }

        exit(255);
    }

    if(strcmp(gob_fifo_name,"") == 0)
       (void)sprintf(gob_fifo_name,"gob.%d.%s.fifo",getpid(),hostname);

    if(access(gob_fifo_name,F_OK) == (-1))
    {  delete_on_exit = TRUE;
       if(mknod(gob_fifo_name,0600 | S_IFIFO,0) == (-1))
       {  if(verbose == TRUE)
          {  (void)fprintf(stderr,"gob [%s] version %s (%d@%s) failed to creat gob hole \"%s\"]\n\n",
                                                     pen,GOB_VERSION,getpid(),hostname,gob_fifo_name);
             (void)fflush(stderr);
          }

          exit(255);
       }
       else
          (void)strlcpy(newstr,"new",SSIZE);
    }

    if(verbose == TRUE)
    {  (void)fprintf(stderr,"\ngob [%s] version %s (%d@%s) started using %s gob hole \"%s\"\n\n",
                                          pen,GOB_VERSION,getpid(),hostname,newstr,gob_fifo_name);
       (void)fflush(stderr);
    }

    indes = open(gob_fifo_name,2);
    do {   bytes_read = read(indes,buf,SSIZE);

           if(bytes_read > 0)
           {  if(end_of_data == TRUE)
              {  if(verbose == TRUE)
                 {  (void)fprintf(stderr,"gob [%s] version %s (%d@%s) new data stream established [frgm gob hole \"%s\"]\n\n",
                                                                              pen,GOB_VERSION,getpid(),hostname,gob_fifo_name);
                    (void)fflush(stderr);
                 }

                 end_of_data = FALSE;
              }

              (void)write(1,buf,bytes_read);
              if(bytes_read != SSIZE)
                 end_of_data = TRUE;
           }

       } while(1);

    exit(0);
}
