/*----------------------------------------------------------------------------- 
    Purpose: Header for the Fast Hartley Transform library.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.02 
    Dated:   4th January 2023
    E-Mail:  mao@tumblingdice.co.uk 
------------------------------------------------------------------------------*/


#ifndef FHT_H
#define FHT_H


/*------------------*/
/* Keep c2man happy */
/*------------------*/

#include <c2man.h>


/***********/
/* Version */
/***********/

#define FHT_VERSION    "3.00"


/*-------------------------------*/
/* Floating point representation */
/*-------------------------------*/

#include <ftype.h>


/*-----------------------------*/
/* Flags (for Fourier routine) */
/*-----------------------------*/

#define RESET         1     // Reset (clear storage for butterfly transform)
#define RESTART       2     // Restat (re-allocate storage for butterfly transform)


/*----------------------*/
/* Transform directions */
/*----------------------*/

#define FORWARD        1    // Forward transform flag
#define REVERSE        2    // Reverse transform flag
#define NOT_PWR_OF_TWO 0    // Not a power of two 



#ifdef __NOT_LIB_SOURCE__

/*-------------------------------------------*/
/* Public constants exported by this library */
/*-------------------------------------------*/

#else
#    undef  _EXPORT
#    define _EXPORT
#endif /* __NOT_LIB_SOURCE__ */


// Get real part of Fourier transform
_PROTOTYPE _EXPORT void get_fft_R(int, FTYPE [], FTYPE []);

// Get imaginary part of Fourier transform
_PROTOTYPE _EXPORT void get_fft_IM(int, FTYPE [], FTYPE []);

// Get power spectral density function
_PROTOTYPE _EXPORT void get_pwr_S(int, FTYPE [], FTYPE []);

// Perform Fast Hartley Transform
_PROTOTYPE _EXPORT void fht(int, int, FTYPE []);

// Set up band pass filter
_PROTOTYPE _EXPORT void band_pass(int, FTYPE, FTYPE, FTYPE, FTYPE *);

// Get Power Spectral Density Function from FFT
_PROTOTYPE _EXPORT void get_FFT_pwr_S(int, FTYPE [], FTYPE [], FTYPE []);

// Resample data [so that the number of data points is a power of two]
_PROTOTYPE _EXPORT void resample(int, int, FTYPE [], FTYPE []);

// Perform Fast Fourier Transform on complex array of data
_PROTOTYPE _EXPORT void Fourier(int, int, int, FTYPE [], FTYPE []);

// Compute linear predictive coefficients
_PROTOTYPE _EXPORT void memcof(FTYPE [], int, int, FTYPE *, FTYPE [], int);

// Compute power spectrum using maximum entropy [all poles] algorithm
_PROTOTYPE _EXPORT FTYPE evlmem(FTYPE, FTYPE [], int, FTYPE);

// Set up Daub wavelet coeffiecients for n=4, n=1 or n = 20
_PROTOTYPE _EXPORT int pwtset(int);

// Partial wavelet transform
_PROTOTYPE _EXPORT int pwt(FTYPE [], unsigned long int, int);

// Compute discrete wavelet transform of input vector
_PROTOTYPE _EXPORT void wt1(FTYPE [], unsigned long int, int, void (*)(float [], unsigned long int, int));

// Transform filter for daub4 [Daubechies 4 coeffiecient wavelets]
_PROTOTYPE _EXPORT void daub4(FTYPE [], unsigned long, int);

// Compute N dimensional wavelet transform of N dimensional dataset
_PROTOTYPE _EXPORT void wtn(FTYPE [], unsigned long int [], int, int, void (*)(float [], unsigned long int, int));

#ifdef _CPLUSPLUS
#   undef  _EXPORT
#   define _EXPORT extern C
#else
#   undef  _EXPORT
#   define _EXPORT extern
#endif

#endif /* FHT_H */
