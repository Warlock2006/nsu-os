#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread(void *args) {
  printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(), gettid());
	return NULL;
}

int main(void) {
  pthread_t tid;
  pthread_attr_t attr;
  int err = 0;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  while (1) {
    err = pthread_create(&tid, &attr, mythread, NULL);

    if (err) {
      perror("Thread creating error");
      break;
    }
  }
  pthread_attr_destroy(&attr);

  return 0;
}