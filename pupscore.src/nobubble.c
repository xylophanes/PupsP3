/*----------------------------------------------------------------------------
    Purpose: provide null interfaces to bubble library components called by P3
             when bubble support is disabled.

    Author: M.A. O'Neill
            Tumbling Dice
            Gosforth
            Tyne and Wear
            NE3 4RT

    Version: 1.02
    Dated:   29th December 2024
    E-mail:  mao@tumblingdice.co.uk
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <xtypes.h>
#include <stdint.h>


/*-------------------------------*/
/* Dummy jmalloc_usage() routine */
/*-------------------------------*/

_PUBLIC  int32_t jmalloc_usage(void)
{
}
