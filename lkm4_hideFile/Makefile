obj-m	:= lkm4_hideFile.o
KDIR	:= /lib/modules/$(shell uname -r)/build
PWD	:= $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
	rm -rf *.o modules.* Module.* *.mod.c *.o.*
