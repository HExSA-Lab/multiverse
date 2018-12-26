MYCC:=gcc
MYCFLAGS:= -Wall -O2

GUEST_KERN_DIR:=/home/kyle/cs562/p3-device/linux
INITRAMFS_DIR:=busybox-1.29.3/initramfs/bb-x86

OBJ_LIST := hvm-user hvm-driver.ko

THEDIR=$(PWD)

all: $(OBJ_LIST)

hvm-user: hvm-user.c
	$(MYCC) $(MYCFLAGS) -static -o $@ $<

obj-m += hvm-driver.o

hvm-driver.ko: hvm-driver.c
	$(MAKE) -C $(GUEST_KERN_DIR) M=$(PWD) modules

initramfs: 
	@scripts/gen_initramfs.sh $(INITRAMFS_DIR) $@.cpio.gz

hdd: $(OBJ_LIST)
	@dd if=/dev/zero of=$@.img count=10000 bs=1024
	@mkfs.ext3 $@.img
	@mkdir _test
	@sudo mount -o loop $@.img _test
	@sudo cp $(OBJ_LIST) _test
	@sudo chown -R $(shell whoami) _test
	@sync
	@sudo umount _test
	@sudo rm -rf _test

run:
	@scripts/runit.sh


clean:
	@rm -f hvm-user *.cpio.gz
	$(MAKE) -C $(GUEST_KERN_DIR) M=$(PWD) clean

.PHONY: clean
