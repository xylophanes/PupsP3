/*------------------------------------------------------------------------------
    Purpose: Numerical function support for scientific programming. This
             version contains fixes for the bugs discovered by Mike Cook
             of Laserscan Laboratories [mikec@uk.co.lsl]

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.00
    Dated:   4th January 2022
    E-Mail:  mao@tumblingdice.co.uk 
------------------------------------------------------------------------------*/

#include <me.h>
#include <utils.h>
#include <nfo.h>
#include <unistd.h>
#include <stdlib.h>


/*------------------------------------------------------------------------------
    Slot and usage functions - used by slot manager ...
------------------------------------------------------------------------------*/


/*---------------------*/
/* Slot usage function */
/*---------------------*/

_PRIVATE void nfolib_slot(int level)
{   (void)fprintf(stderr,"int lib nfolib %s: [ANSI C]\n",NFO_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(C) 1985-2022 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"PUPS/P3 numerical function library (built %s %s)\n\n",__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");
    (void)fflush(stderr);
}



/*-------------------------------------------------------*/
/* Segment identification for numerical function library */
/*-------------------------------------------------------*/

#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT )() __attribute__ ((aligned(16))) = nfolib_slot;
#endif /* SLOT */




/*-----------------------------------------------------------------*/
/* Do not have _EXTERN defined for the header file of this library */
/*-----------------------------------------------------------------*/

#undef   __NOT_LIB_SOURCE__
#include <nfo.h>
#define  __NOT_LIB_SOURCE__

#include <casino.h>
#include <utils.h>


/*-----------------------------------------*/
/* Constants/variables exported by library */
/*-----------------------------------------*/

_PUBLIC _CONST FTYPE NATURAL = 1.0e30;      // Natural spline boundries


/*-----------------------------------------------------*.
/* Public variable sqrarg which is used by some macros */
/*-----------------------------------------------------*/

_PUBLIC FTYPE sqrarg;


/*--------------------------------------------------------*/
/* Standard definitions required by minimisation routines */
/*--------------------------------------------------------*/

_PRIVATE _CONST FTYPE GOLD   = 1.618034;
_PRIVATE _CONST FTYPE GLIMIT = 100.0;


/*---------------*/
/* Golden Ratios */
/*---------------*/

_PRIVATE _CONST FTYPE R      = 0.61803399;
_PRIVATE _CONST FTYPE C      = 0.38196601;  
_PRIVATE _CONST FTYPE CGOLD  = 0.3819660;
_PRIVATE _CONST FTYPE ZEPS   = 1.0e-10;


/*-----------------------*/
/* Minimum cut off value */
/*-----------------------*/

_PRIVATE _CONST FTYPE TINY  = 1.0e-20;


/*------------------------------------*/
/* Maximum iterations Brent Minimiser */
/*------------------------------------*/

_PRIVATE _CONST int BR_ITMAX = 100;


/*-------------------------------------*/
/* Maximum iterations Powell Minimiser */
/*-------------------------------------*/

#define PO_ITMAX   200
#define PASS_ITMAX 10


/*--------------------------------*/
/* Tolerance for Powell Minimiser */
/*--------------------------------*/

_PRIVATE _CONST FTYPE TOL     = 2.0e-4;




/*-----------------------------------------------------------------------------
    Static functions which are local to this library ...
-----------------------------------------------------------------------------*/


/*----------------------------------------*/
/* Linear minimisation routine for Powell */
/*----------------------------------------*/

_PROTOTYPE _PRIVATE void linmin  (FTYPE [],       \
                                  FTYPE [],       \
                                  int,            \
                                  FTYPE *,        \
                                  FTYPE (* )());


/*--------------------------------------------*/
/* Support routines for least squares fitting */
/*--------------------------------------------*/

_PROTOTYPE _PRIVATE FTYPE xtran(FTYPE, int);
_PROTOTYPE _PRIVATE FTYPE ytran(FTYPE, int);
_PROTOTYPE _PRIVATE FTYPE atran(FTYPE, int); 


/*----------------------------*/
/* Support routine for linmin */
/*----------------------------*/

_PROTOTYPE _PRIVATE FTYPE f1dim(FTYPE);



/*------------------------------------------------------------------------------
    Standard macros used by nfolib ...
------------------------------------------------------------------------------*/

_EXTERN FTYPE sqrarg;

#ifdef MAX
#undef MAX
#endif /* MAX */

#ifdef MIN
#undef MIN
#endif /* MIN */

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define SIGN(a,b) ((b) > 0.0 ? FABS(a) : -FABS(a))
#define SHFT(a,b,c,d) (a)=(b);(b)=(c);(c)=(d);
#define SQR(a) (sqrarg = (a), sqrarg*sqrarg)




/*-----------------------------------------------------------------------------
    Least squares regression package.

    Linear, Inverse, Log, Power and Exponential least squares fits.
    See Micro User, Educational suppliment for further details.
-----------------------------------------------------------------------------*/

_PUBLIC void least_squares(int            fit,  // Type of fit required 
                           int          n_pts,  // Number of data points 
                           FTYPE      x_arr[],  // Array of X data 
                           FTYPE     y_arr[],   // Array of Y data
                           FTYPE         *r1,   // Goodness of fit
                           FTYPE         *r2,   // Coeff of determination
                           FTYPE lsq_coeff[])   // Least squares coeffs

{   int i,
        numpts = 0;

    FTYPE divisor = 0.0,
          a       = 0.0,
          x       = 0.0,
          y       = 0.0,
          xsum    = 0.0,
          ysum    = 0.0,
          xysum   = 0.0,
          x2sum   = 0.0,
          y2sum   = 0.0;

   for(i=0; i<n_pts; ++i)
   {   x      = xtran(x_arr[i],fit);
       y      = ytran(y_arr[i],fit);
       xsum   = xsum + x;
       ysum   = ysum + y;
       x2sum  = x2sum + x * x;
       y2sum  = y2sum + y * y;
       xysum  = xysum + x * y;
       numpts = numpts + 1;
   }

   divisor = x2sum * numpts - xsum * xsum;
   if(divisor == 0)
      pups_error("[least_squares] regression not possible");
   else
   {  a = (ysum * x2sum - xsum * xysum) / divisor;
      lsq_coeff[0] = atran(a,fit);
      lsq_coeff[1] = (numpts * xysum - xsum * ysum) / divisor;


         /*--------------------------------------*/
         /* Calculate coefficient of correlation */
         /*--------------------------------------*/

         *r2 = a * ysum + lsq_coeff[0] * xysum - 1 / numpts * ysum * ysum;
         *r2 = (*r2) / (y2sum - 1 / numpts * ysum * ysum);
         *r1 = (*r2) / (*r2) * sqr(FABS(*r2));
    }
}





/*------------------------------------------------------------------------------
    Transform X co-ordinate ...
------------------------------------------------------------------------------*/

_PRIVATE FTYPE xtran(FTYPE x, int fit)

{  if(fit == LOGFIT || fit == EXPNFIT)
      return(LOG(x));
   else
      return(x);
}





/*-----------------------------------------------------------------------------
    Transform Y co-ordinate ...
-----------------------------------------------------------------------------*/

_PRIVATE FTYPE ytran(FTYPE y, int fit)

{  if(fit == PWRFIT || fit == EXPNFIT)
      return(LOG(y));

   if(fit == INVSFIT)
      return(1.0 / y);
   else
      return(y);
}






/*------------------------------------------------------------------------------
    Transform a ...
------------------------------------------------------------------------------*/

_PRIVATE FTYPE atran(FTYPE a, int fit)

{  if(fit == PWRFIT || fit == EXPNFIT)
      return(LOG(a));
   else
      return(a);
}





/*------------------------------------------------------------------------------
    Routine to generate a cubic  interpolation table ...
------------------------------------------------------------------------------*/

_PUBLIC void spline(FTYPE  x[],    // Array of X values
                    FTYPE  y[],    // Array of Y values
                    int      n,    // Number of points
                    FTYPE  yp1,    // Boundary condition 1
                    FTYPE  ypn,    // Boundary condition 2
                    FTYPE  y2[])   // Array of Y derivatives dy/dx

{   int i,
        k;

    FTYPE p,
          qn,
          sig,
          un,
          *u  = (FTYPE *)NULL;

    /*----------------------------------------------------------------------*/
    /* Given the arrays x[1..n] and y[1..n] containing a tabulated function */
    /* i.e. yi = f(xi) with x1 < x2 < .. < xn, and given values yp1 and ypn */
    /* for the first derivative of the interpolating function at points     */
    /* 1 and n respectively, this routine returns an array y2[1..n] that    */
    /* contains the second derivatives of the interpolating function at     */
    /* the tabulated points xi. If y1 and/or ypn are equal to 1e30 or       */
    /* greater, the routine is signalled to set the corresponding           */
    /* boundary conditions for a natural spline, with zero second           */
    /* derivative on that boundry                                           */
    /* ---------------------------------------------------------------------*/

    u = (FTYPE *)pups_malloc(n*sizeof(FTYPE));


    /*-------------------------------------*/
    /* Set boundry condition to be natural */
    /*-------------------------------------*/

    if(yp1 > (FTYPE)0.99e30)
       y2[0] = u[0] = (FTYPE)0.0;
    else


    /*------------------------------------------------------------*/
    /* Set boundry condition to have a specified first derivative */
    /*------------------------------------------------------------*/

    {
        y2[0] = (FTYPE)(-0.5);
        u[0] = (3.0/(x[1] - x[0]))*((y[1] - y[0])/(x[1] - x[0]) - yp1);
    }


    /*-------------------------------------------------------------*/
    /* This is the decomposition loop of the tridiagonal algorithm */
    /*-------------------------------------------------------------*/

    for(i=1; i < n - 1; i++)  // MAO  Was <= in original version
    {   sig = (x[i] - x[i-1]) / (x[i+1] - x[i-1]);
        p   = sig*y2[i-1] + (FTYPE)2.0;


       /*-----------------------------------------------------------*/
       /* y2 and u are used for temporary storage of the decomposed */
       /* factors                                                   */
       /*-----------------------------------------------------------*/


       y2[i] = (sig - (FTYPE)1.0) / p;
       u[i]  = (y[i+1] - y[i]) / (x[i+1] - x[i]) -
                                 (y[i] - y[i-1]) / (x[i] - x[i-1]);
       u[i]  = ((FTYPE)6.0*u[i] / (x[i+1] - x[i-1]) - sig*u[i-1]) / p;
    }


    /*------------------------------------------*/
    /* Set upper boundry conditon to be natural */
    /*------------------------------------------*/

    if(ypn > (FTYPE)0.99e30)
        qn = un = (FTYPE)0.0;
    else


    /*-------------------------------------------------------------*/
    /* Set upper boundry condition to a specified first derivative */
    /*-------------------------------------------------------------*/

    {  qn = (FTYPE)0.5;
       un = ((FTYPE)3.0 / (x[n-1] - x[n-2]))*(ypn - (y[n-1] - y[n-2]) /
                                                     (x[n-1] - x[n-2]));
    }
    y2[n-1] = (un - qn*u[n-2]) / (qn*y2[n-2] + (FTYPE)1.0);


    /*------------------------------------------------------*/
    /* This is the backsubstitution loop of the tridiagonal */
    /* algorithm                                            */
    /*------------------------------------------------------*/

    for(k = n - 2; k >= 0; k--)
        y2[k] = y2[k]*y2[k+1] + u[k];

    free((char *)u);
}





/*------------------------------------------------------------------------------
    Routine to generate fitted function from least squares parameters ...
------------------------------------------------------------------------------*/

_PUBLIC FTYPE lsq_fgen(FTYPE tr, int fit, FTYPE lsq_coeff[])

{  if(fit == LINFIT)
      tr = lsq_coeff[0] + lsq_coeff[1]*tr;
   else
      if(fit == INVSFIT)
         tr = 1 / (lsq_coeff[0] + lsq_coeff[2]*tr);
      else
         if(fit == LOGFIT)
            tr = lsq_coeff[1] + lsq_coeff[2]*log(tr);
         else
            if(fit == EXPNFIT)
               tr = lsq_coeff[1]*EXP(lsq_coeff[2]*tr);
            else
               if(fit == PWRFIT)
                  tr = lsq_coeff[1] + EXP(log(tr) * lsq_coeff[2]);
               else
                  pups_error("[lsq_fgn] incorrect fit parameter");

    return(tr);
}





/*------------------------------------------------------------------------------
    Linear interpolation routine ...
------------------------------------------------------------------------------*/

_PUBLIC FTYPE lint(FTYPE xa[],    // Array of X values
                   FTYPE ya[],    // Array of Y values
                   int      n,    // Number of points
                   FTYPE    x)    // Point to interpolate

{   int klo,
        khi,
          k;

    FTYPE h,
          ret;

    /*-----------------------------------------------------------------------*/
    /* We will find the correct place in the table by means of a bisection.  */
    /* This is optimal if sequential calls to this routine are at random     */
    /* values of X. If the sequential calls are in order and closely spaced, */
    /* it would be better to store the previous values of klo,khi and test,  */
    /* if they remain appropriate on the next call                           */
    /* ----------------------------------------------------------------------*/

    klo = 0;
    khi = n;

    while(khi - klo > 1)
    {   k = (khi + klo) >> 1;
        if(xa[k] > x)
           khi = k;
        else
           klo = k;
    }


    /*---------------------------*/
    /* The xa's must be distinct */
    /*---------------------------*/

    if(xa[khi] - xa[klo] == 0.0)
        pups_error("[lint] xa's must be distinct");


    /*----------------------------------------------*/
    /* klo and khi now bracket the input value of x */
    /*----------------------------------------------*/

    h = (ya[khi] - ya[klo]) / (xa[khi] - xa[klo]);


    /*--------------------------------------------------------*/
    /* Interpolate numerical function within bracketed region */
    /*--------------------------------------------------------*/

    ret = ya[klo] + h*(x - xa[klo]);
    return(ret);
}




/*-----------------------------------------------------------------------------
    Cubic spline interpolation. For further details see
    "Numerical Recipes in C" pp96-97 ...
-----------------------------------------------------------------------------*/

_PUBLIC FTYPE splint(FTYPE  xa[],     // Array of X values 
                     FTYPE  ya[],     // Array of Y values 
                     FTYPE y2a[],     // Array of derivatives dy/dx 
                     int       n,     // Number of points 
                     FTYPE     x)     // Point to interpolate 

{


    /*----------------------------------------------------------------------*/
    /* Given the arrays xa[1..n] and ya[1..n] which tabulate a funstion     */
    /* (with the xa's in order) and given the array y2a[1..n] which is the  */
    /* output from the spline function and given a value of x, this routine */
    /* returns a cubic spline interpolated y                                */
    /* ---------------------------------------------------------------------*/

    int k;

    _IMMORTAL int klo,
                  khi;

    _IMMORTAL _BOOLEAN init = TRUE;

    FTYPE h,
          b,
          a,
          ret;

    /*-----------------------------------------------------------------------*/
    /* We will find the correct place in the table by means of a bisection.  */
    /* This is optimal if sequential calls to this routine are at random     */
    /*                                                                       */
    /* values of X. If the sequential calls are in order and closely spaced, */
    /* it would be better to store the previous values of klo,khi and test,  */
    /* if they remain appropriate on the next call                           */
    /* ----------------------------------------------------------------------*/

    if(init == TRUE)
    {  klo = 0;
       khi = n;
    }

    while(khi - klo > 1)
    {   k = (khi + klo) >> 1;
        if(xa[k] > x)
           khi = k;
        else
           klo = k;
    }


    /*----------------------------------------------*/
    /* klo and khi now bracket the input value of x */
    /*----------------------------------------------*/

    h = xa[khi] - xa[klo];


    /*---------------------------*/
    /* The xa's must be distinct */
    /*---------------------------*/

    if(h == (FTYPE)0.0)
        pups_error("[splint] xa's must be distinct");


    /*-----------------------------------*/
    /* Cubic spline may now be evaluated */
    /*-----------------------------------*/

     a = (xa[khi] - x) / h;
     b = (x - xa[klo]) / h;
     ret = a*ya[klo] + b*ya[khi] + ((a*a*a - a)*y2a[klo] +
                                   (b*b*b - b)*y2a[khi])*(h*h) /
                                                     (FTYPE)6.0;

     return(ret);
}





/*------------------------------------------------------------------------------
    Routine to integrate a numerical function using the Trapezium rule ...
------------------------------------------------------------------------------*/

_PUBLIC FTYPE trapspl(FTYPE    t_1,    // Integral lower limit 
                      FTYPE    t_2,    // Integral upper limit
                      FTYPE      h,    // Trapezium size h 
                      FTYPE    y[],    // Array of Y values 
                      FTYPE   y2[],    // Array of derivatives dy/dx 
                      FTYPE    x[],    // Array of Y values 
                      int    n_pts)    // Number of pts in num function 

{   int i,
        i_pts;

    FTYPE sum,
         comp,
         time,
         t_pts,
         frac_pts,
         i_sign;


    /*-----------------------------------------------------------------------*/
    /* If the upper and the lower limits of the integral are the same return */
    /* zero result                                                           */
    /* ----------------------------------------------------------------------*/

    if(t_1 == t_2)
      return((FTYPE)0.0);


    /*------------------------------*/
    /* Get the sign of the integral */
    /*------------------------------*/

    if(t_2 < t_1)
    {  time = t_2;
       i_sign = (-1.0);
    }
    else
    {  time   = t_1;
       i_sign = 1.0;
    }


    /*----------------------------------*/
    /* Get effective number of trapezia */
    /*----------------------------------*/

    t_pts    = FABS(t_2 - t_1) / h;
    frac_pts = t_pts - FLOOR(t_pts);


    /*-----------------------------------------------------------------------*/
    /* If there is a fractional number of trapezia, round up to next integer */
    /* value and recalculate step length                                     */
    /*-----------------------------------------------------------------------*/

    if(frac_pts > ASSUMED_ZERO)
    {  i_pts = (int)FLOOR(t_pts) + 1;


       /*---------------------------------*/
       /* Get the effective step length h */
       /*---------------------------------*/

       h = FABS(t_2 - t_1) / i_pts;
    }
    else
       i_pts = (int)(t_pts);



    /*-----------------------------------------------------------*/
    /* Find the values of the intergrand at the endpoints of the */
    /* integral                                                  */
    /*-----------------------------------------------------------*/

    sum =  splint(x,y,y2,n_pts,t_1)*h / 2.0;
    sum += splint(x,y,y2,n_pts,t_2)*h / 2.0;



    /*-----------------------------------------*/
    /* Find the pups_main part of the integral */
    /*-----------------------------------------*/

    for(i=1; i<i_pts; ++i)
    {  comp = splint(x,y,y2,n_pts,time)*h;
       sum  += comp;
       time += h;
    }


    /*-------------------*/
    /* Return the result */
    /*-------------------*/

    return(sum*i_sign);
}




/*------------------------------------------------------------------------------
    Integration routine using Simpsons rule ...
------------------------------------------------------------------------------*/

_PUBLIC FTYPE Simpson_spl(FTYPE        from,   // Integral lower limit
                          FTYPE          to,   // Integral upper limit
                          FTYPE        step,   // Simpson step length
                          FTYPE   y_table[],   // Array of Y values
                          FTYPE  yd_table[],   // Array of derivatives
                          FTYPE   x_table[],   // Array of X values
                          int         n_pts)   // Number of pts in function

{  int i,
       s_pts;

   FTYPE sum,
         sume,
         sumo,
         val;

    /*-------------------------------------------------------------------------*/
    /* Get number of points over which integration will take place, if this is */
    /* even then auto adjust the number of points                              */ 
    /*-------------------------------------------------------------------------*/

    s_pts = (int)((to - from) / step) + 1;
    if(s_pts < 3)
    {  s_pts = 3;
       step = (to - from) / s_pts;
    }

    if(ieven(s_pts) == TRUE)
    {  ++s_pts;
       step = (to - from) / s_pts;
    }


    /*----------------*/
    /* Sum odd points */
    /*----------------*/

    i   = 0;
    sumo = 0.0;
    val = from + step;

    do {  sumo += 4*splint(x_table,y_table,yd_table,n_pts,val);
          val  += 2*step;
          i    += 2;
       } while(i < s_pts - 1);


    /*-----------------*/
    /* Sum even points */
    /* ----------------*/

    i = 2;
    sume = 0.0;
    val = from + 2*step;

    do {  sume += 2*splint(x_table,y_table,yd_table,n_pts,val);
          val += 2*step;
          i += 2;
       } while(i < s_pts - 2);


    /*----------------*/
    /* Add end points */
    /*----------------*/

    sum = (splint(x_table,y_table,yd_table,n_pts,from) +
           splint(x_table,y_table,yd_table,n_pts,to))  * step/3;

    return((sum + (step*(sumo + sume) / 3))*fsign(to - from));
}



/*------------------------------------------------------------------------------
    Routine to differentiate using Milnes' method ...
------------------------------------------------------------------------------*/

_PUBLIC FTYPE Milne_diff(int el, FTYPE h, FTYPE y_arr[])

{   return((-3 * y_arr[el - 1] + 4*y_arr[el] - y_arr[el + 1]) / (2*h));
}




/*------------------------------------------------------------------------------
    Global definitions required by linmin/f1dim routines ...
------------------------------------------------------------------------------*/

int ncom = 0;

FTYPE pcom[MAX_D]  = { 0.0 },
      xicom[MAX_D] = { 0.0 },
      (*nrfunc)()  = (FTYPE *)NULL;

/*------------------------------------------------------------------------------
    Linear minimisation routine ...
------------------------------------------------------------------------------*/

_PRIVATE void linmin(FTYPE        p[],
                     FTYPE       xi[],
                     int            n,
                     FTYPE      *fret,
                     FTYPE  (* func)())

{   int j;

    FTYPE xx,
          xmin,
          fx,
          fb,
          fa,
          bx,
          ax;


    /*---------------------------------------------------------------------*/
    /* Given an n dimesional point p[1..n], and an n dimensional direction */
    /* xi[1..n] moves and resets p to where the function, func(p) takes on */
    /* a minimum along the direction xi from p, and replaces by the actual */
    /* vector displacement that p was moved. Also returns fret, the value  */
    /* of func at location p. This is all actually accomplished by calling */
    /* the routines mbrak and Brent ...                                    */
    /*                                                                     */
    /* Define global variables                                             */
    /* --------------------------------------------------------------------*/

    ncom   = n;
    nrfunc = func;

    for(j=0; j<n; j++)
    {   pcom[j]  = p[j];
        xicom[j] = xi[j];
    }


    /*----------------------------*/
    /* Initial guess for brackets */
    /*----------------------------*/

    ax = 0.0;
    xx = 1.0;
    bx = 2.0;

    mnbrak(&ax,&xx,&bx,&fa,&fx,&fb,f1dim);
    *fret = Brent(ax,xx,bx,f1dim,TOL,&xmin);


    /*----------------------------*/
    /* Construct vector to return */
    /*----------------------------*/

    for(j=0; j<n; j++)
    {   xi[j] *= xmin;
        p[j]  += xi[j];
    }
}



/*------------------------------------------------------------------------------
    This function MUST accompany linmin ...
------------------------------------------------------------------------------*/

_PRIVATE FTYPE f1dim(FTYPE x)

{   int j;

    FTYPE f,
          xt[32] = { 0.0 };

    for(j=0; j<ncom; j++)
        xt[j] = pcom[j] + x*xicom[j];

    f = (*nrfunc)(xt);
    return(f);
}




/*------------------------------------------------------------------------------
    Routine to minimise a linear function using Brent's method ...
------------------------------------------------------------------------------*/

_PUBLIC FTYPE Brent(FTYPE      ax,    // Bracket 1
                    FTYPE      bx,    // Bracket 2
                    FTYPE      cx,    // Bracket 3
                    FTYPE ( *f)(),    // Function to be minimised
                    FTYPE     tol,    // Relaxation tolerance
                    FTYPE   *xmin)    // X co-ordinate of minimum

    /*-------------------------------------------------------------------------*/
    /* Given a function f, and given a bracketing triplet of abiscissas ax,    */
    /* bx, cx (such that bx is between ax and cx) and f(bx) is less than both. */
    /* f(ax) and f(cx), this routine isolates the minimum to fractional     .  */
    /* precision of about tol using Brent's method. The abscisa of the value   */
    /* is returned as xmin, and the minimum function is returned as Brent, the */
    /* returned function value                                                 */
    /* ------------------------------------------------------------------------*/

{   int iter;

    FTYPE a,
          b,
          d,
          etemp,
          fu,
          fv,
          fw,
          fx,
          p,
          q,
          r,
          tol1,
          tol2,
          u,
          v,
          w,
          x,
          xm,
          e = 0.0;

    /*---------------------------------------------------------------------*/
    /* a and b must be in ascending order, though the input abscissas need */
    /* not be                                                              */
    /*---------------------------------------------------------------------*/

    a = ((ax < cx) ? ax : cx);
    b = ((ax > cx) ? ax : cx);

    x  = w  = v  = bx;
    fw = fv = fx = (*f)(x);


    /*-------------------*/
    /* Main routine loop */
    /*-------------------*/

    for(iter=1; iter<= BR_ITMAX; iter++)
    {   xm   = 0.5*(a+b);
        tol2 = 2.0*(tol1 = tol*FABS(x) + ZEPS);


        /*--------------------*/
        /* Test for done here */
        /*--------------------*/

        if(FABS(x - xm) <= (tol2 - 0.5*(b - a)))
        {  *xmin = x;
           return(fx);
        }


        /*---------------------------------*/
        /* Construct a trial parabolic fit */
        /*---------------------------------*/

        if(FABS(e) > tol1)
        {  r = (x - w)*(fx - fv);
           q = (x - v)*(fx - fw);
           p = (x - v)*q - (x - w)*r;
           q = 2.0*(q - r);
           if(q > 0.0)
              p = (-p);
           q = FABS(q);
           etemp = e;
           e = d;


           /*----------------------------------------------------------------------*/
           /* Determine the acceptabilty of parabolic fit, and take Golden Section */
           /* Search into the larger of the two segments                           */
           /*----------------------------------------------------------------------*/

           if(FABS(p) >= FABS(0.5*q*etemp) ||
              p <= q*(a - x)               ||
              p >= q*(b - x))
              d = CGOLD*(e = (x >= xm ? a-x : b-x));
           else
           {


              /*--------------------------*/
              /*  Take the parabolic step */
              /*--------------------------*/

              d = p / q;
              u = x + d;

              if(u-a < tol2 || b-u < tol2)
                 d = SIGN(tol1,xm-x);
           }
       }
       else
          d = CGOLD*(e = (x >= xm ? a-x : b-x));

       u = (FABS(d) >= tol1 ? x+d : x + SIGN(tol1,d));


       /*----------------------------------------------------*/
       /*  This is the one function evaluation per iteration */
       /*----------------------------------------------------*/

       fu = (*f)(u);


       /*----------------------------------------------------------*/
       /* Now we have to decide what to do with generated function */
       /*----------------------------------------------------------*/

       if(fu <= fx)
       {  if(u >= x)
             a = x;
          else
             b = x;
          SHFT(v,w,x,u);
          SHFT(fv,fw,fx,fu);
       }
       else
       {  if(u < x)
             a = u;
          else
             b = u;
          if(fu <= fw || w == x)
          {  v  = w;
             w  = u;
             fv = fw;
             fw = fu;
          }
          else
          {  if(fu <= fv || v == x || v == w)
             {  v  = u;
                fv = fu;
             }
          }
      }
   }
   pups_error("[Brent] too many iterations");

   /*---------------------------------------------*/
   /* Stop compiler from producing error messages */
   /*---------------------------------------------*/

   return(/*DDUM*/ 0);
}



/*------------------------------------------------------------------------------
    Minimisation in N dimesions using Powells' method ...
------------------------------------------------------------------------------*/

_PUBLIC void Powell(FTYPE          p[],  // Co-ordinate of minimum
                    FTYPE  xi[][MAX_D],  // Direction set matrix
                    int              n,  // Number of dimensions
                    int        maxiter,  // Iteration limit
                    FTYPE         ftol,  // Relaxation tolerance
                    int          *iter,  // Actual iterations
                    FTYPE        *fret,  // Value of minimum
                    FTYPE   (* func)(),  // Function to minimise
                    FTYPE   (* m_f1)(),
                    FTYPE   (* m_f2)())


    /*-------------------------------------------------------------------------*/
    /* Minimisation of a function of n variables. Input consists of an         */
    /* initial starting point p[1..n]; an initial matrix xi[1..n][1..n] whose  */
    /* columns usually contain the unit vectors in the directions defined by   */
    /* a set of orthognal vectors at the starting point. ftol is a fractional  */
    /* tolerance function value such that failure to decrease by more than     */
    /* this amount on 1 iteration signifies doneness. On output, p is set to   */
    /* the best point found, xi is the current direction set, fret is then the */
    /* returned function value at the current location of p.                   */
    /*                                                                         */
    /* Further details of the operation of this routine are given in           */
    /* "Numerical Recipes in C, pp.314-316                                     */
    /* ------------------------------------------------------------------------*/

{   int i,
        j,
        ibig;

    FTYPE t,
          fptt,
          fp,
          del,
          pt[MAX_D]  = { 0.0 },
          ptt[MAX_D] = { 0.0 },
          xit[MAX_D] = { 0.0 };

    /*-----------------------*/
    /* Compute initial point */
    /*-----------------------*/

    if(maxiter < 3)
       maxiter = 3;
   
    *fret = (*func)(p);


    /*-------------------*/
    /* Save intial point */
    /*-------------------*/

    for(j=0; j<n; ++j)
    {  pt[j]  = p[j];


       /*----------------------------------------------------------*/
       /* Initialise ptt - prevents loop monitor from getting junk */
       /* at first invocation                                      */
       /*----------------------------------------------------------*/

       ptt[j] = p[j];
    }

    for(*iter=1;;(*iter)++)
    {   fp   = (*fret);
        ibig = 0;
        del  = 0.0;


        /*---------------------------------------------------------*/
        /* For each iteration, loop over all directions in the set */
        /*---------------------------------------------------------*/

        for(i=0; i<n; i++)
        {

            /*--------------------*/
            /* Copy the direction */
            /*--------------------*/

            for(j=0; j<n; j++)
                xit[j] = xi[j][i];

            fptt = (*fret);


            /*-------------------*/
            /* Minimise along it */
            /*-------------------*/

            linmin(p,xit,n,fret,func);


            /*------------------------------------------------*/
            /* Record it if it is the largest decrease so far */
            /*------------------------------------------------*/

            if(FABS(fptt - (*fret)) > del)
            {  del  = FABS(fptt - (*fret));
               ibig = i;
            }

        }


        /*----------------------------------------------*/
        /* If appropriate enter the performance monitor */
        /*----------------------------------------------*/

        if(*m_f1 != NULL)
          (*m_f1)(*iter,*fret,ptt);


        /*-----------------------*/
        /* Termination criterion */
        /*-----------------------*/

        if(2.0*FABS(fp - (*fret)) <= ftol*(FABS(fp)+FABS(*fret)) ||
           (*iter > maxiter))
        {  

           /*------------------------------------------*/
           /* Force Powell to do at least 3 iterations */
           /*------------------------------------------*/

           if(*m_f2 != NULL)
              (*m_f2)(*iter,*fret);
           return;
        }


        /*------------------------------------------------------------------------*/
        /* Construct the extrapolated point and average the direction moved. Save */
        /* the old starting point                                                 */
        /*------------------------------------------------------------------------*/

        for(j=0; j<n; j++)
        {   ptt[j] = 2.0*p[j] - pt[j];
            xit[j] = p[j] - pt[j];
            pt[j]  = p[j];
        }


        /*------------------------------------------*/
        /* Function value at the extrapolated point */
        /*------------------------------------------*/

        fptt = (*func)(ptt);
        if(fptt < fp)
        {  t = 2.0*(fp - 2.0*(*fret) + fptt)*SQR(fp - (*fret) -  del) -
                                                    del*SQR(fp - fptt);


           /*--------------------------------------*/
           /* Move to minimum of the new direction */
           /*--------------------------------------*/

           if(t < 0.0)
           {  linmin(p,xit,n,fret,func);


              /*--------------------*/
              /* Save new direction */
              /*--------------------*/

              for(j=0; j<n; j++)
                  xi[j][ibig] = xit[j];
           }
       }
    }
}




/*------------------------------------------------------------------------------
    Routine to bracket a minimum in 1 dimension ...
------------------------------------------------------------------------------*/

_PUBLIC void mnbrak(FTYPE         *ax,   // Bracket 1 
                    FTYPE         *bx,   // Bracket 2
                    FTYPE         *cx,   // Bracket 3
                    FTYPE         *fa,   // f(bracket 1)
                    FTYPE         *fb,   // f(bracket 2)
                    FTYPE         *fc,   // f(bracket 3)
                    FTYPE  (* func)())   // Function to minimise 

{   FTYPE u,
          r,
          q,
          fu,
          dum,
          ulim;


    /*-------------------------------------------------------------------------*/
    /* Given a function func, and given distinct intial points at ax and bx,   */
    /* this routine searches in the downhill direction (defined by the         */
    /* function evaluated at the intial points), and returns new points, ax,   */
    /* bx, and cx which bracket the minimum of the function. Also returned are */
    /* the function values at the three points, fa, fb and fc                  */
    /*-------------------------------------------------------------------------*/

    *fa = (*func)(*ax);
    *fb = (*func)(*bx);


    /*---------------------------------------------------------------------*/
    /* Switch roles of a and b so that we can go downhill in the direction */
    /* from a to b                                                         */
    /* --------------------------------------------------------------------*/

    if(*fb > *fa)
    {  SHFT(dum,*ax,*bx,dum)
       SHFT(dum,*fb,*fa,dum)
    }


    /*-------------------*/
    /* First guess for C */
    /*-------------------*/

    *cx = (*bx) + GOLD*(*bx - *ax);
    *fc = (*func)(*cx);


    /*-------------------------------------*/
    /* Keep returning here until bracketed */
    /*-------------------------------------*/

    while(*fb > *fc)
    {   r = (*bx - *ax)*(*fb - *fc);
        q = (*bx - *cx)*(*fb - *fa);


        /*----------------------------------------------------------------------*/
        /* Compute u by parabolic extrapolation from a,b and c, TINY is used to */
        /* prevent possible devision by zero                                    */
        /*----------------------------------------------------------------------*/

        u = (*bx) - ((*bx - *cx)*q - (*bx - *ax)*r) /
                    (2.0*SIGN(MAX(FABS(q - r),TINY),q - r));

        ulim = (*bx) + GLIMIT*(*cx - *bx);


        /*----------------------------*/
        /* Test various possibilities */
        /*----------------------------*/

        if((*bx - u)*(u - *cx) > 0.0)
        {

            /*--------------------------------------*/
            /* Parabolic u between b and c - try it */
            /*--------------------------------------*/

            fu = (*func)(u);
            if(fu < *fc)
            {


               /*-------------------------------*/
               /* Got a minimum between b and c */
               /*-------------------------------*/

               *ax = (*bx);
               *bx = u;
               *fa = (*fb);
               *fb = fu;
               return;
            }
            else
            {  if(fu > *fb)
               {

                   /*-------------------------------*/
                   /* Got a minimum between a and u */
                   /*-------------------------------*/

                   *cx = u;
                   *fc = fu;
                   return;
                }
           }


           /*------------------------------------------------------*/
           /*  Parabolic fit was no use. Use default magnification */
           /*------------------------------------------------------*/

           u  = (*cx) + GOLD*(*cx - *bx);
           fu = (*func)(u);
        }
        else
        {

           /*--------------------------------------------------*/
           /* Parabolic fit is between c and its allowed limit */
           /*--------------------------------------------------*/

           if((*cx - u)*(u - ulim) > 0.0)
           {  fu = (*func)(u);
              if(fu < *fc)
              {  SHFT(*bx,*cx,u,*cx + GOLD*(*cx - *bx))
                 SHFT(*fb,*fc,fu,(*func)(u))
              }
           }
           else
           {  if((u - ulim)*(ulim - *cx) >= 0.0)
              {


                   /*-----------------------------------------*/
                   /*  Limit parabolic u to its maximum value */
                   /*-----------------------------------------*/

                   u  = ulim;
                   fu = (*func)(u);
               }
               else
               {

                  /*-----------------------------------------------*/
                  /* Reject parabolic u, use default magnification */
                  /*-----------------------------------------------*/

                  u  = (*cx) + GOLD*(*cx - *bx);
                  fu = (*func)(u);
               }
            }
        }


        /*-------------------------------------*/
        /* Eliminate oldest point and continue */
        /*-------------------------------------*/

        SHFT(*ax,*bx,*cx,u);
        SHFT(*fa,*fb,*fc,fu);
    }
}




/*------------------------------------------------------------------------------
    Find the minimum of a function of one dimension using the Golden
    section search ...
------------------------------------------------------------------------------*/

_PUBLIC FTYPE golden(FTYPE       ax,   // Guess [bracket 1 co-ordinate] 
                     FTYPE       bx,   // Guess [bracket 2 co-ordinate]
                     FTYPE       cx,   // Guess [bracket 3 co-ordinate]
                     FTYPE  (* f)(),   // Function to be bracketed
                     FTYPE      tol,   // Bracket tolerance
                     FTYPE    *xmin)   // Co-ordinate of minimum


    /*----------------------------------------------------------------------*/
    /* Given a function f, and given a bracketing triplet of abcissas ax,   */
    /* bx, and cx, (such that bx is between ax and cx), and f(bx) is less   */
    /* than f(ax) and f(cx)), this routine performs golden section search   */
    /* for the minimum, isolating it to a fractional tolerance of about     */
    /* tol. The absissa of the minimum is returned as xmin, and the minimum */
    /* function value is returned as golden, the returned function          */
    /* value                                                                */
    /*----------------------------------------------------------------------*/

{   FTYPE f0,
          f1,
          f2,
          f3,
          x0,
          x1,
          x2,
          x3;


    /*------------------------------------------------------------------*/
    /* At any given time, we will keep track of four points x0,x1,x2,x3 */
    /*------------------------------------------------------------------*/

    x0 = ax;
    x3 = cx;


    /*-----------------------------------*/
    /* Make x0 to x1 the smaller segment */
    /*-----------------------------------*/

    if(FABS(cx - bx) > FABS(bx - ax))
    {  x1 = bx;    // Fill in new point to be tried
       x2 = bx + C*(cx - bx);
    }
    else
    {  x2 = bx;
       x1 = bx + C*(bx - ax);
    }


    /*--------------------------------------------------------------*/
    /* The initial function evaluations. Note that we never need to */
    /* evaluate the function at the original endpoints              */
    /* -------------------------------------------------------------*/

    f1 = (*f)(x1);
    f2 = (*f)(x2);

    while(FABS(x3 - x0) > tol*(FABS(x1) + FABS(x2)));
    {    if(f2 < f1)
         {  SHFT(x0,x1,x2,R*x1 + C*x3)
            SHFT(f0,f1,f2,(*f)(x2));
         }
         else
         {  SHFT(x3,x2,x1,R*x2 + C*x0);
            SHFT(f3,f2,f1,(*f)(x1));
         }
     }


    /*------------------------------------------------------*/
    /*  We are done - output the best of two current values */
    /*------------------------------------------------------*/

    if(f1 > f2)
    {  *xmin = x1;
       return(f1);
    }
    else
    {  *xmin = x2;
       return(x2);
    }
}




/*------------------------------------------------------------------------------
    Routine to minimise a function of N dimesions stochatically using
    simulated annealing ...
------------------------------------------------------------------------------*/

_PUBLIC void anneal(FTYPE          p[],   // Co-ordinate of minimum 
                    FTYPE  delta_range,   // Delta range for random jumping 
                    int              n,   // Number of dimensions in function
                    FTYPE         ftol,   // Relaxation tolerance
                    int          *iter,   // Number of iterations
                    FTYPE        *fret,   // Value of minimum
                    FTYPE   (* func)(),   // Function to minimise
                    FTYPE   (* m_f1)(),   // Loop monitor function
                    FTYPE   (* m_f2)())   // Exit monitor function

{   int i;

    _BOOLEAN looper = TRUE;

    FTYPE current_cost,
          ptt[MAX_D] = { 0.0 };


    /*-----------------------------*/
    /* Initialise the current cost */
    /*-----------------------------*/

    current_cost = (*func)(p);


    /*--------------------------------------------*/
    /* Main loop of simulated annealing minimiser */
    /*--------------------------------------------*/

    do {

           /*-----------------------------------*/
           /* Genrerate new purturbation vector */
           /*-----------------------------------*/

           for(i=0; i<n; ++i)
              ptt[i] =  p[i] + delta_range*(ran1() - 0.5);


           /*--------------------------------------------*/
           /*  Find out the cost of the new state vector */
           /*--------------------------------------------*/

           *fret = (*func)(ptt);


           /*---------------------------------------------------------------------*/
           /* If returned cost less than current cost, replace p with p_delta and */
           /* current cost with fret                                              */
           /*---------------------------------------------------------------------*/

           if(*fret < current_cost)
           {  current_cost = *fret;


              /*----------------------------------------------*/
              /* If appropriate enter the performance monitor */
              /*----------------------------------------------*/

              if(*m_f1 != NULL)
                (*m_f1)(*iter,*fret,ptt);


               /*------------------------------------------------*/
               /*  If the change is less than ftol - we are done */
               /*------------------------------------------------*/

               if(*fret - current_cost < ftol)
               {  if(*m_f2 != NULL)
                    (*m_f2)();
                  return;
               }
            }
        } while(looper == TRUE);

    for(i=1; i<n; ++i)
        p[i] = ptt[i];
}
