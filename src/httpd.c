#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

static void log_exit(char *fmt, ...);
static void* xmalloc(size_t size);
static void install_signal_handlers(void);
static void service(FILE *in, FILE *out, char *docroot);

typedef void (*sighandler_t)(int);

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <docroot>\n", argv[0]);
    exit(1);
  }
  install_signal_handlers();
  service(stdin, stdout, argv[1]);
  exit(0);
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

static void trap_signal(int signal, sighandler_t handler) {
  struct sigaction act;

  act.sa_handler = handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_RESTART;
  if (sigaction(signal, &act, NULL) < 0) {
    log_exit("sigaction() failed: %s", strerror(errno));
  }
}

static void signal_exit(int signal) {
  log_exit("exit by signal %d", signal);
}

static void install_signal_handlers(void) {
  trap_signal(SIGPIPE, signal_exit);
}

static void service(FILE *in, FILE *out, char *docroot) {
  // TODO impliment this
}
