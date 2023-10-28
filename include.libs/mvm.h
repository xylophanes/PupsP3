/*------------------------------------------------------------------------------
    Purpose: header for meta virtual paged object library.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.00 
    Dated:   4th January 2023
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

#define MVMLIB_VERSION       "2.00"


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

typedef struct {   int       v_page_usage;          // Times object has been accessed 
                   int             v_page;          // Index of object in virtual array
                   _BOOLEAN        locked;          // TRUE if page in use 
                   void  *v_page_location;          // Address of object
               } page_status_type;



/*------------------------------------------*/
/* Meta virtual memory mapper datastructure */
/*------------------------------------------*/

typedef struct {   char     name[SSIZE];            // Name of this virtual memory 
                   int      v_page_slots;           // Number of pages in virtual memory 
                   int      max_page_slots;         // Number of pages in physical memory
                   int      page_slots;             // Number of pages in use
                   int      next_lock;              // Number of items on lock list
                   int      max_lock;               // Current maximum size of lock list
                   int      ulist_ptr;              // Usage list pointer
                   int      clock_ptr;              // Usage list clock pointer
                   int      r_w_state;              // Memory access (read/write) state
                   int      sched_policy;           // Page scehduling policy
                   int      current_lock_state;     // Current page lock state flag
                   int      fd;                     // Backing device descriptor
                   long int v_page_base;            // Base (backing) address of mvm pages
                   long int v_page_size;            // Size of page in bytes
                   _BOOLEAN aged_and_ordered;       // TRUE if aged and ordered
                   _BOOLEAN initialised;            // True if mvm memory initialised
                   int      *v_page_map;            // Mapping between phys and virt pages
                   int      *lock_map;              // Indices of locked physical pages
                   void     **vmem;                 // The virtual memory
                   int      *usage_map;             // Index physical pages by usage
                   page_status_type *page_status;   // Page status array
               } mvm_type;
 
 
 

/*------------------------------------------------------------------------------
    Public function prototype exported by module ...
------------------------------------------------------------------------------*/

#ifndef __NOT_LIB_SOURCE__

#else /* External variable declarations */
#   undef  _EXPORT
#   define _EXPORT
#endif




/*------------------------------------------------------------------------------
    Prototype bindings ...
------------------------------------------------------------------------------*/

// Show MVM objects attached to current application
_PROTOTYPE _EXPORT void mvm_show_mvm_objects(FILE *);

// Show meta virtual memory object statistics
_PROTOTYPE _EXPORT void show_mvm(mvm_type *);

// Change cache size of meta virtual memory object
_PROTOTYPE _EXPORT void mvm_set_cache_size(int, mvm_type);

// Page a dynamic (data) object
_PROTOTYPE _EXPORT int mvm_page(int        ,        // Page to cache
                                mvm_type  *);       // Meta virtual memory mapper


// Age and order pages of dynamic object
_PROTOTYPE _EXPORT int mvm_age_and_order(mvm_type *);

// Initialise meta virtual memory object object
_PROTOTYPE _EXPORT void **mvm_init(char      *,     // Name of virtual memory
                                   _BOOLEAN   ,     // Memory initialisation flag
                                   int        ,     // Memeory access type
                                   int        ,     // Page scheduling policy
                                   int        ,     // Pages in virtual memory
                                   int        ,     // Pages in physical memory
                                   int        ,     // Backing store descriptor
                                   long       ,     // Page size in virtual memory
                                   mvm_type  *);    // Virtual memory structure

// Destroy meta virtual memory object
_PROTOTYPE _EXPORT int mvm_destroy(mvm_type *);

// Reset virtual cache after meta virtual memory access operations
_PROTOTYPE _EXPORT int mvm_reset_pager(mvm_type *);

// Create swapfile associated with MVM object
_PROTOTYPE _EXPORT int mvm_create_named_swapfile(char *, int, long);

// Create MVM swapfile using open file handle
_PROTOTYPE _EXPORT int mvm_create_swapfile(int, int, long);

// Delete swapfile associated with MVM object
_PROTOTYPE _EXPORT int mvm_delete_swapfile(int, char *);

// Mark memory as being initialised
_PROTOTYPE _EXPORT int  mvm_initialised(mvm_type *);


#undef _EXPORT
#ifdef CPLUSPLUS
#   define _EXPORT extern C
#else
#   define _EXPORT extern
#endif
#endif /* Meta virtual paged memory library */
