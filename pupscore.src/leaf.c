/*-----------------------------------------------------------------------------
     Purpose: Extracts sub "twig" from pathname branch. 

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.00 
     Dated:   30th August 2019 
     e-mail:  mao@tumblingdice.co.uk
-----------------------------------------------------------------------------*/


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <xtypes.h>


/*-----------------*/
/* Version of leaf */
/*-----------------*/

#define LEAF_VERSION    "2.00"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE           2048 




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




/*---------------------------------*/
/* Main entry point of application */
/*---------------------------------*/

_PUBLIC int main(int argc, char *argv[])

{   int i,
        decoded    = 0,
        start      = 1,
        twigLength = 1,
        cnt        = 0,
        cnt_pos    = 0;

    _BOOLEAN do_verbose = FALSE;

    char hostname[SSIZE] = "",
         pathname[SSIZE] = "",
         twig[SSIZE]     = "";

    (void)gethostname(hostname,SSIZE);
    for(i=0; i<argc; ++i)
    {  if(argc == 1 || argc > 3 || strcmp(argv[i],"-usage") == 0 || strcmp(argv[i],"-help") == 0)
       {  (void)fprintf(stderr,"\nleaf version %s, (C) Tumbling Dice 2010-2019 (built %s %s)\n\n",LEAF_VERSION,__TIME__,__DATE__);
          (void)fprintf(stderr,"LEAF is free software, covered by the GNU General Public License, and you are\n");
          (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
          (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
          (void)fprintf(stderr,"LEAF comes with ABSOLUTELY NO WARRANTY\n\n");
          (void)fprintf(stderr,"\nUsage: leaf [-usage | -help] | [-verbose:FALSE] [-tlength <twig length:1>]\n\n");
          (void)fflush(stderr);

          exit(-1);
       }
       else if(strcmp(argv[i],"-verbose") == 0)
       {  do_verbose = TRUE;
          ++decoded;
          ++start;
       }
       else if(strcmp(argv[i],"-tlength") == 0)
       {  if(i == argc - 2 || argv[i+1][0] == '-')
          {  if(do_verbose == TRUE)
             {  (void)fprintf(stderr,"\nleaf (%d@%s): expecting twig length (cardinal integer >= 0)\n\n",getpid(),hostname);
                (void)fflush(stderr);
             }
             exit(-1);
          }

          if(sscanf(argv[i+1],"%d",&twigLength) != 1 || twigLength < 1)
          {  if(do_verbose == TRUE)
             {  (void)fprintf(stderr,"\nleaf (%d@%s): twig length must be cardinal integer (>= 0)\n\n",getpid(),hostname);
                (void)fflush(stderr);
             }
             exit(-1);
          }

          decoded += 2;
          start   += 2;
          ++i;
       }
    }

    if(decoded < argc - 2)
    {  if(do_verbose == TRUE)
       {  (void)fprintf(stderr,"\nleaf (%d@%s): command tail items unparsed\n\n",getpid(),hostname);
          (void)fflush(stderr);
       }
       exit(-1);
    }

    (void)strlcpy(pathname,argv[start],SSIZE);
    for(i=strlen(pathname); i>=0;  --i)
    {  if(pathname[i] == '/')
       {  ++cnt;
          cnt_pos = i;
       }

       if(cnt == twigLength)
          break;
    }

    if(cnt == twigLength)
       (void)strlcpy(twig,(char *)&pathname[cnt_pos + 1],SSIZE);
    else
       (void)strlcpy(twig,pathname,SSIZE);

    (void)printf("%s\n",twig);
    (void)fflush(stdout);

    exit(0);
}
