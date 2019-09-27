/*-------------------------------------------------------------------------------------------------------
    Purpose: Process which piggybacks a conventional UNIX application and allows it to use
             PUPS/PSRP services without a rewrite. 

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Dated:   30th August 2019 
    Version: 2.00 
    E-mail:  mao@@tumblingdice.co.uk
-------------------------------------------------------------------------------------------------------*/


#include <me.h>
#include <utils.h>
#include <netlib.h>
#include <psrp.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <hipl_hdr.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <vstamp.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>


/*-----------------*/
/* Version of pass */
/*-----------------*/

#define PASS_VERSION    "2.00"


#ifdef BUBBLE_MEMORY_SUPPORT
#include <bubble.h>
#endif /* BUBBLE_MEMORY_SUPPORT */


/*----------------------------------------------------------------*/
/* Get signal mapping appropriate to OS and hardware architecture */
/*----------------------------------------------------------------*/

#define __DEFINE__
#if defined(I386) || defined(X86_64)
#include <sig.linux.x86.h>
#endif

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

_PRIVATE void pass_slot(int level)
{   (void)fprintf(stderr,"int dynamic app (PSRP) pass %s: [ANSI C, PUPS MTD D]\n",PASS_VERSION);
 
    if(level > 1)
    {  (void)fprintf(stderr,"(C) 2005-2019 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"PUPS/PSRP service server-client (built %s %s)\n\n",__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");

    (void)fflush(stderr);
}
 
 
 
 
/*----------------------------*/ 
/* Application usage function */
/*----------------------------*/ 

_PRIVATE void pass_usage(void)

{   (void)fprintf(stderr,"[-payload <payload command pipeline>]\n");
    (void)fprintf(stderr,"[-sdir <sensitive directory:PEN>]\n");
    (void)fprintf(stderr,"[-multithreaded | -stdio]\n");
    (void)fprintf(stderr,"[-fifos]\n");
    (void)fprintf(stderr,"[-tag <tagname>]\n");
    (void)fprintf(stderr,"[-lyosome <lifetime:60 seconds>]\n\n");
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
_EXTERN void (* SLOT)() __attribute__ ((aligned(16))) = pass_slot;
_EXTERN void (* USE )() __attribute__ ((aligned(16))) = pass_usage;
#endif /* SLOT */




/*-------------------------------------------------------------------------------------------------------
    Application build date ...
-------------------------------------------------------------------------------------------------------*/

_EXTERN char appl_build_time[SSIZE] = __TIME__;
_EXTERN char appl_build_date[SSIZE] = __DATE__;




/*-------------------------------------------------------------------------------------------------------
    Public variables and function pointers used by this application ...
-------------------------------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------------------------------
    Functions which are private to this application ...
-------------------------------------------------------------------------------------------------------*/

// Restore (lost) sensitive directory 
_PROTOTYPE _PRIVATE void restore_sdir(char *);

// Is next item in thread table? 
_PROTOTYPE _PRIVATE _BOOLEAN in_thread_table(char *);

// Get a slot in the thread table/
_PROTOTYPE _PRIVATE int get_thread_index(char *, int *);

// Set sentitive directory O/P file type
_PROTOTYPE _PRIVATE int pass_set_sdir_ftype(int, char *[]);

// Set sensitive directory O/P mode
_PROTOTYPE _PRIVATE int pass_set_sdir_omode(int, char *[]);

// Set lyosome lifetime (in seconds)
_PROTOTYPE _PRIVATE int pass_set_sdir_lyosome_lifetime(int, char *[]);

// Remove junk (at exit)
_PROTOTYPE _PRIVATE void remove_junk(char *);

// Hoemeostat for sensitive directory 
_PROTOTYPE _PRIVATE int sdir_homeostat(void *, char *);

// Set command pipeline string
_PROTOTYPE _PRIVATE int pass_set_command_pipeline(int, char *[]);

// Set sensitive directory processing tag
_PROTOTYPE _PRIVATE int pass_set_sdir_tag(int, char *[]);

// Set processing mode for input data (in sensitive directory mode)
_PROTOTYPE _PRIVATE int pass_set_sdir_pmode(int, char *[]);

// Payload homeostat -- montors status of payload command pipeline
_PROTOTYPE _PRIVATE int payload_homeostat(void *, char *);

// Relay data to payload (from PASS server sensitive directory)
_PROTOTYPE _PRIVATE void sdir_loop(char *, char *);

// Relay data to payload (from PASS server stdio)
_PROTOTYPE _PRIVATE void stdio_loop(char *);

// Display current process status (via PSRP client)
_PROTOTYPE _PRIVATE int psrp_process_status(int, char *[]);




/*-------------------------------------------------------------------------------------------------------
    Variables which are private to this module ...
-------------------------------------------------------------------------------------------------------*/
                                                /*-----------------------------------------------------*/ 
_PRIVATE _BOOLEAN child_terminated = FALSE;     /* TRUE if child has been terminated                   */
_PRIVATE _BOOLEAN stdio_output     = FALSE;     /* TRUE if (serial) sdir O/P to stdio (not sdir)       */
_PRIVATE _BOOLEAN pass_master      = FALSE;     /* TRUE if we are the master PASS server               */
_PRIVATE _BOOLEAN sdir_mode        = FALSE;     /* TRUE if in senistive directory mode                 */
_PRIVATE _BOOLEAN in_stdio_loop    = TRUE;      /* TRUE if stdio loop (noit using sensitive directory) */
_PRIVATE _BOOLEAN thread_slot_wait = FALSE;     /* TRUE if waiting for thread slot (sdir par. mode)    */
_PRIVATE _BOOLEAN multithreaded    = FALSE;     /* TRUE if application is multithreaded                */
_PRIVATE _BOOLEAN use_fifos        = FALSE;     /* TRUE if sdir O/P to FIFO's                          */
_PRIVATE _BOOLEAN sdir_wait        = FALSE;     /* TRUE if waiting for file re-use in sdir             */
_PRIVATE char     tag[SSIZE]       = "notset";  /* Processing tag (for sdir)                           */
_PRIVATE char     sdir[SSIZE]      = "";        /* Sensitive directory                                 */
_PRIVATE int      child_pid        = (-1);      /* PID of command pipeline exec shell                  */
_PRIVATE int      thread[SSIZE]    = {(-1)};    /* PID of next slave PASS server                       */
_PRIVATE int      threads          = 1;         /* Number of currently executing threads               */
_PRIVATE int      t_index          = 0;         /* Index of next thread in thread table                */
_PRIVATE int      payload_cnt      = 0;         /* Number of payloads executed                         */
_PRIVATE int      lyosome_lifetime = 60;        /* Lyosome lifetime (in seconds)                       */
_PRIVATE int      losome           = (-1);      /* PID of sdir O/P lyosome process                     */
_PRIVATE int      lesome           = (-1);      /* PID of sdir error/log lyosome process               */
_PRIVATE char     command_pipeline[SSIZE] = ""; /* Payload command pipeline                            */
                                                /*-----------------------------------------------------*/ 

_PRIVATE char     iname[SSIZE]     = "";
_PRIVATE char     sdipath[SSIZE]   = "";
_PRIVATE char     sdopath[SSIZE]   = "";
_PRIVATE char     sdepath[SSIZE]   = "";

                                                         /*--------------------------------------------*/ 
_PRIVATE DIR      *dirp            = (DIR *)NULL;        /* Sensitive directory stream                 */
_PRIVATE char     *tsname[SSIZE]   = { (char *) NULL };  /* List of sdir input files being processed   */ 
                                                         /*--------------------------------------------*/ 


/*-------------------------------------------------------------------------------------------------------
    Software I.D. tag (used if CRIU support enabled to discard stale dynamic
    checkpoint files) ...
-------------------------------------------------------------------------------------------------------*/

#define VTAG  4175

extern int appl_vtag = VTAG;





/*--------------------------------------------------------------------------------------------------------
    Main - decode command tail then interpolate using parameters supplied by user ...
--------------------------------------------------------------------------------------------------------*/

_PUBLIC int pups_main(int argc, char *argv[])

{

    /*--------------------------------*/
    /* Wait for PUPS/P3 to initialise */
    /*--------------------------------*/

    (void)psrp_ignore_requests();


    /*------------------------------------------*/
    /* Get standard items form the command tail */
    /*------------------------------------------*/


    pg_leader = TRUE;
    pups_std_init(TRUE,
                  &argc,
                  PASS_VERSION,
                  "M.A. O'Neill",
                  "(PSRP) pass",
                  "2019",
                  argv);


    /*------------------------*/
    /* Register exit function */
    /*------------------------*/

    (void)pups_register_exit_f("remove_junk",(void *)&remove_junk,NULL);


    /*-----------------------------------------------------------------------*/
    /* Get name of process which is to served by the PUPS/PSRP PASS server   */
    /* A UNIX filter (or filter pipeline) is expected, e.g. input on stdin   */
    /* output on stdout, error on stderr. This pipeline is passed to a shell */
    /* which is slaved to current process.                                   */
    /*-----------------------------------------------------------------------*/

    if((ptr = pups_locate(&init,"payload",&argc,args,0)) != NOT_FOUND)
    {  if(strccpy(command_pipeline,pups_str_dec(&ptr,&argc,args)) == (char *)NULL)
          pups_error("[pass] expecting name of command pipeline");

       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@@%s:%s): payload pipeline: %s\n",
                  date,appl_name,appl_pid,appl_host,appl_owner,command_pipeline);
          (void)fflush(stderr);
       }
    }
    else
       pups_error("[pass] PASS server must have a payload command pipeline");


    /*-----------------------------------*/
    /* Do we have a sensitive directory? */
    /*-----------------------------------*/

    if((ptr = pups_locate(&init,"sdir",&argc,args,0)) != NOT_FOUND)
    {  if(strccpy(sdir,pups_str_dec(&ptr,&argc,args)) == (char *)NULL)
       {  int  i;
          char sdir_etag[SSIZE] = "";


          /*--------------------------------------------------*/
          /* Get name of sensitive directory from environment */
          /*--------------------------------------------------*/

          (void)snprintf(sdir_etag,SSIZE,"%s_SDIR",appl_name);

          for(i=0; i<strlen(sdir_etag); ++i)
             sdir_etag[i] = toupper(sdir_etag[i]);

          if(getenv(sdir_etag) != (char *)NULL)
             (void)strlcpy(sdir,getenv(sdir_etag),SSIZE);
          else 
          {

             /*---------------------------------------------------------*/
             /* Sensitive directory is the same as the application name */
             /*---------------------------------------------------------*/

             (void)strlcpy(sdir,appl_name,SSIZE);
          }
       }

       if(strcmp(sdir,".") == 0)
          pups_error("[pass] sensitive directory cannot be current working directory");

       if(access(sdir,F_OK | R_OK | W_OK) == 0)
       {  char errstr[SSIZE] = "";

          (void)snprintf(errstr,SSIZE,"[pass] sensitive directory \"%s\" already exists",sdir);
          pups_error(errstr);
       }
       else
          (void)mkdir(sdir,0700);

       sdir_mode = TRUE;
 
       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@@%s:%s): sensitive directory is \"%s\"\n",
                                  date,appl_name,appl_pid,appl_host,appl_owner,sdir);
          (void)fflush(stderr);
       }


       /*------------------------------------------------------*/
       /* Is O/P from payload pipeline to be written to FIFOS? */
       /*------------------------------------------------------*/

       if(pups_locate(&init,"fifos",&argc,args,0) != NOT_FOUND)
       {  use_fifos = TRUE;

          if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@@%s:%s): sensitive directory output nodes are FIFO's\n",
                                                        date,appl_name,appl_pid,appl_host,appl_owner);
             (void)fflush(stderr);
          }
       }


       /*-------------------------------------------------------------------*/
       /* Are itmes in the sensitive directory to be processed in parallel? */
       /*-------------------------------------------------------------------*/

       if(pups_locate(&init,"multithreaded",&argc,args,0) != NOT_FOUND)
       {  multithreaded = TRUE;

          if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@@%s:%s): input to sensitive directory processed in parallel\n",
                                                               date,appl_name,appl_pid,appl_host,appl_owner);
             (void)fflush(stderr);
          }

          threads = (-1);
       } 


       /*-----------------------*/
       /* (Serial) O/P to stdio */
       /*-----------------------*/

       if(pups_locate(&init,"stdio",&argc,args,0) != NOT_FOUND)
       {  stdio_output = TRUE;

          if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@@%s:%s): output of data and error/log to stdio enabled\n",
                                                          date,appl_name,appl_pid,appl_host,appl_owner);
             (void)fflush(stderr);
          }
       }     


       /*----------------*/
       /* Processing tag */
       /*----------------*/

       if(pups_locate(&init,"tag",&argc,args,0) != NOT_FOUND)
       {  if(strccpy(tag,pups_str_dec(&ptr,&argc,args)) == (char *)NULL)
             pups_error("[pass] expecting processing tag");
 
          if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@@%s:%s): sensitive directory processing tag is \"%s\" (only filenames containing tag will be processed)\n",
                                                                                                       date,appl_name,appl_pid,appl_host,appl_owner,tag);
             (void)fflush(stderr);
          }
       }
       else
       {  if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@@%s:%s): no processing tag (any sensitive directory file will be processed)\n",
                                                                           date,appl_name,appl_pid,appl_host,appl_owner,tag);
             (void)fflush(stderr);
          } 
       }


       /*------------------*/
       /* Lyosome lifetime */
       /*------------------*/

       if((ptr = pups_locate(&init,"lifetime",&argc,args,0)) != NOT_FOUND) 
       {  if((lyosome_lifetime = pups_i_dec(&ptr,&argc,args)) == (int)INVALID_ARG)
             pups_error("[pass] expecting lyosome lifetime (in seconds)");
       }

       if(appl_verbose == TRUE)
       {  (void)strdate(date);
          if(lyosome_lifetime == 60)
             (void)fprintf(stderr,"%s %s (%d@@%s:%s): lyosome lifetime is %d seconds (default)\n",
                                    date,appl_name,appl_pid,appl_host,appl_owner,lyosome_lifetime);
          else
             (void)fprintf(stderr,"%s %s (%d@@%s:%s): lyosome lifetime is %d seconds\n",
                          date,appl_name,appl_pid,appl_host,appl_owner,lyosome_lifetime);
          (void)fflush(stderr);
       }

    }                                          

    /*---------------------------------------*/
    /* Complain about any unparsed arguments */
    /*---------------------------------------*/

    pups_t_arg_errs(argd,args);


