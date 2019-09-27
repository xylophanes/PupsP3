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
#include <psrp.h>


/*-------------------------------------------------------------*/
/* PSRP communication channels - required if exported function */
/* are dynamic PSRP action functions                           */
/*-------------------------------------------------------------*/

_IMPORT FILE *psrp_in,
             *psrp_out;
