/*---------------------------------
    Purpose: Library slot manager

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.02 
    Dated:   11th December 2024
    E-Mail:  mao@tumblingdice.co.uk
---------------------------------*/

#include <me.h>
#include <dlfcn.h>
#include <utils.h>
#include <stdlib.h>

#undef   __NOT_LIB_SOURCE__
#include <slotman.h>
#define  __NOT_LIB_SOURCE__


/*------------*/
/* Build date */
/*------------*/

_PUBLIC char appl_build_time[SSIZE] = "";
_PUBLIC char appl_build_date[SSIZE] = "";


/*-------------------*/
/* Private variables */
/*-------------------*/

_PRIVATE uint32_t  max_slots     = 0;  // Slots used 
_PRIVATE uint32_t  max_use_slots = 0;  // Usage slots used 



/*-------------------------------------------*/
/* Public variables exported by this library */
/*-------------------------------------------*/

// Segment slot identifier functions -- initialised to NULL
_PUBLIC void (*seg_slot[MAX_SLOTS])(const int32_t); 


// Usage functions -- initialised to NULL 
_PUBLIC void (*usage_slot[MAX_SLOTS])(void); 


/*-------------------------------------------------------*/
/* Segment slot function pointers -- initialised to NULL */
/*-------------------------------------------------------*/

_PUBLIC void  (*seg_slot_1) (int32_t) , 
              (*seg_slot_2) (int32_t) , 
              (*seg_slot_3) (int32_t) , 
              (*seg_slot_4) (int32_t) , 
              (*seg_slot_5) (int32_t) , 
              (*seg_slot_6) (int32_t) , 
              (*seg_slot_7) (int32_t) , 
              (*seg_slot_8) (int32_t) , 
              (*seg_slot_9) (int32_t) , 
              (*seg_slot_10)(int32_t) , 
              (*seg_slot_11)(int32_t) , 
              (*seg_slot_12)(int32_t) , 
              (*seg_slot_13)(int32_t) , 
              (*seg_slot_14)(int32_t) , 
              (*seg_slot_15)(int32_t) , 
              (*seg_slot_16)(int32_t) ,
              (*seg_slot_17)(int32_t) ,
              (*seg_slot_18)(int32_t) , 
              (*seg_slot_19)(int32_t) , 
              (*seg_slot_20)(int32_t) , 
              (*seg_slot_21)(int32_t) , 
              (*seg_slot_22)(int32_t) ,
              (*seg_slot_23)(int32_t) ,
              (*seg_slot_24)(int32_t) , 
              (*seg_slot_25)(int32_t) , 
              (*seg_slot_26)(int32_t) , 
              (*seg_slot_27)(int32_t) , 
              (*seg_slot_28)(int32_t) ,
              (*seg_slot_29)(int32_t) ,
              (*seg_slot_30)(int32_t) ,
              (*seg_slot_31)(int32_t) ,
              (*seg_slot_32)(int32_t) ,
              (*seg_slot_33)(int32_t) ,
              (*seg_slot_34)(int32_t) ,
              (*seg_slot_35)(int32_t) ,
              (*seg_slot_36)(int32_t) ,
              (*seg_slot_37)(int32_t) ,
              (*seg_slot_38)(int32_t) ,
              (*seg_slot_39)(int32_t) ,
              (*seg_slot_40)(int32_t) ,
              (*seg_slot_41)(int32_t) ,
              (*seg_slot_42)(int32_t) ,
              (*seg_slot_43)(int32_t) ,
              (*seg_slot_44)(int32_t) ,
              (*seg_slot_45)(int32_t) ,
              (*seg_slot_46)(int32_t) ,
              (*seg_slot_47)(int32_t) ,
              (*seg_slot_48)(int32_t) ,
              (*seg_slot_49)(int32_t) ,
              (*seg_slot_50)(int32_t) ,
              (*seg_slot_51)(int32_t) ,
              (*seg_slot_52)(int32_t) ,
              (*seg_slot_53)(int32_t) ,
              (*seg_slot_54)(int32_t) ,
              (*seg_slot_55)(int32_t) ,
              (*seg_slot_56)(int32_t) ,
              (*seg_slot_57)(int32_t) ,
              (*seg_slot_58)(int32_t) ,
              (*seg_slot_59)(int32_t) ,
              (*seg_slot_60)(int32_t) ,
              (*seg_slot_61)(int32_t) ,
              (*seg_slot_62)(int32_t) ,
              (*seg_slot_63)(int32_t) ,
              (*seg_slot_64)(int32_t) ;



