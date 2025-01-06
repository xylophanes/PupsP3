/*------------------------------------------------------------------------------
    Purpose: Standard vector operations library. This version has been
             corrected to remove the bugs found by Mike Cook of Laserscan
             Laboratories [mikec@uk.co.lsl]


    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.11
    Dated:   12th December 2024
    E-mail:  mao@tumblingdice.co.uk 
------------------------------------------------------------------------------*/

#include <me.h>

#undef   __NOT_LIB_SOURCE__
#include <vector3.h>
#define  __NOT_LIB_SOURCE__

#include <casino.h>
#include <utils.h>


/*-----------------------------------------------*/
/* Slot and usage functions used by slot manager */
/*-----------------------------------------------*/
/*---------------------*/
/* Slot usage function */
/*---------------------*/

_PRIVATE void veclib_slot(int32_t level)
{   (void)fprintf(stderr,"lib vec3lib %s: [ANSI C]\n",VEC3_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(C) 1985-2024 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"PUPS/P3 vector support library (gcc %s: built %s %s)\n\n",__VERSION__,__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");
    (void)fflush(stderr);
}




/*---------------------------------------------*/
/* Segment identification for 3-vector library */
/*---------------------------------------------*/

#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT )() __attribute__ ((aligned(16))) = veclib_slot;
#endif /* SLOT */



/*-----------------------------------------*/
/* Constants exported by the vector module */
/*-----------------------------------------*/

#define NOT_ASSIGNED   (-999.0)

_PUBLIC  _CONST int32_t  ANNOT   = 1;
_PUBLIC  _CONST int32_t  N_ANNOT = 0;
_PRIVATE _CONST FTYPE    TINY    = 1.0e-20;


/*------------------------------------------*/
/* Skewness threshold for vmlinesv function */
/*------------------------------------------*/

#define NO_SKEW_THRESH -1


/*-------------------------------------------------*/
/* Maximum number of vectors for vmlinesv function */
/*-------------------------------------------------*/

#define MAX_VECS 256 




/*---------------------------------------------*/
/* Functions which are private to this library */
/*---------------------------------------------*/

// Eliminate routine for Gaussian elimination
_PROTOTYPE _PRIVATE _BOOLEAN eliminate(int32_t *, matrix3 *, vector3 *);

// Re-order routine for Gaussian elimination
_PROTOTYPE _PRIVATE _BOOLEAN reorder(uint32_t, matrix3 *, vector3 *);

// Back substitution for Gaussian elimination 
_PROTOTYPE _PRIVATE void backsub(const matrix3 *, const vector3 *, vector3 *);

// LU back substitution routine
_PROTOTYPE _PRIVATE void lubksb(int32_t [], matrix3 *, vector3 *);

// LU decomposisition routine
_PROTOTYPE _PRIVATE void ludcmp(int32_t [], FTYPE *, matrix3 *);




/*----------------*/
/* Assign vector3 */
/*----------------*/

_PUBLIC void v3ass(vector3         *arg,
                      const FTYPE   comp_1,
                      const FTYPE   comp_2,
                      const FTYPE   comp_3)

{   arg->comp[0] = comp_1;
    arg->comp[1] = comp_2;
    arg->comp[2] = comp_3;
}




/*--------------*/
/* Zero vector3 */
/*--------------*/

_PUBLIC void v3zero(vector3 *v)

{   uint32_t i;

    for(i=0; i<3; ++i)
       v->comp[i] = 0.0;
}




/*-------------------*/
/* Add two vector3's */
/*-------------------*/

_PUBLIC vector3 v3add(const vector3 *arg1, const vector3 *arg2)

{   uint32_t   i;
    vector3 ret;

    for(i=0; i<3; ++i)
          ret.comp[i] = arg1->comp[i] + arg2->comp[i];

    return(ret);
}




/*--------------------------------------*/
/* Calculate angle between two vector3s */
/*--------------------------------------*/

_PUBLIC FTYPE v3ang(const vector3 *arg1, const vector3 *arg2)

{   FTYPE arg;

    arg = FABS(v3dot(arg1,arg2)/(v3mag(arg1)*v3mag(arg2)));

    if(arg > 1.0)
       arg = 1.0;

    return(ACOS(arg));
}



/*------------------------------------------------------------*/
/* Transform a vector3 expressed in terms of a given axis set */
/* in terms of another orthogonal axis set                    */
/*------------------------------------------------------------*/

_PUBLIC vector3 v3axtran(const vector3  *arg,
                         const vector3 *ax_x,
                         const vector3 *ax_y,
                         const vector3 *ax_z)

{   vector3 ret;

    ret.comp[0] = arg->comp[0]*ax_x->comp[0] +
                  arg->comp[1]*ax_y->comp[0] +
                  arg->comp[2]*ax_z->comp[0];
 
    ret.comp[1] = arg->comp[0]*ax_x->comp[1] +
                  arg->comp[1]*ax_y->comp[1] +
                  arg->comp[2]*ax_z->comp[1];

    ret.comp[2] = arg->comp[0]*ax_x->comp[2] +
                  arg->comp[1]*ax_y->comp[2] +
                  arg->comp[2]*ax_z->comp[2];

    return(ret);
}




/*------------------------------------------*/
/* Vector3 (cross) product of two vector3's */
/*------------------------------------------*/

_PUBLIC vector3 v3cross(const vector3 *arg1, const vector3 *arg2)

{   vector3 ret;

    ret.comp[0] = arg1->comp[1]*arg2->comp[2] -
                   arg1->comp[2]*arg2->comp[1];

    ret.comp[1] = arg1->comp[2]*arg2->comp[0] -
                   arg1->comp[0]*arg2->comp[2];

    ret.comp[2] = arg1->comp[0]*arg2->comp[1] -
                   arg1->comp[1]*arg2->comp[0];
    return(ret);
}




/*-----------------------------------------------*/
/* Vector3 scalar (dot) product of two vector3's */
/*-----------------------------------------------*/

_PUBLIC FTYPE v3dot(const vector3 *arg1, const vector3 *arg2)

{   uint32_t i;
    FTYPE    sum = 0.0;

    for(i=0; i<3; ++i)
        sum += arg1->comp[i]*arg2->comp[i];

    return(sum);
}




/*---------------------------------*/
/* Test if two vector3's are equal */
/*---------------------------------*/

_PUBLIC _BOOLEAN v3eq(const vector3 *arg1, const vector3 *arg2)

{  uint32_t i,
            cnt = 0;

   for(i=0; i<3; ++i)
       if(arg1->comp[i] - arg2->comp[i] < (FTYPE)ASSUMED_ZERO)
          ++cnt;

       if(cnt == 3)
          return(TRUE);
       else
          return(FALSE);
}




/*----------------*/
/* Invert vector3 */
/*----------------*/

_PUBLIC vector3 v3inv(const vector3 *arg)

{   uint32_t i;
    vector3      ret;

    for(i=0; i<3; ++i)
        ret.comp[i] = -arg->comp[i];

    return(ret);
}




/*-------------------------------*/
/* Find the magnitude of vector3 */
/*-------------------------------*/

_PUBLIC FTYPE v3mag(const vector3 *arg)

{   uint32_t i;
    FTYPE    sum;

    sum = 0.0;
    for(i=0; i<3; ++i)
       sum += sqr(arg->comp[i]);

    return(SQRT(sum));
}




/*---------------------------------------*/
/* Test if pair of vector3s are parallel */
/*---------------------------------------*/

_PUBLIC _BOOLEAN v3par(const vector3 *arg1, const vector3 *arg2)

{   FTYPE angle;


    /*-----------------------------------------------------------------------*/
    /* See if two vectors are parallel by taking the cross product, and then */
    /* seeing whether all the terms are zero                                 */
    /*-----------------------------------------------------------------------*/

    angle = v3ang(arg1,arg2);
    if(angle < (FTYPE)ASSUMED_ZERO)
       return(TRUE);
    else
       return(FALSE);
}




/*-----------------------------------------------------------*/
/* Calculate a unit vector3 perpendicular to arg1 and in the */
/* plane defined by arg_1 and arg_2                          */
/*-----------------------------------------------------------*/

_PUBLIC vector3 v3planp(const vector3 *arg1, const vector3 *arg2)

{   vector3 ret;

    FTYPE dp_1,
          dp_2,
          ratio;

    dp_1 = v3dot(arg1,arg1);
    dp_2 = v3dot(arg1,arg2);

    ratio = -dp_2/dp_1;
    ret = v3scalm(ratio,arg1);
    ret = v3add(&ret,arg2);
    ret = v3unit(&ret);

    return(ret);
}




/*--------------------------*/
/* Read vector3 from stream */
/*--------------------------*/

_PUBLIC int32_t v3read(char       *vname,
                   vector3    *arg,
                   const FILE *stream)
                  
{   uint32_t i;
    char     line[SSIZE]    = "";

    (void)fgets(line,SSIZE,stream);


    /*------------------------*/
    /* Name must be a comment */
    /* # <vector name>        */
    /*------------------------*/

    if(line[0] != '#')
       return(-1);

    for(i=0; i<3; ++i)
    {  if(fscanf(stdin,"%F",&arg->comp[i]) != 1)
           return(-1);
    }

    return(0);
}




/*----------------------------*/
/* Divide vector3 by a scalar */
/*----------------------------*/

_PUBLIC vector3 v3scald(const FTYPE scalar, const vector3 *arg)

{   uint32_t i;
    vector3      ret;

    for(i=0; i<3; ++i)
        ret.comp[i] = arg->comp[i] / scalar;

    return(ret);
}




/*------------------------------*/
/* Multiply vector3 by a scalar */
/*------------------------------*/

_PUBLIC vector3 v3scalm(const FTYPE scalar, const vector3 *arg)

{   uint32_t i;
    vector3      ret;

    for(i=0; i<3; ++i)
       ret.comp[i] = arg->comp[i] * scalar;

    return(ret);
}




/*---------------------------------------*/
/* Get sign vector3 of argument  vector3 */
/*---------------------------------------*/

_PUBLIC vector3 v3sign(const vector3 *arg)

{  uint32_t i;
   vector3      ret;

   for(i=0; i<3; ++i)
      ret.comp[i] = (FTYPE)isign(arg->comp[i]);

   return(ret);
}



/*-------------------------------*/
/* Subtract vector3 from vector3 */
/*-------------------------------*/

_PUBLIC vector3 v3sub(const vector3 *arg1, const vector3 *arg2)

{   uint32_t i;
    vector3      ret;


    for(i=0; i<3; ++i)
       ret.comp[i] = arg1->comp[i] - arg2->comp[i];

    return(ret);
}





/*------------------------------------------------------------*/
/* Find the unit vector3 in the direction of argument vector3 */
/*------------------------------------------------------------*/

_PUBLIC vector3 v3unit(const vector3 *arg)

{   uint32_t i;
    FTYPE    magnitude = 0.0;

    vector3 ret;


    /*---------------------------------------*/
    /* Calculate the magnitude of the vector */
    /*---------------------------------------*/

    magnitude = 0.0;
    for(i=0; i<3; ++i)
        magnitude += sqr(arg->comp[i]);


    /*--------------------------------------------------*/
    /* Now find the three components of the unit vector */
    /*--------------------------------------------------*/

    for(i=0; i<3; ++i)
        if(magnitude == 0.0)
           ret.comp[i] = 0.0;
        else
           ret.comp[i] = arg->comp[i]/SQRT(magnitude);

    return(ret);
}




/*---------------------------*/
/* Write a vector3 to stream */
/*---------------------------*/

_PUBLIC void v3write(const char    *vname,
                     const vector3 *arg,
                     const FILE    *stream)

{   uint32_t i;

    (void)fprintf(stream,"# %s\n",vname);

    for(i=0; i<3; ++i)
       (void)fprintf(stream,"%E ",arg->comp[i]);

    (void)fprintf(stream,"\n");
    (void)fflush(stream);
}




/*-------------------------------------------*/
/* Rotate vector3 theta degrees about X axis */
/*-------------------------------------------*/

_PUBLIC vector3 v3rotx(const vector3 *arg, const FTYPE theta)

{   vector3 ret;

    FTYPE cos_theta,
          sin_theta;

    sin_theta  = SIN(theta);
    cos_theta  = COS(theta);

    ret.comp[0] = arg->comp[0];
    ret.comp[1] = arg->comp[1]*cos_theta - arg->comp[2]*sin_theta;
    ret.comp[2] = arg->comp[1]*sin_theta + arg->comp[2]*cos_theta;

    return(ret);
}





/*----------------------------------------------*/
/* Rotate vector3 by theta degrees about Y axis */
/*----------------------------------------------*/

_PUBLIC vector3 v3roty(const vector3 *arg, const FTYPE theta)

{   vector3 ret;

    FTYPE cos_theta,
          sin_theta;

    sin_theta  = SIN(theta);
    cos_theta  = COS(theta);

    ret.comp[0] = arg->comp[0]*cos_theta  + arg->comp[2]*sin_theta;
    ret.comp[1] = arg->comp[1];
    ret.comp[2] = -arg->comp[0]*sin_theta + arg->comp[2]*cos_theta;

    return(ret);
}




/*----------------------------------------------*/
/* Rotate vector3 by theta degrees about Z axis */
/*----------------------------------------------*/

_PUBLIC vector3 v3rotz(const vector3 *arg, const FTYPE theta)

{   vector3 ret;

    FTYPE cos_theta,
          sin_theta;

    sin_theta  = SIN(theta);
    cos_theta  = COS(theta);

    ret.comp[0] = arg->comp[0]*cos_theta - arg->comp[1]*sin_theta;
    ret.comp[1] = arg->comp[0]*sin_theta + arg->comp[1]*cos_theta;
    ret.comp[2] = arg->comp[2];

    return(ret);
}


/*################################################*/
/* Euler matrices are only defined for 3D vectors */
/*################################################*/
#ifdef THREE_SPACE
/*---------------------------------------------------*/
/* Euler rotation matrix. Multiply vector3 by this   */
/* matrix to rotate it about arbitrary axis          */
/*                                                   */
/* Based on the Euler Matrix derivation given in     */
/* "The Algebra of Matrices" by E.H. Thompson,       */
/* Edn. 1, 1969, pp.150-151,                         */
/* Published by Hilger.                              */
/*---------------------------------------------------*/

_PUBLIC matrix3 m3euler(const FTYPE   theta,   // Angle of rotation 
                           const vector3 *r_axis) // Rotation axis 

{   FTYPE sin_theta,
          cos_theta;

    matrix3 Euler_mat;

    sin_theta  = SIN(theta);
    cos_theta  = COS(theta);


    /*---------------------------------------------*/
    /* Check that r_axis is in fact a unit vector3 */
    /*---------------------------------------------*/

    if(v3mag(r_axis) - ASSUMED_ZERO > 1.0)
       pups_error("[m3euler] rotation axis is not a unit vector3");


    /*-----------------------------*/
    /* X elements of Euler matrix3 */
    /*-----------------------------*/

    Euler_mat.comp[0][0] = sqr(r_axis->comp[0]) + (1 - sqr(r_axis->comp[0]))*cos_theta;
    Euler_mat.comp[0][1] = r_axis->comp[0]*r_axis->comp[1]*(1 - cos_theta) - r_axis->comp[2]*sin_theta;
    Euler_mat.comp[0][2] = r_axis->comp[0]*r_axis->comp[2]*(1 - cos_theta) + r_axis->comp[1]*sin_theta;


    /*----------------------------*/
    /* Y elements of Euler Matrix */
    /*----------------------------*/

    Euler_mat.comp[1][0] = r_axis->comp[1]*r_axis->comp[0]*(1 - cos_theta) + r_axis->comp[2]*sin_theta;
    Euler_mat.comp[1][1] = sqr(r_axis->comp[1]) + (1 - sqr(r_axis->comp[1]))*cos_theta;
    Euler_mat.comp[1][2] = r_axis->comp[1]*r_axis->comp[2]*(1 - cos_theta) - r_axis->comp[0]*sin_theta;


    /*----------------------------*/
    /* Z elements of Euler Matrix */ 
    /*----------------------------*/

    Euler_mat.comp[2][0] = r_axis->comp[2]*r_axis->comp[0]*(1 - cos_theta) - r_axis->comp[1]*sin_theta;
    Euler_mat.comp[2][1] = r_axis->comp[2]*r_axis->comp[1]*(1 - cos_theta) + r_axis->comp[0]*sin_theta;
    Euler_mat.comp[2][2] = sqr(r_axis->comp[2]) + (1 - sqr(r_axis->comp[2]))*cos_theta;

    return(Euler_mat);
}




/*----------------------------------------------------------------*/
/* Euler matrix3 for small angle rotations [See E.H Thompson, "An */
/* introduction to the algebra of matrices"]                      */
/*----------------------------------------------------------------*/

_PUBLIC matrix3 m3seuler(FTYPE r_theta, vector3 *r_axis)

{   matrix3 s_Euler_mat;


    /*---------------------------------------------*/
    /* Check that r_axis is in fact a unit vector3 */
    /*---------------------------------------------*/

    if(v3mag(r_axis) - ASSUMED_ZERO > 1.0)
       pups_error("[m3seuler] rotation axis is not a unit vector3");

    s_Euler_mat.comp[0][0] =  1.0;
    s_Euler_mat.comp[0][1] = -r_axis->comp[2]*r_theta;
    s_Euler_mat.comp[0][2] =  r_axis->comp[1]*r_theta;

    s_Euler_mat.comp[0][0] =  r_axis->comp[2]*r_theta;
    s_Euler_mat.comp[0][1] =  1.0;
    s_Euler_mat.comp[0][2] = -r_axis->comp[0]*r_theta;

    s_Euler_mat.comp[0][0] = -r_axis->comp[1]*r_theta;
    s_Euler_mat.comp[0][1] =  r_axis->comp[0]*r_theta;
    s_Euler_mat.comp[0][2] =  1.0;

    return(s_Euler_mat);
}
#endif /* THREE_SPACE */



/*-----------------------------------------*/
/* Routine to set up a random unit vector3 */
/*-----------------------------------------*/

_PUBLIC vector3 v3urnd(void)

{   uint32_t i;
    vector3      ret;

    for(i=0; i<3; ++i)
        ret.comp[i] = ran1();

    ret = v3unit(&ret);
    return(ret);
}




/*-------------------------------------------------------*/
/* Set up a randomly pointing vector3 of random size     */
/* max_cs controls the maximum size of any one component */
/*-------------------------------------------------------*/

_PUBLIC vector3 v3rnd(const FTYPE max_cs)

{   uint32_t i;
    vector3      ret;

    for(i=0; i<3; ++i)
        ret.comp[i] = (ran1() - 0.5)*max_cs;

    return(ret);
}




/*--------------------------------------------------*/
/* Compute component sign hetrogeneity of a vector3 */
/*--------------------------------------------------*/

_PUBLIC _BOOLEAN v3hecs(const vector3 *arg)

{   uint32_t i,
             sign_sum = 0;

   for(i=0; i<3; ++i)
       sign_sum += fsign(arg->comp[i]);

   if(fabs((FTYPE)sign_sum) < 3)
      return(TRUE);
   else
      return(FALSE);
}



/*-------------------------*/
/* Write matrix3 to stream */
/*-------------------------*/

_PUBLIC void m3write(const FILE *stream, const char *mname, const matrix3 *arg)

{   uint32_t i,
             j;

    (void)fprintf(stream,"# %s\n\n",mname);
    (void)fflush(stream);

    for(i=0; i<3; ++i)
    {  for(j=0; j<3; ++j)
          (void)fprintf(stream,"[%d,%d]: %E\n",i,j, arg->comp[i][j]);
       (void)fflush(stream);
    }

    (void)fprintf(stream,"\n\n");
    (void)fflush(stream);
}




/*---------------------------------------------------------------*/
/* Find the solution of a set of linear equations using Gaussian */
/* elimination. These routines are based on thosein Pascal in:   */
/*                                                               */
/* "Pascal for Science and Engineering" by McGregor and Watt     */
/* --------------------------------------------------------------*/

_PRIVATE _BOOLEAN eliminate(int32_t *singular, matrix3 *a, vector3 *b)

{   uint32_t i,
             j,
             k;

    FTYPE multiplier;

    i = 0;
    do {   if(reorder(i,a,b) == TRUE)
           {  if(*singular == EXIT_IF_SINGULAR)
                 pups_error("[eliminate] equations singular");
              else
              {  *singular = IS_SINGULAR;
                 return(TRUE);
              }
           }
           else
           {  for(k=i+1; k<3; ++k)
              {   multiplier = a->comp[k][i] / a->comp[i][i];
                  for(j=i+1; j<3; ++j)
                     a->comp[k][j] -= multiplier*a->comp[i][j];
                  b->comp[k] -= multiplier*b->comp[i];
                  a->comp[k][i] = 0.0;
              }
           }

           ++i;
        } while(i != 3);

     if((fabs(a->comp[2][2]) <= ASSUMED_ZERO) == TRUE)
     {   if(*singular == EXIT_IF_SINGULAR)
            pups_error("[eliminate] equations singular");
        else
        {   *singular = IS_SINGULAR;
            return(TRUE);
        }
     }

     *singular = NOT_SINGULAR;
     return(FALSE);
}




/*------------------------------------------*/
/* Reorder routine for Gaussian Elimination */
/*------------------------------------------*/

_PRIVATE _BOOLEAN reorder(uint32_t i, matrix3 *a, vector3 *b)

{   uint32_t k,
             l,
             j;

    FTYPE    temp;

    l = i;
    for(k=i+1; k<3; ++k)
       if(fabs(a->comp[k][i]) > fabs(a->comp[l][i])) l = k;

    if((temp = fabs(a->comp[l][i])) < ASSUMED_ZERO)
       return(TRUE);
    else
    {  if(i != l)
       {  for(j=i; j<3; ++j)
             fswap(&a->comp[i][j],&a->comp[l][j]);
          fswap(&b->comp[i],&b->comp[l]);
       }
    }

    return(FALSE);
}




/*---------------------------------------------------*/
/* Backsubstitution routine for Gaussian Elimination */
/*---------------------------------------------------*/

_PRIVATE void backsub(const matrix3 *a, const vector3 *b, vector3 *x)

{    uint32_t i,
              j;

     FTYPE    s;

     for(i=2; i >= 0; --i) // MAO was >(-1)
     {  s = b->comp[i];
        for(j=i+1; j<3; ++j)
           s -= a->comp[i][j]*x->comp[j];
        x->comp[i] =  s / a->comp[i][i];
     }
}




/*-------------------------------------------------------------*/
/* Solve 3x3 simultaneous equations using Gaussian Elimination */
/*-------------------------------------------------------------*/

_PUBLIC _BOOLEAN m3GE_solve(uint32_t *singular, const matrix3 *a, const vector3 *b, vector3 *x)

{    if(eliminate(singular,a,b) == TRUE)
        return(TRUE);

     backsub(a,b,x);
     return(FALSE);
}




/*---------------------------------------------------------*/
/* Get scalar distance between a point in 3 space and line */
/*---------------------------------------------------------*/

_PUBLIC FTYPE v3dpoints(const vector3 *pt, const vector3 *to_line, const vector3 *along_line)

{   vector3 ret_v;

    ret_v = v3dpointv(pt,to_line,along_line);
    return(v3mag(&ret_v));    
}




/*---------------------------------------------------*/
/* Calculate smallest vector3 between line and point */
/*---------------------------------------------------*/

_PUBLIC vector3 v3dpointv(const vector3 *pt, const vector3 *to_line, const vector3 *along_line)

{   FTYPE theta,
           thi,
           to_pt_mag;

    vector3 p_vec,
            p_u_vec,
            z_vec,
            to_pt_u_vec;


    /*----------------------------------------------------*/
    /* Check that the vector3 long_line is a unit vector3 */
    /*----------------------------------------------------*/

    if(v3mag(along_line) - 1.0 > 0.001)
    {  (void)fprintf(stderr,"   magnitude is:%E\n",v3mag(along_line));
       (void)fflush(stderr);
       pups_error("[vdpointv] vector3 along_line must be a unit vector3");
    }


    /*-----------------------------*/
    /* Set up point plane vector3s */
    /*-----------------------------*/

    p_vec       = v3sub(pt,to_line);
    p_u_vec     = v3unit(&p_vec);
    z_vec       = v3cross(&p_u_vec,along_line);
    z_vec       = v3unit(&z_vec);
    to_pt_u_vec = v3cross(&z_vec,along_line);


    /*---------------------------*/
    /* Calculate principle angle */
    /*---------------------------*/

    thi       = v3ang(&p_u_vec,along_line);
    theta     = FABS(PI / 2.0 - thi);
    to_pt_mag = v3mag(&p_vec)*cos(theta);

    return(v3scalm(to_pt_mag,&to_pt_u_vec));
}



/*--------------------------------------------------*/
/* Calculate smallest vector3 between two skew line */
/*--------------------------------------------------*/

_PUBLIC vector3 v3dlinesv(uint32_t       *singular,    // Singular
                          vector3         *divergence,  // Ray-ray divergence vector3
                          const vector3   *to_1,        // Vector to line 1
                          const vector3   *along_1,     // Vector along line 1
                          const vector3   *to_2,        // Vector to line 2
                          const vector3   *along_2,     // Vector along line 2
                          vector3         *intw_1,      // Skew (intercept) vector3 for line 1
                          vector3         *intw_2)      // Skew (intercept) vector3 for line 2

{   matrix3 a;

    vector3 r,
            b,
            x,
            vtemp_1,
            vtemp_2,
            ret;


    /*---------------------------*/
    /* First find the tie vector */
    /*---------------------------*/

    r = v3cross(along_1,along_2);


    /*----------------------------------------------*/
    /* Build the b vector3 for Gaussian elimination */
    /*----------------------------------------------*/

    b = v3sub(to_1,to_2);


    /*---------------*/
    /* Build matrix3 */
    /*---------------*/

    vtemp_1 = v3inv(along_1);
    vtemp_2 = v3inv(&r);

    v3tomatc(0,&a,along_2);
    v3tomatc(1,&a,&vtemp_1);
    v3tomatc(2,&a,&vtemp_2);


    /*-----------------------------------------------------------------------*/
    /* Solve for lambda, and nu. The space intersect point is then given by: */
    /* s_int = to_1 + lambda*along_1 + (nu / 2.0)*r                          */
    /*-----------------------------------------------------------------------*/

    if(m3GE_solve(singular,&a,&b,&x) == TRUE)
    {  *singular = IS_SINGULAR;
       ret.comp[0] = ret.comp[1] = ret.comp[2] = 0.0;

       return(ret);
    }

    vtemp_1 = v3scalm(x.comp[1],along_1);
    vtemp_2 = v3scalm(x.comp[2]/2.0,&r);
    ret     = v3add(&vtemp_1,&vtemp_2);
    ret     = v3add(to_1,&ret);


    /*-----------------------------*/
    /* Find the vector3 divergence */
    /*-----------------------------*/

    vtemp_1 = v3scalm(x.comp[1],along_1);
    *intw_1 = v3add(to_1,&vtemp_1);

    vtemp_2 = v3scalm(x.comp[0],along_2);
    *intw_2 = v3add(to_2,&vtemp_2);

    vtemp_1 = v3sub(intw_1,intw_2);
    *divergence = vtemp_1;

    return(ret);
}



/*------------------------------------------------------------*/
/* Compute mean nearest approach vector3 of set of skew lines */
/*------------------------------------------------------------*/

_PUBLIC vector3 v3mlinesv(uint32_t     *singular,  // Action if singular
                         int                    n_rays,  // Number of rays to consider
                         const vector3          *pc,  // Array of to_line vector3s
                         const vector3         *ray,  // Array of along_line vector
                         FTYPE             *divergence)  // Mean divergence 
{   uint32_t i,
             j,
             n_vecs = 0;

    vector3 intw_1,
            intw_2,
            skew_v,
            int_v,
            mean_int_v,
            mean_skew_v;

    if(n_rays == 1)
       pups_error("[vmlinesv] need at least two rays to compute intersect");


    /*----------------------------------*/
    /* Consider all possible intersects */
    /*----------------------------------*/

    v3ass(&mean_int_v, 0.0,0.0,0.0);
    v3ass(&mean_skew_v,0.0,0.0,0.0);

    for(i=0; i<n_rays; ++i)
    {   for(j=i;  j<n_rays; ++j)
        {   if(i != j)
            {  int_v = v3dlinesv(singular,
                                 &skew_v,
                                 &pc[i],
                                 &ray[i],
                                 &pc[j],
                                 &ray[j],
                                 &intw_1,
                                 &intw_2);

               if(*singular == IS_SINGULAR)
               {  mean_int_v.comp[0] = mean_int_v.comp[1] = mean_int_v.comp[2] = 0.0;
                  return(mean_int_v);
               }

               mean_int_v  = v3add(&mean_int_v,&int_v);
               mean_skew_v = v3add(&mean_skew_v,&skew_v);
               ++n_vecs;
            }
        }
    }


    /*------------------------------------------------------------*/
    /* Mean scalar distance between rays involved in intersection */
    /* and the intersection point (note this is an RMS measure)   */
    /*------------------------------------------------------------*/

    mean_int_v = v3scald((FTYPE)n_vecs,&mean_int_v);

    *divergence = 0.0; 
    for(i=0; i<n_rays; ++i)
       *divergence += FABS(v3dpoints(&mean_int_v,&pc[i],&ray[i]));

    *divergence = *divergence / (FTYPE)n_rays;
    return(mean_int_v);
}





/*--------------------------------*/
/* Copy vector3 to matrix3 column */
/*--------------------------------*/

_PUBLIC void v3tomatc(const uint32_t row, matrix3 *arg_m, const vector3 *arg_v)

{   uint32_t i;


    /*-------------------------------------------------------------------------*/
    /* If the vector3 is not of the same dimensionality as the matrix3 column, */
    /* print a suitable error message and exit                                 */
    /*-------------------------------------------------------------------------*/

    for(i=0; i<3; ++i)
       arg_m->comp[i][row] = arg_v->comp[i];
}




/*--------------------------------------------------------------------*/
/* Compute planar distortion to vector3 caused by rotation projection */
/*--------------------------------------------------------------------*/

_PUBLIC vector3 v3distort(const vector3 *arg, const FTYPE theta, const FTYPE thi)

{    vector3 ret;


     /*----------------------------------------------------------*/
     /* Apply distortion to X component caused by rotation theta */
     /*----------------------------------------------------------*/

     ret.comp[0] = arg->comp[0] / cos(theta * DEGRAD);


     /*----------------------------------------------------------*/
     /* Apply distortion to Y component caused by rotation thi   */
     /*----------------------------------------------------------*/

     ret.comp[1] = arg->comp[1] / cos(thi * DEGRAD);


     /*----------------------------------------------------------*/
     /* Copy undistorted component of vector3                     */
     /*----------------------------------------------------------*/

     ret.comp[2] = arg->comp[2];

     return(ret);
}



/*---------------------*/
/* Assign zero matrix3 */
/*---------------------*/

_PUBLIC void m3zero(matrix3 *arg)

{   uint32_t i,
             j;

    for(i=0; i<3; ++i)
       for(j=0; j<3; ++j)
          arg->comp[i][j] = 0.0;
}




/*-------------------------*/
/* Assign identity matrix3 */
/*-------------------------*/

_PUBLIC void m3ident(matrix3 *arg)

{   uint32_t i;

    m3zero(arg);
    for(i=0; i<3; ++i)
       arg->comp[i][i] = 1.0;
}



/*----------------*/
/* Assign matrix3 */
/*----------------*/

_PUBLIC void m3ass(matrix3       *arg,
                   const FTYPE    c11,
                   const FTYPE    c12,
                   const FTYPE    c13,
                   const FTYPE    c21,
                   const FTYPE    c22,
                   const FTYPE    c23,
                   const FTYPE    c31,
                   const FTYPE    c32,
                   const FTYPE    c33)

{   arg->comp[0][0] = c11;
    arg->comp[1][0] = c12;
    arg->comp[2][0] = c13;
    arg->comp[0][1] = c21;
    arg->comp[1][1] = c22;
    arg->comp[2][1] = c23;
    arg->comp[0][2] = c31;
    arg->comp[1][2] = c32;
    arg->comp[2][2] = c33;
}




/*------------------------------*/
/* Multiply a pair of matrix3's */
/*------------------------------*/

_PUBLIC matrix3 m3mult(const matrix3 *arg_1, const matrix3 *arg_2)

{   uint32_t i,
             j,
             k;

    matrix3  ret;

    for(i=0; i<3; ++i)
    {  for(j=0; j<3; ++j)
       {   ret.comp[i][j] = 0.0;
           for(k=0; k<3; ++k)
              ret.comp[i][j] += (arg_1->comp[i][k] * arg_2->comp[k][j]);
       }
    }

    return(ret);
}




/*-----------------------------*/
/* Multiply vector3 by matrix3 */
/*-----------------------------*/

_PUBLIC vector3 v3mmult(const matrix3 *arg_1, const vector3 *arg_2)

{   uint32_t i,
             j;

    vector3 ret;

    for(i=0; i<3; ++i)
    {  ret.comp[i] = 0.0;
       for(j=0; j<3; ++j)
          ret.comp[i] += (arg_1->comp[i][j] * arg_2->comp[j]);
    }

    return(ret);
}




/*----------------------------------------------------*/
/*  Compute the spatial entropy of a set of vector3's */
/*----------------------------------------------------*/

_PUBLIC FTYPE v3lvsi(const int32_t n_vecs, const vector3 vec_arr[])

{   uint32_t i,
             j;

    FTYPE    ret    = 0.0,
             mean[] = {0.0, 0.0, 0.0},
             adev[] = {0.0, 0.0, 0.0};


    /*---------------------------------------------------------------------*/
    /* Form up the means for each component of the set of vector3s in turn */
    /*---------------------------------------------------------------------*/

    for(i=0; i<3; ++i)
    {  for(j=0; j<n_vecs; ++j)
          mean[i] += vec_arr[j].comp[i];
    }

    for(i=0; i<3; ++i)
        mean[i] /= n_vecs;


    /*---------------------------------------------------*/
    /* Now find the average deviation for each component */
    /*---------------------------------------------------*/

    for(i=0; i<3; ++i)
    {  for(j=0; j<n_vecs; ++j)
           adev[i] += FABS(vec_arr[j].comp[i] - mean[i]);
    }


    /*------------------------------------------*/
    /* Now form the total linear distortion sum */
    /*------------------------------------------*/

    for(i=0; i<3; ++i) 
        ret += adev[i];

    return(ret);
}  



/*-------------------------*/
/* Add a pair of matrix3's */
/*-------------------------*/

_PUBLIC matrix3 m3add(const matrix3 *arg_1, const matrix3 *arg_2)

{   uint32_t i,
             j;

    matrix3  ret;

    for(i=0; i<3; ++i)
    {  for(j=0; j<3; ++j)
          ret.comp[i][j] = arg_1->comp[i][j] + arg_2->comp[i][j];
    }

    return(ret);
}




/*-------------------------------*/
/* Subtract matrix3 from matrix3 */ 
/*-------------------------------*/

_PUBLIC matrix3 m3sub(const matrix3 *arg_1, const matrix3 *arg_2)

{   uint32_t i,
             j;

    matrix3 ret;

    for(i=0; i<3; ++i)
    {  for(j=0; j<3; ++j)
          ret.comp[i][j] = arg_1->comp[i][j] - arg_2->comp[i][j];
    }

    return(ret);
}




/*----------------------------*/
/* Multiply matrix3 by scalar */
/*----------------------------*/

_PUBLIC matrix3 m3scalm(const FTYPE arg_1, const matrix3 *arg_2)

{   uint32_t i,
             j;

    matrix3 ret;

    for(i=0; i<3; ++i)
    {  for(j=0; j<3; ++j)
          ret.comp[i][j] = arg_1 * arg_2->comp[i][j];
    }

    return(ret);
}



/*-------------------------------*/
/* Find the transpose of matrix3 */
/*-------------------------------*/

_PUBLIC matrix3 m3ransp(const matrix3 *arg)

{   uint32_t i,
             j;

    FTYPE   temp;
    matrix3 ret;

    for(i=0; i<3; ++i)
    {  for(j=0; j<3; ++j)
          ret.comp[i][j] = arg->comp[i][j];
    }
 
    for(i=0; i<3; ++i)
    {  for(j=0; j<3; ++j)
       {  temp            = ret.comp[i][j];
          ret.comp[i][j]  = ret.comp[j][i];
          ret.comp[j][i] = temp;
       }
    }

    return(ret);
}




/*-------------------------------------------------------------*/
/* Solve a set of N linear equations using LU decompostion,    */
/* this routine is taken from "Numerical Recipes in C" pp43-44 */
/*-------------------------------------------------------------*/

_PRIVATE void lubksb(int32_t indx[], matrix3 *a, vector3 *b)

{   int32_t i,
            ip,
            j,
            ii = (-1);

    FTYPE sum;


    /*------------------------------------------------*/
    /* Solves the set of linear equations A.X = B ... */
    /*------------------------------------------------*/

    for(i=0; i<3; ++i)
    {   ip          = indx[i];
        sum         = b->comp[ip];
        b->comp[ip] = b->comp[i];
        if(ii > -1)
           for(j=ii; j<=i-1; j++)
               sum -= a->comp[i][j]*b->comp[j];
        else
           if(sum != 0.0)
              ii = i;
        b->comp[i] = sum;
    }


    /*---------------------*/
    /* Do backsubstitution */
    /*---------------------*/

    for(i=2; i>=0; i--)
    {   sum = b->comp[i];
        for(j=i+1; j<3; j++)
           sum -= a->comp[i][j]*b->comp[j];


        /*---------------------------------------*/
        /* Store component of solution vector3 X */
        /*---------------------------------------*/

        b->comp[i] = sum / a->comp[i][i];
    }
}




/*-------------------------------------------------*/
/* Routine to do LU decomposition on the matrix3 a */
/*-------------------------------------------------*/

_PRIVATE void ludcmp(int32_t indx[], FTYPE *d, matrix3 *a)

{   uint32_t i,
             imax,
             j,
             k;

    FTYPE    big,
             dum,
             sum,
             temp,
             vv[3];

    *d = 1.0;


    /*----------------------------------------------------*/
    /* Loop over rows to get implicit scaling information */
    /*----------------------------------------------------*/

    for(i=0; i<3; ++i)
    {   big = 0.0;
        for(j=0; j<3; ++j)
           if((temp = fabs(a->comp[i][j])) > big)
              big = temp;
        if(big == 0.0)
           pups_error("[ludcmp] matrix3 singular");


        /*------------------*/
        /* Save the scaling */
        /*------------------*/

        vv[i] = 1.0 / big;
    }


    /*----------------------------------------*/
    /* Loop over columns using Crouts' method */
    /*----------------------------------------*/

    for(j=0; j<3; ++j)
    {   for(i=0; i<j; ++i)
        {   sum = a->comp[i][j];
            for(k=0; k<j; ++k)
                sum -= a->comp[i][k]*a->comp[k][j];
            a->comp[i][j] = sum;
        }


        /*---------------------------------------------*/
        /* Initialise search for largest pivot element */
        /*---------------------------------------------*/

        big = 0.0;
        for(i=j; i<3; ++i)
        {   sum = a->comp[i][j];
            for(k=0; k<j; ++k)
                sum -= a->comp[i][k]*a->comp[k][j];
            a->comp[i][j] = sum;


            /*--------------------------------------------------------------------*/
            /*  Is the figure of merit for the pibvot better than the best so far */
            /*--------------------------------------------------------------------*/

            if((dum = vv[i]*fabs(sum)) >= big)
            {  big  = dum;
               imax = i;
            }
        }


        /*--------------------------*/
        /* Test for row interchange */
        /*--------------------------*/

        if(j != imax)
        {  for(k=0; k<3; ++k)
           {   dum              = a->comp[imax][k];
               a->comp[imax][k] = a->comp[j][k];
               a->comp[j][k]    = dum;
           }


           /*--------------------*/
           /* Change parity of d */
           /*--------------------*/

           *d = -(*d);


           /*-------------------------------*/
           /*  And interchange scale factor */
           /*-------------------------------*/

           vv[imax] = vv[j];
        }

        indx[j] = imax;
        if(a->comp[j][j] == 0.0)
           a->comp[j][j] = TINY;


        /*---------------------------------*/
        /* Finally divide by pivot element */
        /*---------------------------------*/

        if(j != 2)
           dum = 1.0/a->comp[j][j];


        /*------------------------------------------------------------------------*/
        /* If the pivot element is zero, the matrix3 is singular (at least to the */
        /* precision of this algorithm). For some applications on singular        */
        /* matrices it is edesirable to substitute TINY for zero                  */
        /*------------------------------------------------------------------------*/

        for(i=j+1; i<3; ++i)
            a->comp[i][j] *= dum;
    }
}  




/*-----------------------*/
/* Is matrix3 symmetric? */
/*-----------------------*/

_PUBLIC _BOOLEAN m3sym(const matrix3 *arg)

{   uint32_t i,
             j;

    for(i=0; i<3; ++i)
    {  for(j=i; j<3; ++j)
       {  if(i != j && arg->comp[i][j] != arg->comp[j][i])
             return(FALSE);
       }
    }

    return(TRUE);
}


/*##########################################################*/
/* Determinant computation is only defined for 3x3 matrices */
/*##########################################################*/
#ifdef THREE_SPACE
/*--------------------------------*/
/* Compute determinant of matrix3 */
/*--------------------------------*/

_PUBLIC FTYPE m3det(const matrix3 *arg)

{   uint32_t i;
    FTYPE    det;

    for(i=0; i<3; ++i) 
    {   det += (arg->comp[0][i] * (arg->comp[1][(i + 1) % 3] * arg->comp[2][(i + 2) % 3] -
                                   arg->comp[1][(i + 2) % 3] * arg->comp[2][(i + 1) % 3]));
    }

    return(det);
}
#endif /* THREE_SPACE */




/*-----------------------------------------*/
/* Invert a matrix3 using LU decomposition */
/*-----------------------------------------*/

_PUBLIC matrix3 m3inv(matrix3 *arg)

{   uint32_t i,
             j,
             *indx = (int32_t *)NULL;

    FTYPE   d;
    vector3 col;


    /*---------------------------------------------------------------------*/
    /* Decompose the matrix3 'arg' using LU decomposition. This routine is */
    /* taken from "Numerical Recipes in C", pp43-44                        */
    /* --------------------------------------------------------------------*/

    indx = (int32_t *)pups_calloc(3,sizeof(int32_t));
    ludcmp(indx,&d,arg);


    /*--------------------------------------------------------------------*/
    /* Use the results of the LU decomposition to find the inverse of the */
    /* matrix3 by columns                                                 */
    /*--------------------------------------------------------------------*/

    for(j=0; j<3; ++j)
    {  for(i=0; i<3; ++i)
           col.comp[i] = 0.0;

       col.comp[j] = 1.0;


       /*---------------------------------------------*/
       /* Solve the resulting set of linear equations */
       /*---------------------------------------------*/

       lubksb(indx,arg,&col);
       for(i=0; i<3; ++i)
          arg->comp[i][j] = col.comp[i];
    }

    return(*arg);
}
