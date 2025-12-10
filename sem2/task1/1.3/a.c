#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct Person {
  int age;
  char *name;
} Person;

void *mythread(void *args) {
  Person person = *(Person *)args;
  printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(), gettid());
  printf("mythread person is: %s age of %d\n", person.name, person.age);
  return NULL;
}

int main(void) {
  Person person = {.age = 21, .name = "Alex"};
  pthread_t tid;
  int err = 0;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

  err = pthread_create(&tid, NULL, mythread, (void *)&person);

  if (err) {
    perror("Thread creating error");
  }

  err = pthread_join(tid, NULL);

  if (err) {
    perror("Thread join error");
  }

  return 0;
}