/*----------------------------------------------------------*/
/* Segment slot identifier functions -- initialised to NULL */
/*----------------------------------------------------------*/

_PUBLIC void  (*usage_slot_1) (void) , 
              (*usage_slot_2) (void) , 
              (*usage_slot_3) (void) , 
              (*usage_slot_4) (void) , 
              (*usage_slot_5) (void) , 
              (*usage_slot_6) (void) , 
              (*usage_slot_7) (void) , 
              (*usage_slot_8) (void) , 
              (*usage_slot_9) (void) , 
              (*usage_slot_10)(void) , 
              (*usage_slot_11)(void) , 
              (*usage_slot_12)(void) , 
              (*usage_slot_13)(void) , 
              (*usage_slot_14)(void) , 
              (*usage_slot_15)(void) , 
              (*usage_slot_16)(void) ,
              (*usage_slot_17)(void) ,
              (*usage_slot_18)(void) , 
              (*usage_slot_19)(void) , 
              (*usage_slot_20)(void) , 
              (*usage_slot_21)(void) , 
              (*usage_slot_22)(void) ,
              (*usage_slot_23)(void) ,
              (*usage_slot_24)(void) , 
              (*usage_slot_25)(void) , 
              (*usage_slot_26)(void) , 
              (*usage_slot_27)(void) , 
              (*usage_slot_28)(void) ,
              (*usage_slot_29)(void) ,
              (*usage_slot_30)(void) ,
              (*usage_slot_31)(void) ,
              (*usage_slot_32)(void) ,
              (*usage_slot_33)(void) ,
              (*usage_slot_34)(void) ,
              (*usage_slot_35)(void) ,
              (*usage_slot_36)(void) ,
              (*usage_slot_37)(void) ,
              (*usage_slot_38)(void) ,
              (*usage_slot_39)(void) ,
              (*usage_slot_40)(void) ,
              (*usage_slot_41)(void) ,
              (*usage_slot_42)(void) ,
              (*usage_slot_43)(void) ,
              (*usage_slot_44)(void) ,
              (*usage_slot_45)(void) ,
              (*usage_slot_46)(void) ,
              (*usage_slot_47)(void) ,
              (*usage_slot_48)(void) ,
              (*usage_slot_49)(void) ,
              (*usage_slot_50)(void) ,
              (*usage_slot_51)(void) ,
              (*usage_slot_52)(void) ,
              (*usage_slot_53)(void) ,
              (*usage_slot_54)(void) ,
              (*usage_slot_55)(void) ,
              (*usage_slot_57)(void) ,
              (*usage_slot_58)(void) ,
              (*usage_slot_59)(void) ,
              (*usage_slot_60)(void) ,
              (*usage_slot_61)(void) ,
              (*usage_slot_62)(void) ,
              (*usage_slot_63)(void) ,
              (*usage_slot_64)(void) ;




/*-----------------------------*/
/* Initialise the slot manager */
/*-----------------------------*/

_PUBLIC void slot_manager_init(void)

