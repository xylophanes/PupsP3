/*---------------------------------------------------------------------------------------
     Purpose: Delayed file destructor process 

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.00 
     Dated:   30th August 2019 
     E-mail:  mao@tumblingdice.co.uk
---------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <xtypes.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>


#define _XOPEN_SOURCE
#include <unistd.h>


/*--------------------------------------------------------------------------------------
    Defines which are private to this application ...
--------------------------------------------------------------------------------------*/
/*-------------------*/
/* Vesion of lyosome */
/*-------------------*/

#define  LYOSOME_VERSION    "2.00"
#define  FOREVER            (-9999.0)


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE               2048 


/*--------------------------------------------------------------------------------------
    Variable which are private to this application ...
--------------------------------------------------------------------------------------*/

_PRIVATE char principal[SSIZE]           = "";
_PRIVATE char principal_link[SSIZE]      = "";
_PRIVATE char hostname[SSIZE]            = "";
_PRIVATE char pname[SSIZE]               = "";
_PRIVATE char new_pname[SSIZE]           = "";
_PRIVATE char ppath[SSIZE]               = "";
_PRIVATE int  pname_pos                  = 0;
_PRIVATE _BOOLEAN principal_is_directory = FALSE;
_PRIVATE _BOOLEAN do_protect             = FALSE;
_PRIVATE _BOOLEAN do_verbose             = FALSE;
_PRIVATE time_t   start_time;
_PRIVATE time_t   lifetime               = 60;



/*--------------------------------------------------------------------------------------
    Functions which are local to this application ...
--------------------------------------------------------------------------------------*/


#ifdef BSD_FUNCTION_SUPPORT
/*--------------------------------------------------------------------------------------
    Strlcpy and stlcat function based on OpenBSD functions ...
--------------------------------------------------------------------------------------*/
/*------------------*/
/* Open BSD Strlcat */
/*------------------*/
_PRIVATE size_t strlcat(char *dst, const char *src, size_t dsize)
{
	const char *odst = dst;
	const char *osrc = src;
	size_t      n    = dsize;
	size_t      dlen;


        /*------------------------------------------------------------------*/
	/* Find the end of dst and adjust bytes left but don't go past end. */
        /*------------------------------------------------------------------*/

	while (n-- != 0 && *dst != '\0')
		dst++;
	dlen = dst - odst;
	n = dsize - dlen;

	if (n-- == 0)
		return(dlen + strlen(src));
	while (*src != '\0') {
		if (n != 0) {
			*dst++ = *src;
			n--;
		}
		src++;
	}
	*dst = '\0';


        /*----------------------------*/
        /* count does not include NUL */
        /*----------------------------*/

	return(dlen + (src - osrc));
}




/*------------------*/
/* Open BSD strlcpy */
/*------------------*/

_PRIVATE size_t strlcpy(char *dst, const char *src, size_t dsize)
{
	const char   *osrc = src;
	size_t nleft       = dsize;


        /*---------------------------------*/
	/* Copy as many bytes as will fit. */
        /*---------------------------------*/

	if (nleft != 0) {
		while (--nleft != 0) {
			if ((*dst++ = *src++) == '\0')
				break;
		}
	}


        /*-----------------------------------------------------------*/
	/* Not enough room in dst, add NUL and traverse rest of src. */
        /*-----------------------------------------------------------*/

	if (nleft == 0) {
		if (dsize != 0)

                        /*-------------------*/
                        /* NUL-terminate dst */
                        /*-------------------*/

			*dst = '\0';
		while (*src++)
			;
	}


        /*----------------------------*/
        /* count does not include NUL */
        /*----------------------------*/

	return(src - osrc - 1);
}
#endif /* BSD_FUNCTION_SUPPORT */





/*---------------------*/
/* Handler for SIGALRM */
/*---------------------*/

_PRIVATE void exit_handler(void)

{   if(principal_is_directory == TRUE)
    {  char rm_cmd[SSIZE] = "";

       (void)snprintf(rm_cmd,SSIZE,"rm -rf %s",principal);
       (void)system(rm_cmd);
    }
    else
    {  (void)unlink(principal_link);
       (void)unlink(principal);
    }

    (void)fprintf(stderr,"\n%s (%d@%s): lifetime of principal [%s] expired -- deleted\n\n",pname,getpid(),hostname,principal);
    (void)fflush(stderr);

    exit(0);
}



/*---------------------*/
/* Handler for SIGTERM */
/*---------------------*/

