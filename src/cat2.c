#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void putsStdout(FILE *f, const char *path);

int main(int argc, char *argv[]) {
    int i;
    FILE *f;

    if (argc < 2) {
        f = fdopen(STDIN_FILENO, "r");
        putsStdout(f, argv[i]);
    }
    for (i = 1; i < argc; i++) {
        f = fopen(argv[i], "r");
        putsStdout(f, argv[i]);
    }
    exit(0);
}

void putsStdout(FILE *f, const char *path) {
    int c;
    if (!f) {
      perror(path);
      exit(1);
    }
    while((c =fgetc(f)) != EOF) {
        if (putchar(c) < 0) { exit(1); }
    }
}
