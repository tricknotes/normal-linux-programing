#include <stdio.h>
#include <signal.h>

typedef void (*sighandler_t)(int);

sighandler_t handleSignal(int sig, sighandler_t handler) {
  struct sigaction act, old;

  act.sa_handler = handler;
  sigemptyset(&act.sa_mask);
  // act.sa_flags = SA_RESTART;
  if (sigaction(sig, &act, &old) < 0) {
    return NULL;
  }
  return old.sa_handler;
}

void printMessage() {
  fprintf(stdout, "TRAP SIGINT\n");
}

int main(int argc, char *argv[]) {
  handleSignal(SIGINT, printMessage);
  pause();
}
