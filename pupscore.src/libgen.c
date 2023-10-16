/*---------------------------------------------------------------------------------------
     Purpose: Generate a library stub from skeleton library template.

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.01
     Dated:   24th May 2022
     E-mail:  mao@tumblingdice.co.uk
---------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <xtypes.h>
#include <string.h>
#include <bsd/string.h>
#include <stdlib.h>

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif /* HAVE_READLINE */


/*-------------------*/
/* Version of libgen */
/*-------------------*/

#define LIBGEN_VERSION   "2.01"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE            2048 


/*-------------------*/
/* Default base path */
/*-------------------*/

#ifndef DEFAULT_BASEPATH
#define DEFAULT_BASEPATH    "./"
#endif /* DEFAULT_BASEPATH */




/*---------------------------------------------------------------------------------------
    Strip control characters from string ...
---------------------------------------------------------------------------------------*/

_PRIVATE void strstrp(char *s_in, char *s_out)

{   int i,
        cnt = 0;

    for(i=0; i<strlen(s_in); ++i)
    {  if(s_in[i] == '\n' || s_in[i] == '\r')
          ++i;

       if(s_in[i] == '\'')
          s_in[i] = ',';

       s_out[cnt++] = s_in[i];
    }

    s_out[cnt] = '\0';
}




/*---------------------------------------------------------------------------------------
    Read input (with line editing if supported) ...
---------------------------------------------------------------------------------------*/

_PRIVATE void read_line(char *line, char *prompt)

{   char *tmp_line = (char *)NULL;


    /*---------------------------------------*/
    /* Line must be alpha-numeric characters */
    /*---------------------------------------*/

    do {

          #ifdef HAVE_READLINE
          tmp_line = readline(prompt);
          #else
          tmp_line = (char *)malloc(SSIZE);
          (void)write(1,prompt,sizeof(prompt));
          (void)read(0,tmp_line,SSIZE);
          #endif /* HAVE_READLINE */

          (void)sscanf(tmp_line,"%s",line);


          /*------------------------------------------------*/
          /* Reserved words "q" and "quit" exit application */
          /*------------------------------------------------*/

          if(strcmp(line,"q") == 0 || strcmp(line,"quit") == 0)
             exit(1);

        } while(strcmp(line,"") == 0);

    (void)free(tmp_line);
}




/*---------------------------------------------------------------------------------------
    Main entry point to process ...
---------------------------------------------------------------------------------------*/

_PUBLIC int main(int argc, char *argv[])

