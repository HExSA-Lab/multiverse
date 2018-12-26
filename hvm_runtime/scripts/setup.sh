#!/bin/sh

echo "Setting up BusyBox Environment"


mkdir qemu
pushd qemu
git clone git@github.com:HExSA-Lab/qemu.git
pushd qemu
./configure --target-list=x86_64-softmmu --enable-debug
make -j `nproc`
popd
popd

# get linux kernel

