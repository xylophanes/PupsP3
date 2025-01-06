/*------------------------------------------------------------------------------
    Purpose: header for meta virtual paged object library.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.02 
    Dated:   27th February 2024
    E-mail:  mao@tumblingdice.co.uk
------------------------------------------------------------------------------*/

#ifndef MVM_HDR
#define MVM_HDR


/*------------------*/
/* Keep c2man happy */
/*------------------*/

#include <c2man.h>

#ifndef __C2MAN__
#include <me.h>
#include <ansi.h>
#include <utils.h>
#endif /* __C2MAN__ */


/*--------------------------------------------*/
/* Defines used by meta virtual memory mapper */
/*--------------------------------------------*/
/***********/
/* Version */
/***********/

#define MVMLIB_VERSION       "2.02"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE                 2048


/*---------------------*/
/* MV table parameters */
/*---------------------*/

#define MVM_TABLE_SIZE        1024 
#define PAGE_NOT_USED         (-9999)


/*------------------------------------*/
/* Supported page scheduling policies */
/*------------------------------------*/

#define MVM_AGED_AND_ORDERED  1
#define MVM_ROUND_ROBIN       2


/*-----------------------*/
/* Mmemory access states */
/*-----------------------*/

#define MVM_READ_ONLY         1
#define MVM_READ_WRITE        2
 

/*------------*/
/* Lock flags */
/*------------*/

#define MVM_LOCK_FLAG_1       1
#define MVM_LOCK_FLAG_2       2


/*-----------------------------------*/
/* Allocation quantum for MVM system */
/*-----------------------------------*/

#define MVM_QUANTUM           16


/*-----------------------------*/
/* Offset for shared MVM files */
/*-----------------------------*/

#ifdef SUPPORT_SHARED_MVM
#define MVM_OFFSET            0 
#else
#define MVM_OFFSET            0
#endif /* SUPPORT_SHARED_MVM */



/*----------------------------------------------*/
/* Types required by meta virtual memory mapper */
/*----------------------------------------------*/


/*---------------------------------------*/
/* Page status information datastructure */
/*---------------------------------------*/

typedef struct {   uint32_t     v_page_usage;       // Times object has been accessed 
                   uint32_t     v_page;             // Index of object in virtual array
                   _BOOLEAN     locked;             // TRUE if page in use 
                   void         *v_page_location;   // Address of object
               } page_status_type;



/*------------------------------------------*/
/* Meta virtual memory mapper datastructure */
/*------------------------------------------*/

typedef struct {   char     name[SSIZE];            // Name of this virtual memory 
                   uint32_t v_page_slots;           // Number of pages in virtual memory 
                   uint32_t max_page_slots;         // Number of pages in physical memory
                   uint32_t page_slots;             // Number of pages in use
                   uint32_t next_lock;              // Number of items on lock list
                   uint32_t max_lock;               // Current maximum size of lock list
                   uint32_t ulist_ptr;              // Usage list pointer
                   uint32_t clock_ptr;              // Usage list clock pointer
                    int32_t r_w_state;              // Memory access (read/write) state
                    int32_t sched_policy;           // Page scehduling policy
                    int32_t current_lock_state;     // Current page lock state flag
                   des_t    fd;                     // Backing device descriptor
                    int64_t v_page_base;            // Base (backing) address of mvm pages
                    int64_t v_page_size;            // Size of page in bytes
                   _BOOLEAN aged_and_ordered;       // TRUE if aged and ordered
                   _BOOLEAN initialised;            // True if mvm memory initialised
                    int32_t *v_page_map;            // Mapping between phys and virt pages
                    int32_t *lock_map;              // Indices of locked physical pages
                   void     **vmem;                 // The virtual memory
                    int32_t *usage_map;             // Index physical pages by usage
                   page_status_type *page_status;   // Page status array
               } mvm_type;
 
 
 

/*----------------------------------------------*/
/* Public function prototype exported by module */
/*----------------------------------------------*/

#ifndef __NOT_LIB_SOURCE__

#else /* External variable declarations */
#   undef  _EXPORT
#   define _EXPORT
#endif




/*--------------------*/
/* Prototype bindings */
/*--------------------*/

// Show MVM objects attached to current application
_PROTOTYPE _EXPORT void mvm_show_mvm_objects(const FILE *);

// Show MVM object statistics
_PROTOTYPE _EXPORT void mvm_stat(const FILE *, const mvm_type *);

// Change cache size of meta virtual memory object
_PROTOTYPE _EXPORT void mvm_set_cache_size(int32_t, mvm_type);

// Page a dynamic (data) object
_PROTOTYPE _EXPORT int32_t mvm_page(uint32_t    ,        // Page to cache
                                    mvm_type   *);       // Meta virtual memory mapper


// Age and order pages of dynamic object
_PROTOTYPE _EXPORT int32_t mvm_age_and_order(mvm_type *);

// Initialise meta virtual memory object object
_PROTOTYPE _EXPORT void **mvm_init(const char      *,     // Name of virtual memory
                                   const _BOOLEAN   ,     // Memory initialisation flag
                                   const int32_t    ,     // Memory access type
                                   const int32_t    ,     // Page scheduling policy
                                   const uint32_t   ,     // Pages in virtual memory
                                   const uint32_t   ,     // Pages in physical memory
                                   const des_t      ,     // Backing store descriptor
                                   const size_t     ,     // Page size in virtual memory
                                   mvm_type        *);    // Virtual memory structure

// Destroy meta virtual memory object
_PROTOTYPE _EXPORT int32_t mvm_destroy(mvm_type *);

// Reset virtual cache after meta virtual memory access operations
_PROTOTYPE _EXPORT int32_t mvm_reset_pager(mvm_type *);

// Create swapfile associated with MVM object
_PROTOTYPE _EXPORT int32_t mvm_create_named_swapfile(const char *, const uint32_t, const size_t);

// Create MVM swapfile using open file handle
_PROTOTYPE _EXPORT int32_t mvm_create_swapfile(const int32_t, const uint32_t, const size_t);

// Delete swapfile associated with MVM object
_PROTOTYPE _EXPORT int32_t mvm_delete_swapfile(const int32_t, const char *);

// Mark memory as being initialised
_PROTOTYPE _EXPORT int32_t  mvm_initialised(mvm_type *);

// Set maximum (resident) cache for MVM object
_PUBLIC void mvm_change_cache_size(const uint32_t, mvm_type *mvm);


#undef _EXPORT
#ifdef CPLUSPLUS
#   define _EXPORT extern C
#else
#   define _EXPORT extern
#endif
#endif /* Meta virtual paged memory library */
