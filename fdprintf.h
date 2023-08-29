#ifndef FDPRINTF_H
#define FDPRINTF_H
#include <stdarg.h>

int vfdprintf(int fd, const char * __restrict format, va_list ap);
int fdprintf(int fd, const char * __restrict format, ...);

#endif
