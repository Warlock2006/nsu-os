#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#define NUM_THREADS 5

void *mythread(void *arg) {
	printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(), gettid());
	return NULL;
}

int main() {
	pthread_t tids[NUM_THREADS];
	int err = 0;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

	for (int i = 0; i < NUM_THREADS; i++) {
		err = pthread_create(&tids[i], NULL, mythread, NULL);
		if (err) {
				printf("main: pthread_create() failed: %s\n", strerror(err));
			return -1;
		}
	}

	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(tids[i], NULL);
	}

	return 0;
}
