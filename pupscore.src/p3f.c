/*-------------------------------------------------------------------------------------
     Purpose: Test to see if a given process is PUPS/P3 aware.

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.00 
     Dated:   2nd September 2019 
     E-mail:  mao@tumblingdice.co.uk
------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <dirent.h>
#include <xtypes.h>
#include <sys/types.h>
#include <sys/stat.h>


/*------------------------------------------------------------------------------------
     Get signal mapping appropriate to OS and hardware architecture ...
------------------------------------------------------------------------------------*/

#define __DEFINE__
#if defined(I386) || defined(X86_64)
#include <sig.linux.x86.h>
#endif

#ifdef ARMV6L
#include <sig.linux.arm.h>
#endif /* ARMV6L */

#ifdef ARMV7L
#include <sig.linux.arm.h>
#endif /* ARMV7L */

#ifdef AARCH64
#include <sig.linux.arm.h>
#endif /* AARCH64 */
#undef __DEFINE__




/*-----------------------------------------------------------------------------------
    Defines which are local to this application ...
-----------------------------------------------------------------------------------*/
/*----------------*/
/* Version of p3f */
/*----------------*/

#define P3F_VERSION    "2.00"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE          2048 




#ifdef BSD_FUNCTION_SUPPORT
/*-----------------------------------------------------------------------------
    Strlcpy and stlcat function based on OpenBSD functions ...
-----------------------------------------------------------------------------*/
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





/*----------------------------------------------------------------------------------
    Convert (local) pidname to list of matching PIDS ... 
----------------------------------------------------------------------------------*/

_PRIVATE int local_pname_to_pid(FILE      *stream,
                                _BOOLEAN  binname,
                                _BOOLEAN  *is_pid,
                                char     *pidname)


{  int  pid,
        cnt                 = 0,
        looper              = TRUE;

   char strdum[SSIZE]       = "",
        cmdline[SSIZE]      = "",
        ps_cmd[SSIZE]       = "";

   FILE          *pstream   = (FILE *)NULL;
   DIR           *dirp      = (DIR *)NULL;
   struct dirent *next_item = (struct dirent *)NULL;


   /*---------------------------------------------------------------*/
   /* If pname is numeric (e.g. a PID) simply convert it and return */
   /*---------------------------------------------------------------*/

   *is_pid = TRUE;
   if(sscanf(pidname,"%d",&pid) == 1)
   {  if(pid <= 1)
      {  if(stream != (FILE *)NULL)
         {  (void)fprintf(stream,"freeze: %d is not a valid PID\n",pid);
            (void)fflush(stream);
         }

         return(-1);
      }

      return(pid);
   }
   else
      *is_pid = FALSE;

   dirp = opendir("/proc");


   /*----------------------------------*/
   /* Scan proc filesystem for pidname */
   /*----------------------------------*/

   while((next_item = readdir(dirp)) != (struct dirent *)NULL)
   {

        /*----------------------------------------*/
        /* Is next item a process (sub)directory? */
        /*----------------------------------------*/

        if(sscanf(next_item->d_name,"%d",&pid) == 1)
        {    int i,
                 nb,
                 fdes = (-1);

           char procpidpath[SSIZE] = "";

           if(binname == TRUE)
           {  (void)snprintf(procpidpath,SSIZE,"/proc/%d/cmdline",pid);
              fdes = open(procpidpath,0);
              nb   = read(fdes,cmdline,SSIZE);
              cmdline[nb] = '\0';
              (void)close(fdes);
           }
           else
           {  /* Get name of binary */
              (void)snprintf(procpidpath,SSIZE,"/proc/%d/exe",pid);
              (void)readlink(procpidpath,cmdline,SSIZE);
           }


           /*------------*/
           /* Strip path */
           /*------------*/

           for(i=strlen(cmdline); i>0; --i)
           {   if(cmdline[i] == '/')
                  goto stripped;
           }

stripped:

           if(strncmp(cmdline,pidname,strlen(pidname)) == 0 || strncmp(&cmdline[i+1],pidname,strlen(pidname)) == 0)
           {  if(cnt == 1)
              {  (void)closedir(dirp);

                 if(stream != (FILE *)NULL)
                 {  (void)fprintf(stream,"freeze: process %s is not uniquely named\n",pidname);
                    (void)fflush(stream);
                 }

                 return(-1);
              }
              else
                 return(pid);
              ++cnt;
           }
        }
   }

   return(-1);
}




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





/*-----------------------------------------------------------------------------------
    Is our process P3 aware ...
-----------------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN p3_aware(int pid, char *chan_dir)

{   DIR           *dirp         = (DIR *)NULL;
    struct dirent *next_item    = (struct dirent *)NULL;
    char          pidstr[SSIZE] = "";


    /*---------------------------------------*/
    /* Asdsuming default P3 patchboard here! */ 
    /*---------------------------------------*/
 
    dirp = opendir(chan_dir); 
    (void)snprintf(pidstr,SSIZE,"%d",pid);

    while((next_item = readdir(dirp)) != (struct dirent *)NULL)
    {   if(strcmp(next_item->d_name,".") != 0 && strcmp(next_item->d_name,"..") != 0)
        {  if(strin(next_item->d_name,"psrp") == TRUE && strin(next_item->d_name,pidstr) == TRUE)
           {  (void)closedir(dirp);
              return(TRUE);
           }
        }
    }
    
    (void)closedir(dirp);
    return(FALSE);
}




