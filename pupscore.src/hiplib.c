/*------------------------------------------------------------------------------
    Copyright (C) 1982 Michael Landy, Yoav Cohen, and George Sperling

    Disclaimer:  No guarantees of performance accompany this software,
    nor is any responsibility assumed on the part of the authors.  All the
    software has been tested extensively and every effort has been made to
    insure its reliability. 

    Modified:  M.A. O'Neill
               Tumbling Dice Ltd
               Gosforth
               Newcastle upon Tyne
               NE3 4RT
               United Kingdom

    Version:  2.02 
    Dated:    7th January 2022
    E-mail:   mao@tumblingdice.co.uk
------------------------------------------------------------------------------*/


#include <me.h>
#include <utils.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <pwd.h>

#if defined(TIFF_SUPPORT) && defined(FLORET_TIFF_SUPPORT)
#include <floret.h>
#endif /* TIFF_SUPPORT && FLORET_TIFF_SUPPORT */

#undef   __NOT_LIB_SOURCE__
#include <hipl_hdr.h>
#define  __NOT_LIB_SOURCE__



/*------------------------------------------------------------------------------
    Slot and usage functions - used by slot manager ...
------------------------------------------------------------------------------*/
/*---------------------*/
/* Slot usage function */
/*---------------------*/

_PRIVATE void hiplib_slot(int level)
{   (void)fprintf(stderr,"lib hiplib %s: [ANSI C]\n",HIPLIB_VERSION);

    if(level > 1)
    {  (void)fprintf(stderr,"(C) 1987-2022 Tumbling Dice\n");
       (void)fprintf(stderr,"Author: M.A. O'Neill\n");
       (void)fprintf(stderr,"PUPS/P3 HIPS support library (built %s %s)\n\n",__TIME__,__DATE__);
    }
    else
       (void)fprintf(stderr,"\n");
    (void)fflush(stderr);
}




/*-------------------------------------------------*/
/* Segment identification for hips support library */
/*-------------------------------------------------*/

#ifdef SLOT
#include <slotman.h>
_EXTERN void (* SLOT )() __attribute__ ((aligned(16))) = hiplib_slot;
#endif /* SLOT */




/*------------------------------------------------------------------------------
    Functions which are local to this library ...
------------------------------------------------------------------------------*/


/*----------------------------------------*/
/* Swap bytes if we have an endian change */
/*----------------------------------------*/

_PROTOTYPE _PRIVATE void swap_bytes(_USHORT *);




/*------------------------------------------------------------------------------
    Dynamically allocate memory for HIPL data array ...
------------------------------------------------------------------------------*/

_PUBLIC void **hips_fralloc(hipl_hdr *hdr)

{   int   element_size;
    void  **ret = (void **)NULL;

    if(hdr == (hipl_hdr *)NULL)
    {  pups_set_errno(EINVAL);
       return((void **)NULL);
    }

    element_size = hips_pixel_size(hdr->pixel_format);
    ret          = (void *)pups_aalloc(hdr->rows,hdr->cols,element_size);

    pups_set_errno(OK);
    return(ret);
}





/*------------------------------------------------------------------------------
    Dynamically allocate data for n rows of HIPL data ...
------------------------------------------------------------------------------*/

_PUBLIC void **hips_nralloc(int n_rows, hipl_hdr *hdr)

{   int element_size;
    void **ret = (void **)NULL;

    if(n_rows < 0 || hdr == (hipl_hdr *)NULL)
    {  pups_set_errno(EINVAL);
       return((void **)NULL);
    }

    if(n_rows > hdr->rows)
       pups_error("[hips_nralloc] more rows specified than present in image");

     element_size = hips_pixel_size(hdr->pixel_format);
     ret = (void **)pups_aalloc(n_rows,hdr->cols,element_size);

     pups_set_errno(OK);
     return(ret);
}




/*------------------------------------------------------------------------------
    Create a one line output buffer ...
------------------------------------------------------------------------------*/


_PUBLIC void *hips_ralloc(hipl_hdr *hdr)

{   int   element_size;
    void *ret = (void *)NULL;

    if(hdr == (hipl_hdr *)NULL)
    {  pups_set_errno(EINVAL);
       return((void *)NULL);
    }

    element_size = hips_pixel_size(hdr->pixel_format);
    ret = (void *)pups_calloc(hdr->cols,element_size);

    pups_set_errno(OK);
    return(ret);
}



 
/*------------------------------------------------------------------------------
    desc_massage.c - HIPL Picture Format description massager.
    Michael Landy - 2/2/82
------------------------------------------------------------------------------*/

_PRIVATE char *hips_desc_massage(char *s)

{  int len,
         i,
         k,
         beg;

    if(s == (char *)NULL || strcmp(s,"") == 0)
    {  pups_set_errno(EINVAL);
       return((char *)NULL);
    }

    len = strlen(s);
    beg = 1;

    for (i=0;i<len;i++)
    {   if (beg && s[i] == '.' && s[i+1] == '\n')
        {
            for (k=i+2;k<=len;k++)
                s[k-2] = s[k];
            i--;
            len -= 2;
        }
        else if (s[i] == '\n')
           beg = 1;
        else
           beg = 0;
    }

    pups_set_errno(OK);
    return(s);
}




/*------------------------------------------------------------------------------
    hips_alloc - HIPL Picture Format core allocation.
    Michael Landy 2/1/82
------------------------------------------------------------------------------*/

_PRIVATE _BYTE *hips_alloc(int i,int j)

{    _BYTE *k = (_BYTE *)NULL;

     k = (char *)pups_calloc(i,j);

     pups_set_errno(OK);
     return(k);
}




/*------------------------------------------------------------------------------
    hips_upd_desc.c - HIPL Picture Format update sequence description
    Michael Landy - 2/1/82
------------------------------------------------------------------------------*/

_PUBLIC void hips_upd_desc(hipl_hdr *hdr, char *txt)

{   if(hdr == (hipl_hdr *)NULL || txt == (char *)NULL || strcmp(txt,"") == 0)
    {  pups_set_errno(EINVAL);
       return;
    }

    if(strlen(hdr->seq_desc)+strlen(txt)+3*sizeof(char) > SSIZE)
       pups_error("[hips_upd_desc] image descriptor string too long (max size %d bytes)");

    if(txt != (char *)NULL && strlen(txt) > 2)
          (void)strlcat(hdr->seq_desc,"\n",SSIZE);

    (void)strlcat(hdr->seq_desc,hips_desc_massage(txt),SSIZE);
    if(hdr->seq_desc[strlen(hdr->seq_desc)-1] != '\n')
       (void)strlcat(hdr->seq_desc,"\n",SSIZE);

    pups_set_errno(OK);
}




/*------------------------------------------------------------------------------
    hips_set_desc - HIPL Picture Format set sequence description
------------------------------------------------------------------------------*/

_PUBLIC void hips_set_desc(hipl_hdr *hdr, char *txt)

{   char s[SSIZE] = "";

    if(hdr == (hipl_hdr *)NULL || txt == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return;
    }

    if(strlen(txt)+3*sizeof(char) > SSIZE)
       pups_error("[hips_set_desc] image descriptor string too long (max size %d bytes)");

    (void)strlcpy(hdr->seq_desc,txt,SSIZE);
    pups_set_errno(OK);
}





/*------------------------------------------------------------------------------
    read_header.c - HIPL Picture Format Header read
    Michael Landy - 2/1/82
    modified to use read/write 4/26/82
------------------------------------------------------------------------------*/

#define LINES      100
#define LINELENGTH SSIZE 

_PRIVATE char *ssave[LINES];
_PRIVATE int  slmax[LINES];
_PRIVATE int  lalloc = 0;

_PROTOTYPE _PRIVATE int  hips_dfscanf(int);
_PROTOTYPE _PRIVATE char *hips_getline(int, char **, int *);



_PUBLIC void hips_rd_hdr(hipl_hdr *hdr)

{    hips_frd_hdr(0,hdr);
}



_PUBLIC void hips_frd_hdr(int fd, hipl_hdr *hdr) 

