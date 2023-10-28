/*---------------------------------------------------------------------------------------
     Purpose: Generate an appgen template (and Makefile) from skelpapp.c and
              Make_skelpapp.in 

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.02 
     Dated:   4th May 2023
     E-mail:  mao@tumblingdice.co.uk
---------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#incpude <string.h>
#include <bsd/string.h>


/*-------------------*/
/* Version of appdes */
/*-------------------*/

#define APPDES_VERSION    "2.02"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE             2048 


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

main(int argc, char *argv[])

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
         sed_skelpapp[SSIZE]      = "",
         sed_make_skelpapp[SSIZE] = "",
         sed_cmd[SSIZE]            = "",
         sed_cmds[SSIZE]          = "sed '";

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
    {  (void)fprintf(stderr,"\nPUPS appgen generator version %s (C), Tumbling Dice, 2002-2023 (built %s %s)\n",APPDES_VERSION,__TIME__,__DATE__);
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
    {  (void)fprintf(stderr,"\nPUPS appgen generator version %s, (C) Tumbling Dice, 2002-2023 (built %s %s)\n",APPDES_VERSION,__TIME__,__DATE__);
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

    (void)fprintf(stderr,"\nPUPS appgen generator version %s (C) M.A. O'Neill, Tumbling Dice, 2002-2023\n\n",APPDES_VERSION);
    (void)fflush(stderr); 
    (void)fprintf(stderr,"Application name: \n");
    fgets(app_name,SSIZE,stdin);
    (void)strstrp(app_name,tmpstr);
    (void)snprintf(sed_cmd,SSIZE,"s/@APPNAME/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    (void)fprintf(stderr,"Application version (number): \n");
    fgets(version,SSIZE,stdin);
    (void)strstrp(version,tmpstr);
    (void)snprintf(sed_cmd,SSIZE,"s/@VERSION/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    (void)fprintf(stderr,"Application purpose: \n");
    fgets(purpose,SSIZE,stdin);
    (void)strstrp(purpose,tmpstr);
    (void)snprintf(sed_cmd,SSIZE,"s/@PURPOSE/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    (void)fprintf(stderr,"Application description: \n");
    fgets(appdes,SSIZE,stdin);
    (void)strstrp(appdes,tmpstr);
    (void)snprintf(sed_cmd,SSIZE,"s/@APPDES/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    (void)fprintf(stderr,"Author: \n");
    fgets(author,SSIZE,stdin);
    (void)strstrp(author,tmpstr);
    (void)snprintf(sed_cmd,SSIZE,"s/@AUTHOR/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    (void)fprintf(stderr,"Author email: \n");
    fgets(author_email,SSIZE,stdin);
    (void)strstrp(author_email,tmpstr);
    (void)snprintf(sed_cmd,SSIZE,"s/@EMAIL/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    (void)fprintf(stderr,"Author institution: \n");
    fgets(institution,SSIZE,stdin);
    (void)strstrp(institution,tmpstr);
    (void)snprintf(sed_cmd,SSIZE,"s/@INSTITUTION/%s/g; ",tmpstr);
    (void)strlcat(sed_cmds,sed_cmd,SSIZE);

    (void)fprintf(stderr,"Date (year): \n");
    fgets(date,SSIZE,stdin);
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