/*-------------------------------------------------------------------------------------------------------------
    Initialise PSRP function dispatch handler - note that the embryo is fully dynamic and prepared
    to import both dynamic functions and data objects ...
-------------------------------------------------------------------------------------------------------------*/

    psrp_init(PSRP_STATIC_FUNCTION | PSRP_STATIC_DATABAG | PSRP_HOMEOSTATIC_STREAMS,NULL);


    /*----------------------------------------------------------------*/
    /* Attach static functions which can be accessed from PSRP client */
    /*----------------------------------------------------------------*/

    (void)psrp_attach_static_function("status",  (void *)&psrp_process_status);
    (void)psrp_attach_static_function("tag",     (void *)&pass_set_sdir_tag);
    (void)psrp_attach_static_function("pmode",   (void *)&pass_set_sdir_pmode);
    (void)psrp_attach_static_function("omode",   (void *)&pass_set_sdir_omode);
    (void)psrp_attach_static_function("ftype",   (void *)&pass_set_sdir_ftype);
    (void)psrp_attach_static_function("lifetime",(void *)&pass_set_sdir_lyosome_lifetime);


    /*-----------------------------------------------------------*/
    /* We must define static bindings BEFORE loading the default */
    /* dispatch table. In the case of static bindings, the only  */
    /* effect of loading a saved dispatch table is to (possibly) */
    /* add object aliases                                        */
    /*-----------------------------------------------------------*/

    (void)psrp_load_default_dispatch_table();


    /*----------------------------------------------------------*/
    /* Tell PSRP clients we are ready to service their requests */
    /*----------------------------------------------------------*/

    psrp_accept_requests();

