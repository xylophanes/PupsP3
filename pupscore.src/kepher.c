/*---------------------------------------------------------------------------------------
     Purpose: Garbage collector 

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 3.00 
     Dated:   30th August 2019 
     E-mail:  mao@tumblingdice.co.uk
---------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <xtypes.h>


/*---------------------------------------------------------------------------------------
    Definitions which are local to this application ...
---------------------------------------------------------------------------------------*/
/*-------------------*/
/* Version of kepher */
/*-------------------*/
 
#define KEPHER_VERSION    "3.00"
#define EXTENSION_LIST    "fifo tmp run agf pheap lid lock"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE             2048 



/*---------------------------------------------------------------------------------------
    Variables which are private to this application ...
---------------------------------------------------------------------------------------*/

_PRIVATE char     hostname[SSIZE]            = "";
_PRIVATE char     mdir[SSIZE]                = "/tmp";
_PRIVATE char     pname[SSIZE]               = "";
_PRIVATE char     kepherRun[SSIZE]           = "";
_PRIVATE char     date[SSIZE]                = "";
_PRIVATE _BOOLEAN do_verbose                 = FALSE;
_PRIVATE _BOOLEAN do_delete                  = FALSE;
_PRIVATE _BOOLEAN do_protect                 = FALSE;
_PRIVATE _BOOLEAN do_exit_empty              = FALSE;
_PRIVATE DIR      *dirp                      = (DIR *)NULL;
_PRIVATE struct   dirent *next_item          = (struct dirent *)NULL;
_PRIVATE char     nextFile[SSIZE]            = "";
_PRIVATE char     nextPathFile[SSIZE]        = "";
_PRIVATE char     nextPathShadow[SSIZE]      = "";
_PRIVATE char     ignoreExtensionList[SSIZE] = "";
_PRIVATE int      nfiles                     = 0;





/*---------------------------------------------------------------------------------------
    Local functions ...
---------------------------------------------------------------------------------------*/


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




#ifdef HAVE_PROCFS
/*---------------------------------------------------------------------------------------
   Translate process name to PID ...
---------------------------------------------------------------------------------------*/

_PRIVATE int getPidFromPname(char *pname)

{   DIR    *pdirp            = (DIR *)NULL;
    struct dirent *next_item = (struct dirent *)NULL;

    int    pid   = (-1),
           found = 0;

    pdirp = opendir("/proc");
    while((next_item = readdir(pdirp)) != (struct dirent *)NULL)
    {   int next_pid = (-1);

        if(sscanf(next_item->d_name,"%d",&next_pid) == 1)
        {  FILE *stream       = (FILE *)NULL;

           char pathname[SSIZE]   = "",
                strdum[SSIZE]     = "",
                next_pname[SSIZE] = "";

           (void)snprintf(pathname,SSIZE,"/proc/%s/status",next_item->d_name);
           if((stream = fopen(pathname,"r")) != (FILE *)NULL)
           {  (void)fscanf(stream,"%s%s",strdum,next_pname);
              (void)fclose(stream);
           }

           if(strcmp(pname,next_pname) == 0)
           {  ++found;
              pid = next_pid;
           }
        }
    }

    if(found != 1)
       return(-1);

    return(pid);
}
#endif /* HAVE_PROCFS */




/*------------------------------------------------------------------------------------
    Look for the occurence of string s2 within string s1 ...
------------------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN strin(char *s1, char *s2)

{   int i,
        cmp_size,
        chk_limit;

    if(strlen(s2) > strlen(s1))
       return(FALSE);

    chk_limit = strlen(s1) - strlen(s2) + 1;
    cmp_size  = strlen(s2);

    for(i=0; i<chk_limit; ++i)
    {  if(strncmp(&s1[i],s2,cmp_size) == 0)
          return(TRUE);
    }

    return(FALSE);
}




/*-------------------------------------------------------------------------------------
    Return current date ...
-------------------------------------------------------------------------------------*/

_PRIVATE void strdate(char *date)

{   time_t tval;

    (void)time(&tval);
    (void)strcpy(date,ctime(&tval));
    date[strlen(date) - 1] = '\0';
}




/*-------------------------------------------------------------------------------------
    Extract process identifier from file name
-------------------------------------------------------------------------------------*/

_PRIVATE int getPidFromFname(char *fname)

