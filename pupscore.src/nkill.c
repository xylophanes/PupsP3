/*-------------------------------------------------------------------------
     Purpose: Send signal to process(es) on remote machine(s)

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 5.00 
     Dated:   30th August 2019 
     E-mail:  mao@tumblingdice.co.uk
------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <xtypes.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pwd.h>

#ifdef SHADOW_SUPPORT
#include <shadow.h>
#endif /* SHADOW_SUPPORT */

#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>
#include <dirent.h>


#define _XOPEN_SOURCE
#include <unistd.h>


/*----------------------------------------------------------------*/
/* Get signal mapping appropriate to OS and hardware architecture */
/*----------------------------------------------------------------*/

#define __DEFINE__
#if defined(I386) || defined(X86_64)
#include <sig.linux.x86.h>
#endif /* I386) || X86_64 */

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



/*------------------------------------------------------------------------
    Defines which are local to this application ...
------------------------------------------------------------------------*/
/*------------------*/
/* Version of nkill */
/*------------------*/

#define NKILL_VERSION "5.00"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE         2048


#define MAX_TRYS      16
#define O_BLOCK       0
#define MAX_HOSTS     512 
#define SSIZE         512


/*----------------------------------------------*/
/* Method flags (for establishing process name) */
/*----------------------------------------------*/

#define COMM          (1 << 0) 
#define CMDLINE       (1 << 1) 
#define BINNAME       (1 << 2) 
#define STATUS        (1 << 3) 

#define NOT_PARSED    0
#define PARSED_LOCAL  1
#define PARSED        2
#define PUPS_AF_INET  AF_INET




/*------------------------------------------------------------------------
    Functions which are local to this application ...
------------------------------------------------------------------------*/

// Get password entry from static table or NIS map by name
_PROTOTYPE _PRIVATE struct passwd *pups_getpwnam(char *);

// Get password entry from static table or NIS map by UID
_PROTOTYPE _PRIVATE struct passwd *pups_getpwuid(int);

// Convert (local) PID to pidname */
_PROTOTYPE _PRIVATE _BOOLEAN local_pid_to_pname(int, char *);

// Convert (local) pidname to pidlist
_PROTOTYPE _PRIVATE _BOOLEAN local_pname_to_pids(FILE *, char *, _BOOLEAN, _BOOLEAN, char *, int []);

// Extract pidname 
_PROTOTYPE _PRIVATE _BOOLEAN get_pid_name(char *, char *, int *, char *);


#ifdef SSH_SUPPORT
// Check authentication token 
_PROTOTYPE _PRIVATE _BOOLEAN checkuser(char *username, char *password);
#endif /* SSH_SUPPORT */


// Look for the occurence of string s2 within string s1
_PROTOTYPE _PRIVATE _BOOLEAN strin(char *, char *);

// Parse (fully qualified) pidname to pidname,hostname pair or pidname,hostname.username tuple 
_PROTOTYPE _PRIVATE int parse_pidname(char *, char *, char *, char *);

// Distribute signal to process on named host
_PROTOTYPE _PRIVATE _BOOLEAN nkill(FILE *, _BOOLEAN, char *, char *, char *, char *, char *);

// Convert signal name to signal number
_PRIVATE int signametosigno(char *);




/*------------------------------------------------------------------------
    Variables which are private to this application ...
------------------------------------------------------------------------*/

_PRIVATE char     username[SSIZE]       = "notset";
_PRIVATE char     password[SSIZE]       = "notset";
_PRIVATE char     localhostname[SSIZE]  = "";
_PRIVATE char     remotehostname[SSIZE] = "";
_PRIVATE _BOOLEAN binname               = COMM;
_PRIVATE _BOOLEAN slaved                = FALSE;




/*------------------------------------------------------------------------
    Functions which are private to this application ...
------------------------------------------------------------------------*/

#ifdef BSD_FUNCTION_SUPPORT
/*------------------------------------------------------------------------
    Strlcpy and stlcat function based on OpenBSD functions ...
------------------------------------------------------------------------*/
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





/*------------------------------------------------------------------------
    Main entry point to code ...
------------------------------------------------------------------------*/

_PUBLIC int main(int argc, char *argv[])

