#!/bin/bash
set -e

# Clean old builds
rm -f *.o kernel.bin app.bin

# Compile kernel C files
gcc -m32 -ffreestanding -fno-pic -fno-pie -nostdlib -fno-stack-protector -c kernel/kernel.c -o kernel.o
gcc -m32 -ffreestanding -fno-pic -fno-pie -nostdlib -fno-stack-protector -c kernel/keyboard.c -o keyboard.o
gcc -m32 -ffreestanding -fno-pic -fno-pie -nostdlib -fno-stack-protector -c kernel/vga.c -o vga.o
gcc -m32 -ffreestanding -fno-pic -fno-pie -nostdlib -fno-stack-protector -c kernel/io.c -o io.o
gcc -m32 -ffreestanding -fno-pic -fno-pie -nostdlib -fno-stack-protector -c kernel/disk.c -o disk.o
gcc -m32 -ffreestanding -fno-pic -fno-pie -nostdlib -fno-stack-protector -c kernel/string.c -o string.o
gcc -m32 -ffreestanding -fno-pic -fno-pie -nostdlib -fno-stack-protector -c kernel/shell.c -o shell.o
gcc -m32 -ffreestanding -fno-pic -fno-pie -nostdlib -fno-stack-protector -c kernel/idt.c -o idt.o
gcc -v -m32 -ffreestanding -fno-pic -fno-pie -nostdlib -fno-stack-protector -c kernel/fs.c -o fs.o
gcc -m32 -ffreestanding -fno-pic -fno-pie -nostdlib -fno-stack-protector -c kernel/malloc.c -o malloc.o
gcc -m32 -ffreestanding -fno-pic -fno-pie -nostdlib -fno-stack-protector -c kernel/stack_chk.c -o stack_chk.o
gcc -m32 -ffreestanding -fno-pic -fno-pie -nostdlib -fno-stack-protector -c kernel/tcc.c -o tcc.o


# Assemble assembly files
as --32 -o boot.o kernel/boot.s
as --32 -o idt_load.o kernel/idt_load.s


ld -m elf_i386 -T linker.ld -o kernel.bin \
    kernel.o boot.o keyboard.o vga.o io.o disk.o string.o shell.o idt.o idt_load.o fs.o malloc.o tcc.o \
    -Lkernel -ltcc

# Compile and link app
gcc -m32 -ffreestanding -fno-pic -fno-pie -nostdlib -c app.c -o app.o
ld -m elf_i386 -Ttext 0x20000 -e _start --oformat binary app.o -o app.bin

# Build filesystem
python make_fs.py

# Run OS
qemu-system-i386 \
    -drive file=fs.img,format=raw,if=ide,index=0 \
    -kernel kernel.bin
