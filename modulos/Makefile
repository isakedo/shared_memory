# export the environment variables CC and KDIR properly set...

obj-m += shared_memory_mod.o

KDIR := /home/empotrados/workspace/emp_petalinux/Avnet-Digilent-ZedBoard-2016.3/build/linux/kernel/xlnx-4.6

PWD := $(shell pwd)

default:
	make ARCH=arm CROSS_COMPILE=arm-xilinx-linux-gnueabi- -C ${KDIR} M=${PWD} modules
	rm *.o modules.order Module.symvers shared_memory_mod.mod.c