/*-------------------------------------------------------------------------------------------------------------
    Main loop of the PASS server. If sdir is set wait for files which are moved into the sensitive
    directory, otherwise wait for data to appear on stdin ...
-------------------------------------------------------------------------------------------------------------*/

    if(sdir_mode == TRUE)
       sdir_loop(sdir,command_pipeline);
    else
       stdio_loop(command_pipeline);


    /*--------------------------------------------------------------------------*/
    /* Exit from PUPS/PSRP application cleaning up any mess it may have created */
    /*--------------------------------------------------------------------------*/

    pups_exit(0);
}



/*-------------------------------------------------------------------------------------------------------------
    Execute command pipeline feeding data to/from PASS server stdio ...
-------------------------------------------------------------------------------------------------------------*/

_PRIVATE void stdio_loop(char *command_pipeline)

{   int nb,
        cin  = (-1),
        cout = (-1),
        cerr = (-1);

    _BYTE buf[512] = "";


    /*-------------------------------------------------*/
    /* Execute the command pipeline as a child process */
    /*-------------------------------------------------*/

    if(pups_system2(command_pipeline,shell,PUPS_ERROR_EXIT,&child_pid,&cin,&cout,&cerr) == (-1))
       pups_error("[pass] failed to launch payload pipeline");


    /*------------------*/
    /* Start homeostats */
    /*------------------*/

    (void)pups_setvitimer("payload_homeostat",1,VT_CONTINUOUS,1,NULL,(void *)payload_homeostat);
  

    /*------------------------------------------------------------*/ 
    /* Main loop of PASS server relaying data to payload pipeline */
    /* We may need to use select system call if this proves to be */
    /* too slow                                                   */
    /*------------------------------------------------------------*/

    (void)fcntl(0,   F_SETFL, fcntl(0,   F_GETFL,0) | O_NONBLOCK);
    (void)fcntl(cout,F_SETFL, fcntl(cout,F_GETFL,0) | O_NONBLOCK);
    (void)fcntl(cerr,F_SETFL, fcntl(cerr,F_GETFL,0) | O_NONBLOCK);

    while(TRUE)
    {   errno=0; nb = read(0,buf,512);
        if(nb == 0)
           (void)close(cin);

        if(nb > 0)
           (void)write(cin,buf,nb);

        nb = read(cout,buf,512);
        if(nb > 0)
           (void)write(1,buf,nb);

        nb = read(cerr,buf,512);
        if(nb > 0)
           (void)write(2,buf,nb);
 
        pups_usleep(100);
    }   
}




/*--------------------------------------------------------------------------------------------------------------
    Execute command pipeline feeding data to/from PSRP server sensitive directory ...
--------------------------------------------------------------------------------------------------------------*/

_PRIVATE void sdir_loop(char *sdir, char *command_pipeline)

