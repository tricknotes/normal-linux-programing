#include <stdio.h>
#include <stdlib.h>

#define _GNU_SOURCE
#include <getopt.h>

static long do_head(FILE *f, long nlines);

#define DEFAULT_N_LINES 10

static struct option longopts[] = {
    {"lines", required_argument, NULL, 'n'},
    {"help",  no_argument,       NULL, 'h'},
    {0, 0, 0, 0}
};

int main(int argc, char *argv[]) {
    int opt;
    long nlines = DEFAULT_N_LINES;

    // parse option
    while ((opt = getopt_long(argc, argv, "n", longopts, NULL)) != -1) {
        switch (opt) {
            case 'n':
                nlines = atoi(optarg);
                break;
            case 'h':
                fprintf(stdout, "Usage: %s [-n LINES] [FILE ...]\n", argv[0]);
                exit(0);
            case '?':
                fprintf(stderr, "Usage: %s [-n LINES] [FILE ...]\n", argv[0]);
                exit(1);
        }
    }

    if (optind == argc) {
        do_head(stdin, nlines);
    } else {
        int i;

        for (i = optind; i < argc; i++) {
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
