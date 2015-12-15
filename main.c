#include "defines.h"
#include "serial.h"
#include "lib.h"

int main(void)
{
    serial_init(SERIAL_DEFAULT_DEVICE);

    puts("Hello World!\n");
    puts("Hello World2\n");

    while(1)
        ;

    return 0;
}
