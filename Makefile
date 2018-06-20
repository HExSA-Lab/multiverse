CC:=gcc
HVM_ROS_DIR=.
AEROKERNEL="\"nautilus.bin.TEST\""
BASE=/v-test/nautilus/hrt_redirect/libelf-0.8.9/build
LIBPATH=$(BASE)/lib
LDFLAGS:= -static -L$(LIBPATH) -L$(HVM_ROS_DIR) -pthread -Wl,--wrap,main -Wl,-rpath,$(LIBPATH)
LIBS:=-lelf -lrt -lv3_hvm_ros_user
INC:=-I$(BASE)/include

PDIR:=/home/pdinda/palacios-hvm/guest/linux/hvm-ros

DEBUG:=0

include wrap.mk

CFLAGS:= $(INC) -DAEROKERNEL_PATH=$(AEROKERNEL)  -I$(HVM_ROS_DIR) 

ifeq ($(DEBUG),1)
	CFLAGS += -DDEBUG_ENABLE=1
endif

all: test_redirect test_threads


hrtrt.o: hrtrt.c
	$(CC) $(CFLAGS) -o $@ -c $<

aerokernel.o: aerokernel.S
	$(CC) $(CFLAGS) -o $@ -c $<

wrap.o: wrap.c
	$(CC) $(CFLAGS) -o $@ -c $<

pthread.o: pthread.c
	$(CC) $(CFLAGS) -o $@ -c $<

hashtable.o: hashtable.c
	$(CC) $(CFLAGS) -o $@ -c $<

wrap_funcs.c: 
	./gen_wrappers.pl test.yaml

wrap_funcs.o: wrap_funcs.c
	$(CC) $(CFLAGS) -o $@ -c $<

test_redirect.o: test_redirect.c
	$(CC) $(CFLAGS) -o $@ -c $<

test_threads.o: test_threads.c
	$(CC) $(CFLAGS) -o $@ -c $<

libv3_hvm_ros_user.a: $(PDIR)/v3_hvm_ros_user.c  $(PDIR)/v3_hvm_ros_user_low_level.S $(PDIR)/v3_hvm_ros_user.h
	make -C $(PDIR) clean
	make -C $(PDIR) DEBUG=$(DEBUG) 
	cp $(PDIR)/$@ .

test_redirect: test_redirect.o hrtrt.o aerokernel.o wrap.o wrap_funcs.o pthread.o hashtable.o libv3_hvm_ros_user.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

test_threads: test_threads.o hrtrt.o aerokernel.o wrap.o wrap_funcs.o pthread.o hashtable.o libv3_hvm_ros_user.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)


clean:
	rm -f hrtrt.o aerokernel.o pthread.o wrap.o wrap_funcs.o hashtable.o $(TARGET).o $(TARGET) wrap_funcs.c libv3_hvm_ros_user.a
