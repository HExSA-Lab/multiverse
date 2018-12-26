#!/bin/sh

echo "Setting up BusyBox Environment"

mkdir bb
pushd bb
git clone git://git.busybox.net/busybox.git
pushd busybox
mkdir -pv obj/bb-x86
echo "Building BusyBox"
make O=obj/bb-x86 defconfig
cp ../../configs/bb-config obj/bb-x86/.config
cp ../../configs/bb-config .config
pushd obj/bb-x86
make -j `nproc` 
make install
popd
echo "Creating initramfs template"
mkdir -p initramfs/bb-x86
pushd initramfs/bb-x86
mkdir -pv {bin,sbin,etc,proc,sys,usr/{bin,sbin}}
cp -av ../../obj/bb-x86/_install/* .
popd
popd
popd
echo "Copying init"
cp configs/init bb/busybox/initramfs/bb-x86/

mkdir qemu
pushd qemu
git clone https://github.com/HExSA-Lab/qemu.git
pushd qemu
echo "Building QEMU"
./configure --target-list=x86_64-softmmu --enable-debug
make -j `nproc`
popd
popd

# get linux kernel
mkdir guest-kernel
pushd guest-kernel
echo "Preparing guest kernel"
git clone https://github.com/torvalds/linux.git
pushd linux
make defconfig
make modules_prepare -j `nproc`
make bzImage -j `nproc`
popd
popd

