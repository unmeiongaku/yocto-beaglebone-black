obj-m += st7735r_core.o

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)


all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules


modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules_install


clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) clean

print:
	@echo "KERNEL_SRC=$(KERNEL_SRC)"
	@echo "PWD=$(PWD)"