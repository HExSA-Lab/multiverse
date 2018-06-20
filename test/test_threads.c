#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>

#define OUTNAME "file.txt"

#define BUF_MAX 256

char buf[BUF_MAX];

/* this also runs in the HRT */
static void *
routine1 (void * in)
{
    printf("lowest child active\n");
    return NULL;
}


/* this runs in the HRT */
static void *
routine (void * in)
{

    pthread_t t;
    pthread_create(&t, NULL, routine1, NULL);

#define SIZE (1024*1024*8)
    printf("Child thread is running\n");
    void * x = malloc(SIZE);
    if (!x) {
        fprintf(stderr, "Could not (libc) malloc pages\n");
        return NULL;
    }
    memset(x, 0, SIZE);
    printf("opening a file\n");

    FILE * fd = fopen("newfile.txt", "w+");

    fprintf(fd, "THIS IS A TEST\n");
    fclose(fd);

    printf("HRT thread done!\n");

    if (pthread_join(t, NULL) != 0) {
        fprintf(stderr, "ERROR joining\n");
    }

    return NULL;
}


/* this runs in the ROS */
int 
main (int argc, char * argv[])
{
    pthread_t t;

    printf("Creating child thread in HRT mode\n");

    pthread_create(&t, NULL, routine, NULL);

    printf("Joining child\n");

#if 0
    while (1) { 
        //printf("main thread looping\n");
    }
#endif

    if (pthread_join(t, NULL) != 0) {
        fprintf(stderr, "ERROR joining in main thread\n");
    }

    printf("Joined child (HRT) thread and exiting.\n");

    return 0;
}
