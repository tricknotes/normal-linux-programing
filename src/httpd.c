#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static void log_exit(char *fmt, ...);

int main(int argc, char *argv[]) {
  // TODO impliment this
}

static void log_exit(char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
  va_end(ap);
  exit(1);
}
