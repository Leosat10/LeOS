#include "kernel.h"
#include "string.h"
#include "fs.h"
#include "io.h"

// ------------------------------------------------------------
// Forward declarations (functions from fs.c)
// ------------------------------------------------------------
int resolve_path(const char *path, int *parent_ino, int *child_ino, char *name);
void read_inode(int ino, struct inode *inode);

// ------------------------------------------------------------
// Current working directory and helpers
// ------------------------------------------------------------
static char cwd[128] = "/";

static void get_full_path(const char *input_path, char *full_path) {
    if (input_path[0] == '/') {
        strcpy(full_path, input_path);
    } else {
        strcpy(full_path, cwd);
        if (cwd[1] != 0) strcat(full_path, "/");
        strcat(full_path, input_path);
    }
}

// ------------------------------------------------------------
// String helper (strrchr) – because we may not have it in string.c
// ------------------------------------------------------------
static char *my_strrchr(const char *s, int c) {
    char *last = 0;
    while (*s) {
        if (*s == (char)c) last = (char*)s;
        s++;
    }
    return last;
}

// ------------------------------------------------------------
// Existing functions from v2.0 shell
// ------------------------------------------------------------
char input[128];
int pos = 0;

void trim_input(char* str) {
    for (int i = 0; str[i]; i++) {
        if (str[i] == '\n' || str[i] == '\r') {
            str[i] = '\0';
            return;
        }
    }
}

