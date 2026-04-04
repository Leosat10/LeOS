#ifndef IDT_H
#define IDT_H

#include <stdint.h>

void idt_init(void);
void set_idt_gate(int n, uint32_t base);

#endif
