/*----------------------------------------------------------------------------
     Purpose: tests for owner of lock (being live) 

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.00 
     Dated:   30th August 2019 
     E-mail:  mao@tumblingdice.co.uk
-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <xtypes.h>




/*-----------------------------------------------------------------------------
    Defines which are local to this application ...
-----------------------------------------------------------------------------*/
/*----------------*/
/* Version of lol */
/*----------------*/

#define LOL_VERSION "2.00"
#define END_STRING  9999


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE       2048 




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





/*-----------------------------------------------------------------------------
    Replace multiple chracters in string with given character ...
-----------------------------------------------------------------------------*/

_PRIVATE void mchrep(char rep_ch, char *ch_to_rep, char *s1)

{   int i,
        j,
        s1_l,
        rep_s_l;

    s1_l    = strlen(s1);
    rep_s_l = strlen(ch_to_rep);

    for(i=0; i<s1_l; ++i)
       for(j=0; j<rep_s_l; ++j)
          if(s1[i] == ch_to_rep[j] || (ch_to_rep[j] == 'D' && isdigit(s1[i])))
             s1[i] = rep_ch;
}




/*-----------------------------------------------------------------------
    Routine to extract substrings which are demarkated by a usr defined
    character ...
-----------------------------------------------------------------------*/
                                    /*---------------------------------*/
_PRIVATE int strext(char dm_ch,     /* Demarcation character           */
                    char   *s1,     /* Extracted sub string            */
                    char   *s2)     /* Argument string                 */
                                    /*---------------------------------*/

{   int s1_ptr = 0;

                                    /*---------------------------------*/
    _PRIVATE int   s2_ptr = 0;      /* Current pointer into arg string */
    _PRIVATE char *s2_was;          /* Copy of current argument string */
                                    /*---------------------------------*/


    /*-------------------------------------------------------------------*/
    /* Entry with null parameters forces reset of pointers within strext */
    /* function                                                          */
    /*-------------------------------------------------------------------*/

    if(s2 == (char *)NULL)
    {  s2_ptr = 0;
       return(FALSE);
    }


    /*--------------------------------------------------------------------*/
    /* Save string length for swap test if this is first pass for current */
    /* extraction string                                                  */
    /*--------------------------------------------------------------------*/

    if(s2_ptr == 0)
    {  s2_was = (char *)malloc(SSIZE);
       (void)strlcpy(s2_was,s2,SSIZE);
    }
    else
    {

       /*------------------------------------------------*/
       /* If extraction string has changed re-initialise */
       /*------------------------------------------------*/

       if(strcmp(s2_was,s2) != 0)
       {  s2_ptr = 0;
          strlcpy(s2_was,s2,SSIZE);
       }
    }


    /*------------------------------------*/
    /*  Wind to substring to be extracted */
    /*------------------------------------*/

    while(s2[s2_ptr] == dm_ch && s2[s2_ptr] != '\0' && s2[s2_ptr] != '\n')
          ++s2_ptr;


    /*---------------------------------------------------------*/
    /* If we have reached the end of the string - reinitialise */
    /*---------------------------------------------------------*/

    if(s2[s2_ptr] == '\0' || s2[s2_ptr] == '\n')
    {  s2_ptr = 0;
       s1[0] = '\0';
       (void)free(s2_was);
       return(FALSE);
    }


    /*-------------------------------------------------*/
    /* Extract substring to next demarcation character */
    /*-------------------------------------------------*/

    while(s2[s2_ptr] != dm_ch && s2[s2_ptr] != '\0' && s2[s2_ptr] != '\n')
          s1[s1_ptr++] = s2[s2_ptr++];
    s1[s1_ptr] = '\0';


    /*--------------------------------------------------------------------*/
    /* If the end of the extraction string has been reached, reinitialise */
    /*--------------------------------------------------------------------*/

    if(s2[s2_ptr] == '\0' || s2[s2_ptr] == '\n')
    {  s2_ptr = 0;
       (void)free(s2_was);

                              /*----------------------------------------------*/
       return(END_STRING);    /* Messy but some applications need to know if  */
                              /* dm_ch was really matched or if we have hit   */
                              /* the end of the string                        */
                              /*----------------------------------------------*/
    }

    return(TRUE);
}





/*-----------------------------------------------------------------------------
    Main entry point to application ...
-----------------------------------------------------------------------------*/

_PUBLIC int main(int argc, char *argv[])

{   int ret,
        lock_pid;

    char tmpstr[SSIZE]    = "",
         strdum[SSIZE]    = "",
         next_item[SSIZE] = "",
         lock_host[SSIZE] = "";

    struct stat buf;

    if(argv[1] == (char *)NULL || strcmp(argv[1],"-help") == 0 || strcmp(argv[1],"-usage") == 0)
    {  (void)fprintf(stderr,"\nlol version %s, (C) Tumbling Dice 2005-2019 (built %s %s)\n\n",LOL_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"LOL is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"LOL comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fprintf(stderr,"\nUsage: lol [-usage | -help] | <lock/P3/PSRP file name>\n\n");
       (void)fflush(stderr);

       exit(-1);
    }


    /*----------------------------------------*/
    /* Does this channel/lock actually exist? */
    /*----------------------------------------*/

    (void)strlcpy(tmpstr,argv[1],SSIZE);
    if(access(tmpstr,F_OK) == (-1))
    {  printf("NOT_FOUND\n");
       (void)fflush(stdout);

       exit(-1);
    }


    /*----------------------*/
    /* Do we own this file? */
    /*----------------------*/

    (void)lstat(tmpstr,&buf);
    if(buf.st_uid != getuid())
    {  printf("NOT_OWNER\n");
       (void)fflush(stderr);

       exit(-1);
    }
    (void)mchrep(' ',".#:/",tmpstr);

    do {    ret = strext(' ',next_item,tmpstr);

            if(sscanf(next_item,"%d",&lock_pid) == 1)
            {

               /*----------------*/
               /* Is owner live? */
               /*----------------*/

               if(kill(lock_pid,SIGCONT) != (-1))
                  printf("LIVE\n");
               else
                  printf("DEAD\n");
               (void)fflush(stdout);

               exit(0);
            }
       } while(ret != END_STRING);

    exit(-1);
}
