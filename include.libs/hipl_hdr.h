/*----------------------------------------------------------------------------- 
    Header for the HIPS library.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.00 
    Dated:   30th August 2019 
    E-Mail:  mao@tumblingdice.co.uk 
------------------------------------------------------------------------------*/


#ifndef HIPL_H
#define HIPL_H


/*------------------*/
/* Keep c2man happy */
/*------------------*/

#include <c2man.h>


/***********/
/* Version */
/***********/

#define HIPLIB_VERSION  "2.00"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE 2048


#ifndef __C2MAN__
#include <sys/types.h>
#include <fcntl.h>
#include <utils.h>
#endif /* __C2MAN__ */


/*---------------------------------------------------------------*/
/* Routines for accessing elements of arrays of known pixel type */
/*---------------------------------------------------------------*/


/*---------------------------------------*/
/* Access an element if a HIPS row array */
/*---------------------------------------*/

#define H_R_ACC(row_arr, i, size) (void *)((key_t)row_arr      + (key_t)i*size)


/*-----------------------------------------*/
/* Access an element if a HIPS frame array */
/*-----------------------------------------*/

#define H_F_ACC(frame_arr, i, j, size) (void *)((key_t)frame_arr[i] + (key_t)j*size)



/*------------------------------------------------------*/
/* Definition of the constants required by this library */
/*------------------------------------------------------*/

#define HIPS_MAGIC "PUPSP3-HIPS-1.00"

#define PFBYTE     0
#define PFSHORT    1
#define PFINT      2
#define PFLONG     6
#define PFFLOAT    3
#define PFCOMPLEX  4
#define PFASCII    5
#define PFDOUBLE   7
#define PFDCOMPLEX 8

#define PFQUAD1    11
#define PFBHIST    12
#define PFSPAN     13
#define PLOT3D     24

#define PFAHC      400
#define PFOCT      401
#define PFBT       402
#define PAHC3      403
#define PFBQ       404
#define PFRLED     500
#define PFRLEB     501
#define PFRLEW     502

#define PFIBYTE    601   
#define PFISHORT   602
#define PFIINT     603
#define PFIFLOAT   604
#define PFICOMPLEX 605
#define PFIASCII   606
#define PFIDOUBLE  607

#define PFANY      999


/*---------------------*/
/* Bit packing formats */
/*---------------------*/

#define MSBFIRST   1
#define LSBFIRST   2



typedef struct
{
   char   orig_name[SSIZE];     // The originator of this sequence 
   char   seq_name[SSIZE];      // The name of this sequence 
   int    num_frame;            // The number of frames in this sequence 
   char   orig_date[SSIZE];     // The date the sequence was originated
   int    rows;                 // The number of rows in each image 
   int    cols;                 // The number of columns in each image
   int    bits_per_pixel;       // The number of significant bits per pixel 
   int    bit_packing;          // Nonzero if bits were packed contiguously
   int    pixel_format;         // The format of each pixel, see below 
   char   seq_history[SSIZE];   // The sequence's history of transformations 
   char   seq_desc[SSIZE];      // Descriptive information 
} hipl_hdr;



/*-------------------*/
/* Complex data type */
/*-------------------*/

typedef struct {   float real,
                         imag;
               }   complex;


/*------------------------------------------*/
/* Constants exported by the library hiplib */
/*------------------------------------------*/

#ifdef __NOT_LIB_SOURCE__

#else /* Export external variables */
#   undef  _EXPORT
#   define _EXPORT
#endif

#define   FBUFLIMIT 30000
#ifdef IMG_SUPPORT



/*----------------------*/
/* Kontron (IMG) format */
/*----------------------*/

#define IMG_GREY    1
#define IMG_RGB     2

typedef struct {    int      rows;           // Rows in image 
                    int      cols;           // Columns in image 
                    int      n_frames;       // Images in sequence 
                    int      colour;         // RGB or GREY 
                    long int datatype;       // Image datatype 
                    long int seeklutdata;    // Image RGB LUT offset
                    long int seekRdata;      // Offset to grey (RGB red) data 
                    long int seekGdata;      // Offset to RGB green data 
                    long int seekBdata;      // Offset to RGB blue data 
                    long     size;           // Image size (in pixels)
                    _BOOLEAN has_lut;        // TRUE if image has LUT
                    _BYTE    lut[768];       // RGB look up table 
               } img_hdr_type;