{   uint32_t i;

    for(i=0; i<MAX_SLOTS; ++i)
    {   seg_slot[i]   = (void *)NULL;
        usage_slot[i] = (void *)NULL;

        switch(i)
        {     case 0:    seg_slot[i]   = seg_slot_1;
                         usage_slot[i] =  usage_slot_1;
                         break; 

              case 1:    seg_slot[i]   = seg_slot_2;
                         usage_slot[i] =usage_slot_2;
                         break; 
              
              case 2:    seg_slot[i]   = seg_slot_3;
                         usage_slot[i] = usage_slot_3;
                         break; 

              case 3:    seg_slot[i]   = seg_slot_4;
                         usage_slot[i] = usage_slot_4;
                         break; 

              case 4:    seg_slot[i]   = seg_slot_5;
                         usage_slot[i] = usage_slot_5;
                         break; 

              case 5:    seg_slot[i]   = seg_slot_6;
                         usage_slot[i] = usage_slot_6;
                         break; 

              case 6:    seg_slot[i]   = seg_slot_7;
                         usage_slot[i] = usage_slot_7;
                         break; 

              case 7:    seg_slot[i]   = seg_slot_8;
                         usage_slot[i] = usage_slot_8;
                         break; 

              case 8:    seg_slot[i]   = seg_slot_9;
                         usage_slot[i] = usage_slot_9;
                         break; 

              case 9:    seg_slot[i]   = seg_slot_10;
                         usage_slot[i] = usage_slot_10;
                         break;

              case 10:   seg_slot[i]    = seg_slot_11;
                         usage_slot[i]  = usage_slot_11;
                         break; 

              case 11:   seg_slot[i]    = seg_slot_12;
                         usage_slot[i]  = usage_slot_12;
                         break; 

              case 12:   seg_slot[i]    = seg_slot_13;
                         usage_slot[i]  = usage_slot_13;
                         break; 

              case 13:   seg_slot[i]    = seg_slot_14;
                         usage_slot[i]  = usage_slot_14;
                         break; 

              case 14:   seg_slot[i]    = seg_slot_15;
                         usage_slot[i]  = usage_slot_15;
                         break; 

              case 15:   seg_slot[i]    = seg_slot_16;
                         usage_slot[i]  = usage_slot_16;
                         break; 

              case 16:   seg_slot[i]    = seg_slot_17;
                         usage_slot[i]  = usage_slot_17;
                         break; 

              case 17:   seg_slot[i]    = seg_slot_18;
                         usage_slot[i]  = usage_slot_18;
                         break; 

              case 18:   seg_slot[i]    = seg_slot_19;
                         usage_slot[i]  = usage_slot_19;
                         break; 

              case 19:   seg_slot[i]    = seg_slot_20;
                         usage_slot[i]  = usage_slot_20;
                         break; 

              case 20:   seg_slot[i]    = seg_slot_21;
                         usage_slot[i]  = usage_slot_21;
                         break; 

              case 21:   seg_slot[i]    = seg_slot_22;
                         usage_slot[i]  = usage_slot_22;
                         break; 

              case 22:   seg_slot[i]    = seg_slot_23;
                         usage_slot[i]  = usage_slot_23;
                         break; 

              case 23:   seg_slot[i]    = seg_slot_24;
                         usage_slot[i]  = usage_slot_24;
                         break; 

              case 24:   seg_slot[i]    = seg_slot_25;
                         usage_slot[i]  = usage_slot_25;
                         break; 

              case 25:   seg_slot[i]    = seg_slot_26;;
                         usage_slot[i]  = usage_slot_26;
                         break; 

              case 26:   seg_slot[i]    = seg_slot_27;
                         usage_slot[i]  = usage_slot_27;
                         break; 

              case 27:   seg_slot[i]    = seg_slot_28;
                         usage_slot[i]  = usage_slot_28;
                         break; 

              case 28:   seg_slot[i]    = seg_slot_29;
                         usage_slot[i]  = usage_slot_29;
                         break; 

              case 29:   seg_slot[i]    = seg_slot_30;
                         usage_slot[i]  = usage_slot_30;
                         break; 

              case 30:   seg_slot[i]    = seg_slot_31;
                         usage_slot[i]  = usage_slot_31;
                         break; 

              case 31:   seg_slot[i]    = seg_slot_32;
                         usage_slot[i]  = usage_slot_32;
                         break; 
         }
    }
}




/*----------------------------*/
/* Display current slot usage */
/*----------------------------*/

_PUBLIC void slot_usage(const int32_t level)

{   uint32_t    i;

    (void)fprintf(stderr,"    slot manager %s, (C) Tumbling Dice, 1999-2024 (gcc %s: built %s %s)\n",SLOTMAN_VERSION,__VERSION__,__TIME__,__DATE__);
    (void)fflush(stderr);

    (void)fprintf(stderr,"\n");
    (void)fflush(stderr);

    for(i=0; i<MAX_SLOTS; ++i)
    {   if(seg_slot[i] != NULL)
        {  (void)fprintf(stderr,"    slot:%d ",i);
           (*seg_slot[i])(level);
        }
   }


   (void)fprintf(stderr,"\n");
   (void)fflush(stderr);
   (void)exit(1);
}



/*---------------------------*/
/* Display application usage */
/*---------------------------*/

_PUBLIC void usage(void)

