/*-------------------------------------------------------------------------------------------------------
    Purpose: The digital maggot -- searches for psrp files and FIFOS and removes them if
             they are stale (e.g. exist without an associated PSRP process)  

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 3.03 
    Dated:   14th October 2023
    E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------------------------------------------*/


#include <me.h>
#include <utils.h>
#include <netlib.h>
#include <psrp.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <bsd/string.h>
#include <errno.h>
#include <vstamp.h>
#include <sys/stat.h>


/*-------------------*/
/* Version of maggot */
/*-------------------*/

#define MAGGOT_VERSION    "3.03"


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



/*------------------------------------------------------------------------------------------------------
    Get application information for slot manager ...
------------------------------------------------------------------------------------------------------*/
/*---------------------------*/ 
/* Slot information function */
/*---------------------------*/ 

_PRIVATE void maggot_slot(int level)
{   (void)fprintf(stderr,"int app (PSRP) maggot %s: [ANSI C]\n",MAGGOT_VERSION);
 
    if(level > 1)
    {  (void)fprintf(stderr,"(C) 1999-2022 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"The digital maggot (PUPS stale resource and garbage collection) (built %s)\n\n",__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");

    (void)fflush(stderr);
}
 
 
 
 
/*----------------------------*/ 
/* Application usage function */
/*----------------------------*/ 

_PRIVATE void maggot_usage()

{   
    (void)fprintf(stderr,"[-search <directory list:/tmp;/fifos/<localhost>]\n");
    (void)fprintf(stderr,"[-parse <key list>\n");
    (void)fprintf(stderr,"[-delay_period <minutes:60>]\n"); 
    (void)fprintf(stderr,"[-global:FALSE]\n\n");
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
_EXTERN void (* SLOT)() __attribute__ ((aligned(16))) = maggot_slot;
_EXTERN void (* USE )() __attribute__ ((aligned(16))) = maggot_usage;
#endif /* SLOT */



/*-------------------------------------------------------------------------------------------------------
    Application build date ...
-------------------------------------------------------------------------------------------------------*/

_EXTERN char appl_build_time[SSIZE] = __TIME__;
_EXTERN char appl_build_date[SSIZE] = __DATE__;




/*-------------------------------------------------------------------------------------------------------
    Defines which are used by this application ...
-------------------------------------------------------------------------------------------------------*/

#define N_ENTRIES     1024
#define MAX_S_DIRS    32
#define MAX_KEYS      32




/*-------------------------------------------------------------------------------------------------------
    Public variables and function pointers used by this application ...
-------------------------------------------------------------------------------------------------------*/




/*-------------------------------------------------------------------------------------------------------
    Functions which are private to this application ...
-------------------------------------------------------------------------------------------------------*/

/* Report on the status of the maggot process */
_PROTOTYPE _PRIVATE int psrp_process_status(int, char *[]);

/* Remove a stale PSRP resource */
_PROTOTYPE _PRIVATE _BOOLEAN psrp_remove_stale_objects(char *, char *);

/* PUPS exit function */
_PROTOTYPE _PRIVATE void maggot_exit_f(char *);

/* Builtin help for maggot PSRP API */
_PROTOTYPE _PRIVATE int psrp_help(int, char *[]);

/* PSRP function to set delay period */
_PROTOTYPE _PRIVATE int set_delay_period(int, char *[]);

/* PSRP function to add a directory to scan list */
_PROTOTYPE _PRIVATE int add_directory(int, char *[]);

/* PSRP function to remove a directory from scan list */
_PROTOTYPE _PRIVATE int remove_directory(int, char *[]);

/* PSRP function to add a key to key list */
_PROTOTYPE _PRIVATE int add_key(int, char *[]);

/* PSRP function to remove a key from key  list */
_PROTOTYPE _PRIVATE int remove_key(int, char *[]);





/*-------------------------------------------------------------------------------------------------------
    Variables which are private to this module ...
-------------------------------------------------------------------------------------------------------*/
                                                           /*---------------------------------------------------------*/
_PRIVATE int         n_entries          = 0;               /* Directory entries (used in directory scanning)          */
_PRIVATE int         scan_cnt           = 0;               /* Number of scan of directories completed                 */
_PRIVATE int         start_time         = 0;               /* Start of last file system scan                          */
_PRIVATE int         delay_period       = 60;              /* Period between file system scans                        */
_PRIVATE int         d_cnt              = 0;               /* Number of user defined directories scanned              */
_PRIVATE int         key_cnt              = 0;             /* Number of user defined serach keys                      */
_PRIVATE int         delete_cnt         = 0;               /* Number of stale PSRP items removed by this maggot       */
_PRIVATE int         log_wrap_cnt       = 1024;            /* Number of objects logged before log file wraps          */
_PRIVATE int         items_logged       = 0;               /* Items in log file                                       */
_PRIVATE long        wrap_pos           = 0L;              /* Rewrap position in log file                             */ 
_PRIVATE _BOOLEAN    e_l_allocated      = FALSE;           /* TRUE if directory entry list allocated                  */
_PRIVATE _BOOLEAN    global_maggot      = FALSE;           /* TRUE if maggot cleaning up global PUPS/P3 log files     */
_PRIVATE char        **entry_list        = (char **)NULL;  /* List of PSRP resources which may be stale               */
_PRIVATE struct stat buf;                                  /* Stat buffer for determining log stream type             */
_PRIVATE char        d_list[MAX_S_DIRS][SSIZE];            /* List of user scanned directories                        */
_PRIVATE char        key_list[MAX_S_DIRS][SSIZE];          /* List of user search keys                                */
                                                           /*---------------------------------------------------------*/



/*-------------------------------------------------------------------------------------------------------
    Software I.D. tag (used if CKPT support enabled to discard stale dynamic
    checkpoint files) ...
-------------------------------------------------------------------------------------------------------*/

#define VTAG  3795

extern int appl_vtag = VTAG;




/*-------------------------------------------------------------------------------------------------------
    Main - decode command tail then interpolate using parameters supplied by user ...
-------------------------------------------------------------------------------------------------------*/

_PUBLIC int pups_main(int argc, char *argv[])

{   int i;


    /*--------------------------------*/
    /* Wait for PUPS/P3 to initialise */
    /*--------------------------------*/

    (void)psrp_ignore_requests();

/*-------------------------------------------------------------------------------------------------------
    Get standard items form the command tail ...
-------------------------------------------------------------------------------------------------------*/

    pups_std_init(TRUE,
                  &argc,
                  MAGGOT_VERSION,
                  "M.A. O'Neill",
                  "(PSRP) maggot",
                  "2023",
                  argv);


    /*-------------------------*/
    /* Are we a global maggot? */
    /*-------------------------*/

    if(pups_locate(&init,"global",&argc,args,0) != NOT_FOUND)
    {  global_maggot = TRUE;

       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@%s:%s): global maggot\n",date,
                                                              appl_name,
                                                               appl_pid,
                                                              appl_host,
                                                             appl_owner,
                                                        delay_period/60);
          (void)fflush(stderr);
       }
    }


    /*-----------------------------------------------------------------------*/           
    /* Get the delay time -- the maggot will search the /fifo/<hostname> and */
    /* /tmp filesystems every delay seconds                                  */
    /*-----------------------------------------------------------------------*/           

    if((ptr = pups_locate(&init,"delay_period",&argc,args,0)) != NOT_FOUND)
    {  if((delay_period = pups_i_dec(&ptr,&argc,args)) == (int)INVALID_ARG)
          pups_error("[maggot] expecting delay period for PSRP file system reaping");

       delay_period = iabs(delay_period)*60;
    }
    else
       delay_period *= 60;

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): PSRP filesystem period of grace for stale resources is %d minutes\n",date,
                                                                                                               appl_name,
                                                                                                                appl_pid,
                                                                                                               appl_host,
                                                                                                              appl_owner,
                                                                                                         delay_period/60);
       (void)fflush(stderr);
    }


    /*---------------------------*/
    /* Initialise directory list */
    /*---------------------------*/

    for(i=0; i<MAX_S_DIRS; ++i)
       (void)strlcpy(d_list[i],"notset",SSIZE);


    /*----------------------------------------------------*/
    /* Get user defined list of directories to be seached */
    /*----------------------------------------------------*/

    if((ptr = pups_locate(&init,"search",&argc,args,0)) != NOT_FOUND)
    {  char d_name[SSIZE] = "";

       do {   
               /*----------------------*/
               /* Test for end of list */
               /*----------------------*/

               (void)strccpy(d_name,pups_str_dec(&ptr,&argc,args));
               if (strcmp(d_name,"") == 0)
                  break;


               /*------------------------*/
               /* Process next directory */
               /*------------------------*/
 
               else
               {
 
                  /*-------------------------*/
                  /* Can we access directory */ 
                  /*-------------------------*/

                  if(stat(d_name,&buf) == (-1) && appl_verbose == TRUE)
                  {  (void)strdate(date);
                     (void)fprintf(stderr,"%s %s (%d@%s:%s): cannot access %s\n",date,
                                                                            appl_name,
                                                                             appl_pid,
                                                                            appl_host,
                                                                           appl_owner,
                                                                               d_name);       
                      (void)fflush(stderr);
                  }


                  /*-------------------------------------------------------*/
                  /* Is the directory actually identified as such by stat? */
                  /*-------------------------------------------------------*/

                  if(!S_ISDIR(buf.st_mode) && appl_verbose == TRUE)
                  {  (void)strdate(date);
                     (void)fprintf(stderr,"%s %s (%d@%s:%s) %s is not a directory\n",date,
                                                                                appl_name,
                                                                                 appl_pid,
                                                                                appl_host,
                                                                               appl_owner,
                                                                                   d_name); 
                      (void)fflush(stderr);
                      pups_exit(255);
                  }


                  /*-----------------------------------------------------------------*/
                  /* Add the directory to the list of those which are to be searched */
                  /* by this maggot                                                  */ 
                  /*-----------------------------------------------------------------*/

                  else
                  {  

                     if(appl_verbose == TRUE)
                     {  (void)strdate(date);
                        (void)fprintf(stderr,"%s %s (%d@%s:%s) searching directory %s\n",date,
                                                                                    appl_name,
                                                                                     appl_pid,
                                                                                    appl_host,
                                                                                   appl_owner,
                                                                                       d_name);
                        (void)fflush(stderr);
                        pups_exit(255);
                     }


                     if(d_cnt == MAX_S_DIRS)
                        pups_error("[maggot] too many user defined search directories");
                     else
                     {  (void)strlcpy(d_list[d_cnt++],d_name,SSIZE);
                        (void)strlcpy(d_name,"",SSIZE);
                     }
                  }
               }
           } while (TRUE);
    }


    /*---------------------*/
    /* Initialise key list */
    /*---------------------*/

    for(i=0; i<MAX_KEYS; ++i)
       (void)strlcpy(key_list[i],"notset",SSIZE);


    /*-------------------------------*/
    /* User user defined search keys */
    /*-------------------------------*/

    if((ptr = pups_locate(&init,"parse",&argc,args,0)) != NOT_FOUND)
    {  char key[SSIZE] = "";

       do {  

               /*----------------------*/
               /* Test for end of list */
               /*----------------------*/

               (void)strccpy(key,pups_str_dec(&ptr,&argc,args));
               if (strcmp(key,"") == 0)
                  break;


               /*------------------------*/
               /* Add search key to list */
               /*------------------------*/

               else
               {  if(appl_verbose == TRUE)
                  {  (void)strdate(date);
                     (void)fprintf(stderr,"%s %s (%d@%s:%s) adding search key %s\n",date,
                                                                               appl_name,
                                                                                appl_pid,
                                                                               appl_host,
                                                                              appl_owner,
                                                                                     key);
                      (void)fflush(stderr);
                  }

                  if(d_cnt == MAX_KEYS)
                     pups_error("[maggot] too many user defined search keys");
                  else
                  {  (void)strlcpy(key_list[key_cnt++],key,SSIZE);
                     (void)strlcpy(key,"",SSIZE);
                  }
               }
           } while(TRUE);
    }


    /*--------------------*/
    /* Get log wrap count */
    /*--------------------*/

    (void)fstat(2,&buf);
    if(S_ISREG(buf.st_mode))
    {  if((ptr = pups_locate(&init,"delay_period",&argc,args,0)) != NOT_FOUND)
       {  if((log_wrap_cnt = pups_i_dec(&ptr,&argc,args)) == (int)INVALID_ARG)
              pups_error("[maggot] expecting maximum number of items in log file");

          log_wrap_cnt = iabs(log_wrap_cnt);
       }
    }

    if(appl_verbose == TRUE)
    {  (void)strdate(date);
       (void)fprintf(stderr,"%s %s (%d@%s:%s): log wrap count is %d items\n",date,
                                                                        appl_name,
                                                                         appl_pid,
                                                                        appl_host,
                                                                       appl_owner,
                                                                     log_wrap_cnt);
       (void)fflush(stderr);
    }


    /*---------------------------------------*/
    /* Complain about any unparsed arguments */
    /*---------------------------------------*/

    pups_t_arg_errs(argd,args);

    if(appl_verbose == TRUE)
    {  (void)fprintf(stderr,"%s %s (%d@%s:%s): started\n",date,
                                                     appl_name,
                                                      appl_pid,
                                                     appl_host,
                                                    appl_owner);
       (void)fflush(stderr);
    }


    /*-----------------------------------------------------------------------------*/
    /* Save file pointer - we will rewind to this position when the log is wrapped */
    /*-----------------------------------------------------------------------------*/

    if(S_ISREG(buf.st_mode))
       wrap_pos = pups_lseek(2,0,SEEK_CUR);

