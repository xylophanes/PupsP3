/*------------------------------------------------------------------
   Purpose: daemon which looks to see if local network is connected
            to the Internet. If local network is connected, then it
            starts name resolution services for host.

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

   Version: 2.03
   Dated:   10th December 2024
   E-mail:  mao@tumblingdice.co.uk
-----------------------------------------------------------------*/

#include <stdio.h>
#include <unistd.h>
#include <setjmp.h>
#include <signal.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <bsd/string.h>
#include <stdlib.h>
#include <stdint.h>


/*--------------------------------------*/
/* Definitions used by this application */
/*--------------------------------------*/
/*----------------------*/
/* Version of connected */
/*----------------------*/
#define CONNECTED_VERSION    "2.03" 


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE                2048 


#define INTERNET_UNREACHABLE 0
#define INTERNET_REACHABLE   1
#define PING_TIMEOUT         5 
#define NET_UMASK            0xff

#ifndef P3_SUPPORT
#define _BOOLEAN              int32_t
#define _PRIVATE             static
#define _PUBLIC

#ifdef FALSE
#undef FALSE
#endif
#define FALSE                0

#ifdef TRUE
#undef TRUE 
#endif
#define TRUE                 255

#endif /* P3_SUPPORT */


/*-------------------------------------------------*/
/* Variables which are private to this application */
/*-------------------------------------------------*/

_PRIVATE sigjmp_buf env;




/*--------------------------------*/
/* Handler for (SIGALRM) timeouts */
/*--------------------------------*/

_PRIVATE void alarm_handler(int signo)

{   (void)siglongjmp(env,1);
}




/*---------------------------------*/
/* Main entry point to application */
/*---------------------------------*/

_PUBLIC int32_t main(int32_t argc, char *argv[])

