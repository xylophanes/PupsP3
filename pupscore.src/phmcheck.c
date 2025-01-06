/*-------------------------------------------------------------------------
    Standard debugging hooks for `malloc'.
    Copyright 1990, 1991, 1992, 1993, 1994 Free Software Foundation, Inc.
    Written May 1989 by Mike Haertel.

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

    Persistent heap modifications by Mark O'Neill (mao@tumblingdice.co.uk)
    (C) 1998-2024 M.A. O'Neill, Tumbling Dice
-------------------------------------------------------------------------*/

#ifndef	_PHMALLOC_INTERNAL
#define	_PHMALLOC_INTERNAL
#include <phmalloc.h>
#include <stdio.h>
#endif /* _PHMALLOC_INTERNAL */
#include <stdint.h>


/*----------------------*/
/* Are you experienced? */
/*----------------------*/

_IMPORT int32_t *__phmalloc_initialized;


/*------------------*/
/* Old hook values  */
/*------------------*/

_PRIVATE void (*old_free_hook)       __P ((int, __ptr_t ptr));
_PRIVATE __ptr_t (*old_malloc_hook)  __P ((int, __malloc_size_t size, char *));
_PRIVATE __ptr_t (*old_realloc_hook) __P ((int, __ptr_t ptr, __malloc_size_t size, char *));


/*-----------------------------------------------*/
/* Function to call when something awful happens */
/*-----------------------------------------------*/

_PRIVATE void (*abortfunc) __P ((enum mcheck_status));


/*----------------------------*/
/* Arbitrary magical numbers  */
/*----------------------------*/

#define MAGICWORD	0xfedabeeb
#define MAGICFREE	0xd8675309
#define MAGICBYTE	((char) 0xd7)
#define MALLOCFLOOD	((char) 0x93)
#define FREEFLOOD	((char) 0x95)

struct hdr
{
                                /*----------------------------------------*/
    __malloc_size_t size;	/* Exact size requested by user.          */
    uint64_t        magic;	/* Magic number to check header integrity */
                                /*----------------------------------------*/

};


#if defined(_LIBC) || defined(STDC_HEADERS) || defined(USG)
#define flood memset
#else
_PRIVATE void flood __P ((__ptr_t, char, __malloc_size_t));

                                                                   /*------------------------------------------*/
_PRIVATE void flood (__ptr_t ptr, char val, __malloc_size_t size)  /* MAO Was int but char seems more sensible */
                                                                   /*------------------------------------------*/
{
  char *cp = ptr;
  while(size--)
       *cp++ = val;
}
#endif /* flood memset */


_PRIVATE enum mcheck_status checkhdr __P ((const struct hdr *));
_PRIVATE enum mcheck_status checkhdr (const struct hdr *hdr)
{
    enum mcheck_status status;
    switch (hdr->magic)
    {      default:
                    status = MCHECK_HEAD;
                    break;

           case MAGICFREE:
                    status = MCHECK_FREE;
                    break;

           case MAGICWORD:
                    if(((char *) &hdr[1])[hdr->size] != MAGICBYTE)
	               status = MCHECK_TAIL;
                    else
	               status = MCHECK_OK;
                    break;
    }

    if (status != MCHECK_OK)
      (*abortfunc) (status);

    return status;
}


_PRIVATE void freehook __P ((int32_t, __ptr_t));
_PRIVATE void freehook (int32_t hdes, __ptr_t ptr)
{
    struct hdr *hdr = ((struct hdr *) ptr) - 1;
    checkhdr (hdr);

    hdr->magic = MAGICFREE;
    flood (ptr, FREEFLOOD, hdr->size);

    __phfree_hook = old_free_hook;
    (void)phfree (hdes, hdr);
    __phfree_hook = freehook;
}


_PRIVATE __ptr_t mallochook __P ((int32_t, __malloc_size_t, char *));
_PRIVATE __ptr_t mallochook (int32_t hdes, __malloc_size_t size, char *name)
{
    struct hdr *hdr = (struct hdr *)NULL;

    __phmalloc_hook = old_malloc_hook;
    hdr = (struct hdr *) phmalloc (hdes, sizeof (struct hdr) + size + 1, name);
    __phmalloc_hook = mallochook;

    if(hdr == NULL)
       return NULL;

    hdr->size                = size;
    hdr->magic               = MAGICWORD;
    ((char *) &hdr[1])[size] = MAGICBYTE;

    flood ((__ptr_t) (hdr + 1), MALLOCFLOOD, size);
    return (__ptr_t) (hdr + 1);
}


_PRIVATE __ptr_t reallochook __P ((int32_t, __ptr_t, __malloc_size_t, char *));
_PRIVATE __ptr_t reallochook (int32_t hdes, __ptr_t ptr, __malloc_size_t size, char *name)
{
    struct hdr *hdr = ((struct hdr *) ptr) - 1;
    __malloc_size_t osize = hdr->size;

    checkhdr (hdr);
    if (size < osize)
      flood ((char *) ptr + size, FREEFLOOD, osize - size);
    __phfree_hook    = old_free_hook;
    __phmalloc_hook  = old_malloc_hook;
    __phrealloc_hook = old_realloc_hook;
    hdr              = (struct hdr *) phrealloc (hdes, (__ptr_t) hdr, sizeof (struct hdr) + size + 1,name);
    __phfree_hook = freehook;
    __phmalloc_hook = mallochook;
    __phrealloc_hook = reallochook;

    if(hdr == (struct hdr *)NULL)
      return (__ptr_t)NULL;

    hdr->size                 = size;
    hdr->magic               = MAGICWORD;
    ((char *) &hdr[1])[size] = MAGICBYTE;

    if(size > osize)
      flood ((char *) (hdr + 1) + osize, MALLOCFLOOD, size - osize);

    return (__ptr_t) (hdr + 1);
}


_PRIVATE void mabort (enum mcheck_status status)
{
    const char *msg = (char *)NULL;
    switch (status)
    {
           case MCHECK_OK:
                  msg = "memory is consistent, library is buggy";
                  break;

           case MCHECK_HEAD:
                  msg = "memory clobbered before allocated block";
                  break;

           case MCHECK_TAIL:
                  msg = "memory clobbered past end of allocated block";
                  break;

           case MCHECK_FREE:
                  msg = "block freed twice";
                  break;

           default:
                  msg = "bogus mcheck_status, library is buggy";
                  break;
    }

    #ifdef __GNU_LIBRARY__
    __libc_fatal (msg);
    #else
    (void)fprintf (stderr, "mcheck: %s\n", msg);
    (void)fflush (stderr);
    abort ();
    #endif /* __GNU_LIBRARY__ */
}


_PRIVATE int32_t mcheck_used = 0;
_PUBLIC  int32_t mcheck (void (*func) __P ((enum mcheck_status)))
{
    abortfunc = (func != NULL) ? func : &mabort;


    /*--------------------------------------------------------------------*/
    /* These hooks may not be safely inserted if malloc is already in use */
    /*--------------------------------------------------------------------*/

    if (!__phmalloc_initialized && !mcheck_used)
    {
        old_free_hook    = __phfree_hook;
        __phfree_hook    = freehook;
        old_malloc_hook  = __phmalloc_hook;
        __phmalloc_hook  = mallochook;
        old_realloc_hook = __phrealloc_hook;
        __phrealloc_hook = reallochook;
        mcheck_used      = 1;
    }

    return mcheck_used ? 0 : -1;
}

_PUBLIC enum mcheck_status mprobe (__ptr_t ptr)
{
    return mcheck_used ? checkhdr (ptr) : MCHECK_DISABLED;
}