{   int i,
        uid,
        start,
        parsed;

    char pidname[SSIZE]         = "",
         hostname[SSIZE]        = "localhost",
         signame[SSIZE]         = "SIGTERM",
         local_hostname[SSIZE]  = "",
         target_hostname[SSIZE] = "";

    _BOOLEAN s_all              = FALSE;
    FILE     *stream            = (FILE *)NULL;


    /*------------------------------------------------------------------*/
    /* Decode the comand tail. nkill commands are of the for            */
    /* nkill !signal | signame! <process-list>                          */
    /* where the processes in the <process-list> can be either          */
    /* of the form numeric-pid, process-name, numeric-pid@hostname or   */
    /* process-name@hostname                                            */
    /*------------------------------------------------------------------*/
    /*-------------------------------------------------------*/
    /* If we only have one argument display help information */
    /*-------------------------------------------------------*/

    if(argc < 2 || strcmp(argv[1],"-usage") == 0 || strcmp(argv[1],"-help") == 0)
    {  (void)fprintf(stderr,"\nnkill version %s, (C) 1999-2019 Tumbling Dice (built %s)\n",NKILL_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"\nUsage: nkill [+/-all] [+/-verbose] [+binname | +status] [-slaved:FALSE] [-psrp] !signum | signame! <process-list>\n");
       (void)fprintf(stderr,"\nProcess list entries have the following forms:\n\n");
       (void)fprintf(stderr,"numeric-pid              :    Process identifier on local host\n");
       (void)fprintf(stderr,"numeric-pid@localhost    :    Process identifier on local host\n");
       (void)fprintf(stderr,"numeric-pid@<host>       :    Process identifier on remote host\n");
       (void)fprintf(stderr,"process-name             :    Named process on local host\n");
       (void)fprintf(stderr,"process-name@localhost   :    Named process on local host\n");
       (void)fprintf(stderr,"process-name@<host>      :    Named process on remote host\n");
       (void)fprintf(stderr,"\n+all kills all named process of given name on selected host\n");
       (void)fprintf(stderr,"-all kills named process on given host only if its name is unique\n");
       (void)fprintf(stderr,"+verbose turns on verbose mode\n");
       (void)fprintf(stderr,"-verbose turns off verbose mode\n");
       (void)fprintf(stderr,"+binname uses binary rather than execution name\n");
       (void)fprintf(stderr,"+status uses name from /proc/status rather than execution name\n");
       (void)fprintf(stderr,"-usage displays (this) usage message\n\n");
       (void)fflush(stderr);

       (void)fprintf(stderr,"NKILL is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"NKILL comes with ABSOLUTELY NO WARRANTY\n\n");

       exit(-1);
    }


    /*----------------------------------------------------*/
    /* Process environment information. nkill() has two   */
    /* environmental flags, NKILL_VERBOSE which means     */
    /* error and status information are to be logged to   */
    /* stdin and NKILL_ALL which means processes which    */
    /* share an overloaded process name all get signalled */
    /*----------------------------------------------------*/

    if(getenv("NKILL_ALL") !=  (char *)NULL)
       s_all = TRUE;

    if(getenv("NKILL_VERBOSE") != (char *)NULL)
       stream = stderr;


    /*---------------------*/ 
    /* Decode command tail */
    /*---------------------*/ 

    if(strncmp(argv[1],"SIG",3) == 0)
    {  start = 2;
       (void)strlcpy(signame,argv[1],SSIZE);
    }
    else
       start = 1; 

    for(i=start; i<argc; ++i)
    {

       /*---------------------------------------*/
       /* Do we have a -all flag ?              */
       /* If so kill all processes of same name */
       /* on a given host                       */
       /*---------------------------------------*/

       if(strcmp(argv[i],"+all") == 0)
          s_all = TRUE;
       else if(strcmp(argv[i],"-all") == 0)


          /*---------------------------------------------*/
          /* Only kill named process on given host if it */
          /* is unique                                   */
          /*---------------------------------------------*/

          s_all = FALSE;
       else if(strcmp(argv[i],"-slaved") == 0)
          slaved = TRUE;
       else if(strcmp(argv[i],"+verbose") == 0)
          stream = stderr;
       else if(strcmp(argv[i],"+cmdline") == 0)
          binname = CMDLINE;
       else if(strcmp(argv[i],"+binname") == 0)
          binname = BINNAME;
       else if(strcmp(argv[i],"+status") == 0)
          binname = STATUS;


       /*------------------------------------------------------------*/
       /* Do we have a process descriptor of the form <pdes>@<host>? */
       /*------------------------------------------------------------*/

       parsed = parse_pidname(argv[i],pidname,target_hostname,username);

       if(parsed == PARSED || parsed == PARSED_LOCAL)
       {  if(parsed == PARSED)
          {  char password_banner[SSIZE] = "",
                  local_hostname[SSIZE]  = "";


             /*-------------------------------------------*/
             /* We must be connected to a terminal device */
             /*-------------------------------------------*/

             if(isatty(0) == 0)
             {  (void)fprintf(stderr,"\nnkill: not connected to a tty\n\n");
                (void)fflush(stderr);

                exit(-1);
             }

             (void)gethostname(local_hostname,SSIZE); 
             if(strncmp(target_hostname,"localhost",strlen(target_hostname)-1) != 0)
             #ifdef SSH_SUPPORT
             {   char pwd_file_name[SSIZE] = "";


                 /*---------------------------------------------------------------------------------*/
                 /* Get authentication token from /tmp/authtok.<PID>.tmp -- to minimise the size    */
                 /* of the security hole this causes, we must delete this file as soon as we have   */
                 /* read the token                                                                  */
                 /*---------------------------------------------------------------------------------*/

                 (void)snprintf(pwd_file_name,SSIZE,"/tmp/authtok.%d.tmp",getuid());

                 if(access(pwd_file_name,F_OK | R_OK | W_OK) != (-1))
                 {  FILE *pwd_stream = (FILE *)NULL;

                    (void)fprintf(stderr,"\nGetting authentication token from PSRP authorisation file \"%s\"\n",pwd_file_name);
                    (void)fflush(stderr);

                    pwd_stream = fopen(pwd_file_name,"r");
                    if(fscanf(pwd_stream,"%s",password) == 1)
                    {  (void)fprintf(stderr,"Authentication token read\n\n");
                       (void)fflush(stderr);
                    }

                    (void)fclose(pwd_stream);
                 }


                 /*--------------------------------------------------*/
                 /* We need to get an authentication token before we */
                 /* can send a signal                                */
                 /*--------------------------------------------------*/

                 if(strcmp(password,"notset") == 0)
                 {  (void)snprintf(password_banner,SSIZE,"PUPS %s [nkill] password (for %s): ",local_hostname,username);
                    (void)strlcpy(password,getpass(password_banner),SSIZE);
                 }


                 /*----------------*/
                 /* Check password */
                 /*----------------*/

                 if(checkuser(username,password) == FALSE)
                 {  if(stream != (FILE *)NULL)
                    {  (void)fprintf(stderr,"Permission denied\n");
                       (void)fflush(stderr);

                       exit(-1);
                   }
                }
             }
             #else
             {  (void)fprintf(stderr,"nkill: signalling of processes on remote host not supported (no ssh support)\n");
                (void)fflush(stderr);

                exit(-1);
             } 
             #endif /* SSH_SUPPORT */
          }
          else if(parsed == PARSED_LOCAL)
          {  (void)strlcpy(pidname,argv[i],SSIZE);
             (void)strlcpy(target_hostname,"localhost",SSIZE);
          }


          /*------------------------------*/
          /* Deliver signal to named host */
          /*------------------------------*/

          (void)nkill(stream,
                      s_all,
                      username,
                      password,
                      signame,
                      target_hostname,
                              pidname);

       }
    }

    exit(0);
}

 


/*------------------------------------------------------------------------
    Parse pidname returning process,host pair ...
------------------------------------------------------------------------*/

_PRIVATE int parse_pidname(char *full_pidname, char *pidname, char *hostname, char *username)

{   int i,
        j,
        cnt = 0;

    struct passwd *pwent = (struct passwd *)NULL;

    if(full_pidname[0] == '+' || full_pidname[0] == '-')
       return(NOT_PARSED);

    for(i=0; i<strlen(full_pidname); ++i)
    {

       /*-----------------*/
       /* Extract pidname */ 
       /*-----------------*/

       if(full_pidname[i] != '@')
          pidname[i] = full_pidname[i];
       else
       {

          /*-------------------*/
          /* Terminate pidname */ 
          /*-------------------*/

          pidname[i] = '\0';


          /*-------------------------------*/
          /* Have we specified a username? */
          /*-------------------------------*/

          for(j=i+1; j<strlen(full_pidname); ++j)
          {  if(full_pidname[j] != ':')
                hostname[cnt++] = full_pidname[j];
             else
             {

                /*----------------------------------------*/
                /* Get username (who owns target process) */
                /*----------------------------------------*/

                ++j;

                if(snprintf(username,SSIZE,"%s",&full_pidname[j]) == 0)
                {  (void)fprintf(stderr,"nkill: expecting name of owner of target pid(name)\n");
                   (void)fflush(stderr);

                   return(NOT_PARSED);
                }


                /*--------------------*/
                /* Terminate hostname */
                /*--------------------*/

                hostname[j-1] = '\0';

                return(PARSED);
             }
          }


          /*--------------------*/
          /* Terminate hostname */
          /*--------------------*/

          hostname[cnt] = '\0';


          /*----------------------------------------*/
          /* No username specified - so use default */
          /*----------------------------------------*/

          pwent = pups_getpwuid(getuid());
          (void)strlcpy(username,pwent->pw_name,SSIZE);

          return(PARSED);
       }
    }


    /*----------------------------------------*/
    /* No hostname specified - so use default */
    /*----------------------------------------*/

    (void)gethostname(hostname,SSIZE);


    /*----------------------------------------*/
    /* No username specified - so use default */
    /*----------------------------------------*/

    pwent = pups_getpwuid(getuid());
    (void)strlcpy(username,pwent->pw_name,SSIZE);

    return(PARSED_LOCAL);
}



     
/*------------------------------------------------------------------------
    Send a signal to a named host first checking to see if it is
    alive ...
------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN nkill(FILE   *stream,   /* Error log/status stream                */
                        _BOOLEAN s_all,   /* Signal all procs called pname  if TRUE */
                        char *username,   /* Username (on remote host)              */
                        char *password,   /* Authentication token                   */
                        char  *signame,   /* Signal name (or number)                */
                        char    *rhost,   /* Remote host                            */
                        char  *pidname)   /* Name of process to signal              */

