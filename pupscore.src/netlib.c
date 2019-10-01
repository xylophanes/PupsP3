/*------------------------------------------------------------------------------
    Purpose: Distributed computing support library.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Dated:   27th September 2019 
    Version: 5.00 
    E-Mail:  mao@tumblingdice.co.uk
------------------------------------------------------------------------------*/

/*-------------------------------------------------------------*/
/* If we have OpenMP support then we also have pthread support */
/* (OpenMP is built on pthreads for Linux distributions).      */
/*-------------------------------------------------------------*/

#if defined(_OPENMP) && !defined(PTHREAD_SUPPORT)
#define PTHREAD_SUPPORT
#endif /* PTHREAD_SUPPORT */


#include <me.h>
#include <utils.h>
#include <psrp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <netdb.h>
#include <ctype.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sched.h>
#include <dirent.h>

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 
#endif /* _XOPEN_SOURCE */

#include <unistd.h>



#undef   __NOT_LIB_SOURCE__
#include <netlib.h>
#define  __NOT_LIB_SOURCE__


/*-------------------------------------------------*/
/* Slot and usage functions - used by slot manager */
/*-------------------------------------------------*/


/*---------------------*/
/* Slot usage function */
/*---------------------*/

_PRIVATE void netlib_slot(int level)
{   (void)fprintf(stderr,"lib netlib %s: [ANSI C]\n",NETLIB_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(C) 1995-2019 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"PUPS/P3 network support library (built %s %s)\n\n",__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");
    (void)fflush(stderr);
}


/*-------------------------------------------*/
/* Segment identification for netlib library */
/*-------------------------------------------*/

#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT )() __attribute__ ((aligned(16))) = netlib_slot;
#endif /* SLOT */




/*-------------------------------------------*/
/* Public variables exported by this library */
/*-------------------------------------------*/
                                  /*---------------------*/
_PUBLIC int rkill_pid = (-1);     /* Xkill child process */
                                  /*---------------------*/



/*------------------------------------------------------------------------------
    Public variables exported by the extended system command processer ...
------------------------------------------------------------------------------*/

_PUBLIC int pups_system(const char        *command_str,  // Command string
                        const char         *exec_shell,  // Shell used for execing
                        const unsigned int exec_flags,   // Exec control flags
                        int                *child_pid)   // PID of child process

{  int status,
          pid,
          io_pipe,                           // Input output pipe
          flag_cnt = 0,
          tty,                               // Generic control terminal
          fildes[2];                         // Extra I/O pipe

    int *istat        = (int *)NULL,
        *qstat        = (int *)NULL;

    char *shell_flags = (char *)NULL;
    _BOOLEAN obituary = FALSE;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(command_str == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*-------------------------------------*/
    /* Check for conflicting flag options. */
    /*-------------------------------------*/

    if(exec_flags & PUPS_READ_FIFO)
    {  if(exec_flags & PUPS_WAIT_FOR_CHILD)
       ++flag_cnt;
    }

    if(exec_flags & PUPS_WRITE_FIFO)
    {  if(exec_flags & PUPS_WAIT_FOR_CHILD)
       ++flag_cnt;
    }

    if(flag_cnt > 1)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*-------------------------------------*/
    /* Get the name of the extra I/O pipe. */
    /*-------------------------------------*/

    if(exec_flags & PUPS_STREAMS_DETACHED)
    {  tty = open("/dev/tty",2,LIVE);
       if(tty == -1)
       {  pups_set_errno(ENOTTY);
          return(-1);
       }
    }

    if((exec_flags & PUPS_READ_FIFO) || (exec_flags & PUPS_WRITE_FIFO))
    {  if(pipe(fildes)  == -1)
       {  pups_set_errno(ESPIPE);
          return(-1);
       }
    }


    /*---------------------------------------------------------------*/
    /* Cancel automatic cleanup of child if we intend to wait for it */
    /*---------------------------------------------------------------*/

    if(exec_flags & PUPS_NOAUTO_CLEAN || exec_flags & PUPS_WAIT_FOR_CHILD)
       (void)pups_sighandle(SIGCHLD,"default",SIG_DFL, (sigset_t *)NULL);


    if(exec_flags & PUPS_OBITUARY)
       obituary = TRUE;


    /*--------------------*/
    /* Child side of fork */
    /*--------------------*/
       
    if((pid = pups_fork(TRUE,obituary)) == 0)
    {  

       /*--------------------*/
       /* Child side of fork */
       /*--------------------*/

       int i;


       /*-----------------------------------*/
       /* Set effective and real user i.d's */
       /*-----------------------------------*/

       (void)setreuid(getuid(),getuid());


       /*-------------------------*/
       /* Clear any pending alarm */
       /*-------------------------*/

       (void)pups_malarm(0);


       /*---------------------------------------*/
       /* Restore all signals to default states */ 
       /*---------------------------------------*/

       for(i=1; i< MAX_SIGS; ++i)
          (void)pups_sighandle(i,"default",SIG_DFL, (sigset_t *)NULL);
 

       /*-----------------------------------------------------------------*/
       /* Connect stdin and stdout to console if detached mode. Otherwise */
       /* retain stdin for piping to new command.                         */
       /*-----------------------------------------------------------------*/

       if(exec_flags & PUPS_STREAMS_DETACHED)
       {  (void)dup2(tty,0);
          (void)dup2(tty,1);
          (void)dup2(tty,2);
          close(tty);
       }


       /*----------------------------------------------------------------*/
       /* Make stdin and stdout an implicit channel connecting the child */
       /* process to its parent.                                         */
       /*----------------------------------------------------------------*/


       /*-------------------------------------------*/
       /* Create an input pipe to the child process */
       /*-------------------------------------------*/

       if(exec_flags & PUPS_WRITE_FIFO)
       {  (void)dup2 (fildes[0],0);
          (void)close(fildes[0]);
          (void)close(fildes[1]);
       }


       /*----------------------------------------------*/
       /* Create an output pipe from the child process */
       /*----------------------------------------------*/

       if(exec_flags & PUPS_READ_FIFO)
       {  (void)dup2 (fildes[1],1);
          (void)close(fildes[0]);
          (void)close(fildes[1]);
       }


       /*------------------------------------------------*/
       /* Detach control terminal and run in new session */
       /*------------------------------------------------*/

       if(exec_flags & PUPS_NEW_SESSION)
          (void)setsid();


       /*-----------------------------------------------------------------*/
       /* Build command to be executed - use a shell if one is requested  */
       /* otherwise directly overlay command on child.                    */
       /*-----------------------------------------------------------------*/


       /*---------------------------------------*/
       /* Build shell flag string if approriate */
       /*---------------------------------------*/

       if(exec_shell != (const char *)NULL)
       {  shell_flags = pups_malloc(SSIZE);
          (void)strlcpy(shell_flags,"-c",SSIZE);

          if(exec_flags & PUPS_ERROR_EXIT)
             strlcat(shell_flags,"e",SSIZE);

          /*---------------------------------------*/
          /* Return the exit status of the command */
          /*---------------------------------------*/

          if(execlp(exec_shell,
                    exec_shell,
                    shell_flags,
                    command_str,(char *)0) == -1)
             _exit(-1);
       }
       else
       {

          /*---------------------*/
          /* Run payload command */
          /*---------------------*/

          (void)pups_execls(command_str);
          _exit(-1);
       }
    }


    /*---------------------*/
    /* Parent side of fork */ 
    /*---------------------*/

    else if(pid == (-1))
    {  pups_set_errno(ENOEXEC);
       return(-1);
    }
    else
       appl_last_child = pid;


    /*---------------------------*/
    /* Set name of child process */
    /*---------------------------*/

    (void)pups_set_child_name(pid,command_str);


    /*------------------------------*/
    /* Return the PID of this child */
    /*------------------------------*/

    if(child_pid != (int *)NULL)
       *child_pid = pid;

    if(exec_flags & PUPS_STREAMS_DETACHED)
       (void)pups_close(tty);


    /*----------------------------------------------------------*/
    /* Set up io_pipe to read or read and write data from child */
    /*----------------------------------------------------------*/

    if((exec_flags & PUPS_READ_FIFO))
    {  io_pipe = fildes[0];
       (void)close(fildes[1]);
    }


    /*---------------------------------------*/
    /* Set up io_pipe to write data to child */
    /*---------------------------------------*/

    if(exec_flags & PUPS_WRITE_FIFO)
    {  io_pipe = fildes[1];
       (void)close(fildes[0]);
    }


    /*-----------------------------------*/
    /* Wait for child process to return. */
    /*------------------------------------*/

    if(exec_flags & PUPS_WAIT_FOR_CHILD)
    {  int ret = 0;


       /*------------------------------------------------------------------*/
       /* If the appropriate flag is set wait for the command to complete, */
       /* otherwise carry on while child is executing.                     */
       /*------------------------------------------------------------------*/

       istat =  (int *)(signal(SIGINT, SIG_IGN));
       qstat =  (int *)(signal(SIGQUIT,SIG_IGN));

       while((ret = waitpid(pid,&status,WNOHANG)) != pid)
       {    if(ret == (-1))
            {  pups_set_errno(ENOEXEC);
               return(-1);
            }

            (void)pups_usleep(100);
       }


       /*------------------------*/
       /* Restore signal status. */
       /*------------------------*/

       (void)signal(SIGINT, (void *)istat);
       (void)signal(SIGQUIT,(void *)qstat);


       /*----------------------------------------*/
       /* Return exit status of executed command */
       /*----------------------------------------*/

       pups_set_errno(OK);
       return(WEXITSTATUS(status));
    }

    pups_set_errno(OK);


    /*-------------------------------------------*/
    /* Are we returning a descriptor to command? */
    /*-------------------------------------------*/

    if((exec_flags & PUPS_READ_FIFO )  ||
       (exec_flags & PUPS_WRITE_FIFO)   )
       return(io_pipe);

    return(0);
}





/*------------------------------------------------------------------------------
    Open a descriptor to a command pipeline ...
------------------------------------------------------------------------------*/

_PUBLIC int pups_copen(const char *command_pipeline, const char *shell, const unsigned int mode)

{   int f_index,
        fildes,
        fifo_mode;

    char *fname = (char *)NULL;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(command_pipeline == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*---------------------------*/
    /* Set up FIFO mode switches */
    /*---------------------------*/

    if(mode == 0)
       fifo_mode = PUPS_READ_FIFO;
    else
    {  if(mode == 1)
          fifo_mode = PUPS_WRITE_FIFO;
       else
       {  pups_set_errno(EINVAL);
          return(-1);
       }
    }


    /*------------------------------------------------*/
    /* Block signals while command pipeline is opened */ 
    /*------------------------------------------------*/

    (void)pupshold(ALL_PUPS_SIGS);


    /*-------------------------*/
    /*  Update PUPS file table */
    /*-------------------------*/

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&ftab_mutex);
    #endif /* PTHREAD_SUPPORT */

    if((f_index = pups_find_free_ftab_index()) == (-1))
    {
   
       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&ftab_mutex);
       #endif /* PTHREAD_SUPPORT */

       (void)pupsrelse(ALL_PUPS_SIGS);
       pups_set_errno(ENFILE);
       return(-1);
    }


    /*--------------------------*/
    /* Execute command pipeline */
    /*--------------------------*/

    if((fildes = pups_system(command_pipeline,shell,fifo_mode                 |
                             PUPS_NEW_SESSION                                 |
                             PUPS_ERROR_EXIT,&ftab[f_index].fifo_pid)) == (-1))
    {  (void)pupsrelse(ALL_PUPS_SIGS);

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&ftab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(ENOEXEC);
       return(-1);
    }

    if(fifo_mode == PUPS_READ_FIFO)
       (void)snprintf(ftab[f_index].fname,SSIZE,"%s (read)", command_pipeline);
    else
       (void)snprintf(ftab[f_index].fname,SSIZE,"%s (write)",command_pipeline);

    ftab[f_index].fdes   = fildes;
    ftab[f_index].mode   = mode;
    ftab[f_index].stream = (FILE *)NULL;
   
    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&ftab_mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)pupsrelse(ALL_PUPS_SIGS);

    pups_set_errno(OK);
    return(fildes);
}




