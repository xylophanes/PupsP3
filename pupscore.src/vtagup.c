/*------------------------------------------------------------
    Purpose: Set/update build tag on a PUPS application source 

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.03
    Dated:   12th December 2024
    E-mail:  mao@tumblingdice.co.uk
------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <xtypes.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>


/*---------*/
/* Defines */
/*---------*/
/*---------*/
/* Version */
/*---------*/

#define VTAGUP_VERSION    "2.00"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE             2048 


/*---------------------------*/
/* Local function prototypes */
/*---------------------------*/

_PRIVATE _BOOLEAN strin(const char *, const char *);




/*------------------*/
/* main entry point */
/*------------------*/

int32_t main(int32_t argc, char *argv[])

{   char      line[SSIZE]    = "",
              tag[SSIZE]     = "",
              strdum[SSIZE]  = "";

    uint32_t  cnt    = 0,
              v_num  = (-1);

    FILE      *stream        = (FILE *)NULL;

    off_t     pos;
    struct    stat buf;


    /*--------------------*/
    /* Parse command line */
    /*--------------------*/

    if(argc < 3 || argc > 5 || strcmp(argv[1],"-help") == 0)
    {  (void)fprintf(stderr,"\nvtagup version %s, (C) Tumbling Dice 2000-2024 (gcc %s: built %s %s)\n\n",VTAGUP_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fprintf(stderr,"VTAGUP is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"VTAGUP comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nUsage: vtagup [-set <tag value>] <tag id> <C/C++ source file>\n\n");
       (void)fflush(stderr);

       exit(0);
    }

    if(strcmp(argv[1],"-set") == 0)
    {  if(sscanf(argv[2],"%d",&v_num) != 1)
       {  (void)fprintf(stderr,"\nvtagup: expecting build tag value\n\n");
          (void)fflush(stderr);

          exit(255);
       }

       cnt += 2;
    }

    if((stream = fopen(argv[cnt + 2],"r+")) == (FILE *)NULL)
    {  (void)fprintf(stderr,"vtagup: cannot open %s\n",argv[cnt + 2]);
       (void)fflush(stderr);

       exit(255);
    }
      
    (void)stat(argv[cnt + 2],&buf);
    if(buf.st_mode & S_IFREG == 0)
    {  (void)fprintf(stderr,"vtagup: %s is not located on a seekable device\n",argv[cnt + 2]);
       (void)fflush(stderr);

       (void)fclose(stream);
       exit(255);
    }

    while(1)
    {   pos = ftell(stream);
        (void)fgets(line,255,stream);

        if(feof(stream))
        {  (void)fprintf(stderr,"vtagup: source file is not tagged\n");
           (void)fflush(stderr);

           (void)fclose(stream);
           exit(255);
        }


        if(strin(line,argv[1]) == TRUE)
        {  if(v_num == (-1))
           { (void)sscanf(line,"%s%s%d",strdum,tag,&v_num);

             if(strcmp(tag,argv[1]) == 0)
             {  if(v_num < 10000000)
                   ++v_num;
                else
                   v_num = 1;

                (void)fseek(stream,pos,SEEK_SET);
             }
           }

           (void)snprintf(line,SSIZE,"#define %s %5d\n",argv[1],v_num);
           (void)fputs(line,stream);

           (void)fseek(stream,0L,SEEK_END);
           (void)fclose(stream);

           (void)fprintf(stderr,"%s: Software version ID [build] tag is now %d\n",argv[2],v_num);
           (void)fflush(stderr);

           exit(0);
        }
    }
}




/*------------------------------------------------------*/
/* Look for the occurence of string s2 within string s1 */
/*------------------------------------------------------*/

_PUBLIC _BOOLEAN strin(const char *s1, const char *s2)
    
{   size_t i,                    
           cmp_size,                                                             
           chk_limit;

    if(strlen(s2) > strlen(s1))
       return(FALSE);

    chk_limit = strlen(s1) - strlen(s2) + 1;
    cmp_size  = strlen(s2);

    for(i=0; i<chk_limit; ++i)
       if(strncmp(&s1[i],s2,cmp_size) == 0)
          return(TRUE);

    return(FALSE);
}

