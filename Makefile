OBJS = kernel/kernel.o kernel/shell.o kernel/string.o

kernel/string.o: kernel/string.c
	i386-elf-gcc -ffreestanding -c kernel/string.c -o kernel/string.o
