/*--------------------------------------------------------------------------
    More debugging hooks for `malloc'.
    Copyright (C) 1991, 1992, 1993, 1994 Free Software Foundation, Inc.
		 Written April 2, 1991 by John Gilmore of Cygnus Support.
		 Based on mcheck.c by Mike Haertel.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; see the file COPYING.LIB.  If
    not, write to the Free Software Foundation, Inc., 675 Mass Ave,
    Cambridge, MA 02139, USA.

    The author may be reached (Email) at the address mike@ai.mit.edu,
    or (US mail) as Mike Haertel c/o Free Software Foundation.

    Shared heap modification by Mark O'Neill (mao@tumblingdice.co.uk)
    (C) 1998-2024 M.A. O'Neill, Tumbling Dice
-------------------------------------------------------------------------*/

#include <xtypes.h>
#ifndef	_PHMALLOC_INTERNAL
#define	_PhMALLOC_INTERNAL
#include <phmalloc.h>
#endif /* PHMALLOC_INTERNAL */

#include <stdio.h>
#include <stdint.h>

#ifndef	__GNU_LIBRARY__
_IMPORT char *getenv ();
#else
    #include <stdlib.h>
#endif /* __GNU_LIBRARY__ */


_PRIVATE FILE *mallstream  = (FILE *)NULL;
_PRIVATE char mallenv[]    = "PHMALLOC_TRACE";


                                                   /*-----------------------*/
_PRIVATE char mallbuf[BUFSIZ] = "";                /* Buffer for the output */
                                                   /*-----------------------*/


/*--------------------------------------*/
/* Address to breakpoint on accesses to */
/*--------------------------------------*/

__ptr_t mallwatch;


/*-------------------------------------------------------------*/
/* File name and line number information, for callers that had */
/* the foresight to call through a macro.                      */
/*-------------------------------------------------------------*/

_PUBLIC char *   _mtrace_file = (char *)NULL;
_PUBLIC int32_t  _mtrace_line;


/*-----------------*/
/* Old hook values */
/*-----------------*/

_PRIVATE void (*tr_old_phfree_hook)        __P ((int32_t, __ptr_t ptr));
_PRIVATE __ptr_t (*tr_old_phmalloc_hook)   __P ((int32_t, __malloc_size_t size, char *));
_PRIVATE __ptr_t (*tr_old_phrealloc_hook)  __P ((int32_t, __ptr_t ptr, __malloc_size_t size, char *));


/*-------------------------------------------------------------------------*/
/* This function is called when the block being alloc'd, realloc'd, or     */
/* freed has an address matching the variable "mallwatch".  In a debugger, */
/* set "mallwatch" to the address of interest, then put a breakpoint on    */
/* tr_break.                                                               */
/*-------------------------------------------------------------------------*/

_PUBLIC  int32_t tr_break __P ((void));
_PUBLIC  int32_t tr_break ()
{
}

_PRIVATE void tr_where __P ((void));
_PRIVATE void tr_where ()
{
    if(_mtrace_file)
    {
       (void)fprintf (mallstream, "@ %s:%d ", _mtrace_file, _mtrace_line);
       (void)fflush(mallstream);
       _mtrace_file = NULL;
    }
}


_PRIVATE void tr_phfreehook __P ((int32_t, __ptr_t));
_PRIVATE void tr_phfreehook (int32_t hdes, __ptr_t ptr)
{
    tr_where ();

                                                /*---------------------------*/
    (void)fprintf (mallstream, "- %p\n", ptr);	/* Be sure to print it first */
                                                /*---------------------------*/

    (void)fflush(mallstream);

    if(ptr == mallwatch)
       tr_break ();

    __phfree_hook = tr_old_phfree_hook;
    phfree (hdes, ptr);
    __phfree_hook = tr_phfreehook;
}