/*---------------------------------------------------------------------------------------------------------
    Initialise PSRP function dispatch handler - note that the embryo is fully dynamic and prepared
    to import both dynamic functions and data objects ...
---------------------------------------------------------------------------------------------------------*/

    (void)psrp_init(PSRP_STATUS_ONLY | PSRP_HOMEOSTATIC_STREAMS,&psrp_process_status);
    (void)psrp_attach_static_function("help",            &psrp_help);
    (void)psrp_attach_static_function("delay_period",    &set_delay_period);
    (void)psrp_attach_static_function("add_directory",   &add_directory);
    (void)psrp_attach_static_function("remove_directory",&remove_directory);
    (void)psrp_attach_static_function("add_key",         &add_key);
    (void)psrp_attach_static_function("remove_key",      &remove_key);
    (void)psrp_accept_requests();

/*---------------------------------------------------------------------------------------------------------
    Set up exit function to free memory allocated by the stale resource removal function ...
---------------------------------------------------------------------------------------------------------*/

    pups_register_exit_f("maggot_exit_f",
                         &maggot_exit_f,
                         (char *)NULL);

/*---------------------------------------------------------------------------------------------------------
    This is the pups_main loop of the maggot -- it periodically checks the /fifo/<hostname> and /tmp
    filesystems of its host and removes stale resources ...
---------------------------------------------------------------------------------------------------------*/

    do {    unsigned int i;


            /*-------------------------------------------*/
            /* Delay period between PSRP directory scans */
            /*-------------------------------------------*/

            if(scan_cnt > 0)
            {  start_time = time((time_t *)NULL);
               (void)pups_sleep(5);
            }
            ++scan_cnt;


            /*----------------------------------------------------*/
            /* Scan PSRP directories for stale objects and if any */
            /* are found remove them                              */
            /*----------------------------------------------------*/

            if(global_maggot == TRUE)
               (void)psrp_remove_stale_objects("/tmp","log"); 
            else
            {  (void)psrp_remove_stale_objects(appl_fifo_dir,"fifo");
               (void)psrp_remove_stale_objects(appl_fifo_dir,"pst");
               (void)psrp_remove_stale_objects("/tmp","fifo");
               (void)psrp_remove_stale_objects("/tmp","pst");


               /*---------------------------*/
               /* Process user defined keys */
               /*---------------------------*/

               for (i=0; i<key_cnt; ++i)
                   (void)psrp_remove_stale_objects("/tmp",key_list[i]);
            }


            /*-------------------------------------------------*/
            /* Scan user defined directories for stale objects */
            /* and remove any which are found                  */
            /*-------------------------------------------------*/

            for(i=0; i<d_cnt; ++i)
            {  unsigned int j;

               if(strcmp(d_list[i],"notset") != 0)
               {  (void)psrp_remove_stale_objects(d_list[i],"fifo");
                  (void)psrp_remove_stale_objects(d_list[i],"pst");


                  /*---------------------------*/
                  /* Process user defined keys */
                  /*---------------------------*/

                  for (j=0; j<key_cnt; ++j)
                      (void)psrp_remove_stale_objects(d_list[i],key_list[j]);
               }
            }

       } while(TRUE);


    /*------------------------------------*/
    /* If we are running as root recreate */
    /* /dev/tty and /dev/null             */
    /*------------------------------------*/

    if (getuid() == 0)
       (void)system("mktty");

    pups_exit(0);
}




