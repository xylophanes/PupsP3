/*-------------------------------------------------------------------------------
    Purpose: Embryonic process - functionality is assigned dynamically using PSRP
             and PSRP components of the PUPS environment 

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 3.03 
    Dated:   29th Decemeber 2024
    E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------------------*/

/*-------------------------------------------------------------*/
/* If we have OpenMP support then we also have pthread support */
/* (OpenMP is built on pthreads for Linux distributions).      */
/*-------------------------------------------------------------*/

#if defined(_OPENMP) && !defined(PTHREAD_SUPPORT)
#define PTHREAD_SUPPORT
#endif /* PTHREAD_SUPPORT */

#define TEST_THREADS

#include <me.h>
#include <utils.h>
#include <netlib.h>
#include <psrp.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <hipl_hdr.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <vstamp.h>
#include <casino.h>
#include <hash.h>
#include <bsd/bsd.h>

#ifdef PERSISTENT_HEAP_SUPPORT
#include <phmalloc.h>
#include <pheap.h>
#endif /* PERSISTENT_HEAP_SUPPORT */

#ifdef PTHREAD_SUPPORT
#include <tad.h>
#endif /* PTHREAD_SUPPORT */


#include <math.h>
#include <stdlib.h>


/*********************************/
/* Floating point representation */
/*********************************/

#include <ftype.h>


/*******************************/
/* Version of this application */
/*******************************/

#define EMBRYO_VERSION    "3.03"

#ifdef BUBBLE_MEMORY_SUPPORT
#include <bubble.h>
#endif /* BUBBLE_MEMORY_SUPPORT */




/***************************/
/* Size of persistent heap */
/***************************/

#define PHEAP_SIZE 64


/*-------------------------------------------------------------------------------*/
/* Function called when checkpoint file reloaded but before user code re-entered */
/*-------------------------------------------------------------------------------*/

_PRIVATE void restartfunc(void)

{   if(appl_verbose == TRUE)
    {  (void)fprintf(stderr,"\n\nrestart function\n\n\n\n");
       (void)fflush(stderr);
    }
}




/*--------------------------------------------*/
/* Function called when checkpoint file saved */
/*--------------------------------------------*/

_PRIVATE void ckptfunc(void)

{   (void)fprintf(stderr,"\n\ncheckpoint function\n\n\n\n");
    (void)fflush(stderr);
}




/*-------------------------------------------------------------------------------------------*/
/* Exit function for testfps() [interrupt when processing transactions to/from remote slave] */ 
/*-------------------------------------------------------------------------------------------*/

_PRIVATE int32_t testfps_exitf(int32_t chid)

{   des_t fdes,
          index;

    index = pups_get_ftab_index_by_id(chid);
    (void)pups_fcclose(ftab[index].stream);

    return(0);
}




/*----------------------------------------------------------*/
/* Display process status (to attached PSRP client process) */
/*----------------------------------------------------------*/

_PRIVATE  int32_t psrp_process_status(int32_t argc, char *argv[])
{

   #ifdef CRIU_SUPPORT
   (void)fprintf(psrp_out,"    Binary is Criu enabled (checkpointable)\n");
   #endif /* CRIU_SUPPORT */

   (void)fprintf(psrp_out,"    Embryo status function\n");
   (void)fflush(psrp_out);

   return(PSRP_OK);
}




/*----------------------------------------*/
/* Kill process which is running remotely */
/*----------------------------------------*/

_PRIVATE int32_t rkill(int32_t argc, char *argv[])

{  int32_t i,
           pid;

   char reply[256]   = "",
        command[256] = "";

   if(argc == 1 || argc > 4)
   {  (void)fprintf(psrp_out,"Usage: rkill <pid> <host> <port>\n");
      (void)fflush(psrp_out);

      return(PSRP_OK);
   }

   (void)pups_rkill(argv[2],argv[3],"mao",argv[1],SIGTERM);
   return(PSRP_OK);            
}




/*---------------------------------------------------------*/
/* Create a slaved interaction client (SIC) and talk to it */ 
/*---------------------------------------------------------*/

_PRIVATE int32_t tsic(int32_t argc, char *argv[])