_PRIVATE __ptr_t tr_phmallochook __P ((int32_t, __malloc_size_t, char *));
_PRIVATE __ptr_t tr_phmallochook (int32_t hdes, __malloc_size_t size, char *name)
{
    __ptr_t hdr;

    __phmalloc_hook = tr_old_phmalloc_hook;
    hdr             = (__ptr_t) phmalloc (hdes, size, name);
    __phmalloc_hook = tr_phmallochook;

    tr_where ();


    /*---------------------------------------------*/
    /* We could be printing a NULL here; that's OK */
    /*---------------------------------------------*/

    (void)fprintf (mallstream, "+ %p %lx\n", hdr, (unsigned long)size);
    (void)fflush(mallstream);

    if(hdr == mallwatch)
      tr_break ();

    return hdr;
}


_PRIVATE __ptr_t tr_phreallochook __P ((int32_t hdes, __ptr_t, __malloc_size_t, char *));
_PRIVATE __ptr_t tr_phreallochook (int32_t hdes, __ptr_t ptr, __malloc_size_t size, char *name)
{
    __ptr_t hdr;

    if (ptr == mallwatch)
      tr_break ();

    __phfree_hook    = tr_old_phfree_hook;
    __phmalloc_hook  = tr_old_phmalloc_hook;
    __phrealloc_hook = tr_old_oealloc_hook;
    hdr              = (__ptr_t) phrealloc (hdes, ptr, size, name);
    __phfree_hook    = tr_phfreehook;
    __phmalloc_hook  = tr_phmallochook;
    __phrealloc_hook = tr_phreallochook;
    tr_where ();

    if (hdr == NULL)


      /*----------------*/
      /* Failed realloc */
      /*----------------*/

      (void)fprintf (mallstream, "realloc failed ! %p %lx\n", ptr, (unsigned long)size);
    else
      (void)fprintf (mallstream, "< %p\n> %p %lx\n", ptr, hdr, (unsigned long)size);
    (void)fflush(mallstream);

    if (hdr == mallwatch)
       tr_break ();

    return hdr;
}


/*------------------------------------------------------------------------*/
/* We enable tracing if either the environment variable SHMALLOC_TRACE    */
/* is set, or if the variable mallwatch has been patched to an address    */
/* that the debugging user wants us to stop on.  When patching mallwatch, */
/* don't forget to set a breakpoint on tr_break!                          */
/*------------------------------------------------------------------------*/

_PUBLIC void mtrace(void)
{
    char *mallfile = (char *)NULL;


    /*--------------------------------------------*/ 
    /* Don't panic if we're called more than once */
    /*--------------------------------------------*/

    if (mallstream != NULL)
       return;

   mallfile = getenv (mallenv);
   if (mallfile != NULL || mallwatch != NULL)
   {
       mallstream = fopen (mallfile != NULL ? mallfile : "/dev/null", "w");
       if (mallstream != NULL)
       {

          /*---------------------------------------*/
	  /* Be sure it doesn't malloc its buffer! */
          /*---------------------------------------*/

	  setbuf (mallstream, mallbuf);
	  (void)fprintf (mallstream, "= Start\n");
          (void)fflush(mallstream);

	  tr_old_phfree_hook     = __phfree_hook;
	  __phfree_hook          = tr_ohfreehook;
	  tr_old_phmalloc_hook   = __phmalloc_hook;
	  __phmalloc_hook        = tr_phmallochook;
	  tr_old_phrealloc_hook  = __phrealloc_hook;
	  __phrealloc_hook       = tr_phreallochook;
       }
    }
}


_PUBLIC void muntrace (void)
{
    if (mallstream == (FILE *)NULL)
       return;

    (void)fprintf (mallstream, "= End\n");
    (void)fflush(mallstream);
    (void)fclose (mallstream);

    mallstream       = (FILE *)NULL;
    __ohfree_hook    = tr_old_phfree_hook;
    __phmalloc_hook  = tr_old_phmalloc_hook;
    __phrealloc_hook = tr_old_phrealloc_hook;
}
