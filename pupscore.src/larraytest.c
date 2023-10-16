/*-------------------------------------------------------------------------------------------------------
    Purpose: Test process list vector and matrix library 

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

    Version: 2.00 
    Dated:   4th January 2022
    E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------------------------------------------*/

#define TEST_THREADS

#include <me.h>
#include <utils.h>
#include <netlib.h>
#include <psrp.h>
#include <ftype.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <hipl_hdr.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pheap.h>
#include <vstamp.h>
#include <casino.h>
#include <hash.h>
#include <tad.h>
#include <larray.h>
#include <math.h>
#include <stdlib.h>


/*-----------------------------*/
/* Version of this application */
/*-----------------------------*/

#define LARRAYTEST_VERSION    "2.00"

#ifdef BUBBLE_MEMORY_SUPPORT
#include <bubble.h>
#endif /* BUBBLE_MEMORY_SUPPORT */




/*------------------------------------------------------------------------------------------------------
    Get application information for slot manager ...
------------------------------------------------------------------------------------------------------*/


/*---------------------------*/ 
/* Slot information function */
/*---------------------------*/ 

_PRIVATE void larraytest_slot(int level)
{   (void)fprintf(stderr,"int app (PSRP) larraytest %s: [ANSI C, PUPS MTD D]\n",LARRAYTEST_VERSION);
 
    if(level > 1)
    {  (void)fprintf(stderr,"(C) 2013-2022 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. ONeill\n");
       (void)fprintf(stderr,"Unassigned PSRP dynamic process (PUPS format) (built %s)\n\n",__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");

    (void)fflush(stderr);
}
 
 
 
 
/*----------------------------*/ 
/* Application usage function */
/*----------------------------*/

_PRIVATE void larraytest_usage(void)

{   (void)fprintf(stderr,"[-state]\n\n");
    (void)fprintf(stderr,"[>& <ASCII log file>]\n\n");

    (void)fprintf(stderr,"Signals\n\n");
    (void)fprintf(stderr,"SIGINIT SIGCHAN SIGPSRP: Process status [PSRP] request (protocol %2.2fF)\n",PSRP_PROTOCOL_VERSION);
    (void)fprintf(stderr,"SIGCLIENT: tell client server is about to segment\n");

#ifdef CRUI_SUPPORT
    (void)fprintf(stderr,"SIGCHECK SIGRESTART:      checkpoint and restart signals\n");
#endif /* CRIU_SUPPORT */

    (void)fprintf(stderr,"SIGALIVE: check for existence of client on signal dispatch host\n");
    (void)fflush(stderr);
}


#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT)() __attribute__ ((aligned(16))) = larraytest_slot;
_EXTERN void (* USE )() __attribute__ ((aligned(16))) = larraytest_usage;
#endif /* SLOT */




/*-------------------------------------------------------------------------------------------------------
   Application build date ...
-------------------------------------------------------------------------------------------------------*/

_EXTERN char appl_build_time[SSIZE] = __TIME__;
_EXTERN char appl_build_date[SSIZE] = __DATE__;




/*-------------------------------------------------------------------------------------------------------
    Software I.D. tag (used if CKPT support enabled to discard stale dynamic
    checkpoint files) ...
-------------------------------------------------------------------------------------------------------*/

#define VTAG  3978

extern int appl_vtag = VTAG;




/*--------------------------------------------------------------------------------------------------------
    Main - decode command tail then interpolate using parameters supplied by user ...
--------------------------------------------------------------------------------------------------------*/

_PUBLIC int pups_main(int argc, char *argv[])

{   int i,
        j,
        pos,
        bytes,
        components,
        iter = 0;

    FTYPE pattern_vector[32],
          pattern_matrix[32*32];

    unsigned char *ptr = (unsigned char *)NULL;
    vlist_type *vector = (vlist_type *)   NULL;
    mlist_type *matrix = (mlist_type *)   NULL;


    /*---------------------------------------------------------------*/
    /* Do not allow PSRP clients to connect until we are initialised */
    /*---------------------------------------------------------------*/

    psrp_ignore_requests();


    /*------------------------------------------*/
    /* Get standard items form the command tail */
    /*------------------------------------------*/

    pups_std_init(TRUE,
                  &argc,
                  TEST_VERSION,
                  "M.A. O'Neill",
                  "(PSRP) larraytest",
                  "2022",
                  argv);


    /*---------------------------------------*/
    /* Complain about any unparsed arguments */
    /*---------------------------------------*/

    pups_t_arg_errs(argd,args);


    /*------------------------------------------------------------------------------------------*/
    /* Initialise PSRP function dispatch handler - note that test is fully dynamic and prepared */
    /* to import both dynamic functions and data objects                                        */
    /*------------------------------------------------------------------------------------------*/

    psrp_init(PSRP_DYNAMIC_FUNCTION | PSRP_STATIC_DATABAG | PSRP_HOMEOSTATIC_STREAMS,NULL);
    (void)psrp_load_default_dispatch_table(stderr);


    /*----------------------------------------------------------*/
    /* Tell PSRP clients we are ready to service their requests */
    /*----------------------------------------------------------*/

    psrp_accept_requests();


    /*------------------------------*/
    /* Seed random number generator */
    /*------------------------------*/

    (void)srand48((long) getpid());


    #ifdef SOAK_TEST
    #ifdef TEST_ALLOC
    while(1)
    {

         /*-----------------------------------*/
         /* Excercise bubble memory allocator */
         /*-----------------------------------*/

         bytes = (int)(drand48()*500000000.0);
         ptr   = (unsigned char *)malloc(bytes);

         (void)fprintf(stderr,"ITERATION %04d allocating and freeing %04d bytes of memory (ptr: 0x%010x)\n",iter,bytes,ptr);
         (void)fflush(stderr);

         for(i=0; i<bytes; ++i)
            ptr[i] = 'w';

         (void)free((void *)ptr);

         (void)pups_sleep(1);

         (void)fprintf(stderr,"ITERATION %d done\n",iter++);
         (void)fflush(stderr);

    }
    #endif /* TEST_ALLOC */
    #endif /* SOAK_TEST  */


    #ifdef TEST_VECTOR
    /*------------------------*/
    /* Generate random vector */
    /*------------------------*/

    for(i=0; i<32; ++i)
    {  if(drand48() > 0.5)
          pattern_vector[i] = 100.0*drand48() + 1.0;
       else
          pattern_vector[i] = 0.0;
    }


    /*----------------------*/
    /* Create a list vector */
    /*----------------------*/

    (void)fprintf(stderr,"\n\n");
    (void)fflush(stderr);

    components = (int)(drand48()*90.0) + 10;
    vector = lvector_create(stderr,components,pattern_vector);

    // Name it
    (void)lvector_set_name("listVector",vector);

    // Print it 
    (void)lvector_print(stderr,4,vector);

    // inflate it
    vector = lvector_inflate(FALSE, vector);

    // Print it (inflated)
    (void)lvector_print(stderr,4,vector);
 
    // deflate it 
    vector = lvector_deflate(stderr, FALSE, NULL, vector);

    // Print it (deflated)
    (void)lvector_print(stderr,4,vector);

    // Save it to an ascii file
    (void)lvector_save_to_file("fred.vla",vector);

    // Destroy it
    vector = lvector_destroy(vector);

    // Reload it/
    vector = lvector_load_from_file("fred.vla");

    // Name it
    (void)lvector_set_name("listVectorASCIIReload",vector);

    // Print it (deflated) */
    (void)lvector_print(stderr,5,vector);

    // Rename it
    (void)lvector_set_name("listVectorBinaryReload",vector);

    // Save it as a binary file 
    (void)lvector_save_to_binary_file("fred.vlb",vector);

    // Destroy it
    vector = lvector_destroy(vector);

    // Reload it from binary file */
    vector = lvector_load_from_binary_file("fred.vlb");

    // Print it (deflated)
    (void)lvector_print(stderr,5,vector);

    // Destroy it
    vector = lvector_destroy(vector);
    #endif /* TEST_VECTOR */


    #ifdef TEST_MATRIX
    /*------------------------*/
    /* Generate random matrix */
    /*------------------------*/

    for(i=0; i<32; ++i)
    {  for(j=0; j<32; ++j)
       {  pos = i*32 + j;
 
          if(drand48() < 0.25)
             pattern_matrix[pos] = 100.0*drand48() + 1.0;
          else
             pattern_matrix[pos] = 0.0;
       }
    }
 
    // Create a matrix 
    matrix = lmatrix_create(stderr,32,32,pattern_matrix);

    // Name it 
    (void)lmatrix_set_name("listMatrix",matrix);

    // Print it
    (void)lmatrix_print(stderr,4,matrix);

    // inflate it 
    matrix = lmatrix_inflate(FALSE, matrix);

    // Print it (inflated)
    (void)lmatrix_print(stderr,4,matrix);

    // deflate it
    matrix = lmatrix_deflate(stderr, FALSE, NULL, matrix);

    // Print it (deflated)
    (void)lmatrix_print(stderr,4,matrix);

    // Save it to an ascii file 
    (void)lmatrix_save_to_file("fred.mla",matrix);

    // Destroy it
    (void)lmatrix_destroy(matrix);

    // Reload it
    matrix = lmatrix_load_from_file("fred.mla");

    // Name it
    (void)lmatrix_set_name("listMatrixASCIIReload",matrix);

    // Print it (deflated)
    (void)lmatrix_print(stderr,5,matrix);

    // Rename it
    (void)lmatrix_set_name("listMatrixBinaryReload",matrix);

    // Save it as a binary file
    (void)lmatrix_save_to_binary_file("fred.mlb",matrix);

    // Destroy it
    (void)lmatrix_destroy(matrix);

    // Reload it from binary file
    matrix = lmatrix_load_from_binary_file("fred.mlb");

    // Print it (deflated)
    (void)lmatrix_print(stderr,5,matrix);

    // Destroy it
    (void)lmatrix_destroy(matrix);
    #endif /* TEST_MATRIX */


    #if defined(TEST_VECTOR) || defined(TEST_MATRIX)
    #ifdef SOAK_TEST
    (void)fprintf(stderr,"SOAK test iteration %d\n",iter++);
    (void)fflush(stderr);

    (void)pups_sleep(1);
    #endif /* SOAK_TEST */
    #endif /* defined(TEST_VECTOR) || defined(TEST_MATRIX) */


    /*--------------------------------------------------------------------------*/
    /* Exit from PUPS/PSRP application cleaning up any mess it may have created */
    /*--------------------------------------------------------------------------*/

    pups_exit(0);
}

