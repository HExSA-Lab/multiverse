#!/bin/sh

qemu/qemu/x86_64-softmmu/qemu-system-x86_64 \
    -kernel $PWD/guest-kernel/linux/arch/x86/boot/bzImage \
    -initrd $PWD/initramfs.cpio.gz \
    -hda hdd.img \
    -m 2G \
    -machine accel=kvm \
    -serial stdio \
    -append "console=ttyS0 root=/dev/sda"
