/*------------------------------------------------------------------------------
    Purpose: header for PUPS fast cache library

    Author:  M.A. O'Neill,
             Tumbling Dice,
             Gosforth,
             Tyne and Wear NE3 4RT.

    Version: 4.18
    Dated:   11th September 2024 
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

#define CACHELIB_VERSION        "4.18"


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

                    uint64_t          crc;                                          // CRC for cache buffer
                    unsigned char     path[SSIZE];                                  // Path to cache (in filesystem)
                    unsigned char     name[SSIZE];                                  // Name of this cache
                    unsigned char     mapinfo_name[SSIZE];                          // Name of memory mapped information file
                    unsigned char     mmap_name[SSIZE];                             // Name of memory mapped file
                    pthread_mutex_t   mutex;                                        // Access mutex
                    des_t             mapinfo_fd;                                   // File descriptor of mapinfo file
                    des_t             mmap_fd;                                      // File descriptor of memory mapped file
                    char              march[SSIZE];                                 // Machine architecture
                    char              auxinfo[SSIZE];                               // Auxilliary information
                    uint32_t          mmap;                                         // Memory mapping flags
                    uint32_t          u_blocks;                                     // Number of used blocks in cache
                    uint32_t          n_blocks;                                     // Number of object blocks in cache
                    uint32_t          n_objects;                                    // Number of objects in cache block
                    char              object_desc[MAX_CACHE_BLOCK_OBJECTS][SSIZE];  // Description of object
                    _BOOLEAN          busy;                                         // Cache busy
                    uint64_t          cache_size;                                   // Size of entire cache
                    uint64_t          block_size;                                   // Size of one cache block
                    uint64_t          object_offset[MAX_CACHE_BLOCK_OBJECTS];       // Offsets to objects in cache block
                    uint64_t          object_size[MAX_CACHE_BLOCK_OBJECTS];         // Sizes of cache block objects
                    uint32_t          colsize;                                      // Cache-cordination list size


                    /*---------------------------------*/
                    /* Specific to each block in cache */
                    /*---------------------------------*/

                    void              *cache_ptr;                                   // Pointer to base of contiguous block of cache memory
                    block_mtype       *blockmap;                                    // Pointer mapping  into cache block
                    uint32_t          *tag;                                         // Block tags
                    _BYTE             *flags;                                       // Block flags
                    uint64_t          *lifetime;                                    // Block lifetime (-1) - IMMORTAL implies block is not volatile
                    uint32_t          *hubness;                                     // Block hubness
                    uint32_t          *binding;                                     // Block binding 
                    pthread_rwlock_t  *rwlock;                                      // Block access rwlocks
               } cache_type;


/*-------------------*/
/* Taglist structure */
/*-------------------*/

typedef struct {    int32_t     tag;                                               // Block tag identifier
                    uint32_t    tagged_blocks;                                     // Blocks tagged with this tag identifier
               } taglist_type;



/*------------------------------------------------------------------------------
    Public variables exported by this module ...
------------------------------------------------------------------------------*/

#ifdef __NOT_LIB_SOURCE__
#else /* External variable declarations */
#   undef  _EXPORT
#   define _EXPORT
#endif /* __NOT_LIB_SOURCE__ */


/*------------------------------------------------------------------------------
    Public functions exported by this module ...
------------------------------------------------------------------------------*/


// Get index of cache (from its name)
_PROTOTYPE _EXTERN  int32_t cache_name2index(const _BOOLEAN, const char *);

// Get name of cache (from its index)
_PROTOTYPE _EXTERN  int32_t cache_index2name(const _BOOLEAN, char *, const uint32_t);

// Initialise the cache table [root thread]
_PROTOTYPE _EXTERN  int32_t cache_table_init(void);

// Add object to cache (prior to creating cache) [root thread]
_PROTOTYPE _EXTERN  int32_t cache_add_object(const _BOOLEAN, const char *, const uint64_t, const uint32_t);

// Create cache [root thread]
_PROTOTYPE _EXTERN  int32_t cache_create(const _BOOLEAN, const uint32_t, const char *,  int32_t, uint64_t *, const uint32_t);

// Sync cache 
_PROTOTYPE _EXTERN  int32_t cache_msync(const _BOOLEAN, const uint32_t);

// Cache live [root thread]
_PUBLIC  int32_t cache_live(const _BOOLEAN, const uint32_t);

// Cache dead [root thread]
_PUBLIC  int32_t cache_dead(const _BOOLEAN, const uint32_t);

// Destroy cache [root thread]
_PROTOTYPE _EXTERN  int32_t cache_destroy(const _BOOLEAN, const _BOOLEAN, const uint32_t);

// Display cache statistics [root thread]
_PROTOTYPE _EXTERN  int32_t cache_display_statistics(const _BOOLEAN, FILE *, const uint32_t);