{    int32_t i,
             ping_timeout = PING_TIMEOUT,
             cnt          = 0;

    _BOOLEAN state = INTERNET_UNREACHABLE;

    char line[SSIZE]         = "",
         hostname[SSIZE]     = "",
         nameserver[SSIZE]   = "",
         upscript[SSIZE]     = "",
         downscript[SSIZE]   = "",
         ping_command[SSIZE] = "";

    FILE *ping_stream        = (FILE *)NULL;


    /*--------------------------------------*/
    /* Daemon runs in its own process group */
    /*--------------------------------------*/

    (void)setsid();


    /*-------------------------------------------------------------*/
    /* Check that we actually have resolv.up and resolv.down files */
    /*-------------------------------------------------------------*/

    if(access("/etc/resolv.up",F_OK | R_OK | W_OK) == (-1) && access("/etc/resolv.down",F_OK | R_OK | W_OK) == (-1))
    {  (void)fprintf(stderr,"connected: cannot find \"/etc/resolv.up\" or \"/etc/resolv.down\" -- aborting\n");
       (void)fflush(stderr);

       exit(255);
    }


    /*------------------------------------------------------*/
    /* Get the I.P. address of the remote nameserver we are */
    /* supposed to query.                                   */
    /*------------------------------------------------------*/

    if(argc != 4)
    {  (void)fprintf(stderr,"\nConnected version %s, (C) Tumbling Dice, 2002-2024 (gcc %s: built %s %s)\n",CONNECTED_VERSION,__VERSION__,__TIME__,__DATE__);
       (void)fprintf(stderr,"\nUsage: connected <nameserver> <upscript> <downscript> [<ping timeout>]\n\n");
       (void)fprintf(stderr,"CONNECTED is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"CONNECTED comes with ABSOLUTELY NO WARRANTY\n\n");
       (void)fflush(stderr);

       (void)fflush(stderr);

       exit(255);
    }
    else
    {  char netname[SSIZE]         = "";

       uint64_t nameserver_address = 0x0,
                host_address       = 0x0;

       struct in_addr ns_addr;
       in_addr_t      ns_net;

       struct netent *host_netent = (struct netent *)NULL;


       /*----------------------------------------------*/
       /* If this is a name convert to an I.P. address */
       /*----------------------------------------------*/

       if((ns_net = inet_network(argv[1])) != 0)
          (void)strlcpy(nameserver,argv[1],SSIZE);
       else
       {  (void)fprintf(stderr,"connected: address is not in I.P. format\n");
          (void)fflush(stderr);

          exit(255);
       }


       /*----------------------------------------------------------------------*/
       /* Check that the address we are monitoring is not on our local network */
       /* (if it is it is a waste of time looking at it!)                      */
       /*----------------------------------------------------------------------*/

       if(getdomainname(netname,SSIZE) == (-1))
       {  (void)fprintf(stderr,"connected: could not get local network number\n");
          (void)fflush(stderr);

          exit(255);
       }
       else
       {  if((host_netent = getnetbyname(netname)) == (struct netent *)NULL)
          {  (void)fprintf(stderr,"connected: could not get network number for local host\n");
             (void)fflush(stderr);

            exit(255);
          }
       }


       /*----------------------------------*/      
       /* Assumes local network is class C */
       /*----------------------------------*/      

       if(abs((unsigned long)ns_net - host_netent->n_net) <= NET_UMASK)
       {  (void)fprintf(stderr,"connected: nameserver I.P. address is local\n");
          (void)fflush(stderr);

          exit(255);
       }

       (void)strlcpy(upscript,argv[2],SSIZE);
       (void)strlcpy(downscript,argv[3],SSIZE);

       if(access(upscript,F_OK | R_OK | W_OK) == (-1))
       {  if(access("/etc/connected.d/connected.upscript",F_OK | R_OK | W_OK) == (-1))
          {  (void)fprintf(stderr,"connected: cannot find script to run when gateway comes up\n");
             (void)fflush(stderr);

             exit(255);
          }
          else
             (void)strlcpy(upscript,"/etc/connected.d/connected.upscript",SSIZE);
       }

       if(access(downscript,F_OK | R_OK | W_OK) == (-1))
       {  if(access("/etc/connected.d/connected.downscript",F_OK | R_OK | W_OK) == (-1))
          {  (void)fprintf(stderr,"connected: cannot find script to run when gateway goes down\n");
             (void)fflush(stderr);

             exit(255);
          }
          else
             (void)strlcpy(upscript,"/etc/connected.d/connected.downscript",SSIZE);
       }
    }


    /*------------------------------------------*/
    /* Are we using a non-default ping timeout? */
    /*------------------------------------------*/

    if(argc == 5)
    {  if(sscanf(argv[5],"%d",&ping_timeout) != 1)
       {  (void)fprintf(stderr,"connected: expecting ping timeout (in seconds)\n");
          (void)fflush(stderr);

          exit(255);
       }
    }


    /*-------------------------*/
    /* Display startup options */
    /*-------------------------*/

    (void)gethostname(hostname,SSIZE);
    (void)fprintf(stderr,"connected (%d@%s): querying nameserver %s\n",getpid(),hostname,nameserver);
    (void)fprintf(stderr,"connected (%d@%s): upscript \"%s\"\n",getpid(),hostname,upscript);
    (void)fprintf(stderr,"connected (%d@%s): downscript \"%s\"\n",getpid(),hostname,downscript);
    (void)fprintf(stderr,"connected (%d@%s): ping timeout is %d seconds\n",getpid(),hostname,ping_timeout);
    (void)fflush(stderr);


    /*---------------------------------------------------------*/
    /* Main loop -- try to ping nameserver -- once name server */
    /* is pingable bring up resolv.conf                        */
    /*---------------------------------------------------------*/

    (void)snprintf(ping_command,SSIZE,"csh -c \"ping %s |& cat\"",nameserver);
    if((ping_stream = popen(ping_command,"r")) == (FILE *)NULL)
    {  (void)fprintf(stderr,"connected: failed to start \"ping\" command\n");
       (void)fflush(stderr);

       exit(255);
    }


    /*------------------------------------------*/
    /* Initally assume Internet gateway is down */
    /*------------------------------------------*/

    (void)system(downscript);


    /*------------------------------------------------------------------*/
    /* Make sure ping command is launched before we read data from pipe */
    /*------------------------------------------------------------------*/

    (void)sleep(5);

    (void)fprintf(stderr,"connected: (%d@%s): started\n",getpid(),hostname);
    (void)fflush(stderr);


    /*----------------------------------------------*/
    /* Main loop -- mointor Internet gateway status */
    /*----------------------------------------------*/

    while(TRUE)
    {

       /*-----------------------------------------*/
       /* Wait for Internet connection to come up */ 
       /*-----------------------------------------*/

       do {    (void)fgets(line,SSIZE,ping_stream);


               /*-----------------------------------------*/
               /* If we get an unknown host here -- abort */
               /*-----------------------------------------*/

               if(strncmp(line,"ping: unknown host",strlen("ping: unknown host")) == 0)
               {  (void)pclose(ping_stream);

                  (void)fprintf(stderr,"connected: unknown host (%s)\n",nameserver);
                  (void)fflush(stderr);

                  exit(255);
               }
 
               (void)usleep(100);
          } while(strncmp(line,"From",4) == 0 || strncmp(line,"PING",4) == 0);

       state = INTERNET_REACHABLE;
       (void)system(upscript);

       (void)fprintf(stderr,"connected: (%d@%s): gateway to Internet up (running upscript \"%s\")\n",
                                                                    getpid(),hostname,upscript);
       (void)fflush(stderr);


       /*-------------------*/
       /* Flush ping buffer */
       /*-------------------*/

       do {    (void)fgets(line,SSIZE,ping_stream);
          } while(strncmp(line,"From",4) == 0);


       /*-----------------------------------------*/
       /* Wait for Internet connection to go down */
       /*-----------------------------------------*/

       do {    if(sigsetjmp(env,1) == 0)
               {  (void)signal(SIGALRM,(void *)&alarm_handler);
                  (void)alarm(ping_timeout);
                  (void)fgets(line,SSIZE,ping_stream);
                  (void)alarm(0);
                  (void)signal(SIGALRM,SIG_DFL);
               }
               else
                  state = INTERNET_UNREACHABLE;

               (void)usleep(100);
          } while(state == INTERNET_REACHABLE);

       (void)system(downscript);

       (void)fprintf(stderr,"connected: (%d@%s): gateway to Internet down (running downscript \"%s\")\n",
                                                                      getpid(),hostname,downscript);
       (void)fflush(stderr);


       /*-------------------*/
       /* Flush ping buffer */
       /*-------------------*/

       do {    (void)fgets(line,SSIZ,ping_stream);
          } while(strncmp(line,"From",4) != 0);
    }
}