{   int i,
        pid,
        status,
        signum,
        reply,
        cnt  = 0,
        sent = 0,
        ptab[MAX_HOSTS];

    char strdum[SSIZE]       = "",
         lhost[SSIZE]        = "",
         next_pidname[SSIZE] = "";

    _BOOLEAN remote = TRUE;

    if(sscanf(signame,"%d",&signum) == 0)
    {

       /*----------------------------------------*/
       /* Translate signal name to signal number */
       /*----------------------------------------*/

       if((signum = signametosigno(signame)) == (-1))
       {  if(stream != (FILE *)NULL)
          {  if(slaved == FALSE)
                (void)fprintf(stream,"nkill: %s is not a valid signal\n",signame);
             else
                (void)fprintf(stream,"%d\n",-EPERM);
             (void)fflush(stream);
          }

          exit(-1);
       }
    }


    /*------------------------*/
    /* Get name of local host */
    /*------------------------*/

    (void)gethostname(lhost,SSIZE);


    /*-------------------------------------------------------*/
    /* Are we trying to signal target running on remote host */
    /*-------------------------------------------------------*/

    if(strcmp(rhost,"localhost") == 0)
       remote = FALSE;


    /*----------------------------------------------------------------*/
    /* If we have a named process we need to resolve the process name */
    /* to PID                                                         */
    /*----------------------------------------------------------------*/

    if(sscanf(pidname,"%d",&ptab[0]) == 0)
    {  int i,
           ret = 0;


       /*-----------------------------------------------------*/
       /* If signal is being sent to localhost run ps locally */
       /*-----------------------------------------------------*/

       if(remote == FALSE)
       {  cnt = local_pname_to_pids(stream,lhost,s_all,binname,pidname,ptab);

          if(cnt == 0 && pidname[0] != '+' && pidname[0] != '-')
          {  if(stream != (FILE *)NULL)
             {  if(slaved == FALSE)
                   (void)fprintf(stream,"nkill: process %s not running on host %s\n",pidname,lhost);
                else
                   (void)fprintf(stream,"%d\n",-EEXIST);
                (void)fflush(stream);
             }

             return(FALSE);
          }
       }
       #ifdef SSH_SUPPORT
       else
       {

          /*------------------------------------------------------*/
          /* Use secure shell (ssh) to send signal to remote host */
          /*------------------------------------------------------*/

          if((pid = fork()) == 0)
          {  char remote_nkill_command[SSIZE] = "";


             /*--------------------*/
             /* Child side of fork */
             /*--------------------*/

             /*---------------------*/
             /* Overlay ssh command */
             /*---------------------*/

             (void)snprintf(remote_nkill_command,SSIZE,"nkill %s",signame);
             if(stream == stderr)
                (void)strlcat(remote_nkill_command," +verbose",SSIZE);

             if(binname == TRUE)
                (void)strlcat(remote_nkill_command," +binname",SSIZE);

             if(s_all == TRUE)
                (void)strlcat(remote_nkill_command," +all",SSIZE);

             (void)strlcat(remote_nkill_command," ",    SSIZE);
             (void)strlcat(remote_nkill_command,pidname,SSIZE);

             if(isatty(0) == 1)
                (void)execlp("ssh","ssh","-t","-u","-l",username,rhost,remote_nkill_command,(char *)NULL);
             else
                (void)execlp("ssh","ssh","-u","-l",username,rhost,remote_nkill_command,(char *)NULL);


             /*------------------------*/
             /* We should not get here */
             /*------------------------*/

             _exit(-1);
         }


         /*---------------------*/
         /* Parent side of fork */
         /*---------------------*/

         else if(pid == (-1))
         {

            /*---------------------------------------------------------------*/
            /* If we get here connection to remote host (via ssh) has failed */
            /*---------------------------------------------------------------*/

            if(slaved == FALSE)
            {  (void)fprintf(stderr,"nkill: failed to connect to remote host (via ssh)\n");
               (void)fflush(stderr);
            }

            return(FALSE);
         }

         /*----------------*/
         /* Wait for child */
         /*----------------*/

         while((ret = waitpid(pid,&status,WNOHANG)) != pid)
         {    if(ret == (-1) || WEXITSTATUS(status) < 0 || WEXITSTATUS(status) == 255)
              {  (void)fprintf(stderr,"nkill: error connecting to remote host (via ssh)\n");
                 (void)fflush(stderr);

                 return(FALSE);
              }
              else
                 (void)usleep(100);
         }
      }
      #endif /* SSH_SUPPORT */
    }
    else
    {

       /*----------------------------------------------*/
       /* We are sending a signal to an unamed process */
       /* Lets look up its name                        */
       /*----------------------------------------------*/

       (void)local_pid_to_pname(ptab[0],pidname);
       ++cnt;


       /*-----------------------------------------*/
       /* See if we have a valid target to signal */
       /*-----------------------------------------*/

       #ifdef SSH_SUPPORT
       if(remote == TRUE)
       {  int ret = 0;

          /*-------------------------------------------------*/
          /* We are sending a signal to a remote numeric PID */ 
          /*-------------------------------------------------*/

          char pidstr[SSIZE]               = "",
               remote_nkill_command[SSIZE] = "";


          if((pid = fork()) == 0)
          {

             /*------------------------------------------------------------------*/
             /* Use secure shell (ssh) rather than PSRP protocols to send signal */
             /* to remote host                                                   */
             /* Overlay ssh command                                              */
             /*------------------------------------------------------------------*/

             (void)snprintf(remote_nkill_command,SSIZE,"nkill %s",signame);
             if(stream == stderr)
                (void)strlcat(remote_nkill_command," +verbose",SSIZE);

             if(s_all == TRUE)
                (void)strlcat(remote_nkill_command," +binname",SSIZE);

             (void)strlcat(remote_nkill_command," ",SSIZE);
             (void)snprintf(pidstr,SSIZE,"%d",ptab[0]);
             (void)strlcat(remote_nkill_command,pidstr,SSIZE);

             if(isatty(0) == 1)
                (void)execlp("ssh","ssh","-t","-l",username,rhost,remote_nkill_command,(char *)NULL);
             else
                (void)execlp("ssh","ssh","-l",username,rhost,remote_nkill_command,(char *)NULL);


             _exit(-1);
          }


          /*---------------------*/
          /* Parent side of fork */
          /*---------------------*/

          else if(pid == (-1))
          {  

             /*---------------------------------------------------------------*/
             /* If we get here connection to remote host (via ssh) has failed */
             /*---------------------------------------------------------------*/

             if(slaved == FALSE)
             {  (void)fprintf(stderr,"nkill: failed to connect to remote host (via ssh)\n");
                (void)fflush(stderr);
             }

             return(FALSE);
          }

          /*----------------*/
          /* Wait for child */
          /*----------------*/

          while((ret = waitpid(pid,&status,WNOHANG) != pid))
          {    if(ret == (-1) || WEXITSTATUS(status) < 0 || WEXITSTATUS(status) == 255)
               {  (void)fprintf(stderr,"nkill: error connecting to remote host (via ssh)\n");
                  (void)fflush(stderr);
 
                  return(FALSE);
               }
               else
                  (void)usleep(100);
          }

          return(TRUE);
       }
       else
       #endif /* SSH_SUPPORT */

       {

          /*----------------------------------------------*/
          /* We are sending signal to a local numeric PID */ 
          /*----------------------------------------------*/

          if((reply = kill(ptab[0],signum)) == (-1))
          {  if(stream != (FILE *)NULL)
             {  if(slaved == FALSE)
                   (void)fprintf(stream,"nkill: process %d not running on host %s\n",ptab[0],lhost);
                else
                   (void)fprintf(stream,"%d\n",-EEXIST);
                (void)fflush(stream);
             }


             return(FALSE);
          }
          else
          {  if(stream != (FILE *)NULL)
             {  if(slaved == FALSE)
                   (void)fprintf(stream,"Signal %s sent to %s [%d@%s:%s] (reply %d)\n",
                                     signame,pidname,ptab[0],lhost,username,reply);
                else
                   (void)fprintf(stream,"%d\n",reply);
             }

             return(TRUE);
          } 
       }
    }


    /*----------------------------------------------------------------*/
    /* We are local instance of nkill so lets nail the target PIDlist */
    /*----------------------------------------------------------------*/

    for(i=0; i<cnt; ++i)
    {  if((reply = kill(ptab[i],signum)) == 0)
       {  if(stream != (FILE *)NULL)
          {  if(slaved == FALSE)
                (void)fprintf(stream,"Signal %s sent to %s [%d@%s:%s] (reply %d)\n",
                                       signame,pidname,ptab[i],lhost,username,reply);
             else
                (void)fprintf(stream,"%d\n",reply);
          }

          ++sent;
       }
       else
       {  if(stream != (FILE *)NULL)
          {  if(slaved == FALSE)
                (void)fprintf(stream,"Failed to send signal %s to %s[%d@%s:%s]\n",signame,pidname,ptab[i],username,lhost);
             else
                (void)fprintf(stream,"%d\n",reply);
          }
       }

       (void)fflush(stream);
    }

    if(sent > 0)
       return(TRUE);

    return(FALSE);
}





