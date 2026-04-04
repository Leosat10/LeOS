#ifndef STRING_H
#define STRING_H

void *memcpy(void *dest, const void *src, unsigned int n);
void *memset(void *s, int c, unsigned int n);
int strcmp(const char *a, const char *b);
int strncmp(const char *s1, const char *s2, unsigned int n);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, unsigned int n);
unsigned int strlen(const char *s);
char *strchr(const char *s, int c);
char *strtok(char *str, const char *delim);
char *strcat(char *dest, const char *src);
#endif
