/*------------------------------------------------------------------------------
    Purpose: Personality module header defines for generic UNIX system.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 1.00
    Dated:   23rd January 2018 
    E-Mail:  mao@tumblingdice.co.uk 
------------------------------------------------------------------------------*/

#ifndef ME_H
#define ME_H


                           /*-------------------------------------------------*/
#define UNIX     1         /* Define this to be a UNIX system                 */
#define VM_SIZE  15000000  /* Virtual memory size (for shared heaps etc.)     */
                           /*-------------------------------------------------*/

#include  <ansi.h>

#ifdef _OPENMP
#include <omp.h>
#endif /* _OPENMP */


/*--------------------------------*/
/* Some systems support full ANSI */
/*--------------------------------*/

#ifdef FULL_ANSI
#   include  <unistd.h>
#   include  <stdlib.h>
#else
#   include <sys/file.h>
#endif /* FULL_ANSI */

#include  <xtypes.h>
#include  <stdio.h>
#include  <fcntl.h>
#include  <math.h>


/*------------------------------------*/
/* Set up the export control variable */
/*------------------------------------*/

#define __NOT_LIB_SOURCE__


       /*----------------------------*/
#endif /* Machine personality module */
       /*----------------------------*/
