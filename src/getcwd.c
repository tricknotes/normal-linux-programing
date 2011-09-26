#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define INIT_BUFSIZE 1024

char *my_getcwd(void) {
  char *buf, *tmp;
  size_t size = INIT_BUFSIZE;

  buf = malloc(size);
  if (!buf) return NULL;

  while (1) {
    errno = 0;
    if (getcwd(buf, size)) {
      return buf;
    }
    if (errno != ERANGE) break;
    size *= 2;
    tmp = realloc(buf, size);
    if (!tmp) {
      break;
    }
    buf = tmp;
  }
  free(buf);
  return NULL;
}

int main(int argc, char *argv[]) {
  fprintf(stdout, "Current DIR: %s\n", my_getcwd());
  return 0;
}
