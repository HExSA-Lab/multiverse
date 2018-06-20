#ifndef __HRT_RUNTIME_H__
#define __HRT_RUNTIME_H__

#include <libelf/libelf.h>

#if DEBUG_ENABLE==1
#define DEBUG(fmt, args...) printf("DEBUG: " fmt, ##args)
#else
#define DEBUG(fmt, args...)
#endif

#define SIG_STACK_SIZE 8192

struct thread_data {
    int is_exited; 
    void * nk_thread_id;     // nautilus thread id
    void * stack;            // stack that is allocated in ROS

    int hrt_thread_started;

    void (*call_addr)(void);  // address of nk_thread_create
    uint64_t nk_func;         // function that nautilus thread should execute
    uint64_t type;            // is it a top-level HRT thread or a child?
    char * ak_func;           // nautilus name of function

};

int hrt_runtime_init(int argc, char * argv[]);
void hrt_runtime_deinit(void);
void * find_aerokernel_addr (Elf * elf, const char * symname);

struct v3_ros_event {
    enum { ROS_NONE=0, ROS_PAGE_FAULT=1, ROS_SYSCALL=2, HRT_EXCEPTION=3, HRT_THREAD_EXIT=4} event_type;
    uint64_t       last_ros_event_result; // valid when ROS_NONE
    union {
	struct {   // valid when ROS_PAGE_FAULT
	    uint64_t rip;
	    uint64_t cr2;
	    enum {ROS_READ, ROS_WRITE} action;
	} page_fault;
	struct { // valid when ROS_SYSCALL
	    uint64_t args[8];
	} syscall;
    struct { // valid when HRT_THREAD_EXIT
        uint64_t nktid;
    } thread_exit;

    };
};

#define HCALL64(rc,id,a,b,c,d,e,f,g,h)		      \
  asm volatile ("movq %1, %%rax; "		      \
		"pushq %%rbx; "			      \
		"movq $0x6464646464646464, %%rbx; "   \
		"movq %2, %%rcx; "		      \
		"movq %3, %%rdx; "		      \
		"movq %4, %%rsi; "		      \
		"movq %5, %%rdi; "		      \
		"movq %6, %%r8 ; "		      \
		"movq %7, %%r9 ; "		      \
		"movq %8, %%r10; "		      \
		"movq %9, %%r11; "		      \
		"vmmcall ;       "		      \
		"movq %%rax, %0; "		      \
		"popq %%rbx; "			      \
		: "=m"(rc)			      \
		: "m"(id),			      \
                  "m"(a), "m"(b), "m"(c), "m"(d),     \
		  "m"(e), "m"(f), "m"(g), "m"(h)      \
		: "%rax","%rcx","%rdx","%rsi","%rdi", \
		  "%r8","%r9","%r10","%r11"	      \
		)

#define HCALL32(rc,id,a,b,c,d,e,f,g,h)		      \
  asm volatile ("movl %1, %%eax; "		      \
		"pushl %%ebx; "			      \
		"movl $0x32323232, %%ebx; "	      \
		"pushl %9;"			      \
		"pushl %8;"			      \
		"pushl %7;"			      \
		"pushl %6;"			      \
		"pushl %5;"			      \
		"pushl %4;"			      \
		"pushl %3;"			      \
		"pushl %2;"			      \
		"vmmcall ;       "		      \
		"movl %%eax, %0; "		      \
		"addl $32, %%esp; "		      \
		"popl %%ebx; "			      \
		: "=r"(rc)			      \
		: "m"(id),			      \
		  "m"(a), "m"(b), "m"(c), "m"(d),     \
		"m"(e), "m"(f), "m"(g), "m"(h)	      \
		: "%eax"			      \
		)

#ifdef __x86_64__
#define HCALL(rc,id,a,b,c,d,e,f,g,h)  HCALL64(rc,id,a,b,c,d,e,f,g,h)
#else
#define HCALL(rc,id,a,b,c,d,e,f,g,h)  HCALL32(rc,id,a,b,c,d,e,f,g,h)   
#endif


#endif

