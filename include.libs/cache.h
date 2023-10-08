/*------------------------------------------------------------------------------
    Purpose: header for PUPS fast cache library

    Author:  M.A. O'Neill,
             Tumbling Dice,
             Gosforth,
             Tyne and Wear NE3 4RT.

    Version: 4.14 
    Dated:   18th September 2023 
    E-mail:  mao@tumblingdice.co.uk
------------------------------------------------------------------------------*/

#ifndef CACHELIB_HDR
#define CACHELIB_HDR

#include <pthread.h>


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
/***********/
/* Version */
/***********/

#define CACHELIB_VERSION        "4.12"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE                   2048


// Maximum caches in the cache table
#define MAX_CACHES              256 

// Maximum number of objects per cache block
#define MAX_CACHE_BLOCK_OBJECTS 256 


// Definitions of byte storage units
#define KILOBYTE                1024
#define MEGABYTE                (1024*1024)
#define GIGABYTE                (1024*1024*1024)


// Block allocation quantum (for extending cache
#define BLOCK_ALLOC_QUANTUM     8192 


// Block access and usage flags
#define BLOCK_HAVELOCK          (1 <<  0)
#define BLOCK_NOLOCK            (1 <<  0)
#define BLOCK_USED              (1 <<  1) 
#define BLOCK_WRLOCK            (1 <<  2) 
#define BLOCK_RDLOCK            (1 <<  3) 
#define BLOCK_LOADED            (1 <<  4) 
#define ANY_CACHE_BLOCK         (-1     ) 
#define ALL_CACHE_BLOCKS        (-1     ) 


// Taglist size
#define MAX_TAGLIST_SIZE        4096


// Tag flags
#define DEFAULT_TAG             0
#define SCRATCH_TAG             9999
#define ALL_CACHE_TAGS          (-1)

// Block flags
#define BLOCK_IMMORTAL          (-1)

// Cache archive flags
#define COMPRESS                (1 <<  0)
#define DELETE                  (1 <<  1) 


// Cache mapping flags
#define CACHE_MMAP              (1 <<  0)
#define CACHE_PRIVATE           (1 <<  1)
#define CACHE_PUBLIC            (1 <<  2)
#define CACHE_POPULATE          (1 <<  3)
#define CACHE_DEPOPULATE        (1 <<  4)
#define CACHE_USE_MAPINFO       (1 <<  5) 
#define CACHE_BUSY              (1 <<  6)
#define CACHE_BUILD             (1 <<  7)
#define CACHE_EXTEND            (1 <<  8)
#define CACHE_DELETE_IFILES     (1 <<  9)
#define CACHE_LOCK              (1 << 10)
#define CACHE_UNLOCK            (1 << 11)
#define CACHE_HAVELOCK          (1 << 12)
#define CACHE_WRLOCK            (1 << 13)
#define CACHE_RDLOCK            (1 << 14)
#define CACHE_LIVE              (1 << 15)


/*-----------------------------------------*/
/*  Types which are defined by this module */ 
/*-----------------------------------------*/
/*-------------------------*/
/* Dynamic cache structure */
/*-------------------------*/

typedef struct { void     *object_ptr[MAX_CACHE_BLOCK_OBJECTS];                     // Pointers to objects within cache block
               } block_mtype;



/*-----------------*/
/* Cache structure */
/*-----------------*/

