/*--------------------------------------------------------------------------------
   Includes for memory limit warnings.
   Copyright (C) 1990, 1993, 1994 Free Software Foundation, Inc.
   Copyright (C) 2001-2023, Mark O'Neill, Tumbling Dice <mao@tumblingdice.co.uk>

   This file is based on part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If
   not, write to the Free Software Foundation, Inc., 675 Mass Ave,
   Cambridge, MA 02139, USA.
--------------------------------------------------------------------------------*/


/*-------------------------------------------------*/
/* Some systems need this before <sys/resource.h>. */
/*-------------------------------------------------*/

#include <sys/types.h>

#ifdef _LIBC
#   include <sys/resource.h>
#else
#   define LIM_DATA 1000000
#   include <sys/time.h>
#   include <sys/resource.h>

#endif /* _LIBC */


#ifdef emacs

/*------------------------------------------------------------------------*/
/* The important properties of this type are that 1) it's a pointer, and  */
/* 2) arithmetic on it should work as if the size of the object pointed   */
/* to has a size of 1.                                                    */
/*------------------------------------------------------------------------*/

#ifdef __STDC__
    typedef void *POINTER;
#else
    typedef char *POINTER;
#endif /* __STDC__ */

typedef unsigned long SIZE;

#ifdef NULL
#   undef NULL
#endif /* NULL */

#define NULL ((POINTER) 0)

extern POINTER start_of_data ();
#ifdef DATA_SEG_BITS
#   define EXCEEDS_LISP_PTR(ptr) (((EMACS_UINT) (ptr) & ~DATA_SEG_BITS) >> VALBITS)
#else
#   define EXCEEDS_LISP_PTR(ptr) ((EMACS_UINT) (ptr) >> VALBITS)
#endif /* DATA_SEG_BITS */


       /*-----------*/
#else  /* not emacs */ 
       /*-----------*/

extern char etext;
#define start_of_data() &etext

       /*-----------*/
#endif /* Not emacs */
       /*-----------*/
  

/*------------------------------------------------------------*/
/* start of data space; can be changed by calling malloc_init */
/*------------------------------------------------------------*/

static POINTER data_space_start;


/*--------------------------------------------------------------------*/
/* Number of bytes of writable memory we can expect to be able to get */
/*--------------------------------------------------------------------*/

static unsigned int lim_data;

#ifdef NO_LIM_DATA

static void get_lim_data ()
{
  lim_data = (-1);
}


      /*-------------*/
#else /* NO_LIM_DATA */
      /*-------------*/

static void get_lim_data (void)
{
  extern long ulimit ();

  lim_data = (-1);

  /*---------------------------------------------*/
  /* Use the ulimit call, if we seem to have it. */
  /*---------------------------------------------*/

  lim_data =  ulimit (3, 0);
  lim_data -= (long) data_space_start;
}

       /*-------------*/
#endif /* NO_LIM_DATA */
       /*-------------*/

