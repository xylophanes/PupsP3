/*---------------------------------------------------------
    Purpose: get textual infomation for use in shell script

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 4.05 
    Dated:   10th December 2024 
    E-mail:  mao@tumblingdice.co.uk
--------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <bsd/string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif /* HAVE_READLINE */

#include <xtypes.h>


/*---------*/
/* Version */
/*---------*/

#define ASK_VERSION     "4.04"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE           2048 


/*---------*/
/* History */
/*---------*/

#define ASK_MAX_HISTORY 1024



/*-----------------------------------------------*/
/* Variables which are local to this application */
/*-----------------------------------------------*/

_PRIVATE pid_t child_pid = (-1);


/*------------------------------------------------------*/
/* Look for the occurence of string s2 within string s1 */
/*------------------------------------------------------*/

_PRIVATE _BOOLEAN strin(char *s1, char *s2)

{   size_t i,
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




/*--------------------------------------------------------------------*/
/* Test for empty string (contains only whitespace and control chars) */ 
/*--------------------------------------------------------------------*/

_PRIVATE _BOOLEAN strempty(const char *s)

{   size_t i,
           size;

    if(s == (const char *)NULL)
       return(-1);

    size = strlen(s);


    /*------------------------------*/
    /* Do not process empnty string */
    /*------------------------------*/

    if(size > 0)
    {  for(i=0; i<strlen(s); ++i)
       {  if(s[i] != ' ' && s[i] != '\n')
             return(FALSE);
       }

       return(TRUE);
    }

    return(-1);
}




/*-------------------------------------------------------------------*/
/* Routine to copy a string filtering out characters which have been */
/* marked as excluded                                                */
/*-------------------------------------------------------------------*/

_PUBLIC size_t strexccpy(const char *s1, char *s2, const char *ex_ch)

{   size_t i,
           j,
           k,
           size_1,
           size_2;

    if(s1    == (const char *)NULL    ||
       s2    == (char *)      NULL    ||
       ex_ch == (const char *)NULL     )
       return(-1);

    size_1 = strlen(s1);
    size_2 = strlen(ex_ch);


    /*-----------------------------*/
    /* Do not process empty string */
    /*-----------------------------*/

    if(size_1 > 0 && size_2 > 0)
    {
       j = 0;
       for(i=0; i<size_1; ++i)
       {   for(k=0; k<size_2; ++k)
           {  if(s1[i] == ex_ch[k])
                 goto exclude;
           }

           s2[j] = s1[i];
           ++j;

exclude:   continue;

       }

       return(j);
    }

    return(-1);
}




/*--------------*/
/* Exit handler */
/*--------------*/

_PRIVATE int32_t exit_handler(int32_t signum)

{   if(child_pid != (-1))
       (void)kill(child_pid,SIGTERM);

    (void)fprintf(stderr,"\n");
    (void)fflush(stderr);

    (void)exit(255);
}




/*---------------------------------------*/
/* Test to see if string contains spaces */
/*---------------------------------------*/

_PRIVATE  int32_t no_space(char *s)

{   uint32_t i,
             cnt = 0;


    /*----------------------*/
    /* Strip leading spaces */
    /*----------------------*/

    for(i=0; i<strlen(s); ++i)
    {  if(s[i] == ' ' || s[i] == '\t')
          ++cnt;
       else
          break;
    }


    /*-----------------------*/
    /* Strip trailing spaces */
    /*-----------------------*/

    for(i=strlen(s); i >= 0; --i)
    {  if(s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\0')
          s[i] = '\0';
       else
          break;
    }

    for(i=cnt; i<strlen(s); ++i)
    {  if(s[i] == ' ' || s[i] == '\t')
	  return(-1);
    }

    return(cnt);
}





/*------------------*/
/* Main entry point */
/*------------------*/

_PUBLIC int32_t main(int32_t argc, char *argv[])

{    int32_t  ret,
              in,
              status,
              out,
              err,
              prompt_index               = 1;

    _BOOLEAN looper                      = TRUE;

    char     value[SSIZE]                = "",
             ask_history[SSIZE]          = "",
             ask_history_pathname[SSIZE] = "";

    FILE     *out_stream                 = (FILE *)NULL;


    /*--------------------*/
    /* Parse command line */
    /*--------------------*/

    if(argc == 2 && (strcmp(argv[1],"-usage") == 0 || strcmp(argv[1],"-help") == 0))
    {  
       (void)fprintf(stderr,"\nask version %s, (C) Tumbling Dice 2003-2024 (gcc %s: built %s %s)\n\n",ASK_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fprintf(stderr,"ASK is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"ASK comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nUsage: ask [-help | -usage] | [-clear] [payload] [prompt]\n\n");
       (void)fflush(stderr);

       exit(1);
    }



    #ifdef HAVE_READLINE
    /*-------------------------------*/
    /* Get (readline) history if any */
    /*-------------------------------*/

    (void)snprintf(ask_history_pathname,SSIZE,"/tmp/ask.%d.history",getuid());

    if(argc == 2 && strcmp(argv[1],"-clear") == 0)
    {  (void)unlink(ask_history_pathname); 
       prompt_index = 2;
    }

    (void)snprintf(ask_history,SSIZE,ask_history_pathname);
    if(access(ask_history,F_OK | R_OK) != (-1))
    {  (void)history_truncate_file(ask_history,ASK_MAX_HISTORY);
       (void)read_history(ask_history);
    }
    #endif /* HAVE_READLINE */

    if(isatty(0) != 1)
    {  (void)fprintf(stderr,"ask: standard I/O is not associated with a terminal\n");
       (void)fflush(stderr);

       exit(255);
    } 

    if(argc == 3)
       prompt_index = 2;

    out = dup(1);

    (void)close(0);
    (void)open("/dev/tty",0);

    (void)close(1);
    (void)open("/dev/tty",1);

    (void)close(2);
    (void)open("/dev/tty",1);


    /*--------------------------------------------------------------------*/
    /* Start payload command which runs while this application is active. */
    /* This gets arount some problems with signal propagation by shells.  */
    /*--------------------------------------------------------------------*/

    (void)signal(SIGHUP, (void *)&exit_handler);
    (void)signal(SIGQUIT,(void *)&exit_handler);
    (void)signal(SIGINT, (void *)exit_handler);
    (void)signal(SIGTERM,(void *)&exit_handler);

    if(argc == 3)
    {  if((child_pid = fork()) == 0)
       {  char payload_cmd[SSIZE] = "";


          /*----------------------------------*/
          /* Payload will exec with child_pid */
          /*----------------------------------*/

          (void)snprintf(payload_cmd,SSIZE,"exec %s &",argv[1]);
          (void)execlp("sh","sh","-c",payload_cmd,(char *)0);

          _exit(0);
       }
       else
       {  

          /*---------------------------------*/
          /* Wait for child to avoid zombies */
          /*---------------------------------*/

          (void)waitpid(child_pid,&status,0);

          prompt_index = 2;
       }
    }


    /*------------------------------*/
    /* Main loop of the ask command */
    /*------------------------------*/

    out_stream = fdopen(out,"w"); 
    do {     

            char prompt[SSIZE]       = "",
		 history_line[SSIZE] = "";
            
	    #ifdef HAVE_READLINE  
            if(argc == 1 || argv[prompt_index] == (char *)NULL)
            {  (void)strlcpy(value,readline("ask> "),SSIZE);
               if(strempty(value) == FALSE && strexccpy(value,history_line,"\t") > 0)
                  (void)add_history(history_line);
            }
            else
            {

	       /*---------------------------------------*/
	       /* If prompt starts with a '^' character */
	       /* insert newline                        */
               /*---------------------------------------*/

	       if(argv[prompt_index][0] == '^')
               {  uint32_t i,
                           cnt          = 0;

		  char     space[SSIZE] = "";

		  /*-------------------*/
                  /* Whitespace filler */
		  /*-------------------*/

		  for(i=1; i<strlen(argv[prompt_index]); ++i)
	          {  if(argv[prompt_index][i] == ' ')
	             {  space[cnt] = ' ';
		        ++cnt;
	             }
		     else
                     {  space[cnt] = '\0';
		        break;
                     }
                  }

                  (void)snprintf(prompt,SSIZE,"%s\n%s> ",(char *)&argv[prompt_index][1],space);
	       }
	       else
                  (void)snprintf(prompt,SSIZE,"%s> ",argv[prompt_index]);

               (void)strlcpy(value,readline(prompt),SSIZE);
               if(strempty(value) == FALSE && strexccpy(value,history_line,"\t") > 0)
                  (void)add_history(history_line);
            }
            #else
            if(argc == 1)
               (void)fprintf(stderr,"ask> ");
            else
            {   
		    
	       /*---------------------------------------*/
	       /* If prompt starts with a '^' character */
	       /* insert newline                        */
               /*---------------------------------------*/

	       if(argv[prompt_index][0] == '^')
               {  uint32_t i,
		           cnt          = 0;

		  char     space[SSIZE] = "";

		  /*-------------------*/
                  /* Whitespace filler */
		  /*-------------------*/

		  for(i=1; i<strlen(argv[prompt_index]); ++i)
	          {  if(argv[prompt_index][i] == ' ')
	             {  space[cnt] = ' ';
		        ++cnt;
	             }
		     else
                     {  space[cnt] = '\0';
		        break;
                     }
                  }

                  (void)snprintf(prompt,SSIZE,"%s\n%s>",(char *)&argv[prompt_index][1],space);
	       }
	       else
                  (void)snprintf(prompt,SSIZE,"%s> ",argv[prompt_index]);

	       (void)fprintf(stderr,"%s",prompt);
               (void)fflush(stderr);
            }

            (void)fgets(value,256,stdin);
            #endif /* HAVE_READLINE */


            /*---------------------------------------------*/
            /* Value of !ls tells ask to list the contents */
            /* of the current working directory            */
            /*---------------------------------------------*/

            looper  = TRUE;
            if(strin(value,"!") == TRUE)
            {  int32_t  n_spaces              = 0;
               char     ls_output_buffer[256] = "";

               /*-------------------------*/
               /* Skip any leading spaces */
               /*-------------------------*/

               while(value[n_spaces] == ' ')
                    ++n_spaces;

               (void)fprintf(stdout,"\n");
               (void)fflush(stdout);

               (void)system((char *)&value[n_spaces+1]);

               (void)fprintf(stdout,"\n");
               (void)fflush(stdout);
            }


            /*--------------------------------*/
            /* Ask only expects one parameter */
            /*--------------------------------*/

            else
	    {  if((ret = no_space(value)) >= 0)
               {  (void)fprintf(out_stream,"%s\n",(char *)&value[ret]);
                  (void)fflush(out_stream);

                  looper = FALSE;
               }


               /*-------*/
               /* Error */
               /*-------*/

	       else 
               {  (void)fprintf(stderr,"ask: expecting one argument only\n\n");
	          (void)fflush(stderr);
	       }
            }
       } while(looper == TRUE);


    /*-----------------------*/
    /* Save readline history */
    /*-----------------------*/

    #ifdef HAVE_READLINE
    (void)write_history(ask_history);
    #endif /* HAVE_READLINE */

    (void)fclose(out_stream);
    return(0); 
}
