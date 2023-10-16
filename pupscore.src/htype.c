/*-------------------------------------------------------------------------
    Purpose: Test to see if linux is running in a container 

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.00 
    Dated:   4th January 2022
    E-mail:  mao@tumblingdice.co.uk
-------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


/*------------------*/
/* Version of htype */
/*------------------*/

#define HTYPE_VERSION    "2.00"


/*-------------*/
/* String size */
/*-------------*/

#define SSIZE            2048 


/*------------------*/
/* Main entry point */
/*------------------*/

int main(int argc, char *argv[])

{   char str[SSIZE] = "",
         cmd[SSIZE] = "ps aux | head -2 | grep systemd";

    FILE *pstream = (FILE *)NULL;

    if((argc == 2) && strcmp(argv[1],"-usage") == 0 || argc > 2)
    {  (void)fprintf(stderr,"\nhtype version %s, (C) Tumbling Dice 2000-2022 (built %s %s)\n\n",HTYPE_VERSION,__TIME__,__DATE__);
       (void)fprintf(stderr,"HTYPE is free software, covered by the GNU General Public License, and you are\n");
       (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
       (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
       (void)fprintf(stderr,"HTYPE comes with ABSOLUTELY NO WARRANTY\n\n");

       (void)fprintf(stderr,"\nusage: htype [-usage]\n\n");
       (void)fflush(stderr);

       exit(255);
    }

    if((pstream = popen(cmd,"r")) == (FILE *)NULL)
    {  (void)fprintf(stderr,"htype: system probe command failed\n");
       (void)fflush(stderr);

       exit(255);
    }

    (void)fgets(str,SSIZE,pstream);
    (void)pclose(pstream);

    if(strcmp(str,"") == 0)
       (void)fprintf(stdout,"htype: linux is running in a (Docker or LXC) CONTAINER\n");
    else 
       (void)fprintf(stdout,"htype: linux is running on a regular or virtual HOST\n");
    (void)fflush(stdout);

    exit(0);
}