{   struct dirent *next_item = (struct dirent *)NULL;

    int nb,
        cin        = (-1),
        cout       = (-1),
        cerr       = (-1),
        sdin       = (-1),
        sdout      = (-1),
        sderr      = (-1);

    _BYTE buf[512] = "";


    /*-------------------------------------*/
    /* Start sensitive directory homeostat */
    /*-------------------------------------*/

    (void)pups_setvitimer("sdir_homeostat",1,VT_CONTINUOUS,1,NULL,(void *)sdir_homeostat);


    /*----------------------------------------------------*/
    /* Wait for data to be put in the sensitive directory */
    /*----------------------------------------------------*/

    dirp = opendir(sdir);

    do {    next_item = readdir(dirp);


            /*---------------------------------------------*/
            /* If we have reached the end of the directory */
            /* simply rewind                               */
            /*---------------------------------------------*/

            if(next_item == (struct dirent *)NULL)
            {  (void)rewinddir(dirp); 
               next_item = readdir(dirp);
            }

            if((next_item                       != (struct dirent *)NULL                       &&
                strcmp(next_item->d_name,".")   != 0 && strcmp(next_item->d_name,"..") != 0)   && 
                strin(next_item->d_name,".out") != TRUE                                        &&
                strin(next_item->d_name,".err") != TRUE                                        &&
                in_thread_table(next_item->d_name) == FALSE                                    &&
                (strcmp(tag,"notset") == 0 || strin(next_item->d_name,tag) == TRUE)             )
            {  char lyosome_command[SSIZE] = "";

               if(appl_verbose == TRUE)
               {  (void)strdate(date);
                  if(pass_master == FALSE)
                     (void)fprintf(stderr,"%s %s (%d@@%s:%s): processing \"%s\" (in serial mode)\n",
                                     date,appl_name,appl_pid,appl_host,appl_owner,next_item->d_name);
                  else
                     (void)fprintf(stderr,"%s %s (%d@@%s:%s): processing \"%s\" (in multithreaded mode)\n",
                                            date,appl_name,appl_pid,appl_host,appl_owner,next_item->d_name);
                  (void)fflush(stderr);
               } 


               if(multithreaded == TRUE)
               {  

                  /*------------------------------------------------*/
                  /* Fork off a new relay process for each new item */
                  /* in the sensitive directory. We shall act as a  */
                  /* master for these processess                    */
                  /*------------------------------------------------*/

                  pass_master = TRUE;

                  threads = get_thread_index(next_item->d_name,&t_index);
                  if((thread[t_index] = pups_fork(FALSE,FALSE)) == 0)
                  {

                      /*--------------------*/
                      /* Restart homeostats */
                      /*--------------------*/

                      (void)pups_clearvitimer("psrp_homeostat");
                      (void)pups_malarm(1);


                      /*----------------------------------------------------*/
                      /* We are slave and cannot interact with PSRP clients */
                      /*----------------------------------------------------*/

                      pass_master = FALSE;
                      appl_pid    = getpid();

                      //(void)closeall((FILE *)NULL);

                      (void)strlcpy(iname,next_item->d_name,SSIZE);
                      (void)snprintf(sdipath,SSIZE,"%s/%s",sdir,next_item->d_name);
                      sdin = pups_open(sdipath,0,LIVE);

                      (void)snprintf(sdopath,SSIZE,"%s/%s.out",sdir,next_item->d_name);
                      if(stdio_output == FALSE && access(sdopath,F_OK | R_OK | W_OK) != (-1))
                      {  if(appl_verbose == TRUE)
                         {  (void)strdate(date);
                            (void)fprintf(stderr,"%s %s (%d@@%s:%s): ouput file for \"%s\" already exists -- waiting\n",
                                                         date,appl_name,appl_pid,appl_host,appl_owner,next_item->d_name);
                            (void)fflush(stderr);
                         }

                         sdir_wait = TRUE;
                         while(access(sdopath,F_OK | R_OK | W_OK) != (-1))
                              pups_usleep(100);
                         sdir_wait = FALSE;
                      } 
      
                      if(stdio_output == FALSE && access(sdepath,F_OK | R_OK | W_OK) != (-1))
                      {  if(appl_verbose == TRUE)
                         {  (void)strdate(date);
                            (void)fprintf(stderr,"%s %s (%d@@%s:%s): error/log file for \"%s\" already exists -- waiting\n",
                                                             date,appl_name,appl_pid,appl_host,appl_owner,next_item->d_name);
                            (void)fflush(stderr);
                         }

                         sdir_wait = TRUE;
                         while(access(sdepath,F_OK | R_OK | W_OK) != (-1))
                               pups_usleep(100);
                         sdir_wait = FALSE;
                      }   
    
                      if(use_fifos == TRUE)
                      {  (void)mkfifo(sdopath,0600);
                          sdout = pups_open(sdopath,2,LIVE); 
                      }
                      else if(stdio_output == TRUE)
                         sdout = pups_open("/dev/tty",1,LIVE);
                      else
                      {  (void)pups_creat(sdopath,0600);
                         sdout = pups_open(sdopath,1,LIVE); 
                      }

                      (void)snprintf(sdepath,SSIZE,"%s/%s.err",sdir,next_item->d_name);
                      if(access(sdepath,F_OK | R_OK | W_OK) != (-1))
                      {  if(appl_verbose == TRUE)
                         {  (void)strdate(date);
                            (void)fprintf(stderr,"%s %s (%d@@%s:%s): error/log file for \"%s\" already exists -- aborting\n",
                                                              date,appl_name,appl_pid,appl_host,appl_owner,next_item->d_name);
                            (void)fflush(stderr);
                         }

                         exit(-1);
                      }
                         
                      if(use_fifos == TRUE)
                      {  (void)mkfifo(sdepath,0600);
                          sderr = pups_open(sdepath,2,LIVE); 
                      }
                      else if(stdio_output == TRUE)
                         sderr = pups_open("/dev/tty",1,LIVE);
                      else
                      {  (void)pups_creat(sdepath,0600);
                         sderr = pups_open(sdepath,1,LIVE); 
                      }


                      /*-------------------------------------------------*/
                      /* Execute the command pipeline as a child process */
                      /*-------------------------------------------------*/

                      if(pups_system2(command_pipeline,shell,PUPS_ERROR_EXIT,&child_pid,&cin,&cout,&cerr) == (-1))
                      {

                         /*--------------------------------------------------------------*/
                         /* Enter idle loop until we have a new payload command pipeline */
                         /*--------------------------------------------------------------*/

                         while(strcmp(command_pipeline,"idle") == 0)
                               (void)pups_usleep(100);
                      }


                      /*--------------------------------------------------------------*/           
                      /* Start payload homeostat (for exec shell of command pipeline) */
                      /*--------------------------------------------------------------*/           

                      (void)pups_setvitimer("payload_homeostat",1,VT_CONTINUOUS,1,NULL,(void *)&payload_homeostat);
      
                      (void)fcntl(sdin,F_SETFL, fcntl(sdin,F_GETFL,0) | O_NONBLOCK);
                      (void)fcntl(cout,F_SETFL, fcntl(cout,F_GETFL,0) | O_NONBLOCK);
                      (void)fcntl(cerr,F_SETFL, fcntl(cerr,F_GETFL,0) | O_NONBLOCK);

                      do {     nb = read(sdin,buf,512);
                               if(nb == 0)
                                  (void)close(cin);

                               if(nb > 0)
                                  (void)write(cin,buf,nb);

                               nb = read(cout,buf,512);
                               if(nb > 0)
                                 (void)write(sdout,buf,nb);

                               nb = read(cerr,buf,512);
                               if(nb > 0)
                                  (void)write(sderr,buf,nb);

                               (void)pups_usleep(100);
                          } while(child_terminated == FALSE);


                       /*-------------------*/
                       /* Remove input file */
                       /*-------------------*/

                       (void)close(cout);
                       (void)close(cerr);
                       (void)pups_close(sdin);
                       (void)pups_close(sdout);
                       (void)pups_close(sderr);


                       /*------------------------------------*/
                       /* Reset looper switch for relay loop */
                       /*------------------------------------*/

                       child_terminated = FALSE;


                       /*---------------------------------------*/
                       /* Increment number of payloads executed */
                       /*---------------------------------------*/

                       ++payload_cnt;

                       if(stdio_output == FALSE)
                       {

                          /*----------------------------------------------*/
                          /* Start lyosomes to remove .out and .err files */
                          /* after given time period                      */
                          /*----------------------------------------------*/

                          (void)snprintf(lyosome_command,SSIZE,"lyosome -lifetime 60 %s",sdopath);
                          if((losome = pups_fork(FALSE,FALSE)) == 0)
                          {  (void)setsid();
                             (void)pups_execls(lyosome_command);

                             exit(-1);
                          }

                          (void)snprintf(lyosome_command,SSIZE,"lyosome -lifetime 60 %s",sdepath);
                          if((lesome = pups_fork(FALSE,FALSE)) == 0)
                          {  (void)setsid();
                             (void)pups_execls(lyosome_command);

                             exit(-1);
                          }
                       }         

                       exit(0);
                  }
                  else
                  {  if(kill(child_pid,SIGALIVE) != (-1))


                        /*------------------------------------------------------*/
                        /* Start payload homeostat (for next slave pass server) */
                        /*------------------------------------------------------*/

                        (void)pups_setvitimer("slavepass_homeostat",1,VT_CONTINUOUS,1,NULL,(void *)&payload_homeostat);
                  }
               }
               else
               {  

                  /*---------------------------------------------------*/
                  /* Process items in the sensitive directory serially */
                  /*---------------------------------------------------*/

                  (void)strlcpy(iname,next_item->d_name,SSIZE);
                  (void)snprintf(sdipath,SSIZE,"%s/%s",sdir,next_item->d_name);
                  sdin = pups_open(sdipath,0,LIVE);

                  (void)snprintf(sdopath,SSIZE,"%s/%s.out",sdir,next_item->d_name);
                  if(stdio_output == FALSE && access(sdopath,F_OK | R_OK | W_OK) != (-1))
                  {  if(appl_verbose == TRUE)
                     {  (void)strdate(date);
                        (void)fprintf(stderr,"%s %s (%d@@%s:%s): ouput file for \"%s\" already exists -- waiting\n",
                                                     date,appl_name,appl_pid,appl_host,appl_owner,next_item->d_name);
                        (void)fflush(stderr);
                     }

                     sdir_wait = TRUE;
                     while(access(sdopath,F_OK | R_OK | W_OK) != (-1))
                          pups_usleep(100);
                     sdir_wait = FALSE;
                  }
                            
                  if(use_fifos == TRUE)
                  {  (void)mkfifo(sdopath,0600);
                      sdout = pups_open(sdopath,2,LIVE);
                  }
                  else if(stdio_output == TRUE)
                     sdout = pups_open("/dev/tty",1,LIVE);
                  else
                  {  (void)pups_creat(sdopath,0600);
                      sdout = pups_open(sdopath,1,LIVE);
                  }

                  (void)snprintf(sdepath,SSIZE,"%s/%s.err",sdir,next_item->d_name);
                  if(stdio_output == FALSE && access(sdepath,F_OK | R_OK | W_OK) != (-1))
                  {  if(appl_verbose == TRUE)
                     {  (void)strdate(date);
                        (void)fprintf(stderr,"%s %s (%d@@%s:%s): error/log file for \"%s\" already exists -- waiting\n",
                                                         date,appl_name,appl_pid,appl_host,appl_owner,next_item->d_name);
                        (void)fflush(stderr);
                     }

                     sdir_wait = TRUE;
                     while(access(sdepath,F_OK | R_OK | W_OK) != (-1))
                          pups_usleep(100);
                     sdir_wait = FALSE;
                  }
   
                  if(use_fifos == TRUE)
                  {  (void)mkfifo(sdepath,0600);
                      sderr = pups_open(sdepath,2,LIVE);
                  }
                  else if(stdio_output == TRUE)
                     sderr = pups_open("/dev/tty",1,LIVE);
                  else
                  {  (void)pups_creat(sdepath,0600);
                      sderr = pups_open(sdepath,1,LIVE);
                  }


                  /*-------------------------------------------------*/                                  
                  /* Execute the command pipeline as a child process */
                  /*-------------------------------------------------*/                                  

                  if(pups_system2(command_pipeline,shell,PUPS_ERROR_EXIT,&child_pid,&cin,&cout,&cerr) == (-1))
                  {

                     /*--------------------------------------------------------------*/
                     /* Enter idle loop until we have a new payload command pipeline */
                     /*--------------------------------------------------------------*/

                     while(strcmp(command_pipeline,"idle") == 0)
                           (void)pups_usleep(100);
                  }

                  (void)fcntl(sdin,F_SETFL, fcntl(sdin,F_GETFL,0) | O_NONBLOCK);
                  (void)fcntl(cout,F_SETFL, fcntl(cout,F_GETFL,0) | O_NONBLOCK);
                  (void)fcntl(cerr,F_SETFL, fcntl(cerr,F_GETFL,0) | O_NONBLOCK);


                  /*--------------------------------------------------------------*/
                  /* Start payload homeostat (for exec shell of command pipeline) */
                  /*--------------------------------------------------------------*/

                  (void)pups_setvitimer("payload_homeostat",1,VT_CONTINUOUS,1,NULL,(void *)&payload_homeostat);

                  do {     nb = read(sdin,buf,512);
                           if(nb == 0)
                              (void)close(cin);

                           if(nb > 0)
                              (void)write(cin,buf,nb);

                           nb = read(cout,buf,512);
                           if(nb > 0)
                              (void)write(sdout,buf,nb);

                           nb = read(cerr,buf,512);
                           if(nb > 0)
                              (void)write(sderr,buf,nb);

                           (void)pups_usleep(10);
                      } while(child_terminated == FALSE);


                  /*-------------------*/
                  /* Remove input file */
                  /*-------------------*/

                  (void)close(cout);
                  (void)close(cerr);
                  (void)pups_close(sdin);
                  (void)pups_close(sdout);
                  (void)pups_close(sderr);


                  /*------------------------------------*/
                  /* Reset looper switch for relay loop */
                  /*------------------------------------*/

                  child_terminated = FALSE;


                  /*---------------------------------------*/
                  /* Increment number of payloads executed */
                  /*---------------------------------------*/

                  ++payload_cnt;

                  if(stdio_output == FALSE)
                  {  

                     /*----------------------------------------------*/
                     /* Start lyosomes to remove .out and .err files */
                     /* after given time period                      */
                     /*----------------------------------------------*/

                     (void)snprintf(lyosome_command,SSIZE,"lyosome -lifetime %d %s",lyosome_lifetime,sdopath);
                     if((losome = pups_fork(FALSE,FALSE)) == 0)
                     {   (void)setsid();
                         (void)pups_execls(lyosome_command);

                         exit(0);
                     }

                     (void)snprintf(lyosome_command,SSIZE,"lyosome -lifetime %d %s",lyosome_lifetime,sdepath);
                     if((lesome = pups_fork(FALSE,FALSE)) == 0)
                     {   (void)setsid();
                         (void)pups_execls(lyosome_command);

                         exit(0);
                     }
                  }
               }
            }

            (void)pups_usleep(100);
         } while(TRUE);
}