#ifdef SSH_SUPPORT
/*------------------------------------------------------------------------
    Check to see if remote user is permitted to run this service ...
------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN checkuser(char *username, char *password)

{   struct passwd  *pwent = (struct passwd *)NULL;

    char salt[2]                 = "",
         crypted_password[SSIZE] = "";


    /*--------------------------------------------------*/
    /* Check arguments -- if any are NULL simply return */
    /*--------------------------------------------------*/

    if(username == (char *)NULL || password == (char *)NULL)
       return(FALSE);


    /*--------------------------*/
    /* Authenticate remote user */
    /*--------------------------*/

    if((pwent = pups_getpwnam(username)) == (struct passwd *)NULL)
       return(FALSE);


    /*-------------------------------------------*/
    /* Crypt password using first two characters */
    /* in password for "salt"                    */
    /* Crypt password using first two characters */
    /* in in encrypted password for "salt"       */
    /*-------------------------------------------*/

    salt[0] = pwent->pw_passwd[0];
    salt[1] = pwent->pw_passwd[1];
    (void)strlcpy((char *)&crypted_password,(char *)crypt(password,(char *)&salt),SSIZE);

    if(strcmp(pwent->pw_passwd,crypted_password) != 0)
       return(FALSE);

    return(TRUE);
}
#endif /* SSH_SUPPORT */




