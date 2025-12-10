#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

volatile long counter = 0;

void *mythread(void *args) {
  printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(), gettid());

  while (1) {
    counter++;
    pthread_testcancel();
  }
  return NULL;
}

int main(void) {
  pthread_t tid;
  int err = 0;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

  err = pthread_create(&tid, NULL, mythread, NULL);

  if (err) {
    perror("Thread creating error");
  }

  err = pthread_cancel(tid);
  if (err) {
    perror("Cancel error");
  }

  err = pthread_join(tid, NULL);

  if (err) {
    perror("Thread join error");
  }

  printf("Counter is: %d\n", counter);

  return 0;
}