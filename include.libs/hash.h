/*------------------------------------------------------------------------------
    Purpose: Portable hash access library.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 3.00 
    Dated:   4th January 2023
    E-Mail:  mao@tumblingdice.co.uk
------------------------------------------------------------------------------*/

#ifndef HASH_H
#define HASH_H

/*----------------------------------*/
/* Defines used by the hash library */
/*----------------------------------*/


/*------------------*/
/* Keep c2man happy */
/*------------------*/

#include <c2man.h>


/***********/
/* Version */
/***********/

#define HASH_VERSION  "3.00"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE 2048


/*-------------------------------*/
/* Floating point representation */
/*-------------------------------*/

#include <ftype.h>

#ifdef PTHREAD_SUPPORT
#include <tad.h>
#endif /* PTHREAD_SUPPORT */


/*--------------------------*/
/* Hash entry datastructure */
/*--------------------------*/

typedef struct {    int       size;          // Number of slots in hash entry 
                    int       used;          // Number of used slots in hash entry
                    int       *index;        // List of indices (for object list)
                    int       *object_size;  // List of object sizes
                    char      **object_type; // List of object types at hash entry
                    void      **object;      // List of objects at hash entry
               } hash_type;


/*--------------------------*/
/* Hash table datastructure */
/*--------------------------*/

typedef struct {    int       size;          // Size of hash table 
                    char      name[SSIZE];   // Name of hash table
                    hash_type *hashentry;    // Hash table entries
               } hash_table_type;

#ifdef __NOT_LIB_SOURCE__

#else /* External variable declarations */
#   undef  _EXPORT
#   define _EXPORT
#endif /* __NOT_LIB_SOURCE__ */


/*-------------------------------------------*/
/* Public functions exported by hash library */
/*-------------------------------------------*/

// Create hash table object
_PUBLIC hash_table_type *hash_table_create(int, char *);

// Destroy hash table object
_PROTOTYPE _EXPORT hash_table_type *hash_table_destroy(hash_table_type *);

//  Return object at a given location
_PROTOTYPE _EXPORT int hash_get_object(int, void *, char *, hash_table_type *);

// Add object to hashed object list
_PROTOTYPE _EXPORT int hash_put_object(int, void *, char *, int, hash_table_type *);

// Routine to delete object from hash table
_PROTOTYPE _EXPORT int hash_delete_object(int, hash_table_type *);

// Report on hash table accesses
_PROTOTYPE _EXPORT int hash_show_stats(FILE *, _BOOLEAN,  hash_table_type *);

#ifdef _CPLUSPLUS
#   undef  _EXPORT
#   define _EXPORT extern C
#else
#   undef  _EXPORT
#   define _EXPORT extern
#endif

#endif /* HASH_H */
