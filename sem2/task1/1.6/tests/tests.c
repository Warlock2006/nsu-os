#define _GNU_SOURCE
#include "mythreads.h"
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

void cleanup_func1(void *arg) {
  int *counter = (int*)arg;
  (*counter)++;
}

void cleanup_func2(void *arg) {
  *(int*)arg = 42;
}

void *worker_id(void *arg) {
  (void)arg;
  return arg;
}

void *worker_detach(void *arg) {
  int *finished = (int*)arg;
  printf("DEBUG: worker_detach started\n");
  *finished = 1;
  printf("DEBUG: worker_detach finished\n");
  return (void*)123;
}

void *worker_cancellable(void *arg) {
  int *flag = (int*)arg;
  printf("DEBUG: worker_cancellable started\n");
  for (int i = 0; i < 100; i++) {
    usleep(1000);
    mythread_testcancel();
  }
  *flag = 1;
  printf("DEBUG: worker_cancellable finished\n");
  return NULL;
}

void *worker_cleanup(void *arg) {
  int *counter = (int*)arg;
  printf("DEBUG: worker_cleanup started\n");
  mythread_cleanup_push(cleanup_func1, counter);
  mythread_cleanup_push(cleanup_func2, counter);
  mythread_exit((void*)777);
  return NULL;
}

int test_basic() {
  printf("DEBUG: Starting test_basic\n");
  const int N = 10;
  mythread_t threads[N];
  uint64_t ids[N];

  for (int i = 0; i < N; i++) {
    int err = mythread_create(&threads[i], worker_id, (void*)(long)i);
    if (err != MYTHREADS_SUCCESS) {
      printf("DEBUG: test_basic failed at create %d\n", i);
      return 1;
    }
    ids[i] = mythread_get_id(threads[i]);
  }

  for (int i = 0; i < N; i++) {
    for (int j = i + 1; j < N; j++) {
      if (ids[i] == ids[j]) {
        printf("DEBUG: test_basic failed: duplicate IDs %llu\n", (unsigned long long)ids[i]);
        return 1;
      }
      if (mythread_equal(threads[i], threads[j])) {
        printf("DEBUG: test_basic failed: equal threads %d and %d\n", i, j);
        return 1;
      }
    }
  }

  mythread_t main_thr = mythread_self();
  if (mythread_get_id(main_thr) == 0) {
    printf("DEBUG: test_basic failed: main thread ID is 0\n");
    return 1;
  }
  if (mythread_equal(threads[0], main_thr)) {
    printf("DEBUG: test_basic failed: worker equals main\n");
    return 1;
  }

  for (int i = 0; i < N; i++) {
    void *ret;
    int err = mythread_join(threads[i], &ret);
    if (err != MYTHREADS_SUCCESS) {
      printf("DEBUG: test_basic failed at join %d\n", i);
      return 1;
    }
    if (ret != (void*)(long)i) {
      printf("DEBUG: test_basic failed: wrong return value %d\n", i);
      return 1;
    }
  }

  printf("\tBasic functionality PASSED\n");
  return 0;
}

int test_detach() {
  printf("DEBUG: Starting test_detach\n");
  mythread_t t;
  int finished = 0;
  finished = 0;

  int err = mythread_create(&t, worker_detach, (void*)&finished);
  if (err != MYTHREADS_SUCCESS) {
    printf("DEBUG: test_detach failed at create\n");
    return 1;
  }

  err = mythread_detach(t);
  printf("Thread detached!\n");
  if (err != MYTHREADS_SUCCESS) {
    printf("DEBUG: test_detach failed at detach\n");
    return 1;
  }

  printf("DEBUG: test_detach waiting for worker to finish\n");
  while (!finished) {
    usleep(1000);
  }
  printf("DEBUG: test_detach worker finished\n");

  printf("\tDetach PASSED\n");
  return 0;
}

int test_cancel() {
  printf("DEBUG: Starting test_cancel\n");
  mythread_t t;
  int flag = 0;

  int err = mythread_create(&t, worker_cancellable, &flag);
  if (err != MYTHREADS_SUCCESS) {
    printf("DEBUG: test_cancel failed at create\n");
    return 1;
  }

  usleep(10000);
  mythread_cancel(t);
  printf("DEBUG: test_cancel sent cancel request\n");

  void *ret;
  err = mythread_join(t, &ret);
  if (err != MYTHREADS_SUCCESS) {
    printf("DEBUG: test_cancel failed at join\n");
    return 1;
  }
  if (flag != 0) {
    printf("DEBUG: test_cancel failed: thread was not canceled\n");
    return 1;
  }
  if (ret != (void*)-1) {
    printf("DEBUG: test_cancel failed: wrong return value\n");
    return 1;
  }

  printf("\tCancel PASSED\n");
  return 0;
}

int test_cleanup() {
  printf("DEBUG: Starting test_cleanup\n");
  mythread_t t;
  int counter = 0;

  int err = mythread_create(&t, worker_cleanup, &counter);
  if (err != MYTHREADS_SUCCESS) {
    printf("DEBUG: test_cleanup failed at create\n");
    return 1;
  }

  void *ret;
  err = mythread_join(t, &ret);
  if (err != MYTHREADS_SUCCESS) {
    printf("DEBUG: test_cleanup failed at join\n");
    return 1;
  }
  if (counter != 43) {
    printf("DEBUG: test_cleanup failed: counter = %d\n", counter);
    return 1;
  }
  if (ret != (void*)777) {
    printf("DEBUG: test_cleanup failed: wrong return value\n");
    return 1;
  }

  printf("\tCleanup handlers PASSED\n");
  return 0;
}

int test_errors() {
  printf("DEBUG: Starting test_errors\n");
  int err = mythread_create(NULL, NULL, NULL);
  if (err == MYTHREADS_SUCCESS) {
    printf("DEBUG: test_errors failed: create with NULL should fail\n");
    return 1;
  }

  err = mythread_join(NULL, NULL);
  if (err == MYTHREADS_SUCCESS) {
    printf("DEBUG: test_errors failed: join with NULL should fail\n");
    return 1;
  }

  err = mythread_detach(NULL);
  if (err == MYTHREADS_SUCCESS) {
    printf("DEBUG: test_errors failed: detach with NULL should fail\n");
    return 1;
  }

  mythread_t t;
  err = mythread_create(&t, worker_id, NULL);
  if (err != MYTHREADS_SUCCESS) {
    printf("DEBUG: test_errors failed: valid create failed\n");
    return 1;
  }

  err = mythread_detach(t);
  if (err != MYTHREADS_SUCCESS) {
    printf("DEBUG: test_errors failed: first detach failed\n");
    return 1;
  }

  err = mythread_detach(t);
  if (err == MYTHREADS_SUCCESS) {
    printf("DEBUG: test_errors failed: second detach should fail\n");
    return 1;
  }

  err = mythread_join(t, NULL);
  if (err == MYTHREADS_SUCCESS) {
    printf("DEBUG: test_errors failed: join after detach should fail\n");
    return 1;
  }

  printf("\tError handling PASSED\n");
  return 0;
}

int main() {

  if (test_basic()) return 1;
  if (test_detach()) return 1;
  if (test_cancel()) return 1;
  if (test_cleanup()) return 1;
  if (test_errors()) return 1;

  return 0;
}