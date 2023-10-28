/*---------------------------------------------------------------------------------------
    Purpose: small DLL to exercise the dynamic loading features of
             the PUPS system

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

    Version: 2.00
    Dated:   4th January 2023
    E-mail:  mao@tumblingdice.co.uk
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


/*-----------------------------------------------------------------------------*/
/* Each dynamic function must supply four pieces of information. Firstly the   */
/* _BOOLEAN <func>_is_orifice = TRUE tells the binder (in dlllib) that this    */
/* function may be legitimately exported. The <func>_prototype is used to make */
/* sure that importing process and the exportying DLL can do dynamic type      */
/* checking (in the future, they may also do type broking aka CORBA). The      */
/* <func>_info pointer optionally points to a string which supplies user       */
/* readable information about the exported function.                           */
/*                                                                             */
/* The last piece of information supplied is, of course, the function itself.  */
/*-----------------------------------------------------------------------------*/


/*------------------------------------------------*/
/* Mark this function as being orifice exportable */
/*------------------------------------------------*/

_PUBLIC _BOOLEAN phlegm_is_orifice = TRUE;


/*----------------------------*/
/* Optional information field */
/*----------------------------*/

_PUBLIC char     *phlegm_info      = "Mucus by any other name";


/*-------------------------------------------------------------------------*/
/* Prototype for function (note this is a PSRP dynamic function prototype) */
/*-------------------------------------------------------------------------*/

_PUBLIC char     *phlegm_prototype = "int (*)(int, char *[])";


/*--------------------------------*/
/* The function actually exported */
/*--------------------------------*/

_PUBLIC int phlegm(int argc, char *argv[])

{   (void)fprintf(psrp_out,"phlegm\n");
    (void)fflush(psrp_out);

    return(PSRP_OK);
}




/*------------------------------------------------*/
/* Mark this function as being orifice exportable */
/*------------------------------------------------*/

_PUBLIC _BOOLEAN sickbag_is_orifice = TRUE;


/*----------------------------*/
/* Optional information field */
/*----------------------------*/

_PUBLIC char     *sickbag_info      = "The sickbag";


/*-------------------------------------------------------------------------*/
/* Prototype for function (note this is a PSRP dynamic function prototype) */
/*-------------------------------------------------------------------------*/

_PUBLIC char     *sickbag_prototype = "int (*)(int, char *[])";


/*--------------------------------*/
/* The function actually exported */
/*--------------------------------*/

_PUBLIC int sickbag(int argc, char *argv[])

{   (void)fprintf(psrp_out,"sickbag\n");
    (void)fflush(psrp_out);

    return(PSRP_OK);
}
