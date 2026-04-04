#include "kernel.h"
#include "io.h"
volatile char* video = (volatile char*)0xB8000;

int row = 0;
int col = 0;
int cursor_pos = 0;
void scroll()
{
    char* video = (char*)0xb8000;
    for (int i = 0; i < 24 * 80; i++)
    {
        video[i * 2] = video[(i + 80) * 2];
        video[i * 2 + 1] = video[(i + 80) * 2 + 1];
    }
    for (int i = 24 * 80; i < 25 * 80; i++)
    {
        video[i * 2] = ' ';
        video[i * 2 + 1] = 0x0F;
    }

    cursor_pos -= 80;
}


void putc(char c)
{
    char* video = (char*)0xb8000;

    if (c == '\n')
    {
        cursor_pos += 80 - (cursor_pos % 80);
    }
    else
    {
        video[cursor_pos * 2] = c;
        video[cursor_pos * 2 + 1] = 0x0F;
        cursor_pos++;
    }

    if (cursor_pos >= 80 * 25)
    {
        scroll();
    }

    set_cursor(cursor_pos);
}

void print(const char* s)
{
    while(*s)
    {
        putc(*s++);
    }
}

void clear()
{
    for(int i = 0; i < 80 * 25; i++)
    {
        video[i * 2] = ' ';
        video[i * 2 + 1] = 0x07;
    }

    cursor_pos = 0;
    set_cursor(0);
}

void backspace()
{
    if (cursor_pos > 0)
    {
        cursor_pos--;

        char* video = (char*)0xb8000;
        video[cursor_pos * 2] = ' ';
        video[cursor_pos * 2 + 1] = 0x0F;

        set_cursor(cursor_pos);
    }
}

void set_cursor(int pos)
{
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));

    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

int get_cursor()
{
    return cursor_pos;
}
