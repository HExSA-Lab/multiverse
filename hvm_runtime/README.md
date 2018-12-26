# HVM runtime system

## Getting set up

Get the environment ready. This grabs, builds, and prepares recent copies of
BusyBox (for the guest userspace), the Linux kernel (for the guest kernel), and QEMU (the VMM)

```Shell
[you@you] make setup
```

Now build an initramfs to pass to QEMU:

```Shell
[you@you] make initramfs
```

Build the user-space utility and the guest HVM kernel driver:

```Shell
[you@you] make 
```

Build the virtual disk image that will contain the driver/utility in 
the guest (this image will be mounted in `/mnt` in the guest):

```Shell
[you@you] make hdd
```

Run using QEMU:

```Shell
[you@you] make run
```
