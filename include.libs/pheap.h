/*------------------------------------------------------------------
    Header file for persistent heap library. A persistent heap is an
    area of data memory which may be mapped serially  int32_to the
    address spaces of multiple process.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.03
    Dated:   12th December 2024
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

#define PHEAP_VERSION    "2.03"


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

#define ATTACH_PHEAP    (1 << 0)
#define CREATE_PHEAP    (1 << 2) 
#define LIVE_PHEAP      (1 << 3) 


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

typedef struct {    des_t             fd;               // File descriptor for heap backing store file
                    int32_t           flags;            // Flags (for associated descriptor)
                    uint32_t          m_cnt;            // Number of processes mapping heap 
                    int64_t           mtime;            // Lat modification time of heap file 
                    char              name[SSIZE];      // Name of heap backing store file 
                    uint32_t          ptrsize;          // Number of address bits for heap addresses
                    size_t            segment_size;     // Size of reserved virtual memory for heap
                    size_t            sdata;            // Address of first byte on heap
                    size_t            edata;            // Address of last byte on heap
                    uint64_t          heapmagic;        // Magic number for persistent heaps 
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
                    pid_t    pid;                       // Last PID to acquire lock
                    size_t   size;                      // Size of object (in bytes)
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

// Initialise persistent heap [root thread]
_PROTOTYPE _EXTERN int32_t msm_init(const int32_t);

// Extend [grow] persistent heap table [root thread]
_PROTOTYPE _EXTERN int32_t msm_extend(const uint32_t, uint32_t);

// Attach a persistent heap to process returning its heap descriptor [root thread]
_PROTOTYPE _EXTERN int32_t msm_heap_attach(const char *, const int32_t);

// Detach a persistent heap [root thread]
_PROTOTYPE _EXTERN int32_t msm_heap_detach(const uint32_t, const  int32_t);

// Extend the memory within a persistent heap
_PROTOTYPE _EXTERN void *msm_sbrk(const uint32_t, const size_t);

// Gather statistics on heap
_PROTOTYPE _EXTERN int32_t msm_hstat(const uint32_t, heap_type *);

// Find first free named persistent object slot in the map area
_PROTOTYPE _EXTERN int32_t msm_get_free_mapslot(const uint32_t);

// Find object in persistent object map
_PROTOTYPE _EXTERN int32_t msm_find_mapped_object(const uint32_t, const void *);

// Add object to the persitent heap object map
_PROTOTYPE _EXTERN int32_t msm_map_object(const uint32_t, const uint32_t, const void *, const char *);

// Remove persisent object from object map
_PROTOTYPE _EXTERN int32_t msm_unmap_object(const uint32_t, const uint32_t);

// Translate persistent object name to address
_PROTOTYPE _PUBLIC void *msm_map_objectname2addr(const uint32_t, const char *);

// Translate persistent object name to index
_PROTOTYPE _EXTERN int32_t msm_map_objectname2index(const uint32_t, const char *);

// Translate persistent object map index to name
_PROTOTYPE _EXTERN char *msm_map_objectindex2name(const uint32_t, const uint32_t);

// Translate persistent object map index to address 
_PROTOTYPE _EXTERN void *msm_map_objectindex2addr(const uint32_t, const uint32_t);

// Map a (persistent) address to object table index
_PROTOTYPE _EXTERN int32_t msm_map_objectaddr2index(const uint32_t, const void *);

// Is this mapname unique?
_PROTOTYPE _EXTERN int32_t msm_mapname_unique(const uint32_t, const char *);

// Show persistent heap object table [root thread]
_PROTOTYPE _EXTERN int32_t msm_show_mapped_objects(const uint32_t, const FILE *);

// Show details of object in persistent heap
_PROTOTYPE _EXTERN int32_t msm_show_mapped_object(const uint32_t, const uint32_t, const FILE *);

// Translate persistent heap descriptor to name
_PROTOTYPE _EXTERN char *msm_hdes2name(const uint32_t);

// Translate persistent heap name to heap descriptor
_PROTOTYPE _EXTERN int32_t msm_name2hdes(const char *);

// Translate persistent heap fdes (file descriptor) to heap descriptor
_PROTOTYPE _EXTERN int32_t msm_fdes2hdes(const des_t);

// Set info field of mapped object
_PROTOTYPE _EXTERN int32_t map_setinfo(const uint32_t, const uint32_t, const char *); 

// Set the size field of mapped object
_PROTOTYPE _EXTERN int32_t map_setsize(const uint32_t, const uint32_t, size_t);

// Inverse synchronisation of heap table
_PROTOTYPE _EXTERN int32_t msm_isync_heaptables(const uint32_t);

// Synchronisation of heap table
_PROTOTYPE _EXTERN int32_t msm_sync_heaptables(const uint32_t); 

// Print heaptable (debugging) [root thread]
_PROTOTYPE _EXTERN int32_t msm_print_heaptables(const FILE *, const uint32_t, const char *, const size_t);

// Detach all persistent heaps [root thread[
_PROTOTYPE _EXTERN void msm_exit(const uint32_t);

// Mark a persistent heap for "autodestruct-on-detach" [root thread]
_PROTOTYPE _EXTERN int32_t msm_set_autodestruct_state(const uint32_t, const _BOOLEAN);

// See if a persistent object exists
_PROTOTYPE _EXTERN _BOOLEAN msm_phobject_exists(const uint32_t, const char *);

// Set pesistent heap addressing to local (PHM_MAP_LOCAL) or relative (PHM_MAP_REMOTE) 
_PROTOTYPE _EXTERN int32_t msm_map_address_mode(const uint32_t, const uint32_t);

// Translate heap name to heap table index
_PROTOTYPE _EXTERN int32_t msm_name2index(const char *);

// Check if address is in heap
_PROTOTYPE _EXTERN _BOOLEAN msm_addr_in_heap(const uint32_t, const void *);

// Translate heap fdes to heap table index 
_PROTOTYPE _EXTERN int32_t msm_fdes2hdes(const des_t fdes);

// Check to see if an address lies within persistent heap
_PROTOTYPE _EXTERN _BOOLEAN msm_addr_in_heap(const uint32_t, const void *);

// Convert global heap address to local process address
_PROTOTYPE _EXTERN void *msm_map_haddr_to_paddr(const uint32_t, const void *);

// Convert local process address to global heap address
_PROTOTYPE _EXTERN void *msm_map_paddr_to_haddr(const uint32_t, const void *);

// Save state of heap
_PROTOTYPE _EXTERN int32_t msm_save_heapstate(const char *);

#endif /* PHEAP_H */

