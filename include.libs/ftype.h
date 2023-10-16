/*-----------------------------------------------------
    Header file defining floating point representation.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.04 
    Dated:   8th September 2022
    E-Mail:  mao@tumblingdice.co.uk
-----------------------------------------------------*/

#ifndef FTYPE_H 
#define FTYPE_H 


/*--------*/
/* Limits */
/*--------*/

#include <float.h>

/*-------------------------------*/
/* Floating point representation */
/*-------------------------------*/

/********************/
/* Single precision */
/********************/

#ifdef FLOAT

#define FTYPE_MIN  FLT_MIN
#define FTYPE_MAX  FLT_MAX

#define FTNAM      "float"
#define FTYPE      float
#define F          f
#define E          e


/*******************/
/* Float functions */
/*******************/

#define FABS       fabsf
#define FLOOR      floorf
#define CEIL       ceilf
#define ROUND      roundf
#define RINT       rintf
#define SQRT       sqrtf
#define LOG        logf
#define LOG10      log10f
#define POW        powf
#define EXP        expf
#define TAN        tanf
#define ATAN       atanf
#define ATAN2      atan2f
#define COS        cosf
#define ACOS       acosf
#define SIN        sinf
#define ASIN       asinf
#define LGAMMA     lgammaf
#define LGAMMA_R   lgammaf_r

#endif /* FLOAT */


/********************/
/* Double precision */
/********************/

#ifdef DOUBLE 

#define FTYPE_MIN  DBL_MIN
#define FTYPE_MAX  DBL_MAX
#define FTNAM      "double"
#define FTYPE      double
#define F          lf
#define E          le


/********************/
/* Double functions */
/********************/

#define FABS       fabs
#define FLOOR      floor
#define CEIL       ceil
#define ROUND      round
#define RINT       rint
#define SQRT       sqrt
#define LOG        log
#define LOG10      log10
#define POW        pow
#define EXP        exp
#define TAN        tan
#define ATAN       atan
#define ATAN2      atan2
#define COS        cos
#define ACOS       acos
#define SIN        sin
#define ASIN       asin
#define LGAMMA     lgamma
#define LGAMMA_R   lgamma_r

#endif /* DOUBLE */


/*************************/
/* Long double precision */
/*************************/

#ifdef LDOUBLE 

#define FTYPE_MIN  DBL_MIN
#define FTYPE_MAX  DBL_MAX
#define FTNAM      "long double"
#define FTYPE      long double
#define F          Lf
#define E          Le


/*************************/
/* Long double functions */
/*************************/

#define FABS       fabsl
#define FLOOR      floorl
#define CEIL       ceill
#define ROUND      roundl
#define RINT       rintl
#define SQRT       sqrtl
#define LOG        logl
#define LOG10      log10l
#define POW        powl
#define EXP        expl
#define TAN        tanl
#define ATAN       atanll
#define ATAN2      atan2l
#define COS        cosl
#define ACOS       acosl
#define SIN        sinl
#define ASIN       asinl
#define LGAMMA     lgammal
#define LGAMMA_R   lgammal_r

#endif /* LDOUBLE */

#endif /* FTYPE_H */
