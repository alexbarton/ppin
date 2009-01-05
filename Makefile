ifeq ($(KERNELRELEASE),)

# normal Makefile

KERNELDIR := /lib/modules/`uname -r`/build

all:
	$(MAKE) -C $(KERNELDIR) M=`pwd` CONFIG_PPIN=m modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=`pwd` modules_install

clean:
	$(MAKE) -C $(KERNELDIR) M=`pwd` clean
	rm -f modules.order

else

# kbuild part of makefile

obj-$(CONFIG_PPIN) := ppin.o

endif