{
   int lineno,
       i;

   char strdum[SSIZE]     = "",
        hips_magic[SSIZE] = "",
        *s                = (char *)NULL;

    if(fd < 0 || hdr == (hipl_hdr *)NULL)
    {  pups_set_errno(EINVAL);
       return;
    }

    if(lalloc < 1)
    {  ssave[0] = hips_alloc(LINELENGTH, sizeof (char));
       slmax[0] = LINELENGTH;
       lalloc   = 1;
    }



    /*----------------*/
    /* Get HIPS Magic */
    /*----------------*/

    (void)hips_getline(fd,&ssave[0],&slmax[0]);
    if(strncmp(ssave[0],HIPS_MAGIC,strlen(HIPS_MAGIC)) != 0)
        pups_error("[hips_frd_hdr] not a HIPS image");



    /*------------*/
    /* Get header */
    /*------------*/

    (void)hips_getline(fd,&ssave[0],&slmax[0]);
    if(strlen(ssave[0])+1*sizeof(char) > SSIZE)
       pups_error("[hips_frd_hdr] orig name too long (max size %d bytes)");

    (void)hips_getline(fd,&ssave[0],&slmax[0]);
    if(strlen(ssave[0])+1*sizeof(char) > SSIZE)
       pups_error("[hips_frd_hd]: sequence name too long (max size %d bytes)");

    (void)strlcpy(hdr->seq_name,ssave[0],SSIZE);
    hdr->num_frame = hips_dfscanf(fd);

    (void)hips_getline(fd,&ssave[0],&slmax[0]);
    if(strlen(ssave[0])+1*sizeof(char) > SSIZE)
       pups_error("[hips_frd_hdr] orig date name too long (max size %d bytes)");

    (void)strlcpy(hdr->orig_date,ssave[0],SSIZE);

    hdr->rows           = hips_dfscanf(fd);
    hdr->cols           = hips_dfscanf(fd);
    hdr->bits_per_pixel = hips_dfscanf(fd);
    hdr->bit_packing    = hips_dfscanf(fd);
    hdr->pixel_format   = hips_dfscanf(fd);
    lineno              = 0;

    (void)hips_getline(fd,&ssave[0],&slmax[0]);
    s = ssave[0];

    while(strlen(s) > 3 && *(s += strlen(s)-3) == '|')
    {
       lineno++;
       if (lineno >= LINES)
          pups_error("[hips_frd_hdr] too many lines in header history");

       if(lineno >= lalloc)
       {
          ssave[lineno] = hips_alloc(LINELENGTH, sizeof (char));
          slmax[lineno] = LINELENGTH;
          lalloc++;
       }

       (void)hips_getline(fd,&ssave[lineno],&slmax[lineno]);
       s = ssave[lineno];
    }

    hdr->seq_history[0] = '\0';
    for (i=0; i<=lineno; i++)
         (void)strlcat(hdr->seq_history,ssave[i],SSIZE);

    lineno = 0;
    while(strcmp(hips_getline(fd,&ssave[lineno],&slmax[lineno]),".\n")) // has a .
    {
       lineno++;
       if (lineno >= LINES)
          pups_error("[hips_frd_hdr] too many lines in header desc");

       if(lineno >= lalloc)
       {
          ssave[lineno] = hips_alloc(LINELENGTH, sizeof (char));
          slmax[lineno] = LINELENGTH;
          lalloc++;
       }
    }

    hdr->seq_desc[0] = '\0';
    for (i=0; i<lineno; i++)
         (void)strlcat(hdr->seq_desc,ssave[i],SSIZE);

    pups_set_errno(OK);
}


_PRIVATE char *hips_getline(int fd, char **s, int *l)

{
   int i,
       m,
       ret;

   char c,
        *s1  = (char *)NULL,
        *s2  = (char *)NULL;
        i    = 0;
        s1   = *s;
        m    = *l;

   if(fd < 0 || s == (char **)NULL || l == (int *)NULL)
   {  pups_set_errno(EINVAL);
      return((char *)NULL);
   }

   while((ret = read(fd,&c,1)) == 1 && c != '\n')
   {  if(ret < 0)
         pups_error("[hips_getline]: invalid file descriptor");

      if (m-- <= 2)
      {  if(*l > SSIZE)
            pups_error("[hips_getline] line too long");

         s2 = hips_alloc(LINELENGTH+*l,sizeof (char));
         (void)strlcpy(s2,*s,SSIZE);

         *s  = s2;
         *l += LINELENGTH;
         m   = LINELENGTH;
         s1  = s2 + strlen(s2);
      }

      *s1++ = c;
   }

   if (c == '\n')
   {  *s1++ = '\n';
      *s1 = '\0';

      pups_set_errno(OK);
      return(*s);
   }

   pups_error("[hips_getline] unexpected EOF while reading header");


   /*---------------------------------*/
   /* Stops compiler printing warning */
   /*---------------------------------*/

   return((char *)NULL);
}



_PRIVATE int hips_dfscanf(int fd)

{  int i;

   if(hips_getline(fd,&ssave[0],&slmax[0]) == (char *)NULL)
      pups_error("[hips_dfscanf]: invalid file descriptor");
   else
   {  if(sscanf(ssave[0],"%d",&i) == 1)
         return(i);
   }

   return(-1);
}





/*------------------------------------------------------------------------------
    HIPL Picture Format Header write
    Michael Landy - 2/1/82
    modified to use read/write 4/26/82
------------------------------------------------------------------------------*/

_PROTOTYPE _PRIVATE void hips_wstr(int, char *);
_PROTOTYPE _PRIVATE void hips_wnocr(int, char *);
_PROTOTYPE _PRIVATE void hips_dfprintf(int, int);

_PUBLIC void hips_wr_hdr(hipl_hdr *hd)

{  hips_fwr_hdr(1,hd);
}



_PUBLIC void hips_fwr_hdr(int fd, hipl_hdr *hdr)

{  
   if(fd < 0 || hdr == (hipl_hdr *)NULL)
   {  pups_set_errno(EINVAL);
      return;
   }



   /*------------*/
   /* HIPS magic */
   /*------------*/

   hips_wnocr   (fd,HIPS_MAGIC);


   /*--------*/
   /* Header */
   /*--------*/

   hips_wnocr   (fd,hdr->orig_name);
   hips_wnocr   (fd,hdr->seq_name);
   hips_dfprintf(fd,hdr->num_frame);
   hips_wnocr   (fd,hdr->orig_date);
   hips_dfprintf(fd,hdr->rows);
   hips_dfprintf(fd,hdr->cols);
   hips_dfprintf(fd,hdr->bits_per_pixel);
   hips_dfprintf(fd,hdr->bit_packing);
   hips_dfprintf(fd,hdr->pixel_format);
   hips_wstr    (fd,hdr->seq_history);
   hips_wstr    (fd,hdr->seq_desc);

   (void)write(fd,".\n",2);

   pups_set_errno(OK);
}


_PRIVATE void hips_wstr(int fd, char *s)

{  
   if(write(fd,s,strlen(s)) == (-1))
      pups_error("[hips_wstr]: invalid file descriptor");

   if(s[0] == '\0' || s[strlen(s)-1] != '\n')
      (void)write(fd,"\n",1);
}


_PRIVATE void hips_wnocr(int fd, char *s)

{  char *t = (char *)NULL;

   if(s == (char *)NULL)
   {  if(write(fd,"\n\0",2) == (-1))
         pups_set_errno("[hips_wnocr]: invalid file descriptor");
      else
         return;
   }
   else
      t = s;

   while (*t != '\n' && *t != '\0')
         (void)write(fd,(t++),1);
   (void)write(fd,"\n",1);
}


_PRIVATE void hips_dfprintf(int fd, int i)

{  char s[50] = "";

   (void)sprintf(s,"%d\n",i);
   if(write(fd,s,strlen(s)) == (-1))
      pups_set_errno("[hips_dfprintf]: invalid file descriptor");
} 




/*------------------------------------------------------------------------------
    initialise HIPL header ...
------------------------------------------------------------------------------*/

_PUBLIC void hips_init_hdr(hipl_hdr *hdr,  // HIPL header structure
                           char     *onm,  // Origin name
                           char     *snm,  // Sequence name
                           int       nfr,  // Number of frames [bands]
                           char     *odt,  // Origin date
                           int        rw,  // Image rows
                           int        cl,  // Image cols
                           int       bpp,  // Bits per pixel
                           int       bpk,  // Bit packing
                           int      pfmt,  // Pixel format
                           char     *shi,  // Sequence history
                           char    *desc)  // Sequence descriptor

