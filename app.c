void _start()
{
    const char* msg = "Hello from syscall!\n";
    __asm__ volatile(
        "mov %0, %%ebx\n"
        "int $0x80"
        : : "r"(msg) : "ebx"
    );
}
