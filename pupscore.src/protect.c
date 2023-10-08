/*-------------------------------------------------------------------------------------------------------
    Purpose: Multi file homeostat 

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.00 
    Dated:   26th January 2022
    E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------------------------------------------*/


#include <me.h>
#include <utils.h>
#include <netlib.h>
#include <psrp.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>
#include <vstamp.h>
#include <time.h>


/*--------------------*/
/* Version of protect */
/*--------------------*/

#define PROTECT_VERSION    "2.00"

#ifdef BUBBLE_MEMORY_SUPPORT
#include <bubble.h>
#endif /* BUBBLE_MEMORY_SUPPORT */


/*----------------------------------------------------------------*/
/* Get signal mapping appropriate to OS and hardware architecture */
/*----------------------------------------------------------------*/

#define __DEFINE__
#if defined(I386) || defined(X86_64)
#include <sig.linux.x86.h>
#endif /* I386 || X86_64 */

#ifdef ARMV6L
#include <sig.linux.arm.h>
#endif /* ARMV6L */

#ifdef ARMV7L
#include <sig.linux.arm.h>
#endif /* ARMV7L */

#ifdef AARCH64
#include <sig.linux.arm.h>
#endif /* AARCH64 */
#undef __DEFINE__


#define DELAY 100000



/*------------------------------------------------------------------------------------------------------
    Get application information for slot manager ...
------------------------------------------------------------------------------------------------------*/



/*---------------------------*/ 
/* Slot information function */
/*---------------------------*/ 

