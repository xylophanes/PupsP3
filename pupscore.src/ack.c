#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])

{   int  i         = 0;
    char line[256] = "";

    (void)fprintf(stdout,"PSRP\n");
    (void)fflush(stdout);

    do {    fgets(line,256,stdin);

            (void)fprintf(stdout,"DONE: %d\n",i);
            (void)fflush(stdout);

            (void)fprintf(stderr,"NOW: %d\n",i);
            (void)fflush(stderr);

            ++i;

       } while(1);

}
