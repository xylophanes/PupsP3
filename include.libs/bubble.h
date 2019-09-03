/*-----------------------------------------------------------------------------
    Purpose: Header for bubble memory allocation library.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    E-Mail:  mao@tumblingdice.co.uk
    Dated:   30th August 2019 
    Version: 2.00 
------------------------------------------------------------------------------*/


#ifndef BUBBLE_H
#define BUBBLE_H

#include <stdio.h>

#ifdef PUPS_SUPPORT
#define __NOT_LIB_SOURCE__
#include <xtypes.h>
#include <utils.h>
#endif /* PUPS_SUPPORT */

#include <setjmp.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <signal.h>
#include <malloc.h>


#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#ifdef THREAD_SUPPORT
#include <pthread.h>
#endif /* THREAD_SUPPORT */

#include <stdlib.h>

                              /*-------------------------*/
#ifndef SYSTEM_C              /* don't include these two */
#include <unistd.h>           /* files in system.c       */
#endif /* SYSTEM_C */         /*-------------------------*/

#include <fcntl.h>


#define MAXLEN          1024


/*----------------------------------*/
/* Used to be included from local.h */
/*----------------------------------*/

#define _CKPT_FTABLE_SIZE 1024
#define GPAGESIZE ((unsigned long)(4096))


/*---------------------------------------------------------*/
/* Jmalloc structure - used to be included from typedefs.h */
/*---------------------------------------------------------*/


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


#ifdef PUPS_SUPPORT
#ifdef  __STDC__
/*------------------------*/
/* functions in jmalloc.c */
/*------------------------*/

_PROTOTYPE _PUBLIC void *mmap_sbrk(unsigned long);
_PROTOTYPE _PUBLIC int  mallopt(int, int);
_PROTOTYPE _PUBLIC void jmalloc_usage(int, FILE *);
#endif /* __STDC__ */
#endif /* PUPS_SUPPORT */


                              /*-----------------------------------------------*/
#define MUNMAP_THRESHOLD 1    /* Memory bubble (minimum) utilisation threshold */
                              /*-----------------------------------------------*/


/*-------------------------------------*/
/* Define TRUE and FALSE if we need to */
/*-------------------------------------*/

#ifndef TRUE
#define TRUE		1
#endif /* TRUE /

#ifndef FALSE
#define FALSE		0
#endif /*  FALSE */


/*------------------------*/
/* Some macro definitions */
/*------------------------*/

#define max(x,y)        (((x) < (y)) ? (y) : (x))
#define min(x,y)        (((x) < (y)) ? (x) : (y))

#endif /* BUBBLE_H */