_PRIVATE void protect_slot(int level)
{   (void)fprintf(stderr,"int [PUPS/PSRP] application protect %s: [ANSI C, PUPS/PSRP]\n",PROTECT_VERSION);
 
    if(level > 1)
    {  (void)fprintf(stderr,"(C) 2005-2022 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"File and directory protector (built %s %s)\n\n",__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");

    (void)fflush(stderr);
}
 
 

 
/*----------------------------*/ 
/* Application usage function */
/*----------------------------*/ 

_PRIVATE void protect_usage()

{   (void)fprintf(stderr,"!-principal <file/directory to protect>!\n");
    (void)fprintf(stderr,"[-defer:FALSE]\n");
    (void)fprintf(stderr,"[-lifetime <lifetime in minutes>]\n");
    (void)fprintf(stderr,"[-key <key (for files in directory):all>]\n\n"); 
    (void)fprintf(stderr,"[>& <ASCII log file>]\n\n");

    (void)fprintf(stderr,"Signals\n\n");
    (void)fprintf(stderr,"SIGINIT SIGCHAN SIGPSRP: Process status [PSRP] request (protocol %4.2F)\n",PSRP_PROTOCOL_VERSION);
    (void)fprintf(stderr,"SIGCLIENT: tell client server is about to segment\n");

    #ifdef CRUI_SUPPORT
    (void)fprintf(stderr,"SIGCHECK SIGRESTART:      checkpoint and restart signals\n");
    #endif /* CRIU_SUPPORT */

    (void)fprintf(stderr,"SIGALIVE: check for existence of client on signal dispatch host\n");
    (void)fflush(stderr);
}


#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT)() __attribute__ ((aligned(16))) = protect_slot;
_EXTERN void (* USE )() __attribute__ ((aligned(16))) = protect_usage;
#endif /* SLOT */



/*-------------------------------------------------------------------------------------------------------
    Application build date ...
-------------------------------------------------------------------------------------------------------*/

_EXTERN char appl_build_time[SSIZE] = __TIME__;
_EXTERN char appl_build_date[SSIZE] = __DATE__;




/*-------------------------------------------------------------------------------------------------------
    Defines which are private to this application ...
-------------------------------------------------------------------------------------------------------*/

#define FOREVER               1
#define DEFAULT_LIFETIME      600 


/*-------------------------------------------------------------------------------------------------------
    Functions which are private to this application ...
-------------------------------------------------------------------------------------------------------*/

// Exit function
_PROTOTYPE _PRIVATE int exit_func(char *arg);

// Set/display principal (protected object)
_PROTOTYPE _PRIVATE int set_principal(int, char *[]);

// Set/display protection lifetime (in seconds)
_PRIVATE int set_lifetime(int, char *[]);

// Set/display file protection key
_PROTOTYPE _PRIVATE int set_key(int, char *[]);

// Protect a file (in principal directory)
_PROTOTYPE _PRIVATE int myprotect(int, char *[]);

// Unprotect a file (in principal directory)
_PROTOTYPE _PRIVATE int myunprotect(int, char *[]);

// Show list of protected file (in principal directory)
_PROTOTYPE _PRIVATE int show_protected_files(int, char *[]);

// PSRP process status display dispatch function
_PROTOTYPE _PRIVATE int psrp_process_status(int, char *[]);

// Scan directory (for new files to protect)
_PROTOTYPE _PRIVATE void scan_directory(char *);



/*-------------------------------------------------------------------------------------------------------
    Variables which are private to this module ...
-------------------------------------------------------------------------------------------------------*/
                                                     /*------------------------------------------------*/
_PRIVATE _BOOLEAN is_directory      = FALSE;         /* TRUE if we are protecting a directory of files */
_PRIVATE _BOOLEAN do_defer          = FALSE;         /* TRUE if we have deferred protection enabled    */
_PRIVATE char file_name[SSIZE]      = "";            /* Name of file/directory to be protected         */
_PRIVATE char key[SSIZE]            = "all";         /* Key (for matching directory files)             */
_PRIVATE char prot_pathname[SSIZE]  = "";            /* Protection flag (for protected directory)      */
_PRIVATE char pathname[SSIZE]       = "";            /* Pathname                                       */
_PRIVATE struct stat stat_buf;                       /* File (inode) statistics buffer                 */
_PRIVATE int  n_files               = 0;             /* Number of protected files (in directory)       */
_PRIVATE int fdes                   = 0;             /* Number of times principal attacked             */
_PRIVATE time_t lifetime            = FOREVER;       /* Protection lifetime                            */
                                                     /*------------------------------------------------*/

#ifdef HYDRA_OF_LERNA
                                                     /*------------------------------------------------*/
_PRIVATE int child_pid;                              /* PID of child bud process                       */
                                                     /*------------------------------------------------*/
#endif /* HYDRA_OF_LERNA */



/*-------------------------------------------------------------------------------------------------------
    Software I.D. tag (used if CKPT support enabled to discard stale dynamic
    checkpoint files) ...
-------------------------------------------------------------------------------------------------------*/

#define VTAG  3746
extern int appl_vtag = VTAG;





/*-------------------------------------------------------------------------------------------------------
    Main - decode command tail then interpolate using parameters supplied by user ...
-------------------------------------------------------------------------------------------------------*/

_PUBLIC int pups_main(int argc, char *argv[])

{   DIR           *dirp       = (DIR *)NULL;
    struct dirent *next_entry = (struct dirent *)NULL;

    time_t tdum,
           start_time;


    _IMMORTAL _BOOLEAN entered = FALSE;

    /*------------------------------------------------------------------------------*/
    /* Ignore any clients who may be trying to attach to us until we are intialised */
    /*------------------------------------------------------------------------------*/

    psrp_ignore_requests();


    /*---------------------------------------------------------------------*/
    /* Use a hydra (of lerna) algorithm to protect our thread of execution */
    /*---------------------------------------------------------------------*/
    #ifdef HYDRA_OF_LERNA

    /*-----------------------------------------------------------------*/
    /* Make sure that we run in our own session (so parent pups_exit() */
    /* will not cause child to exit                                    */
    /*-----------------------------------------------------------------*/

    (void)setsid();

refork:

    if((child_pid = pups_fork(FALSE,FALSE)) == 0)
    {

       /*---------------------------------------------------*/ 
       /* We are a child -- wait for our parent to die      */
       /* before activating ourself and protecting our file */
       /*---------------------------------------------------*/ 

       while(getppid() != 1)
            (void)pups_usleep(DELAY);

       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): parent terminated [central head crushed -- regrowing] (reforking)\n",
                                                                          date,appl_name,appl_pid,appl_host,appl_owner);
          (void)fflush(stderr);

       }


       /*----------------------------------------*/
       /* Our parent is gone - take over from it */
       /*----------------------------------------*/

       entered = FALSE;
       goto refork;
    }

    start_time = time(&tdum);
    if(entered != TRUE)
    {  while(TRUE)
       {   if(lifetime != FOREVER)
           {  if(time(&tdum) - start_time > lifetime)
              { if(appl_verbose == TRUE)
                {  (void)strdate(date);
                   (void)fprintf(stderr,"%s %s (%d@%s:%s): protection lifetime expired -- exiting\n",
                                                        date,appl_name,appl_pid,appl_host,appl_owner);
                   (void)fflush(stderr);

                   pups_exit(0);
                }
              }
           }


           /*-----------------------------------------------------------------------*/
           /* Check to see if we have any new entries in directory -- if they match */
           /* key we will have to protect them                                      */
           /*-----------------------------------------------------------------------*/


           /*-----------------------------------------------------------------------*/
           /* Check to see if we have any new entries in directory -- if they match */
           /* key we will have to protect them                                      */
           /*-----------------------------------------------------------------------*/

           if(is_directory == TRUE)
              scan_directory(file_name); 


           /*---------------------------------------------------------------------------*/
           /* Check to see if child has been terminated -- if it has create a new child */
           /* to take its place                                                         */
           /*---------------------------------------------------------------------------*/

           if(kill(child_pid,SIGALIVE) == (-1))
           {  if(appl_verbose == TRUE)
              {  (void)strdate(date);
                 (void)fprintf(stderr,"%s %s (%d@%s:%s): child terminated [head crushed -- regrowing] (reforking)\n",
                                                                        date,appl_name,appl_pid,appl_host,appl_owner);
                 (void)fflush(stderr);

              }

              goto refork;
           }

           (void)pups_usleep(DELAY);
       }
    }
    else
       entered = TRUE;
    #endif  /* HYDRA_OF_LERNA */


