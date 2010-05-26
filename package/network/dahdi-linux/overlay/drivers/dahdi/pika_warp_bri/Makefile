# WARNING: 2.6 kernels only!

# SAM hardcode for now until we decide where it will be
EXTRA_CFLAGS += -I$(KBUILD_EXTMOD)/../dahdi/linux/include

# This is for developers
EXTRA_CFLAGS += $(PIKA_KERNEL_CFLAGS)

# The brie driver will *only* build against a PPC kernel
obj-m += pika-brie.o

ifeq ($(KERNELRELEASE),)

all:
	$(MAKE) -C $(KERNELSRCDIR) KBUILD_EXTMOD=$(shell pwd)

clean:
	rm -f *.o *.ko *.mod.c .*.cmd TAGS
	rm -f  Module.symvers modules.order
	rm -rf .tmp_versions

endif
