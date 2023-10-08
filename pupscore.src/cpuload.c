/*--------------------------------------------------------------------------------
    Purpose: Compute CPU loading using mpstat untility

    Author:  Mark A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle
             NE3 4RT
             Tyne and Wear

    Version: 1.02
    Dated:   24th May 2022
    E-mail:  mao@tumblingdice.co.uk
---------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <bsd/string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <xtypes.h>
#include <dirent.h>


/*---------*/
/* Defines */
/*---------*/

#define CPULOAD_VERSION  "1.02"
#define SSIZE            2048
#define WINDOW_SIZE      32 


/*------------------*/
/* Global variables */
/*------------------*/

FILE  *pstream             = (FILE *)NULL;
FILE  *stream              = (FILE *)NULL;
char  linkName[SSIZE]      = "";
float loading[WINDOW_SIZE] = { 0.0 };


/*----------------*/
/* Signal handler */
/*----------------*/

static int sigHandler(int signum)

{    struct stat buf;

     if(pstream != (FILE *)NULL)
        (void)fclose(pstream);


     if(stream != (FILE *)NULL)
        (void)fclose(stream);


     /*---------------------------------*/
     /* Are we the last usage instance? */
     /*---------------------------------*/

     (void)stat("/tmp/cpuloading",&buf);


     /*-------------------*/
     /* Remove usage case */
     /*-------------------*/

     if(buf.st_nlink > 2)
        (void)unlink(linkName);


     /*-------------------------------------------*/
     /* Last usage case                           */
     /* Switch off the lights and close the door! */
     /*-------------------------------------------*/

     else
     {  (void)unlink("/tmp/cpuloading");
        (void)unlink("/tmp/.cpuloading");

        (void)unlink("/tmp/cpuload.pid");
        (void)unlink("/tmp/.cpuload.pid");
     }

     (void)fprintf(stderr,"    cpuload: finished (%d remaining usage cases)\n",buf.st_nlink - 2);
     (void)fflush(stderr);

     exit(1);
}




/*----------------------------*/
/* Window average CPU loading */
/*----------------------------*/

_PRIVATE float window_average_loading(unsigned int cnt, float next_loading)

{   unsigned int i;
    float        sum = 0.0;
    
    if(cnt < WINDOW_SIZE)
    {  loading[cnt] = next_loading;
 
       for(i=0; i<cnt+1; ++i)
          sum += loading[i];

       if(sum > 0.0)
          sum /= (float)(cnt + 1);
    }

    else
    {  loading[cnt % WINDOW_SIZE] = next_loading;
 
       for(i=0; i<WINDOW_SIZE; ++i)
          sum += loading[i];
       sum /= (float)WINDOW_SIZE;
    }

    return(sum);
}


/*------------------------*/
/* Remove stale links and */
/* repair critical files  */
/*------------------------*/

_PRIVATE void file_homeostat()

