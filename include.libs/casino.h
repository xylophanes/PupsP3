/*----------------------------------------------------------------------------- 
    Header for the random number generator library.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.00 
    Dated:   27th February 2024
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


/*-------------------------------*/
/* Floating point representation */
/*-------------------------------*/

#include <ftype.h>




/*---------------------------------------------------------------*
/* Definition of gate datastructure required by the user defined */
/* distribution generator                                        */
/*---------------------------------------------------------------*/

typedef struct {   uint32_t     n_gates;     // Number of gates 
                   FTYPE        frame_size;  // Total size of gate frame
                   FTYPE        *gate_x;     // Pointer to returned deviate 
                   FTYPE        *gate_y;     // Pointer to transform function 
               } gate_type;


#ifdef __NOT_LIB_SOURCE__

_EXPORT  int64_t  r_init;

      /*--------------------------------*/
#else /* External variable declarations */
      /*--------------------------------*/
#   undef  _EXPORT
#   define _EXPORT
#endif /* __NOT_LIB_SOURCE__ */




/*--------------------*/
/* Prototype bindings */
/*--------------------*/

// Return n! as floating point number
_PROTOTYPE _EXPORT FTYPE factrl(const uint32_t);

// Return ln(n!) as floating point number
_PROTOTYPE _EXPORT FTYPE factln(const uint32_t);

// Return binomial coefficient as floating point number
_PROTOTYPE _EXPORT FTYPE bico(const uint32_t, const uint32_t);

// High precision linear congruential random deviates
_PROTOTYPE _EXPORT FTYPE ran1(void);

// Reduced precision linear congruential generator 
_PROTOTYPE _EXPORT FTYPE ran2(void);

// Random deviates using Knuths' method 
_PROTOTYPE _EXPORT FTYPE ran3(void);

// Random deviate long period random number generator
_PROTOTYPE _EXPORT FTYPE ran4(int64_t *);

// Park-Miller random number generator (with Bays-Durham shuffle)
_PROTOTYPE _EXPORT FTYPE ran5(int64_t *);

// Gaussian deviates via the Box-Muller Transfor 
_PROTOTYPE _EXPORT FTYPE gasdev(FTYPE (* )(__UDEF_ARGS__), const FTYPE);

// Gaussian Deviates via Central Limit Theorem
_PROTOTYPE _EXPORT FTYPE gasdev2(FTYPE (*ran)(__UDEF_ARGS__) , const uint32_t, const FTYPE);

// Gamma deviates via substitution method
_PROTOTYPE _EXPORT FTYPE gamdev(const uint32_t, FTYPE (* )(__UDEF_ARGS__));

// Poisson distribution via substitution method
_PROTOTYPE _EXPORT FTYPE poidev(FTYPE, FTYPE (* )(__UDEF_ARGS__));

// Binomial deviates via substitution method
_PROTOTYPE _EXPORT FTYPE bnldev(const FTYPE, const uint32_t, FTYPE (* )(__UDEF_ARGS__));

// User defined deviates via substitution method
_PROTOTYPE _EXPORT FTYPE numdev(FTYPE (* )(__UDEF_ARGS__), const gate_type *);

// Number of unique combinations of n objects taken from m
_PROTOTYPE _EXPORT int32_t n_from_m(const uint32_t, const uint32_t);

// Generate error function erfcc
_PROTOTYPE _EXPORT FTYPE erfcc(const FTYPE x);

// Incomplete beta function
_PROTOTYPE _EXPORT FTYPE betai(const FTYPE, const FTYPE, const FTYPE);


// Linear correlation coefficient (Pearson's r)
_PROTOTYPE _EXPORT void  pearsn(const FTYPE []      ,
                                const FTYPE []      ,
                                const uint32_t      ,
                                FTYPE              *,
                                FTYPE              *,
                                FTYPE              *);

// Spearman's rank correlation coefficient
_PROTOTYPE _EXPORT void  spear(const FTYPE []       ,
                               const FTYPE []       ,
                               const uint32_t       ,
                               FTYPE               *,
                               FTYPE               *,
                               FTYPE               *,
                               FTYPE               *,
                               FTYPE               *);


// Compute mean, standard, deviation, variance, skewness and kurtosis of distribution 
_PROTOTYPE _EXPORT void moment(const FTYPE     [] ,  // Data to be analysed
                               const uint32_t     ,  // Number of pts in dataset
                               FTYPE          *ave,  // Average deviation
                               FTYPE          *rms,  // Root mean square error
                               FTYPE         *adev,  // Average deviation
                               FTYPE         *sdev,  // Standard deviation
                               FTYPE         *svar,  // Variance
                               FTYPE         *skew,  // Skewness
                               FTYPE         *curt); // Kurtosis

// Student's t test to test if two populations have significantly different means 
_PROTOTYPE _EXPORT void ttest(const FTYPE [], const uint32_t, const FTYPE [], const uint32_t, FTYPE *, FTYPE *);

// Find mean and variance of data array
_PROTOTYPE _EXPORT void avevar(const FTYPE [], const uint32_t, FTYPE *ave, FTYPE *var);

// Student's t for significantly different means where the two populations have significantly different variances
_PROTOTYPE _EXPORT void tutest(const FTYPE [], const uint32_t, const FTYPE [], const uint32_t, FTYPE *, FTYPE *);

// Chi-square measure of association for two different distributions 
_PROTOTYPE _EXPORT void cntab1(const uint32_t  **, const uint32_t, const uint32_t, FTYPE *,  FTYPE *,  FTYPE *,  FTYPE *,  FTYPE *);

// Evaluate the continued fraction for incomplete beta function using Lentz's method
_PROTOTYPE _EXPORT FTYPE betacf(const FTYPE, const FTYPE, const FTYPE);

// Evaluate the incomplete gamma function
_PROTOTYPE _EXPORT FTYPE gammq(const FTYPE, const FTYPE);

// Evaluate the incomplete gamma function by its series representation
_PROTOTYPE _EXPORT void gser(FTYPE *, const FTYPE, const FTYPE, FTYPE *);

// Evaluate the incomplete gamma function by its continued fraction
_PROTOTYPE _EXPORT void gcf(FTYPE *, const FTYPE, const FTYPE, FTYPE *);

#ifdef _CPLUSPLUS
#   undef  _EXPORT
#   define _EXPORT extern C
#else
#   undef  _EXPORT
#   define _EXPORT extern
#endif

#endif /* CASINO.H */
