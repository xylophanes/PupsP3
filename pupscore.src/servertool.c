/*---------------------------------------------------------------------------------------------
    Purpose: starts PSRP server outputting error log in new window.

    Author: M.A. O'Neill
            Tumbling Dice
            Gosforth
            Oxfordshire
            OX11 8QY

    Version: 2.00 
    Dated:   30th August 2019 
    E-mail:  mao@tumblingdice.co.uk
----------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <xtypes.h>
#include <string.h>
#include <stdlib.h>


/*-----------------------*/
/* Version of servertool */
/*-----------------------*/

#define SERVERTOOL_VERSION    "2.00"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE                 2048 




#ifdef BSD_FUNCTION_SUPPORT
/*---------------------------------------------------------------------------------------------
    Strlcpy and stlcat function based on OpenBSD functions ...
---------------------------------------------------------------------------------------------*/
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

{   int i;

    char server_name[SSIZE]   = "",
         server_hname[SSIZE]  = "",
         terminator[SSIZE]    = "",
         host_name[SSIZE]     = "",
         xterm_command[SSIZE] = "";


    /*---------------------------------------------------------*/
    /* Display help information if "-help" or "-usage" flagged */
    /*---------------------------------------------------------*/

    if(argc == 1 || (argc == 2 && (strcmp(argv[1],"-usage") == 0 || strcmp(argv[1],"-help") == 0)))
    {  (void)fprintf(stderr,"\nservertool version %s, (C) Tumbling Dice 2003-2019 (built %s %s)\n\n",SERVERTOOL_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"\nUsage: psrptool [-help | -usage] | !server!  [PSRP server  argument list]\n\n");                                              

       (void)fprintf(stderr,"SERVERTOOL is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"SERVERTOOL comes with ABSOLUTELY NO WARRANTY\n\n");
               
       exit(-1);
    }


    /*----------------------*/    
    /* Get psrp server name */
    /*----------------------*/    

    (void)strcpy(server_name,argv[1]);


    /*---------------------------*/ 
    /* Set client and host names */
    /*---------------------------*/ 

    (void)gethostname(host_name,SSIZE);
    (void)snprintf(server_hname,SSIZE,"%s@%s",server_name,host_name);

    for(i=2; i<argc; ++i) 
    {

       /*-----------------------------------------*/
       /* Note remote host to connect to (if any) */
       /*-----------------------------------------*/

       if(strcmp(argv[i],"-on") == 0)
       {  if(argv[i+1][0] == '-')
          {  (void)fprintf(stderr,"\nservertool: illegal command tail syntax\n\n");
             (void)fflush(stderr);

             exit(-1);
          }
          (void)snprintf(host_name,SSIZE,argv[i+1]);
       }          
    }

    for(i=2; i<argc; ++i) 
    {

       /*-------------------*/
       /* Note pen (if any) */
       /*-------------------*/

       if(strcmp(argv[i],"-pen") == 0)
       {  if(argv[i+1][0] == '-')
          {  (void)fprintf(stderr,"\nservertool: illegal command tail syntax\n\n");
             (void)fflush(stderr);

             exit(-1);
          }
          (void)snprintf(server_hname,SSIZE,"%s@%s",argv[i+1],host_name);
       }
    }


    /*-----------------------------*/
    /* Get argument list for xterm */
    /*-----------------------------*/

    (void)snprintf(xterm_command,SSIZE,"xterm -geometry 132x20 -T \"psrp server: %s\" -e %s",server_hname,server_name);
    for(i=2; i<argc; ++i)
    {  if(strcmp(argv[1],"-usage") != 0 && strcmp(argv[1],"-help") != 0)
       {  (void)strlcat(xterm_command," ",    SSIZE);
          (void)strlcat(xterm_command,argv[i],SSIZE);
       }


       /*-------------------*/
       /* Note pen (if any) */
       /*-------------------*/

       if(strcmp(argv[i],"-pen") == 0)
          (void)strcpy(server_name,argv[i+1]);
    }


    /*---------------------------------------------------*/
    /* Server MUST terminate if servertool is terminated */
    /*---------------------------------------------------*/

    (void)snprintf(terminator,SSIZE," -parent %d -parent_exit ",getpid());
    (void)strcat(xterm_command,terminator);


    /*------------------------------*/        
    /* Run psrp client in new xterm */
    /*------------------------------*/        

    (void)system(xterm_command);
    exit(0);
}
