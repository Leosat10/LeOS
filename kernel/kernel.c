#include "kernel.h"
#include "idt.h"
#include "fs.h"
#define SYS_PRINT   1
#define SYS_COMPILE 2
#define SYS_OPEN    3
#define SYS_READ    4
#define SYS_WRITE   5
#define SYS_CLOSE   6
#define SYS_CREATE  7
#define SYS_DELETE  8
#define SYS_MKDIR   9
#define SYS_READDIR 10


void syscall_handler()
{
    unsigned int syscall_num;
    char* msg;
    int fd, ret;
    char *path, *buf;
    uint32_t count;
    int ino;
    const char *name;

    __asm__ volatile("mov %%eax, %0" : "=r"(syscall_num));

    switch(syscall_num) {
        case SYS_PRINT:
            __asm__ volatile("mov %%ebx, %0" : "=r"(msg));
            print(msg);
            break;

        case SYS_COMPILE:
            // For now, just print a message (you'll implement later)
            print("Compile syscall not implemented yet\n");
            break;

        case SYS_OPEN:
            __asm__ volatile("mov %%ebx, %0" : "=r"(path));
            ret = fs_open(path);
            __asm__ volatile("mov %0, %%eax" : : "r"(ret));
            break;

        case SYS_READ:
            __asm__ volatile("mov %%ebx, %0" : "=r"(fd));
            __asm__ volatile("mov %%ecx, %0" : "=r"(buf));
            __asm__ volatile("mov %%edx, %0" : "=r"(count));
            ret = fs_read(fd, buf, count);
            __asm__ volatile("mov %0, %%eax" : : "r"(ret));
            break;

        case SYS_WRITE:
            __asm__ volatile("mov %%ebx, %0" : "=r"(fd));
            __asm__ volatile("mov %%ecx, %0" : "=r"(buf));
            __asm__ volatile("mov %%edx, %0" : "=r"(count));
            ret = fs_write(fd, buf, count);
            __asm__ volatile("mov %0, %%eax" : : "r"(ret));
            break;

        case SYS_CLOSE:
            __asm__ volatile("mov %%ebx, %0" : "=r"(fd));
            ret = fs_close(fd);
            __asm__ volatile("mov %0, %%eax" : : "r"(ret));
            break;

        case SYS_CREATE:
            __asm__ volatile("mov %%ebx, %0" : "=r"(path));
            __asm__ volatile("mov %%ecx, %0" : "=r"(count)); // mode
            ret = fs_create(path, count);
            __asm__ volatile("mov %0, %%eax" : : "r"(ret));
            break;

        case SYS_DELETE:
            __asm__ volatile("mov %%ebx, %0" : "=r"(path));
            ret = fs_delete(path);
            __asm__ volatile("mov %0, %%eax" : : "r"(ret));
            break;

        case SYS_MKDIR:
            __asm__ volatile("mov %%ebx, %0" : "=r"(path));
            ret = fs_mkdir(path);
            __asm__ volatile("mov %0, %%eax" : : "r"(ret));
            break;

        case SYS_READDIR:
            __asm__ volatile("mov %%ebx, %0" : "=r"(path));
            break;

        default:
            print("Unknown syscall\n");
            break;
    }
}

void panic(const char *msg) {
    print("PANIC: ");
    print(msg);
    print("\n");
    __asm__ volatile("cli; hlt");
    while(1);
}

void kernel_main()
{
    clear();

    idt_init();       

    set_idt_gate(0x80, (uint32_t)syscall_handler);
	 fs_init();
    print("MiniOS v1.0\n");
    print("Type help\n\n");
    fs_init();
    shell();
}