/*------------------------------------------------------------------------------------------------------
    Get standard items form the command tail ...
------------------------------------------------------------------------------------------------------*/

    pups_std_init(TRUE,
                  &argc,
                  PROTECT_VERSION,
                  "M.A. O'Neill",
                  "protect",
                  "2022",
                  argv);


    /*-----------------------------------------------------*/
    /* Application specific arguments should go here       */
    /* embryo.c has some examples of how it should be done */
    /*-----------------------------------------------------*/

    if((ptr = pups_locate(&init,"principal",&argc,args,0)) !=  NOT_FOUND)
    {  if(strccpy(file_name,pups_str_dec(&ptr,&argc,args)) == (char *)NULL)
          pups_error("protect: expecting name of file/directory to protect");
    }
    else
       pups_error("protect: expecting name of file/directory to protect");

    if(pups_locate(&init,"defer",&argc,args,0) !=  NOT_FOUND) 
       do_defer = TRUE;

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): deferred protection enabled\n",
                                 date,appl_name,appl_pid,appl_host,appl_owner);
       (void)fflush(stderr);
    }

    if((ptr = pups_locate(&init,"lifetime",&argc,args,0)) !=  NOT_FOUND) 
    {  if((lifetime = pups_i_dec(&ptr,&argc,args)) == (int)INVALID_ARG)
          pups_error("[protect] expecting lifetime (in seconds)");

       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): principal will be protected for %d seconds\n",
                                          date,appl_name,appl_pid,appl_host,appl_owner,lifetime);
          (void)fflush(stderr);
       }
    }

    if((ptr = pups_locate(&init,"key",&argc,args,0)) !=  NOT_FOUND)
    {  if(strccpy(key,pups_str_dec(&ptr,&argc,args)) == (char *)NULL)
          pups_error("[protect] expecting key (for directory-file protection)");
    }

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): key (for directory-file protection) is \"%s\"\n",
                                               date,appl_name,appl_pid,appl_host,appl_owner,key);
       (void)fflush(stderr);
    }


    /*---------------------------------------*/
    /* Complain about any unparsed arguments */
    /*---------------------------------------*/

    (void)pups_t_arg_errs(argd,args);


    /*---------------------------------------------------------*/
    /* Initialise the PSRP system here using psrp_init()       */
    /* Note any PSRP dyanmic load option you do not want       */
    /* is simply excluded from the first set of OR'd args      */
    /* If you don't want homeostasis protection for the        */
    /* server messaging channels omit PSRP_HOMEOSTATIC_STREAMS */
    /*---------------------------------------------------------*/

    psrp_init(PSRP_STATIC_FUNCTION | PSRP_STATIC_DATABAG | PSRP_HOMEOSTATIC_STREAMS,NULL);


    /*-------------------------------------------------------------*/
    /* Load default dispatch table for this applications (loads    */
    /* dynamic objects which have been previously attached to this */
    /* application and remembers any aliases we may have had for   */
    /* dispatch table objects                                      */
    /*-------------------------------------------------------------*/

    (void)psrp_attach_static_function("protect",  (void *)&myprotect);
    (void)psrp_attach_static_function("unprotect",(void *)&myunprotect);
    (void)psrp_attach_static_function("protstat", (void *)&show_protected_files);
    (void)psrp_attach_static_function("key",      (void *)&set_key);
    (void)psrp_attach_static_function("lifetime", (void *)&set_lifetime);
    (void)psrp_attach_static_function("status",   (void *)&psrp_process_status);
    (void)psrp_attach_static_function("principal",(void *)&set_principal);
    (void)psrp_load_default_dispatch_table();


    /*--------------------------------------------------------------------------*/
    /* Once we have initialised the PSRP system, attached any dynamic objects   */
    /* and loaded any alias tables lets tell the world we are open for business */
    /*--------------------------------------------------------------------------*/

    psrp_accept_requests();


    /*----------------------------------------------------------------------------------------*/
    /* Any payload functions which attach or create objects like files and persistent heaps   */
    /* should register an exit function here, so that any temporary objects are destroyed     */
    /* by pups_exit()                                                                         */
    /*----------------------------------------------------------------------------------------*/

    (void)pups_register_exit_f("exitfunc",(void *)&exit_func,(char *)NULL);


    /*-----------------------------*/
    /* Payload of application here */
    /*-----------------------------*/


    /*------------------------*/
    /* Do we have a directory */
    /*------------------------*/

    if((dirp = opendir(file_name)) != (DIR *)NULL)
    {  char hname[SSIZE]    = "",
            pathname[SSIZE] = "";

       if(do_defer == TRUE)
          pups_error("[protect] cannot have deferred protection for directories");


       /*------------------------------------------------------------------------*/
       /* Do we have a protect process already protecting this directory? If so  */
       /* it will create <file_name>.prot in the same branch of the file tree as */
       /* the directory entry.                                                   */
       /*------------------------------------------------------------------------*/

       (void)snprintf(prot_pathname,SSIZE,"%s.prot",file_name);

       if(access(prot_pathname,F_OK | R_OK | W_OK) != (-1))
       {  FILE *stream = (FILE *)NULL;
          int  prot_pid;
          char prot_host[SSIZE] = "";

          stream = fopen(prot_pathname,"r"); 
          (void)fscanf(stream,"%d",&prot_pid);

          if(kill(SIGALIVE,prot_pid) != (-1))
          {  if(appl_verbose == TRUE)
             {  (void)strdate(date);
                (void)fprintf(stderr,"%s %s (%d@%s:%s): directory \"%s\" is already protected\n",
                                          date,appl_name,appl_pid,appl_host,appl_owner,file_name);
                (void)fflush(stderr);
             }
          }

          pups_exit(255);
       }
       else
       {  FILE *stream = (FILE *)NULL;


          /*------------------------------*/
          /* Create a protection flag file*/ 
          /*------------------------------*/

          (void)pups_creat(prot_pathname,0600);
          stream = pups_fopen(prot_pathname,"w",LIVE);

          (void)fprintf(stream,"%d\n",appl_pid);
          (void)fflush(stream);
       }


       /*--------------------------------------------------------------*/  
       /* Yes we do - set up protection for each file in the directory */
       /*--------------------------------------------------------------*/  

       is_directory = TRUE;

       while((next_entry = readdir(dirp)) != (struct dirent *)NULL)
       {

           /*-----------------------------------------------------------------*/
           /* If any of the files in the directory are themselves a directory */
           /* they cannot be protected.                                       */
           /*-----------------------------------------------------------------*/

           (void)snprintf(pathname,SSIZE,"%s/%s",file_name,next_entry->d_name);
           (void)stat(pathname,&stat_buf);

           if((S_ISREG(stat_buf.st_mode) || S_ISFIFO(stat_buf.st_mode))        &&
              strcmp(next_entry->d_name,".")    != 0                           &&
              strcmp(next_entry->d_name,"..")   != 0                           &&
              strin(next_entry->d_name,"uprot") == FALSE                       &&
              (strin(next_entry->d_name,key) == TRUE  || strcmp(key,"all") == 0))
           {   (void)snprintf(pathname,SSIZE,"%s/%s",file_name,next_entry->d_name);

               #ifndef USE_PROTECT
               fdes = pups_open(pathname,0,LIVE);
               (void)pups_creator(fdes);
               #endif /* USE_PROTECT */

               (void)snprintf(hname,SSIZE,"default_fd_homeostat: %s",pathname);

               #ifdef USE_PROTECT
               fdes = pups_protect(pathname,hname,(void *)&pups_default_fd_homeostat);
               #else
               (void)pups_fd_alive(fdes,hname,(void *)&pups_default_fd_homeostat);
               #endif /* USE_PROTECT */

               ++n_files;

              if(appl_verbose == TRUE)
              {  (void)strdate(date);
                 (void)fprintf(stderr,"%s %s (%d@%s:%s): protecting file \"%s\" (in directory \"%s\")\n",
                               date,appl_name,appl_pid,appl_host,appl_owner,next_entry->d_name,file_name);
                 (void)fflush(stderr);
              }
           }
        }

        (void)closedir(dirp);
    }
    else
    {  char hname[SSIZE] = "";


       /*------------------------------------------------------*/
       /* Wait for file which is to be protected to be created */
       /*------------------------------------------------------*/

       if(access(file_name,F_OK | R_OK | W_OK) == (-1))
       {  if(do_defer == FALSE)
              pups_exit(255);
          else
          {  if(appl_verbose == TRUE)
             {  (void)strdate(date);
                (void)fprintf(stderr,"%s %s (%d@%s:%s): waiting for file \"%s\"\n",
                            date,appl_name,appl_pid,appl_host,appl_owner,file_name);
                (void)fflush(stderr);
             }

             while(access(file_name,F_OK | R_OK | W_OK) == (-1))
                   pups_usleep(DELAY);

             if(appl_verbose == TRUE)
             {  (void)strdate(date);
                (void)fprintf(stderr,"%s %s (%d@%s:%s): file \"%s\" now created\n",
                            date,appl_name,appl_pid,appl_host,appl_owner,file_name);
                (void)fflush(stderr);
             }
          }
       }

       fdes = pups_open(file_name,0,LIVE);
       (void)pups_creator(fdes);
       (void)snprintf(hname,SSIZE,"default_fd_homeostat: %s",file_name);
       (void)pups_fd_alive(fdes,hname,&pups_default_fd_homeostat);

       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): protecting file \"%s\"\n",
                     date,appl_name,appl_pid,appl_host,appl_owner,file_name);
          (void)fflush(stderr);
       }
    }


    /*------------------------------------------------------------------------*/
    /* simply wait until protected files are tampered with (until terminated) */
    /*------------------------------------------------------------------------*/

    start_time = time(&tdum);
    while(TRUE)
    {   if(lifetime != FOREVER)
        {  if(time(&tdum) - start_time > lifetime)
           {  if(appl_verbose == TRUE)
              {  (void)strdate(date);
                 (void)fprintf(stderr,"%s %s (%d@%s:%s): protection lifetime expired -- exiting\n",
                                                      date,appl_name,appl_pid,appl_host,appl_owner);
                 (void)fflush(stderr);
              }

              pups_exit(0);
           }
        }


        /*-----------------------------------------------------------------------*/
        /* Check to see if we have any new entries in directory -- if they match */
        /* key we will have to protect them                                      */
        /*-----------------------------------------------------------------------*/

        if(is_directory == TRUE)
           scan_directory(file_name);      


        #ifdef HYDRA_OF_LERNA
        /*---------------------------------------------------------------------------*/
        /* Check to see if child has been terminated -- if it has create a new child */
        /* to take its place                                                         */
        /*---------------------------------------------------------------------------*/

        if(kill(child_pid,SIGALIVE) == (-1))
        {  if(appl_verbose == TRUE)
           {  (void)strdate(date);
              (void)fprintf(stderr,"%s %s (%d@%s:%s): child terminated [head crushed -- regrowing] (reforking)\n",
                                                                     date,appl_name,appl_pid,appl_host,appl_owner);
              (void)fflush(stderr);
           }

           goto refork;
        }
        #endif /* HYDRA_OF_LERNA */

        (void)pups_usleep(DELAY);
    }


    /*-----------------------------------------------*/
    /* Exit nicely clearing up and mess we have made */
    /*-----------------------------------------------*/

    pups_exit(0);
}




