/*----------------------------------------------------------------------------------------
    Purpose: get textual infomation for use in shell script.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.02 
    Dated:   18th September 2019 
    E-mail:  mao@tumblingdice.co.uk
-----------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif /* HAVE_READLINE */

#include <xtypes.h>


/*---------*/
/* Version */
/*---------*/

#define ASK_VERSION  "2.02"


/*-------------*?
/* String size */
/*-------------*/

#define SSIZE        2048 


/*----------------------------------------------------------------------------------------
    Variables which are local to this application ...
----------------------------------------------------------------------------------------*/

_PRIVATE int child_pid = (-1);




#ifdef BSD_FUNCTION_SUPPORT
/*---------------------------------------------------------------------------------------
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




/*----------------------------------------------------------------------------------------
    Handler for signals ...
----------------------------------------------------------------------------------------*/

_PRIVATE int exit_handler(int signum)

{   if(child_pid != (-1))
       (void)kill(child_pid,SIGTERM);

    (void)fprintf(stderr,"\n");
    (void)fflush(stderr);

    (void)exit(-1);
}




/*----------------------------------------------------------------------------------------
    Test to see if string contains spaces ...
----------------------------------------------------------------------------------------*/

_PRIVATE int no_space(char *s)

{   int i,
        cnt = 0;


    /*----------------------*/
    /* Strip leading spaces */
    /*----------------------*/

    for(i=0; i<strlen(s); ++i)
    {  if(s[i] == ' ' || s[i] == '\t')
          ++cnt;
       else
          break;
    }


    /*-----------------------*/
    /* Strip trailing spaces */
    /*-----------------------*/

    for(i=strlen(s); i >= 0; --i)
    {  if(s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\0')
          s[i] = '\0';
       else
          break;
    }

    for(i=cnt; i<strlen(s); ++i)
    {  if(s[i] == ' ' || s[i] == '\t')
	  return(-1);
    }

    return(cnt);
}





/*----------------------------------------------------------------------------------------
    Main entry point ...
----------------------------------------------------------------------------------------*/

_PUBLIC int main(int argc, char *argv[])

{   int  ret,
         in,
         status,
         out,
         err,
         prompt_index = 1;

    char value[SSIZE]  = "";

    if(argc == 2 && (strcmp(argv[1],"-usage") == 0 || strcmp(argv[1],"-help") == 0))
    {  
       (void)fprintf(stderr,"\nask version %s, (C) Tumbling Dice 2003-2019 (built %s %s)\n\n",ASK_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"ASK is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"ASK comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nUsage: ask [payload] [prompt]\n\n");
       (void)fflush(stderr);

       exit(1);
    }

    if(isatty(0) != 1)
    {  (void)fprintf(stderr,"ask: standard I/O is not associated with a terminal\n");
       (void)fflush(stderr);

       exit(-1);
    } 

    if(argc == 3)
       prompt_index = 2;

    out = dup(1);

    (void)close(0);
    (void)open("/dev/tty",0);

    (void)close(1);
    (void)open("/dev/tty",1);

    (void)close(2);
    (void)open("/dev/tty",1);


    /*--------------------------------------------------------------------*/
    /* Start payload command which runs while this application is active. */
    /* This gets arount some problems with signal propagation by shells.  */
    /*--------------------------------------------------------------------*/

    (void)signal(SIGHUP, (void *)&exit_handler);
    (void)signal(SIGQUIT,(void *)&exit_handler);
    (void)signal(SIGINT, (void *)exit_handler);
    (void)signal(SIGTERM,(void *)&exit_handler);

    if(argc == 3)
    {  if((child_pid = fork()) == 0)
       {  char payload_cmd[SSIZE] = "";


          /*----------------------------------*/
          /* Payload will exec with child_pid */
          /*----------------------------------*/

          (void)snprintf(payload_cmd,SSIZE,"exec %s &",argv[1]);
          (void)execlp("sh","sh","-c",payload_cmd,(char *)0);

          _exit(0);
       }
       else
       {  

          /*---------------------------------*/
          /* Wait for child to avoid zombies */
          /*---------------------------------*/

          (void)waitpid(child_pid,&status,0);

          prompt_index = 2;
       }
    }


    /*------------------------------*/
    /* Main loop of the ask command */
    /*------------------------------*/

    do {    

            #ifdef HAVE_READLINE
            if(argc == 1)
               (void)strlcpy(value,readline("ask> "),SSIZE);
            else
            {  char prompt[SSIZE] = "";

               (void)snprintf(prompt,SSIZE,"%s> ",argv[prompt_index]);
               (void)strlcpy(value,readline(prompt),SSIZE);
            }
            #else
            if(argc == 1)
               (void)fprintf(stderr,"ask> ");
            else
               (void)fprintf(stderr,"%s> ",argv[prompt_index]);
            (void)fflush(stderr);

            (void)fgets(value,256,stdin);
            #endif /* HAVE_READLINE */

	    if((ret = no_space(value)) >= 0)
	    {  FILE *out_stream = (FILE *)NULL;

               out_stream = fdopen(out,"w"); 
               (void)fprintf(out_stream,"%s\n",(char *)&value[ret]);
               (void)fflush(out_stream);
            }

	    if(ret == (-1))
	    {  (void)fprintf(stderr,"ask: expecting one argument only\n\n");
	       (void)fflush(stderr);
	    }

       } while(ret < 0);

    return(0); 
}
