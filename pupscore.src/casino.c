/*------------------------------------------------------------------------------
    Purpose: Portable random number generator library. This version has bug in
             ran3() routine corrected.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 1.16
    Dated:   4th January 2023
    E-Mail:  mao@tumblingdice.co.uk
------------------------------------------------------------------------------*/

#include <me.h>
#include <utils.h>
#include <errno.h>
#include <math.h>

#undef   __NOT_LIB_SOURCE__
#include <casino.h>
#define  __NOT_LIB_SOURCE__


_PRIVATE void  avevar(FTYPE [], int, FTYPE *, FTYPE *);
_PRIVATE FTYPE betacf(FTYPE, FTYPE, FTYPE x);
_PRIVATE FTYPE betai(FTYPE, FTYPE, FTYPE);
_PRIVATE void  gser(FTYPE *, FTYPE a, FTYPE, FTYPE *);
_PRIVATE void  gcf(FTYPE *, FTYPE, FTYPE, FTYPE *);
_PRIVATE FTYPE gammq(FTYPE, FTYPE);
_PRIVATE FTYPE factln(int);


/*------------------------------------------------------------------------------
    Slot and usage functions - used by slot manager ...
------------------------------------------------------------------------------*/


/*---------------------*/
/* Slot usage function */
/*---------------------*/

