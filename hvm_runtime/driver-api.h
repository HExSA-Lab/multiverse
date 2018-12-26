#ifndef __DRIVER_API__
#define __DRIVER_API__

#define HVM_DEL_IOCTL_PERFORM_OUTB 0x20

struct hvm_del_req {
    unsigned short port;
    unsigned char val;
};

#if 0
static inline uint8_t
inb (uint16_t port)
{
    uint8_t ret;
    asm volatile ("inb %1, %0":"=a" (ret):"dN" (port));
    return ret;
}

static inline void 
outb (uint8_t val, uint16_t port)
{
    asm volatile ("outb %0, %1"::"a" (val), "dN" (port));
}
#endif

#endif
