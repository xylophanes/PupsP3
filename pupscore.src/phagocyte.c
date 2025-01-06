/*-----------------------------------------------------------------
     Purpose: Remove processes using too much or too litle CPU time 

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 1.02 
     Dated:   10th December 2024 
     E-mail:  mao@tumblingdice.co.uk
-----------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <bsd/string.h>
#include <xtypes.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <ftype.h>
#include <stdint.h>


#define _XOPEN_SOURCE
#include <unistd.h>


/*---------*/
/* Defines */
/*---------*/
/*---------*/
/* Version */
/*---------*/

#define  PHAGOCYTE_VERSION "1.02"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE               2048 


/*----------------------*/
/* Argument vector size */
/*----------------------*/

#define ARGC                255


/*-------------------*/
/* Private variables */
/*-------------------*/

_PRIVATE unsigned char hostname[SSIZE]        = "";
_PRIVATE unsigned char monitor_pname[SSIZE]   = "";
_PRIVATE unsigned char pname[SSIZE]           = "";
_PRIVATE unsigned char new_pname[SSIZE]       = "";
_PRIVATE unsigned char ppath[SSIZE]           = "";
_PRIVATE uint32_t      pname_pos              = 0;
_PRIVATE uint32_t      period                 = 0;
_PRIVATE uint32_t      n_killed               = 0;
_PRIVATE FTYPE         min_cpu_usage          = 10.0;
_PRIVATE FTYPE         max_cpu_usage          = 90.0;
_PRIVATE _BOOLEAN      do_daemon              = FALSE;
_PRIVATE _BOOLEAN      do_verbose             = FALSE;


/*-----------------------------------------------*/
/* Functions which are local to this application */
/*-----------------------------------------------*/
/*---------------------*/
/* Handler for SIGALRM */
/*---------------------*/

_PRIVATE void alarm_handler(void)

{   unsigned char  ps_cmd[SSIZE]   = "";
    pid_t          pid             = (-1);
    FTYPE          next_cpu_usage  = 0.0;
    FILE           *ps_stream      = (FILE *)NULL;


    /*---------------------------------*/
    /* Run ps command to get cpu usage */
    /*---------------------------------*/

    (void)snprintf(ps_cmd,SSIZE,"ps ux | grep %s | awk '{print $2,$3}'",monitor_pname);

    if ((ps_stream = popen(ps_cmd,"r")) == (FILE *)NULL)
    {  if (do_verbose == TRUE)
       {  (void)fprintf(stderr,"\n%s (%d@%s): failed to open stream for 'ps' command\n\n",pname,pid);
          (void)fflush(stderr);
       }
      
       exit(255);
    } 


    /*---------------------------------------------*/
    /* Look at cpu usages for named process/binary */
    /*---------------------------------------------*/

    do {   (void)fscanf(ps_stream,"%d%F",&pid,&next_cpu_usage);


           /*-----------------------------------------------*/
           /* Process above threshold cpu usage - terminate */
           /*-----------------------------------------------*/

           if (next_cpu_usage > max_cpu_usage)
           {  (void)kill(pid,SIGKILL);
              ++n_killed;

              if (do_verbose == TRUE)
              {  (void)fprintf(stderr,"\n%s (%d@%s): %s process [%d] killed\n\n",pname,getpid(),hostname,monitor_pname,pid);
                 (void)fflush(stderr);
              }
           }


           /*-----------------------------------------------*/
           /* Process below threshold cpu usage - terminate */
           /*-----------------------------------------------*/

           else if (next_cpu_usage < min_cpu_usage)
           {  (void)kill(pid,SIGKILL);
              ++n_killed;

              if (do_verbose == TRUE)
              {  (void)fprintf(stderr,"\n%s (%d@%s): %s process [%d] killed\n\n",pname,getpid(),hostname,monitor_pname,pid);
                 (void)fflush(stderr);
              }
           }

        } while(feof(ps_stream) != 0);


    /*-------------------------*/
    /* Close ps command stream */
    /*-------------------------*/

    (void)pclose(ps_stream);


    /*-----------------------*/
    /* Schedule next SIGALRM */
    /*-----------------------*/

    (void)alarm(period);
}



/*--------------*/
/* Exit handler */
/*--------------*/

_PRIVATE int32_t exit_handler(const int32_t signum)