/*---------------------------------------------------------------------------------------------------------
   Report on the status of this maggot ...
---------------------------------------------------------------------------------------------------------*/

_PRIVATE int psrp_process_status(int argc, char *argv[])

{    int i,
         u_d_scan_dirs  = 0,
         u_key_cnt      = 0;

     (void)fprintf(psrp_out,"\n\n    maggot version %s status\n",MAGGOT_VERSION);
     (void)fprintf(psrp_out,"    ==========================\n\n");

#if defined(CRIU_SUPPORT)
     (void)fprintf(psrp_out,"    Binary is Crui enabled (checkpointable)\n");
#endif  /* CRUI_SUPPORT */

     (void)fprintf(psrp_out,"    Stale resources will be deleted after %d minutes\n",delay_period/60);    
     (void)fprintf(psrp_out,"    Scanning \"%s\" for stale PSRP objects\n",appl_fifo_dir);

     if(strin(appl_fifo_dir,"fifos") == TRUE)
        (void)fprintf(psrp_out,"    Scanning \"/tmp\" for stale PSRP objects\n");


     /*----------------------------------*/
     /* User defined directories scanned */
     /*----------------------------------*/

     for(i=0; i<d_cnt; ++i)
     {  if(strcmp(d_list[i],"notset") != 0)
        {  if(u_d_scan_dirs == 0)
           {  (void)fprintf(psrp_out,"\n\n    User defined directory(s) to be scanned\n");
              (void)fprintf(psrp_out,"    =======================================\n\n");
              (void)fflush(psrp_out);
           } 

           (void)fprintf(psrp_out,"    %d: Scanning \"%s\" for stale PSRP objects\n",i,d_list[i]);
           (void)fflush(psrp_out);

           ++u_d_scan_dirs;
        }
     } 

     if(u_d_scan_dirs > 0)
        (void)fprintf(psrp_out,"\n    Total of %d user defined directory(s) to be scanned\n",d_cnt);


     /*----------------*/
     /* User file keys */
     /*----------------*/

     for(i=0; i<key_cnt; ++i)
     {  if(strcmp(key_list[i],"notset") != 0)
        {  if(u_key_cnt == 0)
           {  (void)fprintf(psrp_out,"\n\n    User file keys\n");
              (void)fprintf(psrp_out,"    ==============\n\n");
              (void)fflush(psrp_out);
           } 

           (void)fprintf(psrp_out,"    %d:  Stale files containing key \"%s\" will be removed\n",i,key_list[i]);
           (void)fflush(psrp_out);

           ++u_key_cnt;
        }
     } 

     if(u_key_cnt > 0)
        (void)fprintf(psrp_out,"\n    Total of %d user defined file keys\n",key_cnt);


     if(delete_cnt == 0)
        (void)fprintf(psrp_out,"\n\n    No stale PSRP resources removed\n\n",delete_cnt);
     else
        (void)fprintf(psrp_out,"\n\n    %d stale PSRP resources removed\n\n",delete_cnt);
     (void)fflush(psrp_out);

     return(PSRP_OK);
}



