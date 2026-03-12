build:
	arm-linux-gnueabi-as -o Listports.o src/Listports.S
	arm-linux-gnueabi-ld -o Listports Listports.o

run:
	qemu-arm ./Listports

clean:
	rm -f Listports.o Listports