#endif
 
 
/*------------------------------------------------------------------------------
    Functions and procedures which work on the hipl_header data type ...
------------------------------------------------------------------------------*/

// Get HIPS header from stdin
_PROTOTYPE _EXPORT int  hips_rd_hdr_info(int, int *, int *, int *, hipl_hdr *);

// Display HIPS header information in textual format
_PROTOTYPE _EXPORT int  hips_display_hdr_info(FILE *, hipl_hdr *);

// Get HIPS header from file 
_PROTOTYPE _EXPORT int  hips_f_rd_hdr_info(int, int, int *, int *, int *, hipl_hdr *);

// Write HIPS header to stdout
_PROTOTYPE _EXPORT void hips_wr_hdr_info(char *, int, char *[], char *, hipl_hdr *);


// Write HIPS header to file
_PROTOTYPE _EXPORT void hips_f_wr_hdr_info(int, char *, int, char *[], char *, hipl_hdr *);

// Read HIPS header
_PROTOTYPE _EXPORT void hips_rd_hdr(hipl_hdr *);

// Read HIPS header from file
_PROTOTYPE _EXPORT void hips_frd_hdr(int, hipl_hdr *);

// Write hips header
_PROTOTYPE _EXPORT void hips_wr_hdr(hipl_hdr *);

// Write HIPS header to file 
_PROTOTYPE _EXPORT void hips_fwr_hdr(int, hipl_hdr *);

// Initialise HIPS header
_PROTOTYPE _EXPORT void hips_init_hdr  (hipl_hdr *, char *, char *, int, char *, int, int, int, int, int, char *, char *);

// Skip frame of data
_PROTOTYPE _EXPORT void hips_frame_skip(int, int, hipl_hdr *);

// Update the header descriptor
_PROTOTYPE _EXPORT void hips_upd_hdr(hipl_hdr *, char *, int, char **);

// Update the sequence discriptor
_PROTOTYPE _EXPORT void hips_upd_desc(hipl_hdr *, char *);

// Set header descriptor
_PROTOTYPE _EXPORT void hips_set_desc(hipl_hdr *, char *);

// Allocate memory for a frame
_PROTOTYPE _EXPORT void **hips_fralloc(hipl_hdr *);

// Allocate memory for n rows of image
_PROTOTYPE _EXPORT void **hips_nralloc(int, hipl_hdr *);

// Allocate memory for one image row
_PROTOTYPE _EXPORT void *hips_ralloc(hipl_hdr *);

// Pread routine to read correctly from a pipe 
_PROTOTYPE _EXPORT int pipe_pread(int, void *, int);

// Routine to return size of pixel in bytes
_PROTOTYPE _EXPORT int hips_pixel_size(int);

// Routine to decode a pixel output type
_PROTOTYPE _EXPORT int hips_output_pixel_type(char *);

// Routine to return type string corresponding to a HIPL pixel format
_PROTOTYPE _EXPORT char *hips_pixstr(int);

// Routine to destroy HIPS header datastructure
_PROTOTYPE _EXPORT void hips_destroy_hdr(hipl_hdr *);


#ifdef TIFF_SUPPORT
// Convert a HIPS file to a TIFF file
_PROTOTYPE _EXPORT void hips_to_tiff(_BOOLEAN, int, int, char **, int, char *, char *);

// Convert a TIFF file to a HIPS file
_PROTOTYPE _EXPORT void tiff_to_hips(_BOOLEAN, int, char **, char *, char *);
#endif /* TIFF_SUPPORT */


#ifdef IMG_SUPPORT
// Read Kontron IMG image
_PROTOTYPE _EXPORT void konrton_read_hdr(int  fdes, _BOOLEAN, img_hdr_type *);

// Write Kontron IMG image
_PROTOTYPE _EXPORT void kontron_write_hdr(int fdes, img_hdr_type *);

// Convert Kontron image to HIPS image
_PUBLIC void kontron_to_hips(_BOOLEAN, int, char *[], char *, char *);

// Convert HIPS image to Kontron image
_PUBLIC void hips_to_kontron(char *, char *);
#endif /* IMG_SUPPORT */

#ifdef _CPLUSPLUS
#   undef  _EXPORT
#   define _EXPORT extern C
#else
#   undef  _EXPORT
#   define _EXPORT extern
#endif

#endif /* HIPL_H */
