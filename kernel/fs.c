#include "fs.h"
#include "disk.h"
#include "kernel.h"
#include "string.h"

#define USE_SIMULATION 1

#ifdef USE_SIMULATION
static char disk_sim[BLOCK_SIZE * 20480];
#endif

struct superblock sb;
#define MAX_FD 32
struct file {
    int inode;
    int offset;
    int mode;
} fd_table[MAX_FD];

// ----------------------------------------------------------------------------
// Helper functions (forward declarations)
// ----------------------------------------------------------------------------
void read_block(uint32_t block, void *buffer);
void write_block(uint32_t block, void *buffer);
void read_inode(int ino, struct inode *inode);
void write_inode(int ino, struct inode *inode);
int alloc_block(void);
void free_block(uint32_t block);
int alloc_inode(void);
void free_inode(int ino);
int resolve_path(const char *path, int *parent_ino, int *child_ino, char *name);
int add_dir_entry(int dir_ino, const char *name, int ino);
int find_dir_entry(int dir_ino, const char *name, uint32_t *inode_out);

// ----------------------------------------------------------------------------
// Integer printer (debugging)
// ----------------------------------------------------------------------------
static void print_int(int n) {
    if (n < 0) {
        print("-");
        n = -n;
    }
    char buf[16];
    int i = 0;
    if (n == 0) {
        print("0");
        return;
    }
    while (n > 0) {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    for (int j = i - 1; j >= 0; j--) {
        char s[2] = {buf[j], 0};
        print(s);
    }
}

// ----------------------------------------------------------------------------
// Block I/O
// ----------------------------------------------------------------------------
void read_block(uint32_t block, void *buffer) {
#ifdef USE_SIMULATION
    memcpy(buffer, disk_sim + block * BLOCK_SIZE, BLOCK_SIZE);
#else
    ata_read_sector(block, (char*)buffer);
#endif
}

void write_block(uint32_t block, void *buffer) {
#ifdef USE_SIMULATION
    memcpy(disk_sim + block * BLOCK_SIZE, buffer, BLOCK_SIZE);
#else
    ata_write_sector(block, (const char*)buffer);
#endif
}

// ----------------------------------------------------------------------------
// Inode operations
// ----------------------------------------------------------------------------
void read_inode(int ino, struct inode *inode) {
    uint32_t block = sb.inode_table_start + (ino * sizeof(struct inode) / BLOCK_SIZE);
    uint32_t offset = (ino * sizeof(struct inode)) % BLOCK_SIZE;
    char buf[BLOCK_SIZE];
    read_block(block, buf);
    memcpy(inode, buf + offset, sizeof(struct inode));
}

void write_inode(int ino, struct inode *inode) {
    uint32_t block = sb.inode_table_start + (ino * sizeof(struct inode) / BLOCK_SIZE);
    uint32_t offset = (ino * sizeof(struct inode)) % BLOCK_SIZE;
    char buf[BLOCK_SIZE];
    read_block(block, buf);
    memcpy(buf + offset, inode, sizeof(struct inode));
    write_block(block, buf);
}

// ----------------------------------------------------------------------------
// Block allocation (bitmap)
// ----------------------------------------------------------------------------
#define BITMAP_BLOCKS ((sb.total_blocks + 7) / 8 / BLOCK_SIZE + 1)
static char *bitmap = 0;

void load_bitmap(void) {
    if (bitmap == 0) {
        static char bitmap_buffer[BLOCK_SIZE * 16];
        bitmap = bitmap_buffer;
    }
    for (int i = 0; i < BITMAP_BLOCKS; i++) {
        read_block(1 + i, bitmap + i * BLOCK_SIZE);
    }
}

void save_bitmap(void) {
    for (int i = 0; i < BITMAP_BLOCKS; i++) {
        write_block(1 + i, bitmap + i * BLOCK_SIZE);
    }
}

int alloc_block(void) {
    load_bitmap();
    for (uint32_t i = 0; i < sb.total_blocks; i++) {
        int byte = i / 8;
        int bit = i % 8;
        if ((bitmap[byte] & (1 << bit)) == 0) {
            bitmap[byte] |= (1 << bit);
            save_bitmap();
            sb.free_blocks--;
            write_block(0, &sb);
            return i;
        }
    }
    return -1;
}

void free_block(uint32_t block) {
    if (block >= sb.total_blocks) return;
    load_bitmap();
    int byte = block / 8;
    int bit = block % 8;
    bitmap[byte] &= ~(1 << bit);
    save_bitmap();
    sb.free_blocks++;
    write_block(0, &sb);
}

// ----------------------------------------------------------------------------
// Inode allocation
// ----------------------------------------------------------------------------
int alloc_inode(void) {
    for (int i = 0; i < sb.inode_count; i++) {
        struct inode inode;
        read_inode(i, &inode);
        if (inode.mode == 0) {
            memset(&inode, 0, sizeof(inode));
            inode.mode = 0x81FF;
            write_inode(i, &inode);
            sb.free_inodes--;
            write_block(0, &sb);
            return i;
        }
    }
    return -1;
}

void free_inode(int ino) {
    struct inode inode;
    read_inode(ino, &inode);
    if (inode.mode == 0) return;
    for (int i = 0; i < 12; i++) {
        if (inode.blocks[i] != 0) free_block(inode.blocks[i]);
    }
    if (inode.indirect != 0) free_block(inode.indirect);
    memset(&inode, 0, sizeof(inode));
    write_inode(ino, &inode);
    sb.free_inodes++;
    write_block(0, &sb);
}

// ----------------------------------------------------------------------------
// Directory operations
// ----------------------------------------------------------------------------
int add_dir_entry(int dir_ino, const char *name, int ino) {
    struct inode dir_inode;
    read_inode(dir_ino, &dir_inode);
    if ((dir_inode.mode & 0xF000) != 0x4000) {
        print("add_dir_entry: not a directory (ino "); print_int(dir_ino); print(")\n");
        return -1;
    }

    int entry_index = dir_inode.size / sizeof(struct dir_entry);
    int block_index = entry_index * sizeof(struct dir_entry) / BLOCK_SIZE;
    if (block_index >= 12) {
        print("add_dir_entry: too many entries\n");
        return -1;
    }
    int block = dir_inode.blocks[block_index];
    if (block == 0) {
        block = alloc_block();
        if (block < 0) return -1;
        dir_inode.blocks[block_index] = block;
        char zero[BLOCK_SIZE];
        memset(zero, 0, BLOCK_SIZE);
        write_block(block, zero);
    }

    char buf[BLOCK_SIZE];
    read_block(block, buf);
    int offset = (entry_index * sizeof(struct dir_entry)) % BLOCK_SIZE;
    struct dir_entry *entry = (struct dir_entry*)(buf + offset);
    entry->inode = ino;
    strncpy(entry->name, name, 27);
    entry->name[27] = 0;
    write_block(block, buf);

    dir_inode.size += sizeof(struct dir_entry);
    write_inode(dir_ino, &dir_inode);
    return 0;
}

int find_dir_entry(int dir_ino, const char *name, uint32_t *inode_out) {
    struct inode dir_inode;
    read_inode(dir_ino, &dir_inode);
    if ((dir_inode.mode & 0xF000) != 0x4000) return -1;

    int total_entries = dir_inode.size / sizeof(struct dir_entry);
    for (int i = 0; i < total_entries; i++) {
        int block_index = i * sizeof(struct dir_entry) / BLOCK_SIZE;
        int block = (block_index < 12) ? dir_inode.blocks[block_index] : 0;
        if (block == 0) continue;
        char buf[BLOCK_SIZE];
        read_block(block, buf);
        int offset = (i * sizeof(struct dir_entry)) % BLOCK_SIZE;
        struct dir_entry *entry = (struct dir_entry*)(buf + offset);
        if (entry->inode != 0 && strcmp(entry->name, name) == 0) {
            *inode_out = entry->inode;
            return 0;
        }
    }
    return -1;
}

// ----------------------------------------------------------------------------
// Path resolution (tokenizing version)
// ----------------------------------------------------------------------------

int resolve_path(const char *path, int *parent_ino, int *child_ino, char *name) {
    print("resolve_path: path='"); print(path); print("'\n");
    if (path[0] != '/') return -1;
    if (path[1] == 0) {
        if (parent_ino) *parent_ino = sb.root_inode;
        if (child_ino) *child_ino = sb.root_inode;
        if (name) name[0] = 0;
        return 0;
    }

    char buf[128];
    strncpy(buf, path, 127);
    buf[127] = 0;
    print("resolve_path: buffer='"); print(buf); print("'\n");
    char *token = strtok(buf, "/");
    char *last_token = 0;
    int cur_ino = sb.root_inode;
    int prev_ino = sb.root_inode;
    while (token) {
        print("resolve_path: token='"); print(token); print("'\n");
        prev_ino = cur_ino;
        last_token = token;
        uint32_t next_ino;
        if (find_dir_entry(cur_ino, token, &next_ino) != 0) {
            if (parent_ino) *parent_ino = prev_ino;
            if (child_ino) *child_ino = -1;
            if (name) strcpy(name, token);
            print("resolve_path: token not found\n");
            return -1;
        }
        cur_ino = next_ino;
        token = strtok(0, "/");
    }
    if (parent_ino) *parent_ino = prev_ino;
    if (child_ino) *child_ino = cur_ino;
    if (name && last_token) strcpy(name, last_token);
    print("resolve_path: success\n");
    return 0;
}

// ----------------------------------------------------------------------------
// File system initialization and formatting
// ----------------------------------------------------------------------------
void fs_init(void) {
    read_block(0, &sb);
    if (sb.magic != MAGIC) {
        print("No valid file system found, formatting...\n");
        fs_format();
        read_block(0, &sb);
    }
    for (int i = 0; i < MAX_FD; i++) {
        fd_table[i].inode = -1;
        fd_table[i].offset = 0;
        fd_table[i].mode = 0;
    }
}

int fs_format(void) {
    print("Format: starting...\n");
    uint32_t total_blocks = 20480;
    uint32_t inode_table_start = 2;
    uint32_t data_start = inode_table_start + (INODE_COUNT * sizeof(struct inode) + BLOCK_SIZE - 1) / BLOCK_SIZE;

    sb.magic = MAGIC;
    sb.total_blocks = total_blocks;
    sb.inode_count = INODE_COUNT;
    sb.inode_table_start = inode_table_start;
    sb.data_start = data_start;
    sb.root_inode = 0;
    sb.free_blocks = total_blocks - data_start;
    sb.free_inodes = INODE_COUNT - 1;

    print("Format: writing superblock...\n");
    write_block(0, &sb);

    print("Format: writing bitmap...\n");
    int bitmap_bytes = (total_blocks + 7) / 8;
    char *bitmap_buf = (char*)malloc(bitmap_bytes);
    if (!bitmap_buf) return -1;
    memset(bitmap_buf, 0, bitmap_bytes);
    for (uint32_t i = 0; i < data_start; i++) {
        int byte = i / 8;
        int bit = i % 8;
        bitmap_buf[byte] |= (1 << bit);
    }
    for (int i = 0; i < (bitmap_bytes + BLOCK_SIZE - 1) / BLOCK_SIZE; i++) {
        write_block(1 + i, bitmap_buf + i * BLOCK_SIZE);
    }
    free(bitmap_buf);

    print("Format: clearing inode table...\n");
    char zero_block[BLOCK_SIZE];
    memset(zero_block, 0, BLOCK_SIZE);
    int inode_blocks = (INODE_COUNT * sizeof(struct inode) + BLOCK_SIZE - 1) / BLOCK_SIZE;
    for (int i = 0; i < inode_blocks; i++) {
        write_block(inode_table_start + i, zero_block);
    }

    struct inode root_inode;
    memset(&root_inode, 0, sizeof(root_inode));
    root_inode.mode = 0x41FF;
    root_inode.size = 0;
    int root_block = alloc_block();
    if (root_block < 0) {
        print("Failed to allocate root block\n");
        return -1;
    }
    root_inode.blocks[0] = root_block;
    write_inode(0, &root_inode);
    char zero[BLOCK_SIZE];
    memset(zero, 0, BLOCK_SIZE);
    write_block(root_block, zero);

    add_dir_entry(0, ".", 0);
    add_dir_entry(0, "..", 0);

    for (int i = 0; i < MAX_FD; i++) {
        fd_table[i].inode = -1;
    }

    print("File system formatted successfully.\n");
    print("Format: done.\n");
    return 0;
}

// ----------------------------------------------------------------------------
// File operations
// ----------------------------------------------------------------------------
int fs_create(const char *path, uint32_t mode) {
    int parent, child;
    char name[28];
    if (resolve_path(path, &parent, &child, name) != 0) {
        if (child != -1) return -1;
        int ino = alloc_inode();
        if (ino < 0) return -1;
        if (add_dir_entry(parent, name, ino) != 0) {
            free_inode(ino);
            return -1;
        }
        return 0;
    }
    return -1;
}

int fs_open(const char *path) {
    int ino;
    if (resolve_path(path, 0, &ino, 0) != 0) return -1;
    for (int i = 0; i < MAX_FD; i++) {
        if (fd_table[i].inode == -1) {
            fd_table[i].inode = ino;
            fd_table[i].offset = 0;
            fd_table[i].mode = 1;
            return i;
        }
    }
    return -1;
}

int fs_read(int fd, char *buf, uint32_t count) {
    if (fd >= MAX_FD || fd_table[fd].inode == -1) return -1;
    struct inode inode;
    read_inode(fd_table[fd].inode, &inode);
    int offset = fd_table[fd].offset;
    int remaining = inode.size - offset;
    if (remaining <= 0) return 0;
    if (count > remaining) count = remaining;

    int bytes_read = 0;
    while (bytes_read < count) {
        int block_index = offset / BLOCK_SIZE;
        int block_num;
        if (block_index < 12) {
            block_num = inode.blocks[block_index];
        } else {
            break;
        }
        if (block_num == 0) break;
        int block_offset = offset % BLOCK_SIZE;
        int to_read = count - bytes_read;
        if (to_read > BLOCK_SIZE - block_offset) to_read = BLOCK_SIZE - block_offset;
        char block_buf[BLOCK_SIZE];
        read_block(block_num, block_buf);
        memcpy(buf + bytes_read, block_buf + block_offset, to_read);
        bytes_read += to_read;
        offset += to_read;
    }
    fd_table[fd].offset = offset;
    return bytes_read;
}

int fs_write(int fd, const char *buf, uint32_t count) {
    if (fd >= MAX_FD || fd_table[fd].inode == -1) return -1;
    struct inode inode;
    read_inode(fd_table[fd].inode, &inode);
    int offset = fd_table[fd].offset;
    int bytes_written = 0;
    while (bytes_written < count) {
        int block_index = offset / BLOCK_SIZE;
        int block_num;
        if (block_index < 12) {
            block_num = inode.blocks[block_index];
            if (block_num == 0) {
                block_num = alloc_block();
                if (block_num < 0) break;
                inode.blocks[block_index] = block_num;
            }
        } else {
            break;
        }
        int block_offset = offset % BLOCK_SIZE;
        int to_write = count - bytes_written;
        if (to_write > BLOCK_SIZE - block_offset) to_write = BLOCK_SIZE - block_offset;
        char block_buf[BLOCK_SIZE];
        read_block(block_num, block_buf);
        memcpy(block_buf + block_offset, buf + bytes_written, to_write);
        write_block(block_num, block_buf);
        bytes_written += to_write;
        offset += to_write;
    }
    if (offset > inode.size) {
        inode.size = offset;
        write_inode(fd_table[fd].inode, &inode);
    }
    fd_table[fd].offset = offset;
    return bytes_written;
}

int fs_close(int fd) {
    if (fd >= MAX_FD) return -1;
    fd_table[fd].inode = -1;
    return 0;
}

int fs_delete(const char *path) {
    int parent, child;
    char name[28];
    if (resolve_path(path, &parent, &child, name) != 0) return -1;
    if (child < 0) return -1;
    struct inode parent_inode;
    read_inode(parent, &parent_inode);
    int total_entries = parent_inode.size / sizeof(struct dir_entry);
    for (int i = 0; i < total_entries; i++) {
        int block_index = i * sizeof(struct dir_entry) / BLOCK_SIZE;
        int block = (block_index < 12) ? parent_inode.blocks[block_index] : 0;
        if (block == 0) continue;
        char buf[BLOCK_SIZE];
        read_block(block, buf);
        int offset = (i * sizeof(struct dir_entry)) % BLOCK_SIZE;
        struct dir_entry *entry = (struct dir_entry*)(buf + offset);
        if (entry->inode == child) {
            entry->inode = 0;
            write_block(block, buf);
            break;
        }
    }
    free_inode(child);
    return 0;
}

int fs_mkdir(const char *path) {
    int parent, child;
    char name[28];
    if (resolve_path(path, &parent, &child, name) != 0) {
        int ino = alloc_inode();
        if (ino < 0) return -1;
        struct inode inode;
        read_inode(ino, &inode);
        inode.mode = 0x41FF;
        write_inode(ino, &inode);
        if (add_dir_entry(parent, name, ino) != 0) {
            free_inode(ino);
            return -1;
        }
        add_dir_entry(ino, ".", ino);
        add_dir_entry(ino, "..", parent);
        return 0;
    }
    return -1;
}

int fs_readdir(const char *path, void *buf, int (*callback)(const char *name, uint32_t inode)) {
    int ino;
    if (resolve_path(path, 0, &ino, 0) != 0) return -1;
    struct inode dir_inode;
    read_inode(ino, &dir_inode);
    if ((dir_inode.mode & 0xF000) != 0x4000) return -1;

    int total_entries = dir_inode.size / sizeof(struct dir_entry);
    for (int i = 0; i < total_entries; i++) {
        int block_index = i * sizeof(struct dir_entry) / BLOCK_SIZE;
        if (block_index >= 12) return -1;
        int block = dir_inode.blocks[block_index];
        if (block == 0) continue;

        char buf_block[BLOCK_SIZE];
        read_block(block, buf_block);
        int offset = (i * sizeof(struct dir_entry)) % BLOCK_SIZE;
        struct dir_entry *entry = (struct dir_entry*)(buf_block + offset);
        if (entry->inode != 0) {
            if (callback && callback(entry->name, entry->inode) != 0) break;
        }
    }
    return 0;
}
