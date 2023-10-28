/*-------------------------------------------------------------------
    Header file for persistent heap library. A persistent heap is an
    area of data memory which may be mapped serially into the
    address spaces of multiple process.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.00 
    Dated:   4th January 2023
    Email:   mao@tumblingdice.co.uk
-------------------------------------------------------------------*/

#ifndef PHEAP_H
#define PHEAP_H


/*------------------*/
/* Keep c2man happy */
/*------------------*/

#include <c2man.h>

#ifndef __C2MAN__
#include <me.h>
#include <utils.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/errno.h>
#include <string.h>
#include <setjmp.h>
#endif /* __C2MAN__ */


/***********/
/* Version */
/***********/

#define PHEAP_VERSION    "2.00"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE              2048



/*--------------------------------------------*/
/* Definitions which are local to this module */
/*--------------------------------------------*/

#define PHM_MAP_LOCAL      1
#define PHM_MAP_GLOBAL     2

#define N_PARAMETERS       21 
#define MAGIC_OFFSET       19
#define HEAPMAGIC32BIG     0xff003200L
#define HEAPMAGIC64BIG     0xff006400L
#define HEAPMAGIC32LITTLE  0xff003200L
#define HEAPMAGIC64LITTLE  0xff006400L
#define MAX_PHEAPS         64 
#define PHM_SBRK_SIZE      65535 
#define PHM_BSTORE_SIZE    32000000
#define PHOBMAP_QUANTUM    128 
#define DEFAULT_MAX_TRYS   8


/*------------------*/
/* Heap disposition */
/*------------------*/

#define O_KEEP          0
#define O_DESTROY       128


/*-----------------------*/
/* Heap attachment flags */
/*-----------------------*/

#define ATTACH_PHEAP    1
#define CREATE_PHEAP    2
#define LIVE_PHEAP      4


/*-------------------------------------------------------*/
/* Offset operations (for process address space mapping) */
/*-------------------------------------------------------*/

#define ADD_OFFSET      0
#define SUBTRACT_OFFSET 1


/*--------------------------------*/
/* Lock recursion counter defines */
/*--------------------------------*/

#define LOCK_COUNTERS   256
#define DECREMENT       0
#define INCREMENT       1
#define GET             3


/*------------------------------*/
/* Write/update counter defines */
/*------------------------------*/

#define WRITE_COUNTERS  256
#define READ            0
#define WRITE           1
#define READWRITE       2
#define UPDATE          READWRITE


/*-------------------------------------------------------------*/
/* Definition of heap_type - the heap table is an aggregate of */
/* this structure                                              */
/*-------------------------------------------------------------*/

typedef struct {    int               fd;               // File descriptor for heap backing store file
                    int               flags;            // Flags (for associated descriptor)
                    int               m_cnt;            // Number of processes mapping heap 
                    long int          mtime;            // Lat modification time of heap file 
                    char              name[SSIZE];      // Name of heap backing store file 
                    int               ptrsize;          // Number of address bits for heap addresses
                    size_t            segment_size;     // Size of reserved virtual memory for heap
                    size_t            sdata;            // Address of first byte on heap
                    size_t            edata;            // Address of last byte on heap
                    unsigned long int heapmagic;        // Magic number for persistent heaps 
                    void              *addr;            // Base heap address in caller address space
                    _BOOLEAN          exists;           // TRUE if heap already exists
                    _BOOLEAN          addresses_local;  // TRUE if addresses local to attached process
                    _BOOLEAN          autodestruct;     // Heap autodestruct flag
               } heap_type;


/*------------------------------------------------------------*/
/* Object map - used to translate names to addresses and vice */
/* versa (on peristent heap)                                  */
/*------------------------------------------------------------*/

typedef struct {    void     *addr;                     // Address of persistent object
                    char     name[SSIZE];               // Name of persistent object
                    char     info[SSIZE];               // Object information string
                    int      pid;                       // Last PID to acquire lock
                    unsigned long int size;             // Size of object (in bytes)
               } phobmap_type;


/*---------------------------------------------------*/
/* Variables exported by the persistent heap library */
/*---------------------------------------------------*/

_EXPORT heap_type       *htable;
_EXPORT _BOOLEAN        do_msm_init;
_EXPORT pthread_mutex_t htab_mutex;
_EXPORT pthread_mutex_t phmalloc_mutex;


/*-----------------------------------------------*/
/* Functions exported by persistent heap library */
/*-----------------------------------------------*/

// Initialise persistent heap
_PROTOTYPE _EXTERN int msm_init(const int);

// Extend [grow] persistent heap table
_PROTOTYPE _EXTERN int msm_extend(const unsigned int, unsigned int);

// Attach a persistent heap to process returning its heap descriptor
_PROTOTYPE _EXTERN int msm_heap_attach(const char *, const int);

