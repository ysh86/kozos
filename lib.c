#include "defines.h"
#include "serial.h"
#include "lib.h"

void *memset(void *b, int c, size_t len)
{
    char *p = b;
    for (; len > 0; len--) {
        *(p++) = c;
    }
    return b;
}

void *memcpy(void *dst, const void *src, size_t len)
{
    char *d = dst;
    const char *s = src;
    for (; len > 0; len--) {
        *(d++) = *(s++);
    }
    return dst;
}

int memcmp(const void *b1, const void *b2, size_t len)
{
    const char *p1 = b1, *p2 = b2;
    for (; len > 0; len--) {
        if (*p1 != *p2) {
            return (*p1 > *p2) ? 1 : -1;
        }
        p1++;
        p2++;
    }
    return 0;
}

size_t strlen(const char *s)
{
    size_t len;
    for (len = 0; *s; s++, len++)
        ;
    return len;
}

char *strcpy(char *dst, const char *src)
{
    char *d = dst;
    for (;; dst++, src++) {
        *dst = *src;
        if (!*src) break;
    }
    return d;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 || *s2) {
        if (*s1 != *s2) {
            return (*s1 > *s2) ? 1 : -1;
        }
        s1++;
        s2++;
    }
    return 0;
}

int strncmp(const char *s1, const char *s2, size_t len)
{
    while ((*s1 || *s2) && (len > 0)) {
        if (*s1 != *s2) {
            return (*s1 > *s2) ? 1 : -1;
        }
        s1++;
        s2++;
        len--;
    }
    return 0;
}

int putc(int c)
{
    if (c == '\n') {
        serial_send_byte(SERIAL_DEFAULT_DEVICE, '\r');
    }
    return serial_send_byte(SERIAL_DEFAULT_DEVICE, c);
}

int puts(const char *str)
{
    while (*str) {
        putc(*(str++));
    }
    return 0;
}

int putxval(unsigned int value, size_t column)
{
    char buf[9];
    char *p;

    p = buf + sizeof(buf) - 1;
    *(p--) = '\0';

    if (!value && !column) {
        column++;
    }

    while (value || column) {
        *(p--) = "0123456789abcdef"[value & 0xf];
        value >>= 4;
        if (column) column--;
    }

    puts(p + 1);

    return 0;
}