/*--------------------------------------------*.
/* Parse pid in <file name>-<pid> format file */
/*--------------------------------------------*/

_PRIVATE int parse_pidname(unsigned char *filename)

{    unsigned int i,
                  size    = 0,
                  cnt     = 0,
                  pid_pos = 0;

     int  pid             = (-1);
     char pidstr[SSIZE]   = "";


     /*----------------------*/
     /* Filename string size */
     /*----------------------*/

     size = strlen(filename);


     /*--------------------------*/
     /* Find start of PID string */
     /*--------------------------*/

     for (i=size; i>0; --i)
     {   if (filename[i] == '-')
         {  pid_pos = i + 1;
            break;
         }
     }


     /*--------*/
     /* No PID */
     /*--------*/

     if (pid_pos == 0)
        return (-1);


     /*----------------*/
     /* Get PID string */
     /*----------------*/

     for (i=pid_pos; i<size; ++i)
     {   pidstr[cnt] = filename[i];
         ++cnt;
     }


     /*-------------------*/
     /* Get (integer) PID */
     /*-------------------*/

     (void)sscanf(pidstr,"%d",&pid);
     return (pid);
}




/*------------------------------------------------------*/
/* Routine to remove stale items from PSRP file systems */
/*------------------------------------------------------*/

_PRIVATE _BOOLEAN psrp_remove_stale_objects(char *directory, char *object_key)

