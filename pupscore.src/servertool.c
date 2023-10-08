/*---------------------------------------------------------------------------------------------
    Purpose: starts PSRP server writing error log to new window.

    Author: M.A. O'Neill
            Tumbling Dice
            Gosforth
            Oxfordshire
            OX11 8QY

    Version: 2.01 
    Dated:   24th May 2022
    E-mail:  mao@tumblingdice.co.uk
----------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <xtypes.h>
#include <string.h>
#include <bsd/string.h>
#include <stdlib.h>


/*-----------------------*/
/* Version of servertool */
/*-----------------------*/

#define SERVERTOOL_VERSION    "2.01"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE                 2048 




/*------------------*/
/* Main entry point */
/*------------------*/

_PUBLIC int main(int argc, char *argv[])

{   int i;

    char server_name[SSIZE]   = "",
         server_hname[SSIZE]  = "",
         terminator[SSIZE]    = "",
         host_name[SSIZE]     = "",
         xterm_command[SSIZE] = "";


    /*---------------------------------------------------------*/
    /* Display help information if "-help" or "-usage" flagged */
    /*---------------------------------------------------------*/

    if(argc == 1 || (argc == 2 && (strcmp(argv[1],"-usage") == 0 || strcmp(argv[1],"-help") == 0)))
    {  (void)fprintf(stderr,"\nservertool version %s, (C) Tumbling Dice 2003-2022 (built %s %s)\n\n",SERVERTOOL_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"\nUsage: psrptool [-help | -usage] | !server!  [PSRP server  argument list]\n\n");                                              

       (void)fprintf(stderr,"SERVERTOOL is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"SERVERTOOL comes with ABSOLUTELY NO WARRANTY\n\n");
               
       exit(255);
    }


    /*----------------------*/    
    /* Get psrp server name */
    /*----------------------*/    

    (void)strcpy(server_name,argv[1]);


    /*---------------------------*/ 
    /* Set client and host names */
    /*---------------------------*/ 

    (void)gethostname(host_name,SSIZE);
    (void)snprintf(server_hname,SSIZE,"%s@%s",server_name,host_name);

    for(i=2; i<argc; ++i) 
    {

       /*-----------------------------------------*/
       /* Note remote host to connect to (if any) */
       /*-----------------------------------------*/

       if(strcmp(argv[i],"-on") == 0)
       {  if(argv[i+1][0] == '-')
          {  (void)fprintf(stderr,"\nservertool: illegal command tail syntax\n\n");
             (void)fflush(stderr);

             exit(255);
          }
          (void)snprintf(host_name,SSIZE,argv[i+1]);
       }          
    }

    for(i=2; i<argc; ++i) 
    {

       /*-------------------*/
       /* Note pen (if any) */
       /*-------------------*/

       if(strcmp(argv[i],"-pen") == 0)
       {  if(argv[i+1][0] == '-')
          {  (void)fprintf(stderr,"\nservertool: illegal command tail syntax\n\n");
             (void)fflush(stderr);

             exit(255);
          }
          (void)snprintf(server_hname,SSIZE,"%s@%s",argv[i+1],host_name);
       }
    }


    /*-----------------------------*/
    /* Get argument list for xterm */
    /*-----------------------------*/

    (void)snprintf(xterm_command,SSIZE,"xterm -geometry 132x20 -T \"psrp server: %s\" -e %s",server_hname,server_name);
    for(i=2; i<argc; ++i)
    {  if(strcmp(argv[1],"-usage") != 0 && strcmp(argv[1],"-help") != 0)
       {  (void)strlcat(xterm_command," ",    SSIZE);
          (void)strlcat(xterm_command,argv[i],SSIZE);
       }


       /*-------------------*/
       /* Note pen (if any) */
       /*-------------------*/

       if(strcmp(argv[i],"-pen") == 0)
          (void)strcpy(server_name,argv[i+1]);
    }


    /*---------------------------------------------------*/
    /* Server MUST terminate if servertool is terminated */
    /*---------------------------------------------------*/

    (void)snprintf(terminator,SSIZE," -parent %d -parent_exit ",getpid());
    (void)strcat(xterm_command,terminator);


    /*------------------------------*/        
    /* Run psrp client in new xterm */
    /*------------------------------*/        

    (void)system(xterm_command);
    exit(0);
}
