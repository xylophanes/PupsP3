/*--------------------------------------
    Purpose: Test (fast) caching library 

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 4.19 
    Dated:   10th December 2024 
    E-mail:  mao@tumblingdice.co.uk
-------------------------------------*/

#define TEST_THREADS

#include <me.h>
#include <utils.h>
#include <netlib.h>
#include <psrp.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <vstamp.h>
#include <cache.h>


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

#define CACHE_VERSION    "3.00"

#ifdef BUBBLE_MEMORY_SUPPORT
#include <bubble.h>
#endif /* BUBBLE_MEMORY_SUPPORT */


/*-------------*/
/* Parse flags */
/*-------------*/

// Cache read/write options
#define CACHE_FILE             (1 << 0 )
#define CMAP_FILE              (1 << 1 )
#define CACHE_INIT             (1 << 2 )
#define WCMAP_FILE             (1 << 3 ) 

// Types of cache test
#define CACHE_ACCESS_TEST      (1 << 5 ) 
#define CACHE_CORRELATION_TEST (1 << 6 ) 
#define CACHE_RGB              (1 << 7 ) 
#define CACHE_CWRITE           (1 << 8 ) 

// Cache compression flags
#define ARCHIVE                (1 << 9 ) 
#define EXTRACT                (1 << 10) 
#define COMPRESS               (1 << 11) 
#define DELETE                 (1 << 12) 


/*---------------------------*/
/* Computational state flags */
/*---------------------------*/

#define TEST_PENDING  1
#define TEST_RUNNING  2


/*------------------*/
/* Test definitions */
/*------------------*/

#define CACHE_BLOCKS  5000
#define OBJECTS       3
#define CACHE_INDEX   0
#define OBJECT_INDEX  0
#define ROWS          32
#define COLS          32


/*---------------------------------*/
/* Maximum iterations in soak test */
/*---------------------------------*/

#define DEFAULT_MAXITER    512


/*----------------------------------------*/
/* Default geometric operation parameters */
/*----------------------------------------*/

#define DEFAULT_GT_OPS     COLS / 4
#define DEFAULT_GT_STEP    4




/*----------------------------------------------*/
/* Get application information for slot manager */
/*----------------------------------------------*/
/*---------------------------*/ 
/* Slot information function */
/*---------------------------*/ 