/*----------------------------------------------------------------------------------------------------
    Display process status (to attached PSRP client process) ...
----------------------------------------------------------------------------------------------------*/

_PRIVATE int psrp_process_status(int argc, char *argv[])

{
   (void)fprintf(psrp_out,"\n    Protect (version %s)\n",PROTECT_VERSION);
   (void)fprintf(psrp_out,"    ======================\n\n");
   (void)fflush(psrp_out);

#if defined(CRIIU_SUPPORT)
    (void)fprintf(psrp_out,"    Binary is Criu enabled (checkpointable)\n");
#endif  /* CRIU_SUPPORT */

   if(is_directory == TRUE)
   {  (void)fprintf(psrp_out,"    Protecting directory %s (%d files)\n",file_name,n_files);
      (void)fprintf(psrp_out,"    \"protstat\" PSRP option shows list of protected filenames\n\n");
   }
   else
   {  int lost_cnt;

      lost_cnt = pups_lost(fdes);

      if(lost_cnt > 0)
      {  if(lost_cnt == 1)
            (void)fprintf(psrp_out,"    Protecting regular file %s (attacked once)\n\n",file_name);
         else
            (void)fprintf(psrp_out,"    Protecting regular file %s (attacked %d times)\n\n",file_name,lost_cnt);
      }
      else
         (void)fprintf(psrp_out,"    Protecting regular file %s\n\n",file_name);
   }

   (void)fflush(psrp_out);   

   #ifdef HYDRA_OF_LERNA
   (void)fprintf(psrp_out,"    Thread of execution protected by (bi-process) hydra-of-lerna algorithm\n\n");
   (void)fflush(psrp_out);
   #endif /* HYDRA_OF_LERNA */

   return(PSRP_OK);
}




