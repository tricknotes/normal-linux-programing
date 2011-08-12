#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int count = 0;
    FILE *f = fdopen(STDIN_FILENO, "r");

    char c;
    while ((c = fgetc(f)) != EOF) {
        if ('\n' == c) { count++; }
    }
    fclose(f);
    if ('\n' == c) { count--; }
    printf("%d\n", count);
    exit(0);
}
