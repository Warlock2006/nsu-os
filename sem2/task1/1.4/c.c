#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>

void free_safe(void *arg) {
  void *to_free = *(void **) arg;
  printf("CLEANUP %p", to_free);
  if (to_free) {
    free(to_free); 
  }
}

void *mythread(void *args) {
  char *msg = NULL;

  pthread_cleanup_push(free_safe, (void *)&msg);
  msg = malloc(sizeof("Hello world!\n"));
  printf("msg %p\n", msg);

  if (!msg) {
    perror("Malloc error");
    return NULL;
  }
  snprintf(msg, sizeof("Hello world!\n"), "Hello world!\n");
  printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(), gettid());

  while (1) {
    printf("%s", msg);
    pthread_exit(NULL);
  }

  pthread_cleanup_pop(1);
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

  return 0;
}