/*-----------------------------------------------------------------------------------------------------
    Routine to automically extended protection to key-match files which have been added to
    the protected directory ...
-----------------------------------------------------------------------------------------------------*/

_PRIVATE void scan_directory(char *dir_name)

{   int i,
        index,
        n_used                = 0,
        n_alloc               = 0;

    DIR           *dirp       = (DIR *)NULL;
    struct dirent *next_entry = (struct dirent *)NULL;
    char          **nlist     = (char **)NULL;


    /*---------------------------------------------------*/
    /* Check that we can open the (protection) directory */
    /*---------------------------------------------------*/

    if((dirp = opendir(dir_name)) == (DIR *)NULL)
    {  if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): cannot open (protection) directory (%s)\n",
                                date,appl_name,appl_pid,appl_host,appl_owner,file_name);
          (void)fflush(stderr);

          pups_exit(255);
       }
    }


    /*-----------------------------------------------------*/
    /* Build snapshot of files in the protection directory */
    /*-----------------------------------------------------*/

    while((next_entry = readdir(dirp)) != (struct dirent *)NULL)
    {

        /*-----------------------------------------------------------------*/
        /* If any of the files in the directory are themselves a directory */
        /* they cannot be protected.                                       */
        /*-----------------------------------------------------------------*/

        (void)snprintf(pathname,SSIZE,"%s/%s",file_name,next_entry->d_name); 
        (void)stat(pathname,&stat_buf);

        if((S_ISREG(stat_buf.st_mode) || S_ISFIFO(stat_buf.st_mode))    &&
           strcmp(next_entry->d_name,".")    != 0                       &&
           strcmp(next_entry->d_name,"..")   != 0                       && 
           strin(next_entry->d_name,"uprot") == FALSE                    )
        {

           /*-------------------------------*/
           /* Allocate space for name table */ 
           /*-------------------------------*/

           if(n_used % ALLOC_QUANTUM == 0)
           {  n_alloc += ALLOC_QUANTUM;
              nlist = (char **)pups_calloc(n_alloc,sizeof(char *));
           } 

           nlist[n_used] = (char *)pups_malloc(SSIZE*sizeof(char));
           (void)strlcpy(nlist[n_used++],pathname,SSIZE);
        }
    }


    /*---------------------------------------------------------------------------------*/
    /* Ok - we have the current list of files in the protection directory -- unprotect */
    /* any time expired files and add any new files to the list                        */
    /*---------------------------------------------------------------------------------*/

    for(i=0; i<n_used; ++i)
    {  struct stat statbuf;


       /*---------------------------*/
       /* Do we have any new files? */
       /*---------------------------*/

       if((index = pups_get_ftab_index_by_name(nlist[i])) == (-1) && (strin(nlist[i],key) == TRUE || strcmp(key,"all") == 0))
       {  int fdes = (-1);
          char hname[SSIZE] = "";

          fdes = pups_open(nlist[i],0,LIVE);
          (void)pups_creator(fdes);
          (void)snprintf(hname,SSIZE,"default_fd_homeostat: %s",nlist[i]);
          (void)pups_fd_alive(fdes,hname,&pups_default_fd_homeostat);

          ++n_files;
       }       


       /*---------------------------------------------------*/
       /* If the key has been changed revoke the protection */
       /* of those file which do not match new key          */
       /*---------------------------------------------------*/

       if(index != (-1) && strin(ftab[index].fname,key) == FALSE && strcmp(key,"all") != 0)
       {  if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@%s:%s): protection revoked for file \"%s\"\n",
                            date,appl_name,appl_pid,appl_host,appl_owner,ftab[index].fname);
             (void)fflush(stderr);
          }

          (void)pupsighold(SIGALRM,TRUE);
          (void)pups_fd_dead(ftab[index].fdes);
          (void)pups_close(ftab[index].fdes);
          (void)pupsigrelse(SIGALRM);

          --n_files;
       }
    }


    /*-------------*/
    /* Free memory */
    /*-------------*/

    for(i=0; i<n_alloc; ++i)
       (void)pups_free((void *)nlist[i]);

    (void)pups_free((void *)nlist);
    (void)closedir(dirp);
}




