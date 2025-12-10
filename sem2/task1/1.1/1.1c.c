#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#define NUM_THREADS 5

int global = 5;

void *mythread(void *arg) {
  int local = 10;
  const int const_local = 15;
  static int static_local = 20;
	printf("mythread [%d %d %d(gettid) %lu(pthread_self)]: Hello from mythread! global: %p, local: %p, const local: %p, static local: %p\n", getpid(), getppid(), gettid(), pthread_self(), &global, &local, &const_local, &static_local);
	return NULL;
}

int main() {
	pthread_t tids[NUM_THREADS];
	int err;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

	for (int i = 0; i < NUM_THREADS; i++) {
		err = pthread_create(&tids[i], NULL, mythread, NULL);
		if (err) {
				printf("main: pthread_create() failed: %s\n", strerror(err));
			return -1;
		}
    printf("Created thread %lu\n", tids[i]);
	}
// pthread_equal() ???
// что за ---p между стеками тредов
// CLONE_FS, CLONE_TLS ???
	for (int i = 0; i < NUM_THREADS; i++) {
		err = pthread_join(tids[i], NULL);
		if (err) {
			printf("Join error: %s\n", strerror(err));
		}
	}

  sleep(300);

	return 0;
}
