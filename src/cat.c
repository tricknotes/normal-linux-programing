#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static void do_cat(int fd, const char *path);
static void die(const char *s);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        do_cat(STDIN_FILENO, "STDIN_FILENO");
    } else {
        int i;
        for (i = 1; i < argc; i++) {
            char *path = argv[i];
            int fd = open(path, O_RDONLY);
            do_cat(fd, path);
        }
    }
    exit(0);
}

#define BUFFER_SIZE 2040

static void do_cat(int fd, const char *path) {
    unsigned char buf[BUFFER_SIZE];

    if (fd < 0) { die(path); }
    while (1) {
        int n;
        n = read(fd, buf, sizeof buf);
        if (n < 0) { die(path); }
        if (n == 0) { break; }
        if (write(STDOUT_FILENO, buf, n) < 0) { die(path); }
    }
    if (close(fd) < 0) { die(path); }
}

static void die(const char *s) {
    perror(s);
    exit(1);
}