// Display cache table [root thread]
_PROTOTYPE _EXTERN  int32_t cache_display(const _BOOLEAN, const FILE *);

// Get size of cache object [root thread]
_PROTOTYPE _EXTERN  int32_t cache_object_size(const _BOOLEAN, const uint32_t, const uint32_t, const uint32_t);

// Access object in cache
_PROTOTYPE _EXTERN const void *cache_access_object(const uint32_t, const uint32_t, const uint32_t, const uint32_t, const uint32_t);

// Map 2D (image) array to cache [root thread]
_PROTOTYPE _EXTERN void **cache_map_2D_array(const void *, const uint32_t, const uint32_t, const uint64_t);

// Write contents of cache to file [root thread] 
_PROTOTYPE _EXTERN  int32_t cache_write(const des_t, const _BOOLEAN, const uint32_t);

// Get size of cached object
_PROTOTYPE _EXTERN  int64_t  cache_get_object_size(const _BOOLEAN, const uint32_t, const uint32_t);

// Get size of cache (object) block
_PROTOTYPE _EXTERN  int64_t  cache_get_block_size(const _BOOLEAN, const uint32_t);

// Show statistics for all blocks in specified cache
_PROTOTYPE _EXTERN  int32_t cache_show_blocktag_stats(const _BOOLEAN, FILE *, const  int32_t);

// Reset all block (read/write) locks in cache 
_PROTOTYPE _EXTERN  int32_t cache_reset_blocklocks(const _BOOLEAN, const uint32_t);

// Number of blocks in cache
_PROTOTYPE _EXTERN  int32_t  cache_get_blocks(const _BOOLEAN, const uint32_t c_index);

// Number of objects per cache block
_PROTOTYPE _EXTERN  int32_t  cache_get_objects(const _BOOLEAN, const uint32_t c_index);

// Get mapinfo file name 
_PROTOTYPE _EXTERN  int32_t cache_get_mapinfo_name(const _BOOLEAN, char *, const uint32_t);

// Get cache (memory) map file name
_PROTOTYPE _EXTERN  int32_t cache_get_mmap_name(const _BOOLEAN, char *, const uint32_t);

// Is cache memory mapped?
_PROTOTYPE _EXTERN _BOOLEAN cache_is_mapped(const _BOOLEAN, const uint32_t);

// Is cache memory mapped at specified location (in file system)?
_PROTOTYPE _EXTERN _BOOLEAN cache_is_mapped_at(const _BOOLEAN, const char *, const uint32_t);

// Is cache (already) loaded?
_PROTOTYPE _EXTERN _BOOLEAN cache_already_loaded(const _BOOLEAN, const char *, const uint32_t, uint32_t *);

// Is cache preloaded?
_PROTOTYPE _EXTERN _BOOLEAN cache_is_preloaded(const _BOOLEAN, const uint32_t);

// Is cache allocated? 
_PROTOTYPE _EXTERN void *cache_is_allocated(const _BOOLEAN, const uint32_t);

// Write cache mapping information
_PROTOTYPE _EXTERN uint64_t cache_write_mapinfo(const _BOOLEAN, const _BOOLEAN, const char *, const uint32_t, const uint32_t);

// Read cache mapping information [root thread]
_PROTOTYPE _EXTERN uint64_t cache_read_mapinfo (const _BOOLEAN, const _BOOLEAN, const char *, const uint32_t, const uint32_t);

// Archive cache
_PROTOTYPE _EXTERN  int32_t cache_archive(const _BOOLEAN, const _BOOLEAN, const char *);

// Uncompress cache
_PROTOTYPE _EXTERN  int32_t cache_extract(const char *);

// Is cache block used? 
_PROTOTYPE _EXTERN _BOOLEAN cache_block_used(const _BOOLEAN, const uint32_t, const uint32_t);

// Get number of used blocks in cache
_PROTOTYPE _EXTERN  int32_t cache_blocks_used(const _BOOLEAN, const uint32_t);

// Reset number of used blocks in cache to zero
_PROTOTYPE _EXTERN  int32_t cache_blocks_reset_used(const _BOOLEAN, const uint32_t);

// Lock block
_PROTOTYPE _EXTERN  int32_t cache_lock_block(const uint32_t, const uint32_t, const uint32_t, const uint32_t);

// Unlock block
_PROTOTYPE _EXTERN  int32_t cache_unlock_block(const uint32_t, const uint32_t, const uint32_t);

// Set/reset cache used flag 
_PROTOTYPE _EXTERN  int32_t cache_block_set_used(const _BOOLEAN, const uint32_t, const uint32_t   , const _BOOLEAN);

// Is cache in use? 
_PROTOTYPE _EXTERN  int32_t cache_in_use(const _BOOLEAN,  const uint32_t);

// Is block in use - must lock block before calling this function
_PROTOTYPE _EXTERN _BOOLEAN cache_block_in_use(const _BOOLEAN, const uint32_t, const uint32_t);