/*--------------------------------------------------------------------------------------------------------------
    Homeostat which checks that payload (shell) is still alive ...
--------------------------------------------------------------------------------------------------------------*/

_PRIVATE int payload_homeostat(void *t_info, char *args)

{   if(sdir_mode == FALSE)
    {  if(kill(child_pid,SIGALIVE) == (-1))
       {  if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@@%s:%s): payload pipeline has terminated -- PASS server exiting\n",
                                                                   date,appl_name,appl_pid,appl_host,appl_owner);
             (void)fflush(stderr);
          }

          pups_exit(0);
       }
    }

    if(pass_master == TRUE)
    {  if(kill(thread[t_index],SIGALIVE) == (-1))
       {  thread[t_index] = (-1);
          tsname[t_index] = (char *)pups_free((void *)tsname[t_index]);
          --threads;


          /*--------------------------------------------------------*/
          /* Remove junk associated with last (multithreaded) child */
          /*--------------------------------------------------------*/

          (void)unlink(sdipath);
          (void)unlink(sdopath);
          (void)unlink(sdepath);


          /*-------------------------*/
          /* Clear payload homeostat */
          /*-------------------------*/

          (void)pups_clearvitimer("slavepass_homeostat");
       }
    }
    else
    {  if(child_pid != (-1) && kill(child_pid,SIGALIVE) == (-1))
       {  child_terminated = TRUE;
          child_pid        = (-1);


          /*-------------------------------------------------*/
          /* Remove junk associated with last (serial) child */
          /*-------------------------------------------------*/

          (void)unlink(sdipath);


          /*-------------------------*/
          /* Clear payload homeostat */
          /*-------------------------*/

          (void)pups_clearvitimer("payload_homeostat");

          if(appl_verbose == TRUE)
          {  (void)strdate(date);
             (void)fprintf(stderr,"%s %s (%d@@%s:%s): payload pipeline has terminated -- PASS server waiting for new input\n",
                                                                                 date,appl_name,appl_pid,appl_host,appl_owner);
             (void)fflush(stderr);
          }    
       }
    }
}




