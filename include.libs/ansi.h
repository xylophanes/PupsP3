/*----------------------------------------------------------------------------- 
    This is a set of common types and definitions for a ANSIish compiler.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    E-mail:  mao@tumblingdice.co.uk 
    Version: 2.00 
    Dated:   30th August 2019 

    Adapted from public domain ansi header by J.R. Bammi [jrb@cadence.com]
------------------------------------------------------------------------------*/

#ifndef _ANSI_H
#define _ANSI_H

/*------------------------------------------------------------------------------
    This statement defines whether the compiler to be used is ANSI or
    C++ - this is required so that c++ compilers see the correct tokens
    for turning off argument checking ...
------------------------------------------------------------------------------*/

#ifdef _CPLUSPLUS
#   define __UDEF_ARGS__ ...
#else
#   define __UDEF_ARGS__
#endif /* _CPLUSPLUS */

/*------------------------------------------------------------------------------
    Define FALSE and TRUE if they are not aready defined ...
------------------------------------------------------------------------------*/

#ifdef __BOOLEAN__
#ifndef TRUE
typedef enum {    FALSE,
                  TRUE
             } Boolean;
#endif /* TRUE */
#else


/*------------------------------------------------------------------------------
    Define FALSE and TRUE if they are not aready defined ...
------------------------------------------------------------------------------*/

#ifndef FALSE 
#define FALSE 0
#endif /* FALSE */

#ifndef TRUE
#define TRUE  1 
#endif /* TRUE */

#endif /* __BOOLEAN__ */


/*------------------------------------------------------------------------------
    Equivalence real to double ...
------------------------------------------------------------------------------*/

typedef double real;
 



/*------------------------------------------------------------------------------
    Some common definitions ...
------------------------------------------------------------------------------*/

#define _VOIDSTAR      void *
#define _VOID          void 
#define	_CONST	       const
#define	_VOLATILE      volatile
#define _SIZET	       size_t

#ifdef __BOOLEAN__
#define _BOOLEAN       Boolean
#else
#define _BOOLEAN       int 
#endif /* __BOOLEAN__ */

#define _BYTE          unsigned char




/*------------------------------------------------------------------------------
    Define PUBLIC and PRIVATE a la Tannenbaum ...
------------------------------------------------------------------------------*/

#define _PROTOTYPE          /* Prototype declarator                           */
#define _PRIVATE   static   /* Define private to local library                */
#define _PUBLIC             /* Define a public function of library            */
#define _IMPORT    extern   /* Define function/var as imported                */
#define _IMMORTAL  static   /* Variable exists between function calls         */
#define _EXTERN    extern   /* Define a variable as external                  */




/*------------------------------------------------------------------------------
    Define (local) floating point limits ...
------------------------------------------------------------------------------*/

#define SMALL_FLOAT 1.0e-30
#define BIG_FLOAT   1.0e+30

#ifndef MAXFLOAT
#define MAXFLOAT    1.0e+120
#endif /* MAXFLOAT */

#ifndef MINFLOAT
#define MINFLOAT    -1.0e+120
#endif /* MINFLOAT */




/*------------------------------------------------------------------------------
    Set the source code expansion switch ...
------------------------------------------------------------------------------*/

#define __NOT_LIB_SOURCE__
#define EXIT_SUCCESS     0

#endif /* ANSI_H */
