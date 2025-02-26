/*----------------------------------------------------
     Purpose: Extracts sub "twig" from pathname branch 

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.04 
     Dated:   10th Decemeber 2024 
     e-mail:  mao@tumblingdice.co.uk
----------------------------------------------------*/


#include <stdio.h>
#include <string.h>
#include <bsd/string.h>
#include <unistd.h>
#include <stdlib.h>
#include <xtypes.h>
#include <stdint.h>


/*-------------------*/
/* Version of branch */
/*-------------------*/

#define BRANCH_VERSION    "2.04"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE           2048 




/*---------------------------------*/
/* Main entry point of application */
/*---------------------------------*/

_PUBLIC int32_t main(int32_t argc, char *argv[])

{   size_t i,
           decoded          = 0,
           start            = 1,
           twigLength       = 1,
           cnt              = 0,
           cnt_pos          = 0;

    _BOOLEAN do_verbose     = FALSE;

    char hostname[SSIZE]    = "",
         pathname[SSIZE]    = "",
         twig[SSIZE]        = "";

    (void)gethostname(hostname,SSIZE);


    /*--------------------*/
    /* Parse command line */
    /*--------------------*/

    for(i=0; i<argc; ++i)
    {  if(argc == 1 || argc > 3 || strcmp(argv[i],"-usage") == 0 || strcmp(argv[i],"-help") == 0)
       {  (void)fprintf(stderr,"\nbranch version %s, (C) Tumbling Dice 2010-2024 (gcc %s: built %s %s)\n\n",BRANCH_VERSION,__VERSION__,__TIME__,__DATE__);
          (void)fprintf(stderr,"BRANCH is free software, covered by the GNU General Public License, and you are\n");
          (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
          (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
          (void)fprintf(stderr,"BRANCH comes with ABSOLUTELY NO WARRANTY\n\n");
          (void)fprintf(stderr,"\nUsage: branch [-usage | -help] | [-verbose:FALSE] [-tlength <twig length:1>]\n\n");
          (void)fflush(stderr);

          exit(255);
       }
       else if(strcmp(argv[i],"-verbose") == 0)
       {  do_verbose = TRUE;
          ++decoded;
          ++start;
       }
       else if(strcmp(argv[i],"-tlength") == 0)
       {  if(i == argc - 2 || argv[i+1][0] == '-')
          {  if(do_verbose == TRUE)
             {  (void)fprintf(stderr,"\nleaf (%d@%s): expecting twig length (cardinal  int32_teger >= 0)\n\n",getpid(),hostname);
                (void)fflush(stderr);
             }
             exit(255);
          }

          if(sscanf(argv[i+1],"%d",&twigLength) != 1 || twigLength < 1)
          {  if(do_verbose == TRUE)
             {  (void)fprintf(stderr,"\nleaf (%d@%s): twig length must be cardinal  int32_teger (>= 0)\n\n",getpid(),hostname);
                (void)fflush(stderr);
             }
             exit(255);
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
       exit(255);
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

    pathname[cnt_pos] = '\0';


    /*------------------------------------------*/
    /* Twig is base of branch (i.e. minus leaf) */
    /*------------------------------------------*/

    (void)printf("%s\n",pathname);
    (void)fflush(stdout);

    exit(0);
}