/*-----------------------------------------------------------------------------------
    Main entry point ...
-----------------------------------------------------------------------------------*/

_PUBLIC int main(int argc, char *argv[])

{   int i,
        decoded    = 0,
        start      = 1,
        target_pid = (-1);

    char chan_name[SSIZE] = "/tmp",
         hostname[SSIZE]  = "",
         chan_dir[SSIZE]  = "";

    _BOOLEAN do_verbose = FALSE,
             is_pid     = FALSE;

    (void)gethostname(hostname,SSIZE);
 
    for(i=0; i<argc; ++i)
    {  if(argc == 1 || strcmp(argv[i],"-help") == 0 || strcmp(argv[i],"-usage") == 0)
       { (void)fprintf(stderr,"\np3f version %s, (C) Tumbling Dice 2011-2019 (built %s %s)\n\n",P3F_VERSION,__TIME__,__DATE__);
         (void)fprintf(stderr,"P3F is free software, covered by the GNU General Public License, and you are\n");
         (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
         (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
         (void)fprintf(stderr,"P3F comes with ABSOLUTELY NO WARRANTY\n\n");
         (void)fprintf(stderr,"\nUsage: p3f [-usage | -help] | [-verbose] [-pchan <PSRP channel name:/tmp>] !<PID> | <PID name>! [>& <error/status log>\n\n");
         (void)fflush(stderr);

         exit(1);
       }
       else if(strcmp(argv[i],"-verbose") == 0)
       {  do_verbose == TRUE;
          ++decoded;
          ++start;
       }
       else if(strcmp(argv[i],"-pchan") == 0)
       {  struct stat buf;

          if(i == argc - 1 || argv[i+1][0] == '-')
          {  if(do_verbose == TRUE)
             {  (void)fprintf(stderr,"p3f (%d@%s): expecting name of PSRP channel directory\n",getpid(),hostname);
                (void)fflush(stderr);
             }

             exit(-1);
          }

          (void)strlcpy(chan_name,argv[i+1],SSIZE);


          /*----------------------------------------*/
          /* Does channel directory actually exist? */
          /*----------------------------------------*/

          if(access(chan_dir,F_OK | R_OK | W_OK) == (-1))
          {  if(do_verbose == TRUE)
             {  (void)fprintf(stderr,"p3f (%d@%s): PSRP channel directory \"%s\" does not exist\n",getpid(),hostname,chan_dir);
                (void)fflush(stderr);
             }

             exit(-1);
          }


          /*--------------------------------------------*/
          /* Is channel directory actually a directory? */
          /*--------------------------------------------*/

          (void)stat(chan_dir,&buf);
          if(! S_ISDIR(buf.st_mode))
          {  if(do_verbose == TRUE)
             {  (void)fprintf(stderr,"p3f (%d@%s): \"%s\" is not a directory\n",getpid(),hostname,chan_dir);
                (void)fflush(stderr);
             }

             exit(-1);
          }

          decoded += 1;
          start   += 1;
       }
    }

    if(argc - decoded > 1)
    {  (void)fprintf(stderr,"\np3f version %s, (C) Tumbling Dice 2019 (built %s %s)\n\n",P3F_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"P3F is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"P3F comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fprintf(stderr,"\nUsage: p3f [-usage | -help] | [-verbose] [-pchan <PSRP channel name:/tmp>] !<PID> | <PID name>! [>& <error/status log>\n\n");
       (void)fflush(stderr);

        exit(-1);
    }


    /*-----------------------------------------------------------*/
    /* Is target PID a process name? if so get corresponding PID */
    /*-----------------------------------------------------------*/

    if((target_pid = local_pname_to_pid(stderr,FALSE,&is_pid,argv[start+1])) == (-1))
       exit(-1);


    /*-----------------------------------*/
    /* Is our target a P3 aware process? */
    /*-----------------------------------*/

    if(p3_aware(target_pid,chan_dir) == FALSE)
    {  if(do_verbose == TRUE)
       {  if(is_pid == TRUE)
             (void)fprintf(stderr,"p3f (%d@%s): process %d is not P3 aware\n",getpid(),hostname,target_pid);
          else
             (void)fprintf(stderr,"p3f (%d@%s): process \"%s\" (%d) is not P3 aware\n",getpid(),hostname,argv[start+1],target_pid);
          (void)fflush(stderr);
       }

       exit(255);
    }
    else if(do_verbose == TRUE)
    {  if(is_pid == TRUE)
          (void)fprintf(stderr,"p3f (%d@%s): process %d is P3 aware\n",getpid(),hostname,target_pid);
       else
          (void)fprintf(stderr,"p3f (%d@%s): process \"%s\" (%d) is P3 aware\n",getpid(),hostname,argv[start+1],target_pid);
       (void)fflush(stderr);
    }

    exit(0);
}