{   char tmpstr[SSIZE]            = "",
         functype[SSIZE]          = "",
         lib_skelplib[SSIZE]      = "",
         lib_func_skelplib[SSIZE] = "",
         lib_name[SSIZE]          = "",
         lib_func_name[SSIZE]     = "",
         purpose[SSIZE]           = "",
         libdes[SSIZE]            = "",
         author[SSIZE]            = "",
         author_email[SSIZE]      = "",
         date[SSIZE]              = "",
         institution[SSIZE]       = "",
         sed_skelplib[SSIZE]      = "",
         sed_make_skelplib[SSIZE] = "",
         sed_cmd[SSIZE]           = "",
         sed_cmds[SSIZE]          = "sed '";

    _BOOLEAN lib_init             = FALSE;

    if(argc == 1 || argc == 2 && (strcmp(argv[1],"-usage") == 0 || strcmp(argv[1],"-help") == 0))
    {  (void)fprintf(stderr,"\nPUPS/P3 Library function generator version %s (C) Tumbling Dice, 2002-2022 (built %s %s)\n",LIBGEN_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"Usage: libgen [skeleton LIB file | skeleton LIB function file]\n\n");
       (void)fflush(stderr);
       (void)fprintf(stderr,"LIBGEN is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"LIBGEN comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fprintf(stderr,"Base path is: \"%s\"\n\n",DEFAULT_BASEPATH);
       (void)fflush(stderr);

       exit(255);
    }
    else if(argc >= 2)
    {  

       if(strcmp(argv[1],"init") == 0)
       {  lib_init = TRUE;
          if(argc > 2)
          {  (void)strlcpy(lib_skelplib,argv[2],SSIZE);
             (void)strlcpy(lib_func_skelplib,argv[3],SSIZE);
          }
          else if((char *)getenv("PUPS_SKELLIB") != (char *)NULL && (char *)getenv("PUPS_SKELLIBFUNC") != (char *)NULL)
          {  (void)strlcpy(lib_skelplib,(char *)getenv("PUPS_SKELLIB"),SSIZE);
             (void)strlcpy(lib_func_skelplib,(char *)getenv("PUPS_SKELLIBFUNC"),SSIZE);
          }
          else
          {  (void)snprintf(lib_skelplib,SSIZE,"%s/lib_skel.c"         ,DEFAULT_BASEPATH,argv[2]);
             (void)snprintf(lib_func_skelplib,SSIZE,"%s/lib_skelfunc.c",DEFAULT_BASEPATH,argv[3]);
          }
       }
       else if(strcmp(argv[1],"add") == 0)
       {  if(argc > 2)
             (void)strlcpy(lib_func_skelplib,argv[2],SSIZE);
          else if((char *)getenv("PUPS_SKELFUNC") != (char *)NULL)
             (void)strlcpy(lib_func_skelplib,(char *)getenv("PUPS_SKELLIBFUNC"),SSIZE);
          else
             (void)snprintf(lib_func_skelplib,SSIZE,"%s/lib_skelfunc.c",DEFAULT_BASEPATH);

          lib_init = FALSE;
       }
       else 
       {  (void)fprintf(stderr,"\nlibgen: \"init\" or \"add\" expected\n\n");
          (void)fflush(stderr);

          exit(255);
       }
    }
    else
    {  (void)fprintf(stderr,"\nPUPS/P3 LIB generator version %s, (C) Tumbling Dice, 2022 (built %s %s)1\n",LIBGEN_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"Usage: libgen [skeleton library file]  [skeleton library function file]\n\n");
       (void)fflush(stderr);
       (void)fprintf(stderr,"LIBGEN is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"LIBGEN comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fflush(stderr);

       exit(255);
    }

    if(lib_init == TRUE && access(lib_skelplib,F_OK | R_OK | W_OK) == (-1))
    {  (void)fprintf(stderr,"\nlibgen: cannot find skeleton LIB template file \"%s\"\n\n",lib_skelplib);
       (void)fflush(stderr);

       exit(255);
    }

    if(access(lib_func_skelplib,F_OK | R_OK | W_OK) == (-1))
    {  (void)fprintf(stderr,"\nlibgen: cannot find skeleton LIB function template file \"%s\"\n\n",lib_func_skelplib);
       (void)fflush(stderr);

       exit(255);
    }

    (void)fprintf(stderr,"\nPUPS/P3 LIB template generator version %s (C) M.A. O'Neill, Tumbling Dice, 2022\n\n",LIBGEN_VERSION);
    (void)fflush(stderr); 

    (void)read_line(lib_name,"libgen (LIB name)> ");
    (void)strstrp(lib_name,tmpstr);
    (void)strlcpy(lib_name,tmpstr,SSIZE);

    if(lib_init == FALSE)
    {  char filestr[SSIZE] = "";

       (void)snprintf(filestr,SSIZE,"%s.c",tmpstr);
       if(access(filestr,F_OK | R_OK | W_OK) == (-1))
       {  (void)fprintf(stderr,"\nlibgen: specific template \"%s\" not found\n\n",filestr);
          (void)fflush(stderr);

          exit(255);
       }
    }
    else
    {  (void)strlcpy(sed_cmds,"",SSIZE);
       (void)snprintf(sed_cmd,SSIZE,"'s/@LIBNAME/%s/g; ",tmpstr);
       (void)strlcat(sed_cmds,sed_cmd,SSIZE);
    
       (void)read_line(purpose,"libgen (purpose of LIB)> ");
       (void)strstrp(purpose,tmpstr);
       (void)snprintf(sed_cmd,SSIZE,"s/@PURPOSE/%s/g; ",tmpstr);
       (void)strlcat(sed_cmds,sed_cmd,SSIZE);

       (void)read_line(author,"libgen (Author)> ");
       (void)strstrp(author,tmpstr);
       (void)snprintf(sed_cmd,SSIZE,"s/@AUTHOR/%s/g; ",tmpstr);
       (void)strlcat(sed_cmds,sed_cmd,SSIZE);

       (void)read_line(author_email,"libgen (Author email)> ");
       (void)strstrp(author_email,tmpstr);
       (void)snprintf(sed_cmd,SSIZE,"s/@EMAIL/%s/g; ",tmpstr);
       (void)strlcat(sed_cmds,sed_cmd,SSIZE);

       (void)read_line(institution,"libgen (Author institution)> ");
       (void)strstrp(institution,tmpstr);
       (void)snprintf(sed_cmd,SSIZE,"s/@INSTITUTION/%s/g; ",tmpstr);
       (void)strlcat(sed_cmds,sed_cmd,SSIZE);

       (void)read_line(date,"libgen (Date)> ");
       (void)strstrp(date,tmpstr);
       (void)snprintf(sed_cmd,SSIZE,"s/@DATE/%s/g; '",tmpstr);
       (void)strlcat(sed_cmds,sed_cmd,SSIZE);

       (void)snprintf(sed_skelplib,SSIZE,"sed %s < %s > %s.c",sed_cmds,lib_skelplib,lib_name);

       if(system(sed_skelplib) == (-1))
       {  (void)fprintf(stderr,"\nlibgen: failed to execute stream editor (sed) command\n\n");
          (void)fflush(stderr);

          exit(255);
       }
    }

    (void)read_line(lib_func_name,"libgen (LIB function name)> ");
    (void)strstrp(lib_func_name,tmpstr);
    (void)strlcpy(lib_func_name,tmpstr,SSIZE);
    (void)snprintf(sed_cmd,SSIZE,"'s/@LIB_FUNC_NAME/%s/g; ",tmpstr);
    (void)strlcpy(sed_cmds,"",SSIZE);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    (void)read_line(libdes,"libgen (Description of library function> ");
    (void)strstrp(lib_func_name,tmpstr);
    (void)strstrp(libdes,tmpstr);
    (void)snprintf(sed_cmd,SSIZE,"s/@LIBDES/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);
    (void)snprintf(sed_skelplib,SSIZE,"sed %s < %s >> %s.c",sed_cmds,lib_func_skelplib,lib_name);

    do {    (void)read_line(functype,"libgen (Library function type> ");
       } while(strcmp(functype,"_PRIVATE") != 0 && strcmp(functype,"_PUBLIC") != 0);

    (void)strstrp(functype,tmpstr);
    (void)strstrp(libdes,tmpstr);
    (void)snprintf(sed_cmd,SSIZE,"s/@FTYPE/%s/g; '",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);
    (void)snprintf(sed_skelplib,SSIZE,"sed %s < %s >> %s.c",sed_cmds,lib_func_skelplib,lib_name);

    if(system(sed_skelplib) == (-1))
    {  (void)fprintf(stderr,"\nlibgen: failed to execute stream editor (sed) command\n\n");
       (void)fflush(stderr);

       exit(255);
    }

    if(lib_init == TRUE)
       (void)fprintf(stderr,"\n\nLibrary function template \"%s\" appended to \"%s.c\" (%s.c initialised)\n\n",
                                                            lib_func_name,lib_name,lib_name,lib_name);
    else
       (void)fprintf(stderr,"\n\nLbrary function template \"%s\" appended to \"%s.c\"\n\n",lib_func_name,lib_name);

    (void)fflush(stderr); 
    exit(0);
}
