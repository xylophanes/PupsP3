/*------------------------------------------------------------------------------
    Purpose: Portable hash access library.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 3.00 
    Dated:   27th February 2024
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

typedef struct {    uint32_t      size;          // Number of slots in hash entry 
                    uint32_t      used;          // Number of used slots in hash entry
                    uint32_t      *index;        // List of indices (for object list)
                    size_t        *object_size;  // List of object sizes
                    char          **object_type; // List of object types at hash entry
                    void          **object;      // List of objects at hash entry
               } hash_type;


/*--------------------------*/
/* Hash table datastructure */
/*--------------------------*/

typedef struct {    int32_t   size;              // Size of hash table 
                    char      name[SSIZE];       // Name of hash table
                    hash_type *hashentry;        // Hash table entries
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
_PUBLIC hash_table_type *hash_table_create(const uint32_t   , const char *);

// Destroy hash table object
_PROTOTYPE _EXPORT hash_table_type *hash_table_destroy(hash_table_type *);

//  Return object at a given location
_PROTOTYPE _EXPORT  int32_t hash_get_object(const uint32_t   , void *, const char *, hash_table_type *);

// Add object to hashed object list
_PROTOTYPE _EXPORT  int32_t hash_put_object(const uint32_t   , const void *, const char *, const size_t, hash_table_type *);

// Routine to delete object from hash table
_PROTOTYPE _EXPORT  int32_t hash_delete_object(const uint32_t   , hash_table_type *);

// Show hash statistics [root thread]
_PROTOTYPE _EXPORT  int32_t hash_show_stats(const FILE *, const _BOOLEAN,  hash_table_type *);

#ifdef _CPLUSPLUS
#   undef  _EXPORT
#   define _EXPORT extern C
#else
#   undef  _EXPORT
#   define _EXPORT extern
#endif

#endif /* HASH_H */