{   unsigned int i;

    int entry_cnt  = 0,
        psrp_pid   = (-1),
        owner      = (-1);


    /*----------------------------------------------------------*/
    /* If we cannot access directory containing stale resources */
    /* simply exit                                              */
    /*----------------------------------------------------------*/

    if(access(directory,F_OK | R_OK | W_OK) == (-1))
       return(FALSE);


    /*----------------------------------------------------------*/
    /* If this is the first time we have called this function   */
    /* create an entry list                                     */
    /*----------------------------------------------------------*/

    if(entry_list == (char **)NULL)
    {  entry_list = (char **)pups_calloc(N_ENTRIES,sizeof(char *)); 
       e_l_allocated = TRUE;
    }


    /*----------------------------------------------------------*/
    /* Build a list of objects in all the directories which may */
    /* contain stale PSRP objects                               */
    /*----------------------------------------------------------*/

    entry_list = pups_get_directory_entries(directory,object_key,&n_entries,&entry_cnt);
    for(i=0; i<n_entries; ++i)
    {  char strdum[SSIZE]     = "",
            next_entry[SSIZE] = "",
            hostname[SSIZE]   = "";

       (void)strclr(next_entry);
       (void)strclr(strdum);
       (void)strclr(hostname);
       psrp_pid    = (-1);
       (void)strlcpy(next_entry,entry_list[i],SSIZE);
       mchrep(' ',":#",next_entry);

                                                                                                         /*------------------------------*/
       if(sscanf(next_entry,"%s%s%s%s%s%d",strdum,strdum,hostname,strdum,strdum,&psrp_pid) == 6    ||    /* Standard PUP/P3 files/FIFO's */
          (psrp_pid = parse_pidname(entry_list[i]))                                        >= 0     )    /* <filename>-<pid>             */
                                                                                                         /*------------------------------*/
       {  int    ret,
                 c_time;

          char        pathname[1024] = "";
          struct stat buf;


          /*-----------------------------------------------------------*/
          /* Check if resource is stale and that it has been unchanged */
          /* for user specified delay period                           */
          /*-----------------------------------------------------------*/

          (void)snprintf(pathname,SSIZE,"%s/%s",directory,entry_list[i]);
          (void)stat(pathname,&buf);

          #ifdef SSH_SUPPORT
          if(global_maggot == TRUE)
          {  char pidname[SSIZE] = "";

             (void)snprintf(pidname,SSIZE,"%d%d",&psrp_pid,&owner);
             ret = pups_rkill(hostname,ssh_remote_port,appl_owner,pidname,SIGTERM);
	  }
          else
          #endif /* SSH_SUPPORT */

          ret    = kill(psrp_pid,SIGALIVE);
          c_time = time((time_t *)NULL);

                                                           /*-----------------------------*/
          if(psrp_pid != (-1)                         &&   /* PSRP channel/lockpost etc.  */
             c_time - buf.st_mtime >  delay_period    &&   /* Delay period OK             */
             owner                 == getuid()        &&   /* We own PSRP server          */
             psrp_pid              != appl_pid        &&   /* We are not damaging ourself */
             ret                   == (-1)             )   /* PSRP server is dead         */
                                                           /*-----------------------------*/
          {  

             /*-----------------------*/
             /* Remove stale resource */
             /*-----------------------*/

             (void)unlink(pathname);


             /*------------------------------------------------------*/
             /* Log the fact that we have removed the stale resource */
             /*------------------------------------------------------*/

             if(appl_verbose == TRUE)
             {  ++items_logged;
                if(items_logged == log_wrap_cnt)
                {  items_logged = 0;
                   (void)pups_lseek(2,wrap_pos,SEEK_SET);
                }

                (void)strdate(date);
                (void)fprintf(stderr,"%s %s (%d@%s:%s): stale PSRP resource (%s) has been removed\n",date,
                                                                                                appl_name,
                                                                                                 appl_pid,
                                                                                                appl_host,
                                                                                               appl_owner,
                                                                                            entry_list[i]);
                (void)fflush(stderr);
                ++delete_cnt;
             }
          }
       }
    }


    /*--------------------------------------------*/
    /* Free memory allocated by directory scanner */
    /*--------------------------------------------*/

    if(n_entries > 0)
    {  for(i=0; i<n_entries; ++i)
          entry_list[i] = (char *)pups_free((void *)entry_list[i]);
    }
}