{ 

/*------------------------------------------------------------------------------
    Fix bug in the original version of this software by allocating space for
    string ...
------------------------------------------------------------------------------*/


   if(onm != (char *)NULL)
     (void)strlcpy(hdr->orig_name,onm,SSIZE);

   if(snm != (char *)NULL)
      (void)strlcpy(hdr->seq_name,snm,SSIZE);

   if(shi != (char *)NULL)
      (void)strlcpy(hdr->seq_history,shi,SSIZE);

   if(odt != (char *)NULL)
      (void)strlcpy(hdr->orig_date,odt,SSIZE);

   if(shi != (char *)NULL)
      (void)strlcpy(hdr->seq_history,shi,SSIZE);
   else
      (void)strlcpy(hdr->seq_history,"\n",SSIZE);

   if(desc != (char *)NULL)
      (void)strlcpy(hdr->seq_desc,hips_desc_massage(desc),SSIZE);
   else
      (void)strlcpy(hdr->seq_desc,"\n",SSIZE);

   hdr->num_frame      = nfr;
   hdr->rows           = rw;
   hdr->cols           = cl;
   hdr->bits_per_pixel = bpp;
   hdr->bit_packing    = bpk;
   hdr->pixel_format   = pfmt;


   pups_set_errno(OK);
}



/*------------------------------------------------------------------------------
    Routine to display HIPS header information in text format ... 
------------------------------------------------------------------------------*/

_PUBLIC int hips_display_hdr_info(FILE *stream, hipl_hdr *hdr)