typedef struct {    

                    /*-------------------------------*/
                    /* Common to all blocks in cache */
                    /*-------------------------------*/

                    unsigned long int crc;                                          // CRC for cache buffer
                    unsigned char     path[SSIZE];                                  // Path to cache (in filesystem)
                    unsigned char     name[SSIZE];                                  // Name of this cache
                    unsigned char     mapinfo_name[SSIZE];                          // Name of memory mapped information file
                    unsigned char     mmap_name[SSIZE];                             // Name of memory mapped file
                    pthread_mutex_t   mutex;                                        // Access mutex
                    int               mapinfo_fd;                                   // File descriptor of mapinfo file
                    int               mmap_fd;                                      // File descriptor of memory mapped file
                    char              march[SSIZE];                                 // Machine architecture
                    char              auxinfo[SSIZE];                               // Auxilliary information
                    unsigned int      mmap;                                         // Memory mapping flags
                    int               u_blocks;                                     // Number of used blocks in cache
                    int               n_blocks;                                     // Number of object blocks in cache
                    int               n_objects;                                    // Number of objects in cache block
                    char              object_desc[MAX_CACHE_BLOCK_OBJECTS][SSIZE];  // Description of object
                    _BOOLEAN          busy;                                         // Cache busy
                    unsigned long int cache_size;                                   // Size of entire cache
                    unsigned long int block_size;                                   // Size of one cache block
                    unsigned long int object_offset[MAX_CACHE_BLOCK_OBJECTS];       // Offsets to objects in cache block
                    unsigned long int object_size[MAX_CACHE_BLOCK_OBJECTS];         // Sizes of cache block objects
                    unsigned int      colsize;                                      // Cache-cordination list size


                    /*---------------------------------*/
                    /* Specific to each block in cache */
                    /*---------------------------------*/

                    void              *cache_ptr;                                   // Pointer to base of contiguous block of cache memory
                    block_mtype       *blockmap;                                    // Pointer mapping into cache block
                    unsigned int      *tag;                                         // Block tags
                    _BYTE             *flags;                                       // Block flags
                    int               *lifetime;                                    // Block lifetime (-1) - IMMORTAL implies block is not volatile
                    unsigned int      *hubness;                                     // Block hubness
                    unsigned int      *binding;                                     // Block binding 
                    pthread_rwlock_t  *rwlock;                                      // Block access rwlocks
               } cache_type;


/*-------------------*/
/* Taglist structure */
/*-------------------*/

typedef struct {    int          tag;                                               // Block tag identifier
                    unsigned int tagged_blocks;                                     // Blocks tagged with this tag identifier
               } taglist_type;



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
_PROTOTYPE _EXTERN int cache_name2index(const _BOOLEAN, const char *);

// Get name of cache (from its index)
_PROTOTYPE _EXTERN int cache_index2name(const _BOOLEAN, char *, const unsigned int);

// Initialise the cache table
_PROTOTYPE _EXTERN int cache_table_init(void);

// Add object to cache (prior to creating cache)
_PROTOTYPE _EXTERN int cache_add_object(const _BOOLEAN, const char *, const unsigned long int, const unsigned int);

// Create cache
_PROTOTYPE _EXTERN int cache_create(const _BOOLEAN, const unsigned int, const char *, int, unsigned long int *, const unsigned int);

// Sync cache */
_PROTOTYPE _EXTERN int cache_msync(const _BOOLEAN, const unsigned int);

// Cache live
_PUBLIC int cache_live(const _BOOLEAN, const unsigned int);

// Cache dead 
_PUBLIC int cache_dead(const _BOOLEAN, const unsigned int);

// Destroy cache
_PROTOTYPE _EXTERN int cache_destroy(const _BOOLEAN, const _BOOLEAN, const unsigned int);

// Display cache statistics
_PROTOTYPE _EXTERN int cache_display_statistics(const _BOOLEAN, FILE *, const unsigned int);

// Display cache table 
_PROTOTYPE _EXTERN int cache_display(const _BOOLEAN, const FILE *);

// Get size of cache object
_PROTOTYPE _EXTERN int cache_object_size(const _BOOLEAN, const unsigned int, const unsigned int, const unsigned int);

// Access object in cache
_PROTOTYPE _EXTERN const void *cache_access_object(const unsigned int, const unsigned int, const unsigned int, const unsigned int, const unsigned int);

// Map 2D (image) array to cache
_PROTOTYPE _EXTERN void **cache_map_2D_array(const void *, const unsigned int, const unsigned int, const unsigned long int);

// Write contents of cache to file
_PROTOTYPE _EXTERN int cache_write(const int, const _BOOLEAN, const unsigned int);

// Get size of cached object
_PROTOTYPE _EXTERN long int cache_get_object_size(const _BOOLEAN, const unsigned int, const unsigned int);