/*---------------------------------------------------------------------------------------------------------------
    Homeostat which removes unwanted files from sensitive directory -- anything which does not match the
    filter tag is removed ...
---------------------------------------------------------------------------------------------------------------*/

_PRIVATE int sdir_homeostat(void *t_info, char *args)

{   DIR *dirp                = (DIR *)NULL;
    struct dirent *next_item = (struct dirent *)NULL;


    /*---------------------------------------*/
    /* If lyosomes have finished reset PID's */
    /*---------------------------------------*/

    if(kill(losome,SIGALIVE) == (-1))
       losome = (-1);

    if(kill(lesome,SIGALIVE) == (-1))
       lesome = (-1);

    if(access(sdir,F_OK | R_OK | W_OK) == (-1))
       restore_sdir(sdir);

    dirp = opendir(sdir);
    do {    next_item = readdir(dirp);

            if(next_item != (struct dirent *)NULL                                &&
               strcmp(next_item->d_name,".")   != 0                              &&
               strcmp(next_item->d_name,"..")  != 0                              &&
               strin(next_item->d_name,".out") != TRUE                           &&
               strin(next_item->d_name,".err") != TRUE                           &&
               (strcmp(tag,"notset") != 0 &&strin(next_item->d_name,tag) == FALSE))
            {  char pathname[SSIZE] = "";

               (void)snprintf(pathname,SSIZE,"%s/%s",sdir,next_item->d_name);
               (void)unlink(pathname);

               if(appl_verbose == TRUE)
               {  (void)strdate(date);
                  (void)fprintf(stderr,"%s %s (%d@@%s:%s): \"%s\" removed from sensitive directory \"%s\" (does not match tag \"%s\")\n",
                                                                 date,appl_name,appl_pid,appl_host,appl_owner,next_item->d_name,sdir,tag);
                  (void)fflush(stderr);
               }      
            }
        } while(next_item != (struct dirent *)NULL);

    (void)closedir(dirp);
    return(0);
}




/*---------------------------------------------------------------------------------------------------------------
    Set processing tag for sensitive directory. Any file containing this tag which appear in the sensitive
    directory will be processed ...
---------------------------------------------------------------------------------------------------------------*/

_PRIVATE int pass_set_sdir_tag(int argc, char *argv[])

