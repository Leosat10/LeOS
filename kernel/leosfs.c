#include "kernel.h"
#include "string.h"
#include "disk.h"
#include "leosfs.h"

#define FS_TABLE_SECTOR 21

file_t files[MAX_FILES];

void fs_init()
{
    ata_read_sector(FS_TABLE_SECTOR, (char*)files);
	
}

void fs_list()
{
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (files[i].name[0] != 0)
        {
            print(files[i].name);
            print("\n");
        }
    }
}

file_t* find_file(const char* name)
{
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (files[i].name[0] != 0 &&
            strcmp(files[i].name, name) == 0)
        {
            return &files[i];
        }
    }
    return 0;
}

int fs_read(const char* name, char* buffer)
{
    file_t* f = find_file(name);
    if (!f) return -1;

    for (int i = 0; i < f->size; i++)
    {
        ata_read_sector(f->start + i, buffer + i * 512);
    }

    return f->size * 512;
}
