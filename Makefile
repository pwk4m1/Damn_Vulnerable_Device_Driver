
obj-m += DVDD.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

load:
	insmod DVDD.ko
	chmod 0666 /dev/DVDD
unload:
	rmmod DVDD.ko

