ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
obj-m  := polling_d.o

else
# normal makefile
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD)

endif