{   int i,
        pid  = (-1),
        cnt  = 0,
        cpos = 0;

    char tmpstr[SSIZE]   = "",
         strdum[SSIZE]   = "",
         filePath[SSIZE] = "";

    struct stat statBuf;

    (void)snprintf(filePath,SSIZE,"%s/%s",mdir,fname); 
    (void)stat(filePath,&statBuf);

    if(getuid() != statBuf.st_uid)
    {  (void)lstat(filePath,&statBuf);
       if(getuid() != statBuf.st_uid)
          return(-1);
    }


    /*-------------------------------------*/
    /* Extract PID                         */
    /* Stale PSRP/P3 communication channel */
    /*-------------------------------------*/

    if(strncmp(fname,"psrp#",5) == 0)
    {  (void)strcpy(tmpstr,fname);

       for(i=0; i<strlen(tmpstr); ++i)
       {  if(tmpstr[i] =='#' || tmpstr[i] == '.' || tmpstr[i] == ':')
             tmpstr[i] = ' ';
       }

       if(sscanf(tmpstr,"%s%s%s%s%s%d",strdum,strdum,strdum,strdum,strdum,&pid) == 6)
          return(pid);
       else
          return(-1);
    }


    /*------------------------------*/
    /* Stale PSRP/P3 stdio lockpost */
    /*------------------------------*/

    else if(strncmp(fname,"pups.",5) == 0)
    {  (void)strcpy(tmpstr,fname);

       for(i=0; i<strlen(tmpstr); ++i)
       {  if(tmpstr[i] =='#' || tmpstr[i] == '.' || tmpstr[i] == ':')
             tmpstr[i] = ' ';
       }

       if(sscanf(tmpstr,"%s%s%s%s%s%d",strdum,strdum,strdum,strdum,strdum,&pid) == 6)
          return(pid);
       else
          return(-1);

    }


    /*----------------------------------------*/
    /* Other file which may have become stale */
    /*----------------------------------------*/

    else
    {  char tmpstr2[SSIZE] = "";

       (void)strcpy(tmpstr,fname);
       for(i=strlen(tmpstr); i>0; --i)
       {   if(tmpstr[i] == '.' || tmpstr[i] == '_' || tmpstr[i] =='#' || tmpstr[i] ==':')
           {  tmpstr[i] = ' ';
              ++cnt;
              cpos = i;

              if(cnt == 3)
                 break;
           }
       }

       (void)strcpy(tmpstr2,(char *)&tmpstr[cpos]);
       (void)strcpy(tmpstr,tmpstr2);

       if(cnt >= 2)
       {  char extension[SSIZE]  = "",
               extension2[SSIZE] = "";


          /*---------------------------------*/
          /* Files of form ...<pid>.<ext>... */
          /*---------------------------------*/

          if(sscanf(tmpstr,"%d%s",&pid,extension)          == 2    ||
             sscanf(tmpstr,"%s%d%s",strdum,&pid,extension) == 3     )
          {  


             /*---------------------------------------------------*/
             /* These are the file types we are allowed to remove */
             /*---------------------------------------------------*/

             if(strin(ignoreExtensionList,extension) == TRUE)
                return(-1);
             else if(strin(EXTENSION_LIST,extension) == FALSE)
                return(-1);
             else
                return(pid);
          }


          /*---------------------------------------*/
          /* Files of form ...<pid>.<ext>.<ext>... */
          /*---------------------------------------*/

          else if(sscanf(tmpstr,"%d%s%s",&pid,extension2,extension) == 3)
          {  

             /*---------------------------------------------------*/
             /* These are the file types we are allowed to remove */
             /*---------------------------------------------------*/

             if(strin(ignoreExtensionList,extension) == TRUE)
                return(-1);
             else if(strin(EXTENSION_LIST,extension) == FALSE)
                return(-1);
             else
                return(pid);

          }
          else if(sscanf(tmpstr,"%s%s%d",extension,extension2,&pid) == 3)
          {  if((strcmp(extension,"in") != 0 || strcmp(extension,"out") != 0) && strcmp(extension,"ipm") != 0)
                 return(-1);
          } 
       }
    }

    return(-1);
}




/*-------------------------------------------------------------------------------------
    Delete a file ...
-------------------------------------------------------------------------------------*/

_PRIVATE int deleteFile(_BOOLEAN do_verbose, _BOOLEAN shadowed, int pid, char *fname)

