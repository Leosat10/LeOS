#include <stdint.h>
#include "idt.h"

struct idt_entry {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct idt_entry idt[256];
struct idt_ptr idtp;

extern void idt_load();

void set_idt_gate(int n, uint32_t base)
{
    idt[n].base_low = base & 0xFFFF;
    idt[n].sel = 0x08;
    idt[n].always0 = 0;
    idt[n].flags = 0x8E;
    idt[n].base_high = (base >> 16) & 0xFFFF;
}

void idt_init()
{
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint32_t)&idt;

    idt_load();
}