{   if (strncmp(ppath,"/tmp",4) == 0)
       (void)unlink(ppath);

    if (signum == SIGINT)
       (void)fprintf(stderr,"\n%s (%d@%s): terminated by SIGINT\n\n", pname,getpid(),hostname);

    else if (signum == SIGQUIT)
       (void)fprintf(stderr,"\n%s (%d@%s): terminated by SIGQUIT\n\n", pname,getpid(),hostname);

    else if (signum == SIGHUP)
       (void)fprintf(stderr,"\n%s (%d@%s): terminated by SIGHUP\n\n", pname,getpid(),hostname);

    else if (signum == SIGTERM)
       (void)fprintf(stderr,"\n%s (%d@%s): terminated by SIGTERM\n\n",pname,getpid(),hostname);


    /*---------*/
    /* Default */
    /*---------*/

    else
       (void)fprintf(stderr,"\n%s (%d@%s): terminated by signal %d\n",pname,getpid(),hostname,signum);
    (void)fflush(stderr);

    exit(0);
}




/*----------------*/
/* Status handler */
/*----------------*/

_PRIVATE  int32_t status_handler(const  int32_t signum)


{  (void)fprintf(stderr,"\nphagocyte abnormal process killer version %s, (C) Tumbling Dice, 2024 (gcc %s: built %s %s)\n\n",PHAGOCYTE_VERSION,__VERSION__,__TIME__,__DATE__);

   if (do_verbose == TRUE)
      (void)fprintf(stderr,"    verbose mode                   : on\n");
   else
      (void)fprintf(stderr,"    verbose mode                   : off\n");

   if (do_daemon == TRUE)
      (void)fprintf(stderr,"    daemon mode                    :  on\n");
   else
      (void)fprintf(stderr,"    daemon mode                    :  off\n");

   (void)fprintf(stderr,"    partial monitoredprocess name  :  %s\n",           monitor_pname);
   (void)fprintf(stderr,"    monitor period                 :  %04d seconds\n", period);     
   (void)fprintf(stderr,"    minimum cpu usage              :  %5.2F%%\n",      min_cpu_usage);
   (void)fprintf(stderr,"    maximum cpu usage              :  %5.2F%%\n\n",    max_cpu_usage);

   (void)fprintf(stderr,"    %04d processes killed\n\n",n_killed);
   (void)fflush(stderr);

   return(0);
}




/*---------------------------------------*/
/* Extract leaf from the end of pathname */
/*---------------------------------------*/

_PRIVATE _BOOLEAN strleaf(const unsigned char *pathname, unsigned char *leaf)

{   uint32_t    i;

    for (i=strlen(pathname); i>= 0; --i)
    {  if (pathname[i] == '/')
          break;
    }

    if (i > 0)
       (void)strlcpy(leaf,(unsigned char *)&pathname[i+1],SSIZE);
    else
       (void)strlcpy(leaf,pathname,SSIZE);

    return(0);
}




/*----------------------------*/
/* Make sure path is absolute */
/*----------------------------*/

_PRIVATE int32_t absolute_path(const unsigned char *pathname, unsigned char *absolute_pathname)

{   unsigned char leaf[SSIZE]         = "",
                  current_path[SSIZE] = "";


    /*---------------*/
    /* Absolute path */
    /*---------------*/

    if (pathname[0] == '/')
       (void)strlcpy(absolute_pathname,pathname,SSIZE);


    /*-------------------------------*/
    /* Relative path (make absolute) */
    /*-------------------------------*/

    else
    {  (void)strleaf(pathname, leaf);
       (void)getcwd(current_path,SSIZE);
       (void)snprintf(absolute_pathname,SSIZE,"%s/%s",current_path,leaf);
    }

    return(0);
}




/*--------------------------------------*/
/* Main entry point to this application */
/*--------------------------------------*/

_PUBLIC  int32_t main(int argc, char *argv[])

