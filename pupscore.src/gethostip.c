/*----------------------------------------------------------------------
    Purpose: Get host I.P (by interrogating interface).

    Author:  M.A. O'Neill
             Tumbling Dice Ltd
             Gosforth
             Newcastle upon Tyne
             NE3 4RT
             United Kingdom

    Version: 2.02 
    Dated:   24th May 2023
    E-Mail:  mao@tumblingdice.co.uk
-----------------------------------------------------------------------*/

#include <xtypes.h>
#include <stdio.h>
#include <string.h>
#include <bsd/string.h>
#include <stdlib.h>


/*----------------------------------------------------------------------
    Defines which are local to this application ...
-----------------------------------------------------------------------*/
/*----------------------*/
/* Version of gethostip */
/*----------------------*/

#define GETHOSTIP_VERSION    "2.02"

/*-------------*/
/* String size */
/*-------------*/

#define SSIZE                2048 




/*-----------------------------------------------------------------------
    Remove whitespace and ':' characters from line ...
-----------------------------------------------------------------------*/

_PRIVATE void remove_char(char rm_char, char *line)

{   int  i;
    char tmp_line[SSIZE];

    for(i=0; i<strlen(line); ++i)
    {  if(line[i] == rm_char)
	  line[i] = ' ';
    }

    for(i=0; i<strlen(line); ++i)
    {  if(line[i] != ' ')
       {  (void)strlcpy(tmp_line,&line[i],SSIZE);
	  (void)strlcpy(line,tmp_line,    SSIZE);

	  return;
       }
    }
}



/*-----------------------------*/
/* Entry point to main program */
/*-----------------------------*/

_PUBLIC int main(int argc, char *argv[])

{   FILE *stream = (FILE *)NULL;

    char line[SSIZE],
         strdum[SSIZE],
         ip_addr[SSIZE],
         ifconfig_command[SSIZE];

    _BOOLEAN extract_c_subnet = FALSE;

    if(argc >= 2)
    {  if(strcmp(argv[1],"-usage") == 0 | strcmp(argv[1],"-help") == 0)
       {  (void)fprintf(stderr,"\ngethostip version %s, (C) Tumbling Dice 2003-2023 (built %s %s)\n\n",GETHOSTIP_VERSION,__TIME__,__DATE__);
          (void)fprintf(stderr,"GETHOSTIP is free software, covered by the GNU General Public License, and you are\n");
          (void)fprintf(stderr,"welcome to change it and/or distribute copies of it under certain conditions.\n");
          (void)fprintf(stderr,"See the GPL and LGPL licences at www.gnu.org for further details\n");
          (void)fprintf(stderr,"GETHOSTIP comes with ABSOLUTELY NO WARRANTY\n\n");
          (void)fprintf(stderr,"\nUsage: gethostip [-usage | -help] -cnet] | [interface:eth0]\n\n");
          (void)fflush(stderr);

          exit(255);
       }
    }

    if(argc > 1 && strcmp(argv[1],"-cnet") == 0)
       extract_c_subnet = TRUE;

    if(argc == 3 && extract_c_subnet == TRUE)
       (void)snprintf(ifconfig_command,SSIZE,"ifconfig %s",argv[2]);
    else if(argc == 2 && extract_c_subnet == TRUE)
       (void)snprintf(ifconfig_command,SSIZE,"ifconfig eth0");
    else if(argc == 2 && extract_c_subnet == FALSE)
       (void)snprintf(ifconfig_command,SSIZE,"ifconfig %s",argv[1]);
    else
       (void)snprintf(ifconfig_command,SSIZE,"ifconfig eth0");

    stream = popen(ifconfig_command,"r");


    /*------------------------------------*/
    /* Read in data from ifconfig command */
    /*------------------------------------*/

    (void)fgets(line,255,stream);
    (void)fgets(line,255,stream);
    (void)pclose(stream);


    /*----------------------*/
    /* Clean out whitespace */
    /*----------------------*/

    remove_char(':',line);


    /*--------------------------------------------*/
    /* Now break out I.P. address (and return it) */
    /*--------------------------------------------*/

    if(strncmp(line,"inet addr",strlen("inet addr")) == 0)
    {  (void)sscanf(line,"%s %s %s",strdum,strdum,ip_addr);

       if(extract_c_subnet == TRUE)
       {  char ip_c_addr[SSIZE] = "";
	       
	  remove_char('.',ip_addr); 
	  (void)sscanf(ip_addr,"%s%s%s%s",strdum,strdum,strdum,ip_c_addr);
	  printf("%s\n",ip_c_addr);
       }
       else
	  printf("%s\n",ip_addr);

       (void)fflush(stdout);
    }
    else
    {  (void)fprintf(stderr,"gethostip: interface \"%s\" is not configured\n\n",argv[1]);
       (void)fflush(stderr);

       exit(255);
    }

    exit(0);
}
