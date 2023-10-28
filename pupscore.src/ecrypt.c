/*----------------------------------------------------------------------------
   Purpose: 8 rotor enigma based symetric encryptor.

   Author:  M.A. O'Neill
            Tumbling Dice Ltd
            Gosforth
            Newcastle upon Tyne
            NE3 4RT
            United Kingdom

   Version: 2.00 
   Dated:   4th January 2023
   E-mail:  mao@tumblingdice.co.uk
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <xtypes.h>


/*----------------------------------------------------------------------------
    Defines for this application ...
----------------------------------------------------------------------------*/
/*-------------------*/
/* Version of ecrypt */
/*-------------------*/

#define ECRYPT_VERSION  "1.06"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE           2048 


#define DEFAULT_SEED    9999 




/*----------------------------------------------------------------------------
    Main program starts here
----------------------------------------------------------------------------*/

_PUBLIC int main(int argc, char *argv[])

{   int i,
        index,
        bytes_read,
        s_index     = 1;

    unsigned int seed;

    unsigned char buf[SSIZE] = "";
    _BOOLEAN one_way         = TRUE;

    if(argc > 1 && (strcmp(argv[1],"-usage") == 0 || strcmp(argv[1],"-help") == 0))
    {  (void)fprintf(stderr,"\necrypt version %s, (C) Tumbling Dice 2003-2023 (built %s %s)\n\n",ECRYPT_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"ECRYPT is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"ECRYPT comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nUsage: ecrypt [-usage | -help] [seed] < <plaintext> 1> <cipher> 2> <status log>\n\n");
       (void)fflush(stderr);

       exit(1);
    }
    else if(argc > 1 && strcmp(argv[1],"-bw") == 0)
    {  s_index = 2;
       one_way = FALSE;
    }

    if(argc > 1)
    {  if(sscanf(argv[s_index],"%d",&seed) != 1)
       {  (void)fprintf(stderr,"\necrypt: expecting seed for random number generator\n\n");
          (void)fflush(stderr);

          exit(255);
       }
    }
    else
       seed = DEFAULT_SEED;


    /*------------------------*/
    /* Encrypt standard input */
    /*------------------------*/

    (void)srand(seed);
    do {   bytes_read = read(0,buf,SSIZE);

           if(bytes_read > 0)
           {  for(i=0; i<bytes_read; ++i)
              {  buf[i] = (unsigned char)((int)buf[i] ^ rand());

                 if(one_way == TRUE)
                    while(buf[i] <60 || buf[i] > 85)
                          buf[i] = (unsigned char)((int)buf[i] ^ rand());
              }

              (void)write(1,buf,bytes_read);
           }
       } while(bytes_read > 0);

    exit(0);
}