/*-------------------------------------------------------------------------------
    Close pipe descriptor waiting for associated processes to finish ...
-------------------------------------------------------------------------------*/

_PUBLIC int pups_cclose(const int pdes)

{   return(pups_close(pdes));
}




/*------------------------------------------------------------------------------
    Open a stream to a command pipeline ...
------------------------------------------------------------------------------*/

_PUBLIC FILE *pups_fcopen(const char *command_pipeline, const char *shell, const char *mode)

{   int  i,
         i_mode,
         c_des  = (-1);

    FILE *f_ptr = (FILE *)NULL;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(command_pipeline == (const char *)NULL || mode == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return((FILE *)NULL);
    }


    /*----------------------*/
    /* Set up mode switches */
    /*----------------------*/

    if(strcmp(mode,"r") == 0)
       i_mode = 0;
    else
    {  if(strcmp(mode,"w") == 0)
          i_mode = 1;
       else
       {  if(strcmp(mode,"w+") == 0   ||
             strcmp(mode,"r+") == 0)
             i_mode = 2;    
          else
          {  pups_set_errno(EINVAL);
             return((FILE *)NULL);
          }
       }
    }


    /*-------------------------*/
    /* Open command descriptor */
    /*-------------------------*/

    c_des = pups_copen(command_pipeline,shell,i_mode);
    f_ptr = fdopen(c_des,mode);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&ftab_mutex);
    #endif /* PTHREAD_SUPPORT */


    for(i=0; i<appl_max_files; ++i)
    {  if(ftab[i].fname != (char *)NULL && strcmp(ftab[i].fname,command_pipeline) == 0)
       {  ftab[i].stream = f_ptr;

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&ftab_mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(f_ptr);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&ftab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return(f_ptr);
}




#ifdef ZLIB_SUPPORT 
/*------------------------------------------------------------------------------
    Open a zstream to a command pipeline ...
------------------------------------------------------------------------------*/

_PUBLIC gzFILE *pups_gzcopen(const char *command_pipeline, const char *shell, const char *mode)

{   int  i,
         i_mode,
         c_des = (-1);

    gzFILE *z_ptr = (gzFILE *)NULL;

    if(command_pipeline == (const char *)NULL || mode == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return((gzFILE *)NULL);
    }


    /*----------------------*/
    /* Set up mode switches */
    /*----------------------*/

    if(strcmp(mode,"r") == 0)
       i_mode = 0;
    else
    {  if(strcmp(mode,"w") == 0)
          i_mode = 1;
       else
       {  if(strcmp(mode,"w+") == 0   ||
             strcmp(mode,"r+") == 0)
             i_mode = 2;
          else
          {  pups_set_errno(EINVAL);
             return((gzFILE *)NULL);
          }
       }
    }


    /*-------------------------*/
    /* Open command descriptor */
    /*-------------------------*/

    c_des = pups_copen(command_pipeline,shell,i_mode);
    z_ptr = gzdopen(c_des,mode);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&ftab_mutex);
    #endif /* PTHREAD_SUPPORT */

    for(i=0; i<appl_max_files; ++i)
    {  if(ftab[i].fname != (char *)NULL && strcmp(ftab[i].fname,command_pipeline) == 0)
       {  ftab[i].zstream = z_ptr;

          #ifdef PTHREAD_SUPPORT
          (void)pthread_mutex_unlock(&ftab_mutex);
          #endif /* PTHREAD_SUPPORT */

          pups_set_errno(OK);
          return(z_ptr);
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&ftab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(ESRCH);
    return(z_ptr);
}
#endif /* ZLIB_SUPPORT */




/*-------------------------------------------------------------------------------
    Do not kill pipestream processes explicitly when pipestream is closed ...
-------------------------------------------------------------------------------*/

_PUBLIC int pipestream_kill_disable(const int fdes)

{   int f_index;

    if(fdes < 0 || fdes >= appl_max_files)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&ftab_mutex);
    #endif /* PTHREAD_SUPPORT */

    if((f_index = pups_get_ftab_index(fdes)) == (-1))
    {  

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&ftab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EBADF);
       return(-1);
    }

    if(ftab[f_index].rd_pid > 0)
       ftab[f_index].rd_pid = (-ftab[f_index].rd_pid);
    
    if(ftab[f_index].fifo_pid > 0)
       ftab[f_index].fifo_pid = (-ftab[f_index].fifo_pid);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&ftab_mutex);
    #endif /* PTHREAD_SUPPORT */

    return(0);
}




