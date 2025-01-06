/*------------------------------------------------------------------------------
    Purpose: header which makes sure c2man can process PUPS/P3 libraries 

    Author:  M.A. O'Neill,
             Tumbling Dice,
             Gosforth,
             Tyne and Wear NE3 4RT.

    Version: 2.00
    Dated:   5th February 2024
    E-mail:  mao@tumblingdice.co.uk
------------------------------------------------------------------------------*/

#ifndef C2MAN_H
#define C2MAN_H

#ifdef __C2MAN__ 

/*------------------------------------------------*/
/* Typedef overides for types which confuse c2man */
/* making them void allows c2man to process them  */
/*------------------------------------------------*/

typedef void FILE;
typedef void off_t;
typedef void sigset_t;
typedef void time_t;
typedef void sigjmp_buf;
typedef void FTYPE;
typedef void pthread_t;
typedef void pthread_mutex_t;


/*------------------------------------*/
/* Types which are defined by PUPS/P3 */
/*------------------------------------*/

#include <xtypes.h>

#endif /* __C2MAN__ */
#endif /* C2MAN_H   */