/*------------------------------------------------------------------------
    Look for the occurence of string s2 within string s1 ...
------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN strin(char *s1, char *s2)

{   int i,
        cmp_size,
        chk_limit;

    if(strlen(s2) > strlen(s1))
       return(FALSE);

    chk_limit = strlen(s1) - strlen(s2) + 1;


    /*-----------------------------------------*/
    /* This *IS* correct! Standard routine has */
    /* cmp_size = strlen(s2) - 1               */
    /*-----------------------------------------*/

    cmp_size  = strlen(s2);

    for(i=0; i<chk_limit; ++i)
       if(strncmp(&s1[i],s2,cmp_size) == 0)
          return(TRUE);

    return(FALSE);
}




/*---------------------------------------------------------------------------
    Translate signal name to signal number ...
---------------------------------------------------------------------------*/

_PRIVATE int signametosigno(char *name)

{   int i;

    for(i=0; i<MAX_SIGS; ++i)
       if(signame[i] != (char *)NULL && strin(signame[i],name) == TRUE)
          return(i+1);

    return(-1);
}




/*----------------------------------------------------------------------------
    Extract pidname ...
----------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN get_pid_name(char *pidname, char *line, int *pid, char *next_pidname)

{    char strdum[SSIZE]   = "",
          bpidname[SSIZE] = "";


     /*------------------------------------------------*/
     /* Make sure we have "pidname" and not "-pidname" */
     /*------------------------------------------------*/

     (void)strlcpy(bpidname,pidname,SSIZE);
     if(strin(line,bpidname) == TRUE && strin(line,"nkill") == FALSE)
     {  (void)sscanf(line,"%d",pid);
        return(TRUE);
     }

     return(FALSE);
}




