/*---------------------------------------------------------------------------------------
    Purpose: @PURPOSE 

    Author:  @AUTHOR 
             @INSTITUTION

    Dated:   @DATE 
    Version: 1.00
    E-mail:  @EMAIL 
---------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <xtypes.h>
#include <me.h>
#include <utils.h>
#include <psrp.h>

#undef    __NOT_LIB_SOURCE__
#include <@LIBNAME.h>
#define   __NOT_LIB_SOURCE__




/*----------------------------------------------------------------------------
    Get application information for slot manager ...
----------------------------------------------------------------------------*/


/*---------------------------*/
/* Slot information function */
/*---------------------------*/

_PRIVATE void @LIBNAME_slot(int level)
{   (void)fprintf(stderr,"lib @LIBNAME 1.00: [ANSI C]\n");

    if(level > 1)
    {  (void)fprintf(stderr,"(C) @DATE @INSTITUTION\n");
       (void)fprintf(stderr,"Author: @AUTHOR\n");
       (void)fprintf(stderr,"@LIBDES\n");
       (void)fflush(stderr);
    }

    (void)fprintf(stderr,"\n");
    (void)fflush(stderr);
}


_EXTERN void (* SLOT ) = @LIBNAME_slot;


/*----------------------------------------------------------------------------
    Functions which are private to this library ...
----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
    Variables which are private to this library ...
----------------------------------------------------------------------------*/