{   char rm_cmd[SSIZE]  = "",
         pidInfo[SSIZE] = "";


    /*---------------------*/
    /* Delete primary file */
    /*---------------------*/

    (void)snprintf(rm_cmd,SSIZE,"rm -rf %s/%s",mdir,fname);
    (void)system(rm_cmd);


    /*---------------*/
    /* Delete shadow */
    /*---------------*/

    if(do_protect == TRUE)
    {  (void)snprintf(rm_cmd,SSIZE,"rm -rf %s/.%s",mdir,fname);
       (void)system(rm_cmd);
    }

    if(shadowed == TRUE)
       --nfiles;

    if(pid != (-1))
       (void)snprintf(pidInfo,SSIZE," (monitor pid %d) ",pid);

    if(do_verbose == TRUE)
    {  (void)strdate(date);
       if(strncmp(fname,"psrp#",5) == 0)
       {  (void)fprintf(stderr,"%s kepher (%d@%s): stale PSRP/P3 communication channel \"%s\"%sremoved (from \"%s\")\n",
                                                                              date,getpid(),hostname,fname,pidInfo,mdir);
       }
       else if(strncmp(fname,"pups.",5) == 0)
       {  (void)fprintf(stderr,"%s kepher (%d@%s): stale PSRP/P3 stdio lockpost \"%s\"%sremoved (from \"%s\"\n",
                                                                      date,getpid(),hostname,fname,pidInfo,mdir);
       }
       else
       {  (void)fprintf(stderr,"%s kepher (%d@%s): stale file \"%s\"%sremoved (from \"%s\")\n",
                                                     date,getpid(),hostname,fname,pidInfo,mdir);
       }
       (void)fflush(stderr);
    }


    /*-----------------------------------------------------------------------*/
    /* If we have a lid file we also need to smash the associated stale lock */
    /*-----------------------------------------------------------------------*/

    if(strin(fname,".lid.") == TRUE)
    {  int i;

       for(i=0; i<strlen(fname) - 4; ++i)
       {  if(strncmp((char *)&fname[i],".lid",4) == 0)
          {  char tmpstr[SSIZE] = "";


             (void)strcpy(tmpstr,fname);
             tmpstr[i] = '\0';

             (void)snprintf(rm_cmd,SSIZE,"rm -f %s/%s.lock",mdir,tmpstr);
             (void)system(rm_cmd);

             if(do_protect == TRUE)
             {  (void)snprintf(rm_cmd,SSIZE,"rm -f %s/.%s.lock",mdir,tmpstr);
                (void)system(rm_cmd);
             }

             if(do_verbose == TRUE)
             {  (void)strdate(date);
                (void)fprintf(stderr,"%s kepher (%d@%s): stale lock \"%s.lock\" smashed (in \"%s\")\n",date,getpid(),hostname,tmpstr,mdir);
                (void)fflush(stderr);
             }
             break;
          }
       }
    }
    
    return(0);
}




/*-------------------------------------------------------------------------------------
    Create shadow files ...
-------------------------------------------------------------------------------------*/

_PRIVATE int create_shadowFiles(_BOOLEAN do_create_shadowfiles, DIR *dirp)

{   char fileName[SSIZE]   = "",
         shadowFile[SSIZE] = "";

    struct stat buf;

    (void)rewinddir(dirp);
    while((next_item = readdir(dirp)) != (struct dirent *)NULL)
    {   if(strcmp(next_item->d_name,".") != 0 && strcmp(next_item->d_name,".") != 0)
        {  (void)snprintf(fileName,SSIZE,"%s/%s",mdir,next_item->d_name); 
           (void)stat(fileName,&buf);


           /*-------------------------------*/
           /* Directories are not protected */
           /*-------------------------------*/

           if(next_item->d_name[0] != '.' && !S_ISDIR(buf.st_mode))
           {  ++nfiles;

              if(do_create_shadowfiles == TRUE)
              {  (void)snprintf(shadowFile,SSIZE,"%s/.%s",mdir,next_item->d_name);

                 if(link(fileName,shadowFile) == (-1))
                 {  if(do_verbose == TRUE)
                    {  (void)strdate(date);
                       (void)fprintf(stderr,"%s (%d@%s): WARNING could not link shadow file to \"%s\"\n",date,getpid(),hostname,next_item->d_name);
                       (void)fflush(stderr);
                    }
                 }
              } 
           }
        }
    }

    return(nfiles);
}




