#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <linux/types.h>

#include "driver-api.h"


int main (int argc, char ** argv) 
{
    int ret;
    int fd = open("/dev/hvm", O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Could not open hvm-del device: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct hvm_del_req req;

    req.port = 0x7c4;
    req.val  = 0xb;

    printf("Performing delegation ioctl\n");
    ret = ioctl(fd, HVM_DEL_IOCTL_PERFORM_OUTB,
        (void*)&req);

    if (ret != 0) {
        fprintf(stderr, "Ioctl failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    close(fd);

    return 0;
}
