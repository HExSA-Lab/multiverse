#define _GNU_SOURCE 1
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <syscall.h>
#include <ucontext.h>
#include <sys/types.h>
#include <sched.h>
#include <dlfcn.h>
#include <signal.h>
#include <libelf/libelf.h>
#include <hrtrt.h>
#include "v3_hvm_ros_user.h"

#include "hashtable.h"


/* - read in a map of functions from config file */
/* - find the aerokernel functions in the binary blob */
/* - take each instance of the domain and map it to the nk functions
 *   (e.g. rather than pthreads). this is where we need libelf (for now)
 */

extern const void __aerokernel_start;
extern const void __aerokernel_end;

Elf * elf_handle;

static char * theelf;
static unsigned long elf_size;
static void * sig_stack;
struct nk_hashtable * thread_hash = NULL;
struct nk_hashtable * partner_thread_hash = NULL;


static uint_t 
thread_hash_fn (addr_t key) 
{
    return nk_hash_long((ulong_t)key, sizeof(ulong_t)*8);
}


static int
thread_eq_fn (addr_t key1, addr_t key2)
{
    return key1 == key2;
}


static void 
notify_ros_thread (uint64_t tid)
{
    struct thread_data * td = (struct thread_data*)nk_htable_search(thread_hash, (addr_t)tid);

    if (!td) {
        fprintf(stderr, "Could not find associated ROS thread for Aerokernel thread %p\n", (void*)tid);
        return;
    }

    DEBUG("Notifying ROS partner thread to shut down\n");

    /* notify it */
    __sync_fetch_and_or(&td->is_exited, 1);
}


static Elf64_Shdr * 
find_section (Elf * elf, const char * name)
{
    Elf_Scn * ret = NULL;
    Elf64_Shdr *shdr;
    size_t shstrndx;
    char * secname;

    if (elf_getshstrndx(elf, &shstrndx) != 1) {
        fprintf(stderr, "Could not find section hdr index\n");
        return NULL;
    }

    while ((ret = elf_nextscn(elf, ret)) != NULL) {
        if ((shdr = elf64_getshdr(ret)) == NULL) {
            fprintf(stderr, "Could not get secion header\n");
            return NULL;
        }
        
        if ((secname = elf_strptr(elf, shstrndx, shdr->sh_name)) == NULL) {
            fprintf(stderr, "Could not get section name\n");
            return NULL;
        }

        if (strncmp(secname, name, 128) == 0) {
            return shdr;
        }
    }

    return NULL;
}


void*
find_aerokernel_addr (Elf * elf, const char * symname)
{
    Elf64_Shdr * symtab;
    Elf64_Shdr * strtab;
    Elf64_Sym * symbol;

    if ((symtab = find_section(elf, ".symtab")) == NULL) {
        fprintf(stderr, "Could not find ELF symbol table\n");
        return NULL;
    }

    if ((strtab = find_section(elf, ".strtab")) == NULL) {
        fprintf(stderr, "Could not find ELF string table\n");
        return NULL;
    }

    symbol = (Elf64_Sym*)(theelf + symtab->sh_offset);

    char * limit = theelf + symtab->sh_offset + symtab->sh_size;
    char * strlimit = theelf + strtab->sh_offset + strtab->sh_size;

    while ((char*)symbol < limit) {
        char * str = (char*)(theelf + strtab->sh_offset + symbol->st_name);
        if (strncmp(str, symname, strlimit - str) == 0) {
            return (void*)symbol->st_value;
        }
        symbol++;
    }

    return NULL;
}


static Elf*
parse_aerokernel_elf (void)
{
    Elf * e;
    Elf_Kind ek;
    char * k;


    theelf = (char*)&__aerokernel_start;
    elf_size = &__aerokernel_end - &__aerokernel_start;

    if (elf_version(EV_CURRENT) == EV_NONE) {
        fprintf(stderr, "ELF library initialization failed (%s)\n", elf_errmsg(-1));
        exit(1);
    }

    if ((e = elf_memory(theelf, elf_size)) == NULL) {
        fprintf(stderr, "elf_memory() failed (%s)\n", elf_errmsg(elf_errno()));
        exit(1);
    }

    ek = elf_kind(e);

    if (ek != ELF_K_ELF) {
        fprintf(stderr, "Aerokernel must be in ELF format.\n");
        exit(1);
    }

    return e;
}


