/*---------------------------------------------------------------------------------------------
    Purpose: starts PSRP client in new window.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.00 
    Dated:   30th August 2019 
    E-mail:  mao@tumblingdice.co.uk
----------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xtypes.h>


/*---------------------*/
/* Version of psrptool */
/*---------------------*/

#define PSRPTOOL_VERSION    "2.00"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE               2048 


/*-------------------*/
/* Default font size */
/*-------------------*/

#define DEFAULT_FONT_SIZE   7




#ifdef BSD_FUNCTION_SUPPORT
/*--------------------------------------------------------------------------------------------
    Strlcpy and stlcat function based on OpenBSD functions ...
--------------------------------------------------------------------------------------------*/
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





/*------------------*/
/* Main entry point */
/*------------------*/

_PUBLIC int main(int argc, char *argv[])

{   int i,
        fsize                 = DEFAULT_FONT_SIZE,
        decoded               = 0;

    char hdir[SSIZE]          = "",
         client_name[SSIZE]   = "",
         host_name[SSIZE]     = "",
         soft_dongle[SSIZE]   = "notset",
         psrp_command[SSIZE]  = "",
         xterm_command[SSIZE] = "";


    /*---------------------------------------------------------*/
    /* Display help information if "-help" or "-usage" flagged */
    /*---------------------------------------------------------*/

    if(argc == 2 && (strcmp(argv[1],"-usage") == 0 || strcmp(argv[1],"-help") == 0))
    {  (void)fprintf(stderr,"\npsrptool version %s, (C) 2003-2019 Tumbling Dice (built %s %s)\n",PSRPTOOL_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"\nUsage: psrptool [-help | -usage] [psrp client argument list] [-fsize <font size:%d>] [-sdongle <soft dongle file name | default>]\n\n",
                                                                                                                                                    DEFAULT_FONT_SIZE);

       (void)fprintf(stderr,"PSRPTOOL is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"PSRPTOOL comes with ABSOLUTELY NO WARRANTY\n\n");
               
       exit(-1);
    }
    

    /*---------------------------*/ 
    /* Set client and host names */
    /*---------------------------*/ 

    (void)gethostname(host_name,SSIZE);
    (void)snprintf(client_name,SSIZE,"psrp@%s",host_name);

    for(i=1; i<argc; ++i) 
    {  /* Note remote host to connect to (if any) */
       if(strcmp(argv[i],"-on") == 0)
       {  if(argv[i+1][0] == '-')
          {  (void)fprintf(stderr,"\npsrptool: illegal command tail syntax\n\n");
             (void)fflush(stderr);

             exit(-1);
          }
          (void)snprintf(host_name,SSIZE,argv[i+1]);
       }          
    }

    for(i=1; i<argc; ++i) 
    {

       /*-------------------*/
       /* Note pen (if any) */
       /*-------------------*/

       if(strcmp(argv[i],"-pen") == 0)
       {  if(argv[i+1][0] == '-')
          {  (void)fprintf(stderr,"\npsrptool: illegal command tail syntax\n\n");
             (void)fflush(stderr);

             exit(-1);
          }
          (void)snprintf(client_name,SSIZE,"%s@%s",argv[i+1],host_name);
          ++i;
          ++decoded;
       }
       else if(strcmp(argv[i],"-sdongle") == 0)
       {  if(getenv("HOME") == (char *)NULL)
          {  (void)fprintf(stderr,"\npsrptool: cannot get home directory (from environment)\n\n");
             (void)fflush(stderr);

             exit(-1);
          }

          (void)strlcpy(hdir,getenv("HOME"),SSIZE);

          if(i == argc - 1 || argv[i+1][0] == '-')
          {  (void)snprintf(soft_dongle,SSIZE,"%s/.sdongles/pups.dongle",hdir);
             ++decoded;
          }
          else if(strcmp(argv[i+1],"default") == 0)
          {  (void)snprintf(soft_dongle,SSIZE,"%s/.sdongles/pups.dongle",hdir);
             ++decoded;
          }
          else
          {  (void)snprintf(soft_dongle,SSIZE,"%s/.sdongles/%s",hdir,argv[i+1]);
             ++i;
            decoded += 2;
          }
       } 
       else if(strcmp(argv[i],"-fsize") == 0)
       {  if(i == argc - 1 || argv[i+1][0] == '-')
          {  (void)fprintf(stderr,"\npsrptool: expecting font size\n\n");
             (void)fflush(stderr);


             exit(-1);
          }
          else
          {  if(sscanf(argv[i+1],"%d",&fsize) != 1 || fsize < 6 || fsize > 16)
             {  (void)fprintf(stderr,"\npsrptool: ridiculous font size specification (%d)\n",fsize);
                (void)fflush(stderr);

                exit(-1);
             }

             ++i;
             decoded += 2;
          }
       }
    }


    /*-----------------------------*/ 
    /* Get argument list for xterm */
    /*-----------------------------*/ 

    if(getuid() == 0)
       (void)snprintf(psrp_command,SSIZE,"/usr/bin/psrp");
    else
    {  (void)strlcpy(hdir,getenv("HOME"),SSIZE);
       (void)snprintf(psrp_command,SSIZE,"%s/bin/psrp",hdir);
    }

    if(strcmp(soft_dongle,"notset") != 0)
    {

       /*-----------------------------------------------------------------------------------*/
       /* Set up path to authentication dongle (via environmental variable USE_SOFT_DONGLE) */
       /*-----------------------------------------------------------------------------------*/

       (void)snprintf(xterm_command,SSIZE,"csh -c \'setenv USE_SOFT_DONGLE %s; exec xterm --geometry 132x20 -rightbar -fa Monospace -fs %d  -title \"psrp client: %s\" -e %s",
                                                                                                                            soft_dongle,fsize,client_name,psrp_command);
    }
    else
       (void)snprintf(xterm_command,SSIZE,"xterm --geometry 132x20 -rightbar -fa Monospace -fs %s -title \"psrp client: %s\" -c %s",
                                                                                              fsize,client_name,psrp_command);

    for(i=decoded + 1; i<argc; ++i)
    {  if(strcmp(argv[1],"-usage") != 0 && strcmp(argv[1],"-help") != 0)
       {  (void)strlcat(xterm_command," ",    SSIZE);
          (void)strlcat(xterm_command,argv[i],SSIZE);
       }


       /*-------------------*/
       /* Note pen (if any) */
       /*-------------------*/

       if(strcmp(argv[i],"-pen") == 0)
          (void)strlcpy(client_name,argv[i+1],SSIZE);
    }

    if(strcmp(soft_dongle,"notset") != 0)
       (void)strlcat(xterm_command,"'",SSIZE);


    /*------------------------------*/
    /* Run psrp client in new xterm */
    /*------------------------------*/

    (void)system(xterm_command);

    exit(0);
}
