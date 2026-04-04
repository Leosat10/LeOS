#include "kernel.h"
#include "io.h"
static unsigned char keymap[128] =
{
0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s',
'd','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v',
'b','n','m',',','.','/',0,'*',0,' ',
};

static unsigned char keymap_shift[128] =
{
0,27,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t',
'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,'A','S',
'D','F','G','H','J','K','L',':','"','~',0,'|','Z','X','C','V',
'B','N','M','<','>','?',0,'*',0,' ',
};

char keyboard_read()
{
    static int shift=0;
    unsigned char sc;

    while(1)
    {

	for (volatile int i = 0; i < 10000; i++);
        if(inb(0x64)&1)
        {
            sc=inb(0x60);

            
            if(sc==42 || sc==54)
            {
                shift=1;
                continue;
            }

            if(sc==170 || sc==182)
            {
                shift=0;
                continue;
            }

            if(sc<128)
            {
                if(shift)
                    return keymap_shift[sc];
                else
                    return keymap[sc];
            }
        }
    }
}
