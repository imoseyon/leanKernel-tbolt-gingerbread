obj-m += nsio.o

# Provide path to your kernel source here
KERNEL_SOURCE = ../

# Edit as needed for your cross-compiler
CROSS_COMPILE   = /usr/bin/arm-linux-gnueabi-
ARCH            = arm

all:
	make -C $(KERNEL_SOURCE) M=$(PWD) modules
clean:
	make -C $(KERNEL_SOURCE) M=$(PWD) clean
