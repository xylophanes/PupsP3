/*------------------------------------------------------------------------------
    Header for vector3/matrix3 linear programming library ...

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

     Version: 2.09 
     Date:    4th January 2023
     E-mail:  mao@tumblingdice.co.uk
------------------------------------------------------------------------------*/

#ifndef VECTOR3_H
#define VECTOR3_H


/*------------------*/
/* Keep c2man happy */
/*------------------*/

#include <c2man.h>


/***********/
/* Version */
/***********/

#define VEC3_VERSION    "2.09"

/*-------------------------------*/
/* Floating point representation */
/*-------------------------------*/

#include <ftype.h>


/*-------------------------------------------*/
/* Variables exported by the vector3 library */
/*-------------------------------------------*/

#ifdef _NOT_LIB_SOURCE__

_EXPORT _CONST int ANNOT;
_EXPORT _CONST int N_ANNOT;

#else
#   undef  _EXPORT
#   define _EXPORT
#endif /* _NOT_LIB_SOURCE__ */


/*-----------------------------------------*/
/* Return types (for Gaussian elimination) */
/*-----------------------------------------*/

#define NOT_SINGULAR        0
#define IS_SINGULAR         1
#define EXIT_IF_SINGULAR    0
#define DISCARD_IF_SINGULAR 1


/*--------------------------------*/
/* Definition of the vector3 type */
/*--------------------------------*/

typedef struct {  FTYPE comp[3];                // Vector components
               }  vector3;




/*--------------------------------*/
/* Definition of the matrix3 type */
/*--------------------------------*/

typedef struct {   FTYPE comp[3][3];            // Matrix components
               } matrix3;




/*------------------------------------------------------------------------------
    These are the functions which are associated with the type vector3 ...
------------------------------------------------------------------------------*/

// Assign a vector3
_PROTOTYPE _EXPORT void v3ass(vector3 *, FTYPE, FTYPE, FTYPE);

// Print contents of a vector3
_PROTOTYPE _EXPORT void v3print(FILE *, char *, vector3 *);

// Write vector3 to stream
_PROTOTYPE _EXPORT void v3write(char *, vector3 *, FILE *);

// Solve linear equations using Gaussian Elimination
_PROTOTYPE _EXPORT _BOOLEAN m3GE_solve(int *, matrix3 *, vector3 *, vector3 *);

// Read vector3 from stream
_PROTOTYPE _EXPORT int v3read(char *, vector3 *, FILE *);

// Test whether vector3s parallel
_PROTOTYPE _EXPORT _BOOLEAN v3par(vector3 *, vector3 *);

// Test whether vector3s equal
_PROTOTYPE _EXPORT _BOOLEAN v3eq(vector3 *, vector3 *);

// Test vector3 sign hetrogeneity
_PROTOTYPE _EXPORT _BOOLEAN v3hecs(vector3 *);

// Get vector3 magnitude
_PROTOTYPE _EXPORT FTYPE v3mag(vector3 *);

// Get dot product
_PROTOTYPE _EXPORT FTYPE v3dot(vector3 *, vector3*);

// Get angle between two vector3s
_PROTOTYPE _EXPORT FTYPE v3ang(vector3 *, vector3*);

// Get scalar distance between a point in 3 space and line
_PROTOTYPE _EXPORT FTYPE v3dpoints(vector3 *, vector3 *, vector3 *);

// Compute the spatial entropy of a set of vector3s
_PROTOTYPE _EXPORT FTYPE v3lvsi(int, vector3 *);

// Convert vector3 to matrix3 column
_PROTOTYPE _EXPORT void v3tomatc(int, matrix3 *, vector3 *);

// Calculate the additive inverse of a vector3
_PROTOTYPE _EXPORT vector3 v3inv(vector3 *);

// Zero a vector3
_PROTOTYPE _EXPORT void v3zero(vector3 *);

// Add vector3s
_PROTOTYPE _EXPORT vector3 v3add(vector3 *, vector3 *);

// Subtract vectors
_PROTOTYPE _EXPORT vector3 v3sub(vector3 *, vector3 *);

// Shift vector3
_PROTOTYPE _EXPORT vector3 v3shift(vector3 *, vector3 *);

// Find cross product for pair of 3-vector3s
_PROTOTYPE _EXPORT vector3 v3cross(vector3 *, vector3 *);

// Find unit vector3 in direction of argument vector3
_PROTOTYPE _EXPORT vector3 v3unit(vector3 *);

