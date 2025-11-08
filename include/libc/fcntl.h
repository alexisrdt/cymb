#ifndef FCNTL_H
#define FCNTL_H

#define O_RDONLY  00
#define O_WRONLY  01
#define O_RDWR    02

#define O_CREAT   0100
#define O_EXCL    0200
#define O_TRUNC  01000
#define O_APPEND 02000

#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR)
#define S_IRGRP (S_IRUSR >> 3)
#define S_IWGRP (S_IWUSR >> 3)
#define S_IXGRP (S_IXUSR >> 3)
#define S_IRWXG (S_IRWXU >> 3)
#define S_IROTH (S_IRGRP >> 3)
#define S_IWOTH (S_IWGRP >> 3)
#define S_IXOTH (S_IXGRP >> 3)
#define S_IRWXO (S_IRWXG >> 3)

#define AT_FDCWD -100

typedef unsigned int mode_t;

int openat(int directory, const char* pathname, int flags, ...);

#endif
