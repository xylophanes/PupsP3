/*-------------------------------------------------------------------------------------------------
    Purpose: main for a PUPS/P3 application 

    Author: M.A. O'Neill
            Tumbling Dice
            Gosforth
            Tyne and Wear
            NE3 4RT

    Version: 2.00 
    Dated:   30th August 2019  
    E-mail:  mao@tumblingdice.co.uk 
-------------------------------------------------------------------------------------------------*/


#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <bubble.h>
#include <errno.h>
#include <xtypes.h>


/*--------------------------------------------------------------------------------------------------
    Definitions which are local to this application ...
--------------------------------------------------------------------------------------------------*/

#define CRC_SIZE 1024
#define SSIZE    2048 

#ifdef THREAD_SUPPORT
_PUBLIC pthread_t root_tid;
#endif /* THREAD_SUPPORT */


/*--------------------------------------------------------------------------------------------------
    Variables imported by this application ...
--------------------------------------------------------------------------------------------------*/

_IMPORT _BOOLEAN initialised;

#ifdef PUPS_SUPPORT
_PUBLIC _BOOLEAN checkpointing = TRUE;
_PUBLIC _BOOLEAN infected      = FALSE;
#endif /* PUPS_SUPPORT */


_PUBLIC int _bubble_lang_flag = 0;
_PUBLIC int main(int argc, char **argv, char **envp)
{    int   bin_des   = (-1);

     off_t pos,
           size;

     char        itag[SSIZE] = "";
     struct stat stat_buf;

     #ifdef THREAD_SUPPORT
     _root_tid = pthread_self();
     #endif /* THREAD_SUPPORT */


     #ifdef PUPS_SUPPORT
     #ifdef IMMUNE
     /*-------------------------------------------------------------*/
     /* The first thing we do is check for possible virus infection */
     /*-------------------------------------------------------------*/

     bin_des = open(argv[0],0);

     (void)fstat(bin_des,&stat_buf);
     pos = lseek(bin_des,stat_buf.st_size,SEEK_SET);

     pos -= 9;
     (void)lseek(bin_des,pos,SEEK_SET);
     (void)read(bin_des,itag,4);

     if(strcmp(itag,"ITAG") == 0)
     {  unsigned long crc,
                      new_crc;

       _BYTE buf[CRC_SIZE];

       (void)read(bin_des,&crc,4);
       (void)lseek(bin_des,0,SEEK_SET); 

       (void)read(bin_des,buf,CRC_SIZE);
       (void)close(bin_des);

       new_crc = pups_crc_p32(CRC_SIZE,buf);
       if(new_crc != crc)
       {  char hostname[SSIZE] = "";

          (void)gethostname(hostname,SSIZE); 
          (void)fprintf(stderr,"\ncmain (%d@%s): CRC error (0x%010x != 0x%010x) binary possibly infected -- aborting\n\n",
                                                                                            getpid(),hostname,crc,new_crc);
          (void)fflush(stderr);

          _exit(-1);
       }
    }
    #endif /* IMMUNE       */
    #endif /* PUPS_SUPPORT */
  
    initialised = TRUE;

    #ifdef BUBBLE_MEMORY_SUPPORT
    return bubble_target(argc, argv, envp);
    #else
    return pups_main(argc, argv, envp);
    #endif /* BUBBLE_MEMORY_SUPPORT */
}