// Express vector3 in given axis system
_PROTOTYPE _EXPORT  vector3 v3axtran(vector3 *,  \
                                     vector3 *,  \
                                     vector3 *,  \
                                     vector3 *);

// Compute vector3 sign
_PROTOTYPE _EXPORT vector3 v3sign(vector3 *);

// Multiply vector3 by scalar
_PROTOTYPE _EXPORT vector3 v3scalm(FTYPE, vector3 *);

// Divide vector3 by scalar
_PROTOTYPE _EXPORT vector3 v3scald(FTYPE, vector3 *);

// Find vector3 perpendicular to plane defined by #1 and #2
_PROTOTYPE _EXPORT vector3 v3planp(vector3 *, vector3 *);

// Generate unit vector3 pointing in random direction
_PROTOTYPE _EXPORT vector3 v3urnd(void);

// Generate unit vector3 pointing in random direction of random size
_PROTOTYPE _EXPORT vector3 v3rnd(FTYPE);

// Rotate vector3 about X axis of reference axis system
_PROTOTYPE _EXPORT vector3 v3rotx(vector3 *, FTYPE);

// Rotate vector3 about y axis of reference axis system
_PROTOTYPE _EXPORT vector3 v3roty(vector3 *, FTYPE);

// Rotate vector3 about z axis of reference axis system
_PROTOTYPE _EXPORT vector3 v3rotz(vector3 *, FTYPE);

// Multiply vector3 by matrix3
_PROTOTYPE _EXPORT vector3 v3mult(matrix3 *, vector3 *);

// Calculate smallest vector3 between line and point
_PROTOTYPE _EXPORT vector3 v3dpointv(vector3 *, vector3 *, vector3 *);

// Calculate smallest vector3 between two skew lines
_PROTOTYPE _EXPORT  vector3 v3dlinesv(int    *,
                                      vector3 *,
                                      vector3 *,
                                      vector3 *,
                                      vector3 *,
                                      vector3 *,
                                      vector3 *,
                                      vector3 *);

// Compute the nearest approach of m vector3s
_PROTOTYPE _EXPORT vector3 v3mlinesv(int *, int, vector3 *, vector3 *, FTYPE *);

// Compute planar distortion to vector3 caused by rotation projection
_PROTOTYPE _EXPORT vector3 v3distort(vector3 *, FTYPE, FTYPE);


/*-----------------------------*/
/* Standard 3matrix3 functions */
/*-----------------------------*/

// Assign zero matrix3 
_PROTOTYPE _EXPORT void m3zero(matrix3 *);

// Assign 3x3 matrix3
_PROTOTYPE _EXPORT void m3ass(matrix3 *,
                              FTYPE,    \
                              FTYPE,    \
                              FTYPE,    \
                              FTYPE,    \
                              FTYPE,    \
                              FTYPE,    \
                              FTYPE,    \
                              FTYPE,    \
                              FTYPE);

// Assign identity matrix3
_PROTOTYPE _EXPORT void m3ident(matrix3 *);

// Print matrix3
_PROTOTYPE _EXPORT void m3print(FILE *, char *, matrix3 *);

// Test if matrix3 is symmetric
_PROTOTYPE _EXPORT _BOOLEAN m3sym(matrix3 *);

// Test if matrices are equal
_PROTOTYPE _EXPORT _BOOLEAN m3eq(matrix3 *, matrix3 *);

// Add matrices
_PROTOTYPE _EXPORT matrix3 m3add(matrix3 *, matrix3 *);

// Subtract matrices
_PROTOTYPE _EXPORT matrix3 m3sub(matrix3 *, matrix3 *);

// Multiply matrices
_PROTOTYPE _EXPORT matrix3 m3mult(matrix3 *, matrix3 *);

// Multiply matrix3 by scalar
_PROTOTYPE _EXPORT matrix3 mscalm(FTYPE, matrix3 *);

// Form Euler matrix3
_PROTOTYPE _EXPORT matrix3 m3euler(FTYPE, vector3 *);

// Form small angle Euler matrix3
_PROTOTYPE _EXPORT matrix3 m3seuler(FTYPE, vector3 *);

// Find inverse of matrix3
_PROTOTYPE _EXPORT matrix3 m3inv(matrix3 *);

#ifdef _CPLUSPLUS
#   undef  _EXPORT
#   define _EXPORT extern C
#else
#   undef _EXPORT
#   define _EXPORT extern
#endif

#endif /* VECTOR3_H */

