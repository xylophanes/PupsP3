/*-------------------------------------------------------------------------
    Double 'in place' Fast Hartley transform routine. This routine is based
    upon the Pascal version of the Fast Hartley routine described in "Byte"
    April 1988 edition.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

   Version: 3.00
   Dated:   10th Decemeber 2024
   E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------------*/

#include <me.h>
#include <nfo.h>
#include <utils.h>

#undef   __NOT_LIB_SOURCE__
#include <fht.h>
#define  __NOT_LIB_SOURCE__

#include <utils.h>


/*-------------------------------------------------*/
/* Slot and usage functions - used by slot manager */
/*-------------------------------------------------*/
/*---------------------*/
/* Slot usage function */
/*---------------------*/

_PRIVATE void fhtlib_slot(int32_t level)
{   (void)fprintf(stderr,"lib fhtlib %s: [ANSI C]\n",FHT_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(C) 1987-2024 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"PUPS/P3 fast Fourier/Hartley transform library (gcc %s: built %s %s)\n\n",__VERSION__,__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");
    (void)fflush(stderr);
}




/*-------------------------------------------*/
/* Segment identification for fhtlib library */
/*-------------------------------------------*/

#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT )() __attribute__ ((aligned(16))) = fhtlib_slot;
#endif /* SLOT */


/*----------------------------------------*/
/* Prototypes private to this application */
/*----------------------------------------*/

// Find power of two represented by input number
_PROTOTYPE _PRIVATE int32_t get_pwr(int32_t);

// Index permutation routine
_PROTOTYPE _PRIVATE int32_t permute(int32_t,  int32_t);

// Generate trig tables
_PROTOTYPE _PRIVATE void trig_table(int32_t);

// Modify retrograde indexes
_PROTOTYPE _PRIVATE int32_t modify(int32_t,  int32_t,  int32_t,  int32_t,  int32_t);

// Butterfly transform an index pair
_PROTOTYPE _PRIVATE void butterfly(int32_t,   \
                                   int32_t,   \
                                   int32_t,   \
                                   int32_t,   \
                                   int32_t,   \
                                   FTYPE [],  \
                                   FTYPE []);

// Routine to swap a pair of pointers
_PROTOTYPE _PRIVATE void swap_pointers(FTYPE *, FTYPE *);


/*----------------------------------------------------------*/
/* Private global variables used by all the Hartley library */
/*  procedures                                              */
/*----------------------------------------------------------*/

_PRIVATE FTYPE start,
               step,
               *sne   = (FTYPE *)NULL,
               *csn   = (FTYPE *)NULL;;




/*-----------------------------------------------*/
/* Find power of two represented by input number */
/*-----------------------------------------------*/

_PRIVATE int32_t get_pwr(int32_t arg)

{    int32_t i = 0;

    while(arg != 1)
    {  arg = arg >> 1;
       ++i;
    }
    return(i);
}




/*--------------------------------------------------------------------------*/
/* Permutation routine. This routine re-orders the data before the butterly */
/* transform routine is called                                              */
/*--------------------------------------------------------------------------*/

_PRIVATE int32_t permute(int32_t b_index, int32_t pwr)

{    int32_t i,
             r,
             s,
             j = 0;

   for(i=0; i<pwr; ++i)
   {  s       = b_index >> 1;
      j       = (j << 1) + b_index - (s << 1);
      b_index = s;
   }
   return(j);
}



/*----------------------------------------------------------------------------*/
/* Calculate the trignometric functions required by the FHT and store values. */
/* For a N point transform, the trignometric functions will be calculated  at */
/*  int32_tervals of Nths of a turn                                                */
/*----------------------------------------------------------------------------*/

_PRIVATE void trig_table(int32_t size)

{   uint32_t i;

    FTYPE omega,
          angle = 0.0;

    sne = (FTYPE *)pups_calloc(size,sizeof(FTYPE));
    csn = (FTYPE *)pups_calloc(size,sizeof(FTYPE));

    omega = 2.0 * PI / size;
    for(i=0; i<size; ++i)
    {  sne[i] = SIN(angle);
       csn[i] = COS(angle);
       angle  += omega;
    }
}




/*-------------------------------------------------------------------------*/
/* Calculate the address of the retrograde index for the sine term for the */
/* dual place alogrithm, if it is required                                 */
/*-------------------------------------------------------------------------*/

_PRIVATE int32_t modify(int32_t    power,
                        int32_t  s_start,
                        int32_t    s_end,
                        int32_t  b_index,
                        int32_t     size)

{    int32_t ret;

    if(s_end == b_index /* || power < 3 */)
       ret = b_index;
    else
       ret = (power << 1) + s_start + s_end - b_index;
    return(ret);
}



/*-----------------------------------*/
/* Butterfly transform an index pair */
/*-----------------------------------*/

_PRIVATE void butterfly(int32_t trig_ind,
                        int32_t size,
                        int32_t i_1,
                        int32_t i_2,
                        int32_t i_3,
                        FTYPE   f_a[],
                        FTYPE   t_a[])

{   t_a[i_1] = f_a[i_1] + f_a[i_2]*csn[trig_ind] + f_a[i_3]*sne[trig_ind];
    trig_ind += size >> 1;
    t_a[i_2] = f_a[i_1] + f_a[i_2]*csn[trig_ind] + f_a[i_3]*sne[trig_ind];
}




/*--------------------------------------------*/
/* Get the real part of the Fourier transform */
/*--------------------------------------------*/

_PUBLIC void get_fft_R(uint32_t     size,
                       FTYPE da_arr_in[],
                       FTYPE da_arr_out[])

{   uint32_t i;

    for(i=0; i<size; ++i)
        da_arr_out[i] = da_arr_in[i] + da_arr_in[size-i-1];
}




/*-------------------------------------------------*/
/* Get the imaginary part of the Fourier transform */
/*-------------------------------------------------*/

_PUBLIC void get_fft_IM(uint         size,
                        FTYPE da_arr_in[],
                        FTYPE da_arr_out[])

{   uint32_t i;

    for(i=0; i<size; ++i)
        da_arr_out[i] = da_arr_in[i] - da_arr_in[size-i-1];
}




/*------------------------*/
/* Get the power spectrum */
/*------------------------*/

_PUBLIC void get_pwr_S(uint         size,
                       FTYPE da_arr_in[],
                       FTYPE da_arr_out[])

{   uint32_t i;

    for(i=0; i<size; ++i)
        da_arr_out[i] = (sqr(da_arr_in[i]) + sqr(da_arr_in[size-i-1])) / 2;
}




/*---------------------------------------------------------------*/
/* Function to test whether input data is in fact a power of two */
/*---------------------------------------------------------------*/

_PRIVATE _BOOLEAN pwr_of_2(uint size)

{   uint32_t    i;

    while(size%2 == 0)
         size = size >> 1;

    if(size != 1)
       return(FALSE);
    else
       return(TRUE);
}




/*------------------------------------------------------------------------------
   Main program for the fast Hartley transform. Translated from ISO Pascal
   to C 14th April 1988.
------------------------------------------------------------------------------*/

_PUBLIC void fht(int32_t sign,  int32_t size, FTYPE da_arr[])

{   int32_t pwr,
            i,
            j,
            k,
            trg_ind,
            trg_inc,
            power,
            section,
            s_start,
            s_end,
            st;

    FTYPE *da_temp_1  = (FTYPE *)NULL,
          *da_temp_2  = (FTYPE *)NULL, 
                *f_a  = (FTYPE *)NULL,
                *t_a  = (FTYPE *)NULL;

    da_temp_1 = (FTYPE *)pups_calloc(size,sizeof(FTYPE));
    da_temp_2 = (FTYPE *)pups_calloc(size,sizeof(FTYPE));


    /*-----------------------------------------------------------------------*/
    /*  First equivalence 'from data' array pointer to the input data array, */
    /* da_arr                                                                */
    /*-----------------------------------------------------------------------*/

     f_a = &da_temp_1[0];
     t_a = &da_temp_2[0];

     if(pwr_of_2(size) == TRUE)
     {  power = 1;
        pwr   = get_pwr(size);
        trig_table(size);


        /*-----------------*/
        /* Permute indexes */
        /*-----------------*/

        for(i=0; i<size; ++i)
            f_a[i] = da_arr[permute(i,pwr)];


        /*------------------------------------------*/
        /* Start of the Hartley butterfly transform */
        /*------------------------------------------*/

        for(i=1; i<pwr+1; ++i)
        {  j = 0;
           section = 0;
           trg_inc = (int)(size / (power << 1));
           do {  trg_ind = 0;
                 s_start = section*power;
                 s_end   = (section + 1)*power;

                 for(k=0; k<power; ++k)
                 {   butterfly(trg_ind,
                               size,
                               j,
                               j + power,
                               modify(power,s_start,s_end,j + power,size),
                                                                      f_a,
                                                                      t_a);
                     trg_ind += trg_inc;
                     ++j;
                  }

                  j += power;
                  section += 2;
              }   while(j < size);
           power = power << 1;


           /*-----------------------------*/
           /* Swap dual in place pointers */
           /*-----------------------------*/

           swap_pointers(f_a,t_a);
        }


        /*-------------------------------------------------------------------------*/
        /* End of Hartley butterfly. The results are scaled in necessary, and then */
        /*  placed in back  int32_to the array data                                     */
        /*-------------------------------------------------------------------------*/

        for(i=0; i<size; ++i)
           if(sign == 1)
               da_arr[i] = f_a[i] / size;
            else
               da_arr[i] = f_a[i];
    }
    else
       pups_error("[fht] number of points is not a power of two");

    (void)pups_free((void *)f_a);
    (void)pups_free((void *)t_a);
}




/*--------------------------------------------------*/
/* This routine is used to apply a band-stop filter */
/*--------------------------------------------------*/

_PUBLIC void band_pass(uint32_t      size,   // Data size
                       FTYPE      f_start,   // Start of band pass
                       FTYPE       f_stop,   // End of band pass
                       FTYPE        t_inc,   // Time dopups_main increment
                       FTYPE  *trans_func)   // Transfer function 
{    int32_t i,
             nf1,
             nf2;

    FTYPE band;

    band = 0.5 / t_inc;


    /*---------------------------------------*/
    /* Zero width filters are a little silly */
    /*---------------------------------------*/

    if(f_start == f_stop)
       pups_error("[band_pass] zero width filter");


    /*--------------------------------*/
    /* Set up band pass filter limits */
    /*--------------------------------*/

    nf1 = (int)(f_start * (FTYPE)(size >> 1) / band + 0.5) - 1;
    if(nf1 < 0)
       nf1 = 0;

    nf2 = (int)(f_stop  * (FTYPE)(size >> 1) / band + 0.5) - 1;
    if(nf2 > size)
       nf2 = size >> 1;


    /*-----------------------------------------------------------------*/
    /* No point in continuing if the data isn't likely to get filtered */
    /*-----------------------------------------------------------------*/

    if(nf1 == 0 && nf2 == size >> 1)
       pups_error("[band_pass] filter width is size of dataset");
 

    /*-----------------------------*/
    /* Build the transfer function */
    /*-----------------------------*/

    for(i=0; i<size; ++i)
    {   if(i >= nf1 && i <= nf2)
        {  trans_func[i]        = 0.0;
           trans_func[size - i] = 0.0;
        }
        else
        {  trans_func[i]            = 1.0;
           trans_func[size - i - 1] = 1.0;
        }
    }
}




/*-------------------------------------------------------------------*/
/* Routine to generate power spectral density function from FFT data */
/*-------------------------------------------------------------------*/

_PUBLIC void get_FFT_pwr_S(uint32_t         size, // Data size
                           FTYPE  pwr_spectrum[], // Power spectrum
                           FTYPE        r_data[], // Real part of FFT
                           FTYPE        i_data[]) // Imaginary part of FFT

{   uint32_t i;

    for(i=0; i<size >> 1; ++i)
       pwr_spectrum[i] = r_data[i]*r_data[i] + 2.0*r_data[i]*i_data[i] +
                                                    i_data[i]*i_data[i];
}




/*---------------------------------------------------------------------*/
/* Routine to resample the data - for HIPS data, will have to supply a */
/* line of data to be resampled                                        */
/*---------------------------------------------------------------------*/

_PUBLIC void resample(uint32_t  old_size,
                      uint32_t  new_size,
                      FTYPE     in_data[],
                      FTYPE     out_data[])

{   uint32_t    i;

    FTYPE *in_x      = (FTYPE *)NULL,
          *in_data_2 = (FTYPE *)NULL;


    /*---------------------*/
    /* Set up cubic spline */
    /*---------------------*/

    in_x      = (FTYPE *)pups_calloc(old_size, sizeof(FTYPE));
    in_data_2 = (FTYPE *)pups_calloc(old_size, sizeof(FTYPE));
    
    for(i=0; i<old_size; ++i)
        in_x[i] = (FTYPE)i*(FTYPE)new_size/(FTYPE)old_size;

    spline(in_x,
           in_data,
           old_size,
           NATURAL,
           NATURAL,
           in_data_2);


    /*---------------------------------------------------*/
    /* Spline data, and put it  int32_to the resampling array */
    /*---------------------------------------------------*/

    for(i=0; i<new_size; ++i)
        out_data[i] = splint(in_x,
                             in_data,
                             in_data_2,
                             old_size,
                             (FTYPE)i);
}



      
/*----------------------------------------------------------------*/
/* Routine to replace a function by its complex Fourier Transform */
/*----------------------------------------------------------------*/

_PUBLIC void Fourier(int32_t     flags,  // Initialisation flags 
                     int32_t     isign,  // Transform direction
                     int32_t     isize,  // Data size
                     FTYPE  r_da_arr[],  // Real part of data
                     FTYPE  i_da_arr[])  // Imaginary part of data

{    int32_t i,
             j,
             k,
             m,
             n,
             mb,
             ja,
             jb,
             jja,
             jjb,
             mmax,
             itemp,
             istep,
             i_temp;


    FTYPE scale,
          r_temp,
          f_temp,
          theta,
          r_theta,
          i_theta;

   
    _IMMORTAL  int32_t ifirst     = 0,
                  started    = FALSE,
                  *b_index   = (int *)NULL;

    _IMMORTAL FTYPE *i_array = (FTYPE *)NULL,
                    *r_array = (FTYPE *)NULL,
                    *r_omega = (FTYPE *)NULL,
                    *i_omega = (FTYPE *)NULL;


    /*----------------------------------*/
    /* Free resources if asked to do so */
    /*----------------------------------*/

    if(flags & RESET)
    {  b_index = (int *)  pups_free((void *)b_index);
       i_array = (FTYPE *)pups_free((void *)i_array);
       r_array = (FTYPE *)pups_free((void *)r_array);
       i_omega = (FTYPE *)pups_free((void *)i_omega);
       r_omega = (FTYPE *)pups_free((void *)r_omega);

       return;
    }


    /*----------------------------------------------------*/
    /* If size is not a power of two, flag error and exit */
    /*----------------------------------------------------*/

    if(pwr_of_2(isize) == FALSE)
       pups_error("[Fourier] not a power of two");


    /*-----------------------------------------------------------------*/
    /* This section of the routine is only executed the first time the */
    /* routine is entered. This sets up the indexes for the butterfly  */
    /* transform                                                       */
    /*-----------------------------------------------------------------*/

    if(started == FALSE || flags & RESTART)
    {  b_index = (int *)  pups_calloc(isize,sizeof(int));
       i_array = (FTYPE *)pups_calloc(isize,sizeof(FTYPE));
       r_array = (FTYPE *)pups_calloc(isize,sizeof(FTYPE));
       r_omega = (FTYPE *)pups_calloc(isize,sizeof(FTYPE));
       i_omega = (FTYPE *)pups_calloc(isize,sizeof(FTYPE));

       for(i=0; i<isize; ++i)
           b_index[i] = i;
   
       for(j=0; j<isize; ++j)
       {  jb = 0;
          ja = j - 1;
          m  = isize >> 1;
          n  = 1;

          do {   if(j >= m)
                 {  ja = ja - m;
                    jb = jb + n;
                 }

                 m = m > 1;
                 n = n < 1;
             } while(ja != 0);

          if(j - 1 < jb)
          {  itemp           = b_index[j];
             b_index[j]      = b_index[jb + 1];
             b_index[jb + 1] = itemp;
          }
       }
       started = TRUE;
    }


    /*--------------------------------------------------*/
    /* This is the start of the FFT butterfly transform */
    /*--------------------------------------------------*/

    mb    = 0;
    mmax  = 1;

    do {   istep = mmax << 1;

           for(m=0; m<mmax; ++m)
           {   ++mb;

               if(ifirst != isign)
               {  theta   = PI * (FTYPE)(isign * (m - 1)) / (FTYPE)mmax;
                  r_theta = 0.0;
                  i_theta = theta;

                  r_omega[m] = COS(theta);
                  i_omega[m] = SIN(theta);
               }

               for(jja = m; jja < isize; jja += istep)
               {  jjb = jja + mmax;
                  ja  = b_index[jja];
                  jb  = b_index[jjb];

                  r_temp = r_array[jb]*r_omega[mb] + i_array[jb]*i_omega[mb];
                  i_temp = r_array[jb]*i_omega[mb] + i_array[jb]*r_omega[mb];

                  r_array[jb] = r_array[ja] - r_temp;
                  i_array[jb] = i_array[ja] - i_temp;

                  r_array[ja] = r_array[ja] + r_temp;
                  i_array[ja] = i_array[ja] + i_temp;
              }
           }
       } while(mmax < isize);


    /*----------------------------------------------*/
    /* End of Butterfly - scale and reorder results */
    /*----------------------------------------------*/

    scale = 1.0 / sqrt(isize);
    for(i=0; i<isize; ++i)
    {   k = b_index[i];
        if(k >= i)
        {  r_temp     = r_array[k]*scale;
           i_temp     = i_array[k]*scale;
           r_array[k] = r_array[i]*scale;
           i_array[k] = i_array[i]*scale;
           r_array[i] = r_temp;
           i_array[i] = i_temp;
        }
    }

    /*--------------------------------*/
    /* End of scaling and re-ordering */
    /*--------------------------------*/

    ifirst = 1;
}




/*------------------------------------*/
/* Routine to swap a pair of pointers */
/*------------------------------------*/

_PRIVATE void swap_pointers(FTYPE *p_1, FTYPE *p_2)

{   FTYPE *tmp_ptr = (FTYPE *)NULL;

    tmp_ptr = p_1;
    p_1     = p_2;
    p_2     = tmp_ptr;
}





/*-------------------------------------------------------------------------*/
/* Given a real vector of data[1 ... n], and given m this routine returns  */
/* the m linear prediction coefficients as d[1 ... m] and also returns the */
/*  mean square discrepancy as xms                                         */
/*-------------------------------------------------------------------------*/

 
_PUBLIC void memcof(FTYPE data[], int32_t n, int32_t m, FTYPE *xms, FTYPE d[], int32_t rpt_cnt)

{    int32_t i,
             j,
             k,
             v_index,
             n_eff;

    FTYPE p      = 0.0,
          *wk1   = (FTYPE *)NULL,
          *wk2   = (FTYPE *)NULL,
          *wkm   = (FTYPE *)NULL;

    wk1   = (FTYPE *)pups_calloc(n,  sizeof(FTYPE));
    wk2   = (FTYPE *)pups_calloc(n,  sizeof(FTYPE));
    wkm   = (FTYPE *)pups_calloc(m+1,sizeof(FTYPE));
    n_eff = n*rpt_cnt;

    for(j=1; j<=n_eff; j++)
    {  v_index = j%n;
       if(v_index == 0)
          v_index = 1;

       p += sqr(data[v_index]);
    }

    *xms = p / n_eff;

    wk1[1]   = data[1];
    wk2[n-1] = data[n];

    for(j=2; j<=n-1; ++j)
    {  wk1[j]   = data[j];
       wk2[j-1] = data[j];
    }

    for(k=1; k<=m; ++k)
    {  FTYPE num   = 0.0;
       FTYPE denom = 0.0;

       for(j=1; j<=(n_eff-k); ++j)
       {   v_index = j%n;
           if(v_index == 0)
              v_index = 1;

          num   += wk1[v_index]*wk2[v_index];
          denom += sqr(wk1[v_index]) + sqr(wk2[v_index]);
       }

       d[k] =  2.0*num/denom;
       *xms += (1.0 - sqr(d[k]));

       for(i=1; i<=(k-1); ++i)
          d[i] = wkm[i] - d[k]*wkm[k-i];

       /*--------------------------------------------------------------------------*/
       /* This algorithm is recursive, building up the answer for larger values of */
       /* m until the desired value is reached. At this point the algorithm could  */
       /* return the vector d and scalar xms for a set of LP coefficients with k   */
       /* (rather than m) terms                                                    */
       /* -------------------------------------------------------------------------*/

       if(k == m)
       {  (void)pups_free((void *)wk1);
          (void)pups_free((void *)wk2);
          (void)pups_free((void *)wkm);

          return;
       }

       for(i=1; i<=k; ++i)
          wkm[i] = d[i];

       for(j=1; j<=(n_eff-k-1); ++j)
       {   v_index = j%(n-1);
           if(v_index == 0)
              v_index = 1;

          wk1[v_index] -= wkm[k]*wk2[v_index];
          wk2[v_index] = wk2[v_index+1] - wkm[k]*wk1[v_index+1];
       }
   }
}




/*-----------------------------------------------------------------------------*/
/* Routine to compute the power spectrum using the maximum entropy [all poles] */
/* method. Given d[1 ... m], m and xms returned by memcof, this function       */
/* returns the power spectrum estimate, p(f) as a function of fdt              */
/*-----------------------------------------------------------------------------*/

_PUBLIC FTYPE evlmem(FTYPE fdt, FTYPE d[], int32_t m, FTYPE xms)

{   uint32_t i;

    FTYPE sumr = 1.0,
          sumi = 0.0,
          wr   = 1.0,
          wi   = 0.0,
          wpr,
          wpi,
          wtemp,
          theta;


    theta = 2*PI*fdt;

    wpr = cos(theta);
    wpi = sin(theta);

    for(i=1; i<=m; ++i)
    {  wr   = (wtemp = wr)*wpr - wi*wpi;
       wi   = wi*wpr + wtemp*wpi;
       sumr -= d[i]*wr;
       sumi -= d[i]*wi;
    }

    return(xms / (sumr*sumr + sumi*sumi));
}




/*-----------------------------*/
/* Definition of wavefilt_type */
/*-----------------------------*/

typedef struct {    int32_t ncof,
                            ioff,
                        joff;

                    FTYPE *cc,
                          *cr;
               } wfilt_type;

_PRIVATE wfilt_type wfilt;


/*----------------------------------------------------------------------------------*/
/* Initialising routine for generalised wavelet function generator, pwt. Implements */
/* Debauchies wavelet filters with 4, 12 and 20 coefficients (selected by bvalue n) */
/* This routine is called once, prior to calling pwt                                */
/*----------------------------------------------------------------------------------*/


_PUBLIC  int32_t pwtset(int n)

{   uint32_t k;

    FTYPE sig = (-1.0);

    _IMMORTAL FTYPE c4[5]   = { 0.0,                                                                               /*--------*/
                                0.4829629131445341, 0.8365163037378079, 0.2241438680420134, -0.1294095225512604 }; /* Daub4  */
                                                                                                                   /*--------*/


    _IMMORTAL FTYPE c12[13] = { 0.0,                                                                               /*--------*/
                                0.111540743350,  0.494623890398,  0.751133908021,                                  /* Daub12 */
                                0.315250351709, -0.226264693965, -0.129766867567,                                  /*--------*/
                                0.097501605587,  0.027522865530, -0.031582039318,
                                0.000553842201,  0.004777257511, -0.001077301085};


    _IMMORTAL FTYPE c20[21] = {  0.0,                                                                              /*--------*/
                                 0.026670057901,   0.188176800078,  0.527201188932,                                /* Daub20 */
                                 0.688459039454,   0.281172343661, -0.249846424327,                                /*--------*/
                                -0.195946274377,   0.127369340336,  0.093057364604,
                                -0.071394147166,  -0.029457536822,  0.033212674059,
                                 0.003606553567,  -0.010733175483, -0.000116466855,
                                 0.000093588670,  -0.000013264203                 }; 
                              

    _IMMORTAL FTYPE c4r [5],
                    c12r[13],
                    c20r[21];

    wfilt.ncof = n;
    if(n == 4)
    {  wfilt.cc = c4;
       wfilt.cr = c4r;
    }
    else if(n == 12)
    {  wfilt.cc = c12;
       wfilt.cr = c12r;
    }
    else if(n == 20)
    {  wfilt.cc = c20;
       wfilt.cr = c20r;
    }
    else
       return(-1); 

    for(k=1; k<=n; k++)
    {  wfilt.cr[wfilt.ncof + 1 -k]  = sig*wfilt.cc[k];
       sig                          = -sig;
    }

    wfilt.ioff = wfilt.joff = -(n >> 1);

    return(0);
}



/*-------------------------------------------------------------------------------*/
/* Partial wavelet transform, applies arbitary wavelet filter to data vector a[] */
/* (isign = 1) or its inverse transform (isign = -1)                             */
/*-------------------------------------------------------------------------------*/

_PUBLIC  int32_t pwt(FTYPE a[], uint64_t n, int32_t isign)

{   FTYPE ai,
          ai1,
          *wksp = (FTYPE *)NULL;

    uint64_t i,
             ii,
             j,
             jf,
             jr,
             k,
             n1,
             ni,
             nj,
             nh,
             nmod;

    if(n < 4)
      return(-1);

    wksp = pups_calloc(n + 1,sizeof(FTYPE));
    nmod = wfilt.ncof*n;
    n1   = n - 1;
    nh   = n >> 1;

    for(j=1; j<=n; j++)
       wksp[j] = 0.0;

    if(isign >= 0)
    {  for(ii=1,i=1; i<=n; i += 2,ii++)
       {  ni = i + nmod + wfilt.ioff;
          nj = i + nmod + wfilt.joff;

          for(k=1; k<=wfilt.ncof; k++)
          {  jf           = n1 & (ni + k);
             jr           = n1 & (nj + k);
             wksp[ii]    += wfilt.cc[k]*a[jf+1];
             wksp[ii+nh] += wfilt.cr[k]*a[jr+1];
          }
       }
    }
    else
    {  for(ii=1,i=1; i <= n; i += 2,ii++)
       {  ai  = a[ii];
          ai1 = a[ii+nh];
          ni  = i + nmod + wfilt.ioff;
          nj  = i + nmod + wfilt.joff;

          for(k=1; k <= wfilt.ncof; k++)
          {  jf        = (n1 & (ni + k)) + 1;
             jr        = (n1 & (nj + k)) + 1;
             wksp[jf] += wfilt.cc[k]*ai;
             wksp[jr] += wfilt.cr[k]*ai1;
          }
       }
    }

    for(j=1; j <= n; j++)
       a[j] = wksp[j];

    (void)pups_free((void *)wksp);

    return(0);
}




/*---------------------------------------------------------------------------------*/
/* One dimensional discrete wavelet transform. This routine implements the pyramid */
/* algorithm replacing an input vector a[1..n] by its wavelet transform (for isign */
/*  = 1) or performing the inverse operation (for isign = -1). Note that as in the */
/* Cooley Tukey FFT algorithm n MUST be an integer power of 2. The routine wstep   */
/* whose name is supplied to the calling routine, is the underlying wavelet filter */
/* used by this routine                                                            */
/*---------------------------------------------------------------------------------*/

_PUBLIC void wt1(FTYPE     a[],
                 uint64_t  n,
                 int32_t   isign,
                 void      (*wstep)(FTYPE [], uint64_t, int32_t))

{   uint64_t nn;

    if(n < 4)
       return;

    if(isign >= 0)
    {

       /*-----------------------------------------------------------------------------*/
       /* Forward wavelet transform - note that we start at the largest hierarchy and */
       /* work towards the smallest                                                   */
       /*-----------------------------------------------------------------------------*/

       for(nn=n; nn>=4; nn>>=1)
          (*wstep)(a,nn,isign);
    }
    else
    {

       /*---------------------------*/
       /* Inverse wavelet transform */
       /*---------------------------*/

       for(nn=4; nn<=n; nn<<=1)
          (*wstep)(a,nn,isign);

    }
}





/*-------------------------------------------------------------*/
/* Coefficients for Debauchies 4-coefficient wavelet transform */
/*-------------------------------------------------------------*/

#define C0  0.4829629131445431
#define C1  0.8365163037378079
#define C2  0.2241438680420134
#define C3 -0.1294095225512604


/*------------------------------------------------------------------------------*/
/* Applies Debauchies 4-coefficent wavelet filter to a data vector a[1..n] (for */
/* isign = 1) or applies its transpose (for isign = -1). Used by hiearchical    */
/* routine wt1                                                                  */
/*------------------------------------------------------------------------------*/

_PUBLIC void daub4(FTYPE a[], uint64_t n, int32_t isign)

{   FTYPE a_max = 0.0,
          *wksp = (FTYPE *)NULL;

    uint64_t nh,
             nh1,
             i,
             j;

    if(n < 4)
      return;

    wksp = (FTYPE *)pups_calloc(n+1,sizeof(FTYPE));
    nh1 = (nh = n >> 1) + 1;

    if(isign >= 0)
    {

        /*----------------------*/
        /* Apply forward filter */
        /*----------------------*/

        for(i=1,j=1; j<=n-3; j+=2,i++)
        {  wksp[i]      = C0*a[j] + C1*a[j+1] + C2*a[j+2] + C3*a[j+3];
           wksp[i + nh] = C3*a[j] - C2*a[j+1] + C1*a[j+2] - C0*a[j+3];
        }

        wksp[i]      = C0*a[n-1] + C1*a[n] + C2*a[1] + C3*a[2];
        wksp[i + nh] = C3*a[n-1] - C2*a[n] + C1*a[1] - C0*a[2];
    }

    else
    {

       /*----------------------------------*/
       /* Apply inverse [transpose] filter */
       /*----------------------------------*/

       wksp[1] = C2*a[nh] + C1*a[n] + C0*a[1] + C3*a[nh1];
       wksp[2] = C3*a[nh] - C0*a[n] + C1*a[1] - C2*a[nh1];

       for(i=1,j=3; i<nh; ++i)
       {  wksp[j++] = C2*a[i] + C1*a[i+nh] + C0*a[i+1] + C3*a[i+nh1];
          wksp[j++] = C3*a[i] - C0*a[i+nh] + C1*a[i+1] - C2*a[i+nh1];
       }
    }

    for(i=1; i<=n; i++)
    {  a[i] = wksp[i];

       if(a_max < wksp[i])
          a_max =  wksp[i];
    }

    (void)pups_free((void *)wksp);
}




/*---------------------------------*/
/* N dimensional wavelet transform */
/*---------------------------------*/

_PUBLIC void wtn(FTYPE     a[],
                 uint64_t  nn[],
                 int32_t   ndim,
                 int32_t   isign,
                 void      (*wstep)(FTYPE [], uint64_t, int32_t))

{   uint64_t i1,
             i2,
             i3,
             k,
             n,
             nnew,
             nprev = 1,
             nt,
             ntot  = 1;

    int32_t  idim;
    FTYPE    *wksp = (FTYPE *)NULL;

    for(idim=1; idim<=ndim; idim++)
       ntot *= nn[idim];

    wksp = (FTYPE *)pups_calloc(ntot+1,sizeof(FTYPE));


    /*---------------------------*/
    /* Main loop over dimensions */
    /*---------------------------*/

    for(idim=1; idim<=ndim; idim++)
    {  n = nn[idim];
       nnew = n*nprev;
 
       if(n > 4)
       {  for(i2=0; i2<ntot; i2 += nnew)
          {  for(i1=1; i1<=nprev; i1++)
             {  for(i3=i1+i2,k=1; k<=n; k++,i3+=nprev)
                   wksp[k] = a[i3];


                /*----------------------------------------*/
                /* Copy relevant dimension  int32_to workspace */
                /*----------------------------------------*/

                if(isign >= 0)
                {  

                   /*-------------------*/
                   /* Forward transform */
                   /*-------------------*/

                   for(nt=n; nt>=4; nt >>= 1)
                      (*wstep)(wksp,nt,isign);
                }

                else
                {

                   /*-------------------*/
                   /* Inverse transform */
                   /*-------------------*/

                   for(nt=4; nt<=n; nt <<= 1)
                      (*wstep)(wksp,nt,isign);
                }


                /*----------------------------*/
                /* Copy transformed data back */
                /*----------------------------*/

                for(i3=i1+i2,k=1; k<=n; k++,i3+=nprev)
                   a[i3] = wksp[k];
             }
          }
       }

       nprev = nnew;
   }

   (void)pups_free((void *)wksp);
}
