/*-------------------------------------------------------------------------------------
    Purpose: Numerical recipes matrix routines to support PCA classification

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.00 
    Dated:   5th February 2024
    E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------------------------*/

#ifndef EIGEN_H
#define EIGEN_H


/*------------------*/
/* Keep c2man happy */
/*------------------*/

#include <c2man.h>
#include <xtypes.h>


/*-------------------------*/
/* Defines for this module */
/*-------------------------*/

/***********/
/* Version */
/***********/

#define EIGEN_VERSION  "2.00"




/*-------------------------------*/
/* Floating point representation */
/*-------------------------------*/

#include <ftype.h>


/*---------------------*/
/* Function prototypes */
/*---------------------*/

// Reduce matrix to tridiagonal form
_PROTOTYPE _EXTERN int32_t tqli(FTYPE *, FTYPE *, int32_t, FTYPE **);

// Get eigenvalues and eigenvectors of matrix by Householder reduction
_PROTOTYPE _EXTERN void tred2(_BOOLEAN, FTYPE **, int32_t, FTYPE *, FTYPE *e);

// Get eigenvalues and eigenvectors of a real symmetric matrix by Jacobi rotation
_PUBLIC int32_t jacobi(FTYPE **, int32_t, FTYPE *, FTYPE **, int32_t *);

// Sort eigenvectors  into ascending order
_PUBLIC void eigsrt(FTYPE *, FTYPE **, int32_t);

// Get pattern vector covariance matrix
_PUBLIC void get_covariance_matrix(int, int32_t, FTYPE **, FTYPE **);

// Get eigenvectors (of covariance matrix)
_PUBLIC int32_t get_eigenvectors(FTYPE, int32_t, FTYPE **, FTYPE *, FTYPE *);

// Transform pattern vectors to basis defined by eignvectors of covariance matrix
_PUBLIC FTYPE **get_eigenpatterns(int, int32_t, int32_t, FTYPE **, FTYPE **);

// Map pattern  into covariance basis (returning projection weights in that basis)
_PUBLIC FTYPE *generate_weight_vector(int32_t,  int32_t, FTYPE *, FTYPE **);

#endif /* EIGEN_H */