/*----------------------------------------------------------------------------
    Convert (local) pidname to list of matching PIDS ...
----------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN local_pname_to_pids(FILE      *stream,
                                      char       *lhost,
                                      _BOOLEAN    s_all,
                                      _BOOLEAN  binname,
                                      char     *pidname,
                                      int        ptab[])


{  int  pid,
        cnt = 0;

   char line[SSIZE]         = "",
        next_pidname[SSIZE] = "";

   DIR           *dirp      = (DIR *)NULL;
   struct dirent *next_item = (struct dirent *)NULL;

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

           char cmdline[SSIZE]     = "",
                procpidpath[SSIZE] = ""; 

           if(binname == COMM)
           {  (void)snprintf(procpidpath,SSIZE,"/proc/%d/comm",pid);
              fdes = open(procpidpath,0);
              nb   = read(fdes,cmdline,SSIZE);
              cmdline[nb] = '\0';
              (void)close(fdes);
           }
           else if(binname == CMDLINE)
           {  (void)snprintf(procpidpath,SSIZE,"/proc/%d/cmdline",pid);
              fdes = open(procpidpath,0);
              nb   = read(fdes,cmdline,SSIZE); 
              cmdline[nb] = '\0';
              (void)close(fdes);
           }
           else if(binname == BINNAME)
           {  

              /*--------------------*/
              /* Get name of binary */ 
              /*--------------------*/

              (void)snprintf(procpidpath,SSIZE,"/proc/%d/exe",pid);
              (void)readlink(procpidpath,cmdline,SSIZE);
           }
           else if(binname == STATUS)
           {  char strdum[SSIZE] = "",
                   tmpstr[SSIZE] = "";

              (void)snprintf(procpidpath,SSIZE,"/proc/%d/status",pid);
              fdes = open(procpidpath,0);
              nb   = read(fdes,tmpstr,SSIZE);
              tmpstr[nb] = '\0';
              (void)close(fdes);
              (void)sscanf(tmpstr,"%s%s",strdum,cmdline);
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
           {  if(cnt == 1 && s_all == FALSE && binname == FALSE)
              {  (void)closedir(dirp);

                 if(stream != (FILE *)NULL)
                 {  (void)fprintf(stream,"nkill: process %s[@%s] is not uniquely named\n",pidname,lhost);
                    (void)fflush(stream);
                 }

                 return(-1);
              }
              else
              {  ptab[cnt] = pid;
                 ++cnt;
              }                  
           }
        }
   }

   return(cnt);
}




