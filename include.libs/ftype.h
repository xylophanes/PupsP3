/*------------------------------------------------------------------------------
    Header file defining floating point representation.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.00 
    Dated:   30th August 2019 
    E-Mail:  mao@tumblingdice.co.uk
------------------------------------------------------------------------------*/

#ifndef FTYPE_H 
#define FTYPE_H 

/*------------------------------------------------------------------------------
    Floating point representation ...
------------------------------------------------------------------------------*/

/********************/
/* Single precision */
/********************/

#ifdef FLOAT

#define FTNAM "float"
#define FTYPE float
#define F     "f"
#define E     "e"


/*******************/
/* Float functions */
/*******************/

#define FABS  fabsf
#define FLOOR floorf
#define CEIL  ceilf
#define ROUND roundf
#define RINT  rintf
#define SQRT  sqrtf
#define LOG   logf
#define LOG10 log10f
#define POW   powf
#define EXP   expf
#define TAN   tanf
#define ATAN  atanf
#define ATAN2 atan2f
#define COS   cosf
#define ACOS  acosf
#define SIN   sinf
#define ASIN  asinf

#else


/********************/
/* Double precision */
/********************/

#define FTNAM "double"
#define FTYPE double
#define F     "lf"
#define E     "le"


/********************/
/* Double functions */
/********************/

#define FABS  fabs
#define FLOOR floor
#define CEIL  ceil
#define ROUND round
#define RINT  rint
#define SQRT  sqrt
#define LOG   log
#define LOG10 log10
#define POW   pow
#define EXP   exp
#define TAN   tan
#define ATAN  atan
#define ATAN2 atan2
#define COS   cos
#define ACOS  acos
#define SIN   sin
#define ASIN  asin


#endif /* FLOAT */

#endif /* FTYPE_H */
