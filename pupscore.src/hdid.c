/*---------------------------------------------------------------------------
    Purpose: get disk serial number

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.02 
    Dated:   24th May 2022
    E-mail:  mao@tumblingdice.co.uk
---------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <bsd/string.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <linux/hdreg.h>
#include <xtypes.h>


/*-----------------*/
/* Version of hdid */
/*-----------------*/

#define HDID_VERSION    "2.02"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE           2048 


/*-------------------------------------------------*/
/* Functions which are private to this application */
/*-------------------------------------------------*/

_PROTOTYPE _PRIVATE int ecryptstr(int, char *, char *);
_PROTOTYPE _PRIVATE _BOOLEAN strin(char *, char *);




/*--------------------------*/
/* Main program entry point */
/*--------------------------*/

_PUBLIC int main(int argc, char *argv[])

{   int    i,
           seed,
           dev_index,
           ddes           = (-1);

    char   hostname[SSIZE],
           hd_device[SSIZE] = "/dev/hda";

    struct hd_driveid disk_info;

    _BOOLEAN do_ecrypt = FALSE;

    (void)gethostname(hostname,SSIZE);
    if(argc == 1 || strcmp(argv[1],"-usage") == 0 || strcmp(argv[1],"-help") == 0)
    {  (void)fprintf(stderr,"\nhdid version %s, (C) Tumbling Dice 2003-2022 (built %s %s)\n\n",HDID_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"HDID is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"HDID comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fprintf(stderr,"\nUsage: hdid [-usage | -help] | [-ecrypt <seed>] [disk device name | default]\n\n");
       (void)fflush(stderr);

       exit(255);
    }
    else if(argc == 2)
    {  if(strin(argv[1],"/dev/") == TRUE)
          (void)strlcpy(hd_device,argv[1],SSIZE);
       else if(strcmp(argv[1],"default") == 0)
          (void)strlcpy(hd_device,"/dev/hda",SSIZE);
       else
          (void)snprintf(hd_device,SSIZE,"/dev/%s",argv[1]);
    }
    else if(argc >= 3)
    {  if(strcmp(argv[1],"-ecrypt") == 0)
       {  do_ecrypt = TRUE;

          if(argc == 4)
          {  if(sscanf(argv[2],"%d",&seed) != 1)
             {  (void)fprintf(stderr,"hdid (%d@%s): encryption (%s) seed must be an integer (>= 0)\n",getpid(),hostname,argv[3]);
                (void)fflush(stderr);

                exit(255);
             }

             dev_index = 3;
          }
          else
          {  seed = 9999;
             dev_index = 2;
          }
       }
       else
       {  (void)fprintf(stderr,"hdid (%d@%s): expecting \"-ecrypt\" parameter\n",getpid(),hostname);
          (void)fflush(stderr);

          exit(255);
       }

       if(strin(argv[dev_index],"/dev/") == TRUE)
          (void)strlcpy(hd_device,argv[dev_index],SSIZE);
       else if(strcmp(argv[dev_index],"default") == 0)
          (void)strlcpy(hd_device,"/dev/sda",SSIZE);
       else  
          (void)snprintf(hd_device,SSIZE,"/dev/%s",argv[dev_index]);
    }


    /*---------------------------------------------*/
    /* Strip trailing partition number from device */
    /*---------------------------------------------*/

    for(i=strlen(hd_device)-1; i > 0; --i)
    {  if(isdigit(hd_device[strlen(hd_device)-1]))
          hd_device[strlen(hd_device)-1] = '\0';
       else
          break;
    }

    if((ddes = open(hd_device,0)) == (-1))
    {  (void)fprintf(stderr,"hdid (%d@%s): failed to open (disk) device \"%s\"\n",getpid(),hostname,hd_device);
       (void)fflush(stderr);

       exit(255);
    }

    if(ioctl(ddes,HDIO_GET_IDENTITY,(void *)&disk_info) == (-1))
    {  (void)fprintf(stderr,"hdid (%d@%s): failed to probe (disk) device \"%s\"\n",getpid(),hostname,hd_device);
       (void)fflush(stderr);

       (void)close(ddes);
       exit(255);
    }

    (void)close(ddes);

    if(do_ecrypt == TRUE)
    {  char ecrypted_serial[SSIZE] = "";

       (void)ecryptstr(seed,(char *)disk_info.serial_no,(char *)ecrypted_serial);
       (void)fprintf(stdout,"%s [seed %d, device %s]\n",ecrypted_serial,seed,hd_device);
       (void)fflush(stdout); 
    }
    else
    {  (void)fprintf(stdout,"%s [device %s]\n",disk_info.serial_no,hd_device);
       (void)fflush(stdout); 
    }

    exit(0);
}




/*------------------------------------------------------------------------------
    Encrypt a string ...
------------------------------------------------------------------------------*/

_PRIVATE int ecryptstr(int seed, char *plaintext, char *cipher)

{   int i;


    /*----------------*/
    /* Encrypt string */
    /*----------------*/

    (void)srand(seed);

    for(i=0; i<strlen(plaintext); ++i)
    {   cipher[i] = (unsigned char)((int)plaintext[i] ^ rand());

        while((int)cipher[i] < 60 || (int)cipher[i] > 85)
              cipher[i] = (unsigned char)((int)cipher[i] ^ rand());
    }

    cipher[i] = '\0';
    return(0);
}




/*-----------------------------------------------------------------------------
    Look for the occurence of string s2 within string s1 ...
-----------------------------------------------------------------------------*/

_PRIVATE _BOOLEAN strin(char *s1, char *s2)

{   int i,
        cmp_size,
        chk_limit;

    if(strlen(s2) > strlen(s1))
       return(FALSE);

    chk_limit = strlen(s1) - strlen(s2) + 1;
    cmp_size  = strlen(s2);

    for(i=0; i<chk_limit; ++i)
    {  if(strncmp(&s1[i],s2,cmp_size) == 0)
          return(TRUE);
    }

    return(FALSE);
}