{   if(sdir_mode == FALSE)
    {  (void)fprintf(psrp_out,"\nno sensitive directory (cannot set processing tag)\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }    

    if(argc == 1)
    {  (void)fprintf(psrp_out,"\ncurrent tag (for sensitive directory \"%s\" is \"%s\")\n\n",sdir,tag);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"help") == 0 || strcmp(argv[1],"usage") == 0 || argc != 2)
    {  (void)fprintf(psrp_out,"\n\nUsage  tag [help | usage] [<processing tag:%s>]\n\n",tag);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    (void)strlcpy(tag,argv[1],SSIZE);

    (void)fprintf(psrp_out,"\n tag (for sensitive directory \"%s\" set to \"%s\"\n\n",sdir,tag);
    (void)fflush(psrp_out);

    return(PSRP_OK);
}




/*---------------------------------------------------------------------------------------------------------------
    Set lifetime for lyosome function ... 
---------------------------------------------------------------------------------------------------------------*/

_PRIVATE int pass_set_sdir_lyosome_lifetime(int argc, char *argv[])

{   int i_tmp;

    if(sdir_mode == FALSE)
    {  (void)fprintf(psrp_out,"\nno sensitive directory (cannot set lyosome lifetime)\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(argc == 1)
    {  (void)fprintf(psrp_out,"\ncurrent lyosome lifetime (for sensitive directory \"%s\" is %d seconds)\n\n",sdir,lyosome_lifetime);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"help") == 0 || strcmp(argv[1],"usage") == 0 || argc != 2)
    {  (void)fprintf(psrp_out,"\n\nUsage  tag [help | usage] [<lifetime in seconds> | default]\n\n",tag);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"default") == 0)
       lyosome_lifetime = 60;
    else
    {  if(sscanf(argv[1],"&d",&i_tmp) != 1 || i_tmp < 0)
       {  (void)fprintf(psrp_out,"\n\nlifetime must be a (positive) integer\n\n",tag);
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }
       else
          lyosome_lifetime = i_tmp;     
    }

    if(lyosome_lifetime == 60)
       (void)fprintf(psrp_out,"\nlyosome lifetime (for sensitive directory \"%s\" set to %d seconds (default)\n\n",sdir,lyosome_lifetime);
    else
       (void)fprintf(psrp_out,"\nlyosome lifetime (for sensitive directory \"%s\" set to %d seconds\n\n",sdir,lyosome_lifetime);

    (void)fflush(psrp_out);

    return(PSRP_OK);
}         



/*---------------------------------------------------------------------------------------------------------------
    Set sensitive directory processing model. Currently multithreaded and serial modes are supported ...
---------------------------------------------------------------------------------------------------------------*/

_PRIVATE int pass_set_sdir_pmode(int argc, char *argv[])

{   if(sdir_mode == FALSE)
    {  (void)fprintf(psrp_out,"\nno sensitive directory (cannot set processing mode)\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }    

    if(argc == 1)
    {  if(multithreaded == FALSE)
          (void)fprintf(psrp_out,"\ncurrent processing mode for sdir \"%s\" is \"serial\"\n\n",sdir);
       else
          (void)fprintf(psrp_out,"\ncurrent processing mode for sdir \"%s\" is \"multithreaded\"\n\n",sdir);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"help") == 0 || strcmp(argv[1],"usage") == 0 || argc != 2)
    {  (void)fprintf(psrp_out,"\n\nUsage  sdpm [help | usage] [serial | multithreaded | parallel]\n\n",tag);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"serial") == 0)
    {  multithreaded = FALSE;

       (void)fprintf(psrp_out,"\n mode (for processing sensitive directory input serial)\n\n");
       (void)fflush(psrp_out);    
    }
    else if(strcmp(argv[1],"parallel") == 0)
    {  multithreaded = TRUE;

       (void)fprintf(psrp_out,"\n mode (for processing sensitive directory input multithreaded)\n\n");
       (void)fflush(psrp_out);    
    }
    else if(strcmp(argv[1],"multithreaded") == 0)
    {  multithreaded = TRUE;

       (void)fprintf(psrp_out,"\n mode (for processing sensitive directory input multithreaded)\n\n");
       (void)fflush(psrp_out);    
    }
    else
    {  (void)fprintf(psrp_out,"\nexpecting \"serial\", \"parallel\" or \"multithreaded\"\n\n");
       (void)fflush(psrp_out);
    }

    return(PSRP_OK);
}




/*---------------------------------------------------------------------------------------------------------------
    Set output mode (for sdir) ...
---------------------------------------------------------------------------------------------------------------*/

_PRIVATE int pass_set_sdir_omode(int argc, char *argv[])

{   if(sdir_mode == FALSE)
    {  (void)fprintf(psrp_out,"\nno sensitive directory (cannot set sdir O/P mode)\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(argc == 1)
    {  if(stdio_output == TRUE)
          (void)fprintf(psrp_out,"\ncurrent payload O/P (for sensitive directory \"%s\" is sent to stdout and stderr\n\n",sdir);
       else
          (void)fprintf(psrp_out,"\ncurrent payload O/P (for sensitive directory \"%s\" is sent to sensitive directory\n\n",sdir);
        
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"help") == 0 || strcmp(argv[1],"usage") == 0 || argc != 2)
    {  (void)fprintf(psrp_out,"\n\nUsage  sdpm [help | usage] [stdio | sdir | default]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"stdio") == 0)
    {  stdio_output = TRUE;

       (void)fprintf(psrp_out,"\nsensitive directory O/P sent to stdout and stderr\n\n");
       (void)fflush(psrp_out);
    }
    else if(strcmp(argv[1],"sdir") == 0)
    {  stdio_output = FALSE;

       (void)fprintf(psrp_out,"\nsensitive directory O/P sent to sensitive directory\n\n");
       (void)fflush(psrp_out);
    }
    else if(strcmp(argv[1],"default") == 0)
    {  stdio_output = FALSE;

       (void)fprintf(psrp_out,"\nsensitive directory O/P sent to default location (sensitive directory)\n\n");
       (void)fflush(psrp_out);
    }
    else
    {  (void)fprintf(psrp_out,"\nexpecting \"stdio\", \"sdir\" or \"default\"\n\n");
       (void)fflush(psrp_out);
    }

    return(PSRP_OK);
}




/*---------------------------------------------------------------------------------------------------------------
    Set output file mode (for sdir) ...
---------------------------------------------------------------------------------------------------------------*/

_PRIVATE int pass_set_sdir_ftype(int argc, char *argv[])

{   if(sdir_mode == FALSE)
    {  (void)fprintf(psrp_out,"\nno sensitive directory (cannot set sdir file type)\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(stdio_output == TRUE)
    {  (void)fprintf(psrp_out,"\noutput being sent to stderr and stdin (cannot set sdir file type)\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }    

    if(argc == 1)
    {  if(use_fifos == TRUE)
          (void)fprintf(psrp_out,"\ncurrent O/P file type (for sensitive directory \"%s\" is FIFO\n\n",sdir);
       else
          (void)fprintf(psrp_out,"\ncurrent O/P file type (for sensitive directory \"%s\" is REGF\n\n",sdir);

       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"help") == 0 || strcmp(argv[1],"usage") == 0 || argc != 2)
    {  (void)fprintf(psrp_out,"\n\nUsage  sdpm [help | usage] [fifo | regf | default]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"fifo") == 0)
    {  use_fifos = TRUE;

       (void)fprintf(psrp_out,"\nsensitive directory O/P file type set to FIFO\n\n");
       (void)fflush(psrp_out);
    }
    else if(strcmp(argv[1],"regf") == 0)
    {  use_fifos = FALSE;

       (void)fprintf(psrp_out,"\nsensitive directory O/P file type set to REGF\n\n");
       (void)fflush(psrp_out);
    }
    else if(strcmp(argv[1],"default") == 0)
    {  use_fifos = FALSE;

       (void)fprintf(psrp_out,"\nsensitive directory O/P file type set to default (REGF)\n\n");
       (void)fflush(psrp_out);
    }
    else
    {  (void)fprintf(psrp_out,"\nexpecting \"fifo\", \"regf\" or \"default\"\n\n");
       (void)fflush(psrp_out);
    }

    return(PSRP_OK);
}




/*---------------------------------------------------------------------------------------------------------------
    Set sensitive directory processing model. Currently multithreaded and serial modes are supported ...
---------------------------------------------------------------------------------------------------------------*/

_PRIVATE int pass_set_command_pipeline(int argc, char *argv[])

{   if(sdir_mode == FALSE)
    {  (void)fprintf(psrp_out,"\nno sensitive directory (cannot set command pipeline)\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(argc == 1)
    {  (void)fprintf(psrp_out,"\ncurrent command pipeline is \"%s\"\n\n",command_pipeline);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    if(strcmp(argv[1],"help") == 0 || strcmp(argv[1],"usage") == 0 || argc != 2)
    {  (void)fprintf(psrp_out,"\n\nUsage  sdcp [help | usage] <command pipeline>\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    (void)strlcpy(command_pipeline,argv[1],SSIZE);

    (void)fprintf(psrp_out,"command pipeline is now \"%s\"\n\n",command_pipeline);
    (void)fflush(psrp_out);

    return(PSRP_OK);
}                   




/*------------------------------------------------------------------------------
    Report process status ...
------------------------------------------------------------------------------*/

_PRIVATE int psrp_process_status(int argc, char *argv[])

{    (void)fprintf(psrp_out,"\n    PASS server status status\n");
     (void)fprintf(psrp_out,"    =========================\n\n");
     (void)fflush(psrp_out);

#if defined(CRIU_SUPPORT)
        (void)fprintf(psrp_out,"    Binary is Criu enabled (checkpointable)\n");
#endif /* CRIU_SUPPORT */

     /* I/O mode information */
     if(sdir_mode == TRUE)
     {  (void)fprintf(psrp_out,"    I/O mode              : sdir (data read from sensitive directory)\n"); 
        (void)fprintf(psrp_out,"    sensitive directory is: %s (processing tag is \"%s\")\n",sdir,tag);
        if(multithreaded == TRUE)
        {  if(thread_slot_wait == TRUE)
              (void)fprintf(psrp_out,"    processing mode       : multithreaded (waiting for free thread slot)\n");
           else
              (void)fprintf(psrp_out,"    processing mode       : multithreaded\n");
           if(threads == (-1))
              (void)fprintf(psrp_out,"    Active payload threads: 0\n");
           else
              (void)fprintf(psrp_out,"    Active payload threads: %d\n",threads);   
        }
        else
           (void)fprintf(psrp_out,"    processing mode       : serial\n");

        if(stdio_output == TRUE)
           (void)fprintf(psrp_out,"    output/log file type  : stdio (stdout and stderr)\n");
        else if(use_fifos == TRUE)
           (void)fprintf(psrp_out,"    output/log file type  : FIFO\n"); 
        else
           (void)fprintf(psrp_out,"    output/log file type  : REGF\n"); 

        if(lyosome_lifetime == 60)
           (void)fprintf(psrp_out,"    Lyosome lifetime      : 60 seconds (default)\n");
        else
           (void)fprintf(psrp_out,"    Lyosome lifetime      : %d seconds\n",lyosome_lifetime);
        (void)fflush(psrp_out); 

        if(multithreaded == FALSE)
        {  if(losome != (-1) || lesome != (-1))
              (void)fprintf(psrp_out,"    Lyosome processe s    : active (protecting %s.out; %s.err\n",iname,iname);
           else
              (void)fprintf(psrp_out,"    Lyosome processes     : inactive\n");
           (void)fflush(psrp_out);
        }

        if(multithreaded == FALSE)
        {  if(stdio_output == FALSE && sdir_wait == TRUE)
              (void)fprintf(psrp_out,"    sdir wait             : TRUE (waiting to reuse name \"%s\")\n",iname);
           else
              (void)fprintf(psrp_out,"    sdir wait             : FALSE\n");
           (void)fflush(psrp_out);
        }
     }
     else
     {  (void)fprintf(psrp_out,"    I/O mode              : stdio (data read from stdio)\n");
        (void)fprintf(psrp_out,"    processing mode       : serial\n");
     }


     /*----------------------------------*/
     /* Current payload pipeline/command */
     /*----------------------------------*/

     if(multithreaded == FALSE)
     {  if(threads == 1)
           (void)fprintf(psrp_out,"    Payload pipeline      : \"%s\" (1 concurrent thread, %d payload(s) processed)\n",
                                                                                     command_pipeline,payload_cnt); 
        else
           (void)fprintf(psrp_out,"    Payload pipeline      : \"%s\" (%d concurrent threads, %d payload(s) processed)\n",
                                                                               command_pipeline,threads,payload_cnt); 
        (void)fflush(psrp_out);
     }

     (void)fprintf(psrp_out,"\n\n");
     (void)fflush(psrp_out);

     return(PSRP_OK);
}




/*------------------------------------------------------------------------------
    Remove junk (at exit)  ...
------------------------------------------------------------------------------*/

_PRIVATE void remove_junk(char *arg_str)

{   char sdir_command[SSIZE] = "";


    /*----------------------------------------*/
    /* Terminate any active lyosome processes */
    /*----------------------------------------*/

    if(losome != (-1))
       (void)kill(losome,SIGTERM);
    
    if(lesome != (-1))
       (void)kill(lesome,SIGTERM);


    /*--------------------------------------------------*/    
    /* Remove sensitive directory (and any files in it) */
    /*--------------------------------------------------*/    

    (void)snprintf(sdir_command,SSIZE,"rm -rf %s",sdir);
    (void)system(sdir_command);
}




/*--------------------------------------------------------------------------------
    Get thread index ...
--------------------------------------------------------------------------------*/

_PRIVATE int get_thread_index(char *in_name, int *t_index)

{   int i;


    /*----------------------------------*/
    /* Find free entry in thread index  */
    /* We may have to wait until a slot */
    /* in the thread table is available */
    /*----------------------------------*/

    if(threads > 256)
    {  if(appl_verbose == TRUE)
       {  (void)strdate(date);
          (void)fprintf(stderr,"%s %s (%d@@%s:%s): thread table full -- waiting for slot\n",
                                               date,appl_name,appl_pid,appl_host,appl_owner);
          (void)fflush(stderr);
       }

       thread_slot_wait = TRUE;
    }

    while(threads > 256)
         (void)pups_usleep(100);
    thread_slot_wait = FALSE;


    /*---------------------------------*/   
    /* We have a slot -- lets find it! */
    /*---------------------------------*/   

    for(i=0; i<256; ++i)
    {  if(thread[i] == (-1))
       {  *t_index = i;
          tsname[i] = (char *)pups_malloc(SSIZE);
          (void)strlcpy(tsname[i],in_name,SSIZE);

          ++threads;
          return(i);
       }
    }

    /*------------------------*/
    /* We should not get here */
    /*------------------------*/

    pups_error("[update_thread_index] thread slot allocation failed");
}




/*--------------------------------------------------------------------------------
    Is next item in the thread table  ...
--------------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN in_thread_table(char *item)

{   int i;

    if(multithreaded == FALSE)
       return(FALSE);

    for(i=0; i<256; ++i)
    {  if(tsname[i] != (char *)NULL && strcmp(tsname[i],item) == 0)
          return(TRUE);
    }

    return(FALSE);
}




/*---------------------------------------------------------------------------------
    Restore the sensitive directory ...
---------------------------------------------------------------------------------*/

_PRIVATE void restore_sdir(char *sdir)

{    (void)mkdir(sdir,0700);
     dirp = opendir(sdir);

     if(appl_verbose == TRUE)
     {  (void)strdate(date);
        (void)fprintf(stderr,"%s %s (%d@@%s:%s): sensitive directory \"%s\" lost -- restoring\n",
                                               date,appl_name,appl_pid,appl_host,appl_owner,sdir);
        (void)fflush(stderr);
     }
}
