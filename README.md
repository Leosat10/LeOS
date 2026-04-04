# **LeOS – A Hobby Operating System**

LeOS is a simple, educational 32-bit x86 operating system written in C and assembly. It boots using GRUB (Multiboot compliant), provides a basic shell, and supports a custom read-only filesystem (LEOSFS).

--------------------------------------------------

FEATURES

- Multiboot compliant – Boots using GRUB
- Basic shell – Commands: help, ls, cat, reboot
- Custom filesystem (LEOSFS) – Read-only FS with superblock, inodes, and directories
- VGA text mode – 80×25 display with scrolling and color
- Keyboard driver – Handles scancodes (US QWERTY)
- Interrupt handling – IDT, PIC remapping, keyboard IRQ
- Disk I/O – ATA PIO (primary master)
- Memory management – Simple malloc() / free()
- Stack protection – __stack_chk_fail enabled

--------------------------------------------------

BUILDING

Prerequisites:

- i686-elf GCC (or GCC with -m32)
- as, ld, make
- grub-mkrescue, xorriso
- QEMU (recommended)
- Python 3

Build:

git clone https://github.com/Leosat10/LeOS.git
cd LeOS
make all

Outputs:

- kernel.bin
- fs.img
- os.iso

--------------------------------------------------

RUNNING

QEMU:

make run

Or:

qemu-system-i386 -cdrom os.iso -drive file=fs.img,format=raw,index=0,media=disk

Real Hardware:

Write os.iso to USB/CD and boot.

--------------------------------------------------

PROJECT STRUCTURE

.
├── app.c
├── boot
│   └── grub
│       └── grub.cfg
├── build
├── build.sh
├── fs.img
├── iso
│   └── boot
│       ├── grub
│       │   └── grub.cfg
│       └── kernel.bin
├── kernel
│   ├── boot.s
│   ├── disk.c
│   ├── disk.h
│   ├── fs.c
│   ├── fs.h
│   ├── idt.c
│   ├── idt.h
│   ├── idt_load.s
│   ├── io.c
│   ├── io.h
│   ├── kernel.c
│   ├── kernel.h
│   ├── keyboard.c
│   ├── leosfs.c
│   ├── leosfs.h
│   ├── libtcc.a
│   ├── libtcc.h
│   ├── malloc.c
│   ├── shell.c
│   ├── stack_chk.c
│   ├── string.c
│   ├── string.h
│   ├── tcc.c
│   └── vga.c
├── linker.ld
├── Makefile
├── make_fs.py
├── os.iso

--------------------------------------------------

FILESYSTEM

fs.img is generated via make_fs.py, which packs files into LEOSFS format.

--------------------------------------------------

LIMITATIONS

- Read-only filesystem
- No user-space programs
- Minimal shell

--------------------------------------------------

FUTURE PLANS

- Integrate Tiny C Compiler (TCC)
- Add ELF loader
- Implement scheduler
- Add write support to filesystem

--------------------------------------------------

LICENSE

MIT License

--------------------------------------------------

ACKNOWLEDGEMENTS

- GRUB
- OSDev Wiki
- MikeOS
- ToAruOS

--------------------------------------------------

LeOS – building an OS from scratch, one interrupt at a time.
