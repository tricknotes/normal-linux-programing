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
struct HTTPRequest;
static void free_request(struct HTTPRequest *req);
static struct HTTPRequest *read_request(FILE *in);
#define MAX_REQUEST_BODY_LENGTH 1024

typedef void (*sighandler_t)(int);

struct HTTPHeaderField {
  char *name;
  char *value;
  struct HTTPHeaderField *next;
};

struct HTTPRequest {
  int protocol_minor_version;
  char *method;
  char *path;
  struct HTTPHeaderField *header;
  char *body;
  long length;
};

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

static void free_request(struct HTTPRequest *req) {
  struct HTTPHeaderField *h, *head;

  head = req->header;
  while (head) {
    h = head;
    head = head->next;
    free(h->name);
    free(h->value);
    free(h->value);
    free(h);
  }
  free(req->method);
  free(req->path);
  free(req->body);
  free(req);
}

static void read_request_line(struct HTTPRequest *req, FILE *in) {
  // TODO impliment this
}

static struct HTTPHeaderField *read_header_field(FILE *in) {
  // TODO impliment this
}

static long content_length(struct HTTPRequest *req) {
  // TODO impliment this
}

static struct HTTPRequest *read_request(FILE *in) {
  struct HTTPRequest *req;
  struct HTTPHeaderField *h;

  req = xmalloc(sizeof(struct HTTPRequest));
  read_request_line(req, in);
  req->header = NULL;
  while (h = read_header_field(in)) {
    h->next = req->header;
    req->header = h;
  }
  req->length = content_length(req);
  if (req->length != 0) {
    if (req->length > MAX_REQUEST_BODY_LENGTH) {
      log_exit("request body too long");
    }
    req->body = xmalloc(req->length);
    if (fread(req->body, req->length, 1, in) < 1) {
      log_exit("failed to read request body");
    }
  } else {
    req->body = NULL;
  }
  return req;
}
