/*----------------------------------------------------------------------------
    Purpose: DLL support for PUPS environment

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.00 
    Dated:   27th September 2019 
    E-mail:  mao@tumblingdice.co.uk
----------------------------------------------------------------------------*/

#ifdef DLL_SUPPORT


/*-------------------------------------------------------------*/
/* If we have OpenMP support then we also have pthread support */
/* (OpenMP is built on pthreads for Linux distributions).      */
/*-------------------------------------------------------------*/

#if defined(_OPENMP) && !defined(PTHREAD_SUPPORT)
#define PTHREAD_SUPPORT
#endif /* PTHREAD_SUPPORT */


#include <me.h>
#include <string.h>
#include <xtypes.h>
#include <errno.h>
#include <utils.h>
#include <dlfcn.h>
#include <unistd.h>

#undef   __NOT_LIB_SOURCE__
#include <tad.h>
#define  __NOT_LIB_SOURCE__

#ifdef PTHREAD_SUPPORT
#include <tad.h>
#endif /* PTHREAD_SUPPORT */

#undef    __NOT_LIB_SOURCE__
#include <dll.h>
#define   __NOT_LIB_SOURCE__




/*----------------------------------------------------------------------------
    Get application information for slot manager ...
----------------------------------------------------------------------------*/


/*---------------------------*/
/* Slot information function */
/*---------------------------*/

_PRIVATE void dlllib_slot(int level)
{   (void)fprintf(stderr,"lib dlllib %s: [ANSI C]\n",DLL_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(C) 1999-2019 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"PUPS/P3 DLL support library (built %s %s)\n\n",__TIME__,__DATE__);
       (void)fflush(stderr);
    }

    (void)fprintf(stderr,"\n");
    (void)fflush(stderr);
}


/*------------------------------------------------------------------------------
    Segment identification for dynamic link support library ...
------------------------------------------------------------------------------*/

#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT ) __attribute__ ((aligned(16))) = dlllib_slot;
#endif /* SLOT */




/*----------------------------------------------*/
/* Variables which are imported by this library */
/*----------------------------------------------*/


/*-----------------------------------------------------------------------*/
/*  Public variables exported by this library (note this is not the best */
/*  way of exporting the thread table -- it would be better to have all  */
/*  the manipulation of the thread table done by functions defined here  */
/*  which are themselves _PUBLIC functions of this library)              */
/*-----------------------------------------------------------------------*/

_PUBLIC int             n_orifices                   = 0;    // Number of orifices in use
_PUBLIC int             ortab_slots                  = 0;    // Number of active orifice slots
_PUBLIC ortab_type      *ortab      = (ortab_type *)NULL;    // Orifice table




/*----------------------------------------------------------------------------
    Get next free slot in the orifice table ...
----------------------------------------------------------------------------*/

_PRIVATE int get_ortab_slot(void)

{   int i;

    for(i=0; i<appl_max_orifices; ++i)
    {  if(ortab[i].dll_handle == (void *)NULL)
       {  pups_set_errno(EAGAIN);
          return(i);
       }
    }

    ++ortab_slots;

    pups_set_errno(OK);
    return(ortab_slots);
}




/*-----------------------------------------------------------------------------
    Find an ortab slot given orifice handle ...
-----------------------------------------------------------------------------*/

_PUBLIC int find_ortab_slot_by_handle(const void *orifice_handle)

