DRIVER = embeddedLamp

obj-m := $(DRIVER).o
PWD := $(shell pwd)

all:
	gcc wlan_info.c -o wlan_info -L. -liw -lm
Makefile (END)