/*----------------------------------------------------------------------------
    Translate PID to (local) process name ...
----------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN local_pid_to_pname(int pid, char *pidname)

{   int  i;

    char tmpstr[SSIZE]  = "",
         cmdline[SSIZE] = "";

    int nb,
        fdes = (-1);

    char procpidpath[SSIZE] = "";

    (void)snprintf(procpidpath,SSIZE,"/proc/%d/cmdline",pid);
    if((fdes = open(procpidpath,0)) == (-1))
        return(FALSE);

    nb = read(fdes,tmpstr,SSIZE);
    tmpstr[nb] = '\0';


    /*------------*/
    /* Strip path */
    /*------------*/

    for(i=strlen(tmpstr); i>0; --i)
    {   if(tmpstr[i] == '/')
           goto stripped;
    }

stripped:  

    (void)strlcpy(pidname,tmpstr,SSIZE);
    return(TRUE);
}



/*----------------------------------------------------------------------------------------
    Extended PUPS getpwnam routine which searches static password table. If this
    fails, the appropriate NIS map is then searched ...
----------------------------------------------------------------------------------------*/

_PRIVATE struct passwd *pups_getpwnam(char *name)

{   _IMMORTAL struct passwd *pwent = (struct passwd *)NULL;


    /*--------------------------*/
    /* Authenticate remote user */
    /* By static table          */
    /*--------------------------*/

    if((pwent = getpwnam(username)) == (struct passwd *)NULL)
    {  

       #ifdef NIS_SUPPORT
       _BYTE  pwent_buf[204]     = "";
       struct passwd **pwent_ptr = (struct passwd **)NULL;


       /*------------*/
       /* By NIS map */
       /*------------*/

       if((_nss_nis_getpwnam_r(username,&pwent,pwent_buf,pwent_ptr)) == (-1))
          return((struct passwd *)NULL);
       #else
       return((struct passwd *)NULL);
       #endif /* NIS_SUPPORT */
    }


    #ifdef SHADOW_SUPPORT
    /*----------------------------------------------------------------------*/
    /* Are we running a shadow password file? If so get the "real" password */
    /* from /etc/shadow                                                     */
    /*----------------------------------------------------------------------*/

    if(strcmp(pwent->pw_passwd,"x") == 0 && geteuid() == 0)
    {  struct spwd *shadow = (struct spwd *)NULL;

       if((shadow = getspnam(username)) == (struct spwd *)NULL)
       {  (void)fprintf(stderr,"No such (shadow) user \"%s\"\n",username);
          (void)fflush(stderr);

          return(FALSE);
       }

       pwent->pw_passwd = shadow->sp_pwdp;
    }
    #endif /* SHADOW_SUPPORT */

    return(pwent);
}




