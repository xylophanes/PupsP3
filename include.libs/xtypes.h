/*--------------------------------------------------- 
    Header for extended type definitions used by PUPS

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.00 
    Dated:    2024
    12th December 2024
    E-Mail:  mao@tumblingdice.co.uk
---------------------------------------------------*/

#ifndef XTYPES_H
#define XTYPES_H


/*-------------------------------------------*/
/* Make sure we havr standard interger types */
/*-------------------------------------------*/

#include <stdint.h>

/*-----------------------------------------------------------------*/
/* The definitions that follow enable the PUPS libraries and other */
/* things written by me to compile - Mark O'Neill                  */
/*-----------------------------------------------------------------*/

#ifdef __BOOLEAN__

#ifndef TRUE
typedef enum {    FALSE,
                  TRUE
             } Boolean;
#endif /* __BOOLEAN__ */

#else


/*------------------------------------------------------*/
/* Define FALSE and TRUE if they are not aready defined */
/*------------------------------------------------------*/


#undef FALSE
#define FALSE 0

#undef TRUE
#define TRUE  255 

#endif /* __BOOLEAN__ */

#define NO_CHECKPOINTING (-1234)


/*-------------------------------------------*/
/* Define PUBLIC and PRIVATE a la Tannenbaum */
/*-------------------------------------------*/

#define _PROTOTYPE                    // Prototype declarator
#define _PRIVATE  static              // Define private to local library 
#define _PUBLIC                       // Define a public function of library 
#define _PROTECTED                    // Define a protected function of library
#define _IMPORT   extern              // Define function/var as imported
#define _IMMORTAL static              // Variable exists between function calls
#define _EXTERN   extern              // Define a variable as external
#define _EXPORT   extern              // Header export variable definition
#define _BOOLEAN  int32_t             // Define the Boolean type
#define _BYTE     unsigned char       // Definition of byte type
#define _DYNAMIC                      // Dynamic object identifier
#define _WRITEABLE                    // Writeable protected type modifier
#define _REAL     double              // Real variable
#define _USHORT   unsigned short      // Unsigned short
#define _ULONG    unsigned long       // Unsigned long



/*-------------------------------------------------*/
/* Transformed dynamic I/O types produced by slink */
/*-------------------------------------------------*/

#define _DYNAMIC_IMPORT static
#define _DYNAMIC_EXPORT




/*------------------------------*/
/* Some common type definitions */
/*------------------------------*/

typedef uint64_t         psize_t;   // Used to set the contents of a pointer
typedef uint32_t         pindex_t;  // Used for array indexing
typedef int32_t          des_t;     // File descriptor type




/*--------------------------------------*/
/* Standard dummy variable declarations */
/*--------------------------------------*/

#define CHDUM '\0'
#define IDUM  (int32_t)0
#define DDUM  (double)0.0




/*---------------------------------------------------------------------*/
/* This statement defines whether the compiler to be used is ANSI or   */
/* C++ - this is required so that c++ compilers see the correct token  */
/* for turning off argument checking                                   */
/* --------------------------------------------------------------------*/

#ifdef _CPLUSPLUS
#   define __UDEF_ARGS__ ...
#else
#   define __UDEF_ARGS__
#endif

#endif /* XTYPES_H */

