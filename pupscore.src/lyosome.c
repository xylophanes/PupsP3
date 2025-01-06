/*-------------------------------------------
     Purpose: Delayed file destructor process 

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 3.03 
     Dated:   10th December 2024
     E-mail:  mao@tumblingdice.co.uk
-------------------------------------------*/

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
#include <time.h>
#include <stdint.h>


#define _XOPEN_SOURCE
#include <unistd.h>


/*-----------------------------------------------*/
/* Defines which are private to this application */
/*-----------------------------------------------*/
/*-------------------*/
/* Vesion of lyosome */
/*-------------------*/

#define  LYOSOME_VERSION    "3.03"
#define  FOREVER            (-9999.0)


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE               2048 


/*----------------------*/
/* Argument vector size */
/*----------------------*/

#define ARGC                255



/*------------------*/
/* Global variables */
/*------------------*/

_PRIVATE char          boldOn [8]             = "\e[1m",    // Make character bold
                       boldOff[8]             = "\e[m" ;    // Make character non-bold

_PRIVATE unsigned char principal[SSIZE]       = "";
_PRIVATE unsigned char principal_link[SSIZE]  = "";
_PRIVATE unsigned char hostname[SSIZE]        = "";
_PRIVATE unsigned char pname[SSIZE]           = "";
_PRIVATE unsigned char new_pname[SSIZE]       = "";
_PRIVATE unsigned char ppath[SSIZE]           = "";
_PRIVATE uint32_t      pname_pos              = 0;
_PRIVATE _BOOLEAN      principal_is_directory = FALSE;
_PRIVATE _BOOLEAN      do_daemon              = FALSE;
_PRIVATE _BOOLEAN      do_protect             = FALSE;
_PRIVATE _BOOLEAN      do_verbose             = FALSE;
_PRIVATE time_t        start_time             = 0;
_PRIVATE time_t        lifetime               = 60;




/*-----------------------------------------------*/
/* Functions which are local to this application */
/*-----------------------------------------------*/
/*---------------------*/
/* Handler for SIGALRM */
/*---------------------*/

_PRIVATE void exit_handler(void)

{   if (principal_is_directory == TRUE)
    {  unsigned char rm_cmd[SSIZE] = "";

       (void)snprintf(rm_cmd,SSIZE,"rm -rf %s",principal);
       (void)system(rm_cmd);
    }

    else
    {  (void)unlink(principal_link);
       (void)unlink(principal);
    }

    (void)fprintf(stderr,"\n%s (%d@%s): %sWARNING%s lifetime of principal [%s] expired -- deleted\n\n",pname,getpid(),hostname,boldOn,boldOff,principal);
    (void)fflush(stderr);

    exit(0);
}



/*---------------------*/
/* Handler for SIGTERM */
/*---------------------*/

_PRIVATE int32_t term_handler(const int32_t signum)

{   (void)unlink(principal_link);

    if (strncmp(ppath,"/tmp",4) == 0)
       (void)unlink(ppath);

    if (signum == SIGINT)
       (void)fprintf(stderr,"\n%s (%d@%s): terminated by SIGINT\n\n", pname,getpid(),hostname);

    else if (signum == SIGQUIT)
       (void)fprintf(stderr,"\n%s (%d@%s): terminated by SIGQUIT\n\n",pname,getpid(),hostname);

    else if (signum == SIGHUP)
       (void)fprintf(stderr,"\n%s (%d@%s): terminated by SIGHUP\n\n",pname,getpid(),hostname);

    if (signum == SIGTERM)
       (void)fprintf(stderr,"\n%s (%d@%s): terminated by SIGTERM\n\n",pname,getpid(),hostname);

    else
       (void)fprintf(stderr,"\n%s (%d@%s): terminated by signal %d\n",pname,getpid(),hostname,signum);

    (void)fflush(stderr);

    exit(0);
}



/*--------------------------*/
/* (Lifetime) reset handler */
/*--------------------------*/

