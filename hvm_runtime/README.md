# HVM runtime system

## Getting set up

First install prerequisites for building QEMU and Linux kernel (assuming CentOS/Fedora package names):

```Shell
[you@you] sudo dnf install -y glib2-devel zlib-devel pixman-devel bison flex elfutils-libelf-devel openssl-devel glibc-static
```

Get the environment ready. This grabs, builds, and prepares recent copies of
BusyBox (for the guest userspace), the Linux kernel (for the guest kernel), and QEMU (the VMM)

```Shell
[you@host] make setup
```

Now build the initramfs, the guest linux user-space HVM utility, the
guest Linux kernel driver, and the guest virtual disk image by running


```Shell
[you@host] make 
```

The disk image will be mounted in the guest at `/mnt`. You can
build the initramfs explicitly by running `make initramfs` and you can build 
the disk image explicitly by running `make hdd`. `make` by itself will
invoke these automatically if they haven't been build yet.


You can run using QEMU:

```Shell
[you@host] make run
```

You can run the guest user-space utility in the guest as follows:

```Shell
[you@guest> /mnt/hvm-user
```
