#ifndef KERNEL_H
#define KERNEL_H
int get_cursor();
void print(const char* str);
void panic(const char *msg);
void clear();
void backspace();
void set_cursor(int pos);

void *malloc(unsigned int size);
void free(void *ptr);

void *realloc(void *ptr, unsigned int size);
void *tcc_compile(const char *source);
char keyboard_read();

void shell();

#endif
