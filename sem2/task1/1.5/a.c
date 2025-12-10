#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/syscall.h>

void sigint_handler(int sig, siginfo_t *si, void *ucontext) {
  char msg[] = "thread sigint (sigaction): received SIGINT\n";
  write(STDERR_FILENO, msg, sizeof(msg));
}

void *sigint_handler_thread(void *args) {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  pthread_sigmask(SIG_UNBLOCK, &set, NULL);

  struct sigaction sa;
  sa.sa_sigaction = sigint_handler;
  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGINT, &sa, NULL) == -1) {
    perror("sigaction error");
    return NULL;
  }

  printf("thread sigint [%d %d %d]: Hello from thread sigint!\n", getpid(), getppid(), gettid());

  while (1) {
    sleep(5);
  }

  return NULL;
}

void *sigwait_thread(void *args) {
  int err;
  
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGQUIT);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  printf("thread sigwait [%d %d %d]: Hello from thread sigwait!\n", getpid(), getppid(), gettid());

  int sig;
  while (1) {
    err = sigwait(&set, &sig);

    if (err != 0) {
      perror("sigwait");
      break;
    }

    if (sig == SIGQUIT) {
      printf("thread sigwait received SIGQUIT!\n");
    }
  }
  
  return NULL;
}

int main(void) {
  sigset_t set;
  sigfillset(&set);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  pthread_t t2, t3;

  pthread_create(&t2, NULL, sigint_handler_thread, NULL);
  pthread_create(&t3, NULL, sigwait_thread, NULL);

  printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

  while (1) {
    sleep(1);
  }
  
  pthread_exit(NULL);
}