/*---------------------------------------------------------------------------------------------------------
    Routine to remove stale items from PSRP file systems ...
---------------------------------------------------------------------------------------------------------*/

_PRIVATE void maggot_exit_f(char *arg_str)

{   int i;

    if(e_l_allocated == TRUE)
    {  for(i=0; i<n_entries; ++i)
       {  if(entry_list[i] != (char *)NULL)
             (void *)pups_free((void *)entry_list[i]);
       }

       (void *)pups_free((void *)entry_list);
    } 
}




/*---------------------------------------------------------------------------------------------------------
   Static PSRP function which adds directories to the list of directories to be scanned ...
---------------------------------------------------------------------------------------------------------*/

_PRIVATE int add_directory(int argc, char *argv[])

{   unsigned int i;


    /*-----------------------------------------------*/
    /* Check that we have the correct argument count */   
    /*-----------------------------------------------*/

    if(argc != 2)
    {  (void)fprintf(psrp_out,"\n    usage: add_directory <name of scanned directory>\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*-------------------------------*/
    /* Check argument is a directory */
    /*-------------------------------*/

    (void)stat(argv[1],&buf);
    if(!S_ISDIR(buf.st_mode))
    {  (void)fprintf(psrp_out,"\n    ERROR add_directory: %s is not a directory\n\n",argv[1]);
       (void)fflush(psrp_out);

       return(PSRP_ERROR);
    }


    /*----------------------------------------------------*/
    /* Check to see if we already scanning this directory */
    /*----------------------------------------------------*/

    for(i=0; i<MAX_S_DIRS; ++i)
    {  if(strcmp(d_list[i],argv[1]) == 0)
       {  (void)fprintf(psrp_out,"    ERROR add_directory: %s is already in the list of scanned directories\n",argv[1]);
          (void)fflush(psrp_out);

          return(PSRP_ERROR);
       }
    }


    /*-----------------------*/
    /* Add directory to list */
    /*-----------------------*/

    for(i=0; i<MAX_S_DIRS; ++i)
    {  
       if(strncmp(d_list[i],"notset",6) == 0)
       {  (void)strlcpy(d_list[i],argv[1],SSIZE);

          if(i >= d_cnt)
             ++d_cnt;

          (void)fprintf(psrp_out,"    add_directory: %s added to scanned directory list\n",argv[1]);
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }
    }
}




/*---------------------------------------------------------------------------------------------------------
   Static PSRP function which removes directory from the list of directories to be scanned ...
---------------------------------------------------------------------------------------------------------*/

_PRIVATE int remove_directory(int argc, char *argv[])

{   unsigned int i;


    /*-----------------------------------------------*/
    /* Check that we have the correct argument count */
    /*-----------------------------------------------*/

    if(argc != 2)
    {  (void)fprintf(psrp_out,"\n    usage: remove_directory <name of scanned directory>\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*----------------------------------------------------------------*/
    /* Search for directory and remove it from scanned directory list */
    /*----------------------------------------------------------------*/

    for(i=0; i<d_cnt; ++i)
    {  if(strcmp(argv[1],d_list[i]) == 0)
       {  (void)strlcpy(d_list[i],"notset",SSIZE);

          (void)fprintf(psrp_out,"    remove_directory: %s has been removed from scanned directory list\n",argv[1]);
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }
    }

    (void)fprintf(psrp_out,"\n    ERROR remove_directory: %s not found in scanned directory list\n\n",argv[1]);
    (void)fflush(psrp_out);

    return(PSRP_ERROR);
}




/*---------------------------------------------------------------------------------------------------------
   Static PSRP function which adds key to key list ...
---------------------------------------------------------------------------------------------------------*/

_PRIVATE int add_key(int argc, char *argv[])

{   unsigned int i;


    /*-----------------------------------------------*/
    /* Check that we have the correct argument count */   
    /*-----------------------------------------------*/

    if(argc != 2)
    {  (void)fprintf(psrp_out,"usage: add_key <key>\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*-------------------------------------*/
    /* Check to see if key already in list */
    /*-------------------------------------*/

    for(i=0; i<MAX_KEYS; ++i)
    {  if(strcmp(key_list[i],argv[1]) == 0)
       {  (void)fprintf(psrp_out,"\n    ERROR add_key: %s is already in key list\n\n",argv[1]);
          (void)fflush(psrp_out);

          return(PSRP_ERROR);
       }
    }


    /*-----------------*/
    /* Add key to list */
    /*-----------------*/

    for(i=0; i<MAX_KEYS; ++i)
    {  
       if(strncmp(key_list[i],"notset",6) == 0)
       {  (void)strlcpy(key_list[i],argv[1],SSIZE);

          if(i >= key_cnt)
             ++key_cnt;

          (void)fprintf(psrp_out,"    add_key: %s added to key list\n",argv[1]);
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }
    }
}




/*---------------------------------------------------------------------------------------------------------
   Static PSRP function which removes key from key list ...
---------------------------------------------------------------------------------------------------------*/

_PRIVATE int remove_key(int argc, char *argv[])

{   unsigned int i;


    /*-----------------------------------------------*/
    /* Check that we have the correct argument count */
    /*-----------------------------------------------*/

    if(argc != 2)
    {  (void)fprintf(psrp_out,"    usage: remove_key <key>\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*----------------------------------------------------------------*/
    /* Search for directory and remove it from scanned directory list */
    /*----------------------------------------------------------------*/

    for(i=0; i<key_cnt; ++i)
    {  if(strcmp(argv[1],key_list[i]) == 0)
       {  (void)strlcpy(key_list[i],"notset",SSIZE);

          (void)fprintf(psrp_out,"    remove_key: %s has been removed from file key list\n\n",argv[1]);
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }
    }

    (void)fprintf(psrp_out,"\n    ERROR remove_key: %s not found in file key list\n\n",argv[1]);
    (void)fflush(psrp_out);

    return(PSRP_ERROR);
}




/*---------------------------------------------------------------------------------------------------------
   Static PSRP function which sets scan delay period ...
---------------------------------------------------------------------------------------------------------*/

_PRIVATE int set_delay_period(int argc, char *argv[])

{   int tmp_delay_period;


    /*-----------------------------------------------*/
    /* Check that we have the correct argument count */
    /*-----------------------------------------------*/

    if(argc != 2)
    {  (void)fprintf(psrp_out,"    usage: delay_period <seconds>\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if (sscanf(argv[1],&tmp_delay_period) != 1 || tmp_delay_period < 0)
    {  (void)fprintf(psrp_out,"\n    ERROR: expecting delay period (integer >= 0)\n\n");
       (void)fflush(psrp_out);

       return(PSRP_ERROR);
    }


    /*----------------------*/
    /* Set new delay period */
    /*----------------------*/

    delay_period = tmp_delay_period;

    (void)fprintf(psrp_out,"    set_delay_period: scan delay period is now %04d seconds\n\n");
    (void)fflush(psrp_out);

    return(PSRP_OK);
}



/*---------------------------------*/
/* PSRP API - maggot specific help */
/*---------------------------------*/

_PRIVATE int psrp_help(int argc, char *argv[])

{
     (void)fprintf(psrp_out,"\n\n    Maggot version %s PSRP API\n",MAGGOT_VERSION);
     (void)fprintf(psrp_out,"    (C) Tumbling Dice 2023\n\n");
     (void)fprintf(psrp_out,"    PSRP command interface\n");
     (void)fprintf(psrp_out,"    ======================\n\n");
     (void)fprintf(psrp_out,"    status                                           : display current status of maggot\n");
     (void)fprintf(psrp_out,"    delay_period      !<seconds>!                    : set scan delay period <seconds>\n");
     (void)fprintf(psrp_out,"    add_directory     !<name>!                       : add directory <name> to list of scanned directories\n");
     (void)fprintf(psrp_out,"    remove_directory  !<name>!                       : remove directory <name> from list of scanned directories\n");
     (void)fprintf(psrp_out,"    add_key           !<name>!                       : add key <name> to list of files keys to check for removal\n");
     (void)fprintf(psrp_out,"    remove_key        !<name>!                       : remove key <name> to list of file keys to check for removal\n");
     (void)fflush(psrp_out);

     return(PSRP_OK);
}

