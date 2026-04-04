#include "string.h"

void *memcpy(void *dest, const void *src, unsigned int n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    for (unsigned int i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

void *memset(void *s, int c, unsigned int n) {
    unsigned char *p = s;
    for (unsigned int i = 0; i < n; i++) p[i] = (unsigned char)c;
    return s;
}

int strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *a - *b;
}

// Only one strncmp (with unsigned int)
int strncmp(const char *s1, const char *s2, unsigned int n) {
    while (n--) {
        if (*s1 != *s2) return *s1 - *s2;
        if (*s1 == 0) break;
        s1++;
        s2++;
    }
    return 0;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

char *strncpy(char *dest, const char *src, unsigned int n) {
    unsigned int i;
    for (i = 0; i < n && src[i]; i++) dest[i] = src[i];
    for (; i < n; i++) dest[i] = 0;
    return dest;
}

unsigned int strlen(const char *s) {
    unsigned int len = 0;
    while (s[len]) len++;
    return len;
}

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) return (char*)s;
        s++;
    }
    return 0;
}

char *strtok(char *str, const char *delim) {
    static char *last;
    if (str) last = str;
    if (!last || !*last) return 0;
    // Skip leading delimiters
    while (*last && strchr(delim, *last)) last++;
    if (!*last) return 0;
    char *start = last;
    while (*last && !strchr(delim, *last)) last++;
    if (*last) {
        *last = 0;
        last++;
    }
    return start;
}

char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) d++;
    while ((*d++ = *src++));
    return dest;
}
