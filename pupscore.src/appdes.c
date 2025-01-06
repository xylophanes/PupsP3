/*--------------------------------------------------------------------------
     Purpose: Generate an appgen template (and Makefile) from skelpapp.c and
              Make_skelpapp.in 

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.03 
     Dated:   10th Decemeber 2024 
     E-mail:  mao@tumblingdice.co.uk
--------------------------------------------------------------------------*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <bsd/string.h>
#include <stdint.h>

#ifdef HAVE_READLINE
#include <readline/readline.h>
#endif /* HAVE_READLINE */


/*-------------------*/
/* Version of appdes */
/*-------------------*/

#define APPDES_VERSION    "2.03"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE             2048 


/*--------------------------------------*/
/* Strip control characters from string */
/*--------------------------------------*/

static void strstrp(char *s_in, char *s_out)

{   uint32_t i,
             cnt = 0;

    for(i=0; i<strlen(s_in); ++i)
    {  if(s_in[i] == '\n' || s_in[i] == '\r')
          ++i;

       if(s_in[i] == '\'')
          s_in[i] = ',';

       s_out[cnt++] = s_in[i];
    }
}




/*---------------------------------------------*/
/* Read input (with line editing if supported) */
/*---------------------------------------------*/

static void read_line(char *line, char *prompt)

{   char *tmp_line = (char *)NULL;


    /*-------------------------------------------------*/
    /* Line must be string of alpha-numeric characters */
    /*-------------------------------------------------*/

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





/*-----------------------------*/
/* Main entry point to process */
/*-----------------------------*/

int32_t main(int32_t argc, char *argv[])

{   char tmpstr[SSIZE]             = "",
         skelpapp[SSIZE]           = "",
         version[SSIZE]            = "",
         make_skelpapp[SSIZE]      = "",
         app_name[SSIZE]           = "",
         purpose[SSIZE]            = "",
         appdes[SSIZE]             = "",
         author[SSIZE]             = "",
         author_email[SSIZE]       = "",
         date[SSIZE]               = "",
         institution[SSIZE]        = "",
         sed_skelpapp[SSIZE]       = "",
         sed_make_skelpapp[SSIZE]  = "",
         sed_cmd[SSIZE]            = "",
         sed_cmds[SSIZE]           = "sed '";


    /*--------------------*/
    /* parse command line */
    /*--------------------*/

    if(argc == 1)
    {  if((char *)getenv("PUPS_SKELPAPP") != (char *)NULL && (char *)getenv("PUPS_MAKE_SKELPAPP") != (char *)NULL)
       { (void)strcpy(skelpapp,(char *)getenv("PUPS_MAKE_SKELPAPP")); 
         (void)strcpy(skelpapp,(char *)getenv("PUPS_SKELPAPP"));
       }
       else
       {  (void)strcpy(make_skelpapp,"Make_skelpapp.in");
          (void)strcpy(skelpapp,"skelpapp.c");
       }
    }

    else if(argc == 2 && strcmp(argv[1],"-usage") == 0)
    {  (void)fprintf(stderr,"\nPUPS appgen generator version %s (C), Tumbling Dice, 2002-2024 (gcc %s: built %s %s)\n",APPDES_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fprintf(stderr,"Usage: appgen [skeleton appgen file]\n\n");
       (void)fflush(stderr);
       (void)fprintf(stderr,"APPLICATION is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"APPLICATION comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fflush(stderr);

       exit(255);
    }

    else if(argc == 3)
    {  (void)strcpy(make_skelpapp,argv[1]);
       (void)strcpy(skelpapp,argv[2]);
    }

    else
    {  (void)fprintf(stderr,"\nPUPS appgen generator version %s, (C) Tumbling Dice, 2002-2024 (gcc %s: built %s %s)\n",APPDES_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fprintf(stderr,"Usage: appgen [skeleton appgen file]\n\n");
       (void)fflush(stderr);
       (void)fprintf(stderr,"APPLICATION is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"APPLICATION comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fflush(stderr);

       exit(255);
    }

    if(access(skelpapp,F_OK | R_OK | W_OK) == (-1))
    {  (void)fprintf(stderr,"\nappgen: cannot find skeleton appgen template file \"%s\" or makefile \"%s\"\n\n",skelpapp,make_skelpapp);
       (void)fflush(stderr);

       exit(255);
    }

    if(access(make_skelpapp,F_OK | R_OK | W_OK) == (-1))
    {  (void)fprintf(stderr,"\nappgen: cannot find skeleton appgen template makefile \"%s\"\n\n",make_skelpapp);
       (void)fflush(stderr);

       exit(255);
    }


    /*------------------------------------*/
    /* Generate from appgen template file */
    /*------------------------------------*/

    (void)fprintf(stderr,"\nPUPS appgen generator version %s (C) M.A. O'Neill, Tumbling Dice, 2002-2024\n\n",APPDES_VERSION);
    (void)fflush(stderr); 

    read_line(app_name,"Application name> "),SSIZE);
    (void)strstrp(app_name,tmpstr);
    (void)snprintf(sed_cmd,SSIZE,"s/@APPNAME/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    read_line(version,"Application version (number)> "),SSIZE);
    (void)strstrp(version,tmpstr);
    (void)snprintf(sed_cmd,SSIZE,"s/@VERSION/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    read_line(purpose,"Application purpose> "),SSIZE);
    (void)fgets(purpose,SSIZE,stdin);
    (void)strstrp(purpose,tmpstr);
    (void)snprintf(sed_cmd,SSIZE,"s/@PURPOSE/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    read_line(appdes,"Application description> "),SSIZE);
    (void)strstrp(appdes,tmpstr);
    (void)snprintf(sed_cmd,SSIZE,"s/@APPDES/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    read_line(author,"Author> "),SSIZE);
    (void)strstrp(author,tmpstr);
    (void)snprintf(sed_cmd,SSIZE,"s/@AUTHOR/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    read_line(author_email,"Author email> "),SSIZE);
    (void)strstrp(author_email,tmpstr);
    (void)snprintf(sed_cmd,SSIZE,"s/@EMAIL/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    read_line(institution,"Author insitution> "),SSIZE);
    (void)strstrp(institution,tmpstr);
    (void)snprintf(sed_cmd,SSIZE,"s/@INSTITUTION/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);


    readline(date,"Date (year)> "),SSIZE);
    (void)strstrp(date,tmpstr);
    (void)snprintf(sed_cmd,SSIZE,"s/@DATE/%s/g; '",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);


    (void)sprintf(sed_skelpapp,"%s < %s > %s.c",sed_cmds,skelpapp,app_name);
    (void)sprintf(sed_make_skelpapp,"%s < %s > Make_%s.in",sed_cmds,make_skelpapp,app_name);
    (void)system(sed_skelpapp);
    (void)system(sed_make_skelpapp);

    (void)fprintf(stderr,"\n\nApplication \"%s\" generated (in %s.c, makefile is Make_%s.in)\n\n",app_name,app_name,app_name);
    (void)fflush(stderr); 
    exit(0);
}
