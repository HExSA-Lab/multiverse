#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <asm/prctl.h>
#include <sys/prctl.h>
#include <sched.h>
#include <sys/mman.h>

#include "hrtrt.h"
#include "v3_hvm_ros_user.h"
#include "wrap.h"

#define STACK_SIZE (1024*1024*8)
#define MAX_HRT_THREADS 128


#include "hashtable.h"


extern Elf * elf_handle;
extern int hrt_mode_enabled;
extern int hrt_mode_switch(void);

extern struct nk_hashtable * thread_hash;
extern struct nk_hashtable * partner_thread_hash;


/* - If function overrides are enabled, the first pthread will be a "master" in that it will
 *   be the one thread connecting the ROS and the HRT. All of its children will be in HRT mode
 *   (or will have a divergent worldline in the other "universe")
 *
 * - we need to be able to join this thread from the ROS
 * 
 *
 */


static void * 
partner (void * in)
{
    struct thread_data * td = (struct thread_data*)in;
    unsigned long long rc, num, a1=0, a2=0, a3=0, a4=0, a5=0, a6=0, a7=0, a8=0;
    struct v3_ros_event event;
    uint64_t fsbase;

    memset(&event, 0xff, sizeof(event));

    rc = 1;

    // get fsbase
    arch_prctl(ARCH_GET_FS, &fsbase);

    DEBUG("Partner requesting GDT sync with fsbase=%p\n", (void*)fsbase);

    // sync the GDT
    if (v3_hvm_ros_mirror_gdt(fsbase)) {
        fprintf(stderr, "Could not mirror GDT area\n");
        exit(EXIT_FAILURE);
    }

    // create the nautilus thread
    rc = call_aerokernel_func(td->call_addr,
            td->nk_func,
            (uint64_t)NULL,
            (uint64_t)td->stack,
            td->type,
            0,
            (uint64_t)&(td->nk_thread_id),
            0,
            0,
            td->ak_func);

    if (rc != 0) {
        fprintf(stderr, "Could not create Nautilus thread\n");
        exit(EXIT_FAILURE);
    }

    // notify our parent that we created the nautilus thread
    __sync_fetch_and_or(&td->hrt_thread_started, 1);

    // wait on it to complete, checking for events in the process
    while (!__sync_fetch_and_and(&td->is_exited, 1)) {
        num = 0xf00d;
        a1 = 0xf;
        a2 = (unsigned long long)&event;
        HCALL(rc,num,a1,a2,a3,a4,a5,a6,a7,a8);
        if (event.event_type != ROS_NONE) {
            if (event.event_type == HRT_THREAD_EXIT) {
                // we're done here, ack it
                a1 = 0x1f;
                a2 = 0;
                HCALL(rc,num,a1,a2,a3,a4,a5,a6,a7,a8);
                // HACK: we want to let the Nautilus side finish descheduling the 
                // thread before we kill the stack etc.
                sleep(2);
                return NULL;
            } else {
                v3_hvm_handle_ros_event(&event);
            }
        }
    }

    return NULL;
}



int __real_pthread_create(pthread_t* a0, const pthread_attr_t * a1, void *(*a2)(void*), void * a3);

