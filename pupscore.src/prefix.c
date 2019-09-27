/*-----------------------------------------------------------------------------
    Purpose: Return prefix of string of form prefix.suffix. 

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Dated:   18th September 2019 
    Version: 2.00 
    E-mail:  mao@tumblingdice.co.uk
-----------------------------------------------------------------------------*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <xtypes.h>


/*-------------------*/
/* Version of prefix */
/*-------------------*/

#define PREFIX_VERSION  "2.00"


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




/*------------------*/
/* Main entry point */
/*------------------*/

int main(int argc, char *argv[])

{   int i,
        cnt = 0;

    char prefix[SSIZE] = "";

    if(argc != 2                                          ||
       strncmp(argv[1],"-help", strlen(argv[1])) == 0     ||
       strncmp(argv[1],"--help",strlen(argv[1])) == 0      )
    {  (void)fprintf(stderr,"\nprefix version %s, (C) Tumbling Dice 2000-2019 (built %s %s)\n\n",PREFIX_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"PREFIX is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"PREFIX comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nusage: prefix <string>\n\n");
       (void)fflush(stderr);

       exit(-1);
    }

    (void)strlcpy(prefix,argv[i],SSIZE); 
    for(i=strlen(argv[1]); i >= 0; --i)
    {  if(argv[1][i] == '.')
       {  (void)strlcpy(prefix,argv[1],SSIZE); 
          prefix[i] = '\0';
          (void)fprintf(stdout,"%s\n",prefix);
          (void)fflush(stdout);

          exit(0);
       }
     }

     (void)fprintf(stdout,"%s\n",argv[1]);
     (void)fflush(stdout);

     exit(0);
}