{   uint32_t i;

    (void)fprintf(stderr,"Slot manager %s (C) Tumbling Dice, 1999-2024 (gcc %s: built %s %s), usage:\n\n",SLOTMAN_VERSION,__VERSION__,__TIME__,__DATE__);
    (void)fflush(stderr);


    #ifdef FLOAT
    (void)fprintf(stderr,"    Floating point representation is single precision (%d bytes)\n",sizeof(FTYPE));
    #else
    (void)fprintf(stderr,"    Floating point representation is double precision (%d bytes)\n",sizeof(FTYPE));
    #endif /* FLOAT */

    #ifndef SINGLE_THREAD
    #ifdef _OPENMP
    (void)fprintf(stderr,"    %d parallel (OMP) threads available\n",omp_get_max_threads());
    #endif /* _OPENMP       */
    #else
    (void)fprintf(stderr,"    OMP is disabled\n");
    #endif /* SINGLE_THREAD */

    (void)fprintf(stderr,"\n");

    #ifdef SHADOW_SUPPORT
    (void)fprintf(stderr,"    [with shadow support                  ]\n");
    #endif /* SHADOW_SUPPORT */


    #ifdef NIS_SUPPORT
    (void)fprintf(stderr,"    [with NIS support                     ]\n");
    #endif /* NIS_SUPPORT */


    #ifdef SSH_SUPPORT
    (void)fprintf(stderr,"    [with ssh support                     ]\n");
    #endif /* SSH_SUPPORT */

    #ifdef IPV6_SUPPORT
    (void)fprintf(stderr,"    [with IPV6 support                    ]\n");
    #endif /* IPV6_SUPPORT */

    #ifdef DLL_SUPPORT
    (void)fprintf(stderr,"    [with DLL support                     ]\n");
    #endif /* DLL_SUPPORT */


    #ifdef PTHREAD_SUPPORT
    (void)fprintf(stdout,"    [with thread support                  ]\n");
    #endif /* PTHREAD_SUPPORT */


    #ifdef PERSISTENT_HEAP_SUPPORT
    (void)fprintf(stderr,"    [with persistent heap support         ]\n");
    #endif /* PERSISTENT_HEAP_SUPPORT */


    #ifdef BUBBLE_MEMORY_SUPPORT
    (void)fprintf(stderr,"    [with dynamic bubble memory support   ]\n");
    #endif /* BUBBLE_MEMORY_SUPPORT */


    #ifdef CRIU_SUPPORT
    (void)fprintf(stderr,"    [with criu support                    ]\n");
    #endif /* CRUI_SUPPORT */

    (void)fprintf(stderr,"\n\n    Application build date is %s %s\n",appl_build_time,appl_build_date);
    (void)fprintf(stderr,"\n");
    (void)fflush(stderr);

    for(i=0; i<MAX_SLOTS; ++i)
    {   if(usage_slot[i] != NULL)
           (*usage_slot[i])();
    }
 
    (void)fprintf(stderr,"\n");
    (void)fflush(stderr);
    (void)exit(1);
}




#ifdef DLL_SUPPORT
/*-----------------------------------------------------------------------*/
/* Routine to add a slot to the slot manager - this is generally done by */
/* a DLL after loading                                                   */
/*-----------------------------------------------------------------------*/

_PUBLIC void add_dll_slot(const void *d_l_handle, const char *dll_name)

{    char slot_name[SSIZE] = ""; 


     /*--------------*/
     /* Sanity check */
     /*--------------*/

     if(d_l_handle == (const void *)NULL || dll_name == (const char *)NULL)
        pups_error("[add dll slot] incorrect parameter(s)"); 

     if(max_slots > MAX_SLOTS)
        pups_error("[add_dll_slot] slot manager has no free slots");

     (void)snprintf((char *)&slot_name[0],SSIZE,"%s_slot",dll_name);
     if((seg_slot[max_slots++] = (void *)dlsym(d_l_handle,slot_name)) == NULL)
        pups_error("add_dll_slot: no slot supplied by DLL");  

     (void)fprintf(stderr,"    %-24s [at 0x%010x] registered with slot manager at slot %04d\n",
                                                                                      dll_name,
                                                                                    d_l_handle,
                                                                                   max_slots-1);
     (void)fflush(stderr);
}




/*--------------------------------------------------------------------------*/
/* Routine to add a usage slot to the slot manager - this is generally done */
/* by a DLL after loading                                                   */
/*--------------------------------------------------------------------------*/

_PUBLIC void add_dll_usage_slot(const void *d_l_handle, const char *dll_usage_name)

{    char usage_slot_name[SSIZE] = ""; 


     /*--------------*/
     /* Sanity check */
     /*--------------*/

     if(d_l_handle == (const void *)NULL || dll_usage_name == (const char *)NULL)
        pups_error("[add dll slot] incorrect parameter(s)"); 

     if(max_use_slots > MAX_SLOTS)
        pups_error("[add_dll_usage_slot] slot manager has no free usage slots");

     (void)snprintf(usage_slot_name,SSIZE,"%s_use_slot",dll_usage_name);
     if((usage_slot[max_slots] = (void *)dlsym(d_l_handle,
                                               dll_usage_name)) != NULL)
     {  (void)fprintf(stderr,"   %s registered with slot manager at usage slot %d\n",
                                                                      dll_usage_name,
                                                                         max_slots-1);
        (void)fflush(stderr);
     }
}
#endif /* DLL_SUPPORT */
