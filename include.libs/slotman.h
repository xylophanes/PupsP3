/*------------------------------------------------------------------------------
    Purpose: Header for library slot manager.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    E-Mail:  mao@tumblingdice.co.uk 
    Dated:   8th June 2018 
    Version: 1.16
------------------------------------------------------------------------------*/

#ifndef SLOTMAN_H
#define SLOTMAN_H


/*------------------*/
/* Keep c2man happy */
/*------------------*/

#include <c2man.h>

#define SLOT_BASE 18 

/*-----------------------------------*/
/* Definitions local to this library */
/*-----------------------------------*/

#define SLOTMAN_VERSION "1.16"

#ifndef MAX_SLOTS
#define MAX_SLOTS    64 
#endif

#ifdef __NOT_LIB_SOURCE__
#else
#   undef  _EXPORT
#   define _EXPORT
#endif


/*-------------------------------------------*/
/* Public functions exported by slot manager */
/*-------------------------------------------*/

// Initialise slot management
_EXPORT _PROTOTYPE void slot_manager_init(void);

// Display current slot usage
_EXPORT _PROTOTYPE void slot_usage(int);

// Display application shortform usage
_EXPORT _PROTOTYPE void usage(void);

#ifdef SUPPORT_DLL

// Routine for dynamic slot allocation
_EXPORT _PROTOTYPE void add_dll_slot(const void *, const char *);

// Routine for dynamic usage slot allocation
_EXPORT _PROTOTYPE void add_dll_usage_slot(const void *, const char *);

#endif    /* DLL support */
#ifdef _CPLUSPLUS
#   undef  _EXPORT
#   define _EXPORT extern C
#else
#   undef  _EXPORT
#   define _EXPORT extern
#endif

#endif /* Slot manager   */
