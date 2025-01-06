/*-------------------------------------------------------
    Purpose: Tool to configure P3 Makefiles for different
             architectures.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 3.02 
    Dated:   11th December 2024 
    E-mail:  mao@tumblingdice.co.uk
------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <bsd/string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>


/*---------*/
/* Defines */
/*---------*/
/*---------*/
/* Version */
/*---------*/

#define PUPSCONF_VERSION    "3.02"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE               2048 


/*---------------*/
/* Boolean flags */
/*---------------*/

#ifdef TRUE
#undef TRUE
#endif /* TRUE */

#ifdef FALSE 
#undef FALSE 
#endif /* FALSE */

#define TRUE      255
#define FALSE     0


/*-----------------------------------------------*/
/* Default config dir (if non supplied via make) */
/*-----------------------------------------------*/

#ifndef DEFAULT_CONFIGDIR
#define DEFAULT_CONFIGDIR "../config"
#endif /* DEFAULT_CONFIGDIR */


/*-------------------------------------------------*/
/* Functions which are private to this application */
/*-------------------------------------------------*/

static int32_t strin    (const char *, const char *);
static int32_t error    (const char *);




/*------------------*/
/* Main entry point */
/*------------------*/

int main(int argc, char *argv[])

{   uint32_t i;

    int32_t  start                     = 1,
             ddes                      = (-1),
             m_names                   = 0;

    FILE     *pstream                  = (FILE *)NULL,
             *cstream                  = (FILE *)NULL,
             *ostream                  = (FILE *)NULL;

    char     m_d_name[SSIZE]           = "",
             next_m_s_frag[SSIZE]      = "",
             m_s_name[SSIZE]           = "",
             tmpfile[SSIZE]            = "",
             sed_c_frag[SSIZE]         = "",
             sed_command[SSIZE]        = "",
             disk_serial[SSIZE]        = "",
             hd_device[SSIZE]          = "",
             config_target[SSIZE]      = "",
             m_d_namelist[256][SSIZE]  = { "" };


    /*--------------------*/
    /* Parse command line */
    /*--------------------*/

    if(argc < 4 || strcmp(argv[1],"-help") == 0 || strcmp(argv[1],"-usage") == 0)
    {  (void)fprintf(stderr,"\npupsconf version %s, (C) Tumbling Dice 2002-2024 (gcc %s: built %s %s)\n\n",PUPSCONF_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fprintf(stderr,"PUPSCONF is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"PUPSCONF comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nusage: pupsconf [-usage | -help] | [-cdir <configuration directory:../config>]\n");
       (void)fprintf(stderr,"                !<target> <buildfile.in> <buildfile>!\n");


       (void)fprintf(stderr,"\nDefault config directory is: \"%s\"\n\n",DEFAULT_CONFIGDIR); 
       (void)fflush(stderr);

       exit(255);
    }

    if(strcmp(argv[1],"-cdir") == 0)
    {  start = 3;
       (void)snprintf(config_target,SSIZE,"%s/%s",argv[2],argv[3]);
    }
    else if(getenv("P3_CONFDIR") == (char *)NULL)
       (void)snprintf(config_target,SSIZE,"%s/%s",DEFAULT_CONFIGDIR,argv[1]);
    else
       (void)snprintf(config_target,SSIZE,"%s/%s",getenv("P3_CONFDIR"),argv[1]);


    #ifdef DEBUG  
    (void)fprintf(stderr,"P3_CONFDIR: %s\n",getenv("P3_CONFDIR"));
    (void)fprintf(stderr,"TARGET: %s\n",argv[1]);
    (void)fflush(stderr);
    #endif /* DEBUG */
     
    if((cstream = fopen(config_target,"r")) == (FILE *)NULL)
    {  (void)fprintf(stderr,"pupsconf: cannot find configuration file for target %s (pathname is %s)\n",argv[start],config_target);
       (void)fflush(stderr);

       (void)kill(0,SIGTERM);
       exit(255);
    }
    else
    {  (void)fprintf(stderr,"    Configuring %s for %s system\n",argv[start+1],argv[start]);
       (void)fflush(stderr);
    }


    /*---------------------------------------------------*/
    /* Check that we can actually access the target file */
    /*---------------------------------------------------*/

    if(access(argv[start + 1],R_OK) == (-1))
    {  (void)fprintf(stderr,"pupsconf: cannot find target %s\n",argv[start + 1]);
       (void)fflush(stderr);

       exit(255);
    }


    /*-----------------------------------------------------------------*/
    /* Main macro substitution loop -- get dummy name and string to be */
    /* substituted and build an appropriate sed command                */
    /*-----------------------------------------------------------------*/

    (void)strlcpy(sed_command,"sed \"",SSIZE);
    do {    do {   (void)fgets(m_d_name,SSIZE,cstream);
               } while(m_d_name[0] == '#' || m_d_name[0] == '\n');
            m_d_name[strlen(m_d_name) - 1] = '\0';

            if(strcmp(m_d_name,"end") == 0 || strcmp(m_d_name,"END") == 0)
               goto run_sed_command;


            /*------------------------------------------------------*/
            /* Move to next macro ignoring comments and empty lines */
            /*------------------------------------------------------*/

            do {   (void)fgets(next_m_s_frag,255,cstream);
               } while(next_m_s_frag[0] == '#' || next_m_s_frag[0] == '\n');


            /*---------------------------------*/
            /* Extract macro substitution text */
            /*---------------------------------*/

            next_m_s_frag[strlen(next_m_s_frag) - 1] = '\0';
            (void)strlcpy(m_s_name,next_m_s_frag,SSIZE);
            do {   (void)fgets(next_m_s_frag,255,cstream);

                   if(next_m_s_frag[0] != '#' && next_m_s_frag[0] != '\n')
                   {  next_m_s_frag[strlen(next_m_s_frag) - 1] = '\0';
                      (void)strlcat(m_s_name," ",          SSIZE);
                      (void)strlcat(m_s_name,next_m_s_frag,SSIZE);
                   }

               } while(next_m_s_frag[0] != '#' && next_m_s_frag[0] != '\n');

            for(i=0; i<m_names; ++i)
            {  if(strin(m_d_name,m_d_namelist[i]) == TRUE)
               {  (void)fprintf(stderr,"pupsconf: macro name \"%s\" (in %s) not unique\n",
                                                                          m_d_namelist[i],
                                                                            config_target);
                  (void)fflush(stderr);

                  (void)kill(0,SIGTERM);
                  exit(255);
               }
            }

            (void)strlcpy(m_d_namelist[m_names],m_d_name,SSIZE);

            if(strcmp(m_s_name,"no") == 0 || strcmp(m_s_name,"none") == 0)
               (void)snprintf(sed_c_frag,SSIZE," s!%s!!g;",m_d_name);
            else
               (void)snprintf(sed_c_frag,SSIZE," s!%s!%s!g;",m_d_name,m_s_name);


            #ifdef DEBUG
            (void)fprintf(stderr,"CMD: %s\n",sed_command);
            (void)fprintf(stderr,"FRAG: %s [ %s ] (%s)\n\n",m_d_name,m_s_name,sed_c_frag);
            (void)fflush(stderr);
            #endif /* DEBUG */

            (void)strlcat(sed_command,sed_c_frag,SSIZE);
            ++m_names;
       } while(1);

run_sed_command:

    if(access(argv[start + 2],F_OK | R_OK | W_OK) == (-1))
       (void)creat(argv[start + 2],0644);
    if((ostream = fopen(argv[start + 2],"w")) == (FILE *)NULL)
    {  (void)fprintf(stderr,"pupsconf: cannot open \"%s\"\n",ostream);
       (void)fflush(stderr);

       exit(01);
    }

    (void)fprintf(ostream,"# Build file generated automatically by pupsconf (version 1.0)\n");
    (void)fprintf(ostream,"# do not edit\n\n");

    (void)fclose(ostream);

    (void)snprintf(tmpfile,SSIZE,"\" <%s >%s",argv[start + 1],argv[start + 2]);
    (void)strlcat(sed_command,tmpfile,SSIZE);
    (void)system(sed_command);


    (void)fprintf(stderr,"    Build file for target \"%s\" generated in %s (%d macros)\n",
                                                  argv[start + 1],argv[start + 2],m_names);
    (void)fflush(stderr);

    exit(0);
}



/*------------------------------------------------------*/
/* Look for the occurence of string s2 within string s1 */
/*------------------------------------------------------*/              
             
static int32_t strin(const char *s1, const char *s2)                                                   
             
{   uint32_t i,                                                                                   
             cmp_size,
             chk_limit;

    if(strlen(s2) > strlen(s1))                                                
       return(FALSE);                                             

    chk_limit = strlen(s1) - strlen(s2) + 1;
    cmp_size  = strlen(s2);               

    for(i=0; i<chk_limit; ++i)         
       if(strncmp(&s1[i],s2,cmp_size) == 0)
          return(TRUE);                     
       
    return(FALSE);             
}




/*---------------*/
/* Error handler */
/*---------------*/

static int32_t error(const char *error_string)

{  char hostname[SSIZE] = "";

   (void)gethostname(hostname,SSIZE);
   (void)fprintf(stderr,"    ERROR pupsconf (%d@%s)]: %s\n",getpid(),hostname,error_string);
   (void)fflush(stderr);

   exit(255);
}