// Detach a persistent heap
_PROTOTYPE _EXTERN int msm_heap_detach(const unsigned int, const int);

// Extend the memory within a persistent heap
_PROTOTYPE _EXTERN void *msm_sbrk(const unsigned int, const size_t);

// Gather statistics on heap
_PROTOTYPE _EXTERN int msm_hstat(const unsigned int, heap_type *);

// Find first free named persistent object slot in the map area
_PROTOTYPE _EXTERN int msm_get_free_mapslot(const unsigned int);

// Find object in persistent object map
_PROTOTYPE _EXTERN int msm_find_mapped_object(const unsigned int, const void *);

// Add object to the persitent heap object map
_PROTOTYPE _EXTERN int msm_map_object(const unsigned int, const unsigned int h_index, const void *ptr, const char *name);

// Remove persisent object from object map
_PROTOTYPE _EXTERN int msm_unmap_object(const unsigned int hdes, const unsigned int h_index);

// Translate persistent object name to address
_PROTOTYPE _PUBLIC void *msm_map_objectname2addr(const unsigned int, const char *);

// Translate persistent object name to index
_PROTOTYPE _EXTERN int msm_map_objectname2index(const unsigned int, const char *);

// Translate persistent object map index to name
_PROTOTYPE _EXTERN char *msm_map_objectindex2name(const unsigned int, const unsigned int);

// Translate persistent object map index to address 
_PROTOTYPE _EXTERN void *msm_map_objectindex2addr(const unsigned int, const unsigned int);

// Map a (persistent) address to object table index
_PROTOTYPE _EXTERN int msm_map_objectaddr2index(const unsigned int, const void *);

// Is this mapname unique?
_PROTOTYPE _EXTERN int msm_mapname_unique(const unsigned int, const char *);

// Show persistent heap object table
_PROTOTYPE _EXTERN int msm_show_mapped_objects(const unsigned int, const FILE *);

// Show details of object in persistent heap
_PROTOTYPE _EXTERN  int msm_show_mapped_object(const unsigned int, const unsigned int, const FILE *);

// Translate persistent heap descriptor to name
_PROTOTYPE _EXTERN char *msm_hdes2name(const unsigned int);

// Translate persistent heap name to heap descriptor
_PROTOTYPE _EXTERN int msm_name2hdes(const char *);

// Translate persistent heap fdes (file descriptor) to heap descriptor
_PROTOTYPE _EXTERN int msm_fdes2hdes(int);

// Set info field of mapped object
_PROTOTYPE _EXTERN int map_setinfo(const unsigned int, const unsigned int, const char *); 

// Set the size field of mapped object
_PROTOTYPE _EXTERN int map_setsize(const unsigned int, const unsigned int, size_t);

// Inverse synchronisation of heap table
_PROTOTYPE _EXTERN int msm_isync_heaptables(const unsigned int);

// Synchronisation of heap table
_PROTOTYPE _EXTERN int msm_sync_heaptables(const unsigned int); 

// Print heaptable (debugging)
_PROTOTYPE _EXTERN int msm_print_heaptables(const FILE *, const unsigned int, const char *, const size_t);

// Detach all persistent heaps
_PROTOTYPE _EXTERN void msm_exit(const unsigned int);

// Mark a persistent heap for "autodestruct-on-detach"
_PROTOTYPE _EXTERN int msm_set_autodestruct_state(const unsigned int, const _BOOLEAN);

// See if a persistent object exists
_PROTOTYPE _EXTERN _BOOLEAN msm_phobject_exists(const unsigned int, const char *);


/*--------------------------------------------------------*/
/* Set pesistent heap addressing to local (PHM_MAP_LOCAL) */
/* or relative (PHM_MAP_REMOTE)                           */
/*--------------------------------------------------------*/

_PROTOTYPE _EXTERN int msm_map_address_mode(const unsigned int, const unsigned int);


// Translate heap name to heap table index
_PROTOTYPE _EXTERN int msm_name2index(const char *);

// Check if address is in heap
_PROTOTYPE _EXTERN _BOOLEAN msm_addr_in_heap(const unsigned int, const void *);

// Translate heap fdes to heap table index 
_PROTOTYPE _EXTERN int msm_fdes2hdes(const int fdes);

// Check to see if an address lies within persistent heap
_PROTOTYPE _EXTERN _BOOLEAN msm_addr_in_heap(const unsigned int hdes, const void *ptr);

// Convert global heap address to local process address
_PROTOTYPE _EXTERN void *msm_map_haddr_to_paddr(const unsigned int, const void *);

// Convert local process address to global heap address
_PROTOTYPE _EXTERN void *msm_map_paddr_to_haddr(const unsigned int, const void *);

// Save state of heap
_PROTOTYPE _EXTERN int msm_save_heapstate(const char *);

#endif /* PHEAP_H */