int __wrap_pthread_create(pthread_t * a0, const pthread_attr_t * a1, void *(*a2)(void*), void * a3);
int 
__wrap_pthread_create (pthread_t * a0, const pthread_attr_t * a1, void *(*a2)(void*), void * a3)
{

    char * ak_func;
    struct thread_data * td = NULL;
    uint64_t type, rc;
    pthread_t p;
    void * stack;
    void * call_addr;

    // we align the stack to a 4K page boundary just to be safe
    if (posix_memalign(&stack, 0x1000, STACK_SIZE) != 0) {
        fprintf(stderr, "could not malloc thread stack in ROS\n");
        exit(EXIT_FAILURE);
    }
    mlock(stack, STACK_SIZE);

    ak_func = "nk_thread_start";

    if (!ak_func) {
        fprintf(stderr, "Could not find corresponding AeroKernel func name for (%s)\n", __func__);
        exit(EXIT_FAILURE);
    }

    call_addr = find_aerokernel_addr(elf_handle, ak_func);
    if (!call_addr) {
        fprintf(stderr, "Could not find target for (%s)\n", ak_func);
        exit(EXIT_FAILURE);
    }

    type = hrt_mode_enabled ? 3 : 2;

    if (!hrt_mode_enabled) {

        /* from now on, threads will be pure nautilus threads, i.e. no partner */
        if (hrt_mode_switch() != 0) {
            fprintf(stderr, "Could not switch into HRT mode, defaulting to original function\n");
            exit(EXIT_FAILURE);
        }
        
        td = malloc(sizeof(struct thread_data));
        if (!td) {
            fprintf(stderr, "Could not malloc partner thread data\n");
            exit(EXIT_FAILURE);
        }
        memset(td, 0, sizeof(struct thread_data));

        td->ak_func   = ak_func;
        td->call_addr = call_addr;
        td->nk_func   = (uint64_t)a2;
        td->type      = (uint64_t)type;
        td->stack     = stack;

        DEBUG("Creating partner thread\n");
        if (rc = __real_pthread_create(&p, NULL, partner, (void*)td)) {
            fprintf(stderr, "Could not create partner thread, exiting\n");
            exit(EXIT_FAILURE);
        }

        // wait on partner to finish creating nautilus thread
        while (td->hrt_thread_started != 1) { }

        // overwrite the return value with partner thread's opaque ID
        *a0 = p;

        // nk_thread_id should now be filled in 
        DEBUG("Inserting thread data into hash table\n");
        nk_htable_insert(thread_hash, (addr_t)td->nk_thread_id, (addr_t)td);

        /* we keep a separate hashtable for partner threads, this will make lookup easy when we join it */
        nk_htable_insert(partner_thread_hash, (addr_t)p, (addr_t)td);

    } else {
        int (*func)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) =  
            (int (*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t))  call_addr;

        rc = func((uint64_t)a2, 0, (uint64_t)stack, (uint64_t)type, 0, (uint64_t)a0, 0, 0);
    }

    return rc;
}


int __real_pthread_join(pthread_t a0,void ** a1);
int __wrap_pthread_join(pthread_t a0,void ** a1);

int 
__wrap_pthread_join(pthread_t a0, void ** a1)
{
    struct thread_data * td = NULL;
    if (td = (struct thread_data*)nk_htable_search(partner_thread_hash, (addr_t)a0)) {
        DEBUG("Joining ROS thread\n");
        int rc = __real_pthread_join(a0, a1);
        if (rc != 0) {
            fprintf(stderr, "Could not join partner thread\n");
            exit(EXIT_FAILURE);
        }

        DEBUG("Cleaning up partner thread data\n");

        // KCH NOTE: There's a race between the ROS-side join freeing the HRT thread stack
        // and the HRT thread descheduling itself. If we free the stack *immediately* after
        // we've received notification from the HRT that the thread is exiting, then when it
        // calls schedule(), it won't have a stack and will fall over
        //free(td->stack);

        // clean up the hash entries for this thread
        nk_htable_remove(thread_hash, (addr_t)td->nk_thread_id, 0);
        nk_htable_remove(partner_thread_hash, (addr_t)a0, 0);

        // we're done with this thread
        free(td);

        return rc;

    } else {
        /* we're in the HRT */
        char * ak_func;
        int rc;
        ak_func = "nk_join";

        if (!ak_func) {
            fprintf(stderr, "Could not find corresponding AeroKernel func name for (%s)\n", __func__);
            exit(EXIT_FAILURE); }

        void * call_addr = find_aerokernel_addr(elf_handle, ak_func);
        if (!call_addr) {
            fprintf(stderr, "Could not find target for (%s)\n", ak_func);
            exit(EXIT_FAILURE);
        }

        int (*func)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) =  
            (int (*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t))  call_addr;

        rc = func((uint64_t)a0, (uint64_t)a1,0,0,0,0,0,0);

        // TODO: we need a way to free the stack for *child* HRT threads
        return rc;
    }
}
