/*-------------------------------------------------------------------------------------
     Purpose: creates an anus (arse hole) to which data from payload pipeline is cat'ed

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.03 
     Dated:   10th Decemeber 2024 
     e-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------------------------*/

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


/*---------------------------------------------*/
/* Defines which are local to this application */
/*---------------------------------------------*/
/*-----------------*/
/* Version of arse */
/*-----------------*/

#define ARSE_VERSION   "2.02"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE          2048 


#ifndef FALSE 
#define FALSE 0 
#endif /* FALSE */

#ifndef TRUE
#define TRUE 255
#endif /* TRUE */




/*-------------------------------------------------*/
/* Variables which are private to this application */
/*-------------------------------------------------*/

_PRIVATE char hostname[SSIZE]       = "";
_PRIVATE char pen[SSIZE]            = "";
_PRIVATE char arse_fifo_name[SSIZE] = "";
_PRIVATE  int32_t  delete_on_exit   = FALSE;
_PRIVATE  int32_t  verbose          = FALSE;
_PRIVATE  int32_t  outdes           = (-1);




/*---------------------------*/
/* Handle SIGTERM and SIGINT */
/*---------------------------*/

_PRIVATE int32_t arse_exit(int32_t signum)

{   if(delete_on_exit == TRUE)
    {  (void)unlink(arse_fifo_name);

       if(verbose == TRUE)
       {  (void)fprintf(stderr,"%s (%d@%s): exit (arse hole \"%s\" deleted)\n\n",pen,getpid(),hostname,arse_fifo_name);
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

_PRIVATE int32_t arse_fifo_homeostat(int32_t signum)

{    int32_t      bytes_read = (-1);
    unsigned char buf[SSIZE] = "";

    if(access(arse_fifo_name,F_OK | R_OK) == (-1))
    {  

       /*---------------------------------------------------------*/
       /* Suck contents of (stale) FIFO which has become nameless */
       /*---------------------------------------------------------*/

       (void)fcntl(outdes,F_SETFL,O_NONBLOCK);
       bytes_read = read(outdes,buf,SSIZE);

       if(bytes_read > 0)
       {  if(verbose == TRUE)
          {  (void)fprintf(stderr,"arse [%s] version %s (%d@%s) flushing stale arse hole [\"%s\"]\n\n",
                                                     pen,ARSE_VERSION,getpid(),hostname,arse_fifo_name);
             (void)fflush(stderr);
          }

          (void)write(1,buf,bytes_read);
       }

       (void)close(outdes);

       if(mknod(arse_fifo_name,0600 | S_IFIFO,0) == (-1))
       {  if(verbose == TRUE)
          {  (void)fprintf(stderr,"arse [%s] version %s (%d@%s) failed to creat arse hole \"%s\"]\n\n",
                                                     pen,ARSE_VERSION,getpid(),hostname,arse_fifo_name);
             (void)fflush(stderr);
          }

          exit(255);
       }
       outdes = open(arse_fifo_name,2);

       if(verbose == TRUE)
       {  (void)fprintf(stderr,"%s (%d@%s): arse hole \"%s\" lost - restoring\n",pen,getpid(),hostname,arse_fifo_name);
          (void)fflush(stderr);
       }
    }


    /*----------------------*/
    /* Reset signal handler */
    /*----------------------*/

    (void)signal(SIGALRM,(void *)&arse_fifo_homeostat);
    (void)alarm(1);

    return(0);
}




/*-----------------------------------*/
/* Main entry point for application  */
/*-----------------------------------*/

_PUBLIC int32_t main(int32_t argc, char *argv[])

{   uint32_t      i,
                  bytes_read,
                  decoded       = 0,
                  do_sidebottom = FALSE,
                  end_of_data   = TRUE;

    char          newstr[SSIZE] = "";
    unsigned char buf[SSIZE]    = "";


    /*----------------------------*/
    /* Get process execution name */
    /*----------------------------*/

    (void)strlcpy(pen,argv[0],SSIZE);


    /*----------------------*/
    /* Exit signal handlers */
    /*----------------------*/

    (void)signal(SIGTERM,(void *)&arse_exit);
    (void)signal(SIGINT, (void *)&arse_exit);
    (void)signal(SIGALRM,(void *)&arse_fifo_homeostat);
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
       {  (void)fprintf(stderr,"\narse version %s, (C) Tumbling Dice 2005-2024 (gcc %s: built %s %s)\n\n",ARSE_VERSION,__VERSION__,__TIME__,__DATE__);
          (void)fprintf(stderr,"ARSE is free software, covered by the GNU General Public License, and you are\n");
          (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
          (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
          (void)fprintf(stderr,"ARSE comes with ABSOLUTELY NO WARRANTY\n\n");
          (void)fprintf(stderr,"\nUsage: arse [-usage | -help] | [-pen <execution name>] [-sidebottom:FALSE] !-hole <arse hole FIFO name>!\n\n");
          (void)fflush(stderr);

          exit(255);
       }
    }

    for(i=1; i<argc; ++i)
    {  if(strcmp(argv[i],"-verbose") == 0)
          verbose = TRUE;
       else if(strcmp(argv[i],"-pen") == 0)
       {  if(fork() == 0)
          {   int32_t  j,
                       cnt = 1;

             char args[32][SSIZE] = { "" },
                  *argptr[32]     = { NULL };

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
             argptr[0] = (char *)&args[0];

              if(verbose == TRUE)
              {  (void)fprintf(stderr,"%s (%d@%s): executing as \"%s\"\n\n",pen,getpid(),hostname,argptr[0]);
                 (void)fflush(stderr);
              }

              (void)execvp("arse",argptr);

              if(verbose == TRUE)
              {  (void)fprintf(stderr,"%s (%d@%s): failed to exec (arse) child process\n\n",pen,getpid(),hostname);
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
             {  (void)fprintf(stderr,"%s (%d@%s): no arse hole FIFO specified\n\n",pen,getpid(),hostname);
                (void)fflush(stderr);
             }

             exit(255);
          }

          (void)strlcpy(arse_fifo_name,argv[i+1],SSIZE);
          ++i;
          decoded += 2;
       }
       else  if(strcmp(argv[i],"-sidebottom") == 0)
       {  do_sidebottom = TRUE;
          ++decoded;
      }
    }

    if(decoded < argc - 2)
    {   if(verbose == TRUE)
        {  (void)fprintf(stderr,"%s (%d@%s): unparsed command tail arguments\n\n",pen,getpid(),hostname);
           (void)fflush(stderr);
        }

        exit(255);
    }

    if(strcmp(arse_fifo_name,"") == 0)
       (void)snprintf(arse_fifo_name,SSIZE,"arse.%d.%s.fifo",getpid(),hostname);

    if(access(arse_fifo_name,F_OK) == (-1))
    {  delete_on_exit = TRUE;

       if(mknod(arse_fifo_name,0600 | S_IFIFO,0) == (-1))
       {  if(verbose == TRUE)
          {  (void)fprintf(stderr,"arse [%s] version %s (%d@%s) failed to creat arse hole \"%s\"]\n\n",
                                                     pen,ARSE_VERSION,getpid(),hostname,arse_fifo_name);
             (void)fflush(stderr);
          }

          exit(255);
       }
       else
          (void)strlcpy(newstr,"new",SSIZE);
    }

    if(verbose == TRUE)
    {  (void)fprintf(stderr,"\narse [%s] version %s (%d@%s) started using %s arse hole \"%s\"\n\n",
                                          pen,ARSE_VERSION,getpid(),hostname,newstr,arse_fifo_name);
       (void)fflush(stderr);
    }

    outdes = open(arse_fifo_name,2);
    do {   bytes_read = read(0,buf,512);

           if(bytes_read > 0)
           {  if(end_of_data == TRUE)
              {  if(verbose == TRUE)
                 {  (void)fprintf(stderr,"arse [%s] version %s (%d@%s) new data stream established [to arse hole \"%s\"]\n\n",
                                                                            pen,ARSE_VERSION,getpid(),hostname,arse_fifo_name);
                    (void)fflush(stderr);
                 }

                 end_of_data = FALSE;
              }

              (void)write(outdes,buf,bytes_read);


              /*---------------------------------------------------*/
              /* If we are a sidebottom we must also write data to */
              /* standard output as well                           */
              /*---------------------------------------------------*/

              if(do_sidebottom == TRUE)
                 (void)write(1,buf,bytes_read); 

              if(bytes_read != 512)
                 end_of_data = TRUE;
           }

       } while(1);

    exit(0);
}