/*----------------------------------------------------------------------------------------
    Extended PUPS getpwnam routine which searches static password table. If this
    fails, the appropriate NIS map is then searched ...
----------------------------------------------------------------------------------------*/

_PRIVATE struct passwd *pups_getpwuid(int uid)

{   _IMMORTAL struct passwd *pwent = (struct passwd *)NULL;


    /*--------------------------*/
    /* Authenticate remote user */
    /* By static table          */
    /*--------------------------*/

    if((pwent = getpwuid(uid)) == (struct passwd *)NULL)
    {  

       #ifdef NIS_SUPPORT
       _BYTE  pwent_buf[204]     = "";
       struct passwd **pwent_ptr = (struct passwd **)NULL;


       /*------------*/
       /* By NIS map */
       /*------------*/

       if((_nss_nis_getpwnam_r(uid,&pwent,pwent_buf,pwent_ptr)) == (-1))
          return((struct passwd *)NULL);
       #else
       return((struct passwd *)NULL);
       #endif /* NIS_SUPPORT */
    }


    #ifdef SHADOW_SUPPORT 
    /*----------------------------------------------------------------------*/
    /* Are we running a shadow password file? If so get the "real" password */
    /* from /etc/shadow                                                     */
    /*----------------------------------------------------------------------*/

    if(strcmp(pwent->pw_passwd,"x") == 0  && geteuid() == 0) 
    {  struct spwd *shadow = (struct spwd *)NULL;
       
       if((shadow = getspnam(pwent->pw_name)) == (struct spwd *)NULL)
          return(FALSE);
       
       pwent->pw_passwd = shadow->sp_pwdp;
    }
    #endif /* SHADOW_SUPPORT */

    return(pwent);
}

