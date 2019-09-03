/*------------------------------------------------------------------
     Purpose: tests for a lock file in a directory which can be
              accessed by multiple processes.

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.00 
     Dated:   2nd September 2019 
     e-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------*/

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>




/*----------------*/
/* Version of tas */
/*----------------*/

#define TAS_VERSION    "2.00"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE          2048 


/*-------------------------------------------------------------------
    Variables which are global to this application ...
-------------------------------------------------------------------*/

static int  lock_pid         = (-1);
static int  n_args           = (-1);
static char lock_name[SSIZE] = "";




#ifdef BSD_FUNCTION_SUPPORT
/*------------------------------------------------------------------
    Strlcpy and stlcat function based on OpenBSD functions ...
------------------------------------------------------------------*/
/*------------------*/
/* Open BSD Strlcat */
/*------------------*/
static size_t strlcat(char *dst, const char *src, size_t dsize)
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

static size_t strlcpy(char *dst, const char *src, size_t dsize)
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





/*-------------------------------------------------------------------
   Remove lock on exit ...
-------------------------------------------------------------------*/

static int exit_handler(int signum)

{

    /*----------------------------------------------------*/
    /* Lock intelligently -- if we lose the process which */
    /* asked for the lock -- delete lock                  */
    /*----------------------------------------------------*/

    if(n_args == 3)
    {  while(1)
       {   if(kill(lock_pid,SIGCONT) == (-1))
           {  if(access(lock_name,F_OK | R_OK | W_OK) != (-1))
                 (void)unlink(lock_name);
           }
       }
   }

   exit(0);
}



/*------------------*/
/* Main entry point */
/*------------------*/

int main(int argc, char *argv[])

{   int entered = 0;

    if(argc != 2 || strcmp(argv[1],"-usage") == 0 || strcmp(argv[1],"-help") == 0)
    {  (void)fprintf(stderr,"\ntas version %s, (C) Tumbling Dice 2002-2018 (built %s %s)\n\n",TAS_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"TAS is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"TAS comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fprintf(stderr,"\nUsage: tas [-usage | -help] | <lock file name> [<lock PID>]\n\n");
       (void)fflush(stderr);

       exit(-1);
    }

    (void)strlcpy(lock_name,argv[1],SSIZE);
    if(argc == 3)
    {  if(sscanf(argv[2],"%d",&lock_pid) != 1)
       {  (void)fprintf(stderr,"tas: expecting lock PID\n");
          (void)fflush(stderr);

          exit(-1);
       }

       (void)signal(SIGTERM,(void *)&exit_handler);
       (void)signal(SIGINT, (void *)&exit_handler);
       (void)signal(SIGQUIT,(void *)&exit_handler);

       n_args = argc;
    }

    while(mkdir(lock_name,0700) == (-1))
    {    if(entered == 0)
         {  entered = 1;

            (void)fprintf(stderr,"\ntas: lock (%s) is in use\n\n",lock_name);
            (void)fflush(stderr);
         }

         sleep(1);
    }

    (void)fprintf(stderr,"\n    tas: got lock (%s)\n\n",lock_name);
    (void)fflush(stderr);


    /*----------------------------------------------------*/
    /* Lock intelligently -- if we lose the process which */
    /* asked for the lock -- delete lock                  */
    /*----------------------------------------------------*/

    if(argc == 3)
    {  while(1)
       {   if(kill(lock_pid,SIGCONT) == (-1))
   	   {  if(access(lock_name,F_OK | R_OK | W_OK) != (-1))
	         (void)unlink(lock_name);
	
              exit(0);
	   }

	   (void)usleep(100);
       }
    }

    exit(0);
}
