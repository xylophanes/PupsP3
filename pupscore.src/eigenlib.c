/*-----------------------------------------------------------------------------------
    Purpose:  Eigenvector anaylis library

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 1.05
    Dated:   4th Janiary 2023
    E-mail:  mao@tumblingdice.co.uk

    Some routines here are derived from those given in Press et al (Numerical
    Recipes in C).
----------------------------------------------------------------------------------*/


#include <me.h>
#include <utils.h>
#include <casino.h>
#include <errno.h>
#include <string.h>


/*----------------------------------------*/
/* Do not expand _EXTERN for this library */
/*----------------------------------------*/

#undef   __NOT_LIB_SOURCE__
#include <eigen.h>
#define  __NOT_LIB_SOURCE__



/*---------------------------------------------*/
/* Definitions which are local to this library */
/*---------------------------------------------*/
 
#ifndef SIGN
#define SIGN(a,b) ((b) >= 0.0 ? FABS(a) : -FABS(a))
#endif /* SIGN */


#ifndef SQR
_PRIVATE FTYPE sqrarg;
#define SQR(a) ((sqrarg = (a)) == 0.0 ? 0.0 : sqrarg*sqrarg)
#endif /* SQR */




/*-------------------------------------------------*/
/* Slot and usage functions - used by slot manager */
/*-------------------------------------------------*/


/*---------------------*/
/* Slot usage function */
/*---------------------*/

