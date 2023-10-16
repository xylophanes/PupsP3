/*-----------------------------------------------------------------------------
    Purpose: Start a homogenous farm of processes

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.03
    Dated:   24th May 2022
    E-mail:  mao@tumblingdice.co.uk
-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <xtypes.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <bsd/string.h>
#include <unistd.h>
#include <stdlib.h>


/*-----------------------------------------------------------------------------
    Defines which are private to this application ...
-----------------------------------------------------------------------------*/
/*-----------------*/
/* Version of farm */
/*-----------------*/

#define FARM_VERSION     "2.03"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE            20148 


#define MAX_PIDS         1024 
#define END_STRING       (-1)




/*-----------------------------------------------------------------------------
    Look for the occurence of string s2 within string s1 ...
-----------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN strin(char *s1, char *s2)

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
    Routine to extract substrings which are demarkated by a usr defined
    character ...
------------------------------------------------------------------------------*/
                                      /*--------------------------------------*/
_PRIVATE _BOOLEAN strext(char dm_ch,  /* Demarcation character                */
                         char   *s1,  /* Extracted sub string                 */
                         char   *s2)  /* Argument string                      */
                                      /*--------------------------------------*/
{   int s1_ptr = 0;

                                      /*--------------------------------------*/
    _IMMORTAL int  s2_ptr = 0;        /* Current pointer into arg string      */
    _IMMORTAL char *s2_was;           /* Copy of current argument string      */
                                      /*--------------------------------------*/


    /*-------------------------------------------------------------------*/
    /* Entry with null parameters forces reset of pointers within strext */
    /* function                                                          */
    /*-------------------------------------------------------------------*/

    if(s2 == (char *)NULL)
    {  s2_ptr = 0;
       return(FALSE);
    }


    /*--------------------------------------------------------------------*/
    /* Save string length for swap test if this is first pass for current */
    /* extraction string                                                  */
    /*--------------------------------------------------------------------*/

    if(s2_ptr == 0)
    {  s2_was = (char *)malloc((size_t)SSIZE);
       (void)strlcpy(s2_was,s2,SSIZE);
    }
    else
    {

       /*------------------------------------------------*/
       /* If extraction string has changed re-initialise */
       /*------------------------------------------------*/

       if(strcmp(s2_was,s2) != 0)
       {  s2_ptr = 0;
          strlcpy(s2_was,s2,SSIZE);
       }
    }


    /*-----------------------------------*/
    /* Wind to substring to be extracted */
    /*-----------------------------------*/

    while(s2[s2_ptr] == dm_ch && s2[s2_ptr] != '\0' && s2[s2_ptr] != '\n')
          ++s2_ptr;


    /*---------------------------------------------------------*/
    /* If we have reached the end of the string - reinitialise */
    /*---------------------------------------------------------*/

    if(s2[s2_ptr] == '\0' || s2[s2_ptr] == '\n')
    {  s2_ptr = 0;
       s1[0] = '\0';
       free(s2_was);
       return(FALSE);
    }


    /*-------------------------------------------------*/
    /* Extract substring to next demarcation character */
    /*-------------------------------------------------*/

    while(s2[s2_ptr] != dm_ch && s2[s2_ptr] != '\0' && s2[s2_ptr] != '\n')
          s1[s1_ptr++] = s2[s2_ptr++];
    s1[s1_ptr] = '\0';


    /*--------------------------------------------------------------------*/
    /* If the end of the extraction string has been reached, reinitialise */
    /*--------------------------------------------------------------------*/

    if(s2[s2_ptr] == '\0' || s2[s2_ptr] == '\n')
    {  s2_ptr = 0;
       free(s2_was);

                              /*----------------------------------------------*/
       return(END_STRING);    /* Messy but some applications need to know if  */
                              /* dm_ch was really matched or if we have hit   */
                              /* the end of the string                        */
                              /*----------------------------------------------*/
    }

    return(TRUE);
}




/*-----------------------------------------------------------------------------
    Execls - routine to overlay and execute a command string ...
-----------------------------------------------------------------------------*/

_PRIVATE int execls(char *command)

{   int ret,
          j,
          i = 0;

    char     exec_command[SSIZE] = ""; 
    _BOOLEAN looper;


    if(command == (char *)NULL)
       return(-1);

    (void)snprintf(exec_command,SSIZE,"exec %s",command);
    ret = execlp("bash","bash","-c",exec_command,(char *)NULL);


    /*------------------------*/
    /* We should not get here */
    /*------------------------*/

    return(-1);
}



/*-----------------------------------------------------------------------------
    Handler for failed exec attempts ...
-----------------------------------------------------------------------------*/

_PRIVATE int current_index;
_PRIVATE int  pid[MAX_PIDS] = { (-1) };
_PRIVATE int exec_handler(int signum)

{   pid[current_index] = (-1);
    (void)signal(SIGUSR1,(void *)&exec_handler);
}


int term_handler(int signum)

{   (void)fprintf(stderr,"terminated\n");
    (void)fflush(stderr);
}




/*-----------------------------------------------------------------------------
    Main entry point for program ...
-----------------------------------------------------------------------------*/