_PRIVATE int term_handler(int signum)

{   (void)unlink(principal_link);

    if(strncmp(ppath,"/tmp",4) == 0)
       (void)unlink(ppath);

    if(signum == SIGTERM)
       (void)fprintf(stderr,"\n%s (%d@%s): terminated by SIGTERM\n\n",pname,getpid(),hostname);
    else if(signum == SIGINT)
       (void)fprintf(stderr,"\n%s (%d@%s): terminated by SIGINT\n\n", pname,getpid(),hostname);
    else
       (void)fprintf(stderr,"\n%s (%d@%s): terminated by signal %d\n",pname,getpid(),hostname,signum);

    (void)fflush(stderr);

    exit(0);
}



/*--------------------------*/
/* (Lifetime) reset handler */
/*--------------------------*/

_PRIVATE int rejuvenation_handler(int signum)
{
    if(lifetime != FOREVER)
    {  start_time = time(NULL);

       if(do_verbose == TRUE)
       {  (void)fprintf(stderr,"\n%s (%d@%s): lifetime (of principal \"%s\") reset (%d seconds)\n\n",pname,getpid(),hostname,principal,lifetime);
          (void)fflush(stderr);
       }
    }
    else
    {  if(do_verbose == TRUE)
       {  (void)fprintf(stderr,"\n%s (%d@%s): lifetime (of principal \"%s\") is unlimited\n\n",pname,getpid(),hostname,principal);
          (void)fflush(stderr);
       }
    }

    return(0);
}




/*----------------*/
/* Status handler */
/*----------------*/

_PRIVATE int status_handler(int signum)

{   time_t lifetime_left;
    char ftype[SSIZE] = "";

    lifetime_left = lifetime - (time((time_t)NULL) - start_time); 

    if(principal_is_directory == TRUE)
      (void)strlcpy(ftype,"(directory)",SSIZE);
    else
      (void)strlcpy(ftype,"(file)",SSIZE);

    if(do_protect)
       (void)fprintf(stderr,"\n%s (%d@%s): (protected) principal %s \"%s\" will expire in %d seconds\n\n",pname,getpid(),hostname,ftype,principal,lifetime_left);
    else
       (void)fprintf(stderr,"\n%s (%d@%s): principal %s \"%s\" will expire in %d seconds\n\n",pname,getpid(),hostname,ftype,principal,lifetime_left);
    (void)fflush(stderr);

    return;
}




/*---------------------------------------*/
/* Extract leaf from the end of pathname */
/*---------------------------------------*/

_PRIVATE _BOOLEAN strleaf(char *pathname, char *leaf)

{   int i;

    for(i=strlen(pathname); i>= 0; --i)
    {  if(pathname[i] == '/')
          break;
    }

    if(i > 0)
       (void)strlcpy(leaf,(char *)&pathname[i+1],SSIZE);
    else
       (void)strlcpy(leaf,pathname,SSIZE);

    return(0);
}




/*----------------------------*/
/* Make sure path is absolute */
/*----------------------------*/

_PRIVATE int absolute_path(char *pathname, char *absolute_pathname)

{   char leaf[SSIZE]         = "",
         current_path[SSIZE] = "";

    if(pathname[0] == '/')
       (void)strlcpy(absolute_pathname,pathname,SSIZE);
    else
    {  (void)strleaf(pathname, leaf);
       (void)getcwd(current_path,SSIZE);
       (void)snprintf(absolute_pathname,SSIZE,"%s/%s",current_path,leaf);
    }

    return(0);
}




/*--------------------------------------------------------------------------------------
    Main entry point to this application ...
--------------------------------------------------------------------------------------*/

_PUBLIC int main(int argc, char *argv[])

