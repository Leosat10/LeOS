#ifndef FS_H
#define FS_H
#include "string.h"
#include <stdint.h>

#define BLOCK_SIZE 512
#define INODE_COUNT 128
#define MAX_FILES 128
#define MAGIC 0x4C454F53  // "LEOS"

struct superblock {
    uint32_t magic;
    uint32_t total_blocks;
    uint32_t inode_count;
    uint32_t inode_table_start;
    uint32_t data_start;
    uint32_t root_inode;
    uint32_t free_blocks;
    uint32_t free_inodes;
};

struct inode {
    uint32_t mode;        // file type + permissions
    uint32_t size;
    uint32_t blocks[12];  // direct block pointers
    uint32_t indirect;    // indirect block pointer
    uint32_t create_time;
    uint32_t modify_time;
};

struct dir_entry {
    uint32_t inode;       // 0 = free entry
    char name[28];        // max filename length 27 chars + null
};

void fs_init(void);
int fs_format(void);
int fs_create(const char *path, uint32_t mode);
int fs_open(const char *path);
int fs_read(int fd, char *buf, uint32_t count);
int fs_write(int fd, const char *buf, uint32_t count);
int fs_close(int fd);
int fs_delete(const char *path);
int fs_mkdir(const char *path);
int fs_readdir(const char *path, void *buf, int (*callback)(const char *name, uint32_t inode));

void read_inode(int ino, struct inode *inode);
void write_inode(int ino, struct inode *inode);

// Path resolution helpers
int get_parent_inode(int ino);
char *get_path(int ino);
int resolve_path_relative(const char *path, int base_ino, int *parent_ino, int *child_ino, char *name);

int resolve_path(const char *path, int *parent_ino, int *child_ino, char *name);
void read_inode(int ino, struct inode *inode);

#endif