{   int           pid;
    DIR           *dirp      = (DIR *)NULL;
    struct dirent *next_item = (struct dirent *)NULL;


    /*----------------*/
    /* Critical files */
    /*----------------*/
    /*-------------*/
    /* CPU loading */
    /*-------------*/
    if(access("/tmp/cpuloading",F_OK) == (-1))
    {  (void)link("/tmp/.cpuloading","/tmp/cpuloading");

       (void)fprintf(stderr,"file /tmp/cpuloading lost (recreating)\n");
       (void)fflush(stderr);
    }


    /*--------------------*/
    /* CPU loading shadow */
    /*--------------------*/

    else if(access("/tmp/.cpuloading",F_OK) == (-1))
    {  (void)link("/tmp/cpuloading","/tmp/.cpuloading");

       (void)fprintf(stderr,"file /tmp/cpuloading shadow lost (recreating)\n");
       (void)fflush(stderr);
    }


    /*-----*/
    /* PID */
    /*-----*/

    if(access("/tmp/cpuload.pid",F_OK) == (-1))
    {  (void)link("/tmp/.cpuload..pid","/tmp/cpuload.pid");

       (void)fprintf(stderr,"file /tmp/cpuload.pid lost (recreating)\n");
       (void)fflush(stderr);
    }


    /*------------*/
    /* PID shadow */
    /*------------*/

    else if(access("/tmp/.cpuload.pid",F_OK) == (-1))
    {  (void)link("/tmp/cpuload..pid","/tmp/.cpuload.pid");

       (void)fprintf(stderr,"file /tmp/cpuload.pid shadow lost (recreating)\n");
       (void)fflush(stderr);
    }


    /*-------------*/
    /* Stale links */
    /*-------------*/

    dirp = opendir("/tmp");
    while((next_item = readdir(dirp)) != (struct dirent *)NULL)
    {  
       if(sscanf(next_item->d_name,"cpuloading.%d.link",&pid) == 1)
       {  char procpid[SSIZE] = "";
 
          (void)snprintf(procpid,SSIZE,"/proc/%d",pid);
          if(access(procpid,F_OK) == (-1))
          {  char tmpstr[SSIZE] = "";

             (void)snprintf(tmpstr,SSIZE,"/tmp/%s",next_item->d_name); 
             (void)unlink(tmpstr);

             (void)fprintf(stderr,"    cpuload: stale link (%s) removed\n",tmpstr);
             (void)fflush(stderr);
          }
       }
    }
    (void)closedir(dirp);
}



/*------------------*/
/* Main entry point */
/*------------------*/

_PUBLIC int main(int argc, char *argv[])