{   int i,
        decoded  = 0,
        p_index  = 1;

    char     tmpstr[SSIZE] = "";
    _BOOLEAN do_daemon   = FALSE;

    sigset_t    set;
    struct stat statBuf;

    (void)gethostname(hostname,SSIZE);


    /*--------------------------------------------------*/
    /* Get process name (stripping out branch from path */
    /*--------------------------------------------------*/

    (void)strncpy(ppath,argv[0],SSIZE);
    (void)absolute_path(ppath,tmpstr);
    (void)strlcpy(ppath,tmpstr,SSIZE);
    (void)strleaf(ppath,pname);


    /*---------------------*/
    /* Decode command tail */
    /*---------------------*/

    if(argc == 1 || strcmp(argv[1],"-usage") == 0)
    {  (void)fprintf(stderr,"\nlyosome lightweight file destructor version %s, (C) Tumbling Dice, 2004-2018 (built %s %s)\n",LYOSOME_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"\nUsage: lyosome [-lifetime <destruct delay in seconds:60>] | [-verbose:FALSE]\n");
       (void)fprintf(stderr,"               [-always:FALSE] [-d:FALSE] [-protect:FALSE] !name of file to protect!\n\n");
       (void)fprintf(stderr,"LYOSOME is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"LYOSOME comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fflush(stderr);
       exit(0);
    }

    for(i=0; i<argc-1; ++i)
    {  if(strcmp(argv[i],"-lifetime") == 0)
       {  if(i == argc - 2 || argv[i+1][0] == '-' || sscanf(argv[i+1],"%d",&lifetime) != 1) 
          {  if(do_verbose == TRUE)
             {  (void)fprintf(stderr,"\n%s (%d@%s): time must be a positive integer\n\n",pname,getpid(),hostname);
                (void)fflush(stderr);
             }

             exit(-1);
          }

          ++i;
          decoded += 2;;
       }
       else if(strcmp(argv[i],"-always") == 0)
       {  p_index  = 2;
          lifetime = FOREVER;
          ++decoded;
       }
       else if(strcmp(argv[i],"-protect") == 0)
       {  do_protect = TRUE;
          ++decoded;
       }
       else if(strcmp(argv[i],"-d") == 0)
       {  do_daemon  = TRUE;

          if(i <= argc - 2 && argv[i+1][0] != '-')
          {  (void)sscanf(argv[i+1],"%s",new_pname); 

             if(strcmp(pname,new_pname) == 0)
             {  if(do_verbose == TRUE)
                {  (void)fprintf(stderr,"\n%s (%d@%s): new and old process names cannot be the same\n\n",pname,getpid(),hostname);
                   (void)fflush(stderr);
                }

                exit(-1);
             }

             pname_pos = i + 1;

             ++i;
             decoded += 2;
          }
          else 
             ++decoded;
       }
       else if(strcmp(argv[i],"-verbose") == 0)
       {  do_verbose = TRUE;
          ++decoded;
       }
    }
   
    if(decoded < argc - 2)
    {  if(do_verbose == TRUE)
       {  (void)fprintf(stderr,"\n%s (%d@%s): command tail items unparsed\n\n",pname,getpid(),hostname);
          (void)fflush(stderr);
       }

       exit(-1);
    }
 
    p_index = argc - 1;
                              
    (void)strlcpy(principal,argv[p_index],SSIZE);
    if(access(principal,F_OK | R_OK | W_OK) == (-1))
    {  if(do_verbose == TRUE)
       {  (void)fprintf(stderr,"\n%s (%d@%s): cannot find principal to protect [%s]\n\n",pname,getpid(),hostname,principal);
          (void)fflush(stderr);
       }

       exit(-1);
    }


    /*------------------------*/ 
    /* Set up signal handlers */
    /*------------------------*/ 

    (void)sigemptyset(&set);
    (void)sigprocmask(SIG_SETMASK,&set,(sigset_t *)NULL);

    (void)signal(SIGTERM, (void *)&term_handler);
    (void)signal(SIGINT,  (void *)&term_handler);
    (void)signal(SIGUSR1, (void *)&status_handler);
    (void)signal(SIGUSR2, (void *)&rejuvenation_handler);

    (void)stat(principal,&statBuf);
    if(S_ISDIR(statBuf.st_mode))
    {  if(do_verbose == TRUE)
       {  (void)fprintf(stderr,"\n%s (%d@%s): destroying principal directory \"%s\" after %d seconds\n\n",pname,getpid(),hostname,principal,lifetime);
          (void)fflush(stderr);
       }

       principal_is_directory = TRUE;
       if(do_protect == TRUE)
       {  do_protect = FALSE;

          if(do_verbose == TRUE)
          {  (void)fprintf(stderr,"\n%s (%d@%s): cannot protect directories (yet)\n",pname,getpid(),hostname);
             (void)fflush(stderr);
          }
       }
    }
    else if(do_protect == TRUE)
    {

       /*--------------------------------------------------------------*/
       /* If we have an absolute path make protective link on its leaf */
       /*--------------------------------------------------------------*/

       for(i=strlen(principal); i>=0; --i)
       {

          /*----------------------------------*/
          /* Is filename actually a pathname? */ 
          /*----------------------------------*/

          if(principal[i] == '/')
          {  char leaf[SSIZE]   = "",
                  branch[SSIZE] = "";

             (void)strlcpy(leaf,&principal[i+1],SSIZE);
             (void)strlcpy(branch,principal,SSIZE);
             branch[i+1] = '\0';

             (void)snprintf(principal_link,SSIZE,"%s.%s",branch,leaf);
             goto dot_inserted;
          }
       }


       /*-----------------*/
       /* Simple filename */
       /*-----------------*/

       (void)snprintf(principal_link,SSIZE,".%s.%d.tmp",principal,getpid());

dot_inserted:

       if(link(principal,principal_link) == (-1))
       {  if(do_verbose == TRUE)
          {  (void)fprintf(stderr,"\nERROR %s (%d@%s): could not make protective link to principal [%s] -- aborting\n\n",pname,getpid(),hostname,principal);
             (void)fflush(stderr);
          }

          exit(-1);
       }
       else
       {  if(do_verbose == TRUE)
          {  (void)fprintf(stderr,"\n%s (%d@%s): destroying principal file \"%s\" after %d seconds\n\n",pname,getpid(),hostname,principal,lifetime);
             (void)fflush(stderr);
          }
       }
    }

    if(do_daemon == TRUE)
    {  char tmpPathName[SSIZE] = "";
       int  nargc            = 1;
       char **nargv          = (char **)NULL;

       if(strcmp(new_pname,"") == 0)
          (void)snprintf(tmpPathName,SSIZE,"/tmp/lyosome"); 
       else
          (void)snprintf(tmpPathName,SSIZE,"/tmp/%s",new_pname); 

       if(symlink(ppath,tmpPathName) == (-1))
       {  if(do_verbose == TRUE)
          {  (void)fprintf(stderr,"%s (%d@%s): failed to link (lyosome) binary\n",pname,getpid(),hostname);
             (void)fflush(stderr);
          }

          exit(-1);
       }

       nargv = (char **)calloc(64,sizeof(char *));
       nargv[0] = (char *)malloc(SSIZE*sizeof(unsigned char));
       (void)strlcpy(nargv[0],tmpPathName,SSIZE);

       for(i=1; i<argc; ++i)
       {  if(i != pname_pos && strcmp(argv[i],"-d") != 0)
          {  nargv[nargc] = (char *)malloc(SSIZE*sizeof(unsigned char));
             (void)strlcpy(nargv[nargc],argv[i],SSIZE);

             ++nargc;
          }
       }

       nargv[nargc] = (char *)NULL;

       if(fork() != 0)
          _exit(0);
       else
       {  if(do_verbose == FALSE)
          {  (void)close(0);
             (void)close(1);
             (void)close(2);
          }

          (void)setsid();
          (void)execvp(tmpPathName,nargv);

          _exit(-1);
       }
    }
    else if(strncmp(ppath,"/tmp",4) == 0)
       (void)unlink(ppath);

    if(lifetime != FOREVER)
       start_time = time((time_t)NULL);

    while(TRUE)
    {   if(do_protect == TRUE)
        {  if(access(principal,F_OK | R_OK | W_OK) == (-1))
           {  (void)link(principal_link,principal);
              if(do_verbose == TRUE)
              {  (void)fprintf(stderr,"%s (%d@%s): attempt to delete principal [%s] -- relinking\n",pname,getpid(),hostname,principal);
                 (void)fflush(stderr);
              }
           }
           else if(access(principal_link,F_OK | R_OK | W_OK) == (-1))
              (void)link(principal,principal_link);
           else if(access(principal,F_OK | R_OK | W_OK) == (-1) && access(principal_link,F_OK | R_OK | W_OK) == (-1))
           {  if(do_verbose == TRUE)
              {  (void)fprintf(stderr,"\n%s (%d@%s): lost principal [%s] -- aborting\n\n",pname,getpid(),hostname,principal);
                 (void)fflush(stderr);
              }

              exit(-1);
           }
        }
        else if(access(principal,F_OK | R_OK | W_OK) == (-1))
        {  if(do_verbose == TRUE)
           {  (void)fprintf(stderr,"%s (%d@%s): principal [%s] deleted -- exiting\n",pname,getpid(),hostname,principal);
              (void)fflush(stderr);
           }

           exit(0);
        }

        (void)usleep(10000);

        if(lifetime != FOREVER)
        {  if(time((time_t)NULL) - start_time >= lifetime)
              exit_handler();
        }
    }
}