// Get size of cache (object) block
_PROTOTYPE _EXTERN long int cache_get_block_size(const _BOOLEAN, const unsigned int);

// Show statistics for all blocks in specified cache
_PROTOTYPE _EXTERN int cache_show_blocktag_stats(const _BOOLEAN, FILE *, const int);

// Reset all block (read/write) locks in cache 
_PROTOTYPE _EXTERN int cache_reset_blocklocks(const _BOOLEAN, const unsigned int);

// Number of blocks in cache
_PROTOTYPE _EXTERN long int cache_get_blocks(const _BOOLEAN, const unsigned int c_index);

// Number of objects per cache block
_PROTOTYPE _EXTERN long int cache_get_objects(const _BOOLEAN, const unsigned int c_index);

// Get mapinfo file name 
_PROTOTYPE _EXTERN int cache_get_mapinfo_name(const _BOOLEAN, char *, const unsigned int);

// Get cache (memory) map file name
_PROTOTYPE _EXTERN int cache_get_mmap_name(const _BOOLEAN, char *, const unsigned int);

// Is cache memory mapped?
_PROTOTYPE _EXTERN _BOOLEAN cache_is_mapped(const _BOOLEAN, const unsigned int);

// Is cache memory mapped at specified location (in file system)? */
_PROTOTYPE _EXTERN _BOOLEAN cache_is_mapped_at(const _BOOLEAN, const char *, const unsigned int);

// Is cache (already) loaded?
_PROTOTYPE _EXTERN _BOOLEAN cache_already_loaded(const _BOOLEAN, const char *, const unsigned int, unsigned int *);

// Is cache allocated? 
_PROTOTYPE _EXTERN void *cache_is_allocated(const _BOOLEAN, const unsigned int);

// Write cache mapping information
_PROTOTYPE _EXTERN unsigned long int cache_write_mapinfo(const _BOOLEAN, const _BOOLEAN, const char *, const unsigned int, const unsigned int);

// Read cache mapping information
_PROTOTYPE _EXTERN unsigned long int cache_read_mapinfo (const _BOOLEAN, const _BOOLEAN, const char *, const unsigned int, const unsigned int);

// Archive cache
_PROTOTYPE _EXTERN int cache_archive(const _BOOLEAN, const _BOOLEAN, const char *);

// Uncompress cache
_PROTOTYPE _EXTERN int cache_extract(const char *);

// Is cache block used? 
_PROTOTYPE _EXTERN _BOOLEAN cache_block_used(const _BOOLEAN, const unsigned int, const unsigned int);

// Get number of used blocks in cache
_PROTOTYPE _EXTERN int cache_blocks_used(const _BOOLEAN, const unsigned int);

// Reset number of used blocks in cache to zero
_PROTOTYPE _EXTERN int cache_blocks_reset_used(const _BOOLEAN, const unsigned int);

// Lock block
_PROTOTYPE _EXTERN int cache_lock_block(const unsigned int, const unsigned int, const unsigned int, const unsigned int);

// Unlock block
_PROTOTYPE _EXTERN int cache_unlock_block(const unsigned int, const unsigned int, const unsigned int);

// Set/reset cache used flag */
_PROTOTYPE _EXTERN int cache_block_set_used(const _BOOLEAN, const unsigned int, const unsigned int, const _BOOLEAN);

// Is cache in use? */
_PROTOTYPE _EXTERN int cache_in_use(const _BOOLEAN,  const unsigned int);

// Is block in use - must lock block before calling this function
_PROTOTYPE _EXTERN _BOOLEAN cache_block_in_use(const _BOOLEAN, const unsigned int, const unsigned int);

// Set cache busy
_PROTOTYPE _EXTERN int cache_set_busy(const _BOOLEAN, const unsigned int, const _BOOLEAN);

// Is cache busy?
_PROTOTYPE _EXTERN _BOOLEAN cache_is_busy(const _BOOLEAN, const unsigned int);

