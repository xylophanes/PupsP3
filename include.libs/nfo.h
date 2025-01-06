/*------------------------------------------------------------------------------
    Header file for numerical operations libraries,

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 3.02 
    Dated:   12th December 2024 
    E-Mail:  mao@tumblingdice.co.uk 
------------------------------------------------------------------------------*/

#ifndef NFO
#define NFO


/*------------------*/
/* Keep c2man happy */
/*------------------*/

#include <c2man.h>


/***********/
/* Version */
/***********/

#define NFO_VERSION    "3.02"



/*-------------------------------*/
/* Floating point representation */
/*-------------------------------*/

#include <ftype.h>


/*----------------------*/
/* Standard definitions */
/*----------------------*/

#define MAX_D      32           // Max minimisation dimensions
#define LINFIT     (1 << 0)     // linear regression
#define INVSFIT    (1 << 1)     // Do inverse regression
#define LOGFIT     (1 << 2)     // Do logarithmic regression 
#define PWRFIT     (1 << 3)     // Do power regression
#define EXPNFIT    (1 << 4)     // Do exponential regression




/*-------------------------------------------------------*/
/* Constant definitions used by least squares regression */
/*-------------------------------------------------------*/

#ifdef __NOT_LIB_SOURCE__

_EXPORT _CONST FTYPE NATURAL;   // Natural boundry conditions for spline
_EXPORT FTYPE  sqrarg;          // Used by the SQR macro
_EXPORT FTYPE  lower_limit;     // Lower limit for mnbrak
_EXPORT FTYPE  upper_limit;     // Upper limit for mnbrak


      /*--------------------------------*/
#else /* External variable declarations */
      /*--------------------------------*/

#   undef  _EXPORT
#   define _EXPORT
#endif /* _NOT_LIB_SOURCE_ */


/*---------------------------------------------------*/
/* Standard function/procedure prototype definitions */
/*---------------------------------------------------*/

// Least squares fitting
_PROTOTYPE _EXPORT void least_squares(uint32_t,
                                      uint32_t,
                                      FTYPE [],
                                      FTYPE [],
                                      FTYPE *,
                                      FTYPE *,
                                      FTYPE []);

// Cubic spline initialisation
_PROTOTYPE _EXPORT void spline(FTYPE [],
                               FTYPE [],
                               uint32_t, 
                               FTYPE,
                               FTYPE,
                               FTYPE []);

// Parabolic bracketer for minimia
_PROTOTYPE _EXPORT void mnbrak(FTYPE *,
                               FTYPE *,
                               FTYPE *,
                               FTYPE *,
                               FTYPE *, 
                               FTYPE *,
                               FTYPE (* )(__UDEF_ARGS__));

// Multidimensional minimisation using Powell's method [not thread safe]
_PROTOTYPE _EXPORT void Powell(FTYPE [],
                               FTYPE [][MAX_D],
                               uint32_t, 
                               uint32_t,
                               FTYPE,
                               uint32_t *,
                               FTYPE *,
                               FTYPE (* )(__UDEF_ARGS__),
                               FTYPE (* )(__UDEF_ARGS__),
                               FTYPE (* )(__UDEF_ARGS__));

// minimise N dimentional function stochatically using simulated annealing
_PROTOTYPE _EXPORT void anneal(FTYPE [], 
                               FTYPE, 
                               uint32_t, 
                               FTYPE,
                               uint32_t  *, 
                               FTYPE *,                  
                               FTYPE (* )(__UDEF_ARGS__),
                               FTYPE (* )(__UDEF_ARGS__),
                               FTYPE (* )(__UDEF_ARGS__));



// Fit function using least squares parameters
_PROTOTYPE _EXPORT FTYPE lsq_fgen(FTYPE, uint32_t, FTYPE []);

// Linear  interpolator
_PROTOTYPE _EXPORT FTYPE lint(FTYPE [], FTYPE [], uint32_t, FTYPE);

// Spline  interpolator [not thread safe]
_PROTOTYPE _EXPORT FTYPE splint(FTYPE [], FTYPE [], FTYPE [], uint32_t, FTYPE);

// Trapezoidal  integration
_PROTOTYPE _EXPORT FTYPE trapspl(FTYPE,
                                 FTYPE,
                                 FTYPE,
                                 FTYPE [],
                                 FTYPE [],
                                 FTYPE [],
                                 uint32_t);

// Integration using Simpsons' rule 
_PROTOTYPE _EXPORT FTYPE Simpson_spl(FTYPE,
                                     FTYPE,
                                     FTYPE,
                                     FTYPE [],
                                     FTYPE [],
                                     FTYPE [],
                                     uint32_t);

// Differentiation using Milnes method
_PROTOTYPE _EXPORT FTYPE Milne_diff(uint32_t   , FTYPE, FTYPE []);

// Linear minimisation using Brents' method [not thread safe]
_PROTOTYPE _EXPORT FTYPE Brent(FTYPE, 
                               FTYPE,
                               FTYPE,
                               FTYPE (* )(__UDEF_ARGS__),
                               FTYPE, 
                               FTYPE *);

// 1 dimensional minimisation by Golden Search
_PROTOTYPE _EXPORT FTYPE golden(FTYPE,
                                FTYPE,
                                FTYPE,
                                FTYPE (* )(__UDEF_ARGS__), 
                                FTYPE, 
                                FTYPE *);

#ifdef _CPLUSPLUS
#   undef  _EXPORT
#   define _EXPORT extern C
#else
#   undef  _EXPORT
#   define _EXPORT extern
#endif

#endif /* NFO.H  */

