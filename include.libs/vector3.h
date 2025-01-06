/*------------------------------------------------------------------------------
    Header for vector3/matrix3 linear programming library ...

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

     Version: 2.09 
     Date:    12th December 2024
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

_EXPORT _CONST int32_t ANNOT;
_EXPORT _CONST int32_t N_ANNOT;

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
_PROTOTYPE _EXPORT void v3ass(vector3 *, const FTYPE, const FTYPE, const FTYPE);

// Write vector3 to stream
_PROTOTYPE _EXPORT void v3write(const char *, const vector3 *, const FILE *);

// Read vector3 from stream
_PROTOTYPE _EXPORT int32_t v3read(char *, vector3 *, const FILE *);

// Test whether vector3s parallel
_PROTOTYPE _EXPORT _BOOLEAN v3par(const vector3 *, const vector3 *);

// Test if two vector3's are equal
_PROTOTYPE _EXPORT _BOOLEAN v3eq(const vector3 *, const vector3 *);

// Compute component sign hetrogeneity of a vector3
_PROTOTYPE _EXPORT _BOOLEAN v3hecs(const vector3 *);

// Get vector3 magnitude
_PROTOTYPE _EXPORT FTYPE v3mag(const vector3 *);

// Vector3 scalar (dot) product of two vector3's
_PROTOTYPE _EXPORT FTYPE v3dot(const vector3 *arg1, const vector3 *arg2);

// Get angle between two vector3s
_PROTOTYPE _EXPORT FTYPE v3ang(const vector3 *, const vector3*);

// Get scalar distance between a point in 3 space and line
_PROTOTYPE _EXPORT FTYPE v3dpoints(const vector3 *, const vector3 *, const vector3 *);

// Compute the spatial entropy of a set of vector3's
_PROTOTYPE _EXPORT FTYPE v3lvsi(const int32_t, const vector3 *);

// Convert vector3 to matrix3 column
_PROTOTYPE _EXPORT void v3tomatc(const uint32_t, matrix3 *, const vector3 *);

// Calculate the additive inverse of a vector3
_PROTOTYPE _EXPORT vector3 v3inv(const vector3 *);

// Zero vector3
_PROTOTYPE _EXPORT void v3zero(vector3 *);

// Add two vector3s
_PROTOTYPE _EXPORT vector3 v3add(const vector3 *, const vector3 *);

// Subtract vector3 from vector3
_PROTOTYPE _EXPORT vector3 v3sub(const vector3 *, const vector3 *);

// Vector3 cross product of two vector3's
_PROTOTYPE _EXPORT vector3 v3cross(const vector3 *, const vector3 *);

// Find unit vector3 in direction of argument vector3
_PROTOTYPE _EXPORT vector3 v3unit(const vector3 *);

// Express vector3 in given axis system
_PROTOTYPE _EXPORT  vector3 v3axtran(const vector3 *,
                                     const vector3 *,
                                     const vector3 *,
                                     const vector3 *);

// Get sign vector3 of argument vector3 
_PROTOTYPE _EXPORT vector3 v3sign(const vector3 *);

// Multiply vector3 by scalar
_PROTOTYPE _EXPORT vector3 v3scalm(const FTYPE, const vector3 *);

// Divide vector3 by scalar
_PROTOTYPE _EXPORT vector3 v3scald(const FTYPE, const vector3 *);

// Find vector3 perpendicular to plane defined by #1 and #2
_PROTOTYPE _EXPORT vector3 v3planp(const vector3 *, const vector3 *);

// Generate unit vector3 pointing in random direction
_PROTOTYPE _EXPORT vector3 v3urnd(void);

// Generate unit vector3 pointing in random direction of random size
_PROTOTYPE _EXPORT vector3 v3rnd(const FTYPE);

// Rotate vector3 theta degrees about X axis
_PROTOTYPE _EXPORT vector3 v3rotx(const vector3 *, const FTYPE);

// Rotate vector3 theta degrees about Y axis
_PROTOTYPE _EXPORT vector3 v3roty(const vector3 *, const FTYPE);

// Rotate vector3 theta degrees about Z axis
_PROTOTYPE _EXPORT vector3 v3rotz(const vector3 *, const FTYPE);

// Multiply vector3 by matrix3
_PROTOTYPE _EXPORT vector3 v3mmult(const matrix3 *, const vector3 *);

// Calculate smallest vector3 between line and point
_PROTOTYPE _EXPORT vector3 v3dpointv(const vector3 *, const vector3 *, const vector3 *);

// Calculate smallest vector3 between two skew lines
_PROTOTYPE _EXPORT vector3 v3dlinesv(uint32_t      *,   // Singular
                                      vector3         *,   // Ray-ray divergence vector3
                                      const vector3   *,   // Vector to line 1
                                      const vector3   *,   // Vector along line 1
                                      const vector3   *,   // Vector to line 2
                                      const vector3   *,   // Vector along line 2
                                      vector3         *,   // Skew (intercept) vector3 for line 1
                                      vector3         *);  // Skew (intercept) vector3 for line 2

// Compute mean nearest approach vector3 of set of skew lines
_PROTOTYPE _EXPORT vector3 v3mlinesv(uint32_t *, int32_t, const vector3 *, const vector3 *, FTYPE *);

// Compute planar distortion to vector3 caused by rotation projection
_PROTOTYPE _EXPORT vector3 v3distort(const vector3 *, const FTYPE, const FTYPE);


/*-----------------------------*/
/* Standard 3matrix3 functions */
/*-----------------------------*/

