#ifndef STRING_H
#define STRING_H

#include "common.h"

void memcpy(void *dst, const void *src, int len);
void memset(void *b, int c, size_t len);
int  memcmp(const void *b1, const void *b2, size_t len);

int  strcmp(const char *s1, const char *s2);
int  strncmp(const char *s1, const char *s2, size_t len);
void strcpy(char *dst, const char *src);
void strcat(char *dst, const char *src);
char *strchr(const char *s, char ch);

size_t strlen(const char *s);

#endif /* STRING_H */