_PRIVATE void casino_slot(int level)
{   (void)fprintf(stderr,"lib casino %s: [ANSI C]\n",CASINO_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(C) 1985-2023 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"PUPS/P3 random number generator library (built %s %s)\n\n",__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");
    (void)fflush(stderr);
}




/*-------------------------------------------*/
/* Segment identification for casino library */
/*-------------------------------------------*/

#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT )() __attribute__ ((aligned(16))) = casino_slot;
#endif /* SLOT */


/*--------------------*/
/* Defines for 'ran1' */
/*--------------------*/

_PRIVATE _CONST int    M1  = 259200;
_PRIVATE _CONST int    IA1 = 7141;
_PRIVATE _CONST int    IC1 = 54773;
_PRIVATE _CONST int    M2  = 134456;
_PRIVATE _CONST int    IA2 = 8121;
_PRIVATE _CONST int    IC2 = 28411;
_PRIVATE _CONST int    M3  = 243000;
_PRIVATE _CONST int    IA3 = 4561;
_PRIVATE _CONST int    IC3 = 51349;
_PRIVATE _CONST FTYPE  RM1 = 3.85802469e-6;
_PRIVATE _CONST FTYPE  RM2 = 7.437377283e-6;


/*--------------------*.
/* Defines for 'ran2' */
/*--------------------*/

_PRIVATE _CONST int M  = 714025;
_PRIVATE _CONST int IA = 1366;
_PRIVATE _CONST int IC = 150889;


/*--------------------*/
/* Defines for 'ran3' */
/*--------------------*/

_PRIVATE _CONST int MBIG   = 1000000000;
_PRIVATE _CONST int MSEED  = 161803398;
_PRIVATE _CONST int MZ     = 0;

_PRIVATE _CONST FTYPE FAC  = 1.0e-9;


/*-------------------------------------*/
/* Public global variable declarations */
/*-------------------------------------*/

_PUBLIC long r_init;


/*---------------------------------------------*/
/* Functions which are private to this library */
/*---------------------------------------------*/

// Log of Gamma function 
_PROTOTYPE _PRIVATE FTYPE gammln(FTYPE);


// Incomplete beta function
_PROTOTYPE _PRIVATE FTYPE betai(FTYPE, FTYPE, FTYPE);


/*------------------------------------------------------------------------------
    Portable random number generator based on the 'ran1' uniform deviate
    generator described in "Numerical Recipes in C" ...
------------------------------------------------------------------------------*/

_PUBLIC FTYPE ran1(void)

{

    /*------------------------------------------------------------------*/
    /* Returns a uniform deviate between 0.0 and 1.0. Set r_init to any */
    /* negative value to reinitialise the sequence                      */
    /*------------------------------------------------------------------*/

    _IMMORTAL long ix1,
                   ix2,
                   ix3;

    _IMMORTAL FTYPE r[98] = { 0.0 };
    FTYPE     temp;

    _IMMORTAL int iff = 0;
    int           j;


    /*------------------------------------------------*/
    /* initialise on first call, even if not negative */
    /*------------------------------------------------*/

    if(r_init < 0 || iff == 0)
    {  iff = 1;


       /*------------------------*/
       /* Seed the first routine */
       /*------------------------*/

       ix1 = (IC1 - r_init)  % M1;
       ix1 = (IA1*ix1 + IC1) % M1;


       /*----------------------------*/
       /* Use ix1 to seed the second */
       /*----------------------------*/

       ix2 = ix1 % M2;
       ix1 = (IA1*ix1 + IC1) % M1;


       /*-----------------------------------*/
       /* Use ix1 to seed the third routine */
       /*-----------------------------------*/

       ix3 = ix1 % M3;


       /*------------------------------------------------------------------*/
       /* Fill the table with sequential uniform deviates generated by the */
       /* first two routines                                               */
       /*------------------------------------------------------------------*/

       for(j=1; j<=97; j++)
       {  ix1 = (IA1*ix1+IC1) % M1;
          ix2 = (IA2*ix2+IC2) % M2;


          /*-----------------------------------------*/
          /* Low and high order peices combined here */
          /*-----------------------------------------*/

          r[j] = (ix1 + ix2*RM2) * RM1;
       }
       r_init = 1;
    }


    /*-------------------------------------------------------------*/
    /* Except when initialising, generate the next number for each */
    /* sequence                                                    */
    /*-------------------------------------------------------------*/

    ix1 = (IA1*ix1+IC1) % M1;
    ix2 = (IA2*ix2+IC2) % M2;
    ix3 = (IA3*ix3+IC3) % M3;


    /*-------------------------------------------------------*/
    /* Use this sequence to get a number between 0.0 and 1.0 */
    /*-------------------------------------------------------*/

    j = 1 + ((97*ix3) / M3);
    if(j > 97 || j < 1)
       pups_error("[ran1] generator error");


    /*---------------------*/
    /*  Return table entry */
    /*---------------------*/

    temp = r[j];


    /*---------------*/
    /* And refill it */
    /*---------------*/

    r[j] = (ix1+ix2*RM2)*RM1;
    return((FTYPE)temp);
}




/*------------------------------------------------------------------------------
    Random number generator based on the 'ran2' deviate generator
    described in "Numerical Recipes in C" ...
------------------------------------------------------------------------------*/

_PUBLIC FTYPE ran2(void)

{

     /*------------------------------------------------------------------*/
     /* Returns a uniform deviate between 0.0 and 1.0. Set ium to be any */
     /* negative value to initialise or reintialise the sequence         */
     /* -----------------------------------------------------------------*/

     _IMMORTAL long iy,
                    ir[98] = { 0L };

     _IMMORTAL int  iff = 0;

     int j;

     if(r_init < 0 || iff == 0)
     {  iff = 1;
        if((r_init = (IC-r_init) % M) < 0)  r_init = -(r_init);


        /*------------------------------*/
        /* Initialise the shuffle table */
        /*------------------------------*/

        for(j=1; j<=97; j++)
        {  r_init = (IA*r_init + IC) % M;
           ir[j] = (r_init);
        }
        r_init = (IA*r_init + IC) % M;
        iy = r_init;
     }


     /*-------------------------------------------------*/
     /* Start of routine if it is not being initialised */
     /*-------------------------------------------------*/

     j = 1 + 97.0*iy / M;
     if(j > 97 || j < 1)
        pups_error("[ran2] generator error");
 
     iy    = ir[j];
     r_init = (IA*r_init + IC) % M;
     ir[j] = r_init;

     return(( FTYPE)iy / M);
}




/*------------------------------------------------------------------------------
    Random number generator based upon the random deviate generator
    'ran3' described in "Numerical Recipes in C", which is in turn
    based on the portable random deviate generator proposed by Knuth ...
------------------------------------------------------------------------------*/

_PUBLIC FTYPE ran3(void)

{

    /*---------------------------------------------------------------------*/
    /* Returns a random deviate in the range 0.0 to 1.0. Set r_init to any */
    /* negative value to intialise or reinitialise the sequence            */
    /*---------------------------------------------------------------------*/

    _IMMORTAL int inext,
                  inextp;


    /*----------------------------------------------*/
    /* This value should not be modified, see Knuth */
    /*----------------------------------------------*/

    _IMMORTAL long ma[56] = { 0L };
    _IMMORTAL int iff     = 0;

    long mj,
         mk;

    int i,
        ii,
         k;


    /*----------------*/
    /* Initialisation */
    /*----------------*/

    if(r_init < 0 || iff == 0)
    {  iff = 1;


       /*--------------------------------------------------------------------*/
       /* Initialize ma[56] using the seed r_init and the large number MSEED */
       /*--------------------------------------------------------------------*/

       mj     =  MSEED - ((int)r_init < 0 ? -(int)r_init: (int)r_init);
       mj     %= MBIG;
       ma[55] =  mj;
       mk     =  1;


       /*------------------------------------------------------------------*/
       /* Now initialise the rest of the table in a slightly random order, */
       /* using numbers which are not especially random                    */
       /*------------------------------------------------------------------*/

       for(i=1; i<=54; i++)
       {  ii     = (21*i) % 55;
          ma[ii] = mk;
          mk     = mj - mk;
          if(mk < MZ) mk += MBIG;
          mj = ma[ii];
       }


       /*-----------------------*/
       /* Warm up the generator */
       /*-----------------------*/

       for(k=1; k<=4; k++)
       {  for(i=1; i<=55; i++)
          {  ma[i] -= ma[1+(i+30) % 55];
             if(ma[i] < MZ)
                ma[i] += MBIG;
          }
       }


       /*----------------------------------------------------------------*/
       /* Prepare indices for first generated number, the constant 31 is */
       /* special, see Knuth                                             */
       /*----------------------------------------------------------------*/

       inext  = 0;
       inextp = 31;
       r_init  = 1;
    }

    if(++inext == 56)  inext  = 1;
    if(++inextp == 56) inextp = 1;

    /*--------------------------------------------------------------------*/
    /* Here is where we start, except on initialisation. Increment inext, */
    /* wrapping around 56 to 1                                            */
    /*--------------------------------------------------------------------*/

                                     /*---------------------------------------*/
    mj = ma[inext] - ma[inextp];     /* Genrate new number subtractively      */
    if(mj < MZ)                      /* Check it is in range                  */
                                     /*---------------------------------------*/
       mj += MBIG;

                                     /*---------------------------------------*/
    ma[inext] = mj;                  /* Store it                              */
    return((FTYPE)(mj*FAC));         /* Output derived uniform deviate        */
                                     /*---------------------------------------*/
}




/*-----------------------------------------------------------------------------*/
/* Long period random number generator of L'Ecuyer with Bays-Durham shuffle    */
/* as added safeguards. Returns a uniform random deviate between 0.0 and 1.0.  */
/* Call with idum as a negative integer to initialise; thereafter do not       */
/* alter idum between calls. RNMX should aproximate the largest floating point */
/* value which is less than one                                                */
/*-----------------------------------------------------------------------------*/

#define LIM1  2147483563
#define LIM2  2147483399
#define LAM   (1.0/LIM1)
#define LIMM1 (LIM1 - 1)
#define LIA1  40014
#define LIA2  40692
#define LIQ1  53688
#define LIQ2  52774
#define LIR1  12211
#define LIR2  3791
#define LNTAB 32
#define LNDIV (1+LIM1/LNTAB)
#define LEPS  1.2e-7
#define LRNMX (1.0 - LEPS)


_PUBLIC FTYPE ran4(long *idum)

{   int  j;
    long k;

    _IMMORTAL long idum2     = 123456789;
    _IMMORTAL long iy        = 0L;
    _IMMORTAL long iv[LNTAB] = { 0L };

    FTYPE temp;


    /*---------------------------------------*/
    /* Initialise if *idum is less than zero */
    /*---------------------------------------*/

    if(*idum <= 0)
    {  

       /*-----------------*/
       /* Avoid *idum = 0 */
       /*-----------------*/

       if(-(*idum) < 1)
          *idum = 1;
       else
         *idum = -(*idum);

       idum2 = (*idum);

       /*--------------------------------------------*/
       /*  Load the shuffle table (after 8 warm-ups) */
       /*--------------------------------------------*/

       for(j=LNTAB+7; j>=0; j--)
       {   k     = (*idum) / LIQ1;
           *idum = LIA1*(*idum - k*LIQ1) - k*LIR1;

           if(*idum < 0)
              *idum += LIM1;

           if(j < LNTAB)
              iv[j] = *idum;
       }

       iy = iv[0];
     }


     /*-----------------------------------------------*/
     /* This is the entry point when not initialising */
     /*-----------------------------------------------*/

      k = (*idum)/LIQ1;


      /*----------------------------------------------------------------------*/
      /* Compute *idum = (IA1*(*idum)) % IM1 without overflows using Schrages */
      /* method                                                               */
      /*----------------------------------------------------------------------*/

      *idum = LIA1*(*idum - k*LIQ1) - k*LIR1;

      if(*idum < 0)
         *idum += LIM1;

      k = idum2/LIQ2;


      /*-------------------------------------------------*/
      /*  Compute idum2 = (IA2 * (*idum)) % IM2 likewise */
      /*-------------------------------------------------*/

      idum2 = LIA2*(idum2 - k*LIQ2) - k*LIR2;

      if(idum2 < 0)
         idum2 += LIM2;


      /*--------------------------------------------------------------*/
      /* Shuffle *idum and combine *idum and idum2 to generate output */
      /*--------------------------------------------------------------*/

      j     = iy/LNDIV;
      iy    = iv[j] - idum2;
      iv[j] = *idum;

      if(iy < 1)
         iy += LIMM1;

      if((temp =LAM*iy) > LRNMX)
         return(LRNMX);
      else
         return(temp);
}



/*------------------------------------------------------------------------*/
/* Minimal random number generator (Park-Miller) with Bays-Durham shuffle */
/* and added safeguards. Returns randome deviate between 0.0 and 1.0      */
/*------------------------------------------------------------------------*/

#define IA    16807
#define IM    2147483647
#define AM    (1.0 / IM)
#define IQ    127773
#define IR    2386
#define NTAB  32
#define NDIV  (1 + (IM - 1)/NTAB)
#define EPS   1.2e-7
#define RNMX  (1.0 - EPS)


_PROTOTYPE _PUBLIC FTYPE ran5(long *idum)

{    int  j;
     long k;

     _IMMORTAL long iy = 0,
                    iv[NTAB];

     FTYPE temp;


     /*-----------------------*/
     /* Initilisation section */
     /*-----------------------*/

     if(*idum <= 0.0 || !iy)
     {  if(-(*idum) < 1)
           *idum = 1;
        else
           *idum = -(*idum);


        /*--------------------------------------*/
        /* load shuffle table (afer 8 warm ups) */
        /*--------------------------------------*/

        for(j=NTAB+7; j>=0; j--)
        {  k = (*idum)/IQ;
           *idum = IA * (*idum - k*IQ) - IR*k;

           if(*idum < 0)
              *idum += IM;

           if(j < NTAB)
              iv[j] = *idum;
        }
        iy = iv[0];
     }


     /*----------------------------------*/
     /* Start here when not initialising */
     /*----------------------------------*/

     k     = (*idum)/IQ;


     /*--------------------------------------------------------------------*/
     /* Compute idum=(IA*idum) % IM without overflow (via Schrages method) */
     /*--------------------------------------------------------------------*/

     *idum = IA*(*idum - k*IQ) - IR*k;

     if(*idum < 0)
        *idum += IM;

     j = iy/NDIV;
     iy = iv[j];
     iv[j] = *idum;

     if((temp = AM*iy) > RNMX)
        return(RNMX);
     else
        return(temp);
}




/*------------------------------------------------------------------------------
    Return a normally distributed deviate with zero mean, and unit
    variance using user defined random number generator ...
------------------------------------------------------------------------------*/

_PUBLIC FTYPE gasdev(FTYPE (* ran)(void), FTYPE variance)

{   _IMMORTAL int iset = 0;
    _IMMORTAL FTYPE  gset;

    FTYPE fac,
          r,
          v1,
          v2;

    if(iset == 0)
    {  do {

              /*--------------------------------------------------------------------*/
              /*  Pick two uniform numbers in the square extending from -1 to +1 in */
              /*  each direction                                                    */
              /*--------------------------------------------------------------------*/

              v1 = 2.0*(*ran)() - 1.0;
              v2 = 2.0*(*ran)() - 1.0;

              r = v1*v1 + v2*v2;
          } while(r >= 1.0);
          fac = SQRT(-2.0*LOG(r)/r);


         /*----------------------------------------------------------------------*/
         /* Now make the Box-Muller transform to get two normal deviates. Return */
         /* one and save the other for next time                                 */
         /*----------------------------------------------------------------------*/

          gset = v1*fac;
          iset = 1;        // Set flag 

          return((FTYPE)(v2*fac*SQRT(variance)));
     }
     else
     {  iset = 0;
        return((FTYPE)gset*SQRT(variance));
     }
}




/*------------------------------------------------------------------------------
    Return Gaussian distribution using the central limit theorem ...
------------------------------------------------------------------------------*/

_PUBLIC FTYPE gasdev2(FTYPE (*ran)(void) , FTYPE variance)

{   int    i = 5;
    FTYPE ret;


    /*----------------------------------------------------------------------*/
    /* Generate deviate (unit variance, centred at zero), using the central */
    /* limit theorem, with n == 5                                           */
    /*----------------------------------------------------------------------*/

    ret = 0.0;
    for(i=0; i<5; ++i)
       ret += (*ran)();

    ret = (ret - 2.50)*SQRT(12.0*variance)/5.0;
    return(ret);
}





/*------------------------------------------------------------------------------
    Return the log of the gamma function ...
------------------------------------------------------------------------------*/

_PRIVATE FTYPE gammln(FTYPE xx)

{

    /*------------------------------------------------------------------------*/
    /* Returns the log of the gamma function for xx > 0. Full accuracy is     */
    /* obtained if xx > 1. For 0 < xx < 1, the refelction formula can be used */
    /* first                                                                  */
    /*------------------------------------------------------------------------*/

    FTYPE x,
          tmp,
          ser;

    _IMMORTAL FTYPE cof[6] = {  76.18009173,
                                -86.50532033,
                                 24.01409822,
                                -1.231739516,
                                 0.120858003e-2,
                                 -0.536382e-5
                             };

    int j;


    /*---------------------------------------------------------------------*/
    /* Internal arithmetic will be done in FTYPE precision, a nicety which */
    /* can be omitted, if five figure accuracy is sufficient               */
    /*---------------------------------------------------------------------*/

    x    = xx - 1.0;
    tmp  =  x + 5.5;
    tmp -= (x+0.5)*LOG(tmp);
    ser  = 1.0;

    for(j=0; j<=5; j++)
    {  x   += 1.0;
       ser =  cof[j]/x;
    }

    return((FTYPE)((-tmp) + LOG(2.50662827465)*ser));
}




/*------------------------------------------------------------------------------
    Return N! as a floating point number ...
------------------------------------------------------------------------------*/

_PUBLIC FTYPE factrl(int n)

{   _IMMORTAL int ntop = 4;
    _IMMORTAL FTYPE a[33] = {1.0, 1.0, 2.0, 6.0, 24.0};

    int j;

    pups_set_errno(OK);
    if(n < 0)
    {  pups_set_errno(EDOM);
       return(-1);
    }

    if(n > 32)
      return(EXP(gammln(n+1.0)));


    /*----------------------------*/
    /* This may cause an overflow */
    /*----------------------------*/

    while(ntop < n)
    {    j = ntop++;
         a[ntop] = a[j]*ntop;
    }

    return(a[n]);
}




/*--------------------------------------------------------------------------------
    Returns ln(n!) ...
--------------------------------------------------------------------------------*/

_PRIVATE FTYPE factln(int n)

{   _IMMORTAL FTYPE a[101];

    pups_set_errno(OK);
    if(n < 0)
    {  pups_set_errno(EDOM);
       return(-1.0);
    }

    if(n <= 1)
       return(0.0);

    if(n <= 100)
       return( a[n] ? a[n] : (a[n] = gammln(n+1.0)) );
    else
       return(gammln(n+1.0));
}




/*--------------------------------------------------------------------------------
    Return binomial coefficient as a floating point number ...
--------------------------------------------------------------------------------*/

_PUBLIC FTYPE bico(int n, int k)

{   return(floor(0.5 + EXP(factln(n) - factln(k) - factln(n-k))));
}




/*------------------------------------------------------------------------------
    Generate gamma distribution ...
------------------------------------------------------------------------------*/

_PUBLIC FTYPE gamdev(int ia, FTYPE (*ran)(void))

{

    /*-----------------------------------------------------------------------*/
    /* Return a deviate distributed as a gamma distribution of integer order */
    /* ia, i.e. a waiting time to the iath event in a Poisson process of     */
    /* unit mean, using the function pointed to by 'ran' as the source of    */
    /* uniform deviates                                                      */
    /*-----------------------------------------------------------------------*/

    int j;

    FTYPE am,
          e,
          s,
          v1,
          v2,
          x,
          y;

    if(ia < 1)
       pups_error("[gamdev] incorrect parameter");


    /*----------------------------------------*/
    /* Use direct method adding waiting times */
    /*----------------------------------------*/

    if(ia < 6)
    {  x = 1.0;
       for(j=1; j<=ia; j++)
          x *= (*ran)();
       x -= LOG(x);
    }
    else
    {

       /*----------------------*/
       /* Use rejection method */
       /*----------------------*/

       do {
              do {

                      /*-------------------------------------------------------------------*/
                      /*  These four lines generate the tangent of a random angle i.e. are */
                      /*  equivalent to y = tan(PI * ran())                                */
                      /*-------------------------------------------------------------------*/

                      do {   v1 = 2.0*(*ran)() - 1.0;
                             v2 = 2.0*(*ran)() - 1.0;
                         } while(v1*v1*v2*v2 > 1.0);

                      y  = v2 / v1;
                      am = ia - 1;
                      s = SQRT(2.0*am + 1.0);


                      /*----------------------------*/
                      /* decide whether to reject x */
                      /*----------------------------*/

                      x = s*y + am;

                                     /*---------------------------------------*/
                  } while(x <= 0.0); /* Reject in region of zero probability  */
                                     /*---------------------------------------*/

              /*-------------------------------------------*/
              /* Ratio of probability fn. to comparison fn */
              /*-------------------------------------------*/

              e = (1.0 + y*y) * EXP(am*LOG(x/am) - s*y);
          } while((*ran)() > e);
    }

    return((FTYPE)x);
}




/*------------------------------------------------------------------------------
    Possion deviate distribution ...
------------------------------------------------------------------------------*/

_PUBLIC FTYPE poidev(FTYPE xm, FTYPE (*ran)(void))

{

    /*--------------------------------------------------------------------*/
    /* Returns as floating point number an integer value that is a random */
    /* deviate drawn from a Poisson distribution of mean xm, using the    */
    /* random deviate generator pointed to by 'ran'                       */
    /*--------------------------------------------------------------------*/

    _IMMORTAL FTYPE sq,
                    alxm,
                    g,
                    oldm = (-1.0);

    FTYPE em,
          t,
          y;


    /*-------------------*/
    /* Use direct method */
    /*-------------------*/

    if(xm < 12.0)
    {

       /*---------------------------------------*/
       /* If xm is new, compute the exponential */
       /*---------------------------------------*/

       if(xm != oldm)
       {  oldm = xm;
          g    = EXP(-xm);
       }

       em = -1;
       t  =  1.0;
       do {

              /*----------------------------------------------------------------------*/
              /* Instead of adding exponential deviates, it is equivalent to multiply */
              /* before uniform deviates. The we nver have to take the log, merely    */
              /* compare the pre-computed exponential                                 */
              /*----------------------------------------------------------------------*/

              em += 1.0;
              t  *= (*ran)();
          } while(t > g);
    }
    else
    {

       /*-------------------------------------------------------------------*/
       /* Use rejection method. If xm has changed since the last call, then */
       /* precompute some functions which occur below                       */
       /*-------------------------------------------------------------------*/

       if(xm != oldm)
       {  oldm = xm;
          sq   = SQRT(2.0*xm);
          alxm = LOG(xm);


          /*--------------------------------------------------------------*/
          /* The function gammln is the natural log of the gamma function */
          /*--------------------------------------------------------------*/

          g = xm*alxm - gammln(xm + 1.0);
       }

       do {
              do {

                     /*---------------------------------------------------------------------*/
                     /* Y is a deviate from a Lorentzian comparison fn, em is y shifted and */
                     /* scaled                                                              */
                     /*---------------------------------------------------------------------*/

                      y = TAN(PI*(*ran)());
                      em = sq*y*xm;


                      /*-----------------------------------------*/
                      /* Reject if in regime of zero probability */
                      /*-----------------------------------------*/

                 } while(em < 0.0);


                 /*--------------------------------------------*/
                 /*  The trick for integer based distributions */
                 /*--------------------------------------------*/

                 em = floor(em);
                 t  = 0.9*(1.0 + y*y)*EXP(em*alxm - gammln(em+1.0) - g);


                 /*-------------------------------------------------------------------*/
                 /* The ratio of the desired distribution to the comparison function; */
                 /* We accept or reject by comparing it to another uniform deviate.   */
                 /* The factor 0.9 is chosen so that t never exceeds 1                */
                 /*-------------------------------------------------------------------*/

             } while((*ran)() > t);
    }

    return((FTYPE)em);
}



/*------------------------------------------------------------------------------
    Binomial distribution ...
------------------------------------------------------------------------------*/

_PUBLIC FTYPE bnldev(FTYPE pp, int n, FTYPE (*ran)())

{

    /*-------------------------------------------------------------------------*/
    /* Return as a FTYPE precision floating point number, an integer value     */
    /* that is a random deviate drawn from a binomial distribution of n trials */
    /* each of probability pp, using 'ran' as a source of uniform random       */
    /* deviates                                                                */
    /*-------------------------------------------------------------------------*/

    int j;

    _IMMORTAL int nold = (-1);

    FTYPE am,
          em,
          g,
          angle,
          p,
          bnl,
          sq,
          t,
          y;

    _IMMORTAL FTYPE pold = (-1.0),
                    pc,
                    plog,
                    pclog,
                    en,
                    oldg;

    p = (pp <= 0.5 ? pp : 1.0 - pp);


    /*------------------------------------------------------------------------*/
    /* The binomial distribution is invariant under chnaging pp to 1.-pp, if  */
    /* we also change the answer to a minus itself, we'll remember to do this */
    /* below. 'am' is the mean of the deviate to be produced                  */
    /* -----------------------------------------------------------------------*/

    am = n*p;


    /*---------------------------------------------------------------------*/
    /* Use the direct method while n is not too large, this can require up */
    /* to 25 calls to the random deviate generator                         */ 
    /*---------------------------------------------------------------------*/

    if(n < 25)
    {  bnl = 0.0;
       for(j=1; j<=n; j++)
          if((*ran)() < p)
            bnl += 1.0;
    }
    else
    {  if(am < 1.0)
       {

          /*-----------------------------------------------------------------------*/
          /* If fewer than one event is expected out of 25 or more trials, the the */
          /* distribution is quite accurately Poisson, theefore use direct Poisson */
          /* method                                                                */
          /*-----------------------------------------------------------------------*/

          g = EXP(-am);
          t = 1.0;
          for(j=0; j<=n; j++)
          {  t *= (*ran)();
             if(t < g) break;
          }
          bnl = (j <= n ? j : n);
       }
       else
       {

          /*----------------------------------------------------------------*/
          /* Use the rejection method. If n has changed then compute useful */
          /* quantities                                                     */
          /*----------------------------------------------------------------*/

          if(n != nold)
          {  en   = n;
             oldg = gammln(en+1.0);
             nold = n;
          }


          /*---------------------------------------------*/
          /* If p has changed, compute useful quantities */
          /*---------------------------------------------*/

          if(p != pold)
          {  pc    = 1.0 - p;
             plog  = LOG(p);
             pclog = LOG(pc);
             pold  = p;
          }


          /*------------------------------------------------------------------*/
          /* The following code implements a rejection method with Lorentzian */
          /* comparision                                                      */
          /*------------------------------------------------------------------*/

          sq = SQRT(2.0*am*pc);
          do {
                 do {   angle = PI*(*ran)();
                        y     = TAN(angle);
                        em    = sq*y+am;
                    } while(em < 0.0 || em >= (en+1.0));           // Reject 


                                    /*---------------------------------------*/
                 em = floor(em);    /* Trick for integer valued distribution */
                                    /*---------------------------------------*/

                 t = 1.2*sq*(1.0 + y*y)*EXP(oldg - gammln(em+1.0) -
                            gammln(en-em+1.0) + em*plog + (en-em)*pclog);


                 /*------------------------------------------------------*/
                 /* Reject, this occurs 1.5 times per deviate on average */
                 /*------------------------------------------------------*/

             } while((*ran)() > t);

          bnl = em;
       }
    }


    /*--------------------------------------------*/
    /* Remember to use the symmtry transformation */
    /*--------------------------------------------*/

    if(p != pp)
       bnl = n - bnl;

    return((FTYPE)bnl);
}




/*------------------------------------------------------------------------------
    Generate a user defined distribution from a numerical function ...
------------------------------------------------------------------------------*/

_PUBLIC FTYPE numdev(FTYPE (*ran)(void) , gate_type *gate_f)

{   int i;
    FTYPE level;


    /*------------------------*/
    /* Generate trigger value */
    /*------------------------*/

    level = (*ran)()*gate_f->frame_size;


    /*----------------------------------------*/
    /* Find out which gate has been triggered */
    /*----------------------------------------*/

    for(i=0; i<gate_f->n_gates; ++i)
    {  if(level <= gate_f->gate_y[i])
           return(gate_f->gate_x[i]);
    }


    /*------------------------------------------------------*/
    /* This should not happen, but flag an error if it does */
    /*------------------------------------------------------*/

    pups_error("[numdev] generator error");


    /*---------------------------------------------*/
    /* Stop compiler from producing error messages */
    /*---------------------------------------------*/

    return(DDUM);
}




/*------------------------------------------------------------------------------
    Routine to compute the number of unique combinations of n objects
    taken from a set of m objects ...
------------------------------------------------------------------------------*/

_PUBLIC int n_from_m(int n, int m)

{   int i,
        ret         = 1,
        n_factorial = 1;


    /*---------------------*/
    /* compute n! / (n-i)! */
    /*---------------------*/

    for(i=m; i=m-n; --i)
        ret *= i;


    /*------------*/
    /* Compute n! */
    /*------------*/

    for(i=0; i<n; ++i)
       n_factorial *= i;

    ret /= n_factorial;
    return(ret);
}



/*-----------------------------------------------------------------------------
    Routine to gernerate the error function, erfcc ...
-----------------------------------------------------------------------------*/

_PRIVATE FTYPE erfcc(FTYPE x)

{   FTYPE t,
          z,
          ans;

    z   = fabs(x);
    t   = 1.0 / (1.0 + 0.5*z);
    ans = t*EXP(-z*z - 1.26551223 + t*(1.00002368 + t*
                       (0.37409196 + t*(0.09678418 + t*
                       (-0.18628806 + t*(0.27886807 + t*
                       (-1.13520398 + t*(1.48851587 + t*
                       (-0.82215223 + t*0.17087277)))))))));

    return(ans);
}



 
/*-----------------------------------------------------------------------------
    Crank routine used by Spearmans rank correlation routine ...
-----------------------------------------------------------------------------*/

_PRIVATE void crank(int n, FTYPE w[], FTYPE *s)

{   int j = 0,
           ji,
           jt;

    FTYPE t,
          rank;

    *s = 0.0;
    while(j < n - 1)
    {   if(w[j+1] != w[j])
        {  w[j] = j;
           ++j;
        }
        else
        {  for(jt=j; jt<n; ++jt)
              if(w[jt] != w[j])
                 break;

            rank = 0.5*(j + jt - 1);
            for(ji=j; ji<(jt - 1); ++ji)
               w[ji] = rank;

            t  =  jt - j;
            *s += t*t*t - t;
            j  =  jt;
         }
    }


    /*------------------------------------------*/
    /* Last element not tied - this is its rank */
    /*------------------------------------------*/

    if(j == (n - 1))
      w[n-1] = n;
}




/*-----------------------------------------------------------------------------
    Routine to perform non parametric rank correlation [Spearman] ...
-----------------------------------------------------------------------------*/

_PUBLIC void spear(FTYPE   data1[],      // Data array 1
                   FTYPE   data2[],      // Data array 2
                   int           n,      // Number of data elements
                   FTYPE        *d,      // Sum squared difference of ranks
                   FTYPE       *zd,      // Deviation from null hypothesis
                   FTYPE    *probd,      // Two sided significance
                   FTYPE       *rs,      // Spearmans's rank correlation
                   FTYPE   *probrs)      // Significance of rs dev from zero

{   int j;

    FTYPE vard,
          t,
          sg     = 0.0,
          sf     = 0.0,
          fac,
          en3n,
          en,
          df     = 0.0,
          aved,
          *wksp1 = (FTYPE *)NULL,
          *wksp2 = (FTYPE *)NULL;

    wksp1 = (FTYPE *)pups_calloc(n,sizeof(FTYPE));
    wksp2 = (FTYPE *)pups_calloc(n,sizeof(FTYPE));

    for(j=0; j<n; ++j)
    {   wksp1[j] = data1[j];
        wksp2[j] = data2[j];
    }


    /*---------------------------------------------------------------*/
    /* Sort each of the data arrays and convert the entries to ranks */
    /*---------------------------------------------------------------*/

    sort2(n,wksp1,wksp2);
    crank(n,wksp1,wksp2);

    sort2(n,wksp1,wksp2);
    crank(n,wksp1,wksp2);

    *d = 0.0;


    /*-------------------------------------*/
    /* Sum the squared difference of ranks */
    /*-------------------------------------*/

    for(j=0; j<n; ++j)
       *d += sqr(wksp1[j] - wksp2[j]);


    /*---------------------*/
    /* Expectation value D */
    /*---------------------*/

    en   = n;
    en3n = en*en*en - en;
    aved = en3n / 6.0 - (sf+sg) / 12.0;
    fac    = (1.0 - sf/en3n)*(1.0 - sg/en3n);


    /*---------------*/
    /* Variance of D */
    /*---------------*/

    vard   = ((en - 1.0)*en*en*sqr(en + 1.0)/36.0)*fac;


    /*----------------------------------*/
    /* Number of significant deviations */
    /*----------------------------------*/

    *zd    = (*d - aved)/SQRT(vard);


    /*--------------*/
    /* Significance */
    /*--------------*/

    *probd = erfcc(fabs(*zd)/1.4142136);


    /*------------------------------------------*/
    /* Spearman's rank correlation coeffiecient */
    /*------------------------------------------*/

    *rs    = (1.0 - (6.0/en3n)*(*d + 0.5*(sf + sg)))/fac;


    /*----------------------------------------------------*/
    /* t value of Spearman's rank correlation coefficient */
    /*----------------------------------------------------*/

    t      = (*rs)*SQRT((en - 2.0)/((*rs + 1.0)*(1.0 - (*rs))));
    df     = en - 2.0;


    /*---------------------------------------------------------*/
    /* Significance of Spearman's rank correlation coefficient */
    /*---------------------------------------------------------*/

    *probrs = betai(0.5*df, 0.5, df / (df + t*t));

    pups_free((void *)wksp1);
    pups_free((void *)wksp2);
}




/*-----------------------------------------------------------------------------
    Routine to compute the incomplete beta functions ...
-----------------------------------------------------------------------------*/

_PRIVATE FTYPE betai(FTYPE a, FTYPE b, FTYPE x)

{   FTYPE bt;

    if(x < 0.0 || x > 1.0)
      pups_error("[betai] x outside range 0.0 - 1.0");

    if(x == 0.0 || x == 1.0)
       bt = 0.0;
    else
       bt = EXP(gammln(a+b) - gammln(a) - gammln(b) +
                              a*LOG(x) + b*LOG(1.0 - x));

    if(x < (a + 1.0) / (a + b + 2.0))
       return(bt*betacf(a,b,x) / a);
    else
       return(1.0 - bt*betacf(b,a,1.0 - x) / b);
}




/*-----------------------------------------------------------------------------
    Routine to determine the linear correlation between two sets of
    variables ...
-----------------------------------------------------------------------------*/

#define TINY 1.0e-20

_PUBLIC void pearsn(FTYPE     x[],      // Data array 1 
                    FTYPE     y[],      // Data array 2
                    int         n,      // Number of elements in data array
                    FTYPE      *r,      // Linear correlation coefficient
                    FTYPE   *prob,      // Significance of correlation
                    FTYPE      *z)      // Fisher's Z

{   int j;

    FTYPE yt,
          xt,
          t,
          df,
          syy = 0.0,
          sxy = 0.0,
          sxx = 0.0,
          ay  = 0.0,
          ax  = 0.0;


    /*----------------*/
    /* Find the means */
    /*----------------*/

    for(j=0; j<n; ++j)
    {  ax += x[j];
       ay += y[j];
    }

    ax /= n;
    ay /= n;


    /*----------------------------------*/
    /* Compute correlation coefficients */
    /*----------------------------------*/

    for(j=0; j<n; ++j)
    {  xt = x[j] - ax;
       yt = y[j] - ay;
       sxx += xt*xt;
       syy += yt*yt;
       sxy += xt*yt;
    }

    *r = sxy / SQRT(sxx*syy);


    /*----------------------*/
    /* Fisher's Z transform */
    /*----------------------*/

    *z = 0.5*LOG((1.0 + (*r) + TINY) / (1.0 - (*r) + TINY));

    df = n - 2;


    /*-------------------------*/
    /* Student's t probability */
    /*-------------------------*/

    t  = (*r)*SQRT(df / ((1.0 - (*r) + TINY) * (1.0 - (*r) + TINY)));
    *prob = betai(0.5*df, 0.5, df / (df*t*t));
}




/*------------------------------------------------------------------------------
    Return the mean, standard, deviation, variance, skewness and kurtosis of a
    distribution ...
------------------------------------------------------------------------------*/

_PUBLIC void moment(FTYPE data[],   // Data to be analysed
                    int        n,   // Number of pts in dataset
                    FTYPE   *ave,   // Average deviation
                    FTYPE   *rms,   // Root mean square error
                    FTYPE  *adev,   // Average deviation
                    FTYPE  *sdev,   // Standard deviation
                    FTYPE  *svar,   // Variance
                    FTYPE  *skew,   // Skewness
                    FTYPE  *curt)   // Kurtosis

{   int j;

    FTYPE s = 0.0,
           r = 0.0,
           p = 0.0;

    if(n <= 1)
    {  *ave  = data[0];
       *rms  = 0.0;
       *adev = 0.0;
       *sdev = 0.0;
       *svar = 0.0;
       *skew = 0.0;
       *curt = 0.0;

       return;
    }


    /*----------------------------*/
    /* First pass to get the mean */
    /*----------------------------*/

    for(j=0; j<n; ++j)
    {  s += data[j];
       r += data[j]*data[j];
    }


    /*---------------------------------------------------*/
    /* Second pass to get the higher statistical moments */
    /*---------------------------------------------------*/

    *ave  = s / n;
    *adev = (*svar) = (*skew) = (*curt) = 0.0;

    for(j=0; j<n; ++j)
    {   *adev += fabs(s=data[j] - (*ave));
        *svar += (p = s*s);
        *skew += (p *= s);
        *curt += (p *= s);
    }


    /*-------------------------------------------------*/
    /* Put the pieces together according to convention */
    /*-------------------------------------------------*/

    *rms  = SQRT(r / n);
    *adev /= n;
    *svar /= (n - 1);
    *sdev =  SQRT(*svar);

    if(*svar)
    {  *skew /= (n*(*svar)*(*sdev));
       *curt = (*curt) / (n*(*svar)*(*svar)) - 3.0;
    }
    else
    {  *skew = 0.0;
       *curt = 0.0;
    }
}




/*------------------------------------------------------------------------------
    Routine to test whether two distributions have the same mean. The
    result of the Student t test is returned as t, and its significance
    as prob. This routine is based on that given in Numerical Recipes in C
    p616 ...
------------------------------------------------------------------------------*/

_PUBLIC void ttest(FTYPE   data_1[],
                   int           n1,
                   FTYPE   data_2[],
                   int           n2,
                   FTYPE         *t,
                   FTYPE      *prob)


{   FTYPE df,
          svar,
          *ave_1 = (FTYPE *)NULL,
          *ave_2 = (FTYPE *)NULL,
          *var_1 = (FTYPE *)NULL,
          *var_2 = (FTYPE *)NULL;

    avevar(data_1,n1,ave_1,var_1);
    avevar(data_2,n2,ave_2,var_2);

    df = n1 + n2 - 2;

    svar  = ((n1 - 1)*(*var_1) + ((FTYPE)n2 - 1)*(*var_2)) / df;
    *t    = ((*ave_1) - (*ave_2)) / SQRT(svar*(1.0  / n1 + 1.0 / n2));
    *prob = betai(0.5*df, 0.5, df / (df + (*t)*(*t)));
}




/*------------------------------------------------------------------------------
    Find mean and variance of array ...
------------------------------------------------------------------------------*/

_PRIVATE void avevar(FTYPE data[], int n, FTYPE *ave, FTYPE *var)

{   int j;

    FTYPE s,
          ep;

    for(*ave=0.0,j=0; j<n; ++j)
       *ave += data[j];

    *ave /= n;
    *var = ep = 0.0;

    for(j=0; j<n; ++j)
    {  s    =  data[j] - (*ave);
       ep   += s;
       *var =  s*s;
    }
  
    *var = (*var - ep*ep/n) / (n - 1);
}




/*-----------------------------------------------------------------------------
    Student's t for significantly different means where the two populations
    to be compared have significantly different variances ...
-----------------------------------------------------------------------------*/

_PUBLIC void tutest(FTYPE data_1[],
                    int         n1,
                    FTYPE data_2[],
                    int         n2,
                    FTYPE       *t,
                    FTYPE    *prob)


{   FTYPE df     = 0.0,
          svar,
          fn_1,
          fn_2,
          *ave_1 = (FTYPE *)NULL,
          *ave_2 = (FTYPE *)NULL,
          *var_1 = (FTYPE *)NULL,
          *var_2 = (FTYPE *)NULL;

    avevar(data_1,n1,ave_1,var_1);
    avevar(data_2,n2,ave_2,var_2);

    fn_1 = (FTYPE)n1;
    fn_2 = (FTYPE)n2;

    svar  = ((fn_1 - 1)*(*var_1) + (fn_2 - 1)*(*var_2)) / df;
    *t    = ((*ave_1) - (*ave_2)) / SQRT(svar*(1.0 / fn_1 + 1.0 / fn_2));
    df    = sqr((*var_1)/fn_1 + (*var_2)/fn_2) / sqr(*var_1/fn_1)/(fn_1 - 1) +
                                                sqr(*var_2/fn_2) / (fn_2 - 1);

    *prob = betai(0.5*df, 0.5, df / (df + (*t)*(*t)));
}




/*------------------------------------------------------------------------------
    Compute chi-square and two measure of association for two different
    distributions, Cramer's V and the contingency coefficient ...
------------------------------------------------------------------------------*/

_PUBLIC void cntab1(int       **nn,
                    int         ni,
                    int         nj,
                    FTYPE   *chisq,
                    FTYPE      *df,
                    FTYPE    *prob,
                    FTYPE  *cramrv,
                    FTYPE     *ccc)
{   int nnj,
        nni,
        j,
        i,
        minij;

    FTYPE sum      = 0.0,
          expectd,
          *sumi    = (FTYPE *)NULL,
          *sumj    = (FTYPE *)NULL,
          temp;


    sumi = (FTYPE *)pups_calloc(ni,sizeof(FTYPE));
    sumj = (FTYPE *)pups_calloc(ni,sizeof(FTYPE));

    nni = ni;
    nnj = nj;

    for(i=0; i<ni; ++i)
    {  sumi[i] = 0.0;
       for(j=0; j<nj; ++j)
       {  sumi[i] += nn[i][j];
          sum     += nn[i][j];
       }


       /*------------------------------------------------*/
       /* Eliminate any zero rows by reducing the number */
       /*------------------------------------------------*/

       if(sumi[i] == 0.0)
          --nni;
    }


    /*-----------------------*/
    /* Get the column totals */
    /*-----------------------*/

    for(j=0; j<nj; ++j)
    {  sumj[j] = 0.0;

       for(i=0; i<ni; ++i)
          sumj[j] += nn[i][j];


       /*----------------------------*/
       /* Eliminate any zero columns */
       /*----------------------------*/

       if(sumj[j] = 0.0)
          --nnj;
    }


    /*----------------------------------------*/
    /* Corrected number of degrees of freedom */
    /*----------------------------------------*/

    *df    = nni*nnj - nni  - nnj + 1;
    *chisq = 0.0;


    /*-----------------------*/
    /* Do the chi-square sum */
    /*-----------------------*/

    for(i=0; i<ni; ++i)
    {  for(j=0; j<nj; ++j)
       {  expectd = sumj[j]*sumi[i] / sum;
          temp    = nn[i][j] - expectd;
          *chisq  += temp*temp / (expectd + TINY);
       }
    }


    /*---------------------------------*/
    /* Chi-square probability function */
    /*---------------------------------*/

    *prob   = gammq(0.5*(*df),0.5*(*chisq));
    minij   = nni < nnj ? nni - 1 : nnj - 1;
    *cramrv = SQRT(*chisq / (sum*minij));
    *ccc    = SQRT(*chisq / (*chisq + sum));

    (void)pups_free((void *)sumi);
    (void)pups_free((void *)sumj);
}




/*--------------------------------------------------------------*/
/* Definitions required by the beta and gamma functions package */
/*--------------------------------------------------------------*/

#define ITMAX 100
#define BEPS  3.0e-7
#define FPMIN 1.0e-30


/*------------------------------------------------------------------------------
    Evaluate the continued fraction for incomplete beta function using
    Lentz's method ...
------------------------------------------------------------------------------*/

_PRIVATE FTYPE betacf(FTYPE a, FTYPE b, FTYPE x)

{   int m,
        m2;

    FTYPE aa,
           c,
           d,
           del,
           h,
           qab,
           qam,
           qap;


    /*------------------------------------------------------------------*/
    /* These q's will be used in factors that occur in the coefficients */
    /*------------------------------------------------------------------*/

    qab = a + b;
    qap = a + 1.0;
    qam = a - 1.0;

    c   = 1.0;
    d   = 1.0 - qab*x / qap;

    if(fabs(d) <FPMIN)
       d = FPMIN;

    d = 1.0 / d;
    h = d;

    for(m=0; m<ITMAX; ++m)
    {  m2 = 2*m;
       aa = m*(b - m)*x / ((qam + m2)*(a + m2));


       /*---------------------------------*/
       /* The even step of the recurrence */
       /*---------------------------------*/

       d = 1.0 + aa*d;
       if(fabs(d) < FPMIN)
          d = FPMIN;

       c = 1.0 + aa / c;
       if(fabs(c) <FPMIN)
          c = FPMIN;

       d  =  1.0 / d;
       h  *= d*c;
       aa =  -(a + m)*(qab + m)*x / ((a + m2)*(qap + m2));


       /*----------------------------*/
       /* Odd step of the recurrence */
       /*----------------------------*/

       d = 1.0 + aa*d;
       if(fabs(d) < FPMIN)
          d = FPMIN; 
		         
       c = 1.0 + aa / c;
       if(fabs(c) <FPMIN)
          c = FPMIN;

       d   =  1.0 / d;
       del =  d*c;
       h   *= del;


       /*-------------------------*/
       /* Test for exit condition */
       /*-------------------------*/

       if(fabs(del - 1.0) < BEPS)
          break;
    }

    if(m > ITMAX)
      pups_error("[betacf] a or b too big or ITMAX too small");

    return(h);
}




/*-----------------------------------------------------------------------------
    Return the incomplete gamma function ...
-----------------------------------------------------------------------------*/

_PRIVATE FTYPE gammq(FTYPE a, FTYPE x)

{   FTYPE gamser,
          gammcf,
          gln;

    if(x <0.0 || a <= 0.0)
      pups_error("[gammq] incorrect argument(s) to gammq");

    if(x < (a + 1.0))
    {  

       /*-------------------------------*/
       /* Use the series representation */
       /*-------------------------------*/

       gser(&gamser,a,x,&gln);
       return(1.0 - gamser);
    }
    else
    {  

       /*-------------------------------------------*/
       /* Use the continued fraction representation */
       /*-------------------------------------------*/

       gcf(&gammcf,a,x,&gln);
       return(gammcf);
    }
}




/*------------------------------------------------------------------------------
    Return the incomplete gamma function evaluated by its series
    representation ...
------------------------------------------------------------------------------*/

_PRIVATE void gser(FTYPE *gamser, FTYPE a, FTYPE x, FTYPE *gln)

{   int n;

    FTYPE sum = 0.0,
           del,
           ap;

    *gln = gammln(a);

    if(x <= 0.0)
    {  if(x < 0.0)
          pups_error("[gser] x less than 0");

       *gamser = 0.0;
       return;
    }
    else
    {  ap  = a;
       del = sum - 1.0/a;

       for(n=1; n<=ITMAX; ++n)
       {  ++ap;
          del *= x / ap;
          sum += del;

          if(fabs(del) < fabs(sum)*EPS)
          {  *gamser = sum*EXP(-x + a*LOG(x) - (*gln));
             return;
          }
       }

       pups_error("[gser] a too large or ITMAX too small");
    }
}



/*------------------------------------------------------------------------------
    Evaluate the incomplete gamma function by its continued fraction ...
------------------------------------------------------------------------------*/

_PRIVATE void gcf(FTYPE *gammcf, FTYPE a, FTYPE x, FTYPE *gln)

{   int i;

    FTYPE an,
           b,
           c,
           d,
           del,
           h;

    *gln = gammln(a);


    /*---------------------------------------------------------------------*/
    /* Set up for evaluating continued fraction by modified Lentz's method */
    /*---------------------------------------------------------------------*/

    b = x + 1.0 - a;
    c = 1.0 / FPMIN;
    d = 1.0 / b;
    h = d;


    /*------------------------*/
    /* Iterate to convergence */
    /*------------------------*/

    for(i=0; i<ITMAX; ++i)
    {  an = -i*(i - a);
       b  += 2.0;

       d  = an*d + b;
       if(fabs(d) < FPMIN)
          d = FPMIN;

       c = b + an / c;
       if(fabs(c) < FPMIN)
          c = FPMIN;

       d   = 1.0 / d;
       del = d*c;
       h   *= del;
       
       if(fabs(del - 1.0) <EPS)
          break;
    }

    if(i >= ITMAX)
       pups_error("[gammcf] a too large or ITMAX too small");


    /*----------------------*/
    /* Put factors in front */
    /*----------------------*/

    *gammcf = EXP(-x + a*LOG(x) - (*gln))*h;
}





/*-----------------------------------------------------------------------------
    Encrypt datastream using 8 rotor enigma machine (only secure enough to
    deter casual snooping) ...
-----------------------------------------------------------------------------*/

_PUBLIC int enigma8write(unsigned long key, int fdes, _BYTE *buf, unsigned long size)

{   _IMMORTAL _BOOLEAN entered = FALSE;
    unsigned long      i;
    int                ret;
    _BYTE              rotor;

    pups_set_errno(OK);
    if(fdes == (-1) || key == 0 || buf == (_BYTE *)NULL || size == 0)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*------------------------------------------------*/
    /* Start random number generator at correct point */
    /* in its sequence                                */
    /*------------------------------------------------*/

    if(entered == FALSE)
    {  entered = TRUE;
       r_init = (unsigned long)getpid();
    }


    /*-------------------*/
    /* Encrypt plaintext */
    /*-------------------*/

    for(i=0; i<size; ++i)
    {  rotor  = (_BYTE)(ran1()*256.0); 
       buf[i] = buf[i] % rotor; 
    }


    /*--------------*/
    /* Write cipher */
    /*--------------*/

    ret = write(fdes,buf,size);
    return(ret);
}




/*-----------------------------------------------------------------------------
    Decrypt datastream using 8 rotor enigma machine (only secure enough to
    stop casual snooping) ...
-----------------------------------------------------------------------------*/

_PUBLIC int enigma8read(unsigned long key, int fdes, _BYTE *buf, unsigned long int size)
 
{   _IMMORTAL _BOOLEAN entered = FALSE;
    unsigned long      i;
    int                ret;
    _BYTE              rotor;
 
    pups_set_errno(OK); 
    if(fdes == (-1) || key == 0 || buf == (_BYTE *)NULL || size == 0) 
    {  pups_set_errno(EINVAL); 
       return(-1); 
    } 


    /*------------------------------------------------*/
    /* Start random number generator at correct point */ 
    /* in its sequence                                */ 
    /*------------------------------------------------*/

    if(entered == FALSE)  
    {  entered = TRUE; 
       r_init = (unsigned long)getpid(); 
    }


    /*-------------*/ 
    /* Read cipher */
    /*-------------*/ 

    ret = pups_pipe_read(fdes,buf,size);


    /*----------------*/ 
    /* Decrypt cipher */
    /*----------------*/ 

    for(i=0; i<size; ++i)  
    {  rotor  = (_BYTE)(ran1()*256.0); 
       buf[i] = buf[i] % rotor;
    } 

    return(ret);
}
