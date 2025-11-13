obj-m += first_drivers.o

KDIR :=../../../linux
PWD :=$(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
