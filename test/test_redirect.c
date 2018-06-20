#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>

#define OUTNAME "file.txt"

#define BUF_MAX 256

char buf[BUF_MAX];


static void *
routine (void * in)
{
    int fd;
    FILE * output;
    FILE * outfile;

    outfile = fopen(OUTNAME, "w+");
    if (!outfile) {
        perror("Couldn't open outfile\n");
        return NULL;
    }

    fprintf(outfile, "Hello World.\n");

    fclose(outfile);
    return NULL;
}


int 
main (int argc, char * argv[])
{
    pthread_t t;

    printf("Creating child thread in HRT mode\n");

    pthread_create(&t, NULL, routine, NULL);

    printf("Joining child\n");

    pthread_join(t, NULL);

    printf("Joined child (HRT) thread and exiting.\n");

    return 0;
}
