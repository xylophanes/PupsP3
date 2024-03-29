/*---------------------------------------------------------------------------------------
     Purpose: Generate an application template (and Makefile) from skelpapp.c and
              Make_skelpapp.in 

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.02
     Dated:   24th May 2023 
     E-mail:  mao@tumblingdice.co.uk
---------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <atring.h>
#include <bsd/string.h>


/*------------------------*/
/* Version of application */
/*------------------------*/

#define APPLICATION_VERSION  "2.02"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE                2048 


/*---------------------------------------------------------------------------------------
    Strip control characters from string ...
---------------------------------------------------------------------------------------*/

void strstrp(char *s_in, char *s_out)

{   int i,
        cnt = 0;

    for(i=0; i<strlen(s_in); ++i)
    {  if(s_in[i] == '\n' || s_in[i] == '\r')
          ++i;

       if(s_in[i] == '\'')
          s_in[i] = ',';

       s_out[cnt++] = s_in[i];
    }
}



/*---------------------------------------------------------------------------------------
    Main entry point to process ...
---------------------------------------------------------------------------------------*/

_PUBLIC int main(int argc, char *argv[])

{   char tmpstr[SSIZE]            = "",
         skelpapp[SSIZE]          = "",
         make_skelpapp[SSIZE]     = "",
         app_name[SSIZE]          = "",
         purpose[SSIZE]           = "",
         appdes[SSIZE]            = "",
         author[SSIZE]            = "",
         author_email[SSIZE]      = "",
         date[SSIZE]              = "",
         institution[SSIZE]       = "",
         sed_skelpapp[SSIZE]      = "",
         sed_make_skelpapp[SSIZE] = "",
         sed_cmd[SSIZE]           = "",
         sed_cmds[SSIZE]          = "sed '";

    if(argc == 1)
    {  if((char *)getenv("PUPS_SKELPAPP") != (char *)NULL && (char *)getenv("PUPS_MAKE_SKELPAPP") != (char *)NULL)
       { (void)strlcpy(skelpapp,(char *)getenv("PUPS_MAKE_SKELPAPP"),SSIZE); 
         (void)strlcpy(skelpapp,(char *)getenv("PUPS_SKELPAPP"),SSIZE);
       }
       else
       {  (void)strlcpy(make_skelpapp,"Make_skelpapp.in",SSIZE);
          (void)strlcpy(skelpapp,"skelpapp.c",SSIZE);
       }
    }
    else if(argc == 2 && strcmp(argv[1],"-usage") == 0)
    {  (void)fprintf(stderr,"\nPUPS application generator version %s, (C) Tumbling Dice, 2002-2023  (built %s %s)\n",APPLICATION_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"Usage: application [skeleton application file]\n\n");
       (void)fflush(stderr);
       (void)fprintf(stderr,"APPLICATION is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"APPLICATION comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fflush(stderr);

       exit(255);
    }
    else if(argc == 3)
    {  (void)strlcpy(make_skelpapp,argv[1],SSIZE);
       (void)strlcpy(skelpapp,argv[2],SSIZE);
    }
    else
    {  (void)fprintf(stderr,"\nPUPS application generator version %s, Tumbling Dice, 2002-2023 (built %s %s)\n",APPLICATION_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"Usage: application [skeleton application file]\n\n");
       (void)fflush(stderr);
       (void)fprintf(stderr,"APPLICATION is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"APPLICATION comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fflush(stderr);

       exit(255);
    }

    if(access(skelpapp,F_OK  | R_OK | W_OK) == (-1))
    {  (void)fprintf(stderr,"\napplication: cannot find/access skeleton application template file \"%s\" or makefile \"%s\"\n\n",
                                                                                                          skelpapp,make_skelpapp);
       (void)fflush(stderr);

       exit(255);
    }

    if(access(make_skelpapp,F_OK | R_OK | W_OK) == (-1))
    {  (void)fprintf(stderr,"\napplication: cannot find/access skeleton application template makefile \"%s\"\n\n",
                                                                                                    make_skelpapp);
       (void)fflush(stderr);

       exit(255);
    }

    (void)fprintf(stderr,"\nPUPS application generator version 1.00 (C) M.A. O'Neill, Tumbling Dice, 2002\n\n");
    (void)fflush(stderr); 
    (void)fprintf(stderr,"Application name: \n");
    fgets(app_name,SSIZE,stdin);
    (void)strstrp(app_name,tmpstr);
    (void)strlcpy(app_name,tmpstr,SSIZE);
    (void)sprintf(sed_cmd,"s/@APPNAME/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    (void)fprintf(stderr,"Purpose of application: \n");
    fgets(purpose,SSIZE,stdin);
    (void)strstrp(purpose,tmpstr);
    (void)sprintf(sed_cmd,"s/@PURPOSE/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    (void)fprintf(stderr,"Description of application: \n");
    fgets(appdes,SSIZE,stdin);
    (void)strstrp(appdes,tmpstr);
    (void)sprintf(sed_cmd,"s/@APPDES/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    (void)fprintf(stderr,"Author: \n");
    fgets(author,SSIZE,stdin);
    (void)strstrp(author,tmpstr);
    (void)sprintf(sed_cmd,"s/@AUTHOR/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    (void)fprintf(stderr,"Author email: \n");
    fgets(author_email,SSIZE,stdin);
    (void)strstrp(author_email,tmpstr);
    (void)sprintf(sed_cmd,"s/@EMAIL/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    (void)fprintf(stderr,"Author institution: \n");
    fgets(institution,SSIZE,stdin);
    (void)strstrp(institution,tmpstr);
    (void)sprintf(sed_cmd,"s/@INSTITUTION/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    (void)fprintf(stderr,"Date (year): \n");
    fgets(date,SSIZE,stdin);
    (void)strstrp(date,tmpstr);
    (void)sprintf(sed_cmd,"s/@DATE/%s/g; '",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    (void)sprintf(sed_skelpapp,"%s < %s > %s.c",sed_cmds,skelpapp,app_name);
    (void)sprintf(sed_make_skelpapp,"%s < %s > Make_%s.in",sed_cmds,make_skelpapp,app_name);

    (void)system(sed_skelpapp);
    (void)system(sed_make_skelpapp);

    (void)fprintf(stderr,"\n\nApplication \"%s\" generated (in %s.c, makefile is Make_%s.in)\n\n",app_name,app_name,app_name);
    (void)fflush(stderr); 
    exit(0);
}