/*-------------------------------------------------------------------------------------
    Destroy shadow files ...
-------------------------------------------------------------------------------------*/

_PRIVATE int destroy_shadowFiles(void)

{   char        shadowFile[SSIZE] = "";
    struct stat buf;

    while((next_item = readdir(dirp)) != (struct dirent *)NULL)
    {   if(strcmp(next_item->d_name,".") != 0 && strcmp(next_item->d_name,"..") != 0)
        {  (void)snprintf(shadowFile,SSIZE,"%s/%s",mdir,next_item->d_name);
           (void)stat(next_item->d_name,&buf);

           if(shadowFile[0] == '.')
           {  if(unlink(shadowFile) == (-1))
              {  if(do_verbose == TRUE)
                 {  (void)strdate(date);
                    (void)fprintf(stderr,"%s (%d@%s): WARNING could not unlink shadow file from \"%s\"\n",date,getpid(),hostname,(char *)&next_item->d_name[1]);
                    (void)fflush(stderr);
                 }
              }
           }
        }
    }

    return(0);
}




/*-------------------------------------------------------------------------------------
    Handle SIGTERM, SIGINT, SIGQUIT, SIGHUP ...
-------------------------------------------------------------------------------------*/

_PRIVATE int term_handler(int signum)

{    if(do_verbose == TRUE)
     {  (void)strdate(date);
        (void)fprintf(stderr,"%s kepher (%d@%s): exiting (signal %d)\n",date,getpid(),hostname,signum);
        (void)fflush(stderr);
     }

     (void)unlink(kepherRun);


     /*-------------------------------------------*/
     /* Delete the monitor directory if specified */
     /*-------------------------------------------*/

     if(do_delete == TRUE)
     {  char rm_cmd[SSIZE] = "";

        (void)closedir(dirp);

        if(do_verbose == TRUE)
        {  (void)strdate(date);
           (void)fprintf(stderr,"%s kepher (%d@%s): monitor directory \"%s\" deleted\n",date,getpid(),hostname,mdir);
           (void)fflush(stderr);
        }

        (void)snprintf(rm_cmd,SSIZE,"rm -rf %s",mdir);
        (void)system(rm_cmd);
     }
     else if(do_protect == TRUE)
     {  (void)destroy_shadowFiles();
        (void)closedir(dirp);
     }

     exit(1);
}




/*-------------------------------------------------------------------------------------
    Ignore files which have file extensions which are on the ignore list ...
-------------------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN ignoreFile(char *fileName)

{   char     *extn_ptr = (char *)NULL;
    _BOOLEAN looper    = TRUE;

    extn_ptr = rindex(fileName,'.');
    if(extn_ptr == (char *)NULL)

       /*-----------------------*/
       /* File has no extension */
       /*-----------------------*/

       return(FALSE);
    else
       ++extn_ptr;

    if(strin(EXTENSION_LIST,extn_ptr) == TRUE)
    {

       /*--------------------------------------------*/
       /* We are ignoring files with given extension */
       /*--------------------------------------------*/

       return(TRUE);
    }

    return(FALSE);
}




/*-------------------------------------------------------------------------------------
    Main entry point for application ...
-------------------------------------------------------------------------------------*/

_PUBLIC int main(int argc, char *argv[])