_PRIVATE int32_t rejuvenation_handler(const int32_t signum)
{
    if (lifetime != FOREVER)
    {  start_time = time((time_t *)NULL);

       if (do_verbose == TRUE)
       {  (void)fprintf(stderr,"\n%s (%d@%s): lifetime (of principal \"%s\") reset (%d seconds)\n\n",pname,getpid(),hostname,principal,lifetime);
          (void)fflush(stderr);
       }
    }

    else
    {  if (do_verbose == TRUE)
       {  (void)fprintf(stderr,"\n%s (%d@%s): lifetime (of principal \"%s\") is unlimited\n\n",pname,getpid(),hostname,principal);
          (void)fflush(stderr);
       }
    }

    return(0);
}




/*----------------*/
/* Status handler */
/*----------------*/

_PRIVATE int32_t status_handler(const int32_t signum)

{   time_t        lifetime_left  = 0;
    unsigned char ftype[SSIZE]   = "";

    lifetime_left = lifetime - (time((time_t *)NULL) - start_time); 

    (void)fprintf(stderr,"\nlyosome lightweight file destructor version %s, (C) Tumbling Dice, 2004-2024 (gcc %s: built %s %s)\n",LYOSOME_VERSION,__VERSION__,__TIME__,__DATE__);

    if (do_verbose == TRUE)
       (void)fprintf(stderr,"    verbose mode                  : on\n");
    else
       (void)fprintf(stderr,"    verbose mode                  : off\n");

    if (do_daemon == TRUE)
       (void)fprintf(stderr,"    daemon mode                   : on\n");
    else
       (void)fprintf(stderr,"    daemon mode                   : off\n");

    if (do_protect == TRUE)
       (void)fprintf(stderr,"    protect mode                  : on\n");
    else
       (void)fprintf(stderr,"    protect mode                  : off\n");

    (void)fprintf(stderr,"    protecting file systems object: \"%s\"\n",principal);

    if (lifetime == FOREVER)
       (void)fprintf(stderr,"    monitoring period             : indefinite\n");
    else
       (void)fprintf(stderr,"    monitoring period             : %04d seconds (%04d seconds left)\n",lifetime,lifetime_left);

    (void)fflush(stderr);

    return(0);
}




/*---------------------------------------*/
/* Extract leaf from the end of pathname */
/*---------------------------------------*/

_PRIVATE _BOOLEAN strleaf(const unsigned char *pathname, unsigned char *leaf)

