/*-------------------------------------------------------------------------------------------------
    The PUPS bubble memory library.
    (C) M.A. O'Neill, 12th December 2024, mao@tumblingdice.co.uk
-------------------------------------------------------------------------------------------------*/

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

/*--------------------------*/
/* Standard integer formats */
/*--------------------------*/

#include <stdint.h>


#define	MAXLEN		1024

/*---------------------------------------------------------------------------- 

   Each memory block has 5 extra 32-bit values associated with it.  If malloc
   returns the pointer p to you, the state really looks like:

jmal(p)------>  |------------------------------------------------------------|
                | flink (next malloc block in absolute order)                |
                |------------------------------------------------------------|
                | blink (prev malloc block in absolute order)                |
                |------------------------------------------------------------|
                |------------------------------------------------------------|
                | mmap size (size of mapped region)                          |
                |------------------------------------------------------------|
                | size (size of memory allocated in mapped region)           |
p------------>  |------------------------------------------------------------|
                | space: the memory block                                    |
                |                  ...                                       |
                |                  ...                                       |
                |------------------------------------------------------------|

----------------------------------------------------------------------------*/


/*--------------------*/
/* String/buffer size */
/*--------------------*/

#define SSIZE 2048

/*-------------------------------------*/
/* Definition of the Jmalloc structure */
/*-------------------------------------*/

typedef struct jmalloc {
    struct jmalloc *flink;
    struct jmalloc *blink;
    size_t         mmap_size;
    size_t         size;
    void           *space;
} *Jmalloc;




/*-----------------------------------------------------------------*/ 
/* Filetable structure. Holds information about file table objects */
/*-----------------------------------------------------------------*/

typedef struct ftable_struct {
    int32_t       inuse;
    int32_t      type;
    char          filename[SSIZE];
    int32_t       size;
    unsigned char buf[SSIZE];
    int32_t       attribute;
    int32_t       mode;
    int32_t       fd;
    off_t         seek_ptr;
} ftable_entry;

#endif /* TYPEDEFS_H */
