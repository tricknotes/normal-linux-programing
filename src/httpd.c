#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netdb.h>

static void log_exit(char *fmt, ...);
static void* xmalloc(size_t size);
static void install_signal_handlers(void);
static void service(FILE *in, FILE *out, char *docroot);
struct HTTPRequest;
static void free_request(struct HTTPRequest *req);
static struct HTTPRequest *read_request(FILE *in);
static void respond_to(struct HTTPRequest *req, FILE *out, char *docroot);
#define MAX_REQUEST_BODY_LENGTH 1024 * 1024
#define LINE_BUF_SIZE 1024
#define BLOCK_BUF_SIZE 1024
#define HTTP_MINOR_VERSION 1
#define TIME_BUF_SIZE 64
#define SERVER_NAME "tricknotes"
#define SERVER_VERSION "1.0"
#define MAX_BACKLOG 5
#define DEFAULT_PORT "80"

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

struct FileInfo {
  char *path;
  long size;
  int ok;
};

#define USAGE "Usage: %s [--port=n] [--chroot --user=u --group=g] [--debug] <docroot>\n"
#define LOG_PID 1101
#define LOG_NDELAY 1
#define LOG_DAEMON "./log_file"

static void setup_environment(char *docroot, char *user, char *group);
static void openlog(char *server_name, int pid, char *log);
static int listen_socket(char *port);
static void server_main(int server, char *docroot);
static void become_daemon(void);

static int debug_mode = 0;

static struct option longopts[] = {
  {"debug",  no_argument,       &debug_mode, 1},
  {"chroot", no_argument,       NULL, 'c'},
  {"user",   required_argument, NULL, 'u'},
  {"group",  required_argument, NULL, 'g'},
  {"port",   required_argument, NULL, 'p'},
  {"help",   no_argument,       NULL, 'h'},
  {0, 0, 0, 0}
};

