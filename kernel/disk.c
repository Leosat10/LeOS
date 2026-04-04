#include "disk.h"
#include "io.h"

// Read one sector (LBA addressing) into buffer
void ata_read_sector(int lba, char* buffer)
{
    // Select drive and LBA
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);
    outb(0x1F3, (unsigned char)(lba));
    outb(0x1F4, (unsigned char)(lba >> 8));
    outb(0x1F5, (unsigned char)(lba >> 16));

    // Read command
    outb(0x1F7, 0x20);

    // Wait for drive ready
    unsigned char status;
    do {
        status = inb(0x1F7);
    } while ((status & 0x80) || !(status & 0x08));

    // Read data (256 words)
    for (int i = 0; i < 256; i++) {
        ((unsigned short*)buffer)[i] = inw(0x1F0);
    }
}

// Write one sector (LBA addressing) from buffer
void ata_write_sector(int lba, const char* buffer)
{
    // Wait for drive ready
    unsigned char status;
    do {
        status = inb(0x1F7);
    } while ((status & 0x80) || !(status & 0x08));

    // Select drive and LBA
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);
    outb(0x1F3, (unsigned char)(lba));
    outb(0x1F4, (unsigned char)(lba >> 8));
    outb(0x1F5, (unsigned char)(lba >> 16));

    // Write command
    outb(0x1F7, 0x30);

    // Wait for DRQ
    do {
        status = inb(0x1F7);
    } while (!(status & 0x08));

    // Write data (256 words)
    for (int i = 0; i < 256; i++) {
        outw(0x1F0, ((unsigned short*)buffer)[i]);
    }

    // Wait for completion
    do {
        status = inb(0x1F7);
    } while ((status & 0x80) || !(status & 0x08));
}