/*------------------------------------------------------------------------------------------------------
    Add files (in directory) to protected list ...
------------------------------------------------------------------------------------------------------*/

_PRIVATE int myprotect(int argc, char *argv[])

{   int  index,
         fdes = (-1);

    char hname[SSIZE]    = "",
         pathname[SSIZE] = "";


    /*--------------------------*/
    /* Check command parameters */
    /*--------------------------*/

    if(argc == 1)
    {  (void)fprintf(psrp_out,"\nusage: protect <filename>\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(is_directory == TRUE)
       (void)snprintf(pathname,SSIZE,"%s/%s",file_name,argv[1]);
    else
       (void)strlcpy(pathname,argv[1],SSIZE);


    /*-------------------------------------------*/
    /* Block SIGALRM while we unprotect the file */
    /*-------------------------------------------*/

    (void)pupsighold(SIGALRM,TRUE);

    if(access(pathname,F_OK | R_OK | W_OK) == (-1))
    {  char uprot_pathname[SSIZE] = "";

       (void)snprintf(uprot_pathname,SSIZE,"%s.uprot",pathname);
       if(access(uprot_pathname,F_OK | R_OK | W_OK) == (-1)) 
       {  (void)fprintf(psrp_out,"\nfile \"%s\" does not exist\n\n",pathname);
          (void)fflush(psrp_out);


          /*---------------------------*/
          /* We can now release signal */
          /*---------------------------*/

          (void)pupsigrelse(SIGALRM);

          return(PSRP_OK);
       }
       else
          (void)rename(uprot_pathname,pathname);
    }


    /*----------------------------------------------------*/
    /* Is the file  we wish to protect already protected? */
    /*----------------------------------------------------*/

    if((index = pups_get_ftab_index_by_name(pathname)) != (-1))
    {  (void)fprintf(psrp_out,"\nfile \"%s\" is already protected\n\n",pathname);
       (void)fflush(psrp_out);


       /*---------------------------*/
       /* We can now release signal */
       /*---------------------------*/

       (void)pupsigrelse(SIGALRM);

       return(PSRP_OK);
    }


    /*-------------------------------------*/
    /* Add file to list of protected files */
    /*-------------------------------------*/

    (void)snprintf(hname,SSIZE,"default_fd_homeostat: %s",pathname);

    #ifdef USE_PROTECT
    fdes = protect((FILE *)NULL,pathname,hname,(void *)&pups_default_fd_homeostat);
    #else
    fdes = pups_open(pathname,2,LIVE);
    (void)pups_creator(fdes);
    (void)pups_fd_alive(fdes,hname,&pups_default_fd_homeostat);
    #endif /* USE_PROTECT */


    /*---------------------------*/
    /* We can now release signal */
    /*---------------------------*/

    (void)pupsigrelse(SIGALRM);

    (void)fprintf(psrp_out,"\nfile \"%s\" (homeostatically) protected\n\n",pathname);
    (void)fflush(psrp_out);
    
    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): file \"%s\" (homeostatically) protected\n",
                                    date,appl_name,appl_pid,appl_host,appl_owner,pathname);
       (void)fflush(stderr);
    }


    return(PSRP_OK);
}




/*------------------------------------------------------------------------------------------------------
    Revoke protection (on currently protected file) ...
------------------------------------------------------------------------------------------------------*/

_PRIVATE int myunprotect(int argc, char *argv[])