{   int i;

    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("[find_ortab_slot_by_handle] attempt by non root thread to perform PUPS/P3 DLL operation");

    if(orifice_handle == (void *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    for(i=0; i<appl_max_orifices; ++i)
    {  if(ortab[i].orifice_handle == orifice_handle)
       {  pups_set_errno(OK);
          return(i);
       }
    }

    pups_set_errno(ESRCH);
    return(-1);
}




/*------------------------------------------------------------------------------
    Return the index of the ortab entry of named orifice ...
------------------------------------------------------------------------------*/

_PUBLIC int find_ortab_slot_by_name(const char *orifice_name)

{   int i;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(orifice_name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("[find_ortab_slot_by_name] attempt by non root thread to perform PUPS/P3 DLL operation");

    for(i=0; i<appl_max_orifices; ++i)
    {  if(ortab[i].orifice_name != (char *)NULL && strcmp(ortab[i].orifice_name,orifice_name) == 0) 
       {  pups_set_errno(OK);
          return(i);
       }
    }

    pups_set_errno(ESRCH);
    return(-1);
}




/*------------------------------------------------------------------------------
    Clear orifice table slot ...
------------------------------------------------------------------------------*/

_PUBLIC int clear_ortab_slot(const _BOOLEAN destroy, const unsigned int index)

{   

    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(index >= appl_max_orifices)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("[clear_ortab_slot] attempt by non root thread to perform PUPS/P3 DLL operation");

    ortab[index].cnt            = 0;
    ortab[index].dll_handle     = (void *)NULL;
    ortab[index].orifice_handle = (void *)NULL;

    if(destroy == FALSE)
    {  ortab[index].orifice_name       = (char *)NULL;
       ortab[index].orifice_prototype  = (char *)NULL;
       ortab[index].dll_name           = (char *)NULL;
       ortab[index].orifice_info       = (char *)NULL;
    }
    else if(destroy == TRUE)
    {  ortab[index].orifice_name       = (char *)pups_free((void *)ortab[index].orifice_name);
       ortab[index].orifice_prototype  = (char *)pups_free((void *)ortab[index].orifice_prototype);
       ortab[index].dll_name           = (char *)pups_free((void *)ortab[index].dll_name);
       ortab[index].orifice_info       = (char *)pups_free((void *)ortab[index].orifice_info);
    }
    else
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    pups_set_errno(OK);
    return(0);

}




/*------------------------------------------------------------------------------
    Initialise orifice table ...
------------------------------------------------------------------------------*/

_PUBLIC void ortab_init(const unsigned int max_orifices)

{   int i;


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("[ortab_init] attempt by non root thread to perform PUPS/P3 DLL operation");


    /*---------------------------------------------*/
    /* Orifice slot count <= 0 implies that we are */
    /* not using orifices                          */
    /*---------------------------------------------*/

    if(max_orifices > MAX_ORIFICES)
    {  pups_set_errno(EINVAL);
       return;
    }

    ortab = (ortab_type *)pups_calloc(max_orifices,sizeof(ortab_type));
    for(i=0; i<max_orifices; ++i)
    {  ortab[i].dll_handle        = (void *)NULL;
       ortab[i].orifice_handle    = (void *)NULL;
       ortab[i].cnt               = 0;
       ortab[i].dll_name          = (char *)NULL;
       ortab[i].orifice_name      = (char *)NULL;
       ortab[i].orifice_prototype = (char *)NULL;
    }

    pups_set_errno(OK);
}




/*------------------------------------------------------------------------------
    Check for existence of an orifice. If the orifice exists its prototype
    is returned ...  
------------------------------------------------------------------------------*/

_PUBLIC _BOOLEAN pups_is_orifice(const char          *dll_name,    // Name of DLL exporting orifice
                                 const char      *orifice_name,    // Name of orifice
                                 const char *orifice_prototype)    // Orifice prototype

{   void *dll_handle            = (void *)NULL,
         *is_orifice            = (void *)NULL,
         *prototype_handle      = (void *)NULL;

    char cwd[SSIZE]             = "",
         is_orifice_name[SSIZE] = "",
         dll_pathname[SSIZE]    = "";


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(dll_name          == (const char *)NULL  ||
       orifice_name      == (const char *)NULL  ||
       orifice_prototype == (const char *)NULL   )
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("[pups_is_orifice] attempt by non root thread to perform PUPS/P3 DLL operation");


    /*-------------------------------------------------------------*/
    /* Open DLL which contains the orifice we wish to investigate  */
    /* Make sure that path is absolute - LINUX dlopen() appears to */
    /* require this!                                               */
    /*-------------------------------------------------------------*/

    if(dll_name[0] != '/')
    {  (void)getcwd(cwd,SSIZE);
       (void)snprintf(dll_pathname,SSIZE,"%s/%s",cwd,dll_name);
    }

    if((dll_handle = dlopen(dll_pathname,RTLD_NOW | RTLD_GLOBAL)) == (void *)NULL)
    {  pups_set_errno(EACCES);
       return(FALSE);
    }


    /*------------------------------------------------------*/
    /* Check that we are about to bind a registered orifice */
    /* function - if not, returt to caller                  */
    /*------------------------------------------------------*/

    (void)snprintf(is_orifice_name,SSIZE,"%s_is_orifice",orifice_name);
    if((is_orifice = dlsym(dll_handle,is_orifice_name)) == (void *)NULL)
    {  (void)dlclose(dll_handle);

       pups_set_errno(EACCES);
       return(FALSE);
    }


    /*----------------------------------------*/
    /* Check that this is an orifice function */
    /*----------------------------------------*/

    if(*(_BOOLEAN *)is_orifice == TRUE)
    {  char prototype_handle_name[SSIZE] = "";

       (void)snprintf(prototype_handle_name,SSIZE,"%s_prototype",orifice_name);
       if((prototype_handle = dlsym(dll_handle,prototype_handle_name)) != (void *)NULL)
       {  if(strcmp(orifice_prototype,*(char **)prototype_handle) != 0)
          {  

             /*-----------------------------------------------*/
             /* Close all dynamically imported variables      */
             /* Note that we have to close the DLL exactly as */
             /* many times as it has been opened in order to  */
             /* detach it from the calling processes address  */
             /* space                                         */
             /*-----------------------------------------------*/

             (void)dlclose(dll_handle);

             if(orifice_prototype != (char *)NULL)
                (void)strlcpy(orifice_prototype,"none",SSIZE);

             (void)pups_set_errno(EACCES);
             return(FALSE);
          }
       }
    }


    /*-----------------------------------------------*/
    /* Close all dynamically imported variables      */
    /* Note that we have to close the DLL exactly as */
    /* many times as it has been opened in order to  */
    /* detach it from the calling processes address  */
    /* space                                         */
    /*-----------------------------------------------*/

    (void)dlclose(dll_handle);

    pups_set_errno(OK);
    return(TRUE);
}




/*------------------------------------------------------------------------------
    Bind orifice handle. An orifice is a function or variable which is
    exported by a DLL. In C, a prototypical orifice looks like:

    _BOOLEAN <name>_is_orifice: Tells binder this is an orifice function.

    char     <name>_prototype : Prototype of orifice should be EXACTLY the
                                same as internal C definition.

    C declaration of orifice function ...
------------------------------------------------------------------------------*/

_PUBLIC void *pups_bind_orifice_handle(const char *dll_name,            // Name of DLL which exports orifice
                                       const char *orifice_name,        // Name of orifice
                                       const char *orifice_prototype)   // Prototype of orifice

{   int index;

    void *dll_handle                 = (void *)NULL, 
         *orifice_handle             = (void *)NULL,
         *info_handle                = (void *)NULL,
         *is_orifice                 = (void *)NULL,
         *prototype_handle           = (void *)NULL;

    char cwd[SSIZE]                   = "",
         dll_pathname[SSIZE]          = "",
         info_handle_name[SSIZE]      = "",
         prototype_handle_name[SSIZE] = "",
         is_orifice_name[SSIZE]       = "";


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(dll_name == (const char *)NULL || orifice_name == (const char *)NULL || orifice_prototype == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return((void *)NULL);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("[pups_bind_orifice_handle] attempt by non root thread to perform PUPS/P3 DLL operation");


    /*--------------------------------------------------*/
    /* Check that we haven't already bound this orifice */
    /*--------------------------------------------------*/

    if((index = find_ortab_slot_by_name(orifice_name)) != (-1))
    {  pups_set_errno(EEXIST);
       return((void *)NULL);
    }


    /*-------------------------------------------------------------*/
    /* Check that we are about to bind a registered orifice        */
    /* function - if not, return to caller                         */
    /* Make sure that path is absolute - LINUX dlopen() appears to */
    /* require this!                                               */
    /*-------------------------------------------------------------*/

    if(strccpy(dll_pathname,pups_search_path("DLLPATH",dll_name)) == (char *)NULL)
    {  if(dll_name[0] != '/')
       {  (void)getcwd(cwd,SSIZE);
          (void)snprintf(dll_pathname,SSIZE,"%s/%s",cwd,dll_name);
       }
       else
          (void)strlcpy(dll_pathname,dll_name,SSIZE);


       /*----------------------------------*/
       /* Check that DLL pathname is valid */
       /*----------------------------------*/

       if(access(dll_pathname,F_OK | R_OK) == (-1))
       {  pups_set_errno(EEXIST);
          return((void *)NULL);
       }
    }


    /*-----------------------------------------------------*/
    /* Open DLL which contains the orifice we wish to bind */
    /*-----------------------------------------------------*/

    if((dll_handle = dlopen(dll_pathname,RTLD_NOW | RTLD_GLOBAL)) == (void *)NULL)
    {  pups_set_errno(EEXIST);
       return((void *)NULL);
    }

    (void)snprintf(is_orifice_name,SSIZE,"%s_is_orifice",orifice_name);
    if((is_orifice = dlsym(dll_handle,is_orifice_name)) == (void *)NULL)
    {  (void)dlclose(dll_handle);

       pups_set_errno(EEXIST);
       return((void *)NULL);
    }


    /*----------------------------------------*/
    /* Check that this is an orifice function */
    /*----------------------------------------*/

    if(*(_BOOLEAN *)is_orifice == TRUE)
    {  char prototype_handle_name[SSIZE]    = "",
            orifice_initialiser_name[SSIZE] = "";

       int  (*initialiser_handle)(void);

       (void)snprintf(prototype_handle_name,SSIZE,"%s_prototype",orifice_name); 
       if((prototype_handle = dlsym(dll_handle,prototype_handle_name)) != (void *)NULL)
       {  if((orifice_handle = dlsym(dll_handle,orifice_name))  == (void *)NULL    ||
             strcmp(orifice_prototype,*(char **)prototype_handle) != 0              )
          {  

             /*-----------------------------------------------*/
             /* Close oriface to dynamically linked library   */ 
             /*                                               */
             /* Note that we have to close the DLL exactly as */
             /* many times as it has been opened in order to  */
             /* detach it from the calling processes address  */
             /* space                                         */
             /*-----------------------------------------------*/

             (void)dlclose(dll_handle);

             (void)pups_set_errno(EACCES);
             return((void *)NULL);
          }

          if((index = get_ortab_slot()) == (-1))
          {  

             /*-----------------------------------------------*/
             /* We have run out of space in the orifice       */
             /* table                                         */ 
             /*                                               */
             /* Close oriface to dynamically linked library   */
             /*                                               */
             /* Note that we have to close the DLL exactly as */
             /* many times as it has been opened in order to  */
             /* detach it from the calling processes address  */
             /* space                                         */
             /*-----------------------------------------------*/

             (void)dlclose(dll_handle);

             (void)pups_set_errno(ENOMEM);
             return((void *)NULL);
          }


          /*-------------------------------------------------------*/
          /* Note that we are limited to around 1000 characters of */
          /* information in the orifice info field                 */
          /*-------------------------------------------------------*/

          (void)snprintf(info_handle_name,SSIZE,"%s_info",orifice_name);
          if((info_handle = dlsym(dll_handle,info_handle_name)) != (void *)NULL)
          {  ortab[index].orifice_info = (char *)pups_malloc(LONG_LINE);
             (void)strlcpy(ortab[index].orifice_info,*(char **)info_handle,SSIZE);
          }


          /*-------------------------------------------------------*/
          /* We have loaded the DLL. Copy data associated with the */
          /* DLL into the orifice table                            */
          /*-------------------------------------------------------*/

          ortab[index].orifice_name      = (char *)pups_malloc(SSIZE);
          (void)strlcpy(ortab[index].orifice_name,     (char *)orifice_name,SSIZE);

          ortab[index].dll_name          = (char *)pups_malloc(SSIZE);
          (void)strlcpy(ortab[index].dll_name,         (char *)dll_name,SSIZE);

          ortab[index].orifice_prototype = (char *)pups_malloc(SSIZE); 
          (void)strlcpy(ortab[index].orifice_prototype,(char *)orifice_prototype,SSIZE);

          ortab[index].dll_handle        = dll_handle;
	  ortab[index].orifice_handle    = orifice_handle;
          ++ortab[index].cnt;


          /*-------------------------------------------------*/
          /* Run any initialisation functions we may require */
          /*-------------------------------------------------*/

          (void)snprintf(orifice_initialiser_name,SSIZE,"%s_initialise",orifice_name);
          if((initialiser_handle = dlsym(dll_handle,orifice_initialiser_name)) != (void *)NULL)
          {  if((*initialiser_handle)() == (-1))
             {  

                /*----------------------------------------------------*/
                /* We have failed to initialise (API) environment for */
                /* dynamic function                                   */
                /*----------------------------------------------------*/

                (void)dlclose(dll_handle);

                (void)pups_set_errno(EACCES);
                return((void *)NULL);
             }
          }

          (void)pups_set_errno(OK);
          return(orifice_handle);
       }
    }

    (void)dlclose(dll_handle);

    (void)pups_set_errno(ESRCH);
    return((void *)NULL);
}


 
/*------------------------------------------------------------------------------
    Free orifice handle ...
------------------------------------------------------------------------------*/

_PUBLIC void *pups_free_orifice_handle(const void *orifice_handle)

{   int  index;
    char orifice_exitf_name[SSIZE] = "";

    void (*orifice_exit_handle)(void);


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(orifice_handle == (const void *)NULL)
    {  pups_set_errno(EINVAL);
       return((void *)NULL);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("[pups_free_orifice_handle] attempt by non root thread to perform PUPS/P3 DLL operation");


    /*------------------------------------------------*/
    /* Check that address supplied actually is a PUPS */
    /* orifice handle                                 */
    /*------------------------------------------------*/

    if((index = find_ortab_slot_by_handle(orifice_handle)) == (-1))
       return((void *)NULL);


    /*----------------------------------------------*/
    /* Check that we can in fact free this orifice  */
    /* handle. If cnt is non-zero there are other   */
    /* threads using this orifice, so we can't free */
    /* it yet                                       */
    /*----------------------------------------------*/

    if(ortab[index].cnt > 1)
    {  --ortab[index].cnt;
       (void)pups_set_errno(OK);

       return(orifice_handle);
    }

    /*---------------------------------------*/
    /* Do any cleanup which may be necessary */
    /*---------------------------------------*/

    (void)snprintf(orifice_exitf_name,SSIZE,"%s_exit",ortab[index].orifice_name);
    if((orifice_exit_handle = dlsym(ortab[index].dll_handle,orifice_exitf_name)) != (void *)NULL)
       (*orifice_exit_handle)();


    /*--------------------------*/
    /* Actually free the handle */
    /*--------------------------*/

    (void)dlclose(ortab[index].dll_handle);
    clear_ortab_slot(FALSE,index);
   
    (void)pups_set_errno(OK);
    return((void *)NULL);
}




/*-----------------------------------------------------------------------------
    Routine to display currently loaded DLL's - this routine will probably
    be called in response to signalling an active process ...
-----------------------------------------------------------------------------*/

_PUBLIC void pups_show_orifices(const FILE *stream)

{   int i,
        orifices = 0;


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(stream == (const FILE *)NULL)
    {  (void)pups_set_errno(EINVAL);
       return;
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("[pups_show_orifices] attempt by non root thread to perform PUPS/P3 DLL operation");

    (void)fprintf(stream,"\n\n    Bound DLL orifices\n");
    (void)fprintf(stream,"    ==================\n\n");
    (void)fflush(stream);

    for(i=0; i<appl_max_orifices; ++i)
    {  if(ortab[i].orifice_handle != (void *)NULL)
       {  ++orifices;


          /*----------------------*/
          /* Display orifice data */
          /*----------------------*/

          (void)fprintf(stream,"    %04d: Orifice %-48s (prototype %-32s) (handle at %018x virtual) imported from DLL %s\n",i,
                                                                                                        ortab[i].orifice_name,
                                                                                                   ortab[i].orifice_prototype,
                                                                                   (unsigned long int)ortab[i].orifice_handle,
                                                                                                            ortab[i].dll_name);


          /*------------------------------------------------------*/
          /* Display information on theb orifice (if we have any) */
          /*------------------------------------------------------*/

          if(ortab[i].orifice_info != (char *)NULL)
             (void)fprintf(stream,"    info: %s\n\n",ortab[i].orifice_info);

          (void)fflush(stream);
       }
    }

    if(orifices == 1)
       (void)fprintf(stream,"\n\n    %04d orifice slots allocated [process DLL limit] (%04d in use)\n\n",appl_max_orifices,1);
    else if(orifices > 1)
       (void)fprintf(stream,"\n\n    %04d orifice slots allocated [process DLL limit] (%04d in use)\n\n",appl_max_orifices,orifices);
    else
       (void)fprintf(stream,"    %d orifice slots allocated [process DLL limit] (none in use)\n\n",      appl_max_orifices);

    (void)fflush(stream);

    pups_set_errno(OK);
}




/*-----------------------------------------------------------------------------
    Decode a formatted function prototype ...
-----------------------------------------------------------------------------*/

_PUBLIC _BOOLEAN dll_decode_ffp(const char *in_proto, const char *orifice_name)

{   int index; 

    char stripped_in_proto[SSIZE] = "",
         stripped_ff_proto[SSIZE] = "";


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(in_proto == (const char *)NULL || orifice_name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("[dll_decode_ffp] attempt by non root thread to perform PUPS/P3 DLL operation");

    if((index = find_ortab_slot_by_name(orifice_name)) == FALSE)
       return(-1);


    /*---------------------------------------------------------*/ 
    /* Make sure the prototye passed by the caller and that    */
    /* of the function match. Note that this is a SCAN mode    */
    /* operation, so we can safely strip all whitespace from   */
    /* both in_proto and ff_proto before we make a comparision */
    /*---------------------------------------------------------*/ 

    (void)strlcpy(stripped_in_proto,strpch(' ',in_proto),SSIZE);
    (void)strlcpy(stripped_ff_proto,strpch(' ',ortab[index].orifice_prototype),SSIZE);


    /*----------------------*/
    /* Do prototypes match? */
    /*----------------------*/

    pups_set_errno(OK);
    if(strcmp(stripped_in_proto,stripped_ff_proto) == 0)
       return(TRUE);

    return(FALSE);
}




/*------------------------------------------------------------------------------
    Scan a DLL and display its orifices ...
------------------------------------------------------------------------------*/

_PUBLIC _BOOLEAN pups_show_dll_orifices(const FILE *stream, const char *dll_name)

{   FILE *nm_stream = (FILE *)NULL;

    int      orifices = 0;
    long int index;

    void *dll_handle                   = (void *)NULL,
         *orifice_prototype_handle     = (void *)NULL,
         *orifice_info_handle          = (void *)NULL;

    char strdum[SSIZE]                 = "",
         line[SSIZE]                   = "",
         cwd[SSIZE]                    = "",
         dll_pathname[SSIZE]           = "",
         orifice_name[SSIZE]           = "",
         orifice_prototype_name[SSIZE] = "",
         orifice_info_name[SSIZE]      = "",
         nm_command[SSIZE]             = "",
         next_symbol[SSIZE]            = "";


    /*--------------*/
    /* Sanity check */
    /*--------------*/

    if(stream == (const FILE *)NULL || dll_name == (const char *)NULL)
    {  pups_set_errno(EINVAL);
       return(FALSE);
    }


    /*----------------------------------*/
    /* Only the root thread can process */
    /* PSRP requests                    */
    /*----------------------------------*/

    if(pupsthread_is_root_thread() == FALSE)
       error("[pups_show_dll_orifices] attempt by non root thread to perform PUPS/P3 DLL operation");


    /*-------------------------------------*/
    /* Check that pathname to DLL is valid */
    /*-------------------------------------*/

    if(access(dll_name,F_OK | R_OK | W_OK) == (-1))
    {  (void)fprintf(stream,"pups_show_dll_orifices: DLL \"%s\" does not exist\n",dll_name);
       (void)fflush(stream);

       pups_set_errno(EEXIST);
       return(FALSE);
    }


    /*------------------------------------------*/
    /* Try to open DLL                          */
    /* LINUX insists on having an absolute path */
    /* for dlopen() -- could be a bug           */
    /*------------------------------------------*/

    if(dll_name[0] != '/')
    {  (void)getcwd(cwd,SSIZE);
       (void)snprintf(dll_pathname,SSIZE,"%s/%s",cwd,dll_name);
    }

    if((dll_handle = dlopen(dll_pathname,RTLD_NOW | RTLD_GLOBAL)) == (void *)NULL)
    {  (void)fprintf(stream,"pups_show_dll_orifices: %s is not a DLL\n",dll_name);
       (void)fflush(stream);

       pups_set_errno(EACCES);
       return(FALSE);
    }


    /*------------------*/
    /* Get symbol table */
    /*------------------*/

    (void)snprintf(nm_command,SSIZE,"nm -p %s",dll_name);
    if((nm_stream = popen(nm_command,"r")) == (FILE *)NULL)
    {  if(stream != (FILE *)NULL)
       {  (void)fprintf(stream,"pups_show_dll_orifices: failed to exec nm command pipeline\n");
          (void)fflush(stream);
       }

       pups_set_errno(EPIPE);

       (void)pclose(nm_stream);
       return(FALSE);
    }

    (void)fprintf(stream,"\nOrifices exported by DLL %s\n\n",dll_name);
    (void)fflush(stream);


    /*-------------------------------------------------*/
    /* Read symbols -- if we have a symbol of the form */
    /* <name>_is_orifice, then we have a valid orifice */
    /* in which case we need to dlopen() it and read   */
    /* prototype and functional information            */
    /*-------------------------------------------------*/

    do {   char tmpstr[SSIZE] = "";

           (void)fgets(line,SSIZE,nm_stream);
           if(feof(nm_stream) == 0)
           {  (void)sscanf(line,"%s%s%s",strdum,strdum,next_symbol);


              /*------------------------*/
              /* Do we have an orifice? */
              /*------------------------*/

              if(strinp((long *)&index,next_symbol,"_is_orifice") == TRUE)
              {

                 /*----------------------*/
                 /* Extract orifice name */
                 /*----------------------*/

                 next_symbol[index] = '\0';
                 (void)strlcpy(orifice_name,next_symbol,SSIZE);
                 ++orifices;


                 /*-----------------------------------------------------*/
                 /* Build symbols for orifice prototype and information */
                 /* handles                                             */
                 /*-----------------------------------------------------*/

                 (void)snprintf(orifice_prototype_name,SSIZE,"%s_prototype",orifice_name);
                 (void)snprintf(orifice_info_name     ,SSIZE,"%s_info"     ,orifice_name);

                 if((orifice_prototype_handle = dlsym(dll_handle,orifice_prototype_name)) != (void *)NULL)
                 {   orifice_info_handle = dlsym(dll_handle,orifice_info_name);


                     /*-----------------------------*/
                     /* Display orifice information */
                     /*-----------------------------*/

                     if(*(char **)orifice_info_handle != (char *)NULL)
                        (void)fprintf(stream,"    %04d: orifice name: %-48s [dynamic prototype %-48s]\n          orifice info: %-48s\n\n",orifices,
                                                                                                                                      orifice_name,
                                                                                                                *(char **)orifice_prototype_handle,
                                                                                                                     *(char **)orifice_info_handle);
                     else
                        (void)fprintf(stream,"    %04d: orifice name: %-48s [dynamic prototype %-48s]\n",orifices,
                                                                                                     orifice_name,
                                                                               *(char **)orifice_prototype_handle);
                     (void)fflush(stream);

                 }
              }
           }
        } while(feof(nm_stream) == 0);

    if(orifices  == 1)
       (void)fprintf(stream,"\n\n    DLL %-48s has exported %04d orifice (%04d orifices available)\n\n",    dll_name,1,MAX_ORIFICES);
    else if(orifices > 1)
       (void)fprintf(stream,"\n\n    DLL %-48s has exported %04d orifices (%04d orifices available)\n\n",   dll_name,orifices,MAX_ORIFICES - orifices);
    else
       (void)fprintf(stream,"    DLL %-48s is not a PUPS dynamic library (%04d orifices available)\n\n",dll_name,MAX_ORIFICES);
  
    (void)fflush(stream); 


    /*---------------*/
    /* Close the DLL */
    /*---------------*/

    (void)dlclose(dll_handle);

    pups_set_errno(OK);
    return(TRUE);
}

#endif /* DLL_SUPPORT */
