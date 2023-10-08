/*-----------------------------------------------------------------------
    Purpose: Tool to configure P3 Makefiles for different
             architectures.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.01 
    Dated:   24th May 2022
    E-mail:  mao@tumblingdice.co.uk
------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <bsd/string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#ifdef SECURE
#include <sed_securicor.h>
#endif /* SECURE */


/*---------------------*/
/* Version of pupsconf */
/*---------------------*/

#define PUPSCONF_VERSION    "2.01"


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


/*------------------------*/
/* Authentication methods */
/*------------------------*/

#define USE_DISK_SERIAL    1
#define USE_SOFT_DONGLE    2


/*-----------------------------------------------*/
/* Default config dir (if non supplied via make) */
/*-----------------------------------------------*/

#ifndef DEFAULT_CONFIGDIR
#define DEFAULT_CONFIGDIR "../config"
#endif /* DEFAULT_CONFIGDIR */


/*-------------------------------------------------*/
/* Functions which are private to this application */
/*-------------------------------------------------*/

static int strin    (char *, char *);
static int ecryptstr(int, char *, char *);
static int error    (char *);





/*------------------*/
/* Main entry point */
/*------------------*/

int main(int argc, char *argv[])

{   int i,
        start                        = 1,
        ddes                         = (-1),
        auth_method                  = USE_DISK_SERIAL,
        m_names                      = 0;

    FILE *pstream                    = (FILE *)NULL,
         *cstream                    = (FILE *)NULL,
         *ostream                    = (FILE *)NULL;

    char m_d_name[SSIZE]             = "",
         next_m_s_frag[SSIZE]        = "",
         m_s_name[SSIZE]             = "",
         tmpfile[SSIZE]              = "",
         sed_c_frag[SSIZE]           = "",
         sed_command[SSIZE]          = "",
         appl_dongle_filename[SSIZE] = "",
         appl_dongle[SSIZE]          = "",
         disk_serial[SSIZE]          = "",
         hd_device[SSIZE]            = "",
         config_target[SSIZE]        = "",
         m_d_namelist[256][SSIZE]    = { "" };

    #ifdef SECURE
    #ifdef LINUX
    struct hd_driveid disk_info;
    #endif /* LINUX */
    #endif /* SECURE */

    if(argc < 4 || strcmp(argv[1],"-help") == 0 || strcmp(argv[1],"-usage") == 0)
    {  (void)fprintf(stderr,"\npupsconf version %s, (C) Tumbling Dice 2002-2022 (built %s %s)\n\n",PUPSCONF_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"PUPSCONF is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"PUPSCONF comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nusage: pupsconf [-usage | -help] | [-cdir <configuration directory:../config>]\n");
       (void)fprintf(stderr,"                !<target> <buildfile.in> <buildfile>!\n");


       #ifdef SECURE
       (void)fprintf(stderr,"                  [sdongle <dongle file name>] | [dserial <serial number>]\n\n");
       (void)fprintf(stderr,"Environmental variables (secure mode): USE_SOFT_DONGLE <dongle file>\n");
       (void)fprintf(stderr,"                                       USE_DISK_SERIAL (default)\n");
       #endif /* SECURE */
     
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
fprintf(stderr,"P3_CONFDIR: %s\n",getenv("P3_CONFDIR"));
fprintf(stderr,"TARGET: %s\n",argv[1]);
fflush(stderr);
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

    #ifdef SECURE
    if(getenv("USE_SOFT_DONGLE") != (char *)NULL)
    {  (void)strlcpy(appl_dongle_filename,getenv("USE_SOFT_DONGLE"),SSIZE);
       auth_method = USE_SOFT_DONGLE;
    }
    else
    {

       /*-------------------------------*/
       /* Default authentication method */
       /*-------------------------------*/

       auth_method = USE_DISK_SERIAL;


       /*-------------------------------------------------------------*/
       /* Has the user supplied an explicit serial number to be used? */
       /*-------------------------------------------------------------*/

       if(getenv("USE_DISK_SERIAL") != (char *)NULL)
          (void)strlcpy(disk_serial,getenv("USE_DISK_SERIAL"),SSIZE);
    }


    /*---------------------------------------------------------------*/
    /* Do we have a explicit disk serial number of soft dongle file? */
    /*---------------------------------------------------------------*/

    if(argc > start + 3)
    {  if(strcmp(argv[start + 3],"dserial") == 0)
       {  if(argc == start + 5)
             (void)strlcpy(disk_serial,argv[start + 4],SSIZE);
       }
       else if(strcmp(argv[start + 3],"sdongle") == 0)
       {  if(argc == start + 5)
          {  int itmp;

             if(sscanf(argv[start + 4],"%x",&itmp) != 1)
                (void)strlcpy(appl_dongle_filename,argv[start + 4],SSIZE);
             else
                (void)snprintf(appl_dongle,SSIZE,"%x",itmp);
          }
       }
       else
       {  (void)fprintf(stderr,"pupsconf: expecting either \"dserial\" or \"sdongle\"\n");
          (void)fflush(stderr);

          (void)kill(0,SIGTERM);
          exit(255);
       }
    }
    #endif /* SECURE */


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


    #ifdef SECURE
    /*--------------------------------------------------------------------*/
    /* If we have a disk serial in the build specification probe disk for */
    /* its serial number.                                                 */
    /*--------------------------------------------------------------------*/

    (void)strlcpy(hd_device,HD_DEVICE,SSIZE);

    if(auth_method & USE_DISK_SERIAL)
    {  

       /*-------------------------------------------------------*/
       /* Default case - probe build disk for its serial number */ 
       /*-------------------------------------------------------*/

       //if(strcmp(disk_serial,"") == 0)
       if(strcmp(hd_device,"/dev/dummy") != 0)
       {  if((ddes = open(hd_device,0)) == (-1))
          {  char errstr[SSIZE] = "";

             (void)snprintf(errstr,SSIZE,"securicor: cannot open \"%s\" to authorise application\n",hd_device);
             (void)error(errstr);
          }

          (void)ioctl(ddes,HDIO_GET_IDENTITY,(void *)&disk_info);
          (void)close(ddes);
          (void)ecryptstr(SEQUENCE_SEED,disk_info.serial_no,disk_serial);
       }
     
       (void)snprintf(sed_command,SSIZE,"sed s/DSN/\"%s\"/g < %s >$$; mv $$ %s",
                                    disk_serial,argv[start + 2],argv[start + 2]);
    
       (void)system(sed_command);
    }
    else if(auth_method & USE_SOFT_DONGLE)
    {  struct stat stat_buf;


       /*------------------------------------------------------------------------------------*/
       /* Default case - used (random) inode from specified soft dongle file to authenticate */
       /*------------------------------------------------------------------------------------*/

       if(strcmp(appl_dongle,"") == 0)
       {  if((ddes = open(appl_dongle_filename,0)) == (-1))
          {  char errstr[SSIZE] = "";

             (void)snprintf(errstr,SSIZE,"securicor: cannot open \"%s\" to authorise application\n",appl_dongle_filename);
             (void)error(errstr);
          }

          (void)fstat(ddes,&stat_buf);
          (void)snprintf(appl_dongle,SSIZE,"%x",stat_buf.st_ino);
          (void)close(ddes);
       }

       (void)snprintf(sed_command,SSIZE,"sed s/DSN/\"%s\"/g < %s >$$; mv $$ %s",
                                    appl_dongle,argv[start + 2],argv[start + 2]);

       (void)system(sed_command);
    }
    #endif /* SECURE */

    (void)fprintf(stderr,"    Build file for target \"%s\" generated in %s (%d macros)\n",
                                                  argv[start + 1],argv[start + 2],m_names);
    (void)fflush(stderr);

    exit(0);
}



/*-----------------------------------------------------------------------------
    Look for the occurence of string s2 within string s1 ...
-----------------------------------------------------------------------------*/              
             
static int strin(char *s1, char *s2)                                                   
             
{   int i,                                                                                   
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




/*------------------------------------------------------------------------------
    Encrypt a string ...
------------------------------------------------------------------------------*/

static int ecryptstr(int seed, char *plaintext, char *cipher)

{   int i;


    /*----------------*/
    /* Encrypt string */
    /*----------------*/

    (void)srand(seed);

    for(i=0; i<strlen(plaintext); ++i)
    {   cipher[i] = (unsigned char)((int)plaintext[i] ^ rand());

        while((int)cipher[i] < 60 || (int)cipher[i] > 85)
              cipher[i] = (unsigned char)((int)cipher[i] ^ rand());
    }

    cipher[i] = '\0';
    return(0);
}




/*------------------------------------------------------------------------------
    Error handler ...
------------------------------------------------------------------------------*/

static int error(char *error_string)

{  char hostname[SSIZE] = "";

   (void)gethostname(hostname,SSIZE);
   (void)fprintf(stderr,"    ERROR pupsconf (%d@%s)]: %s\n",getpid(),hostname,error_string);
   (void)fflush(stderr);

   exit(255);
}

