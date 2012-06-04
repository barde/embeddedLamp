DRIVER = embeddedLamp

obj-m := $(DRIVER).o
PWD := $(shell pwd)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	insmod embeddedLamp.ko
	echo start >/dev/embeddedLamp

clean:
	rmmod embeddedLamp
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