// Set cache busy
_PROTOTYPE _EXTERN  int32_t cache_set_busy(const _BOOLEAN, const uint32_t, const _BOOLEAN);

// Is cache busy?
_PROTOTYPE _EXTERN _BOOLEAN cache_is_busy(const _BOOLEAN, const uint32_t);

// Add data (object) to cache block
_PROTOTYPE _EXTERN  int32_t cache_add_block(const void   *,
                                            const uint64_t,
                                            const uint32_t,
                                            const uint32_t,
                                            const  int32_t,
                                            const  int32_t,
                                            const _BOOLEAN,
                                            const uint32_t);

// Delete a block data from cache
_PROTOTYPE _EXTERN _BOOLEAN cache_delete_block(const _BOOLEAN, const _BOOLEAN, const uint32_t, const uint32_t);

// Restore a block of data to cache
_PROTOTYPE _EXTERN _BOOLEAN cache_restore_block(const _BOOLEAN, const _BOOLEAN, const uint32_t, const uint32_t);

// Clear cache
_PROTOTYPE _EXTERN  int32_t cache_clear(const _BOOLEAN, const uint32_t, const  int32_t);

// Merge pair of caches 
_PUBLIC  int32_t cache_merge(const _BOOLEAN, const uint32_t, const uint32_t, const  int32_t);

// Get string repesentation of blockag i.d
_PROTOTYPE _EXTERN  int32_t cache_get_blocktag_id_str(const uint32_t, char *);

// Get tag of specified cache block 
_PROTOTYPE _EXTERN  int32_t cache_get_blocktag(const _BOOLEAN, const uint32_t, const uint32_t);

// Set tag of specified cache block 
_PROTOTYPE _EXTERN  int32_t cache_set_blocktag(const _BOOLEAN, const uint32_t, const uint32_t, const uint32_t);

// Change tags of specified cache blocks
_PROTOTYPE _EXTERN  int32_t cache_change_blocktag(const _BOOLEAN, const uint32_t, const int32_t, const int32_t);

// Get lifetime of specified cache block 
_PROTOTYPE _EXTERN  int64_t cache_get_blocklifetime(const _BOOLEAN, const uint32_t, const uint32_t);

// Set lifetime of specified cache block 
_PROTOTYPE _EXTERN  int32_t cache_set_blocklifetime(const _BOOLEAN, const int64_t, const uint32_t, const uint32_t);

// Get hubness of specified cache block 
_PROTOTYPE _EXTERN uint32_t cache_get_blockhubness(const _BOOLEAN, const uint32_t, const uint32_t);

// Set hubness of specified cache block 
_PROTOTYPE _EXTERN  int32_t cache_set_blockhubness(const _BOOLEAN, const uint32_t, const uint32_t, const uint32_t);

// Get binding of specified cache block 
_PROTOTYPE _EXTERN uint32_t cache_get_blockbinding(const _BOOLEAN, const uint32_t, const uint32_t);

// Set binding of specified cache block 
_PROTOTYPE _EXTERN  int32_t cache_set_blockbinding(const _BOOLEAN, const uint32_t   , const uint32_t   , const uint32_t   );

// Get co-ordination list size for cache
_PROTOTYPE _EXTERN uint32_t cache_get_colsize(const _BOOLEAN, const uint32_t);

// Set co-ordination list size for cache
_PROTOTYPE _EXTERN int32_t cache_set_colsize(const _BOOLEAN, uint32_t, const uint32_t);

// Compact cache
_PROTOTYPE _EXTERN int32_t cache_compact(const _BOOLEAN, const  int32_t);

// Resize cache
_PROTOTYPE _EXTERN int32_t cache_resize(const _BOOLEAN, const uint32_t, const uint32_t);

// Lock cache
_PROTOTYPE _EXTERN int32_t cache_lock(const uint32_t);

// Unlock cache
_PROTOTYPE _EXTERN int32_t cache_unlock(const uint32_t);

// Is cache is private? (read only)
_PROTOTYPE _EXTERN int32_t cache_is_private(const _BOOLEAN, const uint32_t);

// Is cache is preloaded? 
_PROTOTYPE _EXTERN int32_t cache_is_private(const _BOOLEAN, const uint32_t);

// Set cache auxilliary data field
_PROTOTYPE _EXTERN int32_t cache_set_auxinfo(const _BOOLEAN, const char *, const uint32_t);

// Get cache auxilliary data field
_PROTOTYPE _EXTERN int32_t cache_get_auxinfo(const _BOOLEAN, char *, const uint32_t);

// Detach all caches 
_PROTOTYPE _EXTERN void cache_exit(void);


#undef _EXPORT
#ifdef CPLUSPLUS
#   define _EXPORT extern C
#else
#   define _EXPORT extern
#endif
#endif /* Floret library */

