/*------------------------------------------------------------------------------
    Purpose: Header file for larraylib.

    Author: M.A. O'Neill
            Tumbling Dice Ltd
            Gosforth
            Newcastle upon Tyne
            NE3 4RT
            United Kingdom

    Version: 2.00 
    Dated:   4th January 2023
    E-mail:  mao@tumblingdice.co.uk
------------------------------------------------------------------------------*/


#ifndef LARRAY_HDR
#define LARRAY_HDR


/*------------------*/
/* Keep c2man happy */
/*------------------*/

#include <c2man.h>

#ifndef __C2MAN__
#include <stdio.h>
#include <me.h>
#include <utils.h>
#include <nfo.h>
#include <casino.h>
#include <psrp.h>
#include <signal.h>
#include <string.h>
#include <vstamp.h>
#endif /* __C2MAN__ */

#ifdef CKPT_SUPPORT
extern int checkpointing;
#endif /* CKPT_SUPPORT */


/*------------------------------------------------------------------------------
    Defines used by this application ...
------------------------------------------------------------------------------*/

/***********/
/* Version */
/***********/

#define LARRAY_VERSION                "2.00"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE                         2048



/*********************************/
/* Floating point representation */
/*********************************/

#include <ftype.h>



/*--------------------------------------*/
/* Allocation quantum (for row vectors) */
/*--------------------------------------*/

#define LARRAY_ALLOC_QUANTUM          32 


/*---------------*/
/* Vector states */
/*---------------*/

#define DEFLATED                      1 
#define INFLATED                      2 


/*--------------------------*/
/* Magic (for binary files) */
/*--------------------------*/

#define VLIST_MAGIC                   "vlist"
#define MLIST_MAGIC                   "mlist"


/*------------------------------------------------------------------------------
    Structures defined by this application ...
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    Definition of auxillliary data container type ...
------------------------------------------------------------------------------*/

typedef struct {   void     *save;       /* Save ASCII auxilliary data        */
                   void     *load;       /* Load ASCII auxilliary data        */
                   void     *bsave;      /* Save binary auxilliary data       */
                   void     *bload;      /* Load binary auxilliary data       */
                   void     *data;       /* Auxilliary data                   */
               } auxdata_type;


/*------------------------------------------------------------------------------
    Definition of the vlist_type (vector addressed as a list) ...
------------------------------------------------------------------------------*/

typedef struct {   char         name[256];   /* Name of list vector           */ 
                   int          state;       /* State (inflated or deflated)  */ 
                   int          components;  /* Size of vector                */
                   int          used;        /* Non zero components           */
                   int          allocated;   /* Non zero components           */
                   int          *index;      /* Allocated components          */
                   auxdata_type *auxdata;    /* Auxilliary data               */
                   FTYPE        *value;      /* Non zero component values     */
               } vlist_type;


/*------------------------------------------------------------------------------
    Definition of the mlist_type (matrix addressed as a list) ...
------------------------------------------------------------------------------*/

typedef struct {   char       name[256]; /* Name of matrix                    */
                   int        rows;      /* Rows in matrix                    */
                   int        cols;      /* Cols in matrix                    */
                   vlist_type **vector;  /* Row vectors                       */   
               } mlist_type;

/*------------------------------------------------------------------------------
    Functions exported by this application ...
------------------------------------------------------------------------------*/

// Destroy list vector
_PROTOTYPE _EXPORT vlist_type *lvector_destroy(vlist_type *);

// Destroy list matrix
_PROTOTYPE _EXPORT mlist_type *lmatrix_destroy(mlist_type *);

// Create list matrix
_PROTOTYPE _EXPORT mlist_type *lmatrix_create(FILE *, int, int, FTYPE *);

// Creat list vector
_PROTOTYPE _EXPORT vlist_type *lvector_create(FILE *, int, FTYPE *);

// Copy list matrix/
_PROTOTYPE _EXPORT mlist_type *lmatrix_copy(mlist_type *);

// Copy list vector
_PROTOTYPE _EXPORT vlist_type *lvector_copy(vlist_type *);

// Deflate list vector
_PROTOTYPE _EXPORT vlist_type *lvector_deflate(FILE *, _BOOLEAN, FTYPE *, vlist_type *);

// Deflate list matrix
_PROTOTYPE _EXPORT mlist_type *lmatrix_deflate(FILE *, _BOOLEAN, FTYPE *, mlist_type *);

// Inflate list vector
_PROTOTYPE _EXPORT vlist_type *lvector_inflate(_BOOLEAN, vlist_type *);

