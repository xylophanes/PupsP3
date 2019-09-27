/*-------------------------------------------------------------------------
    Purpose: header for DLL support library.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Dated:   2nd January 2019 
    Version: 1.14
    E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------------*/


#ifndef DLL_H
#define DLL_H


/*------------------*/
/* Keep c2man happy */
/*------------------*/

#include <c2man.h>

#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif /* HAVE_PTHREADS */


/***********/
/* Version */
/***********/

#define DLL_VERSION       "1.14"


/*------------------------------*/
/* Max orifices on PSRP client */
/*------------------------------*/

#define MAX_ORIFICES      64 


/*-------------------------------------------------------------------*/
/* Define RTLD flags for dlopen() interface (if they are not already */
/* defined in dlfcn.h)                                               */
/*-------------------------------------------------------------------*/

#ifndef RTLD_NOW

#define RTLD_NOW          0x00000001          // Set RTLD_NOW for this object
#define RTLD_GLOBAL       0x00000002          // Set RTLD_GLOBAL for this object

#endif /* RTLD_NOW */







/*--------------------------------*/
/* DLL import table datastructure */
/*--------------------------------*/

typedef struct {    int  cnt;                 // Bind count for this orifice 
                    void *orifice_handle;     // Handle of orifice 
                    char *orifice_name;       // Name of orifice 
                    char *orifice_prototype;  // Orifice prototype 
                    char *orifice_info;       // Orifice information string
		    void *dll_handle;         // Handle of DLL 
		    char *dll_name;           // DLL name 
	       } ortab_type;


/*-------------------------------------------------------------*/
/* Variables exported by the multithreaded DLL support library */
/*-------------------------------------------------------------*/

#ifdef __NOT_LIB_SOURCE__

_EXPORT _BOOLEAN   ortab_extend;              // TRUE if extending orifice table
_EXPORT ortab_type *ortab;                    // The orifice [DLL] table 

#endif /* __NOT_LIB_SOURCE__ */




/*-----------------------------------------*/
/* PUPS DLL/Orifice manipulation functions */
/*-----------------------------------------*/

// Initialise orifice table
_PROTOTYPE _EXPORT void ortab_init(const unsigned int);

// Check for existence of orifice and return its prototype
_PROTOTYPE _EXPORT _BOOLEAN pups_is_orifice(const char *, const char *, const char *);

// Bind DLL orifice function to handle
_PROTOTYPE _EXPORT void *pups_bind_orifice_handle(const char *, const char *, const char *);

// Free DLL orifice function
_PROTOTYPE _EXPORT void *pups_free_orifice_handle(const void *);

// Show currently bound DLL/orifice functions
_PROTOTYPE _EXPORT void pups_show_orifices(const FILE *);

// Decode a format-function prototype
_PROTOTYPE _EXPORT _BOOLEAN dll_decode_ffp(const char *, const char *);

// Find orifice table slot with specified handle
_PROTOTYPE _EXPORT int find_ortab_slot_by_handle(const void *);

// Find orifice table slot with specified name
_PROTOTYPE _EXPORT int find_ortab_slot_by_name(const char *);

// Scan a DLL and show its orifices
_PROTOTYPE _EXPORT _BOOLEAN pups_show_dll_orifices(const FILE *, const char *);

// Clear orifice table slot
_PROTOTYPE _EXPORT int clear_ortab_slot(const _BOOLEAN, const unsigned int);


#ifdef _CPLUSPLUS
#   undef  _EXPORT
#   define _EXPORT extern "C"
#else
#   undef  _EXPORT                                                              
#   define _EXPORT extern                                   
#endif                                                                          

#endif  /* DLL_H */

