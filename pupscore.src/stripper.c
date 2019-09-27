/*------------------------------------------------------------------
     Purpose: Strips comments out of streamed file

     Author:  M.A. O'Neill
              Tumbling Dice 
              Gosforth
              Newcastle
              NE3 4RT

     Version: 2.00 
     Dated:   30th August 2019 
     E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <xtypes.h>


/*---------------------*/
/* Version of stripper */
/*---------------------*/

#define STRIPPER_VERSION    "1.05"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE               2048 


/*-------------------------------------------------------------------
    Variables which are globally local to this application ...
-------------------------------------------------------------------*/

/*-------------------------------------------------------------------
    Return first non whitespace character in a string ...
-------------------------------------------------------------------*/

_PRIVATE char *strfirst(char *s1)

{   int i,
        size;

    size = strlen(s1);
    for(i=0; i<size; ++i)
    {  if(s1[i] != ' ' && s1[i] != '\n' && s1[i] != '\r' && s1[i] != '\0')
         return(&s1[i]);
    }

    return((char *)NULL);
}





/*-------------------------------------------------------------------
    Test for empty string (contains only whitespace and
    control chars) ...
-------------------------------------------------------------------*/

_PRIVATE _BOOLEAN strempty(char *s)

{   int i;

    for(i=0; i<strlen(s); ++i)
    {  if(s[i] != ' ' && s[i] != '\n')
         return(FALSE);
    }

    return(TRUE);
}




/*-------------------------------------------------------------------
    Look for the occurence of string s2 within string s1 ...
-------------------------------------------------------------------*/

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




/*--------------------------------------------------------------------
    Strip comments in place ...
--------------------------------------------------------------------*/

_PRIVATE _BOOLEAN strip_comment(FILE *stream, int *line_cnt, char *line)

{

    /*-------------------------------------------------*/
    /* Wind to next line which is not either a comment */
    /* or whitespace                                   */
    /*-------------------------------------------------*/

    do {   (void)fgets(line,SSIZE,stream);

           if(feof(stream) != 0)
              return(FALSE);

           ++(*line_cnt);
       } while(strempty(line) == TRUE || strin(strfirst(line),"#") == TRUE);

    return(TRUE);
}




/*-------------------------------------------------------------------
    Main entry point
-------------------------------------------------------------------*/

_PUBLIC int main(int argc, char *argv[])

{   int i,
        line_cnt   = 0,
        decoded    = 0,
        n_comments = 0;

    _BOOLEAN do_verbose  = FALSE,
             not_comment = FALSE,
             looper      = TRUE;

    char hostname[SSIZE] = "",
         line[SSIZE]     = "";

    for(i=0; i<argc; ++i)
    {  if(argc == 1 || strcmp(argv[1],"-usage") == 0 || strcmp(argv[1],"-help") == 0)
       {  

          (void)fprintf(stderr,"\nstripper version %s, (C) Tumbling Dice 2002-2019 (built %s)\n\n",STRIPPER_VERSION,__TIME__,__DATE__);
          (void)fprintf(stderr,"STRIPPER is free software, covered by the GNU General Public License, and you are\n");
          (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
          (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
          (void)fprintf(stderr,"STRIPPER comes with ABSOLUTELY NO WARRANTY\n\n");

          (void)fprintf(stderr,"\nUsage: stripper [-usage | -help] |  [-verbose] [< <commented file>]  [> <stripped file>] [>& <error/status log]\n\n");
          (void)fflush(stderr);

          exit(1);
       }
       else if(strcmp(argv[i],"-verbose") == 0) 
       {  do_verbose = TRUE;
          ++decoded;
       }
    }

    if(do_verbose == TRUE)
    {  (void)fprintf(stderr,"\n    stripper (comment stripper)  version %s, (C) Tumbling Dice, 2018\n\n",STRIPPER_VERSION);
       (void)fflush(stderr);
    }

    if(decoded < argc - 1)
    {  if(do_verbose == TRUE)
       {  (void)fprintf(stderr,"stripper (%d@%s): undecoded command tail items\n",getpid(),hostname);
          (void)fflush(stderr);
       }

       exit(-1);
    }


    /*--------------*/
    /* Get hostname */
    /*--------------*/

    (void)gethostname(hostname,SSIZE);


    /*---------------------------------------------------*/
    /* Check that we are conencted to a valid datasource */
    /*---------------------------------------------------*/

    if(isatty(0) == 1)
    {  if(do_verbose == TRUE)
       {  (void)fprintf(stderr,"stripper (%d@%s): standard input must be connected to a file or FIFO\n\n",getpid(),hostname);
          (void)fflush(stderr);
       }

       exit(-1);
    }


    /*---------------------------------------------------------------------------*/
    /* Get data from standard input strip comments prepended by a "#" symbol and */
    /* write uncommented data to standard output                                 */
    /*---------------------------------------------------------------------------*/

    do {   not_comment = strip_comment(stdin,&line_cnt,line);
           n_comments += (line_cnt - 1);
           line_cnt    = 0;

           if(feof(stdin) != 0)
              looper = FALSE;
           else
           {  /* Strip comments */
              if(not_comment == TRUE)
              {  (void)fputs(line,stdout);
                 (void)fflush(stdout);
              }
           }
       } while(looper == TRUE);


    /*----------------------*/
    /* Conversion completed */
    /*----------------------*/

    if(do_verbose == TRUE)
    {  (void)fprintf(stderr,"\nstripper (%d@%s): finished (%d comments stripped)\n",getpid(),hostname,n_comments);
       (void)fflush(stderr);
    }
 
    exit(0);
}