{   uint32_t      i,
                  decoded        = 0,
                  p_index        = 1;

    unsigned char tmpstr[SSIZE]  = "";
    _BOOLEAN      do_daemon      = FALSE;

    sigset_t      set;


    /*----------*/
    /* Hostname */
    /*----------*/

    (void)gethostname(hostname,SSIZE);


    /*--------------------------------------------------*/
    /* Get process name (stripping out branch from path */
    /*--------------------------------------------------*/

    (void)strlcpy(ppath,argv[0],SSIZE);
    (void)absolute_path(ppath,tmpstr);
    (void)strlcpy(ppath,tmpstr,SSIZE);
    (void)strleaf(ppath,pname);


    /*--------------------*/
    /* Parse command line */
    /*--------------------*/

    if (argc == 1 || strcmp(argv[1],"-usage") == 0  || strcmp(argv[1],"-help") == 0)
    {  (void)fprintf(stderr,"\nphagocyte abnormal process killer version %s, (C) Tumbling Dice, 2024 (gcc %s: built %s %s)\n\n",PHAGOCYTE_VERSION,__VERSION__,__TIME__,__DATE__);

       (void)fprintf(stderr,"PHAGOCYTE is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"PHAGOCYTE comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"Usage: phagocyte [-usage] | [-help] |\n");
       (void)fprintf(stderr,"       [-verbose:FALSE]\n");
       (void)fprintf(stderr,"       [-daemon [<name of daemon process>]]\n");
       (void)fprintf(stderr,"       !-monitor <process name>!\n");
       (void)fprintf(stderr,"       [-period <process CPU usage polling delay in second]\n");
       (void)fprintf(stderr,"       [-mincpu <usage%%: %5.2F%%>]\n",min_cpu_usage);
       (void)fprintf(stderr,"       [-maxcpu <usage%%: %5.2F%%>]\n\n",max_cpu_usage);
       (void)fflush(stderr);

       exit(1);
    }

    for (i=0; i<argc-1; ++i)
    {

       /*----------------------------------------*/
       /* Log error/status information to stderr */
       /*----------------------------------------*/

       if (strcmp(argv[i],"-verbose") == 0)
       {  do_verbose = TRUE;
          ++decoded;
       }


       /*-------------*/
       /* Daemon mode */
       /*-------------*/

       else if (strcmp(argv[i],"-daemon") == 0)
       {  do_daemon  = TRUE;

          if (i <= argc - 1 && argv[i+1][0] != '-')
          {  (void)sscanf(argv[i+1],"%s",new_pname); 


             /*------------------------------------*/
             /* Daemon runs under new process name */
             /*------------------------------------*/

             if (strcmp(pname,new_pname) == 0)
             {  if (do_verbose == TRUE)
                {  (void)fprintf(stderr,"\n%s (%d@%s): new and old process names cannot be the same\n\n",pname,getpid(),hostname);
                   (void)fflush(stderr);
                }

                exit(255);
             }

             pname_pos = i + 1;

             ++i;
             decoded += 2;
          }


          /*-----------*/
          /* Keep name */
          /*-----------*/

          else 
             ++decoded;
       }


       /*------------------------*/
       /* Monitored process name */
       /*------------------------*/

       else if (strcmp(argv[i],"-monitor") == 0)
       {   

          /*-------------------------------*/
          /* Get name of monitored process */
          /*-------------------------------*/

          if (i <= argc - 1 && argv[i+1][0] != '-')
             (void)sscanf(argv[i+1],"%s",monitor_pname); 


          /*-------*/
          /* Error */
          /*-------*/

          else
          {  if (do_verbose == TRUE)
             {  (void)fprintf(stderr,"\n%s (%d@%s): expecting name of monitored process\n\n",pname,getpid(),hostname);
                (void)fflush(stderr);
             }

             exit(255);
          }

          ++i;
          decoded += 2;;
       }


       /*----------------------------*/
       /* Minimun cpu load (percent) */
       /*----------------------------*/

       else if (strcmp(argv[i],"-mincpu") == 0)
       {  if (i == argc - 1 || argv[i+1][0] == '-' || sscanf(argv[i+1],"%F",&min_cpu_usage) != 1 || min_cpu_usage < 0.0 || min_cpu_usage > 100.0) 
          {  if (do_verbose == TRUE)
             {  (void)fprintf(stderr,"\n%s (%d@%s): min cpu load must be a floating point value (between 0.0 and 100.0)\n\n",pname,getpid(),hostname);
                (void)fflush(stderr);
             }

             exit(255);
          }

          ++i;
          decoded += 2;;
       }


       /*----------------------------*/
       /* Maximum cpu load (percent) */
       /*----------------------------*/

       else if (strcmp(argv[i],"-maxcpu") == 0)
       {  if (i == argc - 1 || argv[i+1][0] == '-' || sscanf(argv[i+1],"%F",&max_cpu_usage) != 1 || max_cpu_usage <= min_cpu_usage || max_cpu_usage > 100.0) 
          {  if (do_verbose == TRUE)
             {  (void)fprintf(stderr,"\n%s (%d@%s): max cpu load must be a floating point value (between %5.2F and 100.0)\n\n",pname,getpid(),hostname,min_cpu_usage);
                (void)fflush(stderr);
             }

             exit(255);
          }

          ++i;
          decoded += 2;;
       }



       /*---------------------------------------*/
       /* Period in seconds (probing CPU usage) */
       /*---------------------------------------*/

       else if (strcmp(argv[i],"-period") == 0)
       {  if (i == argc - 1 || argv[i+1][0] == '-' || sscanf(argv[i+1],"%d",&period) != 1) 
          {  if (do_verbose == TRUE)
             {  (void)fprintf(stderr,"\n%s (%d@%s): period must be a positive integer value\n\n",pname,getpid(),hostname);
                (void)fflush(stderr);
             }

             exit(255);
          }

          ++i;
          decoded += 2;;
       }
    }
  

    /*---------------------------------------*/
    /* Check for unparsed command line items */
    /*---------------------------------------*/
 
    if (decoded < argc - 2)
    {  if (do_verbose == TRUE)
       {  (void)fprintf(stderr,"\n%s (%d@%s): command tail items unparsed\n\n",pname,getpid(),hostname);
          (void)fflush(stderr);
       }

       exit(255);
    }
 

    /*------------------------*/ 
    /* Set up signal handlers */
    /*------------------------*/ 

    (void)sigemptyset(&set);
    (void)sigprocmask(SIG_SETMASK,&set,(sigset_t *)NULL);

    (void)signal(SIGINT, (void *)&exit_handler);
    (void)signal(SIGQUIT,(void *)&exit_handler);
    (void)signal(SIGHUP, (void *)&exit_handler);
    (void)signal(SIGTERM,(void *)&exit_handler);
    (void)signal(SIGALRM,(void *)&alarm_handler);
    (void)signal(SIGUSR1,(void *)&status_handler);


    /*-------------*/
    /* Daemon mode */
    /*-------------*/

    if (do_daemon == TRUE)
    {  unsigned char tmpPathName[SSIZE] = "";
       uint32_t      nargc              = 1;
       char          *nargv[ARGC]       = { (char *)NULL };

       if (strcmp(new_pname,"") == 0)
          (void)snprintf(tmpPathName,SSIZE,"/tmp/lyosome"); 
       else
          (void)snprintf(tmpPathName,SSIZE,"/tmp/%s",new_pname); 

       if (symlink(ppath,tmpPathName) == (-1))
       {  if (do_verbose == TRUE)
          {  (void)fprintf(stderr,"%s (%d@%s): failed to link (lyosome) binary\n",pname,getpid(),hostname);
             (void)fflush(stderr);
          }

          exit(255);
       }


       /*----------------------------------*/
       /* Initrial argument vector element */
       /*----------------------------------*/
       /*-------*/
       /* Error */
       /*-------*/

       if ((nargv[0] = (unsigned char *)malloc(SSIZE*sizeof(unsigned char))) == (unsigned char *)NULL)
       {  if (do_verbose == TRUE)
          {  (void)fprintf(stderr,"%s (%d@%s): failed to allocate initial argument parameter\n",pname,getpid(),hostname);
             (void)fflush(stderr);
          }

          exit(255);
       }


       /*----*/
       /* Ok */
       /*----*/

       else
          (void)strlcpy(nargv[0],tmpPathName,SSIZE);


       /*---------------------*/
       /* Set up command tail */
       /*---------------------*/

       for (i=1; i<argc; ++i)
       {  if (i != pname_pos && strcmp(argv[i],"-d") != 0)
          {  


            /*------------------------------*/
            /* Next argument vector element */
            /*------------------------------*/

            if ((nargv[nargc] = (unsigned char *)malloc(SSIZE*sizeof(unsigned char))) != (unsigned char *)NULL)
                (void)strlcpy(nargv[nargc],argv[i],SSIZE);


             /*-------*/
             /* Error */
             /*-------*/

             else
             {  if (do_verbose == TRUE)
                {  (void)fprintf(stderr,"%s (%d@%s): failed to allocate next argument parameter\n",pname,getpid(),hostname);
                   (void)fflush(stderr);
                }

                exit(255);
             }

             ++nargc;
          }
       }

       nargv[nargc] = (char *)NULL;


       /*--------*/
       /* Parent */
       /*--------*/

       if (fork() != 0)
          _exit(0);


       /*-------*/
       /* Child */
       /*-------*/

       else
       {

          /*-----------------------------*/
          /* Close stdio (if not needed) */
          /*-----------------------------*/

          if (do_verbose == FALSE)
          {  (void)close(0);
             (void)close(1);
             (void)close(2);
          }



          /*----------------------------------*/
          /* Make daemon process group leader */
          /*----------------------------------*/

          (void)setsid();
          (void)execvp(tmpPathName,nargv);


          /*---------------------*/
          /* Should not get here */
          /*---------------------*/

          _exit(255);
       }

       if (strncmp(ppath,"/tmp",4) == 0)
          (void)unlink(ppath);

    }


    /*------------------------------------*/
    /* Wait for SIGALRM (probe cpu usage) */
    /*------------------------------------*/

    (void)alarm(period);
    while (TRUE)
        (void)usleep(1000);
}
