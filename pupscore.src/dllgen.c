/*---------------------------------------------------------------------------------------
     Purpose: Generate a dll (source) library from template 

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.01
     Dated:   24th May 2023
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
/* Version of dllgen */ 
/*-------------------*/

#define DLLGEN_VERSION    "2.01"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE             2048 


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
         dll_skelpapp[SSIZE]      = "",
         dll_func_skelpapp[SSIZE] = "",
         dll_name[SSIZE]          = "",
         dll_func_name[SSIZE]     = "",
         purpose[SSIZE]           = "",
         dlldes[SSIZE]            = "",
         author[SSIZE]            = "",
         author_email[SSIZE]      = "",
         date[SSIZE]              = "",
         institution[SSIZE]       = "",
         sed_skelpdll[SSIZE]      = "",
         sed_make_skelpapp[SSIZE] = "",
         sed_cmd[SSIZE]           = "",
         sed_cmds[SSIZE]          = "sed '";

    _BOOLEAN dll_init             = FALSE;

    if(argc == 1 || argc > 4 || argc == 2 && (strcmp(argv[1],"-usage") == 0 || strcmp(argv[1],"-help") == 0))
    {  (void)fprintf(stderr,"\nPUPS/P3 DLL generator version %s, (C) Tumbling Dice, 2002-2023 (built %s %s)\n",DLLGEN_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"Usage: dllgen [skeleton DLL file | skeleton DLL function file]\n\n");
       (void)fflush(stderr);
       (void)fprintf(stderr,"DLLGEN is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"DLLGEN comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fprintf(stderr,"Base path is: \"%s\"\n\n",DEFAULT_BASEPATH);
       (void)fflush(stderr);

       exit(255);
    }
    else if(argc >= 2)
    {  if(strcmp(argv[1],"init") == 0)
       {  dll_init = TRUE;
          if(argc > 2)
          { (void)strlcpy(dll_skelpapp,argv[2],SSIZE);
            (void)strlcpy(dll_func_skelpapp,argv[3],SSIZE);
          }
          else if((char *)getenv("PUPS_SKELDLL") != (char *)NULL && (char *)getenv("PUPS_SKELDLLFUNC") != (char *)NULL)
          {  (void)strlcpy(dll_skelpapp,(char *)getenv("PUPS_MAKE_SKELDLL"),SSIZE);
             (void)strlcpy(dll_func_skelpapp,(char *)getenv("PUPS_SKELDLLFUNC"),SSIZE);
          }
          else
          {  (void)snprintf(dll_skelpapp,SSIZE,"%s/dll_skel.c",DEFAULT_BASEPATH);
             (void)snprintf(dll_func_skelpapp,SSIZE,"%s/dll_skelfunc.c",DEFAULT_BASEPATH);
          }
       }
       else if(strcmp(argv[1],"add") == 0)
       {  if(argc > 2)
             (void)strlcpy(dll_func_skelpapp,argv[2],SSIZE);
          else if((char *)getenv("PUPS_SKELDLLFUNC") != (char *)NULL)
             (void)strlcpy(dll_func_skelpapp,(char *)getenv("PUPS_SKELDLLFUNC"),SSIZE);
          else
             (void)snprintf(dll_func_skelpapp,SSIZE,"%s/dll_skelfunc.c",DEFAULT_BASEPATH);

          dll_init = FALSE;
       }
       else 
       {  (void)fprintf(stderr,"\ndllgen: \"init\" or \"add\" expected\n\n");
          (void)fflush(stderr);

          exit(255);
       }
    }
    else
    {  (void)fprintf(stderr,"\nPUPS/P3 DLL generator version %s, (C) Tumbling Dice, 2002-2023 (built %s %s)\n",DLLGEN_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"Usage: dllgen [skeleton DLL file | skeleton DLL function file]\n\n");
       (void)fflush(stderr);
       (void)fprintf(stderr,"DLLGEN is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"DLLGEN comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fflush(stderr);

       exit(255);
    }

    if(dll_init == TRUE && access(dll_skelpapp,F_OK | R_OK | W_OK) == (-1))
    {  (void)fprintf(stderr,"\ndllgen: cannot find skeleton DLL template file \"%s\"\n\n",dll_skelpapp);
       (void)fflush(stderr);

       exit(255);
    }

    if(access(dll_func_skelpapp,F_OK | R_OK | W_OK) == (-1))
    {  (void)fprintf(stderr,"\ndllgen: cannot find skeleton DLL function template file \"%s\"\n\n",dll_func_skelpapp);
       (void)fflush(stderr);

       exit(255);
    }

    (void)fprintf(stderr,"\nPUPS/P3 DLL template generator version %s (C) M.A. O'Neill, Tumbling Dice, 2002-2023\n\n",DLLGEN_VERSION);
    (void)fflush(stderr); 

    (void)read_line(dll_name,"dllgen (DLL name)> ");
    (void)strstrp(dll_name,tmpstr);
    (void)strlcpy(dll_name,tmpstr,SSIZE);

    if(dll_init == FALSE)
    {  char filestr[SSIZE] = "";

       (void)snprintf(filestr,SSIZE,"%s.c",tmpstr);
       if(access(filestr,F_OK | R_OK | W_OK) == (-1))
       {  (void)fprintf(stderr,"\ndllgen: specific template \"%s\" not found\n\n",filestr);
          (void)fflush(stderr);

          exit(255);
       }
    }
    else
    {  (void)strlcpy(sed_cmds,"",SSIZE);
       (void)snprintf(sed_cmd,SSIZE,"'s/@DLLNAME/%s/g; ",tmpstr);
       (void)strlcat(sed_cmds,sed_cmd,SSIZE);
    
       (void)read_line(purpose,"dllgen (purpose of DLL)> ");
       (void)strstrp(purpose,tmpstr);
       (void)snprintf(sed_cmd,SSIZE,"s/@PURPOSE/%s/g; ",tmpstr);
       (void)strlcat(sed_cmds,sed_cmd,SSIZE);

       (void)read_line(author,"dllgen (Author)> ");
       (void)strstrp(author,tmpstr);
       (void)snprintf(sed_cmd,SSIZE,"s/@AUTHOR/%s/g; ",tmpstr);
       (void)strlcat(sed_cmds,sed_cmd,SSIZE);

       (void)read_line(author_email,"dllgen (Author email)> ");
       (void)strstrp(author_email,tmpstr);
       (void)snprintf(sed_cmd,SSIZE,"s/@EMAIL/%s/g; ",tmpstr);
       (void)strlcat(sed_cmds,sed_cmd,SSIZE);

       (void)read_line(institution,"dllgen (Author institution)> ");
       (void)strstrp(institution,tmpstr);
       (void)snprintf(sed_cmd,SSIZE,"s/@INSTITUTION/%s/g; ",tmpstr);
       (void)strlcat(sed_cmds,sed_cmd,SSIZE);

       (void)read_line(date,"dllgen (Date)> ");
       (void)strstrp(date,tmpstr);
       (void)snprintf(sed_cmd,SSIZE,"s/@DATE/%s/g; '",tmpstr);
       (void)strlcat(sed_cmds,sed_cmd,SSIZE);

       (void)snprintf(sed_skelpdll,SSIZE,"sed %s < %s > %s.c",sed_cmds,dll_skelpapp,dll_name);

       if(system(sed_skelpdll) == (-1))
       {  (void)fprintf(stderr,"\ndllgen: failed to execute stream editor (sed) command\n\n");
          (void)fflush(stderr);

          exit(255);
       }
    }

    (void)read_line(dll_func_name,"dllgen (DLL function name)> ");
    (void)strstrp(dll_func_name,tmpstr);
    (void)strlcpy(dll_func_name,tmpstr,SSIZE);
    (void)snprintf(sed_cmd,SSIZE,"'s/@DLL_FUNC_NAME/%s/g; ",tmpstr);
    (void)strlcpy(sed_cmds,"",SSIZE);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    (void)read_line(dlldes,"dllgen (Description of DLL function> ");
    (void)strstrp(dll_func_name,tmpstr);
    (void)strstrp(dlldes,tmpstr);
    (void)sprintf(sed_cmd,SSIZE,"s/@DLLDES/%s/g; '",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);
    (void)sprintf(sed_skelpdll,SSIZE,"sed %s < %s >> %s.c",sed_cmds,dll_func_skelpapp,dll_name);

    if(system(sed_skelpdll) == (-1))
    {  (void)fprintf(stderr,"\ndllgen: failed to execute stream editor (sed) command\n\n");
       (void)fflush(stderr);

       exit(255);
    }

    if(dll_init == TRUE)
       (void)fprintf(stderr,"\n\nDLL function template \"%s\" appended to \"%s.c\" (%s.c initialised)\n\n",
                                                            dll_func_name,dll_name,dll_name,dll_name);
    else
       (void)fprintf(stderr,"\n\nDLL function template \"%s\" appended to \"%s.c\"\n\n",dll_func_name,dll_name);

    (void)fflush(stderr); 
    exit(0);
}