/*-------------------------------------------------------------------------------
    Kill pipestream processes explicitly when pipestream is closed ...
-------------------------------------------------------------------------------*/

_PUBLIC int pipestream_kill_enable(const int fdes)

{   int f_index;
    
    if(fdes < 0 || fdes >= appl_max_files)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&ftab_mutex);
    #endif /* PTHREAD_SUPPORT */

    if((f_index = pups_get_ftab_index(fdes)) == (-1))
    {  

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_lock(&ftab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(EBADF);
       return(-1);
    }

    if(ftab[f_index].rd_pid < 0)
       ftab[f_index].rd_pid = (-ftab[f_index].rd_pid);
    
    if(ftab[f_index].fifo_pid < 0)
       ftab[f_index].fifo_pid = (-ftab[f_index].fifo_pid);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&ftab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*-------------------------------------------------------------------------------
    Close a pipestream waiting for its processes ...
-------------------------------------------------------------------------------*/

_PUBLIC FILE *pups_fcclose(const FILE *pipe_stream)

{    return(pups_fclose(pipe_stream));
}




#ifdef ZLIB_SUPPORT
/*-------------------------------------------------------------------------------
    Close a pipezstream waiting for its processes ...
-------------------------------------------------------------------------------*/

_PUBLIC gzFILE *pups_gzcclose(const gzFILE *pipe_zstream)

{    return(pups_gzclose(pipe_zstream));
}
#endif /* ZLIB_SUPPORT */





/*------------------------------------------------------------------------------
    Extended system command processer with bi-directional data streams and
    error/status stream ...
------------------------------------------------------------------------------*/

_PUBLIC int pups_system2(const char       *command_str,  // Command string
                         const char        *exec_shell,  // Shell used for execing
                         const unsigned int exec_flags,  // Exec control flags
                         int                *child_pid,  // PID of child process
                         int                   *in_des,  // Pipe to child process
                         int                  *out_des,  // Pipe from child process
                         int                  *err_des)  // Child error pipe

{  int i,
       status,
       tty,                               // Generic control terminal
       cnt               = 0,
       pid,                               // Pid of child process
       in_fildes[2],                      // Pipe for O/P to child process
       out_fildes[2],                     // Pipe for O/P from child process
       err_fildes[2];                     // Pipe child error/status msgs

    _BYTE buf[256]       = "";
    FILE  *output_stream = (FILE *)NULL;  // Stream for copy environment

    int *istat           = (int *)NULL,
        *qstat           = (int *)NULL;

    char *shell_flags    = (char *)NULL,  // Exec shell flags
                 *cwd    = (char *)NULL;  // Current working directory

    _BOOLEAN obituary    = FALSE;

    if(command_str == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*-------------------------------------------*/
    /* Assign the I/O pipes to the child process */
    /*-------------------------------------------*/

    if(in_des != (int *)NULL)
    {  if(pipe(in_fildes)  == (-1))
       {  pups_set_errno(ESPIPE);
          return(-1);
       }
    }

    if(out_des != (int *)NULL)
    {  if(pipe(out_fildes)  == (-1))
       {  pups_set_errno(ESPIPE);
          return(-1);
       }
    }

    if(err_des != (int *)NULL)
    {  if(pipe(err_fildes)  == (-1))
       {  pups_set_errno(ESPIPE);
          return(-1);
       }
    }


    /*---------------------------------------------------------------*/
    /* Cancel automatic cleanup of child if we intend to wait for it */
    /*---------------------------------------------------------------*/

    if(exec_flags & PUPS_NOAUTO_CLEAN || exec_flags & PUPS_WAIT_FOR_CHILD)
       (void)pups_sighandle(SIGCHLD,"default",SIG_DFL, (sigset_t *)NULL);

    if(exec_flags & PUPS_OBITUARY)
       obituary = TRUE;


    /*--------------------*/
    /* Child side of fork */
    /*--------------------*/

    if((pid = pups_fork(TRUE,obituary)) == 0)
    {  

       /*--------------------*/
       /* Child side of fork */
       /*--------------------*/

       int i;


       /*-----------------------------------*/
       /* Set effective and real user i.d's */
       /*-----------------------------------*/

       (void)setreuid(getuid(),getuid());


       /*-----------------------------------------*/
       /* Run child in new session if appropriate */
       /*-----------------------------------------*/

       if(exec_flags & PUPS_NEW_SESSION)
          (void)setsid();


       /*-------------------------*/
       /* Clear any pending alarm */
       /*-------------------------*/

       (void)pups_malarm(0);


       /*---------------------------------------*/ 
       /* Restore all signals to default states */ 
       /*---------------------------------------*/ 

       for(i=1; i<MAX_SIGS; ++i)
          (void)pups_sighandle(i,"default",SIG_DFL, (sigset_t *)NULL);


       /*----------------------------------------------------------------------*/
       /* Reassign the descriptors which the user has requested to be detached */
       /*----------------------------------------------------------------------*/


       /*---------------------------------*/
       /* Reassign child input descriptor */
       /*---------------------------------*/

       if(in_des == (int *)NULL)
       {  if((tty = open("/dev/null",2)) == (-1))
             _exit(-1);

          (void)dup2 (tty,0);
          (void)close(tty);
       }
       else
       {

          /*------------------------------------------------*/
          /* Attach child I/P descriptor to parent via pipe */
          /*------------------------------------------------*/

          (void)dup2 (in_fildes[0],0);
          (void)close(in_fildes[0]);
          (void)close(in_fildes[1]);
       }


       /*----------------------------------*/
       /* Reassign child output descriptor */
       /*----------------------------------*/

       if(out_des == (int *)NULL)
       {  if((tty = open("/dev/null",2)) == (-1))
             _exit(-1);

          (void)dup2 (tty,1);
          (void)close(tty);
       }
       else      
       {

          /*------------------------------------------------*/
          /* Attach child O/P descriptor to parent via pipe */
          /*------------------------------------------------*/

          (void)dup2 (out_fildes[1],1);
          (void)close(out_fildes[0]);
          (void)close(out_fildes[1]);
       }


       /*--------------------------------*/
       /* Detach error/status descriptor */
       /*--------------------------------*/

       if(err_des == (int *)NULL)
       {  if((tty = open("/dev/null",2)) == (-1))
             _exit(-1);

          (void)dup2 (tty,2);
          (void)close(tty);
       }
       else
       {

          /*------------------------------------------------*/
          /* Attach child O/P descriptor to parent via pipe */
          /*------------------------------------------------*/

          (void)dup2 (err_fildes[1],2);
          (void)close(err_fildes[0]);
          (void)close(err_fildes[1]);
       }


       /*----------------------------------------------------------------*/
       /* Build command to be executed - use a shell if one is requested */
       /* otherwise directly overlay command on child                    */
       /*----------------------------------------------------------------*/


       /*---------------------------------------*/
       /* Build shell flag string if approriate */
       /*---------------------------------------*/

       if(exec_shell != (const char *)NULL)
       {  shell_flags = pups_malloc(SSIZE);
          (void)strlcpy(shell_flags,"-c",SSIZE);
          if(exec_flags & PUPS_ERROR_EXIT)
          {  (void)strlcat(shell_flags,"e",SSIZE);


             /*---------------------------------------*/
             /* Return the exit status of the command */
             /*---------------------------------------*/

             if(execlp(exec_shell,
                       exec_shell,
                      shell_flags,
                      command_str,(char *)0) == -1)
                _exit(-1);
          }
       }
       else
       {  (void)pups_execls(command_str);
          _exit(-1);
       }
    }


    /*----------------------*/
    /* Parent side of fork  */
    /*----------------------*/

    else if(pid == (-1))
    {  pups_set_errno(ENOEXEC);
       return(-1);
    }
    else
    {  *child_pid      = pid;
       appl_last_child = pid;
    } 


    /*----------------------------------*/ 
    /* Set name of child we have forked */
    /*----------------------------------*/ 

    (void)pups_set_child_name(pid,command_str);


    /*-------------------------------------*/
    /* Set up pipe to read data from child */
    /*-------------------------------------*/

    if(in_des != (int *)NULL) 
    {  *in_des = in_fildes[1];
       (void)close(in_fildes[0]);
    }


    /*------------------------------------*/
    /* Set up pipe to write data to child */
    /*------------------------------------*/

    if(out_des != (int *)NULL) 
    {  *out_des = out_fildes[0];
       (void)close(out_fildes[1]);
    }


    /*--------------------------------------------------*/
    /* Set up pipe to read status/error data from child */
    /*--------------------------------------------------*/

    if(err_des != (int *)NULL) 
    {  *err_des = err_fildes[0];
       (void)close(err_fildes[1]);
    }


    /*----------------------------------*/
    /* Wait for child process to return */ 
    /*----------------------------------*/

    if(exec_flags & PUPS_WAIT_FOR_CHILD)
    {  int ret = 0;


       /*------------------------------------------------------------------*/
       /* If the appropriate flag is set wait for the command to complete, */
       /* otherwise carry on while child is executing ...                  */
       /*------------------------------------------------------------------*/

       istat = (void *)(signal(SIGINT, SIG_IGN));
       qstat = (void *)(signal(SIGQUIT,SIG_IGN));

       while((ret = waitpid(&pid,&status,WNOHANG)) != pid)
       {    if(ret == (-1))
            {  pups_set_errno(ENOEXEC);
               return(-1);
            }

            (void)pups_usleep(100);
       }


       /*-------------------------------------------------------------------*/
       /* Re-enabable automatic child handling once this child has returned */
       /*-------------------------------------------------------------------*/

       (void)pups_auto_child();


       /*-----------------------*/
       /* Restore signal status */
       /*-----------------------*/

       (void)signal(SIGINT, (void *)istat);
       (void)signal(SIGQUIT,(void *)qstat);


       /*----------------------------------------*/
       /* Return exit status of executed command */
       /*----------------------------------------*/

       pups_set_errno(OK);
       return(WEXITSTATUS(status));
    }

    pups_set_errno(OK);
    return(0);
}





/*------------------------------------------------------------------------------
    Open a descriptor to a command pipeline - this is effectively a
    simplified interface to the pups_system call ...
------------------------------------------------------------------------------*/

_PUBLIC int pups_copen2(const char *command_pipeline,  // Command stream to execute
                        const char            *shell,  // Exec shell
                        int               *child_pid,  // Child PID
                        int                  *in_des,  // Input descriptor
                        int                 *out_des,  // Output descriptor
                        int                 *err_des)  // Error/status descriptor

{   int i,
        f_index[3];


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(command_pipeline == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*------------------------------------------------*/
    /* Block signals while command pipeline is opened */
    /*------------------------------------------------*/

    (void)pupshold(ALL_PUPS_SIGS);

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&ftab_mutex);
    #endif /* PTHREAD_SUPPORT */

    f_index[0] = (-1);
    if(in_des != (int *)NULL)
       f_index[0] = pups_find_free_ftab_index(); 

    f_index[1] = (-1);
    if(out_des != (int *)NULL)
       f_index[1] = pups_find_free_ftab_index(); 

    f_index[2] = (-1);
    if(err_des != (int *)NULL)
       f_index[2] = pups_find_free_ftab_index(); 

    if(f_index[0] == (-1) || f_index[1] == (-1) || f_index[2] == (-1))
    {  

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&ftab_mutex);
       #endif /* PTHREAD_SUPPORT */

       (void)pupsrelse(ALL_PUPS_SIGS); 
       pups_set_errno(ENFILE);
       return(-1);
    }

    if(pups_system2(command_pipeline,
                    shell,
                    PUPS_NEW_SESSION | PUPS_ERROR_EXIT,
                    child_pid,
                    in_des,
                    out_des,
                    err_des) == -1)
    {

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_unlock(&ftab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(ENOEXEC);
       return(-1);
    }


    /*---------------------------------------*/
    /* Bind descriptors to PUPS ftab entries */
    /*---------------------------------------*/

    for(i=0; i<3; ++i)
    {  if(f_index[i] != (-1))
       {  switch(i)
          {    case 0:  (void)strlcpy(ftab[i].fname,command_pipeline,SSIZE); 
                        ftab[f_index[i]].fdes   = *in_des;
                        ftab[f_index[i]].mode   = 0;
                        ftab[f_index[i]].stream = (FILE *)NULL;
                        break;

               case 1:  (void)strlcpy(ftab[i].fname,command_pipeline,SSIZE);
                        ftab[f_index[i]].fdes   = *out_des;
                        ftab[f_index[i]].mode   = 1;
                        ftab[f_index[i]].stream = (FILE *)NULL;
                        break;

               case 2:  (void)strlcpy(ftab[i].fname,command_pipeline,SSIZE);
                        ftab[f_index[i]].fdes   = *err_des;
                        ftab[f_index[i]].mode   = 1;
                        ftab[f_index[i]].stream = (FILE *)NULL;
                        break;

               default: break;
          }
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_unlock(&ftab_mutex);
    #endif /* PTHREAD_SUPPORT */

    (void)pupsrelse(ALL_PUPS_SIGS);
    pups_set_errno(OK);
    return(0);
}





/*------------------------------------------------------------------------------
    Open command pipeline with optional attached input, output and
    error streams ...
------------------------------------------------------------------------------*/

_PUBLIC int pups_fcopen2(const char *command_pipeline,    // Command stream to exec
                         const char            *shell,    // Exec shell
                         int               *child_pid,    // Child PID
                         FILE              *in_stream,    // Input stream
                         FILE             *out_stream,    // Output stream
                         FILE             *err_stream)    // Error/status stream

{   int i,
        in_des,
        out_des,
        err_des,
        f_index[3];


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(command_pipeline == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&ftab_mutex);
    #endif /* PTHREAD_SUPPORT */

    f_index[0] = (-1);
    if(in_stream  != (FILE *)NULL)
       f_index[0] = pups_find_free_ftab_index();

    f_index[1] = (-1);
    if(out_stream != (FILE *)NULL)
       f_index[1] = pups_find_free_ftab_index();

    f_index[2] = (-1);
    if(err_stream != (FILE *)NULL)
       f_index[2] = pups_find_free_ftab_index();

    if(f_index[0] == (-1) || f_index[1] == (-1) || f_index[2] == (-1))
    {

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_lock(&ftab_mutex);
       #endif /* PTHREAD_SUPPORT */

       (void)pupsrelse(ALL_PUPS_SIGS);
       pups_set_errno(ENFILE);
       return(-1);
    }

    if(pups_system2(command_pipeline,
                    shell,
                    PUPS_ERROR_EXIT,
                    child_pid,
                    &in_des,
                    &out_des,
                    &err_des) == (-1))
    {

       #ifdef PTHREAD_SUPPORT
       (void)pthread_mutex_lock(&ftab_mutex);
       #endif /* PTHREAD_SUPPORT */

       pups_set_errno(ENOEXEC);
       return(-1);
    }


    /*-----------------------------------*/
    /* Bind command stream to descriptor */ 
    /*-----------------------------------*/

    for(i=0; i<3; ++i)
    {  if(f_index[i] != (-1))
       {  switch(i)
          {    case 0:  (void)strlcpy(ftab[i].fname,command_pipeline,SSIZE);
                        ftab[f_index[i]].fdes   = in_des;
                        ftab[f_index[i]].mode   = 0;
                        ftab[f_index[i]].stream = pups_fdopen(in_des,"r");
                        in_stream               = ftab[f_index[i]].stream;
                        break;

               case 1:  (void)strlcpy(ftab[i].fname,command_pipeline,SSIZE);
                        ftab[f_index[i]].fdes   = out_des;
                        ftab[f_index[i]].mode   = 1;
                        ftab[f_index[i]].stream = pups_fdopen(out_des,"w");
                        out_stream              = ftab[f_index[i]].stream;
                        break;

               case 2:  (void)strlcpy(ftab[i].fname,command_pipeline,SSIZE);
                        ftab[f_index[i]].fdes   = err_des;
                        ftab[f_index[i]].mode   = 1;
                        ftab[f_index[i]].stream = pups_fdopen(err_des,"w");
                        err_stream              = ftab[f_index[i]].stream;
                        break;

               default: break;
          }
       }
    }

    #ifdef PTHREAD_SUPPORT
    (void)pthread_mutex_lock(&ftab_mutex);
    #endif /* PTHREAD_SUPPORT */

    pups_set_errno(OK);
    return(0);
}




/*------------------------------------------------------------------------------
    Detach process from pipeline ...
------------------------------------------------------------------------------*/

_PUBLIC void detach_from_pipeline(const int close_status)

{   

    /*--------------------------------------------------*/
    /* Parent side of fork - simply exit after creating */
    /* identical child process                          */ 
    /*--------------------------------------------------*/

    if(pups_fork(TRUE,FALSE) != 0) 
    {  int i;


       /*-----------------------------------*/
       /* Set effective and real user i.d's */
       /*-----------------------------------*/

       (void)setreuid(getuid(),getuid());


       /*-------------------------*/
       /* Clear any pending alarm */
       /*-------------------------*/

       (void)pups_malarm(0);


       /*---------------------------------------*/ 
       /* Restore all signals to default states */ 
       /*---------------------------------------*/ 

       for(i=1; i<MAX_SIGS; ++i)
          (void)pups_sighandle(i,"default",SIG_DFL, (sigset_t *)NULL);

       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s) has detached from pipeline\n",date,appl_name,appl_pid,appl_host,appl_owner);
          (void)fflush(stderr);
       }

       _exit(0);
    }


    /*------------------------------------------------*/
    /* If appropriate detach from pipeline process    */
    /* group                                          */
    /*------------------------------------------------*/

    if(close_status & NEW_PGRP)
       (void)setsid();


    /*------------------------------------------------*/
    /* Child side of fork - close stdio as specified  */ 
    /* by caller                                      */
    /*------------------------------------------------*/


    /*-------------*/
    /* Close stdin */
    /*-------------*/

    if(close_status & CLOSE_STDIN)
      (void)close(0);


    /*--------------*/
    /* Close stdout */
    /*--------------*/

    if(close_status & CLOSE_STDOUT)
      (void)close(1);


    /*--------------*/
    /* Close stderr */
    /*--------------*/

    if(close_status & CLOSE_STDERR)
      (void)close(2);
}




/*--------------------------------------------------------------------------------------
    Send signal to process running on remote host ...
--------------------------------------------------------------------------------------*/

_PUBLIC int pups_rkill(const char *hostname, const char *ssh_port, const char *username, const char *pidname, const unsigned int signum)

{   _BOOLEAN looper = TRUE;

    int trys,
        pid;

    if(pidname == (const char *)NULL || signum > MAX_SIGS)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*-------------------------------------------*/
    /* Convert pid(name) to numeric process i.d. */
    /* if we are signalling a local process      */
    /*-------------------------------------------*/

    if(hostname == (char *)NULL)
    {  if((pid = psrp_pname_to_pid(pidname)) == (-1))
          return(-1);
    }


    /*-----------------------------------------------*/   
    /* Send signal to process running on remote host */
    /*-----------------------------------------------*/   

    else if(hostname != (char *)NULL           &&
            strcmp("localhost",hostname) != 0  &&
            strcmp(appl_host,hostname)   != 0   )
    {

#ifndef SSH_SUPPORT
       (void)pups_set_errno(EINVAL);
       return(-1);
#else
       int nb,
           status = 0,
           ret    = 0;

       char reply[512]              = "",
            xkilld_parameters[512]  = "";


       /*---------------------*/
       /* Remote kill command */
       /*---------------------*/

       (void)snprintf(xkilld_parameters,SSIZE,"nkill %d +verbose %s",signum,pidname);

       #ifdef NETLIB_DEBUG
       (void)fprintf(stderr,"XKILL %s\n",xkilld_parameters);
       (void)fflush(stderr);
       #endif /* NETLIB_DEBUG */

       if((rkill_pid = pups_fork(FALSE,FALSE)) == 0)
       {   char sshPortOpt[SSIZE] = "";

          /*--------------------*/
          /* Child side of fork */
          /*--------------------*/

          int      ret      = 0;
          _BOOLEAN is_a_tty = TRUE;


          /*-----------------------------------------------------------------------*/
          /* Stop homeostats and remove PSRP channels (this process is about to be */
          /* overlayed by ssh)                                                     */
          /*-----------------------------------------------------------------------*/

          (void)pups_malarm(0);
          (void)signal(SIGALRM,SIG_IGN);
 
          appl_verbose = FALSE;

          (void)pups_closeall();
          (void)setsid();


          /*---------------------*/
          /* We don't need stdio */
          /*---------------------*/

          (void)fclose(stdin);
          (void)fclose(stdout);
          (void)fclose(stderr);


          if(ssh_port != (char *)NULL)
             (void)snprintf(sshPortOpt,SSIZE,"-P %s",ssh_port);
          else if(strcmp(ssh_remote_port,"") == 0)
             (void)snprintf(sshPortOpt,SSIZE,"-P %s",ssh_remote_port);
          

          /*--------------------------------------*/
          /* We are not using passwords. You will */
          /* need to generate a public/private    */
          /* keyset for this to work              */
          /*--------------------------------------*/

          if(ssh_compression == TRUE)
             (void)execlp("ssh","ssh",sshPortOpt,"-oPasswordAuthentication=no","+C","-l",username,hostname,xkilld_parameters,(char *)NULL);
          else
             (void)execlp("ssh","ssh",sshPortOpt,"-oPasswordAuthentication=no","-l",username,hostname,xkilld_parameters,(char *)NULL);


          /*---------------------------------------------------------*/
          /* We should not get here -- if we do an error has occured */
          /*---------------------------------------------------------*/

          _exit(-1);
       }


       /*---------------------*/
       /* Parent side of fork */
       /*---------------------*/

       else if(rkill_pid == (-1))
       {  pups_set_errno(ENOEXEC);
          return(-1);
       }


       /*-----------------------------*/ 
       /* Wait for child to terminate */
       /* and read reply (if any)     */
       /*-----------------------------*/ 

       while((ret = waitpid(rkill_pid,&status,WNOHANG)) != pid)
       {    if(ret == (-1))
            {  pups_set_errno(ENOEXEC);
               return(-1);
            }

            (void)pups_usleep(100);
       }

       if(WEXITSTATUS(status) == 255)
       {  

          /*-------------------------------------*/
          /* ssh failed to execute nkill command */
          /*-------------------------------------*/

          pups_set_errno(EACCES);
          return(-1);
       }
       else if(WEXITSTATUS(status) < 0)
       {

          /*---------------------------------*/
          /* Could not signal remote process */
          /*---------------------------------*/

          pups_set_errno(ENOENT);
          return(-1);
       }


       /*----------------------------------*/
       /* We sent signal to remote process */
       /*----------------------------------*/

       (void)pups_set_errno(OK);
       return(ret);
    }
#endif /* SSH_SUPPORT */

    else
    {  

       /*-------------------*/
       /* Send local signal */
       /*-------------------*/

       if(kill(pid,signum) == (-1))
       {  pups_set_errno(EACCES);
          return(-1);
       }

       pups_set_errno(OK);
       return(0);
    }
}
