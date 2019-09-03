/*------------------------------------------------------------------------------
    Purpose: header for PUPS fast cache library

    Author:  M.A. O'Neill,
             Tumbling Dice,
             Gosforth,
             Tyne and Wear NE3 4RT.

    Version: 2.00 
    Dated:   30th August 2019 
    E-mail:  mao@tumblingdice.co.uk
------------------------------------------------------------------------------*/

#ifndef CACHELIB_HDR
#define CACHELIB_HDR


/*--------------------------*/
/* Make sure c2man is happy */
/*--------------------------*/

#include <c2man.h>

#ifndef __C2MAN__
#include <me.h>
#include <ansi.h>
#include <utils.h>
#include <tiff.h>
#include <tiffio.h>
#include <ctype.h>
#endif /* __C2MAN__ */



/*----------------------------------------*/
/* Defines which are local to this module */
/*----------------------------------------*/

// Version
#define CACHELIB_VERSION        "2.00"

// Maximum caches in the cache table
#define MAX_CACHES              32

// Maximum number of objects per cache block
#define MAX_CACHE_BLOCK_OBJECTS 32


// Are we using cache information from map file?
#define CACHE_USING_MAPINFO     (-1)


// Definitions of byte storage units
#define KILOBYTE                1024
#define MEGABYTE                (1024*1024)
#define GIGABYTE                (1024*1024*1024)


// Block access and usage flags
#define BLOCK_USED              1
#define BLOCK_ACCESS            2
#define ANY_BLOCK               (-1)

// Cache archive flags
#define COMPRESS                1
#define DELETE                  2

// Data access flags
#define HOLD_CACHE              0
#define LOCK_CACHE              1
#define UNLOCK_CACHE            2




/*-----------------------------------------*/
/*  Types which are defined by this module */ 
/*-----------------------------------------*/


/*-------------------------*/
/* Dynamic cache structure */
/*-------------------------*/

typedef struct { void     *object_ptr[MAX_CACHE_BLOCK_OBJECTS];                // Pointers to objects within cache block
               } cache_mtype;


typedef struct {    char              name[SSIZE];                             // Name of this cache
                    char              mmap_name[SSIZE];                        // Name of memory mapped file
                    char              march[SSIZE];                            // Machine architecture
                    _BOOLEAN          mmap;                                    // Is cache memory mapped?
                    int               n_blocks;                                // Number of object blcoks in cache
                    int               n_objects;                               // Number of objecs in cache block
                    unsigned long int cache_size;                              // Size of entire cache
                    unsigned long int block_size;                              // Size of one cache block
                    unsigned long int object_offset[MAX_CACHE_BLOCK_OBJECTS];  // Offsets to objects in cache block
                    unsigned long int object_size[MAX_CACHE_BLOCK_OBJECTS];    // Sizes of cache block objects
                    void              *cache_memory;                           // Contiguous block of cache memory
                    cache_mtype       *cachemap;                               // Pointer mapping into cache memory
                    _BYTE             *flags;                                  // Cache block flags
               } cache_type;


/*------------------------------------------------------------------------------
    Public variables exported by this module ...
------------------------------------------------------------------------------*/

#ifdef __NOT_LIB_SOURCE__
#else /* External variable declarations */
#   undef  _EXPORT
#   define _EXPORT
#endif /* __NOT_LIB_SOURCE__


/*------------------------------------------------------------------------------
    Public functions exported by this module ...
------------------------------------------------------------------------------*/


// Get index of cache (from its name)
_PUBLIC int cache_name2index(const char *);

// Get name of cache (from its index)
_PUBLIC _BOOLEAN cache_index2name(char *, const unsigned int);

// Initialise the cache table
_PROTOTYPE _EXTERN int cache_table_init(void);

// Initialise cache datastructure
_PROTOTYPE _EXTERN int cache_init_cache(void);

// Add object to cache (prior to creating cache)
_PROTOTYPE _EXTERN int cache_add_object(const unsigned long int, const unsigned int);

// Create cache
_EXPORT int cache_create(const _BOOLEAN, const char *, const unsigned int, const unsigned int);

// Destroy cache
_PROTOTYPE _EXTERN int cache_destroy(const unsigned int);

// Display cache statistics
_PROTOTYPE _EXTERN int cache_display_statistics(const FILE *, const unsigned int);

// Display cache table 
_PROTOTYPE _EXTERN int cache_display(const FILE *);

// Get size of cache object
_PROTOTYPE _EXTERN int cache_object_size(const unsigned int, const unsigned int, const unsigned int);

// Access object in cache (read only)
_PROTOTYPE _EXTERN const void *cache_access_object(const unsigned int, const unsigned int, const unsigned int);

// Map 2D (image) array to cache
_PROTOTYPE _EXTERN void **cache_map_2D_array(const void *, const unsigned int, const unsigned int, const unsigned long int);

// Write contents of cache to file
_PROTOTYPE _EXTERN int cache_write(const char *, const unsigned int);

// Get size of cached object
_PROTOTYPE _EXTERN long int cache_get_object_size(const unsigned int, const unsigned int);

// Get size of cache (object) block
_PROTOTYPE _EXTERN long int cache_get_block_size(const unsigned int);

// Number of blocks in cache
_PROTOTYPE _EXTERN long int cache_get_blocks(const unsigned int c_index);

// Number of objects per cache block
_PROTOTYPE _EXTERN long int cache_get_objects(const unsigned int c_index);

// Get cache (memory) map file name
_PROTOTYPE _EXTERN _BOOLEAN cache_get_mmap_name(char *, const unsigned int);

// Is cache memory mapped?
_PROTOTYPE _EXTERN _BOOLEAN cache_is_mapped(const unsigned int);

// Write cache mapping information
_PROTOTYPE _EXTERN int cache_write_mapinfo(const char *, const unsigned int);

// Archive cache
_PROTOTYPE _EXTERN int cache_archive(const _BOOLEAN, const _BOOLEAN, const char *);

// Uncompress cache
_PROTOTYPE _EXTERN int cache_extract(const char *);

// Read cache mapping information
_PROTOTYPE _EXTERN int cache_read_mapinfo(const char *, const unsigned int);

// Add data to cache block
_PROTOTYPE _EXTERN int cache_add_block(const void *,
                                       const unsigned long int,
                                       const unsigned int,
                                       const unsigned int,
                                       const unsigned int,
                                       const unsigned int);

// Delete block data from cache
_PROTOTYPE _EXTERN _BOOLEAN cache_delete_block(const unsigned int, const unsigned int);

// Clear cache
_PROTOTYPE _EXTERN int cache_clear(const unsigned int c_index);

// Mark cache block accessible
_PROTOTYPE _EXTERN _BOOLEAN cache_block_set_access(const _BOOLEAN, const unsigned int, const unsigned int);

// Set cache access state
_PROTOTYPE _EXTERN int cache_set_access(const _BOOLEAN, const unsigned int);

// Test cache access state
_PUBLIC _BOOLEAN cache_block_test_access(const unsigned int, const unsigned int);

// Resize cache
_PROTOTYPE _EXTERN int cache_resize(const unsigned int, const unsigned int);


#undef _EXPORT
#ifdef CPLUSPLUS
#   define _EXPORT extern C
#else
#   define _EXPORT extern
#endif
#endif /* Floret library */