{
    if(stream == (FILE *)NULL || hdr == (hipl_hdr *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }


    (void)fprintf(stream,"\n    HIPS header information\n");
    (void)fprintf(stream,"    =======================\n\n");

    (void)fprintf(stream,"    Version magic            : %s (%s)\n",HIPS_MAGIC,__DATE__);
    (void)fprintf(stream,"    Orig name                : %s",hdr->orig_name);
    if(strcmp(hdr->orig_name,"") == 0)
       (void)fprintf(stream,"\n");

    (void)fprintf(stream,"    Orig date                : %s",hdr->orig_date);
    if(strcmp(hdr->orig_date,"") == 0)
       (void)fprintf(stream,"\n");

    (void)fprintf(stream,"    Seq name                 : %s",hdr->seq_name);
    if(strcmp(hdr->seq_name,"") == 0)
       (void)fprintf(stream,"\n");

    if(hdr->num_frame == 1)
       (void)fprintf(stream,"    Num frame                : %d (mono)\n",hdr->num_frame);
    else if(hdr->num_frame == 3)
       (void)fprintf(stream,"    Num frame                : %d (RGB)\n",hdr->num_frame);
    else
       (void)fprintf(stream,"    Num frame                : %d\n",hdr->num_frame);

    (void)fprintf(stream,"    rows                     : %d\n",hdr->rows);
    (void)fprintf(stream,"    cols                     : %d\n",hdr->cols);
    (void)fprintf(stream,"    bits per pixel           : %d\n",hdr->bits_per_pixel);

    if(hdr->bit_packing)
    (void)fprintf(stream,"    bit packing              : yes\n");
    else
    (void)fprintf(stream,"    bit packing              : no\n");

    switch(hdr->pixel_format)
    {     case PFBYTE:      (void)fprintf(stream,"    pixel format             : PFBYTE\n");
                            break;

          case PFSHORT:     (void)fprintf(stream,"    pixel format             : PFSHORT\n");
                            break;

          case PFINT:       (void)fprintf(stream,"    pixel format             : PFINT\n");
                            break;

          case PFLONG:      (void)fprintf(stream,"    pixel format             : PFLONG\n");
                            break;

          case PFFLOAT:     (void)fprintf(stream,"    pixel format             : PFFLOAT\n");
                            break;

          case PFCOMPLEX:   (void)fprintf(stream,"    pixel format             : PFCOMPLEX\n");
                            break;

          case PFASCII:     (void)fprintf(stream,"    pixel format             : PFASCII\n");
                            break;

          case PFDOUBLE:    (void)fprintf(stream,"    pixel format             : PFDOUBLE\n");
                            break;

          case PFDCOMPLEX:  (void)fprintf(stream,"    pixel format             : PFDCOMPLEX\n");
                            break;

          default:          (void)fprintf(stream,"    pixel format             : unknown\n");
                            break;
    }

    (void)fprintf(stream,"    sequence history         : %s",hdr->seq_history);
    if(strcmp(hdr->seq_history,"") == 0)
       (void)fprintf(stream,"\n");

    (void)fprintf(stream,"    sequence descriptor      : %s\n",hdr->seq_desc);
    if(strcmp(hdr->seq_desc,"") == 0)
       (void)fprintf(stream,"\n");

    (void)fflush(stream);

    pups_set_errno(OK);
    return(0);
}




/*------------------------------------------------------------------------------
    Routine to read HIPS header from standard input and compute relevant
    quantities ...
------------------------------------------------------------------------------*/

_PUBLIC int  hips_rd_hdr_info(int pixel_format,
                              int        *rows,
                              int        *cols,
                              int         *nfr,
                              hipl_hdr    *hdr)

{   return(hips_f_rd_hdr_info(0,pixel_format,rows,cols,nfr,hdr));
}




/*------------------------------------------------------------------------------
    Routine to read HIPS header data from file, and to
    compute relevent quantities ...
------------------------------------------------------------------------------*/

_PUBLIC int  hips_f_rd_hdr_info(int       fildes,
                                int pixel_format,
                                int        *rows,
                                int        *cols,
                                int         *nfr,
                                hipl_hdr    *hdr)

{ 


/*------------------------------------------------------------------------------
    Read HIPS header from file ...
------------------------------------------------------------------------------*/

    hips_frd_hdr(fildes,hdr);

/*------------------------------------------------------------------------------
    If pixel type is inconsistent, report it and stop ...
------------------------------------------------------------------------------*/

    if(hdr->pixel_format != pixel_format && pixel_format != PFANY)
       pups_error("[hips_f_rd_hdr_info] incorrect pixel format");

/*------------------------------------------------------------------------------
    Return relevent items to caller ...
------------------------------------------------------------------------------*/

    if(rows != (int *)NULL)
       *rows = hdr->rows;

    if(cols != (int *)NULL)
       *cols = hdr->cols;

    if(nfr != (int *)NULL)
       *nfr  = hdr->num_frame;

    pups_set_errno(OK);
    return(hdr->pixel_format);
}




/*------------------------------------------------------------------------------
    Routine to write HIPS header to standard input - also updates history
    at the same time ...
------------------------------------------------------------------------------*/

_PUBLIC void hips_wr_hdr_info(char     *application,
                              int              argc,
                              char          *args[],
                              char        *desc_str,
                              hipl_hdr         *hdr)

{   hips_f_wr_hdr_info(1,application,argc,args,desc_str,hdr);
}




/*------------------------------------------------------------------------------
    Routine to write HIPS header to file - also updates the
    history at the same time ...
------------------------------------------------------------------------------*/

_PUBLIC void hips_f_wr_hdr_info(int            fildes,
                                char     *application,
                                int              argc,
                                char          *args[],
                                char        *desc_str,
                                hipl_hdr         *hdr)

{   hips_upd_hdr(hdr,application,argc,args);
    hips_upd_desc(hdr,desc_str);
    hips_fwr_hdr(fildes,hdr);
}



/*----------------------------------------------------------------------------
    Routine to update HIPS header description ...
----------------------------------------------------------------------------*/

_PUBLIC void hips_upd_hdr(hipl_hdr          *hd,
                          char     *application,
                          int              argc,
                          char           **argv)

{   int  ac,
         len,
         i;

    char **av = (char **)NULL;

    time_t tm;

    if(hd == (hipl_hdr *)NULL || application == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return;
    }

    av  = argv;
    ac  = argc;
    len = 38 + strlen(hd->seq_history) + strlen(application);

    if(ac > 0)
    {  while (ac--)
          len += strlen(*(av++)) + 100;
    }

    if (hd->seq_history[(len = strlen(hd->seq_history) - 1)] != '\n')
    {  hd->seq_history[0] = '\0';
       len = 0;
    }
    else
    {  hd->seq_history[len++] = '|';
       hd->seq_history[len++] = '\\';
       hd->seq_history[len++] = '\n';
       hd->seq_history[len]   = '\0';
    }

    if(argc > 0 && argv != (char **)NULL)
    {  ac = argc;
       av = argv;

       (void)strlcat(hd->seq_history,application,SSIZE);
       (void)strlcat(hd->seq_history," ",        SSIZE);

       while (ac--)
       {     (void)strlcat(hd->seq_history,*(av++),SSIZE);
             (void)strlcat(hd->seq_history," ",    SSIZE);
       }

       strlcat(hd->seq_history,"\"-D ",SSIZE);
       tm = (long int) time(0);
       (void)strlcat(hd->seq_history, (char *)((long int)ctime(&tm)),SSIZE); // Was unsigned long 

       for (i=len;i<strlen(hd->seq_history);i++)
       {   if (hd->seq_history[i]=='\n')
              hd->seq_history[i] = ' ';
       }

       hd->seq_history[strlen(hd->seq_history)-1]  = '\0';
       (void)strlcat(hd->seq_history,"\"", SSIZE);
       (void)strlcat(hd->seq_history," \n",SSIZE);
    }

    pups_set_errno(OK);
}




/*------------------------------------------------------------------------------
    Get pixel size in bytes ...
------------------------------------------------------------------------------*/

_PUBLIC int hips_pixel_size(int pixel_format)

{   pups_set_errno(OK);

    switch(pixel_format)
    {   case PFBYTE:     return(sizeof(_BYTE));

        case PFSHORT:    return(sizeof(short int));

        case PFINT:      return(sizeof(int));

        case PFLONG:     return(sizeof(long int));

        case PFFLOAT:    return(sizeof(float));

        case PFDOUBLE:   return(sizeof(double));

        case PFCOMPLEX:  return(2*sizeof(float));

        case PFDCOMPLEX: return(2*sizeof(double));

        default: pups_error("[hips_pixel_size] unrecognised pixel format");
    }


    /*--------------------------------------*/
    /* Stops compiler from printing warning */
    /*--------------------------------------*/

    return(-1);
}




/*-----------------------------------------------------------------------------
    Routine to manipulate a HIPS raster of arbitrary type ...
-----------------------------------------------------------------------------*/

_PUBLIC void *hips_access_pixel(int pixel_size, int row, int col, void **raster)

{    void *pixel_ptr = (void *)NULL;


     if(pixel_size < 0 || row < 0 || col < 0 || raster == (void **)NULL)
     {  pups_set_errno(EINVAL);
        return((void *)NULL);
     }

     /*--------------------------------------------------------*/
     /* Get a pointer to the specified pixel noting pixel size */
     /*--------------------------------------------------------*/

     pixel_ptr = (void *)((unsigned long int)raster[row] + (unsigned long int)col*pixel_size);

     pups_set_errno(OK);
     return(pixel_ptr);
}




/*-----------------------------------------------------------------------------
    Routine to return a HIPS descriptor accociated with a given pixel
    type string ...
-----------------------------------------------------------------------------*/

_PUBLIC int hips_output_pixel_type(char *pixel_type_str)

{
    if(pixel_type_str == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return(-1);
    }

/*-----------------------------------------------------------------------------
    These are the acceptable HIPS pixel formats ...
-----------------------------------------------------------------------------*/

    pups_set_errno(OK);
    if(strcmp(pixel_type_str,"byte") == 0)
       return(PFBYTE);

    if(strcmp(pixel_type_str,"short") == 0)
       return(PFSHORT);

    if(strcmp(pixel_type_str,"int") == 0)
       return(PFINT);

    if(strcmp(pixel_type_str,"long") == 0)
       return(PFLONG);

    if(strcmp(pixel_type_str,"float") == 0)
       return(PFFLOAT);

    if(strcmp(pixel_type_str,"complex") == 0)
       return(PFCOMPLEX);

    if(strcmp(pixel_type_str,"double") == 0)
       return(PFDOUBLE);

    if(strcmp(pixel_type_str,"dcomplex") == 0)
       return(PFDCOMPLEX);

    pups_error("[hips_output_pixel_type] unknown pixel format");
    return(-1);
}




/*-----------------------------------------------------------------------------
    Return a descriptor string corresponding to a HIPL pixel type ...
-----------------------------------------------------------------------------*/

_PUBLIC char *hips_pixstr(int hipl_type)

{   switch(hipl_type)
    {   case PFBYTE:     return("byte");
        case PFSHORT:    return("short");
        case PFINT:      return("int");
        case PFLONG:     return("long");
        case PFFLOAT:    return("float");
        case PFCOMPLEX:  return("complex");
        case PFDOUBLE:   return("double");
        case PFDCOMPLEX: return("dcomplex");

        default: pups_error("[hips_pixtr] unknown pixel type\n");
    }
}



#ifdef TIFF_SUPPORT
/*------------------------------------------------------------------------------
    Routines for interconverting between HIPS and TIFF image formats ...
------------------------------------------------------------------------------*/

#include <tiff.h>
#include <tiffio.h>


/*-----------------------------------------------------------------------------
    Convert HIPS file to TIFF file ...
-----------------------------------------------------------------------------*/

_PUBLIC void hips_to_tiff(_BOOLEAN     do_in_mem,    // Force in memory RGB conversion
                          int       planarconfig,    // Image geometry for TIFF
                          int               argc,    // Application argument count
                          char           *argv[],    // Application argument vector
                          int   compression_mode,    // TIFF compression mode
                          char    *hips_filename,    // Name of file containing HIPS data
                          char    *tiff_filename)    // Name of file containing TIFF data

{   int  i,
         j,
         k,
         rows,
         cols,
         bands,
         fdes,
         pixel_type,
         pixel_layers,
         photint_type,
         size_in_bytes,
         rows_per_strip;

    _BYTE *buf                 = (_BYTE *)NULL,
          **bandbuf            = (_BYTE **)NULL;

    hipl_hdr hdr;
    TIFF     *tiff_hdr         = (TIFF *)NULL;
 
    time_t          clock;
    struct tm       *tm        = (struct tm *)NULL;
    struct passwd  *pwd        = (struct passwd *) NULL;

    char   datetime[SSIZE]     = "",
           eff_seq_desc[SSIZE] = "";

    if(argc          < 0                ||
       argv          == (char **)NULL   ||
       tiff_filename == (char *) NULL    )
    {  pups_set_errno(EINVAL);
       return;
    }


    /*----------------*/
    /* Open HIPS file */
    /*----------------*/

    if(hips_filename == (char *)NULL)
       fdes = 0;
    else
       fdes = pups_open(hips_filename,0,DEAD);


    /*------------------*/
    /* Read HIPS header */
    /*------------------*/

    hips_frd_hdr(fdes,&hdr);
    rows  = hdr.rows;
    cols  = hdr.cols;
    bands = hdr.num_frame;
 

    /*-------------------------------------*/
    /* Create TIFF header/file for writing */
    /*-------------------------------------*/

    tiff_hdr = TIFFOpen(tiff_filename,"w");


    /*--------------------------------------------------------------------------*/
    /* Test to make sure that we can actually convert the HIPS image to a       */
    /* TIFF image -- for example we cannot (yet) store arbitrary HIPS sequences */
    /* as TIFF images, n_frames == 3 implies a HIPS RGB image                   */
    /* -------------------------------------------------------------------------*/

    if(hdr.num_frame > 1)
    {  pixel_layers = hdr.num_frame;
       photint_type = 2;
    }
    else
    {  pixel_layers = 1;
       photint_type = 1;
    }


    /*---------------------------------------------------------------*/
    /* Convert HIPS pixel format to corresponding TIFF sample format */
    /*---------------------------------------------------------------*/

    switch(hdr.pixel_format)
    {   case PFBYTE:    pixel_type    = 1;
                        size_in_bytes = 1;
                        break;

        case PFSHORT:   pixel_type    = 1;
                        size_in_bytes = 2;
                        break;

        case PFINT:     pixel_type    = 1;
                        size_in_bytes = 4;
                        break;


        case PFLONG:    pixel_type    = 1;
                        size_in_bytes = 8;
                        break;
 
        case PFFLOAT:   pixel_type    = 3;
                        size_in_bytes = 4;
                        break;

        case PFDOUBLE:  pixel_type    = 3;
                        size_in_bytes = 8;
                        break;

        default:        pups_error("[hips_to_tiff] cannot convert pixel format");
    }


    /*------------------------------------------------------------------*/
    /* Update HIPS header to indicate that hips2tiff operation has been */
    /* carried out                                                      */
    /*------------------------------------------------------------------*/

    hips_upd_hdr(&hdr,appl_name,argc-1,argv);


    /*------------------------------*/
    /* Get number of rows per strip */
    /*------------------------------*/

     rows_per_strip = 8;
     // MAO rows_per_strip = (hdr.cols*hdr.rows*size_in_bytes / 8192) + 1;


    /*------------------------------------------------------------------*/
    /* Update tags in TIFF header to reflect information in HIPS header */
    /*------------------------------------------------------------------*/


    /*---------------------*/
    /* Set TIFF date field */
    /*---------------------*/

    (void)time(&clock);
    tm = localtime(&clock);
    (void)sprintf(datetime,"%04d:%02d:%02d %02d:%02d:%02d",tm->tm_year + 1900,
                                                               tm->tm_mon + 1,
                                                                  tm->tm_mday,
                                                                  tm->tm_hour,
                                                                   tm->tm_min,
                                                                   tm->tm_sec);
    datetime[strlen(datetime)] = '\0';


    /*-------------------------------------------------------------------------*/
    /* Check that we have bilevel image if CCIT group 3/4 compression selected */
    /*-------------------------------------------------------------------------*/

    if(hdr.bits_per_pixel > 1 && compression_mode == 2)
       pups_error("[hips_to_tiff] CCIT group 3/4 compression is only defined for bilevel images");


    pwd = getpwuid(appl_uid);

    for(i=0; i<strlen(hdr.seq_desc); ++i)
    {   if(hdr.seq_desc[i] == '\n')
        {  eff_seq_desc[i] = '\0';
           break;
        }
        else
           eff_seq_desc[i] = hdr.seq_desc[i];
    }

    (void)TIFFSetField(tiff_hdr,TIFFTAG_ARTIST,           pwd->pw_gecos);
    (void)TIFFSetField(tiff_hdr,TIFFTAG_DATETIME,         datetime);
    (void)TIFFSetField(tiff_hdr,TIFFTAG_IMAGELENGTH,      hdr.rows);
    (void)TIFFSetField(tiff_hdr,TIFFTAG_IMAGEWIDTH,       hdr.cols);
    (void)TIFFSetField(tiff_hdr,TIFFTAG_BITSPERSAMPLE,    hdr.bits_per_pixel);
    (void)TIFFSetField(tiff_hdr,TIFFTAG_SAMPLESPERPIXEL,  pixel_layers);
    (void)TIFFSetField(tiff_hdr,TIFFTAG_SAMPLEFORMAT,     pixel_type);
    (void)TIFFSetField(tiff_hdr,TIFFTAG_PLANARCONFIG,     planarconfig);
    (void)TIFFSetField(tiff_hdr,TIFFTAG_PHOTOMETRIC,      photint_type);
    (void)TIFFSetField(tiff_hdr,TIFFTAG_COMPRESSION,      compression_mode);
    (void)TIFFSetField(tiff_hdr,TIFFTAG_ROWSPERSTRIP,     rows_per_strip);
    (void)TIFFSetField(tiff_hdr,TIFFTAG_PAGENAME,         eff_seq_desc);  // MAO to first \n
    (void)TIFFSetField(tiff_hdr,TIFFTAG_SOFTWARE,         "hips2tiff");
    (void)TIFFSetField(tiff_hdr,TIFFTAG_RESOLUTIONUNIT,   1);


    /*------------------------------------*/
    /* Write HIPS image data to TIFF file */
    /*------------------------------------*/

    buf = (_BYTE *)pups_calloc(hdr.cols,pixel_layers*size_in_bytes);


    /*-----------------------------------------------------------------------*/
    /* We can convert the HIPS image to a TIFF efficiently if resulting TIFF */
    /* has only one band or is multiband (as opposed to chunky)              */
    /*-----------------------------------------------------------------------*/

    if(pixel_layers == 1 || planarconfig == 2)
    {  for(j=0; j<hdr.rows; ++j)
       {  (void)pups_read(fdes,buf,cols*size_in_bytes);
          (void)TIFFWriteScanline(tiff_hdr,(tdata_t)buf,j,0);
       }
    }
    else
    {

       /*--------------------------------------------------------------------*/
       /* We are converting an RGB (multiband) HIPS image to a chunky TIFF   */
       /* this is the worst case scenario as we have to read the entire HIPS */
       /* image before converting it                                         */
       /*--------------------------------------------------------------------*/
      
       int   cnt,
             bbcnt;

       _BYTE **bandbuf = (_BYTE **)NULL;


       /*-----------------------------------------------------*/
       /* If fdes is associated with a seekable device we can */
       /* read in a multiband image much more efficiently     */
       /*-----------------------------------------------------*/

       if(do_in_mem == TRUE || pups_is_seekable(fdes) == FALSE)
       {

          /*------------------------------------------------------*/
          /* No - fdes is associated with a FIFO -- we shall have */
          /* to do things inefficiently (in terms of memeory      */
          /*------------------------------------------------------*/

          bandbuf = (_BYTE **)pups_calloc(pixel_layers*rows,sizeof(_BYTE *));
          for(j=0; j<pixel_layers*rows; ++j)
          {  bandbuf[j] = (_BYTE *)pups_calloc(cols,size_in_bytes);
             (void)pups_read(fdes,bandbuf[j],cols*size_in_bytes); 
          }


          /*-----------------------------------------*/
          /* Convert HIPS data to chunky TIFF format */
          /*-----------------------------------------*/

          for(i=0; i<rows; ++i)
          {  bbcnt = 0;
             cnt   = 0;


             /*-----------------------------------*/
             /* Build a scanline of chunky pixels */
             /*-----------------------------------*/

             for(j=0; j<cols; ++j)
             {  for(k=0; k<pixel_layers; ++k)
                {  (void)memcpy(&buf[cnt],&bandbuf[k*rows + i][bbcnt],size_in_bytes);
                    cnt += size_in_bytes;         
                }
                bbcnt += size_in_bytes;
             }


             /*----------------------------------------------------------*/
             /* Free up band memory we are now finished with -- doing    */
             /* this means memory shortages caused when converting large */
             /* files will be acute as opposed to chronic!               */
             /*----------------------------------------------------------*/

             for(k=0; k<pixel_layers; ++k)
                (void)pups_free((void *)bandbuf[k*rows + i]);

             /*----------------------------------------------------*/
             /* We have built up a chunky scanline -- write it out */
             /*----------------------------------------------------*/

             (void)TIFFWriteScanline(tiff_hdr,(tdata_t)buf,i,0);
          }
       }
       else
       {  

          /*-------------------------------------------------*/
          /* fdes is associated with a seekable device       */
          /* we can use lseek() to do conversion efficiently */
          /*-------------------------------------------------*/
       
          unsigned long int pos;

          bandbuf = (_BYTE **)pups_calloc(pixel_layers,sizeof(_BYTE *));
          for(i=0; i<pixel_layers; ++i)
             bandbuf[i] = (_BYTE *)pups_calloc(cols,size_in_bytes);


          /*---------------*/
          /* Header offset */
          /*---------------*/

          pos = pups_lseek(fdes,0,SEEK_CUR);

          for(j=0; j<rows; ++j)
          {

             /*-------------------------------------------------*/
             /* Extract data for next scanline of chunky pixels */ 
             /*-------------------------------------------------*/

             for(i=0; i<pixel_layers; ++i)
             {  (void)pups_lseek(fdes,pos + i*rows*cols*size_in_bytes + j*cols*size_in_bytes,SEEK_SET);
                (void)pups_read(fdes,bandbuf[i],cols*size_in_bytes);
             }


             /*---------------------------------*/
             /* Build scanline of chunky pixels */
             /*---------------------------------*/

             bbcnt = 0;
             cnt   = 0;
             for(k=0; k<cols; ++k)
             {  for(i=0; i<pixel_layers; ++i)
                {  (void)memcpy(&buf[cnt],&bandbuf[i][bbcnt],size_in_bytes);
                   cnt += size_in_bytes;
                }
                bbcnt += size_in_bytes;
             }


             /*--------------------------*/
             /* ... and write it to TIFF */
             /*--------------------------*/

             (void)TIFFWriteScanline(tiff_hdr,(tdata_t)buf,j,0);
          }


          /*-------------*/
          /* Free memory */
          /*-------------*/

          for(i=0; i<pixel_layers; ++i)
             (void)pups_free((void *)bandbuf[i]);
       }

       (void)pups_free((void *)bandbuf); 
    }

    (void)pups_free((void *)buf);

/*---------------------------------------------------------------------------------
    Close HIPS and TIFF files ...
---------------------------------------------------------------------------------*/

    (void)close(fdes);
    (void)TIFFClose(tiff_hdr);

    pups_set_errno(OK);
}




/*------------------------------------------------------------------------------
    Routine to TIFF file to HIPS file ...
------------------------------------------------------------------------------*/

_PUBLIC void tiff_to_hips(_BOOLEAN  do_in_mem,    // Force in memory RGNB conversion
                          int            argc,    // Application argument count
                          char        *argv[],    // Application argument vector
                          char *tiff_filename,    // Name of file containing TIFF data
                          char *hips_filename)    // Name of file containing HIPS data

{   int i,
        j,
        k,
        fdes,
        rows,
        cols,
        pixel_format,
        tile_width,
        tile_length,
        size_in_bytes;

    short int bits_per_pixel = 8,
              pixel_type     = 1,    
              pixel_layers   = 1,
              planarconfig;

    char *orig_name          = (char *)NULL,
         *orig_date          = (char *)NULL,
         *seq_history        = (char *)NULL,
         *seq_desc           = (char *)NULL;

    _BYTE *buf               = (_BYTE *)NULL,
          **band_buf         = (_BYTE **)NULL;

    time_t        clock;
    struct tm     *tm        = (struct tm *)NULL;
    struct passwd *pwd       = (struct passwd *)NULL;

    hipl_hdr hdr;
    TIFF     *tiff_hdr       = (TIFF *)NULL;

    if(argc          < 0   ||
       argv          == (char **)NULL  ||
       tiff_filename == (char *) NULL   )
    {  pups_set_errno(EINVAL);
       return;
    }

    /*-------------------------------------*/
    /* Open TIFF file returning its header */
    /*-------------------------------------*/

    if((tiff_hdr = TIFFOpen(tiff_filename,"r")) == (TIFF *)NULL)
       pups_error("[tiff_to_hips] problem with TIFF file");


    /*--------------------*/
    /* Open HIPS filename */
    /*--------------------*/

    if(hips_filename == (char *)NULL)
       fdes = 1;
    else
       fdes = pups_open(hips_filename,1,DEAD);


    #ifdef FLORET_TIFF_SUPPORT
    /*------------------------------------------------------------------------*/
    /* If we have TIFFTAG_PAGENAME set warn the caller that BIOTIFF data will */
    /* not be converted to HIPS format                                        */
    /*------------------------------------------------------------------------*/

    if(appl_verbose == TRUE && TIFFGetField(tiff_hdr,TIFFTAG_FLORET) == 1)
    {  (void)strdate(date);
       (void)fprintf(stderr,
               "%s %s (%d@%s:%s): Floret TIFF extensions may be present (and will be discarded when converting to HIPS image\n",
                                                                                   date,appl_name,appl_pid,appl_host,appl_owner);
       (void)fflush(stderr);
    }
    #endif /* FLORET_TIFF support */

 
    /*-----------------*/
    /* Get TIFF fields */
    /*-----------------*/

    (void)TIFFGetField(tiff_hdr,TIFFTAG_DATETIME,         &orig_date);
    (void)TIFFGetField(tiff_hdr,TIFFTAG_IMAGELENGTH,      &rows);
    (void)TIFFGetField(tiff_hdr,TIFFTAG_IMAGEWIDTH,       &cols);
    (void)TIFFGetField(tiff_hdr,TIFFTAG_BITSPERSAMPLE,    &bits_per_pixel);
    (void)TIFFGetField(tiff_hdr,TIFFTAG_SAMPLESPERPIXEL,  &pixel_layers);
    (void)TIFFGetField(tiff_hdr,TIFFTAG_SAMPLEFORMAT,     &pixel_type);
    (void)TIFFGetField(tiff_hdr,TIFFTAG_PAGENAME,         &seq_desc);
    (void)TIFFGetField(tiff_hdr,TIFFTAG_PLANARCONFIG,     &planarconfig);

    orig_name = (char *)pups_malloc(SSIZE);

    pwd = getpwuid(appl_uid);
    (void)strlcpy(orig_name,pwd->pw_gecos,SSIZE);
 
    if(orig_date == (char *)NULL)
    {  orig_date = (char *)pups_malloc(SSIZE);


       /*---------------------*/
       /* Set TIFF date field */
       /*---------------------*/

       (void)time(&clock);
       tm = localtime(&clock);
       (void)sprintf(orig_date,"%04d:%02d:%02d %02d:%02d:%02d",tm->tm_year + 1900,
                                                                   tm->tm_mon + 1,
                                                                      tm->tm_mday,
                                                                      tm->tm_hour,
                                                                       tm->tm_min,
                                                                       tm->tm_sec);
    }


    /*------------------------*/
    /* No sequence descriptor */
    /*------------------------*/

    if(seq_desc == (char *)NULL)
    {  hips_init_hdr(&hdr,
                     orig_name,
                     "tiff2hips",
                     pixel_layers,
                     orig_date,
                     rows,
                     cols,
                     bits_per_pixel,
                     0,
                     0,
                     "",
                     (char *)NULL);
    }


    /*---------------------*/
    /* Sequence descriptor */
    /*---------------------*/

    else
       hips_init_hdr(&hdr,
                     orig_name,
                     "tiff2hips",
                     pixel_layers,
                     orig_date,
                     rows,
                     cols,
                     bits_per_pixel,
                     0,
                     0,
                     "",
                     seq_desc);


    /*--------------------*/
    /* Update HIPS header */
    /*--------------------*/

    hips_upd_hdr(&hdr,appl_name,argc,argv);


    /*----------------------*/
    /* Convert pixel format */
    /*----------------------*/

     if(pixel_type == 1)
     {  switch(hdr.bits_per_pixel)
        {   case 8:        hdr.pixel_format  = PFBYTE;
                           size_in_bytes     = 1;
                           break;

            case 16:       hdr.pixel_format  = PFSHORT;
                           size_in_bytes     = 2;
                           break;

            case 32:       hdr.pixel_format  = PFINT;
                           size_in_bytes     = 4;
                           break;

            case 64:       hdr.pixel_format  = PFLONG;
                           size_in_bytes     = 8;

            default:       break;
        }
    }
    else
    {  switch(hdr.bits_per_pixel) 
       {   case 32:     hdr.pixel_format = PFFLOAT;
                        size_in_bytes    = 4;
                        break;

           case 64:     hdr.pixel_format = PFDOUBLE;
                        size_in_bytes    = 8;
                        break;

           default:     break;
       }
    }

    /*---------------------------------------*/
    /* Test to see if this is a colour image */
    /*---------------------------------------*/

    if(pixel_layers != 1)
       hdr.num_frame = pixel_layers;
    else
       hdr.num_frame = 1;


    /*------------------------------------------*/
    /* Convert the image one scanline at a time */
    /*------------------------------------------*/

    hips_fwr_hdr(fdes,&hdr);

    buf = (_BYTE *)pups_calloc(hdr.cols,pixel_layers*size_in_bytes);
    if(pixel_layers == 1 || planarconfig == 2)
    {  for(j=0; j<pixel_layers*hdr.rows; ++j)
       {  (void)TIFFReadScanline(tiff_hdr,(tdata_t)buf,j,i);
          (void)write(fdes,buf,hdr.cols*size_in_bytes);
       }
    }
    else
    {


       /*------------------------------------------------------*/
       /* We are going to have to convert a chunky format TIFF */
       /* to a multiband HIPS image -- lets get on with it and */
       /* hope we have sufficient memory for this ineffcient   */
       /* process                                              */
       /*------------------------------------------------------*/

       int   cnt,
             bbcnt;

       _BYTE **bandbuf = (_BYTE **)NULL;


       /*------------------------------------------------------*/
       /* If fdes is associated with a seekable device we can  */
       /* convert chunky TIFF to band HIPS formats efficeintly */
       /*------------------------------------------------------*/

       if(do_in_mem == TRUE || pups_is_seekable(fdes) == FALSE)
       {

          /*----------------------------------------------------------------*/
          /* fdes is associated with a FIFO -- we are going to have         */
          /* to do things the American way, e.g. brute force and ignorance! */
          /*----------------------------------------------------------------*/

          bandbuf = (_BYTE **)pups_calloc(pixel_layers*rows,sizeof(_BYTE *));
          for(j=0; j<rows; ++j)
          {  (void)TIFFReadScanline(tiff_hdr,(tdata_t)buf,j,0);


             /*-----------------------------------------------*/
             /* Allocate storage for next N layered O/P pixel */
             /*-----------------------------------------------*/

             for(i=0; i<pixel_layers; ++i)
                bandbuf[i*rows + j] = (_BYTE *)pups_calloc(cols,size_in_bytes);


             /*-----------------------------*/
             /* Build banded HIPS image map */
             /*-----------------------------*/

             bbcnt = 0;
             cnt   = 0;
             for(k=0; k<cols; ++k)
             {  for(i=0; i<pixel_layers; ++i)
                {  (void)memcpy(&bandbuf[i*rows + j][bbcnt],&buf[cnt],size_in_bytes); 
                   cnt += size_in_bytes;
                }
                bbcnt += size_in_bytes;
             }
          }


          /*----------------------*/
          /* Write HIPS image map */
          /*----------------------*/

          for(i=0; i<pixel_layers*rows; ++i)
          {  (void)write(fdes,bandbuf[i],cols*size_in_bytes);
             (void)pups_free((void *)bandbuf[i]);
          }
       }
       else
       {

          /*-----------------------------------------------*/
          /* ... fdes is associated with a seekable device */
          /* so we can do things effieciently              */
          /*-----------------------------------------------*/

          unsigned long int pos;


          /*---------------*/
          /* Header offset */
          /*---------------*/

          pos = pups_lseek(fdes,0,SEEK_CUR);

          bandbuf = (_BYTE **)pups_calloc(pixel_layers,size_in_bytes);
          for(i=0; i<pixel_layers; ++i)
             bandbuf[i] = (_BYTE *)pups_calloc(cols,size_in_bytes);

          for(i=0; i<rows; ++i)
          {  (void)TIFFReadScanline(tiff_hdr,(tdata_t)buf,i,0);


             /*---------------------------------------------------*/
             /* Decompose a chunky scanline into N band scanlines */
             /*---------------------------------------------------*/

             bbcnt = 0;
             cnt   = 0; 
             for(k=0; k<cols; ++k)
             {  for(j=0; j<pixel_layers; ++j)
                {  (void)memcpy((void *)&bandbuf[j][bbcnt],(void *)&buf[cnt],size_in_bytes);
                   cnt += size_in_bytes; 
                }
                bbcnt += size_in_bytes;
             }


             /*------------------------------------------------------*/
             /* Seek to correct locations in multiband HIPS file and */
             /* write data to it                                     */
             /*------------------------------------------------------*/

             for(j=0; j<pixel_layers; ++j)
             {

                                      /*--------*/ /*------*/                  /*-------------*/      /*--------------*/
                                      /* Header */ /* Band */                  /* Row in band */      /* Pixel in row */
                                      /*--------*/ /*------*/                  /*-------------*/      /*--------------*/

                (void)pups_lseek(fdes,pos +        j*rows*cols*size_in_bytes + i*cols*size_in_bytes + k*size_in_bytes,SEEK_SET);
                (void)write(fdes,bandbuf[j],cols*size_in_bytes);
             } 
      
             for(j=0; j<pixel_layers; ++j)
                (void)pups_free((void *)bandbuf[j]);
         }

         (void)pups_free((void *)bandbuf);
       } 
    }

    (void)pups_free((void *)buf);


    /*---------------------------*/
    /* Close HIPS and TIFF files */
    /*---------------------------*/

    (void)close(fdes);
    (void)TIFFClose(tiff_hdr);

    pups_set_errno(OK);
}
#endif /* TIFF_SUPPORT */




#ifdef KONTRON_SUPPORT
/*-------------------------------------------------------------------------------------
    Check to see that header matches image specifications ...
-------------------------------------------------------------------------------------*/

_PUBLIC void img_read_hdr(int fdes, _BOOLEAN endian_swap, img_hdr_type *img_hdr)
{
    _USHORT *hd        = (_USHORT *)NULL,
            n_recs;

    long int hdr_size      = 0L,
             datatype,
             startimdata;      

    if(fdes < 0 || img_hdr == (img_hdr_type *)NULL)
    {  pups_set_errno(EINVAL);
       return;
    }

    if(pups_read(fdes,&n_recs,sizeof(_USHORT)) == (-1))
       pups_error("[img_read_hdr] invalid file descriptor\n");

    if(endian_swap == TRUE)
       (void)swap_bytes(&n_recs);


    /*---------------------------------------------------------------------------*/
    /* We must adjust the header size as we have effectively read hd[0] in order */
    /* to determine how big the header is                                        */
    /*---------------------------------------------------------------------------*/

    hdr_size = (long int)(128L*n_recs) - 2;
    hd       = (_USHORT *)pups_malloc(hdr_size);


    /*-----------------*/
    /* Read the header */
    /*-----------------*/

    if(pups_read(fdes,hd,hdr_size) != hdr_size)
    {  (void)close(fdes);
       pups_error("[img_read_hdr] Incorrect number of pixels read\n");
    }


    /*------------------------------*/
    /* Check image ID (hd[1],hd[2]) */
    /*------------------------------*/

    if(endian_swap == TRUE)
    {  (void)swap_bytes(&hd[0]);
       (void)swap_bytes(&hd[1]);
    }

    if(hd[0]!=0x1247 || hd[1]!=0xb06d)
    {  (void)close(fdes);
       pups_error("[img_read_header] not an IMG file\n");
    }

    if(endian_swap == TRUE)
    {  (void)swap_bytes(&hd[2]);
       (void)swap_bytes(&hd[3]);
    } 


    /*-------------------------------*/
    /* Read image size (hd[2],hd[3]) */
    /*-------------------------------*/

    img_hdr->cols  = (int)hd[2];
    img_hdr->rows  = (int)hd[3];

    if(endian_swap == TRUE)
       (void)swap_bytes(&hd[5]);


    /*-----------------------------------*/ 
    /* Read number of images in sequence */
    /*-----------------------------------*/ 

    if((int)hd[5] != 1)
       pups_error("[img_read_hdr] can only have single frame images");
    else
       img_hdr->n_frames = 1;

    if(endian_swap == TRUE)
    {  (void)swap_bytes(&hd[6]);
       (void)swap_bytes(&hd[7]);
    }

    (void)memcpy(&datatype,hd+6,4);
    if(datatype==0x00003008)
       img_hdr->colour = IMG_RGB;
    else
       img_hdr->colour = IMG_GREY;

    img_hdr->size = img_hdr->rows*img_hdr->cols;

    if(hd[9] == 0x544c)
    {  (void)memcpy(&img_hdr->seeklutdata,hd+11,4);
       img_hdr->has_lut = TRUE;
    }


    /*---------------------------------------*/
    /* Get pointers to the image data arrays */
    /*---------------------------------------*/

    img_hdr->seekRdata =  128L * n_recs;
    if(img_hdr->colour == IMG_RGB)
    {  img_hdr->seekGdata = img_hdr->seekGdata + img_hdr->size;
       img_hdr->seekBdata = img_hdr->seekBdata + 2*img_hdr->size;
    }

    pups_set_errno(OK);
}



/*---------------------------------------------------------------------------
    Write KONTRON header to file -- this routine will typically be 
    used by image processing filters which support the KONTRON image
    format ...
---------------------------------------------------------------------------*/

_PUBLIC void img_write_hdr(int fdes, _BOOLEAN endian_swap, img_hdr_type *img_hdr)

{   _USHORT  buf[128] = { 0 };
    long int ltmp;


    if(fdes < 0 || img_hdr == (img_hdr_type *)NULL)
    {  pups_set_errno(EINVAL);
       return;
    }

    /*-------------------------------*/
    /* Number of records in IMG file */
    /*-------------------------------*/

    if(img_hdr->colour == IMG_RGB && img_hdr->has_lut == TRUE)
       buf[0] = (short)7;
    else
       buf[0] = (short)1;


    /*---------------------------*/
    /* Magic number for img file */
    /*---------------------------*/

    buf[1] = (short)0x1247;
    buf[2] = (short)0xb06d; 


    /*------------------------------*/
    /* Image size and sequence size */
    /*------------------------------*/

    buf[3] = (short)img_hdr->cols;
    buf[4] = (short)img_hdr->rows;
    buf[5] = (short)0x4321;
    buf[6] = (short)1;


    /*----------------------------------------------*/
    /* Swap bytes in header if changing endianicity */
    /*----------------------------------------------*/

    if(endian_swap == TRUE)
    {  int i;

       for(i=0; i<7; ++i)
          swap_bytes(&buf[i]);
    } 
  

    /*----------------*/
    /* Image datatype */
    /*----------------*/

    if(img_hdr->colour == IMG_RGB)
    {  ltmp = 0x00003008;
       (void)memcpy(buf+7,&ltmp,4);

       if(img_hdr->has_lut == TRUE)
          buf[10] = 0x544c;
    }
    else
    {  ltmp = 0L;
       (void)memcpy(buf+7,&ltmp,4);
       buf[10] = 0;
    }


    /*------------------------------*/
    /* Image grey (Red) data offset */
    /*------------------------------*/

    ltmp =  (long int)buf[0]*128L;
    (void)memcpy(buf+11,&ltmp,4);


    /*----------------------------------------------------------*/
    /* Write out green and blue plane offsets (if colour image) */
    /*----------------------------------------------------------*/

    if(img_hdr->colour == IMG_RGB)
    {  ltmp = (long int)buf[0]*128L + img_hdr->size;  
       (void)memcpy(buf+13,&ltmp,4);

       ltmp = (long int)buf[0]*128L + 2L*img_hdr->size;
       (void)memcpy(buf+15,&ltmp,4);
    } 


    /*--------------------------*/
    /* Write IMG header to file */
    /*--------------------------*/

    if(write(fdes,buf,128L*buf[0]) == (-1))
       pups_error("[img_write_hdr] invalid file descriptor");


    /*-------------------------------------*/
    /* Write LUT to file (if there is one) */
    /*-------------------------------------*/

    if(img_hdr->has_lut == TRUE)
      (void)write(fdes,(_BYTE *)img_hdr->seeklutdata,768L);

    pups_set_errno(OK);
}




/*-----------------------------------------------------------------------------
    Convert HIPS image to Kontron image ...
-----------------------------------------------------------------------------*/

_PUBLIC void hips_to_img(_BOOLEAN endian_swap,   // Big or little endian?
                         char *hips_filename,    // HIPS filename
                         char  *img_filename)    // Kontron filename

{   int i,
        j,
        fdes  = (-1),
        ides  = (-1);

    _BYTE *buf = (_BYTE *)NULL;

    hipl_hdr     hdr;
    img_hdr_type img_hdr;

    if(hips_filename == (char *)NULL || img_filename == (char *)NULL)
    {  pups_set_errno(EINVAL);
       return;
    }
    
    /*----------------*/
    /* Open HIPS file */
    /*----------------*/

    if(hips_filename == (char *)NULL)
       fdes = 0;
    else
       fdes = pups_open(hips_filename,0,DEAD);


    /*------------------*/
    /* Read HIPS header */
    /*------------------*/

    hips_frd_hdr(fdes,&hdr);


    /*---------------------------*/
    /* Open img file for writing */
    /*---------------------------*/

    if(img_filename == (char *)NULL)
       ides = 1;
    else
       ides = pups_open(img_filename,1,DEAD);


    /*----------------------------------------------*/
    /* Convert HIPS header and write it to img file */
    /*----------------------------------------------*/

    if(hdr.pixel_format != PFBYTE)
       pups_error("[hips_to_img] can only convert byte images to Kontron (img) format");
   
    img_hdr.rows = hdr.rows;
    img_hdr.cols = hdr.cols;

    if(hdr.num_frame != 1 && hdr.num_frame != 3)
       pups_error("[hips_to_img] can only convert single frame sequence to Kontron (img) format");
    else
       img_hdr.n_frames = 1;


    /*----------------------------------------------------------*/
    /* # frame HIPS sequence is interpreted as RGB colour image */ 
    /*----------------------------------------------------------*/

    if(hdr.num_frame == 3)
       img_hdr.colour = IMG_RGB;
    else
       img_hdr.colour = IMG_GREY;

    img_hdr.has_lut = FALSE;
    (void)img_write_hdr(ides,endian_swap,&img_hdr);


    /*-------------------------------------------*/
    /* Transfer image data from hips to img file */ 
    /*-------------------------------------------*/

    buf = (_BYTE *)pups_calloc(hdr.cols,sizeof(_BYTE));
    for(i=0; i<hdr.num_frame; ++i)
    {  for(j=0; j<hdr.rows; ++j)
       {  (void)pups_read(fdes,buf,hdr.cols*sizeof(_BYTE));
          (void)write(ides,buf,hdr.cols*sizeof(_BYTE));
       }
    }
    (void)pups_free((void *)buf);

    (void)close(fdes);
    (void)close(ides);

    pups_set_errno(OK);
} 



/*-----------------------------------------------------------------------------
    Convert IMG image to HIPS image ...
-----------------------------------------------------------------------------*/

_PUBLIC void img_to_hips(_BOOLEAN  endian_swap,    // Big or little endian?
                          int              argc,    // Argument count
                          char          *argv[],    // Argument vector
                          char    *img_filename,    // Kontron filename
                          char   *hips_filename)    // HIPS filename

{   int i,
        j,
        pixel_layers,
        fdes  = (-1),
        ides  = (-1);

    _BYTE *buf = (_BYTE *)NULL;

    _BOOLEAN do_close_fdes = FALSE,
             do_close_des  = FALSE;

    hipl_hdr     hdr;
    img_hdr_type img_hdr;

    time_t    clock;
    struct tm *tm           = (struct tm *)NULL;
    char      datetime[SSIZE] = "";

    if(img_filename == (char *)NULL || hips_filename == (char *)NULL)
    {  pups_set_errno(EINVAL);
        return;
    }


    /*----------------*/
    /* Open HIPS file */
    /*----------------*/

    if(hips_filename == (char *)NULL)
       fdes = 1;
    else
    {  do_close_fdes = TRUE;
       fdes          = pups_open(hips_filename,0,DEAD);
    }


    /*---------------------------*/
    /* Open img file for reading */ 
    /*---------------------------*/

    if(img_filename == (char *)NULL)
       ides = 0;
    else
    {  do_close_des = TRUE;
       ides         = pups_open(img_filename,1,DEAD);
    }


    /*----------------------*/
    /* Read IMG header data */ 
    /*----------------------*/

    img_read_hdr(ides,endian_swap,&img_hdr);

    if(img_hdr.colour == IMG_RGB)
       pixel_layers = 3;
    else
       pixel_layers = 1;


    /*---------------------------------------------*/
    /* Initialise HIPS header and write it to file */
    /*---------------------------------------------*/


    /*---------------------*/
    /* Set HIPS date field */
    /*---------------------*/

    (void)time(&clock);
    tm = localtime(&clock);
    (void)sprintf(datetime,"20%02d:%02d:%02d %02d:%02d:%02d",tm->tm_year,
                                                             tm->tm_mon + 1,
                                                             tm->tm_mday,
                                                             tm->tm_hour,
                                                             tm->tm_min,
                                                             tm->tm_sec);


    hips_init_hdr(&hdr,
                  "img2hips",
                  "HIPS",
                  pixel_layers,
                  datetime,
                  img_hdr.rows,
                  img_hdr.cols,
                  8,
                  0,
                  PFBYTE,
                  "",
                  "");


    /*-------------------*/
    /* Write HIPS header */
    /*-------------------*/

    hips_upd_hdr(&hdr,appl_name,argc,argv);
    hips_fwr_hdr(fdes,&hdr);


    /*------------------------------------------------*/
    /* Transfer image data from img file to hips file */
    /*------------------------------------------------*/

    buf = (_BYTE *)pups_calloc(hdr.cols,sizeof(_BYTE));
    for(i=0; i<hdr.num_frame; ++i)
    {  for(j=0; j<hdr.rows; ++j)
       {  (void)pups_read(ides,buf,hdr.cols*sizeof(_BYTE));
          (void)write(fdes,buf,hdr.cols*sizeof(_BYTE));
       }
    }
    (void)pups_free((void *)buf);

    if(do_close_fdes == TRUE)
       (void)pups_close(fdes);

    if(do_close_des == TRUE)
       (void)pups_close(ides);

    pups_set_errno(OK);
}




/*-------------------------------------------------------------------------
    Routine to switch byte sequence ...
-------------------------------------------------------------------------*/

_PRIVATE void swap_bytes(_USHORT *arg)

{       _USHORT lsb_mask,
                msb_mask;


        /*-------------------------------------------------------*/
        /* Get least significant and most significant byte masks */
        /*-------------------------------------------------------*/

        lsb_mask = (*arg) & 0xff;
        msb_mask = (*arg) & 0xff00;


        /*----------------------------------------------------*/
        /* Shift masks left and right arithmetic respectively */
        /*----------------------------------------------------*/

        lsb_mask *= 0x100;
        msb_mask /= 0x100;


        /*-------------------------*/
        /* Form byte swapped short */
        /*-------------------------*/

        *arg = lsb_mask + msb_mask;
}
#endif /* KONTRON_SUPPORT */