// Add data (object) to cache block
_PROTOTYPE _EXTERN int cache_add_block(const void *,
                                       const unsigned long int,
                                       const unsigned int,
                                       const unsigned int,
                                       const int,
                                       const int,
                                       const _BOOLEAN,
                                       const unsigned int);

// Delete a block data from cache
_PROTOTYPE _EXTERN _BOOLEAN cache_delete_block(const _BOOLEAN, const _BOOLEAN, const unsigned int, const unsigned int);

// Restore a block of data to cache
_PROTOTYPE _EXTERN _BOOLEAN cache_restore_block(const _BOOLEAN, const _BOOLEAN, const unsigned int, const unsigned int);

// Clear cache
_PROTOTYPE _EXTERN int cache_clear(const _BOOLEAN, const unsigned int, const int);

// Merge pair of caches */
_PUBLIC int cache_merge(const _BOOLEAN, const unsigned int, const unsigned int, const int);

// Get string repesentation of blockag i.d
_PROTOTYPE _EXTERN int cache_get_blocktag_id_str(const unsigned int, char *);

// Get tag of specified cache block 
_PROTOTYPE _EXTERN int cache_get_blocktag(const _BOOLEAN, const unsigned int, const unsigned int);

// Set tag of specified cache block 
_PROTOTYPE _EXTERN int cache_set_blocktag(const _BOOLEAN, const unsigned int, const unsigned int, const unsigned int);

// Change tags of specified cache blocks
_PROTOTYPE _EXTERN int cache_change_blocktag(const _BOOLEAN, const unsigned int, const int, const int);

// Get lifetime of specified cache block 
_PROTOTYPE _EXTERN int cache_get_blocklifetime(const _BOOLEAN, const unsigned int, const unsigned int);

// Set lifetime of specified cache block 
_PROTOTYPE _EXTERN int cache_set_blocklifetime(const _BOOLEAN, const int, const unsigned int, const unsigned int);

// Get hubness of specified cache block 
_PROTOTYPE _EXTERN unsigned int cache_get_blockhubness(const _BOOLEAN, const unsigned int, const unsigned int);

// Set hubness of specified cache block 
_PROTOTYPE _EXTERN int cache_set_blockhubness(const _BOOLEAN, const unsigned int, const unsigned int, const unsigned int);

// Get binding of specified cache block 
_PROTOTYPE _EXTERN unsigned int cache_get_blockbinding(const _BOOLEAN, const unsigned int, const unsigned int);

// Set binding of specified cache block 
_PROTOTYPE _EXTERN int cache_set_blockbinding(const _BOOLEAN, const unsigned int, const unsigned int, const unsigned int);

// Get co-ordination list size for cache
_PROTOTYPE _EXTERN unsigned int cache_get_colsize(const _BOOLEAN, const unsigned int);

// Set co-ordination list size for cache
_PROTOTYPE _EXTERN int cache_set_colsize(const _BOOLEAN, unsigned int, const unsigned int);

// Compact cache
_PROTOTYPE _EXTERN int cache_compact(const _BOOLEAN, const int);

// Resize cache
_PROTOTYPE _EXTERN int cache_resize(const _BOOLEAN, const unsigned int, const unsigned int);

// Lock cache
_PROTOTYPE _EXTERN int cache_lock(const unsigned int);

// Unlock cache
_PROTOTYPE _EXTERN int cache_unlock(const unsigned int);

// Is cache is private? (read only)
_PROTOTYPE _EXTERN int cache_is_private(const _BOOLEAN, const unsigned int);

// Is cache is preloaded? 
_PROTOTYPE _EXTERN int cache_is_private(const _BOOLEAN, const unsigned int);

// Set cache auxilliary data field
_PROTOTYPE _EXTERN int cache_set_auxinfo(const _BOOLEAN, const char *, const unsigned int);

// Get cache auxilliary data field
_PROTOTYPE _EXTERN int cache_get_auxinfo(const _BOOLEAN, char *, const unsigned int);


#undef _EXPORT
#ifdef CPLUSPLUS
#   define _EXPORT extern C
#else
#   define _EXPORT extern
#endif
#endif /* Floret library */