_PRIVATE void eigenlib_slot(int level)
{   (void)fprintf(stderr,"lib eigenlib %s: [ANSI C]\n",EIGEN_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(C) 1985-2023 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"PUPS/P3 eigenvector analysis library (built %s %s)\n\n",__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");
    (void)fflush(stderr);
}




/*---------------------------------------------*/
/* Segment identification for eigenlib library */
/*---------------------------------------------*/

#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT )() __attribute__ ((aligned(16))) = eigenlib_slot;
#endif /* SLOT */




/*--------------------------------------------*/
/* Functions which are private to this module */
/*--------------------------------------------*/

_PROTOTYPE _PRIVATE FTYPE pythag(FTYPE a, FTYPE b);


/*----------------------------------------------------------------------------------
    Reduce matrix to tridiagonal form ...
----------------------------------------------------------------------------------*/

_PUBLIC int tqli(FTYPE *d, FTYPE *e, int n, FTYPE **z)

{   int m,
        l,
        iter,
        i,
        k;

     FTYPE s,
           r,
           p,
           g,
           f,
           dd,
           c,
           b;

     for(i=2; i<=n; i++)
        e[i-1]=e[i];

     e[n]=0.0;
     for(l=0; l<=n; l++)
     {   iter=0;

         do {    for(m=l; m<=n-1; m++)
                 {  dd = FABS(d[m])+FABS(d[m+1]);
                    if((FTYPE)(FABS(e[m])+dd) == dd)
                        break;
                 }

                 if(m != l)
                 {  if(iter++ == 30)
                    {  pups_set_errno(E2BIG);
                       return(-1);
                    }
                 } 

                 g = (d[l+1]-d[l])/(2.0*e[l]);
                 r = pythag(g,1.0);
                 g = d[m]-d[l]+e[l]/(g+SIGN(r,g)); 
                 s = c = 1.0;
                 p = 0.0;

                 for(i=m-1; i>=l; i--)
                 {  f      = s*e[i];
                    b      = c*e[i];
                    e[i+1] = (r=pythag(f,g));

                    if(r == 0.0)
                    {  d[i+1] -= p;
                       e[m]   =  0.0;
                       break;
                    }

                    s      = f/r;
                    c      = g/r;
                    g      = d[i+1] - p;
                    r      = (d[i]-g)*s + 2.0*c*b;
                    d[i+1] = g + (p=s*r);
                    g      = c*r - b;

                    for(k=1; k<=n; k++)
                    {  f         = z[k][i+1];
                       z[k][i+1] = s*z[k][i]+c*f;
                       z[k][i]   = c*z[k][i]-s*f;
                    }
                  }

                  if(r == 0.0 && i >= l)
                     continue;

                  d[l] -= p;
                  e[l] =  g;
                  e[m] =  0.0;
            } while (m != l);
    }

    pups_set_errno(OK);
    return(0);
}





/*---------------------------------------------------------------------------------------------
   Get eigenvalues and eigenvectos of a matrix by Householder reduction ...
---------------------------------------------------------------------------------------------*/

_PUBLIC void tred2(_BOOLEAN eigenvectors, FTYPE **a, int n, FTYPE *d, FTYPE *e)

{
    int l,
        k,
        j,
        i;

    FTYPE scale,
          hh,
          h,
          g,
          f;

    for(i=n-1; i>=2; i--)
    {  l = i-1;
       h = scale=0.0;

       if(l > 1)
       {  for(k=1; k<=l; k++)
              scale += FABS(a[i][k]);

          if(scale == 0.0)
             e[i] = a[i][l];
          else
          {  for(k=1; k<=l; k++)
             {   a[i][k] /= scale;
                 h       += a[i][k]*a[i][k];
             }

             f    =  a[i][l];
             g    =  (f >= 0.0 ? -SQRT(h) : SQRT(h));
             e[i] =  scale*g;

             h       -= f*g;
             a[i][l] =  f - g;
             f       =  0.0;

             for(j=1; j<=l; j++)
             {   a[j][i] = a[i][j]/h;
                 g       = 0.0;

                 for(k=1; k<=j; k++)
                    g += a[j][k]*a[i][k];

                 for(k=j+1; k<=l; k++)
                    g += a[k][j]*a[i][k];

                 e[j] =  g/h;
                 f    += e[j]*a[i][j];
             }

             hh=f/(h+h);
             for(j=1; j<=l; j++)
             {   f    =a[i][j];
                 e[j] = g = e[j] - hh*f;

                 for(k=1; k<=j; k++)
                 a[j][k] -= (f*e[k]+g*a[i][k]);
             }
          }
       }
       else
          e[i]=a[i][l];

       d[i]=h;
    }

    d[0] = 0.0;
    e[0] = 0.0;

    for(i=1; i<=n; i++)
    {  if(eigenvectors == TRUE)
       {  l = i-1;

          if(d[i])
          {  for(j=1; j<=l; j++)
             {  g = 0.0;

                for(k=1; k<=l; k++)
                   g += a[i][k]*a[k][j];

                for(k=1; k<=l; k++)
                    a[k][j] -= g*a[k][i];
             }
          }

          d[i]    = a[i][i];
          a[i][i] = 1.0;

          for(j=1; j<=l; j++)
             a[j][i]=a[i][j]=0.0;
       }
       else
          d[i]    = a[i][i];
    }
}




/*------------------------------------------------------------------------------------------
    Support routine for tqli and tred2 ...
------------------------------------------------------------------------------------------*/

_PRIVATE FTYPE pythag(FTYPE a, FTYPE b)
{   FTYPE absa,
          absb;

    absa = FABS(a);
    absb = FABS(b);

    if(absa > absb)
       return absa*SQRT(1.0+SQR(absb/absa));
    else
       return (absb == 0.0 ? 0.0 : absb*SQRT(1.0+SQR(absa/absb))); 
}




/*-------------------------------------------------------------------------------------------
    Jacobi transformation of a symmetric matrix to compute the eigenvalues and
    eigenvectors ...
-------------------------------------------------------------------------------------------*/

#define ROTATE(a,i,j,k,l) g=a[i][j];h=a[k][l];a[i][j]=g-s*(h+g*tau);\
	a[k][l]=h+s*(g-h*tau);

_PUBLIC int jacobi(FTYPE **a, int n, FTYPE *d, FTYPE **v, int *nrot)

{   int i,
        j,
        iq,
        ip;
    
	FTYPE thresh,
              theta,
              tau,
              t,
              sm,
              s,
              h,
              g,
              c,
              *b = (FTYPE *)NULL,
              *z = (FTYPE *)NULL;

	b = (FTYPE *)pups_calloc(n,sizeof(FTYPE));
	z = (FTYPE *)pups_calloc(n,sizeof(FTYPE));

	for(ip=1; ip<=n; ip++)
        {   for(iq=0; iq<n; iq++)
               v[ip][iq] = 0.0;

            v[ip][ip]=1.0;
        }

        for(ip=1; ip<=n; ip++)
        {   b[ip]=d[ip] = a[ip][ip];

            z[ip]=0.0;
        }

        *nrot = 0;
        for(i=1; i<=50; i++)
        {  sm = 0.0;

           for(ip=1; ip<=n-1; ip++)
           {  for(iq=ip+1; iq<=n; iq++)
                 sm += FABS(a[ip][iq]);
           }

           if(sm == 0.0)
           {  (void)pups_free((void *)z);
              (void)pups_free((void *)b);

              pups_set_errno(OK);
              return(0);
           }

           if(i < 4)
              thresh=0.2*sm/(n*n);
           else
              thresh=0.0;

            for(ip=1; ip<=n-1; ip++)
            {   for(iq=ip+1; iq<=n; iq++)
                {   g = 100.0*FABS(a[ip][iq]);

                    if(i > 4 && (FABS(d[ip]) + g) == FABS(d[ip]) && (FABS(d[iq]) + g) == FABS(d[iq]))
                       a[ip][iq]=0.0;
                    else if(FABS(a[ip][iq]) > thresh)
                    {  h = d[iq] - d[ip];

                       if((FABS(h) + g) == FABS(h))
                          t = (a[ip][iq])/h;
                       else
                       {  theta = 0.5*h/(a[ip][iq]);
                          t     = 1.0/(FABS(theta)+SQRT(1.0+theta*theta));

                          if(theta < 0.0)
                             t = -t;
                       }

                       c     =  1.0/SQRT(1+t*t);
                       s     =  t*c;
                       tau   =  s/(1.0+c);
                       h     =  t*a[ip][iq];
                       z[ip] -= h;
                       z[iq] += h;
                       d[ip] -= h;
                       d[iq] += h;
                       a[ip][iq]=0.0;

                       for(j=1; j<=ip-1; j++)
                          ROTATE(a,j,ip,j,iq)

                       for(j=ip+1; j<=iq-1; j++)
                          ROTATE(a,ip,j,j,iq)

                       for(j=iq+1; j<=n; j++)
                           ROTATE(a,ip,j,iq,j)

                       for(j=1; j<=n; j++)
                           ROTATE(v,j,ip,j,iq)

                       ++(*nrot);
                    }
                 }
              }

              for(ip=1; ip<=n; ip++)
              {  b[ip] += z[ip];
                 d[ip] =  b[ip];
                 z[ip] =  0.0;
              }
	}

        pups_set_errno(E2BIG);
        return(-1);
}
#undef ROTATE




/*------------------------------------------------------------------------------------------
     Sort the eigenvalues and eigenvectors calculated by jacobi or tred2 and tqli into
     descending order ...
------------------------------------------------------------------------------------------*/

_PUBLIC void eigsrt(FTYPE *d, FTYPE **v, int n)
{   int k,
        j,
        i;

    FTYPE p;

    for(i=1; i<n; i++)
    {  p = d[k=i];

       for(j=i+1; j<=n; j++)
          if(d[j] >= p)
             p=d[k=j];

       if(k != 1) 
       {  d[k] = d[i];
          d[i] = p;

          for(j=1; j<=n; j++)
          {  p       = v[j][i];
             v[j][i] = v[j][k];
             v[j][k] = p;
          }
      }
    }
}




/*---------------------------------------------------------------------------
    Generate ATA matrix from input pattern vectors ...
---------------------------------------------------------------------------*/

_PUBLIC void get_covariance_matrix(int n_patterns, int pattern_size, FTYPE **cmatrix, FTYPE **pattern_matrix)

{   int i,
        j,
        k;


    for(i=0; i<n_patterns; i++)
    {  for(j=i; j<n_patterns; j++)
       {  cmatrix[i+1][j+1] = 0.0;

          for(k=0; k<pattern_size; k++)
              cmatrix[i+1][j+1] += pattern_matrix[i][k] * pattern_matrix[j][k];

          cmatrix[j+1][i+1] = cmatrix[i+1][j+1];
       }
    }
}




/*---------------------------------------------------------------------------
    Generate eigenvectors ...
---------------------------------------------------------------------------*/

_PUBLIC int get_eigenvectors(FTYPE significance, int n_patterns, FTYPE **cmatrix, FTYPE *d, FTYPE *e)

{   int i,
        nse = 0;

    FTYPE trace;

    tred2(TRUE, cmatrix, n_patterns, d, e );

    if(tqli( d, e, n_patterns, cmatrix) == (-1))
       return(-1);

    eigsrt(d, cmatrix, n_patterns);

/*---------------------------------------------------------------------------
    Calculate trace (simply the sum of the eigenvalues of Cov_matrix) and the
    proportion of variation that each eigenvector accounts for ...
---------------------------------------------------------------------------*/

    trace = 0;
    for(i=1; i<=n_patterns; i++)
        trace += d[i];

    nse = 0;
    for(i=1; i<=n_patterns; i++)
       if((d[i]/trace) * 100.0 >= significance)
          ++nse;

    pups_set_errno(OK);
    return(nse);
}




/*---------------------------------------------------------------------------
    Generate eigenpatterns ...
---------------------------------------------------------------------------*/
  
_PUBLIC FTYPE **get_eigenpatterns(int nse, int n_patterns, int pattern_size, FTYPE **pattern_matrix, FTYPE **eigenvector)

{   int i,
        j,
        k;

    FTYPE **eigenpattern = (FTYPE **)NULL;

    if((eigenpattern = (FTYPE **)pups_calloc(n_patterns,sizeof(FTYPE *))) == (FTYPE **)NULL)
       return((FTYPE **)NULL);

    for(i=0; i<nse; i++)
    {  if((eigenpattern[i] = (FTYPE *)pups_calloc(pattern_size,sizeof(FTYPE))) == (FTYPE *)NULL)
       {  for(j=0; j<i; ++j)
             (void)pups_free((void *)eigenpattern[j]);

          (void)pups_free((void *)eigenpattern);

          pups_set_errno(E2BIG);
          return((FTYPE **)NULL);
       }
    }

    for(i=0; i<pattern_size; i++)
    {   for(j=0; j<nse; j++)
        {   eigenpattern[j][i] = 0.0;
            for(k=0; k<n_patterns; k++)
               eigenpattern[j][i] += pattern_matrix[k][i] * eigenvector[k+1][j+1];
        }
    }

    pups_set_errno(OK); 
    return(eigenpattern);
}





/*-----------------------------------------------------------------------------
    Generate mapping in eigenspace for given patten vector ...
-----------------------------------------------------------------------------*/

_PUBLIC FTYPE *generate_weight_vector(int nse, int pattern_size, FTYPE *pattern, FTYPE **eigenpattern)

{   int i,
        j;

    FTYPE *weight = (FTYPE *)NULL;

    weight = (FTYPE *)pups_calloc(nse,sizeof(FTYPE));
    for(i=0; i<nse; ++i)
    {  weight[i] = 0.0;

       for(j=0; j<pattern_size; ++j)
          weight[i] += pattern[j] *  eigenpattern[i][j];

    }

    return(weight);
}
