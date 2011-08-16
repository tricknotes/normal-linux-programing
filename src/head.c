#include <stdio.h>
#include <stdlib.h>

static long do_head(FILE *f, long nlines);

int main(int argc, char *argv[]) {
    long nlines;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s n [file file ...]\n", argv[0]);
        exit(1);
    }
    nlines = atol(argv[1]);
    if (argc == 2) {
        do_head(stdin, nlines);
    } else {
        int i;

        for (i = 2; i < argc; i++) {
            FILE *f;

            f = fopen(argv[i], "r");
            if (!f) {
                perror(argv[i]);
                exit(1);
            }
            nlines = do_head(f, nlines);
            fclose(f);
        }
    }
    exit(0);
}

static long do_head(FILE *f, long nlines) {
    int c;

    while (nlines > 0 && (c = getc(f)) != EOF) {
        if (putchar(c) < 0) { exit(1); }
        if (c == '\n') { nlines--; }
    }
    return nlines;
}
