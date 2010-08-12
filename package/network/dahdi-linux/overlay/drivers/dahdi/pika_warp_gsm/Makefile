ifeq ($(KERNELRELEASE),)

.PHONY: all clean

ifeq ($(PIKA_KERNEL),)
KERNELRELEASE = $(shell uname -r)
else
KERNELRELEASE = $(PIKA_KERNEL)
endif
ifeq ($(KERNELSRCDIR),)
KERNELSRCDIR := /lib/modules/$(KERNELRELEASE)/build
endif
BUILD_DIR := $(shell pwd)

all:
	$(MAKE) -C $(KERNELSRCDIR) SUBDIRS=$(shell pwd)	KBUILD_EXTRA_SYMBOLS=$(DAHDI_SYM_DIR)/Module.symvers

clean:
	rm -f *.o *.ko .*.cmd pika-gsm.mod.c
	rm -f Modules.symvers Module.symvers modules.order
	rm -f Module.markers
	rm -rf .tmp_versions

else

# For pikataco.h
EXTRA_CFLAGS=-I$(PIKA_DIR)/kernel/common

obj-m += pika-gsm.o

endif
