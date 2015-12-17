#ifndef _LIB_H_INCLUDED_
#define _LIB_H_INCLUDED_

void *memset(void *b, int c, size_t len);
void *memcpy(void *dst, const void *src, size_t len);
int memcmp(const void *b1, const void *b2, size_t len);

size_t strlen(const char *s);
char *strcpy(char *dst, const char *src);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t len);

int putc(int c);
int puts(const char *str);
int putxval(unsigned int value, size_t column);

#endif

