obj-m := first_module.o
obj-m := scull_module.o

KDIR := /linux
PWD := $(shell pwd)

all:
        make -C $(KDIR) M=$(PWD) modules

clean:
        make -C $(KDIR) M=$(PWD) clean