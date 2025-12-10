#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread(void *args) {
  int err = 0;
  err = pthread_detach(pthread_self());
  
  if (err) {
    perror("Detatch error");
    return NULL;
  }

  printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(), gettid());
	return NULL;
}

int main(void) {
  pthread_t tid;
  int err = 0;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

  while (1) {
    err = pthread_create(&tid, NULL, mythread, NULL);

    if (err) {
      perror("Thread creating error");
      break;
    }
  }

  return 0;
}