#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
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
  free(args);
  return NULL;
}

int main(void) {
  Person *person = malloc(sizeof(Person));

  if (!person) {
    perror("malloc error");
    return 1;
  }

  person->age = 21;
  person->name = "Alex";

  pthread_t tid;
  pthread_attr_t attr;
  int err = 0;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  err = pthread_create(&tid, &attr, mythread, (void *)person);
  pthread_attr_destroy(&attr);

  if (err) {
    perror("Thread creating error");
    free(person);
  }

  pthread_exit(0);
}