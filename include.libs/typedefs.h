/*-------------------------------------------------------------------------------------------------
    The PUPS bubble memory library.
    (C) M.A. O'Neill, 8th June 2018, mao@tumblingdice.co.uk
-------------------------------------------------------------------------------------------------*/

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

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
  int      inuse;
  int      type;
  char     filename[SSIZE];
  int      size;
  unsigned char buf[SSIZE];
  int      attribute;
  int      mode;
  int      fd;
  off_t    seek_ptr;
} ftable_entry;



/*---------------------------------------------------------------------------*/
/*  This is the path _ckpt_recover() uses to find ckpt.? files.  It's        */
/*  embedded in a struct so it can be passed into / returned from functions. */
/*---------------------------------------------------------------------------*/

typedef struct pname {
  char name[SSIZE];
} pname_type;

#endif /* TYPEDEFS_H */
