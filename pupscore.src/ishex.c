/*--------------------------------------------------
     Purpose: Tests to see if string is a hex number 

     Author:  M.A. O'Neill
              Tumbling Dice Ltd
              Gosforth
              Newcastle upon Tyne
              NE3 4RT
              United Kingdom

     Version: 2.02 
     Dated:   10th December 2024
     E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <xtypes.h>
#include <ctype.h>
#include <stdint.h>


/*---------*/
/* Defines */
/*---------*/
/*------------------*/
/* Version of ishex */
/*------------------*/

#define ISHEX_VERSION  "2.02"


/*-------------*/
/* String size */
/*-------------*/

#define STRING)SIZE    2048



/*------------------*/
/* Main entry point */
/*------------------*/

_PUBLIC int32_t main(int32_t argc, char *argv[])

{   int32_t i,
            hexval,
            start = 0;


   /*--------------------*/
   /* Parse command line */
   /*--------------------*/

   if(argc != 2)
   {  (void)fprintf(stderr,"\nishex version %s, (C) Tumbling Dice 2002-2024 (gcc %s: built %s %s)\n\n",ISHEX_VERSION,__VERSION__,__TIME__,__DATE__);
      (void)fprintf(stderr,"ISHEX is free software, covered by the GNU General Public License, and you are\n");
      (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
      (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
      (void)fprintf(stderr,"ISHEX comes with ABSOLUTELY NO WARRANTY\n\n");
      (void)fprintf(stderr,"\nUsage: ishex !string!\n\n");
      (void)fflush(stderr);

      exit(255);
   }


   /*-----------------------------*/
   /* Scan for non hex characters */
   /*-----------------------------*/

   if(argv[1][0] == '0' && argv[1][1] == 'x')
      start = 2;

   if(start >= strlen(argv[1]))
   {  (void)fprintf(stdout,"FALSE\n");
      (void)fflush(stdout);

      exit(0);
   }

   for(i=start; i<strlen(argv[1]); ++i)
   {  if(!isxdigit(argv[1][i]))
      {  (void)fprintf(stdout,"FALSE");
         (void)fflush(stdout);

         exit(0);
      }
   }

   if(sscanf(argv[1],"%x",&hexval) == 1)
   {  (void)fprintf(stdout,"TRUE");
      (void)fflush(stdout);
   }
   else
   {  (void)fprintf(stdout,"FALSE");
      (void)fflush(stdout);
   }

   exit(0);
}