_PRIVATE char hostname[SSIZE] = "";
_PUBLIC int main(int argc, char *argv[])

{   int i,
        n_procs,
        status,
        cnt             = 0;

    char command[SSIZE] = "";
    _BOOLEAN verbose    = FALSE;

    (void)gethostname(hostname,SSIZE);


    /*-------------------------------*/
    /* Check and decode command tail */
    /*-------------------------------*/

    if(argc < 3 || argc > 4)
    {  (void)fprintf(stderr,"\nfarm version %s, (C) Tumbling Dice 2002-2022 (built %s %s)\n\n",FARM_VERSION,__TIME__,__DATE__); 
       (void)fprintf(stderr,"FARM is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"FARM comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"usage: farm [-verbose] <n processes in farm> <command name>\n\n");
       (void)fflush(stderr);

       exit(255);
    }

    if(argc == 4)
    {  if(strcmp(argv[1],"-verbose") != 0)
       {  (void)fprintf(stderr,"\nfarm version %s, (C) Tumbling Dice 2022 (built %s %s)\n\n",FARM_VERSION,__TIME__,__DATE__);
          (void)fprintf(stderr,"FARM is free software, covered by the GNU General Public License, and you are\n");
          (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
          (void)fprintf(stderr,"See the GpL and LGPL licences at www.gnu.org for further details\n");
          (void)fprintf(stderr,"FARM comes with ABSOLUTELY NO WARRANTY\n\n");

          (void)fprintf(stderr,"usage: farm [-verbose] <n processes in farm> <command name>\n\n");
          (void)fflush(stderr);

          exit(255);
       }
 
       verbose = TRUE;
       (void)sscanf(argv[2],"%d",&n_procs);
       (void)strlcpy(command,argv[3],SSIZE);
    }
    else
    {  verbose = FALSE;
       (void)sscanf(argv[1],"%d",&n_procs);
       (void)strlcpy(command,argv[2],SSIZE);
    }

    if(n_procs > MAX_PIDS)
    {  if(verbose == TRUE)
       {  (void)fprintf(stderr,"farm (%d@%s): too many processes in farm (maximum of %d)\n",getpid(),hostname,MAX_PIDS);
          (void)fflush(stderr);
       }

       exit(255);
    }


    /*------------*/
    /* Start farm */
    /*------------*/

    for(i=0; i<n_procs; ++i)
    {  

       if((pid[i] = fork()) == 0)
       {  if(execls(command) == (-1))
          {  if(verbose == TRUE)
             {  (void)fprintf(stderr,"farm (%d@%s): failed to exec instance %d of command \"%s\"\n",getpid(),hostname,i,command);
                (void)fflush(stderr);
             }

             (void)killpg(0,SIGTERM);
             exit(255);
          }
       }
       else if(pid[i] == 0)
       {  if(verbose == TRUE)
          {  (void)fprintf(stderr,"farm (%d@%s): fork (%d) failed\n",getpid(),hostname,i);
             (void)fflush(stderr);
          }

          (void)killpg(0,SIGTERM);
          exit(255);
       }
    }

    if(verbose == TRUE)
    {  if(n_procs == 1)
          (void)fprintf(stderr,"farm (%d@%s): 1 instance of \"%s\" launched\n",getpid(),hostname,command);
       else
          (void)fprintf(stderr,"farm (%d@%s): %d instances of \"%s\" launched\n",getpid(),hostname,n_procs,command);

       (void)fflush(stderr);
    }


    /*--------------------------------------------------------------*/
    /* Wait for children to complete if we are a foreground process */
    /*--------------------------------------------------------------*/

    (void)signal(SIGUSR1,(void *)exec_handler);
    (void)signal(SIGTERM,(void *)term_handler);
    do {   for(i=0; i<n_procs; ++i)
           {  if(pid[i] != (-1))
              {  int ret;

                 ret = waitpid(pid[i],&status,WNOHANG);


                 /*-------------------------------*/
                 /* Did the child die of old age? */
                 /*-------------------------------*/

                 if(ret > 0)
                 {  if((WIFSIGNALED(status) && WTERMSIG(status) != SIGSEGV) || WEXITSTATUS(status) == (-1))
                    {  if(verbose == TRUE)
                       {  (void)fprintf(stderr,"farm (%d@%s): launching replacement for (lost) instance %d of command \"%s\"\n",
                                                                                                    getpid(),hostname,i,command);
                          (void)fflush(stderr);
                       }

                       current_index = i; 
                       if((pid[i] = fork()) == 0)
                       {  if(execls(command) == (-1))
                          {  if(verbose == TRUE)
                             {  (void)fprintf(stderr,"farm (%d@%s): failed to launch replacement for (lost) instance %d of command \"%s\"\n",
                                                                                                                 getpid(),hostname,i,command);
                                (void)fflush(stderr);
                             }

                             (void)kill(getppid(),SIGUSR1);
                             _exit(255);
                          }
                       }
                    } 
                    else
                       ++cnt;
                  }
               }

               (void)usleep(100);
           }
       } while(cnt < n_procs);

    exit(0);
}
