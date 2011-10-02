#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>

static void log_exit(char *fmt, ...);
static void* xmalloc(size_t size);

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

static void* xmalloc(size_t size) {
  void *p;

  p = malloc(size);
  if (!p) {
    log_exit("failed to allocate memory");
  }
  return p;
}