int main(int argc, char *argv[]) {
  int server;
  char *port = NULL;
  char *docroot;
  int do_chroot = 0;
  char *user = NULL;
  char *group = NULL;
  int opt;

  while ((opt = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
    switch (opt) {
      case 0:
        break;
      case 'c':
        do_chroot = 1;
        break;
      case 'u':
        user = optarg;
        break;
      case 'g':
        group = optarg;
        break;
      case 'p':
        port = optarg;
        break;
      case 'h':
        fprintf(stdout, USAGE, argv[0]);
        exit(0);
      case '?':
        fprintf(stderr, USAGE, argv[0]);
        exit(1);
    }
  }
  if (optind != argc - 1) {
    fprintf(stderr, USAGE, argv[0]);
    exit(1);
  }
  docroot = argv[optind];

  if (do_chroot) {
    setup_environment(docroot, user, group);
    docroot = "";
  }
  install_signal_handlers();
  server = listen_socket(port);
  if (!debug_mode) {
    openlog(SERVER_NAME, LOG_PID|LOG_NDELAY, LOG_DAEMON);
    become_daemon();
  }
  server_main(server, docroot);
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
  struct HTTPRequest *req;

  req = read_request(in);
  respond_to(req, out, docroot);
  free_request(req);
}

static void free_request(struct HTTPRequest *req) {
  struct HTTPHeaderField *h, *head;

  head = req->header;
  while (head) {
    h = head;
    head = head->next;
    free(h->name);
    free(h->value);
    free(h);
  }
  free(req->method);
  free(req->path);
  free(req->body);
  free(req);
}

static void upcase(char *str) {
  int i;
  for (i = 0; str[i]; i++) {
    str[i] = toupper(str[i]);
  }
}

static void read_request_line(struct HTTPRequest *req, FILE *in) {
  char buf[LINE_BUF_SIZE];
  char *path, *p;

  if(!fgets(buf, LINE_BUF_SIZE, in)) {
    log_exit("no request line");
  }
  p = strchr(buf, ' ');
  if (!p) {
    log_exit("parse error on request line (1): %s", buf);
  }
  *p++ = '\0';
  req->method = xmalloc(p - buf);
  strcpy(req->method, buf);
  upcase(req->method);

  path = p;
  p = strchr(path, ' ');
  if (!p) {
    log_exit("parse error on request line (2): %s", buf);
  }
  *p++ = '\0';
  req->path = xmalloc(p - path);
  strcpy(req->path, path);

  if (strncasecmp(p, "HTTP/1.", strlen("HTTP/1.")) != 0) {
    log_exit("parse error on request line (3): %s", buf);
  }
  p += strlen("HTTP/1.");
  req->protocol_minor_version = atoi(p);
}

static struct HTTPHeaderField *read_header_field(FILE *in) {
  struct HTTPHeaderField *h;
  char buf[LINE_BUF_SIZE];
  char *p;

  if (!fgets(buf, LINE_BUF_SIZE, in)) {
    log_exit("failed to request header field: %s", strerror(errno));
  }
  if ((buf[0] == '\n') || (strcmp(buf, "\r\n") == 0)) {
    return NULL;
  }

  p = strchr(buf, ':');
  if (!p) {
    log_exit("parse error on request header field: %s", buf);
  }
  *p++ = '\0';
  h = xmalloc(sizeof(struct HTTPHeaderField));
  h->name = xmalloc(p - buf);
  strcpy(h->name, buf);

  p += strspn(p, " \t");
  h->value = xmalloc(strlen(p) + 1);
  strcpy(h->value, buf);

  return h;
}

static char *lookup_header_field_value(struct HTTPRequest *req, char *type) {
  struct HTTPHeaderField *h;

  for (h = req->header; h; h = h->next) {
    if (strcasecmp(h->name, type) == 0) {
      return h->value;
    }
  }
  return NULL;
}

static long content_length(struct HTTPRequest *req) {
  char *value;
  long length;

  value = lookup_header_field_value(req, "Content-Length");
  if (!value) {
    return 0;
  }
  length = atoi(value);
  if (length < 0) {
    log_exit("negative Content-Length value");
  }
  return length;
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

static char *build_fspath(char *docroot, char *urlpath) {
  char *path;

  path = xmalloc(strlen(docroot) + 1 + strlen(urlpath) + 1);
  sprintf(path, "%s/%s", docroot, urlpath);
  return path;
}

static struct FileInfo *get_fileinfo(char *docroot, char *urlpath) {
  struct FileInfo *info;
  struct stat st;

  info = xmalloc(sizeof(struct FileInfo));
  info->path = build_fspath(docroot, urlpath);
  info->ok = 0;
  if (lstat(info->path, &st) < 0) {
    return info;
  }
  if (!S_ISREG(st.st_mode)) {
    return info;
  }
  info->ok = 1;
  info->size = st.st_size;
  return info;
}

static void *free_fileinfo(struct FileInfo *info) {
  free(info->path);
  free(info);
}

static void output_common_header_fields(struct HTTPRequest *req, FILE *out, char *status) {
  time_t t;
  struct tm *tm;
  char buf[TIME_BUF_SIZE];

  t = time(NULL);
  tm = gmtime(&t);
  if (!tm) {
    log_exit("gmtime() failed: %s", strerror(errno));
  }
  strftime(buf, TIME_BUF_SIZE, "%a, %d %b %Y %H:%M:%S GMT", tm);
  fprintf(out, "HTTP/1.%d %s\r\n", HTTP_MINOR_VERSION, status);
  fprintf(out, "Date: %s\r\n", buf);
  fprintf(out, "Server: %s\r\n", SERVER_NAME, SERVER_VERSION);
  fprintf(out, "Connection: close\r\n");
}

static void not_found(struct HTTPRequest *req, FILE *out) {
  output_common_header_fields(req, out, "404 Not Found");
}

static void method_not_allowd(struct HTTPRequest *req, FILE *out) {
  output_common_header_fields(req, out, "405 Method Not Allowed");
}

static void not_implemented(struct HTTPRequest *req, FILE *out) {
  output_common_header_fields(req, out, "501 Not Implemented");
}

static char *guess_content_type(struct FileInfo *info) {
  // implement as stub
  return "text/plain";
}

static void do_file_response(struct HTTPRequest *req, FILE *out, char *docroot) {
  struct FileInfo *info;

  info = get_fileinfo(docroot, req->path);
  if (!info->ok) {
    free_fileinfo(info);
    not_found(req, out);
    return;
  }
  output_common_header_fields(req, out, "200 OK");
  fprintf(out, "Content-Length: %ld\r\n", info->size);
  fprintf(out, "Content-Type: %s\r\n", guess_content_type(info));
  fprintf(out, "\r\n");
  if (strcmp(req->method, "HEAD") != 0) {
    int fd;
    char buf[BLOCK_BUF_SIZE];
    ssize_t n;

    fd = open(info->path, O_RDONLY);
    if (fd < 0) {
      log_exit("failed to open %s: %s", info->path, strerror(errno));
    }
    while (1) {
      n = read(fd, buf, BLOCK_BUF_SIZE);
      if (n < 0) {
          log_exit("failed to read %s: %s", info->path, strerror(errno));
      }
      if (n == 0) {
        break;
      }
      if (fwrite(buf, n, 1, out) < 1) {
        log_exit("failed to write to socket: %s", strerror(errno));
      }
    }
    close(fd);
  }
  fflush(out);
  free_fileinfo(info);
}

static void respond_to(struct HTTPRequest *req, FILE *out, char *docroot) {
  if (strcmp(req->method, "GET") == 0) {
    do_file_response(req, out, docroot);
  } else if (strcmp(req->method, "HEAD") == 0) {
    do_file_response(req, out, docroot);
  } else if (strcmp(req->method, "POST") == 0) {
    method_not_allowd(req, out);
  } else {
    not_implemented(req, out);
  }
}

static void setup_environment(char *docroot, char *user, char *group) {
  // TODO implement this
}

static void openlog(char *server_name, int pid, char *log) {
  // TODO implement this
}

static int listen_socket(char *port) {
  struct addrinfo hints, *res, *ai;
  int err;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if ((err = getaddrinfo(NULL, port, &hints, &res)) != 0) {
    // TODO log_exit(gai_strerror(err));
    log_exit("TODO apply: log_exit(gai_strerror(err));");
  }
  for (ai = res; ai; ai = ai->ai_next) {
    int sock;

    sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sock < 0) {
      continue;
    }
    if (bind(sock, ai->ai_addr, ai->ai_addrlen) < 0) {
      close(sock);
      continue;
    }
    if (listen(sock, MAX_BACKLOG) < 0) {
      close(sock);
      continue;
    }
    freeaddrinfo(res);
    return sock;
  }
  log_exit("failed to listen socket");
  return -1; /* NOT REACH */
}

static void server_main(int server, char *docroot) {
  while (1) {
    struct sockaddr_storage addr;
    socklen_t addrlen  = sizeof addr;
    int sock;
    int pid;

    sock = accept(server, (struct sockaddr*)&addr, &addrlen);
    if (sock < 0) {
      log_exit("accept(2) failed: %s", strerror(errno));
    }
    pid = fork();
    if (pid < 0) {
      exit(3);
    }
    if (pid == 0) { /* child */
      FILE *in = fdopen(sock, "r");
      FILE *out = fdopen(sock, "w");

      service(in, out, docroot);
      exit(0);
    }
    close(sock);
  }
}

static void become_daemon(void) {
  // TODO implement this
}
