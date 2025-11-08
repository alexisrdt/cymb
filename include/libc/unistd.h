#ifndef UNISTD_H
#define UNISTD_H

typedef long ssize_t;

long syscall(long number, ...);

int close(int file);

ssize_t read(int file, void* buffer, size_t count);
ssize_t write(int file, const void* buffer, size_t count);

#endif