{   int      i;
    int      decoded           = 0;
    int      mpid              = (-1);
    _BOOLEAN looper            = TRUE;
    struct   stat statBuf;
    int      nextPid           = (-1);
    int      kpid              = (-1);
    FILE     *krstream         = (FILE *)NULL;

    (void)gethostname(hostname,SSIZE);


    /*--------------------*/
    /* Parse command tail */
    /*--------------------*/

    for(i=0; i<argc; ++i)
    {  if(strcmp(argv[i],"-usage") == 0 || strcmp(argv[i],"-help") == 0)
       {  (void)fprintf(stderr,"\nKEPHER is free software, covered by the GNU General Public License, and you are\n");
          (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
          (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
          (void)fprintf(stderr,"KEPHER comes with ABSOLUTELY NO WARRANTY\n\n");
          (void)fprintf(stderr,"\nkepher lightweight garbage collector version %s, (C) Tumbling Dice, 2010-2019 (built %s %s)\n",KEPHER_VERSION,__TIME__,__DATE__);
          (void)fprintf(stderr,"Usage: kepher [-help | -usage] | [-verbose:FALSE]\n");
          (void)fprintf(stderr,"              [-mpid <monitor pid | monitor pname>]\n"); 
          (void)fprintf(stderr,"              [-mdir <directory to clean:/tmp>]\n");
          (void)fprintf(stderr,"              [-ignore <file list>]\n");
          (void)fprintf(stderr,"              [-protect:FALSE]\n");
          (void)fprintf(stderr,"              [-delete:FALSE]\n");
          (void)fprintf(stderr,"              [-exit_empty:FALSE]\n");
          (void)fprintf(stderr,"              [>& <error/log file>]\n\n");
          (void)fflush(stderr);

          exit(0);
       }
       else if(strcmp(argv[i],"-verbose") == 0)
       {  do_verbose = TRUE;
          ++decoded;
       }
       else if(strcmp(argv[i],"-protect") == 0)
       {  do_protect = TRUE;
          ++decoded;
       }
       else if(strcmp(argv[i],"-delete") == 0)
       {  do_delete = TRUE;
          ++decoded;
       }
       else if(strcmp(argv[i],"-exit_empty") == 0)
       {  do_exit_empty = TRUE;
          ++decoded;
       }
       else if(strcmp(argv[i],"-mdir") == 0)
       {  if(i == argc - 1 || argv[i+1][0] == '-')
          {  if(do_verbose == TRUE)
             {  (void)strdate(date);
                (void)fprintf(stderr,"%s kepher (%d@%s): EFFOR expecting monitor directory name\n",
                                                                            date,getpid(),hostname);
                (void)fflush(stderr);
             }

             exit(-1);
          }
          (void)strcpy(mdir,argv[i+1]);

          ++i;
          decoded += 2;
       }
       else if(strcmp(argv[i],"-ignore") == 0)
       {  if(i == argc - 1 || argv[i+1][0] == '-')
          {  if(do_verbose == TRUE)
             {  (void)strdate(date);
                (void)fprintf(stderr,"%s kepher (%d@%s): EFFOR expecting list of (space-separeted) file extensions\n",
                                                                                               date,getpid(),hostname);
                (void)fflush(stderr);
             }

             exit(-1);
          }
          (void)strcpy(ignoreExtensionList,argv[i+1]);

          ++i;
          decoded += 2;
       }
       else if(strcmp(argv[i],"-mpid") == 0)
       {  if(i == argc - 1 || argv[i+1][0] == '-')
          {  if(do_verbose == TRUE)
             {  (void)strdate(date);
                (void)fprintf(stderr,"%s kepher (%d@%s): ERROR expecting monitor pid name\n",date,getpid(),hostname);
                (void)fflush(stderr);
             }

             exit(-1);
          }

          if(sscanf(argv[i+1],"%d",&mpid) != 1 || mpid <= 0)
          {

             #ifndef HAVE_PROCFS
             if(do_verbose == TRUE)
             {  (void)fprintf(stderr,"kepher (%d@%s): ERROR monitor process must be interger (>0)\n",getpid(),hostname);
                (void)fflush(stderr);
             }
             exit(-1);
             #endif /* !HAVE_PROCFS */

             #ifdef HAVE_PROCFS
             (void)sscanf(argv[i+1],"%s",pname);
             if((mpid = getPidFromPname(pname)) == (-1))
             {  if(do_verbose == TRUE)
                {  (void)fprintf(stderr,"kepher (%d@%s): ERROR process \"%s\" not parsed (does not exist or not unique)\n",
                                                                                                   getpid(),hostname,pname);
                   (void)fflush(stderr);
                }
                exit(-1);
             }
             #endif /* HAVE_PROCFS */

          }

          ++i;
          decoded += 2;
       }

    }

    if(do_verbose == TRUE)
    {  (void)fprintf(stderr,"\n    kepher lightweight garbage collector (version %s)\n    (C) M.A. O'Neill, Tumbling Dice, 2010-2019\n",
                                                                                                                         KEPHER_VERSION);
       (void)fprintf(stderr,"\n    Process %d on host \"%s\" monitoring \"%s\" for stale .tmp, .fifo, .run, .lock and .lid files\n",
                                                                                                             getpid(),hostname,mdir);
       (void)fprintf(stderr,"    Stale PSRP/P3 communications channels and lockposts will also be removed\n");

       if(mpid != (-1))
          #ifdef HAVE_PROCFS
          (void)fprintf(stderr,"    Process will terminate if monitored process \"%s\" (%d) is terminated\n",pname,mpid);
          #else
          (void)fprintf(stderr,"    Process will terminate if monitored process (%d) is terminated\n",mpid);
          #endif /* HAVE_PROCFS */

       if(do_exit_empty == TRUE)
       {  (void)fprintf(stderr,"    Process will terminate if monitored directory \"%s\" (%d) is empty\n",mdir,mpid);
          (void)fflush(stderr);
       }

       (void)fprintf(stderr,"\n");
       (void)fflush(stderr);
    }


    /*----------------------------------------------*/
    /* Is someone already monitoring this directory */
    /*----------------------------------------------*/

    (void)snprintf(kepherRun,SSIZE,"%s/kepher.run",mdir);
    if(access(kepherRun,F_OK | R_OK | W_OK) == (-1))
    {  if(close(creat(kepherRun,0600)) == (-1))
       {  if(do_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s kepher (%d@%s): ERROR failed to create run file in \"%s\" (are permissions correct?)\n",
                                                                                                  date,getpid(),hostname,mdir);
             (void)fflush(stderr);
          }

          exit(-1);
       }
       else if(do_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s kepher (%d@%s): created run file in \"%s\"\n",date,getpid(),hostname,mdir);
          (void)fflush(stderr);
       }

       krstream = fopen(kepherRun,"w");
       (void)fprintf(krstream,"%d\n",getpid());
       (void)fclose(krstream);
    }
    else
    {  if((krstream = fopen(kepherRun,"r+")) == (FILE *)NULL)
       {  if(do_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s kepher (%d@%s): ERROR failed to open run file in \"%s\" (are permissions correct?)\n",
                                                                                                date,getpid(),hostname,mdir);
             (void)fflush(stderr);
          }

          exit(-1);
       }

       (void)fscanf(krstream,"%d",&kpid);
       if(kpid != getpid() && kill(kpid,SIGCONT) != (-1))
       {  if(do_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s kepher (%d@%s): ERROR \"%s\" is already being monitored (by kepher process %d)\n",
                                                                                       date,getpid(),hostname,mdir,kpid);
             (void)fflush(stderr);
          }

          exit(1);
       }
       else
       {  if(do_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s kepher (%d@%s): smashing kepher lock (for dead kepher process %d)\n",
                                                                               date,getpid(),hostname,kpid);
             (void)fflush(stderr);
          }

          (void)rewind(krstream);
          (void)fprintf(krstream,"%d\n",getpid());
          (void)fflush(krstream);
       }

       (void)fclose(krstream);
    }


    /*----------------------------*/
    /* Initialise signal handlers */
    /*----------------------------*/

    (void)signal(SIGTERM,(void *)&term_handler);
    (void)signal(SIGINT ,(void *)&term_handler);
    (void)signal(SIGQUIT,(void *)&term_handler);
    (void)signal(SIGHUP ,(void *)&term_handler);


    /*------------------------------------------*/
    /* Check all command line items are decoded */
    /*------------------------------------------*/

    if(decoded != argc - 1)
    {  if(do_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s kepher (%d@%s): command tail items unparsed\n",
                                                           date,getpid(),hostname);
       }

       exit(-1);
    }


    /*-----------------------------------------------------------------------*/
    /* Monitor list of files - as soon as owner is terminated (and fails to  */
    /* delete the file) remove it so system is not drowned in stale files    */
    /* This program is named in honour of the dung beetle Kepher nigroaeneus */
    /* which performs a similar task on the lowveld in East and South Africa */
    /*-----------------------------------------------------------------------*/

    if(access(mdir,F_OK  | R_OK | W_OK) == (-1))
    {  if(do_verbose == TRUE)
       {  if(do_verbose == TRUE)
          {  (void)fprintf(stderr,"%s kepher (%d@%s): ERROR cannot find/access directory \"%s\"\n",
                                                                       date,getpid(),hostname,mdir);
             (void)fflush(stderr);
          }
       }

       exit(-1);
    }


    /*----------------------------*/
    /* Monitor nominate directory */
    /*----------------------------*/

    dirp = opendir(mdir);


    /*--------------------------------------------*/
    /* Build shadow files for nominated directory */
    /*--------------------------------------------*/

    if(do_protect == TRUE)
       (void)create_shadowFiles(TRUE,dirp);
    else
       (void)create_shadowFiles(FALSE,dirp);

    while(looper == TRUE)
    {   if(mpid != (-1) && kill(mpid,SIGCONT) == (-1))
           looper = FALSE;
        else if(do_exit_empty == TRUE && nfiles <= 0)
           looper = FALSE;
 
        if(looper == TRUE)
        {  if((next_item = readdir(dirp)) == (struct dirent *)NULL)
              (void)rewinddir(dirp);
           else if(strcmp(next_item->d_name,".") != 0 && strcmp(next_item->d_name,"..") != 0)
           {   (void)strcpy(nextFile,next_item->d_name);


               /*-----------------------------------------*/
               /* Files associated with PUPS/P3 processes */ 
               /* and shells                              */
               /*-----------------------------------------*/

               if((nextPid = getPidFromFname(nextFile)) != (-1))
               {  if(kill(nextPid,SIGCONT) == (-1))
                     (void)deleteFile(TRUE, FALSE, nextPid, nextFile);
               }


               /*--------------------------------------*/
               /* Protect regular files (with shadows) */
               /*--------------------------------------*/

               else if(do_protect == TRUE)
               {

                  /*------------------------------------------*/
                  /* Shadow exists but shadowed file does not */
                  /* relink shadowed file                     */
                  /*------------------------------------------*/

                  (void)snprintf(nextPathFile,SSIZE,"%s/%s",mdir,(char *)&nextFile[1]);
                  if(nextFile[0] == '.' && access(nextPathFile,F_OK | R_OK | W_OK) == (-1))
                  {  (void)snprintf(nextPathShadow,SSIZE,"%s/%s",mdir,nextFile); 
                     (void)link(nextPathShadow,nextPathFile);
   
                     if(do_verbose == TRUE)
                     {  (void)strdate(date);
                        (void)fprintf(stderr,"%s kepher (%d@%s): file \%s\" lost - relinking\n",date,getpid(),hostname,(char *)&nextFile[1]); 
                        (void)fflush(stderr);
                     }
                  }


                  /*-------------------------------------*/
                  /* File without shadow - delete it     */
                  /* if it doesn't have a file extension */
                  /* we are ignoring                     */
                  /*-------------------------------------*/

                  else if(nextFile[0] != '.')
                  {  (void)snprintf(nextPathShadow,SSIZE,"%s/.%s",mdir,nextFile);
                    
                     if(access(nextPathShadow,F_OK | R_OK | W_OK) == (-1))
                     {  if(ignoreFile(nextFile) == FALSE)
                        {  if(strcmp(nextFile,"kepher.run") != 0)
                           {  (void)deleteFile(FALSE, TRUE, (-1), nextFile);

                              if(do_verbose == TRUE)
                              {  (void)strdate(date);
                                 (void)fprintf(stderr,"%s kepher (%d@%s): file \%s\" has no shadow - deleting\n",date,getpid(),hostname,nextFile); 
                                 (void)fflush(stderr);
                              }
                           }
                           else
                           {  (void)snprintf(nextPathFile,SSIZE,"%s/%s",mdir,nextFile);
                              (void)link(nextPathFile,nextPathShadow);
 
                              if(do_verbose == TRUE)
                              {  (void)strdate(date);
                                 (void)fprintf(stderr,"%s kepher (%d@%s): cannot delete \"kepher.run\"\n",date,getpid(),hostname); 
                                 (void)fflush(stderr);
                              }
                           }
                        }
                     }
                  }
               }
            }

            (void)usleep(10000);
        }
    }

    if(do_verbose == TRUE && kill(mpid,SIGCONT) == (-1))
    {  (void)strdate(date);

       #ifdef HAVE_PROCFS
       (void)fprintf(stderr,"%s kepher (%d@%s): Finished (monitor process \"%d (%s)\" has exited)\n",
                                                                   date,getpid(),hostname,mpid,pname);
       #else
       (void)fprintf(stderr,"%s kepher (%d@%s): Finished (monitor process \"%d\" has exited)\n",
                                                                    date,getpid(),hostname,mpid);
       #endif /* HAVE_PROCFS */

       (void)fflush(stderr);
    }

    (void)term_handler(9999);
}