// Inflate list matrix
_PROTOTYPE _EXPORT mlist_type *lmatrix_inflate(_BOOLEAN, mlist_type *);

// Do fast re-initialisation of pattern vector
_PROTOTYPE _EXPORT int fastResetPatternVector(int, FTYPE *, vlist_type *);

// Get list value from list vector list
_PROTOTYPE _EXPORT FTYPE lvector_get_list_value(int, int *, vlist_type *);

// Get component value from list vector
_PROTOTYPE _EXPORT FTYPE lvector_get_component_value(int, vlist_type *);

// Get list value from list matrix
_PROTOTYPE _EXPORT FTYPE lmatrix_get_list_value(int, int, int *, mlist_type *);

// Get component value from list matrix
_PROTOTYPE _EXPORT FTYPE lmatrix_get_element_value(int row, int col, mlist_type *);

// Normalise list matrix
_PROTOTYPE _EXPORT int lmatrix_normalise(mlist_type *);

// Normalize list vector
_PROTOTYPE _EXPORT int lvector_normalise(vlist_type *);

// Print list vector
_PROTOTYPE _EXPORT int lvector_print(FILE *, int, vlist_type *);

// Scan list vector
_PUBLIC vlist_type *lvector_scan(FILE *);

// Print list matrix
_PROTOTYPE _EXPORT int lmatrix_print(FILE *, int, mlist_type *);

// Scan list matrix
_PUBLIC mlist_type *lmatrix_scan(FILE *);

// Set list vector name
_PROTOTYPE _EXPORT int lvector_set_name(char *, vlist_type *);

// Set list matrix name
_PROTOTYPE _EXPORT int lmatrix_set_name(char *, mlist_type *);

// Get list vector inflation state
_PROTOTYPE _EXPORT int lvector_get_state(vlist_type *);

// Get list vector size
_PROTOTYPE _EXPORT int lvector_get_size(vlist_type *);

// Get list matrix size
_PROTOTYPE _EXPORT int lmatrix_get_size(mlist_type *, int *, int *);

// Get list vector (allocated) size
_PROTOTYPE _EXPORT int lvector_get_allocated(vlist_type *);

// Get size of list vector (non zero components in corresponding vector)
_PROTOTYPE _EXPORT int lvector_get_used(vlist_type *);

// Get list vector compression factor
_PROTOTYPE _EXPORT FTYPE lvector_get_compression_factor(vlist_type *);

// Get list matrix compression factor
_PROTOTYPE _EXPORT FTYPE lmatrix_get_compression_factor(mlist_type *);

// Load list vector from (ASCII) file
_PROTOTYPE _EXPORT vlist_type *lvector_load_from_file(char *);

// Save list vector to (ASCII) file
_PROTOTYPE _EXPORT int lvector_save_to_file(char *, vlist_type *);

// Load list matrix from (ASCII) file
_PROTOTYPE _EXPORT mlist_type *lmatrix_load_from_file(char *filename);

// Save list matrix to (ASCII) file 
_PROTOTYPE _EXPORT int lmatrix_save_to_file(char *, mlist_type *);

// Save list vector in binary format to file
_PROTOTYPE _EXPORT int lvector_save_to_binary_file(char *, vlist_type *);

// Load list vector saved in binary format from file
_PROTOTYPE _EXPORT vlist_type *lvector_load_from_binary_file(char *);

// Save list matrix in binary format to file
_PROTOTYPE _EXPORT int lmatrix_save_to_binary_file(char *, mlist_type *);

// Load list matrix saved in binary format from file
_PROTOTYPE _EXPORT mlist_type *lmatrix_load_from_binary_file(char *);

// Save list vector in binary format to open file descriptor
_PROTOTYPE _EXPORT int lvector_save_to_binary_fildes(int, vlist_type *);

// Load list vector saved in binary format from open file descriptor
_PROTOTYPE _EXPORT vlist_type *lvector_load_from_binary_fildes(int);

// Save list matrix in binary format to open file descriptor
_PROTOTYPE _EXPORT int lmatrix_save_to_binary_fildes(int, mlist_type *);

// Load list matrix saved in binary format from open file descriptor
_PROTOTYPE _EXPORT mlist_type *lmatrix_load_from_binary_fildes(int);


#ifdef _CPLUSPLUS
#   undef  _EXPORT
#   define _EXPORT extern C
#else
#   undef _EXPORT
#   define _EXPORT extern
#endif

#endif /* LARRAY_HDR */
