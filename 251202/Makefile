obj-m += gpio_irq.o

KDIR := ../../../linux
PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules
dtb:
	dtc -I dts -O dtb -o touch_gpio.dtbo touch_gpio.dts
clean:
	make -C $(KDIR) M=$(PWD) clean
