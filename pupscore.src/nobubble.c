/*-----------------------------------------------------------------------------------
    Purpose: provide null interfaces to bubble library components called by P3
             when bubble support is disabled.

    Author: M.A. O'Neill
            Tumbling Dice
            Gosforth
            Tyne and Wear
            NE3 4RT

    Version: 1.00
    Dated:   4th January 2023
    E-mail:  mao@tumblingdice.co.uk
-----------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <me.h>


/*-------------------------------*/
/* Dummy jmalloc_usage() routine */
/*-------------------------------*/

_PUBLIC int jmalloc_usage(void)
{
}
