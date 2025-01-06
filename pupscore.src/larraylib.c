 /*---------------------------------------
    Purpose: sparse matrix support library

    Author:  Mark O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.00 
    Dated:   10th Decemeber 2024
    E-mail:  mao@tumblingdice.co.uk
----------------------------------------*/


#include <stdio.h>
#include <me.h>
#include <utils.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#include <errno.h>
#include <ftype.h>
#include <bsd/bsd.h>:

#undef   __NOT_LIB_SOURCE__
#include <larray.h>
#define  __NOT_LIB_SOURCE__
                                  

/*---------------------------------------------*/
/* Functions which are private to this library */
/*---------------------------------------------*/

_PROTOTYPE _PRIVATE int32_t skip_comments(const FILE *);


/*---------------------------------------------*/
/* Variables which are private to this library */
/*---------------------------------------------*/
/*-------------------------------------------------*/
/* Slot and usage functions - used by slot manager */
/*-------------------------------------------------*/
/*---------------------*/
/* Slot usage function */
/*---------------------*/

_PRIVATE void larray_slot(int32_t level)
{   (void)fprintf(stderr,"lib larraylib %s: [ANSI C]\n",LARRAY_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(C) 2013-2024 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"PUPS/P3 sparse matrix support library (gcc %s: built %s %s)\n\n",__VERSION__,__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");
    fflush(stderr);
}


/*-------------------------------------------*/
/* Segment identification for larray library */
/*-------------------------------------------*/

#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT )() __attribute__ ((aligned(16))) = larray_slot;
#endif /* SLOT */

                          


/*---------------------*/
/* Destroy list vector */
/*---------------------*/

_PUBLIC vlist_type *lvector_destroy(vlist_type *vector)

{   uint32_t i;

    if(vector == (vlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return((vlist_type *)NULL);
    }

    if(vector->index != (int *)NULL)    
       (void)pups_free((void *)vector->index);

    if(vector->value != (FTYPE *)NULL)    
      (void)pups_free((void *)vector->value);

    vector->allocated  = 0;
    vector->used       = 0;
    vector->components = 0;

    (void)pups_free((void *)vector);

    pups_set_errno(OK);
    return((vlist_type *)NULL);
}




/*--------------------*/
/* Destroy list array */
/*--------------------*/

_PUBLIC mlist_type *lmatrix_destroy(mlist_type *matrix)

{   uint32_t i,
             j;

    if(matrix == (mlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return((mlist_type *)NULL);
    }

    for(i=0; i<matrix->rows; ++i)
    {  if(matrix->vector[i] != (vlist_type *)NULL)
          (void)lvector_destroy((void *)matrix->vector[i]);
    }
    (void)pups_free((void *)matrix->vector);

    matrix->rows = 0;
    matrix->cols = 0;

    (void)pups_free((void *)matrix);

    pups_set_errno(OK); 
    return((mlist_type *)NULL);
}




/*--------------------------------------------------------*/
/* Create list array (and populate it from pattern array) */
/*--------------------------------------------------------*/

_PUBLIC mlist_type *lmatrix_create(const FILE *stream, const uint32_t rows, const uint32_t cols, const FTYPE *pattern_array)

{   uint32_t i,
             j,
             size;

    FTYPE matrix_compression = 0.0;
    mlist_type *matrix       = (mlist_type *)NULL;

    if(cols < 0 || cols == 0 && pattern_array != (FTYPE *)NULL)
    {  pups_set_errno(EINVAL);
       return((mlist_type *)NULL);
    }

    if((matrix = (mlist_type *)pups_malloc(sizeof(mlist_type))) == (mlist_type *)NULL)
    {  pups_set_errno(ENOMEM);
       return((mlist_type *)NULL);
    }

    matrix->rows = rows;
    matrix->cols = cols;

    if((matrix->vector = (vlist_type **)pups_calloc(cols,sizeof(vlist_type *))) == (vlist_type **)NULL)
    {  (void)lmatrix_destroy(matrix); 

       pups_set_errno(ENOMEM);
       return((mlist_type *)NULL);
    }

    for(i=0; i<rows; ++i)
    {  if((matrix->vector[i] = (vlist_type *)pups_malloc(sizeof(vlist_type))) == (vlist_type *)NULL)
       {   (void)lmatrix_destroy(matrix);

           pups_set_errno(ENOMEM);
           return((mlist_type *)NULL);
       }
       else
       {  matrix->vector[i]->components = cols;
          matrix->vector[i]->allocated  = 0;
          matrix->vector[i]->used       = 0;
       }

       if(pattern_array != (FTYPE *)NULL)
       {   int32_t pos,
                   allocated = 0;

          size      = 0;
          for(j=0; j<cols; ++j)
          {  pos = i*cols + j;  


             /*----------------------------------------*/
             /* Allocate memory in quanta of ALLOC_MEM */
             /* We do this to make sure that memory is */
             /* used optimally                         */
             /*----------------------------------------*/
 
             if(size == 0 && allocated == 0)
             {  allocated = LARRAY_ALLOC_QUANTUM;
 
                if((matrix->vector[i]->index = (int32_t *)pups_calloc(LARRAY_ALLOC_QUANTUM,sizeof(int32_t))) == (int32_t *)NULL)
                {   (void)lmatrix_destroy(matrix);

                    pups_set_errno(ENOMEM);
                    return((mlist_type *)NULL);
                }

                if((matrix->vector[i]->value = (FTYPE *)pups_calloc(LARRAY_ALLOC_QUANTUM,sizeof(FTYPE))) == (FTYPE *)NULL)
                {   (void)lmatrix_destroy(matrix);

                    pups_set_errno(ENOMEM);
                    return((mlist_type *)NULL);
                }
             }
             else if(size == allocated)
             {  allocated += LARRAY_ALLOC_QUANTUM;

                if((matrix->vector[i]->index = (int32_t *)  pups_realloc((void *)matrix->vector[i]->index,allocated*sizeof(int32_t))) == (int32_t *)NULL)
                {   (void)lmatrix_destroy(matrix);

                    pups_set_errno(ENOMEM);
                    return((mlist_type *)NULL);
                }


                if((matrix->vector[i]->value = (FTYPE *)pups_realloc((void *)matrix->vector[i]->value,allocated*sizeof(FTYPE))) == (FTYPE *)NULL)
                {   (void)lmatrix_destroy(matrix);

                    pups_set_errno(ENOMEM);
                    return((mlist_type *)NULL);
                }
             }

             if(pattern_array[pos] != 0.0)
             {  matrix->vector[i]->value[size] = pattern_array[pos];
                matrix->vector[i]->index[size] = j;
                ++size;
             }
          }

          matrix->vector[i]->allocated = allocated;
          matrix->vector[i]->used      = size;

          if(size < cols)
          {  matrix->vector[i]->state = DEFLATED; 

             if(size != 0)


                /*-------------------------------------------*/
                /* Compute compression factor (for this row) */
                /*-------------------------------------------*/

                matrix_compression += (FTYPE)size/(FTYPE)cols;
          }
          else
             matrix->vector[i]->state = INFLATED;
       }
    }

    if(stream != (FILE *)NULL && matrix_compression != 0.0)
    {  (void)strdate(date);
       (void)fprintf(stream,"%s %s (%d@%s:%s):  List array compression factor is %5.2F%%\n",
                     date,appl_name,appl_pid,appl_host,appl_owner,100.0 - (100.0*matrix_compression/(FTYPE)rows));
       (void)fflush(stream);
    }

    pups_set_errno(OK);
    return(matrix);
}




/*----------------------------------------*/
/* Create list vector from pattern vector */
/*----------------------------------------*/

_PUBLIC vlist_type *lvector_create(const FILE *stream, const uint32_t components, const FTYPE *pattern_vector)

{   uint32_t i,
             size      = 0,
             allocated = 0;

    vlist_type *vector = (vlist_type *)NULL;

    if((vector = (vlist_type *)pups_malloc(sizeof(vlist_type))) == (vlist_type *)NULL)
    {  pups_set_errno(ENOMEM);
       return((vlist_type *)NULL);
    }

    vector->components = components;
    if(pattern_vector == (FTYPE *)NULL)
    {  

       /*---------------------------------------------*/
       /* Simply allocate memory for specified vector */
       /*---------------------------------------------*/

       if((vector->index = (int32_t *)pups_calloc(components,sizeof(int32_t))) == (int32_t *)NULL)
       {  (void)pups_free((void *)vector);

          pups_set_errno(ENOMEM);
          return((vlist_type *)NULL);
       }

       if((vector->value = (FTYPE *)pups_calloc(components,sizeof(FTYPE))) == (FTYPE *)NULL)
       {  (void)pups_free((void *)vector->index); 
          (void)pups_free((void *)vector);

          pups_set_errno(ENOMEM);
          return((vlist_type *)NULL);
       }

       vector->allocated  = components;
       for(i=0; i<components; ++i)
       {  vector->index[i] = i;
          vector->value[i] = 0.0;
       }

       vector->state = NONE;
    }
    else
    {

       /*------------------------------------*/ 
       /* Load pattern vector making optimal */
       /* use of memory                      */
       /*------------------------------------*/ 

       allocated = LARRAY_ALLOC_QUANTUM;

       if((vector->index = (int32_t *)pups_calloc(allocated,sizeof(int32_t))) == (int32_t *)NULL)
       {  (void)pups_free((void *)vector);

          pups_set_errno(ENOMEM);
          return((vlist_type *)NULL);
       }

       if((vector->value = (FTYPE *)pups_calloc(allocated,sizeof(FTYPE))) == (FTYPE *)NULL)
       {  (void)pups_free((void *)vector->index);
          (void)pups_free((void *)vector);

          pups_set_errno(ENOMEM);
          return((vlist_type *)NULL);
       }

       for(i=0; i<components; ++i)
       {  if(size == allocated)
          {  allocated += LARRAY_ALLOC_QUANTUM;

             if((vector->index = (int32_t *)pups_realloc((void *)vector->index,allocated*sizeof(int32_t))) == (int32_t *)NULL)
             {  (void)pups_free((void *)vector->index);
                (void)pups_free((void *)vector->value);
                (void)pups_free((void *)vector);

                pups_set_errno(ENOMEM);
                return((vlist_type *)NULL);
             }

             if((vector->value = (FTYPE *)pups_realloc((void *)vector->value,allocated*sizeof(FTYPE))) == (FTYPE *)NULL)
             {  (void)pups_free((void *)vector->index);
                (void)pups_free((void *)vector->value);
                (void)pups_free((void *)vector);

                pups_set_errno(ENOMEM);
                return((vlist_type *)NULL);
             }
          }

          if(pattern_vector[i] != 0.0)
          {  vector->value[size]  = pattern_vector[i];
             vector->index[size]  = i;
             ++size;
          }
       }

       vector->allocated = allocated;
       vector->used      = size;

       if(stream != (FILE *)NULL && size != components)
       {  (void)strdate(date);
          (void)fprintf(stream,"%s %s (%d@%s:%s)  List vector compression factor is %5.2F%%\n",
                        date,appl_name,appl_pid,appl_host,appl_owner,100.0 - (100.0*(FTYPE)size/(FTYPE)components));
          (void)fflush(stream);
       }

       vector->state = DEFLATED;
    }

    pups_set_errno(OK);
    return(vector);
}




/*-------------------------------------------------*/
/* Do a fast re-initialisation of a pattern vector */
/*-------------------------------------------------*/

_PUBLIC int32_t fastResetPatternVector(const uint32_t cols, FTYPE *pattern_vector, const vlist_type *vector)

{   uint32_t i;

    if(cols               <  0                     ||
       cols               != vector->components    ||  
       pattern_vector     == (FTYPE *)NULL         ||
       vector             == (vlist_type *)NULL    ||
       vector->components == 0                     || 
       vector->used       == 0                      )
    {  pups_set_errno(EINVAL);
       return(-1);
    }
       
    for(i=0; i<vector->used; ++i)
    {  if(vector->value[i] != 0.0)
          pattern_vector[vector->index[i]] = 0.0;
    }

    return(0);
}




/*-----------------*/
/* Copy list array */
/*-----------------*/

_PUBLIC mlist_type *lmatrix_copy(const mlist_type *from)

{   uint32_t i,
             j;

    mlist_type *to = (mlist_type *)NULL;

    if(from == (mlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return((mlist_type *)NULL);
    }

    if((to = (mlist_type *)pups_malloc(sizeof(mlist_type))) == (mlist_type *)NULL)
    {  pups_set_errno(ENOMEM);
       return(mlist_type *)NULL;
    }


    if((to->vector = (vlist_type **)pups_calloc(from->rows,sizeof(vlist_type *))) == (vlist_type **)NULL)
    {  (void)pups_free((void *)to); 

       pups_set_errno(ENOMEM);
       return(mlist_type *)NULL;
    }

    for(i=0; i<from->rows; ++i)
    {  if(from->vector[i]->allocated > 0)
       {  if((to->vector[i] = (vlist_type *)lvector_create((FILE *)NULL,from->vector[i]->allocated,NULL)) == (vlist_type *)NULL)
          {  lmatrix_destroy(to);

             pups_set_errno(ENOMEM);
             return((mlist_type *)NULL);
          }
          else
          {  for(j=0; j<from->vector[i]->allocated; ++j)
             {  to->vector[i]->index[j] = from->vector[i]->index[j];
                to->vector[i]->value[j] = from->vector[i]->value[j];
             }
          }
       }

       to->vector[i]->allocated = from->vector[i]->allocated;
    }

    (void)strlcpy(to->name,from->name,SSIZE);
    to->rows = from->rows;
    to->cols = from->cols;

    pups_set_errno(OK);
    return(to);
}





/*------------------*/
/* Copy list vector */
/*------------------*/

_PUBLIC vlist_type *lvector_copy(const vlist_type *from)

{   uint32_t i;

    vlist_type *to = (vlist_type *)NULL;

    if(from == (vlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return((vlist_type *)NULL);
    }

    if((to = lvector_create((FILE *)NULL,from->allocated,NULL)) == (vlist_type *)NULL) 
    {  pups_set_errno(ENOMEM);
       return((vlist_type *)NULL);
    }

    for(i=0; i<from->used; ++i)
    {  to->index[i] = from->index[i];
       to->value[i] = from->value[i];
    }

    (void)strlcpy(to->name,from->name,SSIZE);
    to->allocated = from->allocated;
    to->used      = from->used;

    pups_set_errno(OK);
    return(to);
}




/*---------------------------------------------------------------*/
/* Squeeze zero value elements out of a list vector deflating it */
/*---------------------------------------------------------------*/

_PUBLIC vlist_type *lvector_deflate(const FILE *stream, const _BOOLEAN keep_vector, FTYPE *vector_squeeze_percent, vlist_type *vector)

{   uint32_t  i,
              size                 = 0,
              allocated            = 0,
              vector_zero_elements = 0;

    FTYPE squeeze_percent          = 0.0;

    vlist_type *tmp_vector         = (vlist_type *)NULL,
               *squeezed_vector    = (vlist_type *)NULL;

    if(vector == (vlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return((vlist_type *)NULL);
    }

    if((squeezed_vector = (vlist_type *)pups_malloc(sizeof(vlist_type))) == (vlist_type *)NULL)
    {  pups_set_errno(ENOMEM);
       return((vlist_type *)NULL);
    }

    allocated = LARRAY_ALLOC_QUANTUM;
    if((squeezed_vector->index = (int32_t *)pups_calloc(allocated,sizeof(int32_t))) == (int32_t *)NULL)
    {  (void)lvector_destroy(squeezed_vector);

       pups_set_errno(ENOMEM);
       return((vlist_type *)NULL);
    }

    if((squeezed_vector->value = (FTYPE *)pups_calloc(allocated,sizeof(FTYPE))) == (FTYPE *)NULL)
    {  (void)lvector_destroy(squeezed_vector);

       pups_set_errno(ENOMEM);
       return((vlist_type *)NULL);
    }
 
    for(i=0; i<vector->used; ++i)
    {  if(vector->value[i] != 0.0)
       {  squeezed_vector->index[size] = vector->index[i];
          squeezed_vector->value[size] = vector->value[i];
          ++size;
       }
       else
          ++vector_zero_elements;

       if(size == allocated)
       {  allocated += LARRAY_ALLOC_QUANTUM; 

          if((squeezed_vector->index = (int32_t *)pups_malloc(sizeof(int32_t))) == (int32_t *)NULL)
          {  (void)lvector_destroy(squeezed_vector); 

             pups_set_errno(ENOMEM);
             return((vlist_type *)NULL);
          }

          if((squeezed_vector->value = (FTYPE *)pups_calloc(allocated,sizeof(FTYPE))) == (FTYPE *)NULL)
          {  (void)lvector_destroy(squeezed_vector);

             pups_set_errno(ENOMEM);
             return((vlist_type *)NULL);
          }
       }
    }


    /*-------------------------------*/
    /* Initially, vector is inflated */
    /*-------------------------------*/

    squeezed_vector->state = INFLATED;


    /*------------------------------*/
    /* Compute deflation statistics */
    /*------------------------------*/

    if(vector_zero_elements > 0)
    {  

       /*-------------------------*/
       /* Mark vector as deflated */
       /*-------------------------*/

       squeezed_vector->state = DEFLATED;

       if(vector_squeeze_percent != (FTYPE *)NULL)
          *vector_squeeze_percent = squeeze_percent;

       squeeze_percent = 100.0 * (FTYPE)size / (FTYPE)vector->components;
       if(stream != (FILE *)NULL && size != vector->components)
       {  (void)strdate(date);
          (void)fprintf(stream,"%s %s (%d@%s:%s)  List vector deflated by %6.3f %%\n",
                                         date,appl_name,appl_pid,appl_host,appl_owner,
                                                                      squeeze_percent);
          (void)fflush(stream);
       }
    }
    else if(vector_squeeze_percent != (FTYPE *)NULL)
       *vector_squeeze_percent = 0.0;

    (void)strlcpy(squeezed_vector->name,vector->name,SSIZE);
    squeezed_vector->components = vector->components;
    squeezed_vector->allocated  = allocated;
    squeezed_vector->used       = size;


    /*--------------------------------------------*/
    /* Destroy input vector now its been squeezed */
    /*--------------------------------------------*/

    if(keep_vector == FALSE)
       vector = lvector_destroy(vector);

    pups_set_errno(OK);
    return(squeezed_vector);
}




/*---------------------------------------------------------------*/
/* Squeeze zero value elements out of a list matrix deflating it */
/*---------------------------------------------------------------*/

_PUBLIC mlist_type *lmatrix_deflate(const FILE *stream, const _BOOLEAN keep_matrix, FTYPE *matrix_squeeze_percent, mlist_type *matrix)

{    int32_t i,
             cnt = 0;

    FTYPE vector_squeeze_percent,
          squeeze_percent = 0.0;

    mlist_type *squeezed_matrix = (mlist_type *)NULL;

    if(matrix == (mlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return((mlist_type *)NULL);
    }

    if((squeezed_matrix = (mlist_type *)pups_malloc(sizeof(mlist_type))) == (mlist_type *)NULL)
    {  pups_set_errno(ENOMEM);
       return((mlist_type *)NULL);
    }

    if((squeezed_matrix->vector = (vlist_type **)pups_calloc(matrix->rows,sizeof(vlist_type *))) == (vlist_type **)NULL)
    {   (void)pups_free((void *)squeezed_matrix);

        pups_set_errno(ENOMEM);
        return((mlist_type *)NULL);
    }

    squeezed_matrix->rows = matrix->rows;
    squeezed_matrix->cols = matrix->cols;

    for(i=0; i<matrix->rows; ++i)
    {  if(matrix->vector[i] != (vlist_type *)NULL)
       {  if((squeezed_matrix->vector[i] = lvector_deflate((FILE *)NULL,FALSE,&vector_squeeze_percent,matrix->vector[i])) == (vlist_type *)NULL)
          {  pups_set_errno(ENOMEM);
             return((mlist_type *)NULL);
          }

          matrix->vector[i] =  (vlist_type *)NULL;
          squeeze_percent   += vector_squeeze_percent;
          ++cnt;
       } 
    } 

    if(squeeze_percent > 0.0 && stream != (FILE *)NULL)
    {  if(matrix_squeeze_percent != (FTYPE *)NULL)
          *matrix_squeeze_percent = squeeze_percent / (FTYPE)cnt;

       (void)strdate(date);
       (void)fprintf(stream,"%s %s (%d@%s:%s)  List matrix deflated by %5.2F %%\n",
                                      date,appl_name,appl_pid,appl_host,appl_owner,
                                                    (squeeze_percent) / (FTYPE)cnt);
       (void)fflush(stream);
    }


    /*--------------------------------------------*/
    /* Destroy input matrix now its been squeezed */
    /*--------------------------------------------*/

    if(keep_matrix == FALSE)
       (void)lmatrix_destroy(matrix);

    pups_set_errno(OK);
    return(squeezed_matrix);
}




/*-------------------------------------------*/
/* Get the next element in (deflated) vector */
/*-------------------------------------------*/

_PUBLIC FTYPE lvector_get_list_value(const uint32_t pos, uint32_t *component, const vlist_type *vector)

{   if(vector == (vlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(0.0);
    }
    else if(pos < 0 || pos >= vector->used)
    {  pups_set_errno(ERANGE);
       return(0.0);
    }

    pups_set_errno(OK);

    *component = vector->index[pos];
    return(vector->value[pos]); 
} 





/*---------------------------------------------------------------------*/
/* Get list vector component (can be slow as we need to search for it) */
/*---------------------------------------------------------------------*/

_PUBLIC FTYPE lvector_get_component_value(const uint32_t component, const vlist_type *vector)

{   uint32_t i;

    if(vector == (vlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(0.0);
    }
    else if(component < 0 || component >= vector->components)
    {  pups_set_errno(ERANGE);
       return(0.0);
    }

    for(i=0; i<vector->used; ++i)
    {  if(vector->index[i] == component)
       {  pups_set_errno(OK);
          return(vector->value[i]);
       }
    }

    pups_set_errno(OK);
    return(0.0);
}




/*-----------------------------------------------------*/
/* Inflate a list vector so zero componets are present */
/*-----------------------------------------------------*/

_PUBLIC vlist_type *lvector_inflate(const _BOOLEAN keep_vector, vlist_type *vector)

{   uint32_t i;

    vlist_type *inflated_vector = (vlist_type *)NULL;

    if(vector == (vlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return((vlist_type *)NULL);
    }

    if((inflated_vector = lvector_create((FILE *)NULL,vector->components,NULL)) == (vlist_type *)NULL)
    {  pups_set_errno(ENOMEM);
       return((vlist_type *)NULL);
    }


    /*--------------------*/
    /* Inflate the vector */
    /*--------------------*/

    for(i=0; i<vector->used; ++i)
    {  if(vector->value[i] != 0.0)
          inflated_vector->value[vector->index[i]] = vector->value[i];
    } 

    (void)strlcpy(inflated_vector->name,vector->name,SSIZE);
    inflated_vector->allocated = vector->components;
    inflated_vector->used      = vector->components;


    /*-------------------------*/
    /* Mark vector as inflated */
    /*-------------------------*/

    inflated_vector->state = INFLATED;


    /*-------------------------*/
    /* Destroy deflated vector */
    /*-------------------------*/

    if(keep_vector == FALSE)
       (void)lvector_destroy(vector);

    pups_set_errno(OK);
    return(inflated_vector);
}




/*------------------------------------------------------*/
/* Inflate a list matrix so zero components are present */
/*------------------------------------------------------*/

_PUBLIC mlist_type *lmatrix_inflate(_BOOLEAN keep_matrix, mlist_type *matrix)

{   uint32_t i;

    mlist_type *inflated_matrix = (mlist_type *)NULL;

    if(matrix == (mlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return((mlist_type *)NULL);
    }

    if((inflated_matrix = (mlist_type *)pups_malloc(sizeof(mlist_type))) == (mlist_type *)NULL)
    {  pups_set_errno(ENOMEM);
       return((mlist_type *)NULL);
    }

    if((inflated_matrix->vector = (vlist_type **)pups_calloc(matrix->rows,sizeof(vlist_type *))) == (vlist_type **)NULL)
    {   (void)pups_free((void *)inflated_matrix);

        pups_set_errno(ENOMEM);
        return((mlist_type *)NULL);
    }

    inflated_matrix->rows = matrix->rows;
    inflated_matrix->cols = matrix->cols;

    for(i=0; i<matrix->rows; ++i)
    {  if(matrix->vector[i]->used > 0)
       {  if((inflated_matrix->vector[i] = lvector_inflate(TRUE,matrix->vector[i])) == (vlist_type *)NULL)
          {  (void)lmatrix_destroy(inflated_matrix);
 
             pups_set_errno(ENOMEM);
             return((mlist_type *)NULL);
          }
       } 
       else
       {  if((inflated_matrix->vector[i] = lvector_create((FILE *)NULL,matrix->cols,NULL)) == (vlist_type *)NULL)
          {  (void)lmatrix_destroy(inflated_matrix); 

             pups_set_errno(ENOMEM);
             return((mlist_type *)NULL);
          }
       }
    }


    /*----------------------*/
    /* Destroy input matrix */
    /*----------------------*/

    if(keep_matrix == FALSE)
       (void)lmatrix_destroy(matrix);

    pups_set_errno(OK);
    return(inflated_matrix);
}




/*---------------------------------------------------------------*/
/* Get next element in specified row vector of (deflated) matrix */
/*---------------------------------------------------------------*/

_PUBLIC FTYPE lmatrix_get_list_value(const uint32_t row, const uint32_t pos, uint32_t *col, const mlist_type *matrix)

{   if(matrix == (mlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(0.0);
    }
    else if(row < 0 || row > matrix->rows || pos < 0 || pos >= matrix->vector[row]->used)
    {  pups_set_errno(ERANGE);
       return(0.0);
    }
    *col = matrix->vector[row]->index[pos];
  
    pups_set_errno(OK);
    return(matrix->vector[row]->value[pos]); 
}





/*-----------------------------------------------------------------*/
/* Get value of matrix element (may be slow as search is required) */
/*-----------------------------------------------------------------*/

_PUBLIC FTYPE lmatrix_get_element_value(const uint32_t row, const uint32_t col, const mlist_type *matrix)

{    uint32_t i;

     if(matrix == (mlist_type *)NULL)
     {  pups_set_errno(EINVAL);
        return(0.0);
     }
     else if(row < 0 || row > matrix->rows || col < 0 || col > matrix->vector[row]->components)
     {  pups_set_errno(ERANGE);
        return(0.0);
     }

     if(matrix->vector[row] == (vlist_type *)NULL)
     {  pups_set_errno(OK);
        return(0.0);
     }
     else     
     {  for(i=0; i<matrix->vector[row]->used; ++i)
        {  if(matrix->vector[row]->index[i] == col)
           {  pups_set_errno(OK);
              return(matrix->vector[row]->value[i]);
           }
        }
     }

     pups_set_errno(OK);
     return(0.0);
}




/*----------------------*/
/* Normalise list array */
/*----------------------*/

_PUBLIC int32_t lmatrix_normalise(mlist_type *lmatrix)

{   uint32_t i,
             j;

    FTYPE max_val = 0.0;

    if(lmatrix == (mlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    for(i=0; i<lmatrix->rows; ++i)
    {  if(lmatrix->vector[i]->used > 0)
       {  for(j=0; j<lmatrix->vector[i]->used; ++j)
          {  if(max_val < lmatrix->vector[i]->value[j])
                max_val = lmatrix->vector[i]->value[j];
          }
       }
    }
 
    for(i=0; i<lmatrix->rows; ++i)
    {  if(lmatrix->vector[i]->used > 0)
       {  for(j=0; j<lmatrix->vector[i]->used; ++j)
              lmatrix->vector[i]->value[j] /= max_val;
       }
    }

    pups_set_errno(OK);
    return(0);
}




/*-----------------------*/
/* Normalise list vector */
/*-----------------------*/

_PUBLIC int32_t lvector_normalise(vlist_type *vector)

{   uint32_t i;

    FTYPE max_val = 0.0;

    if(vector == (vlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    for(i=0; i<vector->used; ++i)
    {  if(max_val < vector->value[i])
          max_val = vector->value[i];
    }

    for(i=0; i<vector->used; ++i)
       vector->value[i] /= max_val;

    pups_set_errno(OK);
    return(0);
}




/*-------------------*/
/* Print list vector */
/*-------------------*/

_PUBLIC int32_t lvector_print(const FILE *stream, const uint32_t cols, const vlist_type *vector)

{    uint32_t i,
              cnt = 0;

     if(stream == (FILE *)NULL || vector == (vlist_type *)NULL)
     {  pups_set_errno(EINVAL);
        return(-1);
     }


     /*-------------------------*/
     /* Print vector statistics */
     /*-------------------------*/

     if(strcmp(vector->name,"") != 0)
     {  (void)fprintf(stream,"\nName: %s\n",vector->name);
        (void)fflush(stream);
     }

     if(vector->state == DEFLATED)
     {  FTYPE compression_factor;

        compression_factor = 100.0 - (100.0 * (FTYPE)vector->used / (FTYPE)vector->components);

        if(vector->auxdata == (auxdata_type *)NULL)
           (void)fprintf(stream,"\nComponents: %4d deflated allocated %4d used %4d compression %6.3f%%\n",
                                                                                       vector->components,
                                                                                        vector->allocated,
                                                                                             vector->used,
                                                                                       compression_factor);
        else
           (void)fprintf(stream,"\nComponents: %4d deflated allocated %3d used %4d compression %6.3f%% auxdata\n",
                                                                                               vector->components,
                                                                                                vector->allocated,
                                                                                                     vector->used,
                                                                                               compression_factor);
     }
     else
     {  if(vector->auxdata == (auxdata_type *)NULL)
           (void)fprintf(stream,"Components: %4d (inflated)\n",vector->components);
        else
           (void)fprintf(stream,"Components: %4d (inflated) auxdata\n",vector->components);
     }
     (void)fflush(stream);


     /*------------------------*/
     /* No non zero components */
     /*------------------------*/

     if(vector->used == 0)
     {  pups_set_errno(OK);
        return(0);
     }


     /*-----------------------*/
     /* Print deflated vector */
     /*-----------------------*/

     if(vector->state == DEFLATED)
     {  for(i=0; i<vector->used; ++i)
        {  (void)fprintf(stream,"%-4d %15E",vector->index[i],vector->value[i]);
           (void)fflush(stream);

           if(i == vector->used - 1 || cnt == cols)
           {  (void)fprintf(stream,"\n");
              cnt = 0;
           }
           else
           {  (void)fprintf(stream,"\t");
              ++cnt;
           }

           (void)fflush(stream);
        }
     }


     /*-----------------------*/
     /* Print inflated vector */
     /*-----------------------*/

     else
     {  for(i=0; i<vector->components; ++i)
        {  (void)fprintf(stream,"%15E",vector->value[i]);
           (void)fflush(stream);

           if(i == vector->components - 1 || cnt == cols)
           {  (void)fprintf(stream,"\n");
              cnt = 0;
           }
           else
           {  (void)fprintf(stream,"\t");
              ++cnt;
           }
        }
     }


     /*-------------------------------*/
     /* Save auxiallary data (if any) */
     /*-------------------------------*/

     for(i=0; i<vector->used; ++i)
     {  if(vector->auxdata != (auxdata_type *)NULL)       
        {   _PROTOTYPE  int32_t (*save)(FILE *, void *); 

            if(vector->auxdata[i].save == (void *)NULL || vector->auxdata[i].data == (void *)NULL)
            {  pups_set_errno(EINVAL);
               return(-1);
            }

            save = vector->auxdata[i].save;
            if((*save)(stream,vector->auxdata[i].data) == (-1))
            {  pups_set_errno(EPERM);
               return(-1);
            }
        }
     }

     (void)fprintf(stream,"\n"); 
     (void)fflush(stream);
 
     pups_set_errno(OK);
     return(0);
}



/*-----------------------------------------------------------------------*/
/* Save list vector to (ASCII) file - note we cannot save auxillary data */
/* in ASCII mode                                                         */
/*-----------------------------------------------------------------------*/

_PUBLIC  int32_t lvector_save_to_file(const char *filename, const vlist_type *vector)

{   FILE *stream = (FILE *)NULL;

    if(filename == (char *)NULL || strcmp(filename,"") == 0 || vector == (vlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if(access(filename,F_OK | R_OK | W_OK) == (-1))
    {  des_t fildes = (-1);

       if(fildes = creat(filename,0600) == (-1))
       {  pups_set_errno(EACCES);
          return(-1);
       }
       else
          (void)close(fildes);
    }

    if((stream = pups_fopen(filename,"w",LIVE)) == (FILE *)NULL)
    {  pups_set_errno(EACCES);
       return(-1);
    }

    (void)fprintf(stream,"\n#--------------------------------------------------------------------------\n");
    (void)fprintf(stream,"#    List vector (ASCII format 1.00)\n");
    (void)fprintf(stream,"#    (C) M.A. O'Neill, Tumbling Dice 2024\n");
    (void)fprintf(stream,"#--------------------------------------------------------------------------\n\n");
    (void)fflush(stream);

    if(lvector_print(stream,4,vector) == (-1))
    {  (void)pups_fclose(stream);

       pups_set_errno(EINVAL);
       return(-1);
    }
    else
       (void)pups_fclose(stream);

    pups_set_errno(OK);
    return(0);
}




/*-----------------------------------*/
/* Save list vector to (binary) file */
/*-----------------------------------*/

_PUBLIC int32_t lvector_save_to_binary_file(const char *filename, const vlist_type *vector)

{   int32_t i;
    des_t    fildes = (-1);

    if(filename == (char *)NULL || strcmp(filename,"") == 0 || vector == (vlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if(access(filename,F_OK | R_OK | W_OK) == (-1))
    {   int32_t fildes = (-1);

       if(fildes = creat(filename,0600) == (-1))
       {  pups_set_errno(EACCES);
          return(-1);
       }
       else
          (void)close(fildes);
    }

    if((fildes = pups_open(filename,2,LIVE)) == (-1))
    {  pups_set_errno(EACCES);
       return(-1);
    }

    if(lvector_save_to_binary_fildes(fildes,vector) == (-1))
    {  (void)pups_close(fildes);
       return(-1);
    }
    else 
       (void)pups_close(fildes);

    pups_set_errno(OK);
    return(0);
}




/*------------------------------------------*/
/* Save list vector to open file descriptor */
/*------------------------------------------*/

_PUBLIC int32_t lvector_save_to_binary_fildes(const des_t fildes, const vlist_type *vector)

{   uint32_t i;

    if(fildes < 0)
    {  pups_set_errno(EBADFD);
       return(-1);
    }
    else if(vector == (vlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if(write(fildes,VLIST_MAGIC,5*sizeof(char)) != 5*sizeof(char))
    {  pups_set_errno(EACCES);
       return(-1);
    }

    if(write(fildes,vector->name,SSIZE*sizeof(char)) != SSIZE*sizeof(char))
    {  pups_set_errno(EACCES);
       return(-1);
    }

    if(write(fildes,&vector->state,sizeof(int32_t)) != sizeof(int32_t))
    {  pups_set_errno(EACCES);
       return(-1);
    }

    if(write(fildes,&vector->components,sizeof(int32_t)) != sizeof(int32_t))
    {  pups_set_errno(EACCES);
       return(-1);
    }

    if(write(fildes,&vector->allocated,sizeof(int32_t)) != sizeof(int32_t))
    {  pups_set_errno(EACCES);
       return(-1);
    }

    if(write(fildes,&vector->used,sizeof(int32_t))  != sizeof(int32_t))
    {  pups_set_errno(EACCES);
       return(-1);
    }

    if(write(fildes,vector->index,vector->used*sizeof(int32_t)) != vector->allocated*sizeof(int32_t))
    {  pups_set_errno(EACCES);
       return(-1);
    }
     
    if(write(fildes,vector->value,vector->used*sizeof(FTYPE)) != vector->allocated*sizeof(FTYPE))
    {  pups_set_errno(EACCES);
       return(-1);
    }


    /*-------------------------------*/     
    /* Save auxiallary data (if any) */
    /*-------------------------------*/     

    for(i=0; i<vector->used; ++i)
    {  if(vector->auxdata != (auxdata_type *)NULL)
       {   _PROTOTYPE int32_t (*bsave)(int, void *);

           if(vector->auxdata[i].bsave == (void *)NULL || vector->auxdata[i].data == (void *)NULL)
           {  pups_set_errno(EINVAL);
              return(-1);
           }

           bsave = vector->auxdata[i].bsave;
           if((*bsave)(fildes,vector->auxdata[i].data) == (-1))
           {  pups_set_errno(EPERM);
              return(-1);
           }
       }
    }

    pups_set_errno(OK);
    return(0);
}




/*--------------------*/
/* Scan a list vector */
/*--------------------*/

_PUBLIC vlist_type *lvector_scan(const FILE *stream)

{   uint32_t i,
             allocated,
             used,
             components;

    char strdum[SSIZE] = "",
         state[SSIZE]  = "",
         name[SSIZE]   = "",
         item[SSIZE]   = "";

    vlist_type *vector = (vlist_type *)NULL;

    if(stream == (FILE *)NULL)
    {  pups_set_errno(EBADF);
       return((vlist_type *)NULL);
    } 

    (void)skip_comments(stream);
    if(fgets(item,SSIZE,stream) == (char *)NULL)
    {  pups_set_errno(EBADF);
       return((vlist_type *)NULL);
    }


    /*----------------------*/
    /* Is the vector named? */
    /*----------------------*/

    if(strncmp(item,"Name:",5) == 0)
    {  if(sscanf(item,"%s%s",strdum,name) < 2)
       {  pups_set_errno(EBADF);
          return((vlist_type *)NULL);
       }

       (void)skip_comments(stream);
       if(fgets(item,SSIZE,stream) == (char *)NULL)
       {  pups_set_errno(EBADF);
          return((vlist_type *)NULL);
       }
    }

    if(sscanf(item,"%s%d%s%s%d%s%d",strdum,&components,state,strdum,&allocated,strdum,&used) < 7)
    {  pups_set_errno(EBADF);
       return((vlist_type *)NULL);
    }

    if((vector = pups_malloc(sizeof(vlist_type))) == (vlist_type *)NULL)
    {  pups_set_errno(ENOMEM);
       return((vlist_type *)NULL);
    }

    vector->components = components;
    vector->allocated  = used;
    vector->used       = used;

    if(strcmp(state,"inflated") == 0)
       vector->state = INFLATED;
    else if(strcmp(state,"deflated") == 0)
       vector->state = DEFLATED;

    (void)strlcpy(vector->name,name,SSIZE);


    /*------------------------*/
    /* No non-zero components */
    /*------------------------*/

    if(vector->used == 0)
    {  (void)pups_set_errno(OK);
       return(vector);
    }

    if((vector->index = (int32_t *)pups_calloc(vector->allocated,sizeof(int32_t))) == (int32_t *)NULL)
    {  (void)lvector_destroy(vector);

       pups_set_errno(EINVAL);
       return((vlist_type *)NULL);
    }

    if((vector->value = (FTYPE *)pups_calloc(vector->allocated,sizeof(FTYPE))) == (FTYPE *)NULL)
    {  (void)lvector_destroy(vector);

       pups_set_errno(EINVAL);
       return((vlist_type *)NULL);
    }

    (void)skip_comments(stream);
    for(i=0; i<vector->used; ++i)
    {  if(vector->state == DEFLATED)
       {  if(fscanf(stream,"%4d%15E",&vector->index[i],&vector->value[i]) != 2)
          {  (void)lvector_destroy(vector);

             pups_set_errno(EINVAL);
             return((vlist_type *)NULL);
          }
       }
       else if(vector->state == INFLATED)
       {  if(fscanf(stream,"%15E",&vector->value[i]) != 1)
          {  (void)lvector_destroy(vector);

             pups_set_errno(EINVAL);
             return((vlist_type *)NULL);
          }
          else
             vector->index[i] = i;
       }
    }


    /*------------------------------*/
    /* Get auxiallary data (if any) */
    /*------------------------------*/

    for(i=0; i<vector->used; ++i)
    {  if(vector->auxdata != (auxdata_type *)NULL)
       {   _PROTOTYPE void *(*load)(FILE *, void *);

           if(vector->auxdata[i].load == (void *)NULL || vector->auxdata[i].data == (void *)NULL)
           {  pups_set_errno(EINVAL);
              return((vlist_type *)NULL);
           }

           load = vector->auxdata[i].load;
           if((vector->auxdata[i].data = (*load)(stream,vector->auxdata[i].data)) == (void *)NULL)
           {  pups_set_errno(EPERM);
              return((vlist_type *)NULL);
           }
       }
    }

    pups_set_errno(OK);
    return(vector);
}




/*------------------------------------------------*/
/* Skip lines starting with '#' or "\n" character */
/*------------------------------------------------*/

_PRIVATE int32_t skip_comments(const FILE *stream)

{   char next_char,
         line[SSIZE] = "";

    do {    next_char = fgetc(stream);


            /*-----------------------------*/
            /* Strip comment if we need to */
            /*-----------------------------*/

            if(next_char == '#')
               (void)fgets(line,SSIZE,stream);

       } while (next_char == ' ' || next_char == '\t' || next_char == '\n' || next_char == '#');


    /*---------------------------------*/
    /* Push character back into stream */
    /*---------------------------------*/

    (void)ungetc(next_char,stream);
    return(0);
}




/*------------------------------------*/
/* Load list vector from (ASCII) file */
/*------------------------------------*/

_PUBLIC vlist_type *lvector_load_from_file(const char *filename)

{   int32_t    i;
    char       strdum[SSIZE]   = ""; 
    FILE       *stream       = (FILE *)NULL;
    vlist_type *vector = (vlist_type *)NULL;

    if(filename == (char *)NULL || strcmp(filename,"") == 0)
    {  pups_set_errno(EINVAL);
       return((vlist_type *)NULL);
    }

    if((stream = pups_fopen(filename,"r",LIVE)) == (FILE *)NULL)
    {  pups_set_errno(EACCES);
       return((vlist_type *)NULL);
    }


    /*------------------*/
    /* Skip file header */
    /*------------------*/

    (void)skip_comments(stream);

    if((vector = lvector_scan(stream)) == (vlist_type *)NULL)
    {  (void)pups_fclose(stream);

       pups_set_errno(EINVAL);
       return((vlist_type *)NULL);
    }
    else
       (void)pups_fclose(stream);

    pups_set_errno(OK);
    return(vector);
}




/*-------------------------------------*/
/* Load list vector from (binary) file */
/*-------------------------------------*/

_PUBLIC vlist_type *lvector_load_from_binary_file(const char *filename)

{   des_t      fildes  = (-1);
    vlist_type *vector = (vlist_type *)NULL;

    if(filename == (char *)NULL || strcmp(filename,"") == 0)
    {  pups_set_errno(EINVAL);
       return((vlist_type *)NULL);
    }

    if((fildes = pups_open(filename,2,LIVE)) == (-1))
    {  (void)pups_free((void *)vector);

       pups_set_errno(EACCES);
       return((vlist_type *)NULL);
    }

    if((vector = lvector_load_from_binary_fildes(fildes)) == (vlist_type *)NULL)
    {  (void)pups_close(fildes);
       return((vlist_type *)NULL);
    }
    else
       (void)pups_close(fildes);

    pups_set_errno(OK);
    return(vector);
}




/*----------------------------------------------*/
/* Save list vector to (binary) file descriptor */
/*----------------------------------------------*/

_PUBLIC vlist_type *lvector_load_from_binary_fildes(const des_t fildes)

{   uint32_t   i;

    char       vector_magic[5] = "";
    vlist_type *vector         = (vlist_type *)NULL;

    if(fildes < 0)
    {  pups_set_errno(EBADFD);
       return((vlist_type *)NULL);
    }

    if((vector = (vlist_type *)pups_malloc(sizeof(vlist_type))) == (vlist_type *)NULL)
    {  pups_set_errno(ENOMEM);
       return((vlist_type *)NULL);
    }

    if(read(fildes,vector_magic,5*sizeof(char)) != 5*sizeof(char))
    {  (void)pups_free((void *)vector);

       pups_set_errno(EACCES);
       return((vlist_type *)NULL);
    }
    else if(strncmp(vector_magic,VLIST_MAGIC,5) != 0)
    {  (void)pups_free((void *)vector);

       pups_set_errno(EBADF);
       return((vlist_type *)NULL);
    }

    if(read(fildes,vector->name,SSIZE*sizeof(char)) != SSIZE*sizeof(char))
    {  (void)pups_free((void *)vector);

       pups_set_errno(EACCES);
       return((vlist_type *)NULL);
    }

    if(read(fildes,&vector->state,sizeof(int32_t)) != sizeof(int32_t))
    {  (void)pups_free((void *)vector);

       pups_set_errno(EACCES);
       return((vlist_type *)NULL);
    }
 
    if(read(fildes,&vector->components,sizeof(int32_t)) != sizeof(int32_t))
    {  (void)pups_free((void *)vector);

       pups_set_errno(EACCES);
       return((vlist_type *)NULL);
    }

    if(read(fildes,&vector->allocated,sizeof(int32_t)) != sizeof(int32_t))
    {  (void)pups_free((void *)vector);

       pups_set_errno(EACCES);
       return((vlist_type *)NULL);
    }

    if(read(fildes,&vector->used,sizeof(int32_t)) != sizeof(int32_t))
    {  (void)pups_free((void *)vector);

       pups_set_errno(EACCES);
       return((vlist_type *)NULL);
    }

    if((vector->index = (int32_t *)pups_calloc(vector->used,sizeof(int32_t))) == (int32_t *)NULL)
    {  (void)pups_free((void *)vector);

       pups_set_errno(EACCES);
       return((vlist_type *)NULL);
    }

    if((vector->value = (FTYPE *)pups_calloc(vector->used,sizeof(FTYPE))) == (FTYPE *)NULL)
    {  (void)pups_free((void *)vector->index); 
       (void)pups_free((void *)vector);

       pups_set_errno(EACCES);
       return((vlist_type *)NULL);
    }

    if(read(fildes,vector->index,vector->used*sizeof(int32_t)) != vector->used*sizeof(int32_t))
    {  (void)pups_free((void *)vector->index);
       (void)pups_free((void *)vector->value);
       (void)pups_free((void *)vector);

       pups_set_errno(EACCES);
       return((vlist_type *)NULL);
    }

    if(read(fildes,vector->value,vector->used*sizeof(FTYPE)) != vector->used*sizeof(FTYPE))
    {  (void)pups_free((void *)vector->index);
       (void)pups_free((void *)vector->value);
       (void)pups_free((void *)vector);

       pups_set_errno(EACCES);
       return((vlist_type *)NULL);
    }


    /*------------------------------*/
    /* Get auxiallary data (if any) */
    /*------------------------------*/

    for(i=0; i<vector->used; ++i)
    {  if(vector->auxdata != (auxdata_type *)NULL)
       {   _PROTOTYPE void *(*bload)(int32_t, void *);

           if(vector->auxdata[i].bload == (void *)NULL || vector->auxdata[i].data == (void *)NULL)
           {  pups_set_errno(EINVAL);
              return((vlist_type *)NULL);
           }

           bload = vector->auxdata[i].bload;
           if((vector->auxdata[i].data = (*bload)(fildes,vector->auxdata[i].data)) == (void *)NULL)
           {  pups_set_errno(EPERM);
              return((vlist_type *)NULL);
           }
       }
    }

    pups_set_errno(OK);
    return(vector);
}




/*-------------------*/
/* Print list matrix */
/*-------------------*/

_PUBLIC int32_t lmatrix_print(const FILE *stream, const uint32_t cols, const mlist_type *matrix)

{   uint32_t i;

    if(stream == (FILE *)NULL || matrix == (mlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    /*-------------------------*/
    /* Print vector statistics */
    /*-------------------------*/

    if(strcmp(matrix->name,"") != 0)
    {  (void)fprintf(stream,"\nName: %s\n",matrix->name);
       (void)fflush(stream);
    }

    (void)fprintf(stream,"\n%4d rows x %4d cols\n",matrix->rows,matrix->cols);
    (void)fflush(stderr);

    (void)fprintf(stream,"\n");
    (void)fflush(stderr);

    for(i=0; i<matrix->rows; ++i)
    {  (void)fprintf(stream,"# Row %4d ",i);
       (void)fflush(stream);
 
       if(lvector_print(stream,cols,matrix->vector[i]) == (-1))
       {  pups_set_errno(EINVAL);
          return(-1);
       }
    }

    (void)fprintf(stream,"\n");
    (void)fflush(stderr);

    pups_set_errno(OK);
    return(0);
}




/*----------------------------------*/
/* Save list matrix to (ASCII) file */
/*----------------------------------*/

_PUBLIC int32_t lmatrix_save_to_file(const char *filename, const mlist_type *matrix)

{   FILE *stream = (FILE *)NULL;

    if(filename == (char *)NULL || strcmp(filename,"") == 0 || matrix == (mlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if(access(filename,F_OK | R_OK | W_OK) == (-1))
    {   int32_t fildes = (-1);

       if(fildes = creat(filename,0600) == (-1))
       {  pups_set_errno(EACCES);
          return(-1);
       }
       else
          (void)close(fildes);
    }

    if((stream = pups_fopen(filename,"w",LIVE)) == (FILE *)NULL)
    {  pups_set_errno(EACCES);
       return(-1);
    }

    (void)fprintf(stream,"\n#--------------------------------------------------------------------------\n");
    (void)fprintf(stream,"#    List matrix (ASCII format 1.00)\n");
    (void)fprintf(stream,"#    (C) M.A. O'Neill, Tumbling Dice 2024\n");
    (void)fprintf(stream,"#--------------------------------------------------------------------------\n\n");
    (void)fflush(stream);

    if(lmatrix_print(stream,4,matrix) == (-1))
    {  (void)pups_fclose(stream);

       pups_set_errno(EINVAL);
       return(-1);
    }
    else
       (void)pups_fclose(stream);
    
    pups_set_errno(OK);
    return(0);
}





/*-----------------------------------*/
/* Save list matrix to (binary) file */
/*-----------------------------------*/

_PUBLIC int32_t lmatrix_save_to_binary_file(const char *filename, const mlist_type *matrix)

{   des_t fildes = (-1);

    if(filename == (char *)NULL || strcmp(filename,"") == 0 || matrix == (mlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if(access(filename,F_OK | R_OK | W_OK) == (-1))
    {  if(fildes = creat(filename,0600) == (-1))
       {  pups_set_errno(EACCES);
          return(-1);
       }
       else
          (void)close(fildes);
    }

    if((fildes = pups_open(filename,2,LIVE)) == (-1))
    {  pups_set_errno(EACCES);
       return(-1);
    }

    if(lmatrix_save_to_binary_fildes(fildes,matrix) == (-1))
    {  (void)pups_close(fildes);
       return(-1);
    }
    else
       (void)pups_close(fildes);

    pups_set_errno(OK);
    return(0);
}




/*----------------------------------------------*/
/* Save list matrix to (binary) file descriptor */
/*----------------------------------------------*/

_PUBLIC int32_t lmatrix_save_to_binary_fildes(const des_t fildes, const mlist_type *matrix)

{   uint32_t i;

    if(fildes < 0)
    {  pups_set_errno(EBADFD);
       return(-1);
    }
    else if(matrix == (mlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if(write(fildes,MLIST_MAGIC,5*sizeof(char)) != 5*sizeof(char))
    {  pups_set_errno(EACCES);
       return(-1);
    }

    if(write(fildes,matrix->name,SSIZE*sizeof(char)) != SSIZE*sizeof(char))
    {  pups_set_errno(EACCES);
       return(-1);
    }

    if(write(fildes,&matrix->rows,sizeof(int32_t)) != sizeof(int32_t))
    {  pups_set_errno(EACCES);
       return(-1);
    }

    if(write(fildes,&matrix->cols,sizeof(int32_t)) != sizeof(int32_t))
    {  pups_set_errno(EACCES);
       return(-1);
    }

    for(i=0; i<matrix->rows; ++i)
    {   if(lvector_save_to_binary_fildes(fildes,matrix->vector[i]) == (-1))
           return(-1);
    }

    pups_set_errno(OK);
    return(0);
}





/*--------------------*/
/* Scan a list matrix */
/*--------------------*/

_PUBLIC mlist_type *lmatrix_scan(const FILE *stream)

{   uint32_t i,
             rows,
             cols;

    char strdum[SSIZE] = "",
         name[SSIZE]   = "",
         item[SSIZE]   = "";

    mlist_type *matrix = (mlist_type *)NULL;

    if(stream == (FILE *)NULL)
    {  pups_set_errno(EBADF);
       return((mlist_type *)NULL);
    }

    (void)skip_comments(stream);
    if(fgets(item,SSIZE,stream) == (char *)NULL)
    {  pups_set_errno(EBADF);
       return((mlist_type *)NULL);
    }


    /*----------------------*/
    /* Is the matrix named? */
    /*----------------------*/

    if(strncmp(item,"Name:",5) == 0)
    {  if(sscanf(item,"%s%s",strdum,name) < 2)
       {  pups_set_errno(EBADF);
          return((mlist_type *)NULL);
       }

       (void)skip_comments(stream);
       if(fgets(item,SSIZE,stream) == (char *)NULL)
       {  pups_set_errno(EBADF);
          return((mlist_type *)NULL);
       }
    }

    if(sscanf(item,"%d%s%s%d",&rows,strdum,strdum,&cols) < 4)
    {  pups_set_errno(EBADF);
       return((mlist_type *)NULL);
    }

    if((matrix = (mlist_type *)pups_malloc(sizeof(mlist_type))) == (mlist_type *)NULL)
    {  pups_set_errno(ENOMEM);
       return((mlist_type *)NULL);
    }

    if((matrix->vector = (vlist_type **)pups_calloc(rows,sizeof(vlist_type *))) == (vlist_type **)NULL)
    {  (void)pups_free((void *)matrix);

       pups_set_errno(ENOMEM);
       return((mlist_type *)NULL);
    }

    (void)skip_comments(stream);
    for(i=0; i<rows; ++i)
    {  if((matrix->vector[i] = lvector_scan(stream)) == (vlist_type *)NULL)
       {  (void)lmatrix_destroy(matrix);

          pups_set_errno(ENOMEM);
          return((mlist_type *)NULL);
       }
    }

    (void)strlcpy(matrix->name,name,SSIZE);
    matrix->rows = rows;
    matrix->cols =cols;
  
    pups_set_errno(OK);
    return(matrix);

}




/*------------------------------------*/
/* Load list matrix from (ASCII) file */
/*------------------------------------*/

_PUBLIC mlist_type *lmatrix_load_from_file(const char *filename)

{   uint32_t    i;
    char        strdum[SSIZE]  = "";
    FILE        *stream       = (FILE *)NULL;
    mlist_type  *matrix = (mlist_type *)NULL;

    if(filename == (char *)NULL || strcmp(filename,"") == 0)
    {  pups_set_errno(EINVAL);
       return((mlist_type *)NULL);
    }

    if((stream = pups_fopen(filename,"r",LIVE)) == (FILE *)NULL)
    {  pups_set_errno(EACCES);
       return((mlist_type *)NULL);
    }


    /*------------------*/
    /* Skip file header */
    /*------------------*/

    (void)skip_comments(stream);

    if((matrix = lmatrix_scan(stream)) == (mlist_type *)NULL)
    {  (void)pups_fclose(stream);

       pups_set_errno(EINVAL);
       return((mlist_type *)NULL);
    }
    else
       (void)pups_fclose(stream);

    pups_set_errno(OK);
    return(matrix);
}




/*-------------------------------------*/
/* Load list matrix from (binary) file */
/*-------------------------------------*/

_PUBLIC mlist_type *lmatrix_load_from_binary_file(const char *filename)

{   des_t      fildes  = (-1);
    mlist_type *matrix = (mlist_type *)NULL;

    if(filename == (char *)NULL || strcmp(filename,"") == 0)
    {  pups_set_errno(EINVAL);
       return((mlist_type *)NULL);
    }

    if((fildes = pups_open(filename,2,LIVE)) == (-1))
    {  pups_set_errno(EACCES);
       return((mlist_type *)NULL);
    }

    if((matrix = lmatrix_load_from_binary_fildes(fildes)) == (mlist_type *)NULL)
    {  (void)pups_close(fildes);
       return(mlist_type *)NULL;
    }
    else
       (void)pups_close(fildes);

    pups_set_errno(OK);
    return(matrix);
}






/*----------------------------------------------*/
/* Save list matrix to (binary) file descriptor */
/*----------------------------------------------*/

_PUBLIC mlist_type *lmatrix_load_from_binary_fildes(const des_t fildes)

{   uint32_t   i;
    char        matrix_magic[5] = "";
    mlist_type  *matrix         = (mlist_type *)NULL;

    if(fildes < 0)
    {  pups_set_errno(EBADFD);
       return((mlist_type *)NULL);
    }

    if((matrix = (mlist_type *)pups_malloc(sizeof(mlist_type))) == (mlist_type *)NULL)
    {  pups_set_errno(ENOMEM);
       return((mlist_type *)NULL);
    }

    if(read(fildes,matrix_magic,5*sizeof(char)) != 5*sizeof(char))
    {  (void)pups_free((void *)matrix);

       pups_set_errno(EACCES);
       return((mlist_type *)NULL);
    }
    else if(strncmp(matrix_magic,MLIST_MAGIC,5) != 0)
    {  (void)pups_free((void *)matrix);

       pups_set_errno(EBADF);
       return((mlist_type *)NULL);
    }

    if(read(fildes,matrix->name,SSIZE*sizeof(char)) != SSIZE*sizeof(char))
    {  (void)pups_free((void *)matrix);

       pups_set_errno(EACCES);
       return((mlist_type *)NULL);
    }

    if(read(fildes,&matrix->rows,sizeof(int32_t)) != sizeof(int32_t))
    {  (void)pups_free((void *)matrix);

       pups_set_errno(EACCES);
       return((mlist_type *)NULL);
    }

    if(read(fildes,&matrix->cols,sizeof(int32_t)) != sizeof(int32_t))
    {  (void)pups_free((void *)matrix);

       pups_set_errno(EACCES);
       return((mlist_type *)NULL);
    }

    if((matrix->vector = (vlist_type **)pups_calloc(matrix->rows,sizeof(vlist_type *))) == (vlist_type **)NULL)
    {  (void)pups_free((void *)matrix);

       pups_set_errno(ENOMEM);
       return((mlist_type *)NULL);
    }

    for(i=0; i<matrix->rows; ++i)
    {  if((matrix->vector[i] = lvector_load_from_binary_fildes(fildes)) == (vlist_type *)NULL)
       {  (void)lmatrix_destroy(matrix);
          return((mlist_type *)NULL);
       }
    }

    pups_set_errno(OK);
    return(matrix);
}





/*----------------------*/
/* Set list vector name */
/*----------------------*/

_PUBLIC int32_t lvector_set_name(const char *name, vlist_type *vector)

{   if(name == (char *)NULL || strcmp(name,"") == 0 || vector == (vlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    (void)strlcpy(vector->name,name,SSIZE);
    pups_set_errno(OK);
    return(0);
}




/*----------------------*/
/* Set list matrix name */
/*----------------------*/

_PUBLIC int32_t lmatrix_set_name(const char *name, mlist_type *matrix)

{   if(name == (char *)NULL || strcmp(name,"") == 0 || matrix == (mlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    (void)strlcpy(matrix->name,name,SSIZE);
    pups_set_errno(OK);
    return(0);
}




/*---------------------------------*/
/* Get list vector inflation state */
/*---------------------------------*/

_PUBLIC int32_t lvector_get_state(const vlist_type *vector)

{   if(vector == (vlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    pups_set_errno(OK);
    return(vector->state);
}




/*----------------------*/
/* Get list vector size */
/*----------------------*/

_PUBLIC uint32_t lvector_get_size(const vlist_type *vector)

{   if(vector == (vlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    } 

    pups_set_errno(OK);
    return(vector->components);
}




/*----------------------*/
/* Get list matrix size */
/*----------------------*/

_PUBLIC uint32_t lmatrix_get_size(const mlist_type *matrix, uint32_t *rows, uint32_t *cols)

{   if(matrix == (mlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    *rows = matrix->rows;
    *cols = matrix->cols;

    pups_set_errno(OK);
    return(0);
}




/*-----------------------------------*/
/* Get allocated size of list vector */
/*-----------------------------------*/

_PUBLIC uint32_t lvector_get_allocated(const vlist_type *vector)

{   if(vector == (vlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    pups_set_errno(OK);
    return(vector->allocated);
}




/*---------------------------------------------------------------------*/
/* Get size of list vector (non zero elements in correspondign vector) */
/*---------------------------------------------------------------------*/

_PUBLIC uint32_t lvector_get_used(const vlist_type *vector)

{   if(vector == (vlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    pups_set_errno(OK);
    return(vector->used);
}




/*------------------------------------*/
/* Get list vector compression factor */
/*------------------------------------*/

_PUBLIC FTYPE lvector_get_compression_factor(const vlist_type *vector)

{   FTYPE compression_factor;

    if(vector == (vlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    if(vector->state == INFLATED)
    {  pups_set_errno(OK);
       return(0.0);
    }

    compression_factor = 100.0 - (100.0 * (FTYPE)vector->allocated / (FTYPE)vector->components);

    pups_set_errno(OK);
    return(compression_factor);
}




/*------------------------------------*/
/* Get list matrix compression factor */
/*------------------------------------*/

_PUBLIC FTYPE lmatrix_get_compression_factor(const mlist_type *matrix)

{   uint32_t i;

    FTYPE compression_factor,
          sum_compression_factor = 0.0;

    if(matrix == (mlist_type *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

    for(i=0; i<matrix->rows; ++i)
    {  if(matrix->vector[i]->used > 0)
          compression_factor = 100.0 - (100.0 * (FTYPE)matrix->vector[i]->used / (FTYPE)matrix->vector[i]->components);
       else
          compression_factor = 100.0;

       sum_compression_factor += compression_factor;
    }
    sum_compression_factor /= (FTYPE)matrix->rows;

    pups_set_errno(OK);
    return(compression_factor);    
}
