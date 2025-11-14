obj-m += cherry_msg_driver.o

KDIR := ../../../linux
PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