{   long  int cnt              = 0;

    char  strdum[SSIZE]        = "",
          mpstat_cmd[SSIZE]    = "";

    float next_loading,
          average_loading;


    /*------*/
    /* Help */
    /*------*/

    if(argc == 2                                           && 
       (strncmp(argv[1],"-help", strlen(argv[1])) == 0     ||
        strncmp(argv[1],"-usage",strlen(argv[1])) == 0     ))
    {  (void)fprintf(stderr,"\ncpuload version %s, (C) Tumbling Dice 2000-2022 (built %s %s)\n\n",CPULOAD_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"CPULOAD is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"CPULOAD comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nusage: cpuload [2> log/status file]\n\n");
       (void)fflush(stderr);

       exit(255);
    }


    /*----------------------------------*/
    /* Unparsed command line parameters */
    /*----------------------------------*/

    else if(argc > 1)
    {  (void)fprintf(stderr,"\n    cpuload ERROR: unparsed command line paramters\n\n");
       (void)fflush(stderr);

       exit(255);
    }


    /*------------------------*/
    /* Install signal handler */
    /*------------------------*/

    (void)signal(SIGTERM,(void *)&sigHandler);
    (void)signal(SIGHUP, (void *)&sigHandler);
    (void)signal(SIGINT, (void *)&sigHandler);
    (void)signal(SIGQUIT,(void *)&sigHandler);


    /*------------------------------------*/
    /* Check for running cpuload instance */
    /*------------------------------------*/
    /*-------------------------------------------*/
    /* First usage instance creates loading file */
    /*-------------------------------------------*/

    if(access("/tmp/cpuloading",F_OK) == (-1))
    {

       /*----------------------------------------*/
       /* /tmp/cpuloading is a devil of a file!! */
       /*----------------------------------------*/

       if(close(creat("/tmp/cpuloading",0666)) == (-1))
       {  (void)fprintf(stderr,"\n    cpuload ERROR: failed to create /tmp/cpuloading\n\n");
          (void)fflush(stderr);

          exit(255);
       }


       /*-----------------------*/
       /* Simple file homeostat */
       /*-----------------------*/

       else
          (void)link("/tmp/cpuloading","/tmp/.cpuloading");


       /*------------*/
       /* Record PID */
       /*------------*/

       (void)close(creat("/tmp/cpuload.pid",0666));
       (void)link("/tmp/cpuload.pid","/tmp/.cpuload.pid");

       stream = fopen("/tmp/cpuload.pid","w");
       (void)fprintf(stream,"%d\n",getpid());
       (void)fflush(stream);
       (void)fclose(stream);
       stream = (FILE *)NULL;

       /*-----------------------*/
       /* Remove any dead links */
       /*-----------------------*/

       (void)system("rm -f /tmp/cpuloading.*.link");

       (void)fprintf(stderr,"    cpuload: process %d monitoring cpu loading\n",getpid());
       (void)fflush(stderr);
    }


    /*----------------------------------------------*/
    /* Tell cpuload we wish to add a usage instance */
    /*----------------------------------------------*/

    else
    {  int  pid;

       char procpid[SSIZE]  = "";

       _BOOLEAN first = TRUE;


       (void)snprintf(linkName,SSIZE,"/tmp/cpuloading.%d.link",getpid());
       if(link("/tmp/cpuloading",linkName) == (-1))
       {  (void)fprintf(stderr,"\n    cpuload ERROR: failed to make (hard) link to /tmp/cpuloading\n\n");
          (void)fflush(stderr);

          exit(255);
       }


       /*--------------------------*/
       /* Has cpuload fallen over? */
       /*--------------------------*/

       stream = fopen("/tmp/cpuload.pid","r");
       (void)fscanf(stream,"%d\n",&pid);
       (void)fclose(stream);
       stream = (FILE *)NULL;


       /*-------------------------------------------*/
       /* Exit (there is a running cpuload process) */
       /*-------------------------------------------*/

       (void)snprintf(procpid,SSIZE,"/proc/%d",pid);
       while(access(procpid,F_OK) != (-1))
       {  if(first == TRUE)
          {  first = FALSE;

             (void)fprintf(stderr,"    cpuload: process %d is live (process %d going dormant)\n",pid,getpid());
             (void)fflush(stderr);
          }

          (void)sleep(1);
       } 
       
       (void)fprintf(stderr,"    cpuload: process %d has been terminated (process %d taking over as cpu load daemon)\n",pid,getpid());
       (void)fflush(stderr);


       /*------------*/
       /* Record PID */
       /*------------*/

       stream = fopen("/tmp/cpuload.pid","w");
       (void)fprintf(stream,"%d\n",getpid());
       (void)fflush(stream);
       (void)fclose(stream);
       stream = (FILE *)NULL;


       /*--------------------*/
       /* Remove (self) link */
       /*--------------------*/

       (void)snprintf(linkName,SSIZE,"/tmp/cpuloading.%d.link",getpid());
       (void)unlink(linkName);
    }


    /*---------------------------------*/
    /* Collect loading statistics once */
    /* per second                      */
    /*---------------------------------*/

    (void)strlcpy(mpstat_cmd,"bash -c \"mpstat 1\"",SSIZE);
    if((pstream = popen(mpstat_cmd,"r")) == (FILE *)NULL)
    {  (void)fprintf(stderr,"\n    cpuload ERROR: could not start mpstat command\n\n");
       (void)fflush(stderr);

       exit(255);
    }


    /*----------------------------------*/
    /* Update loading (in /tmp/cpuload) */
    /*----------------------------------*/

    if((stream = fopen("/tmp/cpuloading","w")) == (FILE *)NULL)
    {  (void)fprintf(stderr,"\n    cpuload ERROR: could not open /tmp/cpuload\n\n");
       (void)fflush(stderr);

       exit(255);
    }

    while(TRUE)
    {   char line[SSIZE] = "";

        (void)fgets(line,SSIZE,pstream);

        if(sscanf(line,"%s%s%f",strdum,strdum,&next_loading) == 3)
        {  average_loading = window_average_loading(cnt,next_loading);
           ++cnt; 

           (void)fprintf(stream,"%5.2f\n",average_loading);
           (void)fflush(stream);
           (void)rewind(stream);
        }


        /*---------------------------*/
        /* Reap any dead links       */
        /* and repair critical files */
        /*---------------------------*/

        file_homeostat();
    }    


    /*---------------------*/
    /* Should not get here */
    /*---------------------*/

    exit(0);
}