{   uint32_t i,
             cnt         = 0,
             n_requests  = 0,
             tail_start  = 1;

    size_t j;

    int32_t ret,
            havePort     = (-1);
 

    char reply[256]      = "",
         rcmd[256]       = "",
         hostname[256]   = "notset",
         ssh_port[256]   = "",
         **replys        = (char **)NULL;
 
    if(argc == 1)
    {  (void)fprintf(psrp_out,"Usage: testsic [-server:port] <command list>\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*----------------------------------*/
    /* Extract remote hostname and port */
    /*----------------------------------*/

    (void)fprintf(psrp_out,"\n\nArgument vector\n");
    (void)fprintf(psrp_out,"===============\n\n");
    (void)fflush(psrp_out);

    for(i=tail_start; i<argc; ++i)
    {  if(argv[1][0] == '-')
       {  (void)strlcpy(hostname,&argv[1][1],SSIZE);
          tail_start = 2;

          for(j=0; j<strlen(hostname); ++j)
          {  if(hostname[j] == ':')
                havePort = j;
             else if(havePort != (-1))
             {  ssh_port[cnt] = hostname[j];
                ++cnt;
             }
          }
 
          if(havePort != (-1))
          {  ssh_port[cnt]      = '\0';
             hostname[havePort] = '\0';
          }
        }
 
        (void)fprintf(psrp_out,"argv[%d] = %s\n",i,argv[i]);
        (void)fflush(psrp_out);
    }
 

    /*----------------------------------------*/
    /* Extract command list for remote server */
    /*----------------------------------------*/

    for(i=tail_start; i<argc; ++i)
    {  

       /*------------------------------------------------------------*/
       /* We do not need to explictly background anything -- in fact */
       /* to do so is an error                                       */
       /*------------------------------------------------------------*/

       if(strin(argv[i],"&") == TRUE)
       {  (void)fprintf(psrp_out,"tsic syntax error: \"&\" found\n");
          (void)fflush(psrp_out);
 
          return(PSRP_OK);
       }
       else
       {

          /*--------------------------------------------*/
          /* Make sure multiple commands are terminated */
          /* using semi-colon                           */
          /*--------------------------------------------*/

          (void)strlcat(rcmd,argv[i],SSIZE);
          if(i < argc -1 )
             (void)strlcat(rcmd,"; ",SSIZE);
          else
            (void)strlcat(rcmd,"",  SSIZE);

          ++n_requests;
       }
    }


    /*-------------------------------*/
    /* Display information about SIC */
    /*-------------------------------*/

    (void)fprintf(psrp_out,"\n\nSIC info\n");
    (void)fprintf(psrp_out,"========\n\n");
    (void)fflush(psrp_out);

    if(strcmp(hostname,"") != 0)
    {  (void)fprintf(psrp_out,"host %s\n",hostname);
       (void)fflush(psrp_out);
    }

    if(strcmp(ssh_port,"") != 0)
    {  (void)fprintf(psrp_out,"port %s\n",ssh_port);
       (void)fflush(psrp_out);
    }

    (void)fprintf(psrp_out,"command list: \"%s\"\n\n",rcmd);
    (void)fflush(psrp_out);


    /*----------------------*/
    /* Process command list */
    /*----------------------*/

    if(strcmp(hostname,"notset") == 0)
       replys = psrp_process_sic_transaction_list((char *)NULL,"",n_requests,rcmd);
    else
       replys = psrp_process_sic_transaction_list(hostname,ssh_port,n_requests,rcmd);

    #ifdef EMBRYO_DEBUG
    (void)fprintf(psrp_out,"TRANSACTION SENT\n");
    (void)fflush(psrp_out);
    #endif /* EMBRYO_DEBUG */

    (void)fprintf(psrp_out,"\n\nReplys\n");
    (void)fprintf(psrp_out,"======\n\n");
    (void)fflush(psrp_out);

    if(errno != 0)
    {  if(strcmp(hostname,"notset") == 0)
          (void)fprintf(psrp_out,"\nfailed to create (local) [SIC] channel to slaved PSRP client [errno: %d]\n\n",errno);
       else
          (void)fprintf(psrp_out,"\nfailed to create (remote) [SIC] channel to slaved PSRP client (on host \"%s\") [errno: %d]\n\n",hostname,errno);
       (void)fflush(stderr);
    }
    else
    {  for(i=0; i<n_requests; ++i)
       {  if(replys[i] != (void *)NULL)
          {  (void)fprintf(psrp_out,"reply[%d]:\n",i); 
             (void)fputs(replys[i],psrp_out);
             (void)fprintf(psrp_out,"****\n\n");
            (void)fflush(psrp_out);
          }
       }
    }

    if(replys != (char **)NULL)
       (void)pups_afree(n_requests,(void *)replys);

    return(PSRP_OK);
}




/*----------------------------------------------*/
/* Get application information for slot manager */
/*----------------------------------------------*/
/*---------------------------*/ 
/* Slot information function */
/*---------------------------*/ 

_PRIVATE void embryo_slot(int32_t level)
{   (void)fprintf(stderr,"int app (PSRP) embryo %s: [ANSI C, PUPS MTD D]\n",EMBRYO_VERSION);
 
    if(level > 1)
    {  (void)fprintf(stderr,"(C) 1996-2024 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. ONeill\n");
       (void)fprintf(stderr,"Unassigned PSRP dynamic process (gcc %s: built %s %s)\n\n",__VERSION__,__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");

    (void)fflush(stderr);
}
 
 
 

/*----------------------------*/ 
/* Application usage function */
/*----------------------------*/ 

_PRIVATE void embryo_usage(void)

{   (void)fprintf(stderr,"[-state] [-hashtest:FALSE] [-pheaptest:FALSE]\n\n");
    (void)fprintf(stderr,"[>& <ASCII log file>]\n\n");

    (void)fprintf(stderr,"Signals\n\n");
    (void)fprintf(stderr,"SIGINIT SIGCHAN SIGPSRP: Process status [PSRP] request (protocol %5.2F)\n",PSRP_PROTOCOL_VERSION);
    (void)fprintf(stderr,"SIGCLIENT: tell client server is about to segment\n");

    #ifdef CRUI_SUPPORT
    (void)fprintf(stderr,"SIGCHECK SIGRESTART:      checkpoint and restart signals\n");
    #endif /* CRIU_SUPPORT */

    (void)fprintf(stderr,"SIGALIVE: check for existence of client on signal dispatch host\n");
    (void)fflush(stderr);
}


#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT)() __attribute__ ((aligned(16))) = embryo_slot;
_EXTERN void (* USE )() __attribute__ ((aligned(16))) = embryo_usage;
#endif /* SLOT */


/*------------------------*/
/* Application build date */
/*------------------------*/

_EXTERN char appl_build_time[256] = __TIME__;
_EXTERN char appl_build_date[256] = __DATE__;


/*-----------------------------------------------------------------*/
/* Public variables and function pointers used by this application */
/*-----------------------------------------------------------------*/

_PRIVATE _BYTE test_databag[1024] = "";




/*-------------------------------------------------*/
/* Functions which are private to this application */
/*-------------------------------------------------*/

// Test function 
_PROTOTYPE _PRIVATE int32_t static_test_function_object(int32_t, char *[]);

// ASCII text generation function 
_PROTOTYPE _PRIVATE int32_t ascii(int32_t, char *[]);

// Embryo exit function 
_PROTOTYPE _PRIVATE int32_t embryo_exit(int32_t, char *[]);

// Createand/or destroy asycnronous (test) threads
_PROTOTYPE _PRIVATE int32_t tthread(int32_t, char *[]);

// Createand/or destroy asycnronous (test) threads
_PROTOTYPE _PRIVATE int32_t threadfunc(void *);

// Simple test function to exercise various psrp functions
_PROTOTYPE _PRIVATE int32_t static_test_function_object(int32_t, char *[]);




/*-------------------------------------------------*/
/* Variables which are private to this application */
/*-------------------------------------------------*/

_BOOLEAN looper = TRUE;




/*--------------------*/
/* Software I.D. tag  */
/*--------------------*/

#define VTAG  8139

extern int32_t appl_vtag = VTAG;

typedef struct {   uint32_t  cnt;
                   FTYPE     x;
                   char      name[256];
               } test_type;




/*---------------------------*/
/* Main - parse command tail */
/*---------------------------*/

_PUBLIC  int32_t pups_main(int32_t argc, char *argv[])

{   uint32_t i,
             iter   = 0,
             cnt    = 0;

    int32_t  pheap_flags,
             loc[256]        = { [0 ... 255] = 0 },
             a,
             o_index1        = 0,
             o_index2        = 0,
             o_index3        = 0,
             o_index4        = 0,
             hdes1           = (-1),
             hdes2           = (-1),
             hdes3           = (-1);

    uint64_t tsize;

    test_type *h_ptr_1 = (test_type *)NULL;
    FTYPE     *h_ptr_2 = (FTYPE *)    NULL;
    test_type *h_ptr_3 = (test_type *)NULL;
    FTYPE     *h_ptr_4 = (FTYPE *)    NULL;

    char ls_path[256] = "",
         buf[256]     = "";

    void *argvec[32] = { [0 ... 31] = (void *)NULL };

    FTYPE             tf            = 1456.82;
    hash_table_type   *htab         = (hash_table_type *)NULL;
    pthread_t         tid,
                      tid1,
                      tid2;

    FILE              *tstream      = (FILE *)NULL;
    psrp_channel_type *embryo_sic   = (psrp_channel_type *)NULL;
    _BYTE             *tbuf         = (_BYTE *)NULL;
    FTYPE             *fbuf         = (FTYPE *)NULL;

    _BOOLEAN test_hash              = FALSE,
             test_pheaps            = FALSE;


    /*---------------------------------------------------------------*/
    /* Do not allow PSRP clients to connect until we are initialised */
    /*---------------------------------------------------------------*/

    psrp_ignore_requests();


    /*------------------------------------------*/
    /* Get standard items form the command tail */ 
    /*------------------------------------------*/

    pups_std_init(TRUE,
                  &argc,
                  EMBRYO_VERSION,
                  "M.A. O'Neill",
                  "(PSRP) embryo",
                  "2024",
                  (void *)argv);


    (void)pups_register_exit_f("retstartfunc",&restartfunc,(char *)NULL);


    /*-------------------------------------------*/
    /* Get the application to tell us what it is */
    /*-------------------------------------------*/

    if((ptr = pups_locate(&init,"state",&argc,args,0)) != NOT_FOUND)
    {  (void)fprintf(stderr,"    I am a PSRP embryo process, I am waiting to be\n");
       (void)fprintf(stderr,"    told what to do ...\n\n");
       (void)fflush(stderr);

       pups_exit(1);
    }


    /*--------------------------------*/
    /* Test PUPS/P3 hashing functions */
    /*--------------------------------*/

    if((ptr = pups_locate(&init,"hashtest",&argc,args,0)) != NOT_FOUND)
       test_hash = TRUE;


    #ifdef PERSISTENT_HEAP_SUPPORT
    /*----------------------------------------*/
    /* Test PUPS/P3 persistent heap functions */
    /*----------------------------------------*/

    if(pups_locate(&init,"pheaptest",&argc,args,0) != NOT_FOUND)
        test_pheaps = TRUE;
    #endif /* PERSISTENT_HEAP_SUPPORT */


    /*---------------------------------------*/
    /* Complain about any unparsed arguments */
    /*---------------------------------------*/

    pups_t_arg_errs(argd,(void *)args);


    /*------------------------------------------------------------------------------------------------*/
    /* Initialise PSRP function dispatch handler - note that the embryo is fully dynamic and prepared */
    /* to import both dynamic functions and data objects                                              */
    /*------------------------------------------------------------------------------------------------*/

    (void)psrp_init(PSRP_DYNAMIC_FUNCTION | PSRP_STATIC_DATABAG | PSRP_HOMEOSTATIC_STREAMS,NULL);


    /*----------------------------------------------------------------*/
    /* Attach static functions which can be accessed from PSRP client */
    /*----------------------------------------------------------------*/

    (void)psrp_attach_static_function("status",    (void *)&psrp_process_status);
    (void)psrp_attach_static_function("tsic",      (void *)&tsic);
    (void)psrp_attach_static_function("rkill",     (void *)&rkill);
    (void)psrp_attach_static_function("tfps",      &static_test_function_object);
    (void)psrp_attach_static_function("tthread",   (void *)&tthread);
    (void)psrp_attach_static_function("threadfunc",(void *)&threadfunc);
    (void)psrp_attach_static_function("ascii",     (void *)&ascii);
    (void)psrp_attach_static_function("leave",     (void *)&embryo_exit);


    /*------------------------------------------------------------*/
    /* We must define static bindinfgs BEFORE loading the default */
    /* dispatch table. In the case of static bindings, the only   */
    /* effect of loading a saved dispatch table is to (possibly)  */
    /* add object aliases                                         */
    /*------------------------------------------------------------*/

    (void)psrp_load_default_dispatch_table();


    /*----------------------------------------------------------*/
    /* Tell PSRP clients we are ready to service their requests */
    /*----------------------------------------------------------*/

    psrp_accept_requests();


    /*--------------------------------*/
    /* test PUPS/P3 hashing functions */
    /*--------------------------------*/

    if(test_hash == TRUE)
    {  htab = hash_table_create(128,"hash1");

       for(i=0; i<20; ++i)
       {   loc[i] = (int32_t)(ran1()*1000000.0);
           tf = (FTYPE)i + 10;
           (void)hash_put_object(loc[i],(void *)&tf,"FTYPE",sizeof(FTYPE),htab);
       }

       (void)hash_show_stats(stderr, FALSE, htab);
       (void)hash_delete_object(loc[0],htab);
       (void)hash_delete_object(loc[9],htab);

       for(i=0; i<20; ++i)
       {  tf = 0.0;
          if(hash_get_object(loc[i], (void *)&tf, "FTYPE", htab) == 0)
             (void)fprintf(stderr,"TF:%F at %d\n",tf,loc[i]);
          else
             (void)fprintf(stderr,"location %d is empty\n",loc[i]);
       }

       tf = 9999.0;
       (void)hash_put_object(loc[0],(void *)&tf,"FTYPE",sizeof(FTYPE),htab);
       (void)hash_put_object(loc[9],(void *)&tf,"FTYPE",sizeof(FTYPE),htab);

       (void)hash_show_stats(stderr, TRUE, htab);
       hash_table_destroy(htab);


       /*-------------------------------------*/
       /* Wait for exit once test is complete */
       /*-------------------------------------*/

       while(looper == TRUE)
           (void)pups_usleep(1000);

       /*--------------------------------------------------------------------------*/
       /* Exit from PUPS/PSRP application cleaning up any mess it may have created */
       /*--------------------------------------------------------------------------*/

       pups_exit(0);
    }


    #ifdef PERSISTENT_HEAP_SUPPORT
    if(test_pheaps == TRUE)
    {  (void)srand48((uint64_t)getpid()); 

       (void)fprintf(stderr,"Persistent heap test\n");
       (void)fprintf(stderr,"=====================\n\n");
       (void)fflush(stderr);


       /*-----------------------*/
       /* Persistent heap flags */
       /*-----------------------*/

       pheap_flags = CREATE_PHEAP | LIVE_PHEAP;


       /*-------------------------------*/
       /* Lets attach a persistent heap */
       /*-------------------------------*/

       if((hdes1 = msm_heap_attach("heap1",
                                   pheap_flags)) == (-1))
           pups_exit(255);


       /*------------------------------------*/
       /* Lets attach another persistentheap */
       /*------------------------------------*/

       if((hdes2 = msm_heap_attach("heap2",
                                   pheap_flags)) == (-1))
           pups_exit(255);


       /*-------------------------------------------*/
       /* Non existent heap - should generate error */
       /*-------------------------------------------*/
       
       if((hdes3 = msm_heap_attach("heap3",
                                   ATTACH_PHEAP | LIVE_PHEAP)) == (-1))
       {   (void)fprintf(stderr,"\n%sNON EXISTENT HEAP ATTACH ERROR%s\n\n",boldOn,boldOff);
           (void)fflush(stderr);
       }


       /*--------------------------------------*/
       /* Persistent heap 1: allocate object 1 */
       /*--------------------------------------*/

       if(phcalloc(hdes1,PHEAP_SIZE,sizeof(test_type),"object1") != (void *)NULL)
       {  uint32_t i;

          h_ptr_1 = (test_type *)msm_map_objectname2addr(hdes1,"object1");

          for(i=0; i<PHEAP_SIZE; ++i)
          {

             /*-----------------------*/
             /* Initialise the object */
             /*-----------------------*/

             h_ptr_1[i].cnt = 0;
             h_ptr_1[i].x   = 0.0;


             /*--------------------*/
             /* Give object a name */
             /*--------------------*/
 
             (void)snprintf(h_ptr_1[i].name,SSIZE,"object1[%04d]",i);

             (void)fprintf(stderr,"heap1 object1 [%04d]: cnt: %04d x: %6.3F name: (%-24s)\n",
                                                                                           i,
                                                                              h_ptr_1[i].cnt,
                                                                                h_ptr_1[i].x,
                                                                             h_ptr_1[i].name);
             (void)fflush(stderr);
          }

          (void)fprintf(stderr,"\nInitialised heap1 object 1\n");
          (void)fprintf(stderr,"**************************\n\n");
          (void)fflush(stderr);
       }


       /*-------------------------------------*/
       /* Object already exist so just map it */
       /*-------------------------------------*/

       else
          h_ptr_1 = (test_type *)msm_map_objectname2addr(hdes1,"object1");


       /*--------------------------------------*/
       /* Persistent heap 1: allocate object 2 */
       /*--------------------------------------*/

       if(phcalloc(hdes1,PHEAP_SIZE,sizeof(FTYPE),"object2") != (void *)NULL)
       { 

          /*--------------------------------------*/
          /* Pointer to object on persistent heap */
          /*--------------------------------------*/

          h_ptr_2 = (FTYPE *)msm_map_objectname2addr(hdes1,"object2");


          /*-----------------------*/
          /* Initialise the object */
          /*-----------------------*/

          for(i=0; i<PHEAP_SIZE; ++i)
          {  h_ptr_2[o_index2] = 0.0;

                
             (void)fprintf(stderr,"heap1 object2 [%04d]: value: %6.3F\n",
                                                                       i,
                                                              h_ptr_2[i]);
             (void)fflush(stderr);
          }


          (void)fprintf(stderr,"\nInitialised heap1 object 2\n");
          (void)fprintf(stderr,"**************************\n\n");
          (void)fflush(stderr);
       }


       /*-------------------------------------*/
       /* Object already exist so just map it */
       /*-------------------------------------*/

       else
          h_ptr_2 = (FTYPE *)msm_map_objectname2addr(hdes1,"object2");


       /*---------------------------------------*/
       /* Persinstent heap 2: allocate object 3 */
       /*---------------------------------------*/

       if(phcalloc(hdes2,PHEAP_SIZE,sizeof(test_type),"object3") != (void *)NULL)
       {

          h_ptr_3 = (test_type *)msm_map_objectname2addr(hdes2,"object3");


          /*-----------------------*/
          /* Initialise the object */
          /*-----------------------*/

          for(i=0; i<PHEAP_SIZE; ++i)
          {  h_ptr_3[i].cnt = 0;
             h_ptr_3[i].x   = 0.0;


             /*--------------------*/
             /* Give object a name */
             /*--------------------*/

             (void)snprintf(h_ptr_1[i].name,SSIZE,"object1[%04d]",i);

             (void)fprintf(stderr,"heap2 object3 [%04d]: cnt: %04d  x: %6.3F  name: (%-24s)\n",
                                                                                             i,
                                                                                h_ptr_3[i].cnt,
                                                                                  h_ptr_3[i].x,
                                                                               h_ptr_3[i].name);
             (void)fflush(stderr);
          }

          (void)fprintf(stderr,"\nInitialised heap2 object 3\n");
          (void)fprintf(stderr,"**************************\n\n");
          (void)fflush(stderr);
       }


       /*-------------------------------------*/
       /* Object already exist so just map it */
       /*-------------------------------------*/

       else
          h_ptr_3 = (test_type *)msm_map_objectname2addr(hdes2,"object3");


       /*---------------------------------------*/
       /* Persinstent heap 2: allocate object 4 */
       /*---------------------------------------*/

       if(phcalloc(hdes2,PHEAP_SIZE,sizeof(FTYPE),"object4") != (void *)NULL)
       {  

          /*--------------------------------------*/
          /* Pointer to object on persistent heap */
          /*--------------------------------------*/

          h_ptr_4 = (FTYPE *)msm_map_objectname2addr(hdes2,"object4");


          /*-----------------------*/
          /* Initialise the object */
          /*-----------------------*/

          for(i=0; i<PHEAP_SIZE; ++i)
          {  h_ptr_4[i] = 0.0;

             
             (void)fprintf(stderr,"heap2 object4 [%04d]:  value: %6.3F\n",
                                                                        i,
                                                               h_ptr_4[i]);
             (void)fflush(stderr);
          }


          (void)fprintf(stderr,"\nInitialised heap2 object 4\n");
          (void)fprintf(stderr,"**************************\n\n");
          (void)fflush(stderr);
       }


       /*-------------------------------------*/
       /* Object already exist so just map it */
       /*-------------------------------------*/

       else
          h_ptr_4 = (FTYPE *)msm_map_objectname2addr(hdes2,"object4");


       /*---------------------------------------------------------*/
       /* Display the current persistent heap objects and clients */
       /*---------------------------------------------------------*/


       /*--------*/
       /* Heap 1 */
       /*--------*/

       (void)msm_show_mapped_objects(hdes1,stderr);

       (void)fprintf(stderr,"\n**********\n\n");
       (void)fflush(stderr);


       /*--------*/
       /* Heap 2 */
       /*--------*/

       (void)msm_show_mapped_objects(hdes2,stderr);

       (void)fprintf(stderr,"\n**********\n\n");
       (void)fflush(stderr);

       (void)pups_sleep(5);
    }
    #endif /* PERSISTENT_HEAP_SUPPORT */


    /*--------------------------------------------------------------*/
    /* This is the event loop of the embryo -- it may well interact */
    /* with windowed applications or file managers                  */
    /*--------------------------------------------------------------*/


    (void)fprintf(stderr,"\n    %s (%d@%s) is waiting to be allocated an application\n\n",appl_name,getpid(),appl_host);
    (void)fflush(stderr);


    /*--------------------------------*/
    /* Test loop for persistent heaps */
    /*--------------------------------*/

    while(looper == TRUE)
    {    char strdum[256] = "";

         #ifdef PERSISTENT_HEAP_SUPPORT
         if(test_pheaps == TRUE)
         {  

            /*----------------------------------------*/
            /* Get random object indexes within heaps */
            /*----------------------------------------*/

            o_index1 = (int32_t)(drand48() * (FTYPE)(PHEAP_SIZE - 1));
            o_index2 = (int32_t)(drand48() * (FTYPE)(PHEAP_SIZE - 1));
            o_index3 = (int32_t)(drand48() * (FTYPE)(PHEAP_SIZE - 1));
            o_index4 = (int32_t)(drand48() * (FTYPE)(PHEAP_SIZE - 1));


            /*-----------------------*/
            /* Update heap 1 objects */
            /*-----------------------*/

            ++h_ptr_1[o_index1].cnt;
            h_ptr_1[o_index1].x += 0.1;
            h_ptr_2[o_index2]    = h_ptr_1[o_index1].x + 0.5;

            (void)fprintf(stderr,"heap1 object1 [%04d]:  cnt:   %04d  %06.3F\n",o_index1,h_ptr_1[o_index1].cnt,h_ptr_1[o_index1].x);
            (void)fprintf(stderr,"heap1 object2 [%04d]:  value: %06.3F\n\n",    o_index2,h_ptr_2[o_index2]);
            (void)fflush(stderr);

            (void)pups_sleep(1);


            /*-----------------------*/
            /* Update heap 2 objects */
            /*-----------------------*/

            ++h_ptr_3[o_index2].cnt;
            h_ptr_3[o_index3].x += 0.5;
            h_ptr_4[o_index4]    = h_ptr_3[o_index3].x + 0.3;

            (void)fprintf(stderr,"heap2 object1 [%04d]:  cnt:   %04d  %06.3F\n",o_index3,h_ptr_3[o_index3].cnt,h_ptr_3[o_index3].x);
            (void)fprintf(stderr,"heap2 object2 [%04d]:  value: %06.3F\n",      o_index4,h_ptr_4[o_index4]);
            (void)fflush(stderr);

            (void)fprintf(stderr,"\n*************\n\n\n\n");
            (void)fflush(stderr);

            (void)pups_sleep(1);
         }
         else
         {  (void)fprintf(stderr,"iteration %d\n",iter++);
            (void)fflush(stderr);

            (void)pups_sleep(1);
         }
         #endif /* PERSISTENT_HEAP_SUPPORT */
    }

    #ifdef PERSISTENT_HEAP_SUPPORT
    if(test_pheaps == TRUE)
    {

       /*----------------------------------------*/
       /* Detach all (attached) persistent heaps */
       /*----------------------------------------*/
       /*--------*/
       /* Heap 1 */
       /*--------*/

       if(hdes1 != (-1))
       {

          /*------------------------------*/
          /* Detach heap1 (and delete it) */
          /*------------------------------*/
          
          (void)msm_heap_detach(hdes1, O_DESTROY);

          (void)fprintf(stderr,"HEAP1 DETACHED AND DESTROYED\n\n");
          (void)fflush(stderr);
       }


       /*--------*/
       /* Heap 2 */
       /*--------*/

       if(hdes2 != (-1))
       {  

          /*------------------------------------------------*/
          /* Deatch heap2 (and keep it for future mappings) */
          /*------------------------------------------------*/

          (void)msm_heap_detach(hdes2, O_KEEP);

          (void)fprintf(stderr,"HEAP2 DETACHED\n\n");
          (void)fflush(stderr);
       }
    }
    #endif /* PERSISTENT_HEAP_SUPPORT */


    /*--------------------------------------------------------------------------*/
    /* Exit from PUPS/PSRP application cleaning up any mess it may have created */
    /*--------------------------------------------------------------------------*/

    pups_exit(0);
}




#ifdef PTHREAD_SUPPORT
/*-------------------------------------------------*/
/* Createand/or destroy asycnronous (test) threads */
/*-------------------------------------------------*/

_PRIVATE int32_t tthread(int32_t argc, char *argv[])

{   pthread_t tid        = (pthread_t)NULL; 
    char      tname[256] = "test_thread";

    /*--------------------------*/
    /* If we have argument kill */
    /* kill running thread      */
    /*--------------------------*/


    if(argc == 2)
    {  if(strcmp(argv[1],"-usage") == 0    ||
          strcmp(argv[1],"-help" ) == 0     )
       {  (void)fprintf(psrp_out,"\n    Usage: tthread [-help] | [<thread name:test_thread>]\n\n");
          (void)fflush(psrp_out);

          return(PSRP_OK);
       }
       else
          (void)strlcpy(tname,argv[1],SSIZE);
    }


    /*--------------------*/
    /* Too many arguments */
    /*--------------------*/

    else if(argc > 2)
    {  (void)fprintf(psrp_out,"\n    Usage: tthread [-help] | [<thread name:test_thread>]\n\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }


    /*-------------------*/
    /* Start test thread */
    /*-------------------*/

    tid = pupsthread_create(tname,
                            &threadfunc,
                            (void *)NULL);


    (void)fprintf(psrp_out,"    Test thread \"%s\" started\n",tname);
    (void)fflush(psrp_out);

    return(PSRP_OK);
}




/*--------------------------------*/
/* Thread test (payload) function */
/*--------------------------------*/

_PRIVATE int32_t threadfunc(void *args)

{   int32_t     t_index;
    uint32_t    cnt = 0;

    pthread_mutex_t test_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;    

    t_index = pupsthread_tid2nid(pthread_self());

    (void)pthread_mutex_lock(&test_mutex);
    while(TRUE)
    {   uint32_t i;

        (void)pthread_mutex_lock(&ttab_mutex);

        (void)fprintf(stderr,"%s (LWP %d): I am a test thread (iteration %d)\n",ttab[t_index].tfuncname,ttab[t_index].tpid,cnt);
        (void)fflush(stderr);

        (void)pthread_mutex_unlock(&ttab_mutex);

        pups_sleep(1);
        ++cnt;

        if(cnt == 25)
          (void)pupsthread_exit((void *)NULL);

    }

    return(0);
}
#endif /* PTHREAD_SUPPORT */




/*---------------------------------------------------------*/
/* Simple test function to exercise various psrp functions */
/*---------------------------------------------------------*/

_PRIVATE int32_t static_test_function_object(int32_t argc, char *argv[])

{   uint32_t i;

    char command[256] = "",
         item[256]    = "";

    FILE *pipestream = (FILE *)NULL;

    if(argc < 2)
    {  (void)fprintf(psrp_out,"usage: testfps <pipestream>\n");
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    (void)strlcpy(command,"",SSIZE);
    for(i=1; i<argc; ++i)
    {  (void)strlcat(command,argv[i],SSIZE);
       (void)strlcat(command," ",    SSIZE);
    }

    if((pipestream = pups_fopen(command,"r",TRUE)) == (FILE *)NULL)
    {  (void)fprintf(psrp_out,"testfps: failed to open pipestream \"%s\"\n",command);
       (void)fflush(psrp_out);

       return(PSRP_OK);
    }

    (void)psrp_set_client_exitf(c_client,"testfps_client",&testfps_exitf);
    (void)pups_set_ftab_id(fileno(pipestream),c_client);


    /*--------------------------------------*/
    /* Loop to extract data from pipestream */
    /*--------------------------------------*/

    do {   (void)pups_fgets(item,256,pipestream);

           if(feof(pipestream) == 0)
           {  (void)fprintf(psrp_out,"%s\n",item);
              (void)fflush(psrp_out);
           }
       } while(feof(pipestream) == 0);

    (void)pups_fclose(pipestream);
    return(PSRP_OK);
}




/*------------------*/
/* Terminate embryo */
/*------------------*/


_PRIVATE int32_t embryo_exit(int32_t argc, char *argv[])

{   looper = FALSE;

    (void)fprintf(psrp_out,"    Exit ...\n");
    (void)fflush(psrp_out);
    
    return(PSRP_OK);
}




/*-------------------------------------------------------------------*/
/* Test function to send stream of ASDII text to PSRP client process */
/*-------------------------------------------------------------------*/

_PRIVATE int32_t ascii(int32_t argc, char *argv[])

{   uint32_t cnt = 0;

    while(cnt < 100000)
    {    (void)fprintf(psrp_out,"SLEEP %d\n",cnt);
         (void)fflush(psrp_out);

         (void)pups_sleep(1);
         ++cnt;
    }

    return(0);
}