// Assign zero matrix3 
_PROTOTYPE _EXPORT void m3zero(matrix3 *);

// Assign matrix3
_PROTOTYPE _EXPORT void m3ass(matrix3   *,
                              const FTYPE, 
                              const FTYPE,
                              const FTYPE,
                              const FTYPE,
                              const FTYPE,
                              const FTYPE,
                              const FTYPE,
                              const FTYPE,
                              const FTYPE);

// Assign identity matrix3
_PROTOTYPE _EXPORT void m3ident(matrix3 *);

// Write matrix3 to stream
_PROTOTYPE _EXPORT void m3write(const FILE *, const char *, const matrix3 *);

// Test if matrices are equal
_PROTOTYPE _EXPORT _BOOLEAN m3eq(const matrix3 *, const matrix3 *);

// Add pair of matrix3's
_PROTOTYPE _EXPORT matrix3 m3add(const matrix3 *, const matrix3 *);

// Subtract matrix3 from matrix3
_PROTOTYPE _EXPORT matrix3 m3sub(const matrix3 *, const matrix3 *);

// Multiply a pair of matrix3's
_PROTOTYPE _EXPORT matrix3 m3mult(const matrix3 *, const matrix3 *);

// Multiply matrix3 by scalar
_PROTOTYPE _EXPORT matrix3 m3scalm(const FTYPE, const matrix3 *);

// Find the transpose of matrix3 */
_PROTOTYPE _EXPORT matrix3 m3transp(const matrix3 *);

// Multiply matrix3 by scalar
_PROTOTYPE _EXPORT matrix3 mscalm(FTYPE, matrix3 *);


/*################################################*/
/* Euler matrices are only defined for 3D vectors */
/*################################################*/
#ifdef THREE_SPACE
// Generate Euler rotation matrix3
_PROTOTYPE _EXPORT matrix3 m3euler(FTYPE, vector3 *);

// Form small angle Euler rotation matrix3
_PROTOTYPE _EXPORT matrix3 m3seuler(FTYPE, vector3 *);
#endif /* THREE_SPACE */


// Is matrix3 symmetric?
_PROTOTYPE _EXPORT _BOOLEAN m3sym(const matrix3 *arg);


/*##########################################################*/
/* Determinant computation is only defined for 3x3 matrices */
/*##########################################################*/
#ifdef THREE_SPACE
// Computer determinant of matrix3
_PROTOTYPE _EXPORT FTYPE m3det(const matrix3 *arg);
#endif /* THREE_SPACE */


// Invert a matrix3 using LU decomposition
_PROTOTYPE _EXPORT matrix3 m3inv(matrix3 *);

// Solve 3x3 simultaneous equations using Gaussian Elimination
_PROTOTYPE _EXPORT _BOOLEAN m3GE_solve(uint32_t *, const matrix3 *, const vector3 *, vector3 *);

#ifdef _CPLUSPLUS
#   undef  _EXPORT
#   define _EXPORT extern C
#else
#   undef _EXPORT
#   define _EXPORT extern
#endif

#endif /* VECTOR3_H */

