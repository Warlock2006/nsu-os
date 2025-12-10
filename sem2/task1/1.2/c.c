#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread(void *args) {
  void *result = (void *)"Hello world!";
  printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(), gettid());
	return result;
}

int main(void) {
  pthread_t tid;
  void *result = NULL;
  int err = 0;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

  err = pthread_create(&tid, NULL, mythread, NULL);

  if (err) {
    perror("Thread creating error");
  }

  err = pthread_join(tid, &result);

  if (err) {
    perror("Thread join error");
  }

  printf("Mythread result is: %s\n", (char *)result);

  return 0;
}