void print_int(int n) {
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

void calc(char* e) {
    int a = 0, b = 0, i = 0;
    char op = 0;
    while (e[i] && e[i] != '+' && e[i] != '-' && e[i] != '*' && e[i] != '/') {
        a = a * 10 + (e[i] - '0');
        i++;
    }
    op = e[i++];
    while (e[i]) {
        b = b * 10 + (e[i] - '0');
        i++;
    }
    int r = 0;
    if (op == '+') r = a + b;
    else if (op == '-') r = a - b;
    else if (op == '*') r = a * b;
    else if (op == '/') r = b ? a / b : 0;
    print("\nResult: ");
    print_int(r);
}

static int safe_print_dir_entry(const char *name, uint32_t inode) {
    (void)inode;
    if (name && *name) {
        print(name);
        print("\n");
    }
    return 0;
}

void cmd_leohelp() {
    print("\nLeosOS Commands:\n");
    print("leohelp\n");
    print("leoclear\n");
    print("leoecho\n");
    print("leoversion\n");
    print("leotime\n");
    print("leocalc\n");
    print("leoreboot\n");
    print("leoshutdown\n");
    print("leols\n");
    print("leocat <file>\n");
    print("leotouch <file>\n");
    print("leorm <file>\n");
    print("leomkdir <dir>\n");
    print("leowrite <file> <text>\n");
    print("leorun <program>\n");
    print("leocd <dir>\n");
    print("leopwd\n");
    print("leonano <file>\n");
}

// ------------------------------------------------------------
// Command execution
// ------------------------------------------------------------
void execute() {
    trim_input(input);

    if (strncmp(input, "leohelp", 7) == 0) {
        cmd_leohelp();
    }
    else if (strncmp(input, "leoclear", 8) == 0) {
        clear();
    }
    else if (strncmp(input, "leoecho ", 8) == 0) {
        print("\n");
        print(input + 8);
        print("\n");
    }
    else if (strncmp(input, "leoversion", 10) == 0) {
        print("\nLeosOS Version 1.0\n");
    }
    else if (strncmp(input, "leotime", 7) == 0) {
        print("\nDemo system clock\n");
    }
    else if (strncmp(input, "leocalc ", 8) == 0) {
        calc(input + 8);
    }
    else if (strncmp(input, "leoreboot", 9) == 0) {
        print("\nRebooting...\n");
        __asm__ volatile("mov $0xFE, %al\nout %al,$0x64");
    }
    else if (strncmp(input, "leoshutdown", 11) == 0) {
        print("\nSystem halted\n");
        while (1) __asm__("hlt");
    }

    // ------------------------------------------------------------
    // File system commands using get_full_path
    // ------------------------------------------------------------
    else if (strncmp(input, "leotouch ", 9) == 0) {
        char* name = input + 9;
        char fullpath[128];
        get_full_path(name, fullpath);
        int ret = fs_create(fullpath, 0x81FF);
        if (ret == 0) print("\nFile created\n");
        else print("\nFailed to create file\n");
    }
    else if (strncmp(input, "leorm ", 6) == 0) {
        char* name = input + 6;
        char fullpath[128];
        get_full_path(name, fullpath);
        int ret = fs_delete(fullpath);
        if (ret == 0) print("\nFile deleted\n");
        else print("\nFailed to delete file\n");
    }
    else if (strncmp(input, "leomkdir ", 9) == 0) {
        char* name = input + 9;
        char fullpath[128];
        get_full_path(name, fullpath);
        int ret = fs_mkdir(fullpath);
        if (ret == 0) print("\nDirectory created\n");
        else print("\nFailed to create directory\n");
    }
    else if (strncmp(input, "leowrite ", 9) == 0) {
        char* rest = input + 9;
        char* filename = rest;
        while (*rest && *rest != ' ') rest++;
        if (*rest) {
            *rest = 0;
            rest++;
            if (*rest == '"') rest++;
            char* text = rest;
            int len = 0;
            while (text[len] && text[len] != '"') len++;
            text[len] = 0;

            char fullpath[128];
            get_full_path(filename, fullpath);
            int fd = fs_open(fullpath);
            if (fd < 0) {
                if (fs_create(fullpath, 0x81FF) != 0) {
                    print("\nCannot create file\n");
                    return;
                }
                fd = fs_open(fullpath);
            }
            if (fd >= 0) {
                int w = fs_write(fd, text, len);
                if (w > 0) {
                    print("\nWritten ");
                    print_int(w);
                    print(" bytes\n");
                } else {
                    print("\nWrite failed\n");
                }
                fs_close(fd);
            } else {
                print("\nCannot open file\n");
            }
        } else {
            print("\nUsage: leowrite <file> <text>\n");
        }
    }
    else if (strncmp(input, "leocat ", 7) == 0) {
        char* name = input + 7;
        char fullpath[128];
        get_full_path(name, fullpath);
        int fd = fs_open(fullpath);
        if (fd < 0) {
            print("\nFile not found\n");
        } else {
            char buf[4096];
            int n = fs_read(fd, buf, sizeof(buf)-1);
            if (n > 0) {
                buf[n] = 0;
                print("\n");
                print(buf);
            }
            fs_close(fd);
            print("\n");
        }
    }
    else if (strncmp(input, "leorun ", 7) == 0) {
        char* name = input + 7;
        trim_input(name);
        char fullpath[128];
        get_full_path(name, fullpath);
        int fd = fs_open(fullpath);
        if (fd < 0) {
            print("\nProgram not found\n");
            return;
        }
        static unsigned char prog[8192];
        int n = fs_read(fd, (char*)prog, sizeof(prog));
        fs_close(fd);
        if (n <= 0) {
            print("\nProgram empty or read error\n");
        } else {
            print("\nRunning...\n");
            print("\n");
            unsigned char* load_addr = (unsigned char*)0x20000;
            for (int i = 0; i < n; i++) load_addr[i] = prog[i];
            void (*entry)() = (void (*)())0x20000;
            entry();
            print("\nReturned OK\n");
        }
    }
    else if (strncmp(input, "leols", 5) == 0) {
        print("\n");
        fs_readdir(cwd, 0, safe_print_dir_entry);
        print("\n");
    }

else if (strncmp(input, "leocd", 5) == 0) {
    char* dir = input + 5;
    while (*dir == ' ') dir++;
    if (*dir == 0) {
        strcpy(cwd, "/");
        print("\n");
    } else if (strcmp(dir, "..") == 0) {
        // Go up one level
        char *last_slash = my_strrchr(cwd, '/');
        if (last_slash == cwd) {
            strcpy(cwd, "/");
        } else {
            *last_slash = 0;
        }
        print("\n");
    } else {
        char fullpath[128];
        get_full_path(dir, fullpath);
        print("leocd: fullpath='"); print(fullpath); print("'\n");
        int ino;
        if (resolve_path(fullpath, 0, &ino, 0) == 0) {
            print("resolve_path succeeded, ino="); print_int(ino); print("\n");
            struct inode inode;
            read_inode(ino, &inode);
            print("inode mode="); print_int(inode.mode); print("\n");
            if ((inode.mode & 0xF000) == 0x4000) {
                strcpy(cwd, fullpath);
                print("\n");
            } else {
                print("\nNot a directory\n");
            }
        } else {
            print("\nDirectory not found\n");
        }
    }
}



    else if (strncmp(input, "leopwd", 5) == 0) {
        print("\n");
        print(cwd);
        print("\n");
    }
    else if (strncmp(input, "leonano ", 8) == 0) {
        char* filename = input + 8;
        char fullpath[128];
        get_full_path(filename, fullpath);
        print("\nSimple editor not yet implemented. Use leowrite to create files.\n");
    }
    else {
        print("\nCommand not found\n");
    }
}

// ------------------------------------------------------------
// Shell main loop
// ------------------------------------------------------------
void shell() {
    print("> ");
    while (1) {
        char c = keyboard_read();
        if (c == '\n' || c == '\r') {
            input[pos] = '\0';
            print("\n");
            execute();
            pos = 0;
            input[0] = '\0';
            print("> ");
        }
        else if (c == 8) {
            if (pos > 0) {
                pos--;
                input[pos] = 0;
                backspace();
            }
        }
        else if (c && pos < 127) {
            input[pos++] = c;
            char s[2] = {c, 0};
            print(s);
        }
    }
}