static void
ros_sig_handler (uint64_t code)
{
    unsigned long long rc, num, a1=0, a2=0, a3=0, a4=0, a5=0, a6=0, a7=0, a8=0;
    struct v3_ros_event event;
    DEBUG("Caught an HVM signal from the VMM (in tid=%x), event is at %p\n", syscall(SYS_gettid), &event);

    memset(&event, 0xff, sizeof(event));

    /* get the status */
    rc = 1;
    num = 0xf00d;
    a1 = 0xf;
    a2 = (unsigned long long) &event;
    HCALL(rc,num,a1,a2,a3,a4,a5,a6,a7,a8);

    if (rc < 0) {
        fprintf(stderr, "Got bad return code from status hypercall (0x%llx)\n", rc);
        return;
    }

    switch (code) {
        case 1:
            DEBUG("ROS handler caught a page fault\n");
            break;
        case 2:
            DEBUG("ROS handler caught a syscall\n");
            break;
        case 3:
            DEBUG("ROS handler caught an exception\n");
            break;
        case 4:
            DEBUG("ROS handler caught a thread exit\n");
            notify_ros_thread(event.thread_exit.nktid);
            return;
        default:
            DEBUG("ROS handler caught an unknown signal (%u)\n", code);
            return;
    }

    v3_hvm_handle_ros_event(&event);
}


static void
sigsegv_handler (int sig, siginfo_t * info, void * u)
{
    unsigned long rc,num,a1,a2,a3,a4,a5,a6,a7,a8;
    ucontext_t * context = (ucontext_t*)u;
    mcontext_t m = context->uc_mcontext;

    fprintf(stderr, "ROS-side caused a page fault in tid=0x%llx (errno=0x%x, addr=%p, rip=0x%llx) Exiting.\n", 
            syscall(SYS_gettid), 
            info->si_errno,
            info->si_addr,
            m.gregs[REG_RIP]);

    rc = 0;
    num = 0xf00d;
    a1 = 0x1f;
    a2 = -1;
    HCALL(rc,num,a1,a2,a3,a4,a5,a6,a7,a8);
    sleep(3);
    exit(EXIT_FAILURE);
}


int 
hrt_runtime_init (int argc, char * argv[])
{
    DEBUG("HRT Runtime Initializing.\n");
    struct sigaction s;
    s.sa_flags     = SA_SIGINFO;
    s.sa_sigaction = sigsegv_handler;

    if (atexit(hrt_runtime_deinit) != 0) {
        fprintf(stderr, "Could not register HRT exit hook\n");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGSEGV, &s, NULL) != 0) {
        fprintf(stderr, "Could not register HRT SIGSEGV handler\n");
        exit(EXIT_FAILURE);
    }

    DEBUG("Parsing AeroKernel binary blob in my address space...");

    sig_stack = malloc(SIG_STACK_SIZE);
    if (!sig_stack) {
        fprintf(stderr, "Could not allocate signal stack\n");
        exit(EXIT_FAILURE);
    }

    v3_hvm_ros_unregister_signal();

    if (!(thread_hash = nk_create_htable(0, thread_hash_fn, thread_eq_fn))) {
        fprintf(stderr, "Could not create thread hashtable\n");
        exit(EXIT_FAILURE);
    }

    if (!(partner_thread_hash = nk_create_htable(0, thread_hash_fn, thread_eq_fn))) {
        fprintf(stderr, "Could not create partner thread hashtable\n");
        exit(EXIT_FAILURE);
    }

    if (v3_hvm_ros_register_signal(ros_sig_handler, sig_stack, 8192) != 0) {
        fprintf(stderr, "Could not register ROS signal\n");
        exit(EXIT_FAILURE);
    }

    elf_handle = parse_aerokernel_elf();

    DEBUG("[OK]\n");

    if (!elf_handle) {
        fprintf(stderr, "Could not parse AeroKernel ELF\n");
        exit(EXIT_FAILURE);
    }

    DEBUG("Installing AeroKernel in HVM addr=%p size=0x%llx\n",theelf,elf_size);

    if (v3_hvm_ros_install_hrt_image(theelf,elf_size)) { 
        fprintf(stderr, "Could not install AeroKernel\n");
        exit(EXIT_FAILURE);
    }

    DEBUG("Booting Aerokernel in HVM\n");

    if (v3_hvm_ros_reset(RESET_HRT)) { 
        fprintf(stderr, "Could not boot AeroKernel\n");
        exit(EXIT_FAILURE);
    }

    sleep(5); // TODO: no protocol for telling when it is done booting yet... 

    DEBUG("Merging address spaces with Aerokernel\n");

    if (v3_hvm_ros_merge_address_spaces()) { 
        fprintf(stderr, "Could not merge address spaces\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}


void
hrt_runtime_deinit (void)
{
    printf("Multiverse runtime shutting down.\n");

    if (v3_hvm_ros_unmerge_address_spaces()) { 
        fprintf(stderr, "Cannot unmerge address spaces\n");
        exit(EXIT_FAILURE);
    }

    /* HACK: we wait for the HRT side to finish up before we blow away any ROS-side
     * page mappings that it might be relying on */
    sleep(2);
}