{   int index;

    char pathname[SSIZE]  = "",
         upathname[SSIZE] = "";


    /*--------------------------*/
    /* Check command parameters */
    /*--------------------------*/

    if(argc == 1)
    {  (void)fprintf(psrp_out,"\nusage: protect <filename>\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(is_directory == TRUE)
       (void)snprintf(pathname,SSIZE,"%s/%s",file_name,argv[1]);
    else
       (void)strlcpy(pathname,argv[1],SSIZE);

    if(access(pathname,F_OK | R_OK | W_OK) == (-1))
    {  (void)fprintf(psrp_out,"\nunprotect: file \"%s\" does not exist\n",pathname);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if((index = pups_get_ftab_index_by_name(pathname)) == (-1))
    {  (void)fprintf(psrp_out,"\nunprotect: file \"%s\" is not protected\n",pathname);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*-------------------------------------------*/
    /* Block SIGALRM while we unprotect the file */
    /*-------------------------------------------*/

    (void)pupsighold(SIGALRM,TRUE);

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): protection revoked for file \"%s\"\n",
                               date,appl_name,appl_pid,appl_host,appl_owner,pathname);
       (void)fflush(stderr);
    }

    #ifdef USE_PROTECT
    (void)unprotect(ftab[index].fdes);
    #else
    (void)pups_fd_dead(ftab[index].fdes);
    (void)pups_close(ftab[index].fdes);
    #endif /* USE_PROTECT */

    if(is_directory == TRUE)
    {  (void)snprintf(upathname,SSIZE,"%s.uprot",pathname);
       (void)rename(pathname,upathname);
    }


    /*---------------------------*/
    /* We can now release signal */
    /*---------------------------*/

    (void)pupsigrelse(SIGALRM);

    (void)fprintf(psrp_out,"\nprotection revoked for file \"%s\"\n\n",pathname);
    (void)fflush(psrp_out);

    return(PSRP_OK);
}




/*------------------------------------------------------------------------------------------------------
    Show files which are currently protected ...
------------------------------------------------------------------------------------------------------*/

_PRIVATE int show_protected_files(int argc, char *argv[])

{   int cnt = 0;

    DIR           *dirp       = (DIR *)NULL;
    struct dirent *next_entry = (struct dirent *)NULL;


    /*--------------------------*/
    /* Check comamnd parameters */
    /*--------------------------*/

    if(argc != 1)
    {  (void)fprintf(psrp_out,"\nusage: show\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(is_directory == FALSE)
    {  (void)fprintf(psrp_out,"show: \"%s\" is not a directory (cannot show files)\n",file_name);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    (void)fprintf(psrp_out,"\n    Protected files in directory \"%s\"\n\n",file_name);
    (void)fflush(psrp_out);

    dirp = opendir(file_name);
    while((next_entry = readdir(dirp)) != (struct dirent *)NULL)
    {

        /*-----------------------------------------------------------------*/
        /* If any of the files in the directory are themselves a directory */
        /* they cannot be protected.                                       */
        /*-----------------------------------------------------------------*/

        (void)snprintf(pathname,SSIZE,"%s/%s",file_name,next_entry->d_name);
        (void)stat(pathname,&stat_buf);

        if((S_ISREG(stat_buf.st_mode) || S_ISFIFO(stat_buf.st_mode))        &&
            strcmp(next_entry->d_name,".")    != 0                          &&
            strcmp(next_entry->d_name,"..")   != 0                          &&
            strin(next_entry->d_name,"uprot") == FALSE                      && 
            (strin(next_entry->d_name,key) == TRUE || strcmp(key,"all") == 0))
         {  (void)fprintf(psrp_out,"    %d:    %s\n",cnt,next_entry->d_name);
            (void)fflush(psrp_out);

            ++cnt;
         }
    }

    (void)closedir(dirp);

    if(cnt == 0)
       (void)fprintf(psrp_out,"\n    No files protected\n\n");
    else if(cnt == 1)
       (void)fprintf(psrp_out,"\n    1 file protected\n\n");
    else
       (void)fprintf(psrp_out,"\n    %d files protected\n\n",cnt);

    (void)fflush(psrp_out);
    return(PSRP_OK);
}



/*-------------------------------------------------------------------------------------------------------
    Set/show matched image I.D. key ...
-------------------------------------------------------------------------------------------------------*/

_PRIVATE int set_lifetime(int argc, char *argv[])

{

   /*-------------*/
   /* Get new key */
   /*-------------*/

    if(argc > 2)
    {  (void)fprintf(psrp_out,"\nusage: lifetime <lifetime in seconds | forever | default>\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*--------------------------------*/
    /* Display current protection key */
    /*--------------------------------*/

    if(argc == 1)
    {  if(lifetime == FOREVER) 
          (void)fprintf(psrp_out,"\nprotection lifetime is forever\n\n",file_name,key);
       else if(lifetime == DEFAULT_LIFETIME)
          (void)fprintf(psrp_out,"\nprotection lifetime is %d seconds (default)\n\n",DEFAULT_LIFETIME);
       else
          (void)fprintf(psrp_out,"\nprotection lifetime is %d seconds\n\n",file_name,key);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"forever") == 0)
    {  lifetime = FOREVER;

       (void)fprintf(psrp_out,"\nprotection lifetime is now forever\n\n");
       (void)fflush(psrp_out);
    }
    else if(strcmp(argv[1],"default") == 0)
    {  lifetime = DEFAULT_LIFETIME;

       (void)fprintf(psrp_out,"\nprotection lifetime is now %d (default)\n\n",file_name,key);
       (void)fflush(psrp_out);
    }
    else
    {  int itmp;

       if(sscanf(argv[1],"%d",&itmp) != 1 || itmp <= 0)
       {  (void)fprintf(psrp_out,"\nprotection lifetime parameter must be a positive integer\n\n",lifetime);
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }

       lifetime = itmp;
       (void)fprintf(psrp_out,"\nprotection lifetime is now %d\n\n",file_name,lifetime);
       (void)fflush(psrp_out);
    }

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       if(lifetime == FOREVER)
          (void)fprintf(stderr,"%s %s (%d@%s:%s): protection lifetime is now forever\n",
                                           date,appl_name,appl_pid,appl_host,appl_owner);
       else if(lifetime == DEFAULT_LIFETIME)
          (void)fprintf(stderr,"%s %s (%d@%s:%s): protection lifetime is now %d (default)\n",
                                       date,appl_name,appl_pid,appl_host,appl_owner,lifetime);
       else
          (void)fprintf(stderr,"%s %s (%d@%s:%s): protection lifetime is now %d\n",
                             date,appl_name,appl_pid,appl_host,appl_owner,lifetime);
       (void)fflush(stderr);
    }

    return(PSRP_OK);
}





/*-------------------------------------------------------------------------------------------------------
    Set/show matched image I.D. key ...
-------------------------------------------------------------------------------------------------------*/

_PRIVATE int set_key(int argc, char *argv[])

{

    /*-------------*/
    /* Get new key */
    /*-------------*/

    if(argc > 2)
    {  (void)fprintf(psrp_out,"\nusage: key [<(directory) file protection key>]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(is_directory == FALSE)
    {  (void)fprintf(psrp_out,"\n\"%s\" is not a directory (key invalid)\n\n",file_name);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*--------------------------------*/
    /* Display current protection key */
    /*--------------------------------*/

    if(argc == 1)
    {  (void)fprintf(psrp_out,"\n(directory \"%s\") file protection key is \"%s\"\n\n",file_name,key);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    (void)strlcpy(key,argv[1],SSIZE);
    (void)fprintf(psrp_out,"\n(directory \"%s\") file key changed to \"%s\"\n\n",file_name,key);
    (void)fflush(psrp_out);


    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): (directory file protection key changed to \"%s\"\n",
                                                  date,appl_name,appl_pid,appl_host,appl_owner,key);
       (void)fflush(stderr);
    }

    return(PSRP_OK);
}




/*-------------------------------------------------------------------------------------------------------
    Set principal (which is protected, or in which files are protected) ...
-------------------------------------------------------------------------------------------------------*/

_PRIVATE int set_principal(int argc, char *argv[])

{   int index;
    DIR           *dirp       = (DIR *)NULL;
    struct dirent *next_entry = (struct dirent *)NULL;


    /*-------------------*/
    /* Get new directory */
    /*-------------------*/

    if(argc > 2)
    {  (void)fprintf(psrp_out,"\nusage: dir [<directory>]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(argc == 1)
    {  if(is_directory == TRUE)
          (void)fprintf(psrp_out,"\n(directory) protecting principal (directory) \"%s\"\n\n",file_name);
       else
          (void)fprintf(psrp_out,"\n(directory) protecting principal (file) \"%s\"\n\n",file_name);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(access(argv[1],F_OK | R_OK | W_OK) == (-1))
    {  (void)fprintf(psrp_out,"\ncannot change principal to \"%s\" (does not exist)\n",file_name);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*---------------------------------------------*/
    /* Revoke all protections on current principal */
    /*---------------------------------------------*/

    (void)pupsighold(SIGALRM,TRUE);
    if(is_directory == TRUE)
    {  dirp = opendir(file_name);

       while((next_entry = readdir(dirp)) != (struct dirent *)NULL)
       {

            /*-----------------------------------------------------------------*/
            /* If any of the files in the directory are themselves a directory */
            /* they cannot be protected.                                       */
            /*-----------------------------------------------------------------*/

            (void)snprintf(pathname,SSIZE,"%s/%s",file_name,next_entry->d_name);
            (void)stat(pathname,&stat_buf);

            if((S_ISREG(stat_buf.st_mode) || S_ISFIFO(stat_buf.st_mode))       &&
               strcmp(next_entry->d_name,".")    != 0                          &&
               strcmp(next_entry->d_name,"..")   != 0                          &&
               strin(next_entry->d_name,"uprot") == FALSE                      &&
               (strin(next_entry->d_name,key) == TRUE || strcmp(key,"all") == 0))
            {  char pathname[SSIZE] = "";

               (void)snprintf(pathname,SSIZE,"%s/%s",file_name,next_entry->d_name);
               index = pups_get_ftab_index_by_name(pathname);

               if(index != (-1)) 
               {  (void)pups_fd_dead(ftab[index].fdes);
                  (void)pups_close(ftab[index].fdes);
               }
            }
       }

       (void)closedir(dirp);
    }
    else
    {  index = pups_get_ftab_index_by_name(file_name); 
       (void)pups_fd_dead(ftab[index].fdes);
       (void)pups_close(ftab[index].fdes);
    }

    (void)strlcpy(file_name,argv[1],SSIZE);
    (void)fprintf(psrp_out,"\nprincipal changed to \"%s\"\n\n",file_name);
    (void)fflush(psrp_out);


    /*---------------------------------------*/
    /* Check if new principal is a directory */
    /*---------------------------------------*/

    if((dirp = opendir(file_name)) != (DIR *)NULL)
    {  (void)closedir(dirp);
       is_directory = TRUE;
    }

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       if(is_directory == TRUE)
          (void)fprintf(stderr,"%s %s (%d@%s:%s): principal changed to (directory) \"%s\"\n",
                                      date,appl_name,appl_pid,appl_host,appl_owner,file_name);
       else
          (void)fprintf(stderr,"%s %s (%d@%s:%s): principal changed to (file) \"%s\"\n",
                                 date,appl_name,appl_pid,appl_host,appl_owner,file_name);
       (void)fflush(stderr);
    }

    return(PSRP_OK);
}




/*-------------------------------------------------------------------------------------------------------
    Exit function -- rename all files which have a .uprot extension ...
-------------------------------------------------------------------------------------------------------*/

_PRIVATE int exit_func(char *arg)

{   DIR           *dirp      = (DIR *)NULL;
    struct dirent *next_item = (struct dirent *)NULL;

    if(is_directory == FALSE)
       return(-1);

    dirp = opendir(file_name);
    while((next_item = readdir(dirp)) != (struct dirent *)NULL)
    {    if(strin(next_item->d_name,".uprot") == TRUE)
         {  char pathname[SSIZE]       = "",
                 uprot_pathname[SSIZE] = "";

            (void)snprintf(uprot_pathname,SSIZE,"%s/%s",file_name,next_item->d_name);
            (void)strlcpy(pathname,uprot_pathname,SSIZE);
            (void)strtrnc(pathname,'.',1);
            (void)rename(uprot_pathname,pathname);
         }
    }

    (void)unlink(prot_pathname);

    return(0);
}
