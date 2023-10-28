/*----------------------------------------------------------------------------- 
    Header for the random number generator library.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.00 
    Dated:   4th January 2023
    E-Mail:  mao@tumblingdice.co.uk 
------------------------------------------------------------------------------*/


#ifndef CASINO_H
#define CASINO_H


/***********/
/* Version */
/***********/

#define CASINO_VERSION  "2.00"


/*------------------*/
/* Keep c2man happy */
/*------------------*/

#include <c2man.h>


/*------------------------------------------------------------------------------
    Floating point representation ...
------------------------------------------------------------------------------*/

#include <ftype.h>




/*------------------------------------------------------------------------------
    Definition of gate datastructure required by the user defined
    distribution generator ...
------------------------------------------------------------------------------*/

typedef struct {   int   n_gates;     // Number of gates 
                   FTYPE frame_size;  // Total size of gate frame
                   FTYPE *gate_x;     // Pointer to returned deviate 
                   FTYPE *gate_y;     // Pointer to transform function 
               } gate_type;


#ifdef __NOT_LIB_SOURCE__

_EXPORT long r_init;

#else /* External variable declarations */
#   undef  _EXPORT
#   define _EXPORT
#endif /* __NOT_LIB_SOURCE__ */




/*------------------------------------------------------------------------------
    Prototype bindings ...
------------------------------------------------------------------------------*/

// Return N! as floating point number
_PROTOTYPE _EXPORT FTYPE factrl(int);

// Return binomial coefficient as floating point number
_PROTOTYPE _EXPORT FTYPE bico(int, int);

// High precision linear congruential random deviates
_PROTOTYPE _EXPORT FTYPE ran1(void);

// Reduced precision linear congruential generator 
_PROTOTYPE _EXPORT FTYPE ran2(void);

// Random deviates using Knuths' method 
_PROTOTYPE _EXPORT FTYPE ran3(void);

// Random deviate long period random number generator
_PROTOTYPE _EXPORT FTYPE ran4(long *);

// Park-Miller random number generator (with Bays-Durham shuffle)
_PROTOTYPE _EXPORT FTYPE ran5(long *);

// Gaussian deviates via the Box-Muller Transfor 
_PROTOTYPE _EXPORT FTYPE gasdev(FTYPE (* )(__UDEF_ARGS__), FTYPE);

// Gaussian Deviates via Central Limit Theorem
_PROTOTYPE _EXPORT FTYPE gasdev2(FTYPE (* )(__UDEF_ARGS__), FTYPE);

// Gamma deviates via substitution method
_PROTOTYPE _EXPORT FTYPE gamdev(int, FTYPE (* )(__UDEF_ARGS__));

// Poisson distribution via substitution method
_PROTOTYPE _EXPORT FTYPE poidev(FTYPE, FTYPE (* )(__UDEF_ARGS__));

// Binomial deviates via substitution method
_PROTOTYPE _EXPORT FTYPE bnldev(FTYPE, int, FTYPE (* )(__UDEF_ARGS__));

// User defined deviates via substitution method
_PROTOTYPE _EXPORT FTYPE numdev(FTYPE (* )(__UDEF_ARGS__), gate_type *);

// Number of unique combinations of n objects taken from m
_PROTOTYPE _EXPORT int n_from_m(int, int);

// Linear correlation coefficient [Pearson's r]
_PROTOTYPE _EXPORT void  pearsn(FTYPE [],
                                FTYPE [],
                                int      ,
                                FTYPE  *,
                                FTYPE  *,
                                FTYPE  *);

// Spearman's rank correlation coefficient
_PROTOTYPE _EXPORT void  spear(FTYPE [],
                               FTYPE [],
                               int      ,
                               FTYPE  *,
                               FTYPE  *,
                               FTYPE  *,
                               FTYPE  *,
                               FTYPE  *);


// Write to (enigma) encrypted data stream
_PROTOTYPE _EXPORT int enigma8read(unsigned long, int, _BYTE *, unsigned long);

// Read from (enigma) encrypted data stream
_PROTOTYPE _EXPORT  int enigma8read(unsigned long, int, _BYTE *, unsigned long);

#ifdef _CPLUSPLUS
#   undef  _EXPORT
#   define _EXPORT extern C
#else
#   undef  _EXPORT
#   define _EXPORT extern
#endif

#endif /* CASINO.H */