_PRIVATE void cache_slot(int level)
{   (void)fprintf(stderr,"int app (PSRP) cache %s: [ANSI C, PUPS MTD D]\n",CACHE_VERSION);
 
    if(level > 1)
    {  (void)fprintf(stderr,"(C) 2024 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. ONeill\n");
       (void)fprintf(stderr,"(fast) cache test version %s (gcc %s: built %s %s)\n\n",CACHE_VERSION,__VERSION__,__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");

    (void)fflush(stderr);
}
 
 
 

/*----------------------------*/ 
/* Application usage function */
/*----------------------------*/ 

_PRIVATE void cache_usage(void)

{   (void)fprintf(stderr,"[-cache_dir <pathname:.>]\n"); 
    (void)fprintf(stderr,"[-delete_cache:FALSE]\n");
    (void)fprintf(stderr,"[-oneshot:FALSE]\n");
    (void)fprintf(stderr,"[-rgb:FALSE]]\n");
    (void)fprintf(stderr,"[-show_raw:FALSE]\n");
    (void)fprintf(stderr,"[-show_2d:FALSE]\n");
    (void)fprintf(stderr,"[-ctest:FALSE\n");
    (void)fprintf(stderr,"   [-show_correlations:FALSE]\n");
    (void)fprintf(stderr,"   [-gt_ops     <n:%d>]\n",COLS/4);
    (void)fprintf(stderr,"   [-gt_op_step <n:%d>]\n",4);
    (void)fprintf(stderr,"]\n");
    (void)fprintf(stderr,"[-rvalues:FALSE]\n");
    (void)fprintf(stderr,"[-cmap_file   <filename:cache.map> [-extract]                     ] |\n");
    (void)fprintf(stderr,"[-wcmap_file  <filename:cache.map> [-archive [-compress] [-delete]]\n");
    (void)fprintf(stderr,"    [-cache_name   <cache name:cache>  ]\n");
    (void)fprintf(stderr,"    [-cache_file   <filename:cache.dat>]\n");
    (void)fprintf(stderr,"    [-cache_name   <cache name:cache>]\n");
    (void)fprintf(stderr,"    [-cache_blocks <blocks:%d> [-cache_quanta <n:1>]]\n",CACHE_BLOCKS);

     #ifndef DAISY_TEST
    (void)fprintf(stderr,"    [-objects      <objects:%d]\n",OBJECTS);
     #endif /* DAISY_TEST */

    (void)fprintf(stderr,"[-cache_index  <block:%d>]\n", CACHE_INDEX);

     #ifndef DAISY_TEST
    (void)fprintf(stderr,"[-object_index <block:%d>]\n", OBJECT_INDEX);
     #endif /* DAISY_TEST */

    (void)fprintf(stderr,"[-object_rows  <rows:%d>]\n",  ROWS);
    (void)fprintf(stderr,"[-object_cols  <rows:%d>]\n",  COLS);
    (void)fflush(stderr);

    (void)fprintf(stderr,"[>& <ASCII log file>]\n\n");

    (void)fprintf(stderr,"Signals\n\n");
    (void)fprintf(stderr,"SIGINIT SIGCHAN SIGPSRP: Process status [PSRP] request (protocol %5.2fF)\n",PSRP_PROTOCOL_VERSION);
    (void)fprintf(stderr,"SIGCLIENT: tell client server is about to segment\n");

     #ifdef CRUI_SUPPORT
    (void)fprintf(stderr,"SIGCHECK SIGRESTART:      checkpoint and restart signals\n");
     #endif /* CRIU_SUPPORT */

    (void)fprintf(stderr,"SIGALIVE: check for existence of client on signal dispatch host\n");
    (void)fflush(stderr);
}




#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT)() __attribute__ ((aligned(16))) = cache_slot;
_EXTERN void (* USE )() __attribute__ ((aligned(16))) = cache_usage;
#endif /* SLOT */




/*------------------------*/
/* Application build date */
/*------------------------*/

_EXTERN char appl_build_time[SSIZE] = __TIME__;
_EXTERN char appl_build_date[SSIZE] = __DATE__;




/*-------------------------------------------------*/
/* Functions which are private to this application */
/*-------------------------------------------------*/


/* Cache correlation test */
_PROTOTYPE _PRIVATE void cache_correlation_test(_BOOLEAN, _BOOLEAN, int32_t);

/* Cache access test */
_PROTOTYPE _PRIVATE void cache_access_test(_BOOLEAN, int32_t, int32_t);




/*-------------------------------------------------*/
/* Variables which are private to this application */
/*-------------------------------------------------*/
                                                             /*----------------------------------------*/ 
_PRIVATE char     cache_dir_name[SSIZE]  = ".";              /* Cache directory                        */
_PRIVATE char     cache_file_name[SSIZE] = "";               /* Cache (memory mapping) file name       */
_PRIVATE char     cmap_file_name[SSIZE]  = "";               /* Cache map info file mame               */
_PRIVATE char     wcmap_file_name[SSIZE] = "";               /* Cache map info dump file mame          */
_PRIVATE char     cache_name[SSIZE]      = "cache";          /* Cache name                             */
_PRIVATE int32_t      parse_flags        = 0;                /* Command tail parsing flags             */
_PRIVATE int32_t      cache_blocks       = CACHE_BLOCKS;     /* Number of data block in cache          */
_PRIVATE int32_t      cache_n_quanta     = 1;                /* Number of quanta in cache              */
_PRIVATE int32_t      objects            = OBJECTS;          /* Number of object in each data block    */
_PRIVATE int32_t      object_rows        = ROWS;             /* Row in simulated image matrix          */
_PRIVATE int32_t      object_cols        = COLS;             /* Columns in simulated image matrix      */
_PRIVATE int32_t      cache_index        = CACHE_INDEX;      /* Cache index (block to manipulate       */
_PRIVATE int32_t      object_index       = OBJECT_INDEX;     /* Object index (object to manipulate     */ 
_PRIVATE int32_t      gt_ops             = DEFAULT_GT_OPS;   /* Simulated geometric transform ops      */
_PRIVATE int32_t      gt_op_step         = DEFAULT_GT_STEP;  /* Simulated geometric transform ops      */
_PRIVATE FTYPE    *flinarr               = (FTYPE *) NULL;   /* Pointer to simulated data in cache     */
_PRIVATE FTYPE    **flin2Darr            = (float **)NULL;   /* Pointer to data 2D mapping array       */
                                                             /*----------------------------------------*/ 

#ifdef DAISY_TEST
                                                             /*----------------------------------------*/ 
_PRIVATE char     *mask                  = (char *)NULL;     /* Pointer to mask data                   */
_PRIVATE char     **mask2Darr            = (char **) NULL;   /* Pointer to mask 2D mapping array       */
                                                             /*----------------------------------------*/ 
#endif /* DAISY_TEST */

                                                             /*----------------------------------------*/ 
_PRIVATE _BOOLEAN do_delete_cache        = FALSE;            /* If TRUE delete existing cache file     */
_PRIVATE  int32_t state_flags            = 0;                /* Show current computational state       */
_PRIVATE  int32_t iter                   = 0;                /* Iteration counter (for soak testing)   */
_PRIVATE  int32_t maxiter                = DEFAULT_MAXITER;  /* Max iterations (for soack testing)     */
_PRIVATE _BOOLEAN raw_object_output      = FALSE;            /* Display raw object ouput if TRUE       */
_PRIVATE _BOOLEAN mapped_object_output   = FALSE;            /* Display 2D mapped object ouput if TRUE */
                                                             /*----------------------------------------*/ 





/*--------------------------------------------------------------------------*/
/* Software I.D. tag (used if CKPT support enabled to discard stale dynamic */
/* checkpoint files)                                                        */
/*--------------------------------------------------------------------------*/

#define VTAG   598
extern  int32_t appl_vtag = VTAG;




/*------------------*/
/* Main entry point */
/*------------------*/

_PUBLIC int32_t pups_main(int32_t argc, char *argv[])

{   _BOOLEAN  looper = FALSE;
    uint32_t  i;

    /*------------------------------*/
    /* Seed random number generator */
    /*------------------------------*/

    (void)srand48((uint64_t)getpid());


    /*---------------------------------------------------------------*/
    /* Do not allow PSRP clients to connect until we are initialised */
    /*---------------------------------------------------------------*/

    psrp_ignore_requests();


   /*------------------------------------------*/
   /* Get standard items form the command tail */
   /*------------------------------------------*/

    pups_std_init(TRUE,
                  &argc,
                  CACHE_VERSION,
                  "M.A. O'Neill",
                  "cache",
                  "2024",
                  argv);


    /*--------------------*/
    /* Parse command line */
    /*--------------------*/
    /*---------------------*/
    /* Set cache directory */
    /*---------------------*/

    if((ptr = pups_locate(&init,"cache_dir",&argc,args,0)) != NOT_FOUND)
    {  if(strccpy(cache_dir_name,pups_str_dec(&ptr,&argc,args)) == (char *)INVALID_ARG)
          pups_error("[cache main] cache directory name expected");


        /*------------------------------------------*/
        /* Check to see if cache directory is valid */
        /*------------------------------------------*/

        if(access(cache_dir_name,F_OK | R_OK | W_OK) == (-1) || chdir(cache_dir_name) == (-1))
        {  (void)snprintf(errstr,SSIZE,"[cache] cannot change directory to",cache_dir_name); 
           pups_error(errstr);
        }
    }

    (void)fprintf(stderr,"cache directory is: \"%s\"\n",cache_dir_name);
    (void)fflush(stderr);


    /*------------------------------------------------*/   
    /* Delete any existing  cache before running test */
    /*------------------------------------------------*/   

    if(pups_locate(&init,"delete_cache",&argc,args,0) != NOT_FOUND)
    {  do_delete_cache = TRUE; 

       if(appl_verbose == TRUE)
       {  (void)printf(stderr,"existing cache file will be deleted\n");
          (void)fflush(stderr);
       } 
    }


    /*--------------------------------*/
    /* Perform one shot test and exit */
    /*--------------------------------*/
 
    if(pups_locate(&init,"oneshot",&argc,args,0) != NOT_FOUND)
    {  looper = FALSE; 

       if(appl_verbose == TRUE)
       {  (void)printf(stderr,"performing one shot cache test\n");
          (void)fflush(stderr);
       } 
    }


    /*------------------------------*/
    /* Set cache simulated RGB mode */
    /*------------------------------*/

    if(pups_locate(&init,"rgb",&argc,args,0) != NOT_FOUND)
       parse_flags |= CACHE_RGB;


    #ifdef DAISY_TEST
    /*--------------------------------*/
    /* Do simulated) correlation test */
    /*--------------------------------*/

    if(pups_locate(&init,"ctest",&argc,args,0) != NOT_FOUND)
    {  parse_flags |= CACHE_CORRELATION_TEST;

       if(appl_verbose == TRUE)
       {  if(parse_flags & CACHE_RGB)
             (void)fprintf(stderr,"doing simulated RGB correlation test\n");
          else
             (void)fprintf(stderr,"doing simulated monochrome correlation test\n");
          (void)fflush(stderr);
       }

       if(pups_locate(&init,"show_correlations",&argc,args,0) != NOT_FOUND)
           parse_flags |= CACHE_CWRITE;


       /*------------------------------------*/
       /* Set number of geometric operations */
       /*------------------------------------*/

       if((ptr = pups_locate(&init,"gt_ops",&argc,args,0)) != NOT_FOUND)
       {  if((gt_ops = pups_i_dec(&ptr,&argc,args)) == (int32_t)INVALID_ARG || gt_ops < 0)
             pups_error("main] number of geomertic operations [>=0] expected");

          if(appl_verbose == TRUE)
          {  (void)fprintf(stderr,"performing %d geometric operations per correlation\n",gt_ops);
             (void)fflush(stderr);
          }
       }

       
       /*------------------------------------*/
       /* Set number of geometric operations */
       /*------------------------------------*/
       
       if((ptr = pups_locate(&init,"gt_op_step",&argc,args,0)) != NOT_FOUND)
       {  if((gt_op_step = pups_i_dec(&ptr,&argc,args)) == (int32_t)INVALID_ARG || gt_op_step < 0)
             pups_error("[cache] geometric operation step [>=0] expected");
          
          if(appl_verbose == TRUE)
          {  (void)fprintf(stderr,"geometric operation step is %d\n",gt_op_step);
             (void)fflush(stderr);
          }
       }
    }
    else
    #endif /* DAISY_TEST */
       parse_flags |= CACHE_ACCESS_TEST;


    /*--------------------------------------------*/
    /* File to write cache mapping information to */
    /*--------------------------------------------*/

    if((ptr = pups_locate(&init,"cmap_file",&argc,args,0)) != NOT_FOUND)
    {  parse_flags |= CMAP_FILE;

       if(strccpy(cmap_file_name,pups_str_dec(&ptr,&argc,args)) == (char *)INVALID_ARG)
          pups_error("[cache] cache mapping info name expected");
       else if(appl_verbose == TRUE)
       {  (void)fprintf(stderr,"reading cache mapping info file: \"%s\"\n",cmap_file_name);
          (void)fflush(stderr);
       }


       /*----------------------------*/
       /* Are we going to uncompress */
       /* cache archive?             */
       /*----------------------------*/
    
       if(pups_locate(&init,"extract",&argc,args,0) != NOT_FOUND)
       {  parse_flags |= EXTRACT;
    
          if(appl_verbose == TRUE)
          {  (void)fprintf(stderr,"extracting cache\n");
             (void)fflush(stderr);
          }
       }
    }
    else
    {

       /*----------------------------------------------*/        
       /* Cache file (which will be used to map cache) */ 
       /*----------------------------------------------*/        

       if((ptr = pups_locate(&init,"cache_file",&argc,args,0)) != NOT_FOUND)
       {  parse_flags |= CACHE_FILE;

         if(strccpy(cache_file_name,pups_str_dec(&ptr,&argc,args)) == (char *)INVALID_ARG)
            pups_error("[cache] cache file name expected");
         else if(appl_verbose == TRUE)
         {  (void)fprintf(stderr,"cache file (for memory mapping) is: \"%s\"\n",cache_file_name);
            (void)fflush(stderr);
         }
       }
    }

    /*-------------------------------*/
    /* File to write copy of heap to */
    /*-------------------------------*/

    if(pups_locate(&init,"rvalues",&argc,args,0) != NOT_FOUND)
    {  parse_flags |= CACHE_INIT;

       (void)fprintf(stderr,"initialising (local heap) cache before writing\n");
       (void)fflush(stderr);
    }


    /*--------------------------------------------*/
    /* File to write cache mapping information to */
    /*--------------------------------------------*/

    if((ptr = pups_locate(&init,"wcmap_file",&argc,args,0)) != NOT_FOUND)
    {  parse_flags |= WCMAP_FILE;

       if(strccpy(wcmap_file_name,pups_str_dec(&ptr,&argc,args)) == (char *)INVALID_ARG)
          pups_error("[cache] cache mapping info name expected");
       else if(appl_verbose == TRUE)
       {  (void)fprintf(stderr,"cache mapping info file: \"%s\"\n",wcmap_file_name);
          (void)fflush(stderr);
       }


       /*--------------------------------------*/
       /* Are we going to archive cache?       */
       /*--------------------------------------*/

       if(pups_locate(&init,"archive",&argc,args,0) != NOT_FOUND)
       {  parse_flags |= ARCHIVE;


          /*------------------*/ 
          /* Compress archive */
          /*------------------*/ 

          if(pups_locate(&init,"compress",&argc,args,0) != NOT_FOUND)
             parse_flags |= COMPRESS;

          if(appl_verbose == TRUE)
          {  if(parse_flags & COMPRESS)
               (void)fprintf(stderr,"archiving and compressing cache\n");
             else
               (void)fprintf(stderr,"archiving cache\n");
             (void)fflush(stderr);
          }


          /*----------------------------------------*/
          /* Delete cache and cachemap one archived */
          /*----------------------------------------*/

          if(pups_locate(&init,"delete",&argc,args,0) != NOT_FOUND)
          {  parse_flags |= DELETE;

             if(appl_verbose == TRUE)
             {  (void)fprintf(stderr,"deleting cache and cachemap (after archiving them)\n");
                (void)fflush(stderr);
             }
          }
       }
    }


    if(!(parse_flags & CMAP_FILE))
    {

       /*----------------*/
       /* Set cache name */
       /*----------------*/

       if((ptr = pups_locate(&init,"cache_name",&argc,args,0)) != NOT_FOUND)
       {  if(strccpy(cache_name,pups_str_dec(&ptr,&argc,args)) == (char *)INVALID_ARG)
             pups_error("[cache] cache name expected");
          else if(appl_verbose == TRUE)
          {  (void)fprintf(stderr,"cache name is: \"%s\"\n",cache_name);
             (void)fflush(stderr);
          }
       }


       /*------------------*/
       /* Set cache blocks */
       /*------------------*/

       if((ptr = pups_locate(&init,"cache_blocks",&argc,args,0)) != NOT_FOUND)
       {  if((cache_blocks = pups_i_dec(&ptr,&argc,args)) == (int32_t)INVALID_ARG)
             pups_error("[cache] number of blccks in cache [>0] expected");
          else if(appl_verbose == TRUE && cache_blocks > 0)
          {  (void)fprintf(stderr,"number of blocks in cache is: %d\n",cache_blocks);
             (void)fflush(stderr);
          }
          else if(cache_blocks <= 0)
             pups_error("[cache] number of blocks in cache [>0] expected");


          /*------------------------------------*/
          /* Set number of quanta. This ootion  */
          /* is used when testing dynamic cache */
          /* resizing.                          */
          /*------------------------------------*/

          if((ptr = pups_locate(&init,"cache_quanta",&argc,args,0)) != NOT_FOUND)
          {  if((cache_n_quanta = pups_i_dec(&ptr,&argc,args)) == (int)INVALID_ARG)
                pups_error("[cache] number of (cache) quanta [>0] expected");
             else if(appl_verbose == TRUE && cache_n_quanta > 0)
             {  (void)fprintf(stderr,"number of cache quanta is: %d\n",cache_n_quanta);
                (void)fflush(stderr);
             }
             else if(cache_blocks <= 0)
                pups_error("number of cache quanta [>0] expected");
          }
       }


       #ifndef DAISY_TEST
       /*-----------------------------*/
       /* Set objects per cache block */
       /*-----------------------------*/

       if((parse_flags & CACHE_ACCESS_TEST) && (ptr = pups_locate(&init,"objects",&argc,args,0)) != NOT_FOUND)
       {  if((objects = pups_i_dec(&ptr,&argc,args)) == (int32_t)INVALID_ARG)
             pups_error("[cache] number of blocks in cache [>0] expected");
          else if(appl_verbose == TRUE && objects > 0)
          {  (void)fprintf(stderr,"number of objects per cache block is: %d\n",objects);
             (void)fflush(stderr);
          }
          else if(objects <= 0)
          {  (void)snprintf(errstr,SSIZE,"number of objects per cache black [2-%d] expected",MAX_CACHE_BLOCK_OBJECTS);
             pups_error(errstr);
          }
       }
       #endif /* DAISY_TEST */
    }


    /*-----------------*/
    /* Set cache index */
    /*-----------------*/

    if((ptr = pups_locate(&init,"cache_index",&argc,args,0)) != NOT_FOUND)
    {  if((cache_index = pups_i_dec(&ptr,&argc,args)) == (int32_t)INVALID_ARG)
          pups_error("[cache] cache index [>=0] expected");
       else if(appl_verbose == TRUE && cache_index >= 0)
       {  (void)fprintf(stderr,"cache index is: %d\n",cache_index);
          (void)fflush(stderr);
       }
       else  if(cache_index < 0)
       {  (void)fprintf(stderr,"randomly testing %d cache blocks\n",abs(cache_index));
          (void)fflush(stderr);
       }
    }


    #ifndef DAISY_TEST
    /*------------------*/
    /* Set object index */
    /*------------------*/

    if((parse_flags & CACHE_ACCESS_TEST) && (ptr = pups_locate(&init,"object_index",&argc,args,0)) != NOT_FOUND)
    {  if((object_index = pups_i_dec(&ptr,&argc,args)) == (int32_t)INVALID_ARG)
          pups_error("[cache] object index [>= 0] expected");
       else if(appl_verbose == TRUE && object_index >= 0)
       {  (void)fprintf(psrp_out,"object index is: %d\n",object_index);
          (void)fflush(psrp_out);
       }
       else if(object_index < 0)
       {  (void)fprintf(stderr,"randomly testing %d object indexes  int32_to each cache block\n",abs(object_index));
          (void)fflush(stderr);
       }
    }
    #endif /* DAISY_TEST */


    /*----------------------------------------------*/
    /* Set object rows - simulates a matrix of data */
    /*----------------------------------------------*/

    if((ptr = pups_locate(&init,"object_rows",&argc,args,0)) != NOT_FOUND)
    {  if((object_rows = pups_i_dec(&ptr,&argc,args)) == (int)INVALID_ARG)
          pups_error("[cache] object rows [>= 1] expected");
       else if(appl_verbose == TRUE && object_rows >= 1)
       {  (void)fprintf(stderr,"object rows: \"%d\"\n",object_index);
          (void)fflush(stderr);
       }
       else if(object_rows <= 0)
          pups_error("[cache] object rows [>= 1] expected");
    }


    /*----------------------------------------------*/
    /* Set object rows - simulates a matrix of data */
    /*----------------------------------------------*/

    if((ptr = pups_locate(&init,"object_cols",&argc,args,0)) != NOT_FOUND)
    {  if((object_cols = pups_i_dec(&ptr,&argc,args)) == (int)INVALID_ARG)
          pups_error("[cache] object cols [>= 1] expected");
       else if(appl_verbose == TRUE && object_cols >= 1)
       {  (void)fprintf(stderr,"object cols: \"%d\"\n",object_index);
          (void)fflush(stderr);
       }
       else if(object_cols <= 0)
          pups_error("[cache] object cols [>= 1] expected");
    }


    /*------------------------*/
    /* Show raw object output */
    /*------------------------*/

    if((parse_flags & CACHE_ACCESS_TEST) && pups_locate(&init,"show_raw",&argc,args,0) != NOT_FOUND)
    {  raw_object_output = TRUE;

       if(appl_verbose == TRUE)
       {  (void)fprintf(stderr,"displaying raw object output\n");
          (void)fflush(stderr);
       }
    }


    /*------------------------------*/
    /* Show 2D mapped object output */
    /*------------------------------*/

    if((parse_flags & CACHE_ACCESS_TEST) && pups_locate(&init,"show_2d",&argc,args,0) != NOT_FOUND)
    {  mapped_object_output = TRUE;

       if(appl_verbose == TRUE)
       {  (void)fprintf(stderr,"displaying 2D mapped object output\n");
          (void)fflush(stderr);
       }
    }


    /*-------------------------------------------*/
    /* Check command tail for unparsed arguments */
    /*-------------------------------------------*/

    (void)pups_t_arg_errs(argc,argd,args);


    /*---------------*/
    /* Sanity checks */
    /*---------------*/

    if(! parse_flags & CMAP_FILE && cache_index >= cache_blocks)
    {  (void)snprintf(errstr,SSIZE,"[cache] cache index [%d] >= cache blocks[%d]\n",cache_index,cache_blocks);
       pups_error(errstr);
    }
    else if(object_index > objects)
    {  (void)snprintf(errstr,SSIZE,"[cache] object index [%d] >= object[%d]\n",object_index,objects);
       pups_error(errstr);
    }

    if((parse_flags & CACHE_INIT) && cache_index < 0 || object_index < 0)
       pups_error("[cache] cannot initialise cache in iterative test mode");


    /*-------------------------------------------*/
    /* Initialise PSRP function dispatch handler */
    /*-------------------------------------------*/

    (void)psrp_init(PSRP_STATIC_DATABAG | PSRP_HOMEOSTATIC_STREAMS,NULL);


    /*------------------------------------------------------------*/
    /* We must define static bindings BEFORE loading the default */
    /* dispatch table. In the case of static bindings, the only   */
    /* effect of loading a saved dispatch table is to (possibly)  */
    /* add object aliases                                         */
    /*------------------------------------------------------------*/

    (void)psrp_load_default_dispatch_table(stderr);


    /*----------------------------------------------------------*/
    /* Tell PSRP clients we are ready to service their requests */
    /*----------------------------------------------------------*/

    psrp_accept_requests();


    /*----------*/
    /* Run test */
    /*----------*/

    do {     int32_t c_maxiter = 1,
                o_maxiter = 1; 

            _BOOLEAN do_pending = FALSE;

            while(state_flags & TEST_PENDING)
            {    if(do_pending == FALSE)
                 {  do_pending = TRUE; 

                    (void)fprintf(stderr,"    Next test pending\n");
                    (void)fflush(stderr);
                 }

                 (void)pups_usleep(100);
            }
            do_pending = FALSE;


            /*-------------------------------------*/
            /* Check to see if we are soak testing */
            /*-------------------------------------*/

            if(cache_index < 0)
               c_maxiter = abs(cache_index);

            if(object_index < 0)
               o_maxiter = abs(object_index);

            maxiter = o_maxiter*c_maxiter;

            if(maxiter == 1)
            {

               if(parse_flags & CACHE_ACCESS_TEST)
               {

                  /*-----------------------------*/
                  /* Do single cache access test */
                  /*-----------------------------*/

                  if(strcmp(cache_file_name,"") != 0)   
                     (void)fprintf(stdout,"\n    Cache is memory mapped (from file \"%s\")\n",cache_file_name);
                  (void)fflush(stdout);

                  if(parse_flags & CACHE_RGB)
                  {  if(parse_flags & WCMAP_FILE)
                        (void)fprintf(stdout,"    Writing RGB cache to \"%s\" ((mapinfo saved to: \"%s\")\n",cache_file_name,wcmap_file_name);
                     cache_access_test(TRUE,cache_index,object_index);
                  }
                  else
                  {  if(parse_flags & WCMAP_FILE)
                        (void)fprintf(stdout,"\n\n    Writing monochrome cache to \"%s\" ((mapinfo saved to: \"%s\")\n",cache_file_name,wcmap_file_name);
                     cache_access_test(FALSE,cache_index,object_index);
                  }
               }

               #ifdef DAISY_TEST
               else if(parse_flags & CACHE_CORRELATION_TEST)
               {

                  /*----------------------------------*/
                  /* Do single cache correlation test */
                  /*----------------------------------*/

                  if(parse_flags & CACHE_RGB)
                  {  if(parse_flags & CMAP_FILE)
                        (void)fprintf(stdout,"\n\n    One shot RGB cache correlation test (reading mapinfo from file \"%s\")\n",cmap_file_name);
                     else
                        (void)fprintf(stdout,"\n\n    One shot RGB cache correlation test\n");
                     (void)fflush(stderr);

                     (void)cache_correlation_test(FALSE,TRUE, cache_index);
                  }
                  else
                  {  if(parse_flags & CMAP_FILE)
                        (void)fprintf(stdout,"\n\n    One shot monochrome cache correlation test (reading mpainfo from file \"%s\")\n",cmap_file_name);
                     else
                        (void)fprintf(stdout,"\n\n    One shot monochrome cache correlation test\n");
                     (void)fflush(stderr);

                     (void)cache_correlation_test(FALSE,FALSE,cache_index);
                  }
               }
               #endif /* DAISY_TEST */

               else
                  pups_error("[cache] unkown test option");
            }
            else
            {

               /*------------------------*/
               /* Do iterative soak test */
               /*------------------------*/

                int32_t eff_cache_index,
                        eff_object_index; 

               if(parse_flags & CACHE_ACCESS_TEST)
               {  if(parse_flags & CACHE_RGB)
                  {  if(parse_flags & CMAP_FILE)
                        (void)fprintf(stdout,"\n\n    One shot RGB cache access test (reading mapfrom file \"%s\")\n",cmap_file_name);
                     else
                        (void)fprintf(stdout,"\n\n    One shot RGB cache access test\n");
                  }
                  else
                  {  if(parse_flags & CMAP_FILE)
                        (void)fprintf(stdout,"\n\n    One shot monochrome cache access test (reading mapinfo from file \"%s\")\n",cmap_file_name);
                     else
                        (void)fprintf(stdout,"\n\n    One shot monochrome cache access test\n");
                  }
                  (void)fflush(stderr);
               }

               #ifdef DAISY_TEST
               else if(parse_flags & CACHE_CORRELATION_TEST)
               {   if(parse_flags & CACHE_RGB)
                   {  if(parse_flags & CMAP_FILE)
                         (void)fprintf(stdout,"\n\n    One shot cache RGB correlation test (reading mapinfo from file \"%s\")\n",cmap_file_name);
                      else
                         (void)fprintf(stdout,"\n\n    One shot cache RGB correlation test\n");
                      (void)fflush(stderr);
                   }
                   else
                   {  if(parse_flags & CMAP_FILE)
                         (void)fprintf(stdout,"\n\n    One shot cache monochrome correlation test (reading mapinfo from file \"%s\")\n",cmap_file_name);
                      else
                         (void)fprintf(stdout,"\n\n    One shot cache monochrome correlation test\n");
                      (void)fflush(stderr);
                   }
               }
               #endif /* DAISY_TEST */

               else
                  pups_error("[cache] unkown test option");

               while(iter < maxiter) 
               {  
                    if(parse_flags & CACHE_ACCESS_TEST) 
                    {  if(o_maxiter > 1)
                          eff_object_index = (int32_t)(drand48()*(double)objects);
                       else
                          eff_object_index = object_index;

                       if(c_maxiter > 1)               
                          eff_cache_index = (int32_t)(drand48()*(double)cache_blocks);
                       else
                          eff_cache_index = cache_index;

                       (void)fprintf(stdout,"\n\n    Cache access test iteration %04d (of %04d)\n",iter + 1,maxiter);
                       (void)fflush(stderr);

                       if(parse_flags & CACHE_RGB)
                          cache_access_test(TRUE,eff_cache_index,eff_object_index);
                       else
                          cache_access_test(FALSE,eff_cache_index,eff_object_index);
                    }

                    #ifdef DAISY_TEST
                    else
                    {  eff_cache_index = (int)(drand48()*(double)cache_blocks);
                       eff_cache_index = cache_index;


                       if(parse_flags & CACHE_RGB)
                       {  (void)fprintf(stdout,"\n\n    Cache (RGB) correlation test iteration %04d (of %04d)\n",iter + 1,maxiter);
                          (void)fflush(stderr);

                          (void)cache_correlation_test(TRUE,TRUE, eff_cache_index);
                       }
                       else
                       {  (void)fprintf(stdout,"\n\n    Cache (monochrome) correlation test iteration %04d (of %04d)\n",iter + 1,maxiter);
                          (void)fflush(stderr);

                          (void)cache_correlation_test(TRUE,FALSE,eff_cache_index);
                       }
                    }
                    #endif /* DAISY_TEST */

                    ++iter;
               }
            }
    } while(looper == TRUE);


    /*--------------------------------------------------------------------------*/
    /* Exit from PUPS/PSRP application cleaning up any mess it may have created */
    /*--------------------------------------------------------------------------*/

    pups_exit(0);
}



#ifdef DAISY_TEST
/*---------------------------*/
/* Simulate DAISY correlator */
/*---------------------------*/

                                                                /*-----------------------------*/
_PRIVATE FTYPE simulate_DAISY_correlation(_BOOLEAN  do_rgb,     /* TRUE if simulating RGB mode */
                                          char      *mask,      /* NVD chip mask               */
                                          FTYPE     *nvd_1,     /* Simulated NVD image chip #1 */
                                          FTYPE     *nvd_2)     /* Simulated NVD image chip #2 */
                                                                /*-----------------------------*/

{   int32_t k;
    FTYPE   min_sum = 0.0;


    /*---------------------------------*/
    /* Simulate geometrical operations */
    /*---------------------------------*/

    for(k=0; k<gt_ops*gt_op_step; k+=gt_op_step)
    {  int32_t i;
       FTYPE   sum = 0.0;

       for(i=0; i<object_rows; ++i)
       {   uint32_t j;

           if(do_rgb == TRUE)
           {  for(j=0; j<object_cols; ++j)
              {    int32_t p_index_1,
                           p_index_2,
                           p_index_2_r,
                           band,
                           eff_j;

                  FTYPE delta = 0.0;

                  eff_j       = (j + gt_ops) % object_cols;
                  p_index_1   = object_cols*i + j;
                  p_index_2   = object_cols*i + eff_j;
                  p_index_2_r = object_cols   - p_index_2;

                  if(mask[p_index_2] == 'T')
                  {  p_index_1 = 3*(object_rows*i + j);
                     p_index_2 = 3*(object_rows*i + eff_j);


                     /*------------------------*/
                     /* Forward rotational arc */
                     /*------------------------*/

                     for(band =0; band < 3; ++band)
                     {

                        /*------------------------*/
                        /* Forward rotational arc */
                        /*------------------------*/

                        if(nvd_2[p_index_2 + band] > 0.0 || nvd_2[p_index_1 + band] > 0.0)
                        {

                            #ifdef POW
                            delta = FABS(POW(nvd_2[p_index_2 + band],2) - POW(nvd_1[p_index_1 + band],2));
                            #else
                            delta = FABS(sqr(nvd_2[p_index_2 + band])   - sqr(nvd_1[p_index_1 + band]));
                            #endif /* POW */
                        }

                        /*------------------------------------*/
                        /* Reverse (reflected) rotational arc */
                        /*------------------------------------*/

                        if(k > 0 && (nvd_2[p_index_2 + band] > 0.0 || nvd_2[p_index_1 + band] > 0.0))
                        {
                           #ifdef POW
                           delta += FABS(POW(nvd_2[p_index_2_r + band],2) - POW(nvd_1[p_index_1 + band],2));
                           #else
                           delta += FABS(sqr(nvd_2[p_index_2_r + band])   - sqr(nvd_1[p_index_1 + band]));
                           #endif /* POW */
                        }

                        sum += delta;
                     }
                  }
              }
           }
           else
           {  for(j=0; j<object_cols; ++j) 
              {    int32_t p_index_1,
                           p_index_2,
                           p_index_2_r,
                           eff_j;

                  FTYPE delta = 0.0;

                  eff_j       = (j + gt_ops) % object_cols;
                  p_index_1   = object_rows*i + j;
                  p_index_2   = object_rows*i + eff_j;
                  p_index_2_r = object_cols   - p_index_2;

                  if(mask[p_index_2] == 'T')
                  {  

                     /*------------------------*/
                     /* Forward rotational arc */
                     /*------------------------*/

                     if(nvd_2[p_index_1] > 0.0 || nvd_2[p_index_2] > 0.0)
                        delta = FABS(POW(nvd_2[p_index_2],2) - POW(nvd_1[p_index_1],2));


                     /*------------------------------------*/
                     /* Reverse (reflected) rotational arc */
                     /*------------------------------------*/

                     if(k > 0 && (nvd_2[p_index_1] > 0.0 || nvd_2[p_index_2] > 0.0))
                        delta += FABS(POW(nvd_2[p_index_2_r],2) - POW(nvd_1[p_index_1],2));

                     sum  += delta;
                  }
               }
           }

           if(min_sum > sum || min_sum == 0.0)
              min_sum = sum;
       }
    }

    min_sum = sqrtf(min_sum);

#ifdef CACHE_DEBUG
(void)fprintf(stderr,"SUM: %f\n",min_sum);
(void)fflush(stderr);
#endif /* CACHE_DEBUG */

    return(min_sum);

}




/*--------------------------*/
/* Used by correlation test */
/*--------------------------*/

typedef struct {    int32_t index;
                    FTYPE   affinity;
               } sum_type;



/*--------------------*/
/* Compare affinities */
/*--------------------*/

_PRIVATE int32_t compare_affinities(sum_type *s_1, sum_type *s_2)

{   if(s_1->affinity > s_2->affinity)
       return(1);
    else if(s_1->affinity < s_2->affinity)
       return(-1);

    return(0);
}




/*-----------------------------------*/
/* Run (fast) cache correlation test */
/*-----------------------------------*/
                                                              /*------------------------------------*/
_PRIVATE void cache_correlation_test(_BOOLEAN   iterative,    /* TRUE if iterative test             */
                                     _BOOLEAN      do_rgb,    /* TRUE if simulating RGB correlation */
                                      int32_t cache_index)    /* Cache index of "unknown" image     */
                                                              /*------------------------------------*/

{    int32_t i,
             c_index,
             n_blocks,
             n_omp_threads = 1;

    double start_time,
           end_time,
           delta;

    _IMMORTAL  int32_t sched_cnt = 0;
    _IMMORTAL double   old_delta = 0.0;

    FTYPE    *nvd_1 = (FTYPE *)NULL;
    sum_type *sum   = (sum_type *)NULL;


    /*------------------------*/
    /* Initialise cache table */
    /*------------------------*/

    (void)cache_table_init();


    /*------------------*/
    /* Initialise cache */
    /*------------------*/

    if(parse_flags & CMAP_FILE)
    {  c_index = cache_init_cache(); 

       /*-------------------------------*/
       /* Uncompress cache if requested */
       /*-------------------------------*/

       if(parse_flags & EXTRACT)
         (void)cache_extract(cmap_file_name);

       (void)cache_read_mapinfo(cmap_file_name,c_index);
       (void)cache_create(FALSE,"",CACHE_USING_MAPINFO,c_index);
    }
    else
    {  c_index = cache_init_cache();


       /*---------------------*/
       /* Add object to cache */
       /*---------------------*/

       #ifndef DAISY_TEST
       if(do_rgb == TRUE)
       {  for(i=0; i<objects; ++i)
              (void)cache_add_object(3*object_rows*object_cols*sizeof(FTYPE),c_index);
       }
       else
       {  for(i=0; i<objects; ++i)
              (void)cache_add_object(object_rows*object_cols*sizeof(FTYPE),c_index);
       }
       #else
       if(do_rgb == TRUE)
          (void)cache_add_object(3*object_rows*object_cols*sizeof(FTYPE),c_index);
       else
          (void)cache_add_object(object_rows*object_cols*sizeof(FTYPE),c_index);

       (void)cache_add_object(object_rows*object_cols*sizeof(char),c_index);
       (void)cache_add_object(100*sizeof(char),c_index);
       #endif /* DAISY_TEST */


       /*----------------*/
       /* Create a cache */
       /*----------------*/

       (void)cache_create(TRUE,cache_file_name,cache_blocks,c_index);
    }


    /*-----------------------*/
    /* Create affinity array */
    /*-----------------------*/

    n_blocks = cache_get_blocks(c_index);
    sum = (sum_type *)pups_calloc(n_blocks,sizeof(sum_type));


    /*-------------------------------------*/
    /*  Correlate image with rest of cache */ 
    /*-------------------------------------*/

    if(do_rgb == TRUE)
       (void)fprintf(stdout,"\n    Simulated NVD correlation of (RGB) image %d with cache\n",cache_index);
    else
       (void)fprintf(stdout,"\n    Simulated NVD correlation of (monochrome) image %d with cache\n",cache_index);
    (void)fflush(stdout);

    (void)fprintf(stdout,"    %d geometric operation (step is %d)\n",gt_ops,gt_op_step);

    #ifndef SINGLE_THREAD
    (void)fprintf(stdout,"    %d OMP threads\n",omp_get_max_threads());
    #endif /* SINGLE_THREAD */

    (void)fflush(stderr);

    if((nvd_1 = (FTYPE *)cache_access_object(cache_index,0,c_index)) == (FTYPE *)NULL)
    {  (void)snprintf(errstr,SSIZE,"[cache_correlation_test] cache index not within cache (%d blocks in cache)",n_blocks);
       pups_error(errstr);
    }


    /*---------------------------*/
    /* OMP parallelised for loop */
    /* with explcit granularity */
    /*---------------------------*/

    n_blocks = cache_get_blocks(c_index);

    #ifdef SINGLE_THREAD
    start_time = omp_get_wtime();
    #else
    start_time = millitime();
    #endif /* SINGLE_THREAD */


    for(i=0; i<n_blocks; ++i)
    {   char  *mask  = (char *)NULL;
        FTYPE *nvd_2 = (FTYPE *)NULL;
 
        nvd_2            = (FTYPE *)cache_access_object(i,0,c_index);
        mask             = (char *) cache_access_object(i,1,c_index);
        sum[i].affinity  = simulate_DAISY_correlation(do_rgb,mask,nvd_1,nvd_2);
        sum[i].index     = i;
    }

    #ifdef SINGLE_THREAD
    end_time = millitime();
    #else
    end_time = omp_get_wtime();
    #endif /* SINGLE_THREAD */


    /*-------------------------------------------*/
    /* Sort the simulated image-image affinities */
    /*-------------------------------------------*/

    (void)qsort((void *)sum,n_blocks,sizeof(sum_type),(void *)compare_affinities);
    if(parse_flags & CACHE_CWRITE) 
    {  for(i=0; i<n_blocks; ++i)
       {   (void)fprintf(stdout,"    %06d Image %06d correlation with image %06d is %9.6f\n",i,cache_index,sum[i].index,1.0 - (sum[i].affinity / sum[n_blocks - 1].affinity));
           (void)fflush(stdout);
       }

       (void)fprintf(stdout,"\n\n");
       (void)fflush(stdout);
    }


    /*----------------------------*/
    /* Time taken to process loop */
    /*----------------------------*/

    (void)fprintf(stderr,"    correlation (%d blocks) took %6.3f seconds\n",n_blocks,end_time - start_time);
    (void)fflush(stderr);


    /*---------------*/
    /* Destroy cache */
    /*---------------*/

    (void)pups_free((void *)sum);
    (void)cache_destroy(c_index);
    

    /*----------------------------------*/
    /* Upate state to show test has run */
    /*----------------------------------*/

    state_flags = TEST_PENDING;

}
#endif /* DAISY_TEST */



/*--------------------------------------*/
/* Randomly initialise objects in cache */
/* all are simulated NVD's              */
/*--------------------------------------*/

_PRIVATE void rinit_cache(int32_t do_rgb, int32_t rows, int32_t cols, int32_t c_index)

{    int32_t i,
             j,
             k,
             size,
             eff_cols;

    char     TorF;

    #ifdef DAISY_TEST
    unsigned char *next_auxdata = (unsigned char *)NULL;
    unsigned char *next_mask    = (unsigned char *)NULL;
    #endif /* DAISY_TEST */

    FTYPE         *next_object  = (FTYPE *)NULL;

    if(do_rgb == TRUE)
       eff_cols = 3*cols;
    else
       eff_cols = cols;

    for(i=0; i<cache_blocks; ++i)
    {  

       #ifndef DAISY_TEST 
       for(j=0; j<objects; ++j)
       {   next_object = (FTYPE *)cache_access_object(i,j,c_index); 
           for(k=0; k<rows*eff_cols; ++k)
               next_object[k] = 255.0*(FTYPE)drand48();

#ifdef DEBUG_CACHE
(void)fprintf(stderr,"OBJECT 0x%010x  value 0 is %6.3f\n",(uint64_t)next_object,next_object[0]);
(void)fflush(stderr);
#endif /* DEBUG_CACHE */

       }
       #else
       next_object   = (FTYPE *)cache_access_object(i,0,c_index);
       next_mask     = (char *) cache_access_object(i,1,c_index);
       next_auxdata  = (char *) cache_access_object(i,2,c_index);

       for(k=0; k<rows*eff_cols; ++k)
           next_object[k] = 255.0*(FTYPE)drand48();

       if(i % 2 == 0)
          TorF = 'F';
       else
          TorF = 'T';

       for(k=0; k<rows*cols; ++k)
           next_mask[k]   = 'T';


       for(k=0; k<100; k+=4)
       {   next_auxdata[k]   = 't';
           next_auxdata[k+1] = 'w';
           next_auxdata[k+2] = 'a';
           next_auxdata[k+3] = 't';
       }
          
#ifdef DEBUG_CACHE
(void)fprintf(stderr,"OBJECT 0x%010x  value 0 is %6.3f\n",(uint64_t)next_object,next_object[0]);
(void)fprintf(stderr,"MASK   0x%010x  value 0 is %c\n",(uint64_t   )next_mask,  next_mask[0]);
(void)fflush(stderr);
#endif /* DEBUG_CACHE */

       #endif /* DAISY_TEST */

    }
}




/*------------------------------*/
/* Run (fast) cache access test */
/*------------------------------*/

_PRIVATE void cache_access_test(_BOOLEAN do_rgb, int32_t cache_index, int32_t object_index)

{    int32_t i,
             c_index,
             eff_object_cols;

    if(do_rgb == TRUE)
       eff_object_cols = 3*object_cols;
    else
       eff_object_cols = object_cols;

    /*------------------------*/
    /* Initialise cache table */
    /*------------------------*/

    (void)cache_table_init();

    /*------------------*/
    /* Initialise cache */
    /*------------------*/

    if(parse_flags & CMAP_FILE)
    {  c_index = cache_init_cache(); 


       /*-------------------------------*/
       /* Uncompress cache if requested */
       /*-------------------------------*/

       if(parse_flags & EXTRACT)
         (void)cache_extract(cmap_file_name);

       (void)cache_read_mapinfo(cmap_file_name,c_index);
       (void)cache_create(FALSE,"",CACHE_USING_MAPINFO,c_index);
    }
    else
    {  c_index = cache_init_cache();


       /*-------------------------*/
       /* Add a couple of objects */
       /* to the cache            */
       /*-------------------------*/

       #ifndef DAISY_TEST
       for(i=0; i<objects; ++i)
          (void)cache_add_object(object_rows*eff_object_cols*sizeof(FTYPE),c_index);
       #else
                                                                                    /*-----------------*/
       (void)cache_add_object(object_rows*eff_object_cols*sizeof(FTYPE),c_index);   /* NVD array       */
       (void)cache_add_object(object_rows*    object_cols*sizeof(char), c_index);   /* NVD mask        */
       (void)cache_add_object(100*sizeof(char),c_index);                            /* Auxilliary data */
                                                                                    /*-----------------*/
       #endif /* DAISY_TEST */


       /*----------------*/
       /* Create a cache */
       /*----------------*/

       if(cache_n_quanta == 1)
          (void)cache_create(TRUE,cache_file_name,cache_blocks,c_index);
       else
       {   int32_t eff_cache_blocks,
                   cache_quantum;

          /*-------------------*/
          /* Test cache resize */
          /*-------------------*/

          // Base cache
          cache_quantum = cache_blocks / cache_n_quanta;
          (void)cache_create(TRUE,cache_file_name,cache_quantum,c_index);

          // Extend cache
          cache_blocks = 0;
          for(i=1; i<cache_n_quanta; ++i)
          {  cache_blocks = cache_quantum + i*cache_quantum;
             (void)cache_resize(cache_blocks,c_index);

             (void)fprintf(stderr,"    %06d: cache extended to %06d blocks (quantum is %06d)\n",i,cache_blocks,cache_quantum);
             (void)fflush(stderr);
          }

          // Shrink cache
          cache_blocks -= cache_quantum;
          (void)cache_resize(cache_blocks,c_index);

          (void)fprintf(stderr,"    %06d: cache shrunk to %06d blocks (quantum is %06d)\n",i,cache_blocks,cache_quantum);
          (void)fflush(stderr);
       }
    }


    /*------------------------*/
    /* Print cache statistics */
    /*------------------------*/

    (void)cache_display_statistics(stderr,c_index);


    /*---------------------------------------*/
    /* Randomly initialise cache if required */
    /*---------------------------------------*/

    if(parse_flags & CACHE_INIT)
    {  rinit_cache(do_rgb, object_rows,object_cols,c_index);
       (void)cache_write(cache_file_name,c_index);
    }

    /*------------------------------*/
    /* Reference an object in cache */
    /*------------------------------*/

    #ifdef DAISY_TEST
    flinarr = (FTYPE *)        cache_access_object(cache_index,0,c_index);
    mask    = (unsigned char *)cache_access_object(cache_index,1,c_index); 

    if(appl_verbose == TRUE)
    {  (void)fprintf(stderr,"    Cache object block:%d (DAISY test)\n",cache_index);
       (void)fprintf(stderr,"    Reference to \"flinarr\" at 0x%010x virtual\n\n",(uint64_t)flinarr);
       (void)fflush(stderr);
    }
    #else
    if((flinarr = (FTYPE *)cache_access_object(cache_index,object_index,c_index)) == (FTYPE *)NULL)
    {  (void)snprintf(errstr,SSIZE,"[cache_access_test] object index %d not within cache block bounds",object_index);
       pups_error(errstr);
    }

    if(appl_verbose == TRUE)
    {  (void)fprintf(stderr,"    Cache object block:%04d\n",cache_index);
       (void)fprintf(stderr,"    Reference to \"flinarr\" at 0x%010x virtual\n\n",(uint64_t)flinarr);
       (void)fflush(stderr);
    }
    #endif /* DAISY_OBJECT_ACCESS_TEST */


    /*---------------------------*/
    /* Display raw object output */
    /*---------------------------*/

    if(raw_object_output == TRUE)
    {   int32_t cnt = 0;

       #ifdef DAISY_TEST
       (void)fprintf(stderr,"\n    1D array access test (cache index %d, DAISY test)\n\n",cache_index,object_index);
       (void)fflush(stderr);
       #else
       (void)fprintf(stderr,"\n    1D array access test (cache index %d, object index %d)\n\n",cache_index,object_index);
       (void)fflush(stderr);
       #endif /* DAISY_TEST */

       for(i=0; i<object_rows*eff_object_cols; ++i)
       {  if(i%eff_object_cols == 0)
          { (void)fprintf(stderr,"    %04d flinarr[%04d]:\t\t%6.3f\n",cnt,i,flinarr[i]);
            (void)fflush(stderr);

            ++cnt;
          }
       }

       #ifdef DAISY_TEST
       (void)fprintf(stderr,"\n\n");
       (void)fflush(stderr);

       cnt = 0;
       for(i=0; i<object_rows*object_cols; ++i)
       {  if(i%object_cols == 0)
          { (void)fprintf(stderr,"    %04d mask[%04d]:\t\t%c\n",cnt,i,mask[i]);
            (void)fflush(stderr);

            ++cnt;
          }
       }
       #endif /* DAISY_TEST */
    }


    /*--------------------------------------*/
    /* Display output of object mapped onto */
    /* a 2D array                           */
    /*--------------------------------------*/

    if(mapped_object_output == TRUE)
    {   int32_t cnt = 0;


       /*---------------------------------*/
       /* Map 2D array onto object memory */
       /*---------------------------------*/

       flin2Darr = (FTYPE **)cache_map_2D_array(flinarr,object_rows,eff_object_cols,sizeof(FTYPE));

       #ifdef DAISY_TEST
       mask2Darr = (FTYPE **)cache_map_2D_array(mask   ,object_rows,object_cols,    sizeof(char));

       (void)fprintf(stderr,"\n    2D array access test (cache index %d, DAISY test\n\n",cache_index);
       #else
       (void)fprintf(stderr,"\n    2D array access test (cache index %d, object index %d)\n\n",cache_index,object_index);
       #endif /* DAISY_TEST */

       (void)fflush(stderr);

       for(i=0; i<object_rows; ++i)
       {  uint32_t j;

          for(j=0; j<eff_object_cols; ++j) 
          {  if(j == 0)
             { (void)fprintf(stderr,"    %04d flin2Darr[%04d][%04d]:\t\t%6.3f\n",cnt,i,j,flin2Darr[i][j]);
               (void)fflush(stderr);

               ++cnt;
             }
          }
       }

       #ifdef DAISY_TEST
       (void)fprintf(stderr,"\n\n");
       (void)fflush(stderr);

       cnt = 0;
       for(i=0; i<object_rows; ++i)
       {  uint32_t j;

          for(j=0; j<object_cols; ++j)
          {  if(j == 0)
             { (void)fprintf(stderr,"    %04d mask2Darr[%04d][%04d]:\t\t%c\n",cnt,i,j,mask2Darr[i][j]);
               (void)fflush(stderr);
             }
          }
       }
       #endif /* DAISY_TEST */

       (void)fprintf(stderr,"\n");
       (void)fflush(stderr);
    }


    /*--------------------------------------*/
    /* Write cache map if requsted to do so */
    /*--------------------------------------*/

    if(parse_flags & WCMAP_FILE)
    {  (void)cache_write_mapinfo(wcmap_file_name,c_index);


       /*-------------------------------*/
       /* Compress archive if requested */
       /*-------------------------------*/

       if(parse_flags & ARCHIVE)
       {  _BOOLEAN archive_compress = FALSE,
                   archive_delete   = FALSE;

          if(parse_flags & COMPRESS)
             archive_compress = TRUE;

          if(parse_flags & DELETE)
             archive_delete = TRUE;

          (void)cache_archive(archive_compress,
                              archive_delete,
                              wcmap_file_name);
       }
    }


    /*---------------*/
    /* Destroy cache */
    /*---------------*/

    (void)cache_destroy(c_index);


    /*----------------------------------*/
    /* Upate state to show test has run */
    /*----------------------------------*/

    state_flags = TEST_PENDING;
}

