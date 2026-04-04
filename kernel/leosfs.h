#pragma once

#define MAX_FILES 16
#define FILENAME_LEN 12

typedef struct __attribute__((packed)) {
    char name[12];
    int start;
    int size;
} file_t;

void fs_init();
void fs_list();
int fs_read(const char* name, char* buffer);

