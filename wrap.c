#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <libelf/libelf.h>
#include "hrtrt.h"


//
// We assume we have the HRT up and running,
// it obeys the calling convention we are using here
// and it has its address space merged with ours.
//
uint64_t
call_aerokernel_func (void (*func)(void), 
		      uint64_t a1,
		      uint64_t a2,
		      uint64_t a3,
		      uint64_t a4,
		      uint64_t a5,
		      uint64_t a6,
		      uint64_t a7,
		      uint64_t a8,
		      char * name)
{
    uint64_t ret;
    uint64_t call_buf[12] = {(uint64_t)func,(uint64_t)&ret,a1,a2,a3,a4,a5,a6,a7,a8,0,0};
#if 0
    uint64_t * call_buf = NULL;
    if (posix_memalign((void**)&call_buf, 16, sizeof(uint64_t)*12) != 0) {
        fprintf(stderr, "Could not execute aerokernel call\n");
        exit(EXIT_FAILURE);
    }
    mlock(call_buf, sizeof(uint64_t)*12);

    call_buf[0] = (uint64_t)func;
    call_buf[1] = (uint64_t)&ret;
    call_buf[2] = a1;
    call_buf[3] = a2;
    call_buf[4] = a3;
    call_buf[5] = a4;
    call_buf[6] = a5;
    call_buf[7] = a6;
    call_buf[8] = a7;
    call_buf[9] = a8;
    call_buf[10] = 0;
    call_buf[11] = 0;
#endif

    DEBUG("Calling %s (%p) (no args yet)\n", name, func);

    v3_hvm_ros_invoke_hrt_async((void*)call_buf,0);

    // TODO: don't leak the call buf (or just have it be a global
    return ret;
}


int __wrap_main(int argc, char * argv[]);

int 
__wrap_main(int argc, char * argv[]) 
{
    hrt_runtime_init(argc, argv);
    return __real_main(argc, argv);
}