{   size_t i;

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

_PRIVATE  int32_t absolute_path(const unsigned char *pathname, unsigned char *absolute_pathname)

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




/*------------------*/
/* Main entry point */
/*------------------*/

_PUBLIC  int32_t main(int argc, char *argv[])

{   uint32_t      i,
                  decoded        = 0,
                  p_index        = 1;

    unsigned char tmpstr[SSIZE]  = "";

    sigset_t      set;
    struct stat   statBuf;


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
    /* Parse command tail */
    /*--------------------*/

    if (argc == 1 || strcmp(argv[1],"-usage") == 0 || strcmp(argv[1],"-help") == 0)
    {  (void)fprintf(stderr,"\nlyosome lightweight file destructor version %s, (C) Tumbling Dice, 2004-2024 (gcc %s: built %s %s)\n\n",LYOSOME_VERSION,__VERSION__,__TIME__,__DATE__);

       (void)fprintf(stderr,"LYOSOME is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"LYOSOME comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"Usage: lyosome [-usage] | [-help] |\n");
       (void)fprintf(stderr,"               [-verbose:FALSE]\n");
       (void)fprintf(stderr,"               [-lifetime <destruct delay in seconds:60>]\n");
       (void)fprintf(stderr,"               [-always:FALSE]\n");
       (void)fprintf(stderr,"               [-daemon:FALSE [<name of daemon process>]]\n");
       (void)fprintf(stderr,"               [-protect:FALSE]\n\n");
       (void)fflush(stderr);

       exit(0);
    }

    for (i=0; i<argc-1; ++i)
    {

       /*--------------*/
       /* Verbose mode */
       /*--------------*/

       if (strcmp(argv[i],"-verbose") == 0)
       {  do_verbose = TRUE;
          ++decoded;
       }


       /*-------------*/
       /* Daemon mode */
       /*-------------*/

       if (strcmp(argv[i],"-daemon") == 0)
       {  do_daemon  = TRUE;

          if (i <= argc - 2 && argv[i+1][0] != '-')
          {  (void)sscanf(argv[i+1],"%s",new_pname); 

             if (strcmp(pname,new_pname) == 0)
             {  if (do_verbose == TRUE)
                {  (void)fprintf(stderr,"\n%s (%d@%s): %sERROR%s new and old process names cannot be the same\n\n",pname,getpid(),hostname,boldOn,boldOff);
                   (void)fflush(stderr);
                }

                exit(255);
             }

             pname_pos = i + 1;

             ++i;
             decoded += 2;
          }
          else 
             ++decoded;
       }


       /*-----------------------------------*/
       /* Monitoring lifetime for principle */
       /*-----------------------------------*/

       if (strcmp(argv[i],"-lifetime") == 0)
       {  if (i == argc - 2 || argv[i+1][0] == '-' || sscanf(argv[i+1],"%d",&lifetime) != 1 || lifetime < 0) 
          {  if (do_verbose == TRUE)
             {  (void)fprintf(stderr,"\n%s (%d@%s): %sERROR%s lifetime must be a positive  int32_teger value\n\n",pname,getpid(),hostname,boldOn,boldOff);
                (void)fflush(stderr);
             }

             exit(255);
          }

          ++i;
          decoded += 2;;
       }


       /*---------------------------*/
       /* Monitor principle forever */
       /*---------------------------*/

       else if (strcmp(argv[i],"-always") == 0)
       {  p_index  = 2;
          lifetime = FOREVER;

          ++decoded;
       }


       /*------------------------*/
       /* Homeostatic protection */
       /*------------------------*/
 
       else if (strcmp(argv[i],"-protect") == 0)
       {  do_protect = TRUE;
          ++decoded;
       }
    }
  

    /*--------------------------------------------*/ 
    /* Check for unparsed command line parameters */
    /*--------------------------------------------*/ 
     
    if (decoded < argc - 2)
    {  if (do_verbose == TRUE)
       {  (void)fprintf(stderr,"\n%s (%d@%s): %sERROR%s command tail items unparsed\n\n",pname,getpid(),hostname,boldOn,boldOff);
          (void)fflush(stderr);
       }

       exit(255);
    }
 
    p_index = argc - 1;


    /*---------------------*/
    /* Monitored principal */
    /*---------------------*/
                              
    (void)strlcpy(principal,argv[p_index],SSIZE);
    if (access(principal,F_OK | R_OK | W_OK) == (-1))
    {  if (do_verbose == TRUE)
       {  (void)fprintf(stderr,"\n%s (%d@%s): %sERROR%s cannot find principal to monitor [%s]\n\n",pname,getpid(),hostname,boldOn,boldOff,principal);
          (void)fflush(stderr);
       }

       exit(255);
    }


    /*------------------------*/ 
    /* Set up signal handlers */
    /*------------------------*/ 

    (void)sigemptyset(&set);
    (void)sigprocmask(SIG_SETMASK,&set,(sigset_t *)NULL);

    (void)signal(SIGINT,  (void *)&term_handler);
    (void)signal(SIGQUIT, (void *)&term_handler);
    (void)signal(SIGHUP,  (void *)&term_handler);
    (void)signal(SIGTERM, (void *)&term_handler);
    (void)signal(SIGUSR1, (void *)&status_handler);
    (void)signal(SIGUSR2, (void *)&rejuvenation_handler);

    (void)stat(principal,&statBuf);


    /*-----------*/
    /* Directory */
    /*-----------*/

    if (S_ISDIR(statBuf.st_mode))
    {  if (do_verbose == TRUE)
       {  (void)fprintf(stderr,"\n%s (%d@%s): monitoring principal directory \"%s\" for %d seconds\n\n",pname,getpid(),hostname,principal,lifetime);
          (void)fflush(stderr);
       }

       principal_is_directory = TRUE;


       /*----------------------------*/
       /* Cannot protect directories */
       /*----------------------------*/

       if (do_protect == TRUE)
       {  do_protect = FALSE;

          if (do_verbose == TRUE)
          {  (void)fprintf(stderr,"\n%s (%d@%s): %sERROR%s cannot protect directories (yet)\n",pname,getpid(),hostname,boldOn,boldOff);
             (void)fflush(stderr);
          }
       }
    }


    /*--------------*/
    /* Regular file */
    /*--------------*/

    else if (do_protect == TRUE)
    {  size_t j;

       /*--------------------------------------------------------------*/
       /* If we have an absolute path make protective link on its leaf */
       /*--------------------------------------------------------------*/

       for (j=strlen(principal); j>=0; --j)
       {

          /*----------------------------------*/
          /* Is filename actually a pathname? */ 
          /*----------------------------------*/

          if (principal[j] == '/')
          {  unsigned char leaf  [SSIZE] = "",
                           branch[SSIZE] = "";

             (void)strlcpy(leaf,&principal[j+1],SSIZE);
             (void)strlcpy(branch,principal,SSIZE);
             branch[j+1] = '\0';

             (void)snprintf(principal_link,SSIZE,"%s.%s",branch,leaf);
             goto dot_inserted;
          }
       }


       /*-----------------*/
       /* Simple filename */
       /*-----------------*/

       (void)snprintf(principal_link,SSIZE,".%s.%d.tmp",principal,getpid());

dot_inserted:


       /*-------*/
       /* Error */
       /*-------*/

       if (link(principal,principal_link) == (-1))
       {  if (do_verbose == TRUE)
          {  (void)fprintf(stderr,"\n%sERROR%s %s (%d@%s): could not make protective link to regular file [%s] -- aborting\n\n",boldOn,boldOff,pname,getpid(),hostname,principal);
             (void)fflush(stderr);
          }

          exit(255);
       }


       /*-------------*/
       /* Can protect */
       /*-------------*/

       else
       {  if (do_verbose == TRUE)
          {  (void)fprintf(stderr,"\n%s (%d@%s): protecting regular file \"%s\" for %04d seconds\n\n",pname,getpid(),hostname,principal,lifetime);
             (void)fflush(stderr);
          }
       }
    }


    /*-------------*/
    /* Daemon mode */
    /*-------------*/

    if (do_daemon == TRUE)
    {  unsigned char tmpPathName[SSIZE]  = "";
       uint32_t nargc         = 1;
       char     *nargv[ARGC]  = { (char *)NULL };

       if (strcmp(new_pname,"") == 0)
          (void)snprintf(tmpPathName,SSIZE,"/tmp/lyosome"); 
       else
          (void)snprintf(tmpPathName,SSIZE,"/tmp/%s",new_pname); 

       if (symlink(ppath,tmpPathName) == (-1))
       {  if (do_verbose == TRUE)
          {  (void)fprintf(stderr,"%s (%d@%s): %sERROR%s failed to link (lyosome) binary\n",pname,getpid(),hostname,boldOn,boldOff);
             (void)fflush(stderr);
          }

          exit(255);
       }


       /*----------------------------------*/
       /* Initial argument vector argument */
       /*----------------------------------*/
       /*-------*/
       /* Error */
       /*-------*/

       if ((nargv[0] = (unsigned char *)malloc(SSIZE*sizeof(unsigned char))) == (unsigned char *)NULL)
       {  if (do_verbose == TRUE)
          {  (void)fprintf(stderr,"\n%s (%d@%s): %sERROR%s failed to allocate memory for first argument vector\n\n",pname,getpid(),hostname,boldOn,boldOff);
             (void)fflush(stderr);
          }

          exit(255);
       }


       /*----*/
       /* Ok */
       /*----*/

       else
          (void)strlcpy(nargv[0],tmpPathName,SSIZE);


       for (i=1; i<argc; ++i)
       {  if (i != pname_pos && strcmp(argv[i],"-d") != 0)
          {  

             /*------------------------------*/
             /* Next argument vector element */
             /*------------------------------*/

             if ((nargv[nargc] = (unsigned char *)malloc(SSIZE*sizeof(unsigned char))) != (unsigned char *)NULL)
             {  (void)strlcpy(nargv[nargc],argv[i],SSIZE);
                ++nargc;
             }


             /*-------*/
             /* Error */
             /*-------*/

             else
             {  if (do_verbose == TRUE)
                {  (void)fprintf(stderr,"\n%s (%d@%s): %sERROR%s failed to allocate memory for next argument vector\n\n",pname,getpid(),hostname,boldOn,boldOff);
                   (void)fflush(stderr);
                }

                exit(255);
             }
          }
       }


       /*-------------------------*/
       /* Terminate argument list */
       /*-------------------------*/

       nargv[nargc] = (unsigned char *)NULL;


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

          /*---------------------------*/
          /* Close stdio if not needed */
          /*---------------------------*/

          if (do_verbose == FALSE)
          {  (void)close(0);
             (void)close(1);
             (void)close(2);
          }


          /*-----------------------------------------------*/
          /* Start daemon and make it process group leader */
          /*-----------------------------------------------*/

          (void)setsid();
          (void)execvp(tmpPathName,nargv);


          /*------------------------*/
          /* We should not get here */
          /*------------------------*/

          _exit(255);
       }
    }

    else if (strncmp(ppath,"/tmp",4) == 0)
       (void)unlink(ppath);



    /*----------------------------------*/
    /* Monitor nominated file/directory */
    /*----------------------------------*/

    if (lifetime != FOREVER)
       start_time = time((time_t *)NULL);

    while (TRUE)
    {   if (do_protect == TRUE)
        {  if (access(principal,F_OK | R_OK | W_OK) == (-1))
           {  (void)link(principal_link,principal);
              if (do_verbose == TRUE)
              {  (void)fprintf(stderr,"%s (%d@%s): %sWARNING%s unexpected attempt to delete monitored file system object [%s] -- relinking\n",pname,getpid(),hostname,boldOn,boldOff,principal);
                 (void)fflush(stderr);
              }
           }

           else if (access(principal_link,F_OK | R_OK | W_OK) == (-1))
              (void)link(principal,principal_link);

           else if (access(principal,F_OK | R_OK | W_OK) == (-1) && access(principal_link,F_OK | R_OK | W_OK) == (-1))
           {  if (do_verbose == TRUE)
              {  (void)fprintf(stderr,"\n%s (%d@%s): %sWARNING%s lost monitored file system object [%s] -- aborting\n\n",pname,getpid(),hostname,boldOn,boldOff,principal);
                 (void)fflush(stderr);
              }

              exit(255);
           }
        }


        /*------------------------------*/
        /* Nothing to monitor (so exit) */
        /*------------------------------*/

        else if (access(principal,F_OK | R_OK | W_OK) == (-1))
        {  if (do_verbose == TRUE)
           {  (void)fprintf(stderr,"%s (%d@%s): monitored file system object [%s] deleted -- exiting\n",pname,getpid(),hostname,principal);
              (void)fflush(stderr);
           }

           exit_handler();
        }


        /*-------*/
        /* Delay */
        /*-------*/

        (void)usleep(10000);


        /*----------------------------------------*/
        /* End of (monitoring) lifetime (so exit) */
        /*----------------------------------------*/

        if (lifetime != FOREVER)
        {  if (time((time_t *)NULL) - start_time >= lifetime)
              exit_handler();
        }
    }
}
