#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/refcount.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/cdev.h>

#include "driver-api.h"

#define DEBUG_ENABLE 1
#define VERSION "0.1"
#define CHRDEV_NAME "hvm"

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Kyle C. Hale and Conghao Liu");
MODULE_DESCRIPTION("HVM delegator module");
MODULE_VERSION(VERSION);

#define ERROR(fmt, args...) printk(KERN_ERR "HVM-DEL: " fmt, ##args)
#define INFO(fmt, args...) printk(KERN_INFO "HVM-DEL: " fmt, ##args)

#if DEBUG_ENABLE==1
#define DEBUG(fmt, args...) printk(KERN_DEBUG "HVM-DEL: " fmt, ##args)
#else 
#define DEBUG(fmt, args...)
#endif


struct hvm_del_info {
    struct mutex lock;
    dev_t chrdev;
    refcount_t refcnt;
};

static struct hvm_del_info * glob_info = NULL;

static ssize_t
hvm_del_read (struct file * file,
        char __user * buffer,
        size_t length,
        loff_t * offset)
{
    DEBUG("Read\n");
    return -EINVAL;
}


static ssize_t
hvm_del_write (struct file * file,
        const char __user * buffer,
        size_t len,
        loff_t * offset)
{
    DEBUG("Write\n");
    return -EINVAL;
}


static int
hvm_del_open (struct inode * inode,
        struct file * file)
{
    DEBUG("Delegator OPEN\n");
    refcount_inc(&glob_info->refcnt);
    return 0;
}


static int
hvm_del_release (struct inode * inode,
        struct file * file)
{
    DEBUG("Delegator RELEASE\n");
    refcount_dec(&glob_info->refcnt);
    return 0;
}


static long
hvm_del_ioctl (struct file * file,
               unsigned int ioctl,
               unsigned long arg)
{
    void __user * argp = (void __user*)arg;
    struct hvm_del_req * req = NULL;

    DEBUG("IOCTL %d\n", ioctl);

    switch (ioctl) {
        case HVM_DEL_IOCTL_PERFORM_OUTB: 
            req = kzalloc(sizeof(*req), GFP_KERNEL);
            if (copy_from_user(req, argp, sizeof(*req)) != 0) {
                ERROR("Could not copy request from user\n");
                return -EFAULT;
            }
            INFO("Performing outb ioctl (port=%x, val=%x)\n",
                    req->port, req->val);
            outb(req->val, req->port);
            kfree(req);
            break;
        default:
            ERROR("Unknown hvm del request: %d\n", ioctl);
            return -EINVAL;
    }

    return 0;
}


static struct file_operations fops = {
    .read           = hvm_del_read,
    .write          = hvm_del_write,
    .unlocked_ioctl = hvm_del_ioctl,
    .open           = hvm_del_open,
    .release        = hvm_del_release,
};


static int __init
hvm_del_init (void)
{
    INFO("HVM Delegator module initializing\n");

    glob_info = kzalloc(sizeof(*glob_info), GFP_KERNEL);
    if (!glob_info) {
        ERROR("Could not allocate global info\n");
        return -1;
    }

    mutex_init(&glob_info->lock);

    refcount_set(&glob_info->refcnt, 1);

    glob_info->chrdev = register_chrdev(0, CHRDEV_NAME, &fops);

    INFO("<maj,min> = <%u,%u>\n", MAJOR(glob_info->chrdev), MINOR(glob_info->chrdev));

    return 0;
}


static void __exit
hvm_del_exit (void)
{
    INFO("HVM Delegator module cleaning up\n");
    unregister_chrdev(MAJOR(glob_info->chrdev), CHRDEV_NAME);
    mutex_destroy(&glob_info->lock);
    kfree(glob_info);
}

module_init(hvm_del_init);
module_exit(hvm_del_exit);
