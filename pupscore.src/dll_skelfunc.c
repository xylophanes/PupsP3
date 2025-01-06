/*----------------------------------------------------------------------------
    Each dynamic function must supply four pieces of information. Firstly the
    _BOOLEAN <func>_is_orifice = TRUE tells the binder (in dlllib) that this
    function may be legitately exported. The <func>_prototype is used to make
    sure that importing process and the exporting DLL can do dynamic type
    checking (in the future, they may also do type broking aka CORBA). The
    <func>_info pointer optionally points to a string which supplies user
    readable information about the exported function.

    The last piece of infomation supplied is, of course, the function
    itself ...
---------------------------------------------------------------------------*/


/*------------------------------------------------*/
/* Mark this function as being orifice exportable */
/*------------------------------------------------*/

_PUBLIC _BOOLEAN @DLL_FUNC_NAME_is_orifice = TRUE;


/*----------------------------*/
/* Optional information field */
/*----------------------------*/

_PUBLIC char     *@DLL_FUNC_NAME_info      = "@DLLDES";


/*-------------------------------------------------------------------------*/
/* Prototype for function (note this is a PSRP dynamic function prototype) */
/*-------------------------------------------------------------------------*/

_PUBLIC char     *@DLL_FUNC_NAME_prototype = "";


/*--------------------------------*/
/* The function actually exported */
/*--------------------------------*/

_PUBLIC  int32_t @DLL_FUNC_NAME(int32_t argc, char *argv[])

{

    /*---------------------------*/
    /* Body of DLL function here */
    /*---------------------------*/


}

