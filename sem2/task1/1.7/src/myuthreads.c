#define _GNU_SOURCE
#include "myuthreads.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <ucontext.h>
#include <unistd.h>
#include <errno.h>
#include <stdatomic.h>

static atomic_uint_fast64_t next_thread_id = ATOMIC_VAR_INIT(1);

typedef enum {
  THREAD_READY,
  THREAD_RUNNING,
  THREAD_BLOCKED,
  THREAD_FINISHED
} thread_state;

typedef struct cleanup_node {
  void (*routine)(void *);
  void *arg;
  struct cleanup_node *next;
} cleanup_node;

struct mythread {
  u_int64_t id;
  void *stack;
  size_t stack_size;
  ucontext_t context;
  thread_state state;
  void *(*start_routine)(void *);
  void *arg;
  void *retval;
  int detached;
  int joined;
  atomic_flag canceled;
  cleanup_node *cleanup_top;
  struct mythread *next;
};

static void free_stack(void *stack, size_t size) {
  void *base = NULL;

  if (!stack) return;
  base = (char *)stack - GUARD_PAGE_OFFSET;
  munmap(base, size + GUARD_PAGE_OFFSET);
}

static void *allocate_stack(size_t size) {
  size_t total = size + GUARD_PAGE_OFFSET;
  void *mem = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
  
  if (mem == MAP_FAILED) {
    return NULL;
  }

  if (mprotect(mem, GUARD_PAGE_OFFSET, PROT_NONE) != 0) {
    munmap(mem, total);
    return NULL;
  }

  return (char *)mem + GUARD_PAGE_OFFSET;
}

int mythread_create(mythread_t *thread, void *(*start_routine)(void *), void *arg) {
  void *stack = NULL;
  mythread_t t = NULL;

  if (!thread || ! start_routine) {
    return MY_USER_THREAD_FAILURE;
  }

  stack = allocate_stack(MYUTHREAD_STACK_SIZE);
  if (!stack) {
    return MY_USER_THREAD_FAILURE;
  }

  t = malloc(sizeof(mythread_t));
  if (!t) {
    free_stack(stack, MYUTHREAD_STACK_SIZE);
    return MY_USER_THREAD_FAILURE;
  }
  t->id = atomic_fetch_add(&next_thread_id, 1);
  t->stack = stack;
  t->stack_size = MYUTHREAD_STACK_SIZE;
  t->start_routine = start_routine;
  t->arg = arg;
  t->canceled.__val = 0;
  t->cleanup_top = NULL;


  return MY_USER_THREAD_SUCCESS;
}
