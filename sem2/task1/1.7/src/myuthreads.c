#define _GNU_SOURCE
#include "myuthreads.h"

#include <ucontext.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <sched.h>
#include <stdio.h>

#define WORKER_COUNT 4

typedef enum {
  UTHREAD_READY,
  UTHREAD_RUNNING,
  UTHREAD_FINISHED
} uthread_state_t;

typedef struct cleanup_node {
  void (*routine)(void *);
  void *arg;
  struct cleanup_node *next;
} cleanup_node_t;

struct mythread {
  uint64_t id;

  ucontext_t ctx;
  void *stack;

  atomic_int state;
  void *retval;

  atomic_int canceled;
  atomic_int detached;
  atomic_int joined;

  cleanup_node_t *cleanup_stack;

  void *(*start_routine)(void *);
  void *arg;

  struct mythread *next;
};

static atomic_uint_fast64_t next_id = ATOMIC_VAR_INIT(1);

static mythread_t run_queue = NULL;
static pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queue_cv = PTHREAD_COND_INITIALIZER;

static __thread mythread_t current = NULL;
static __thread ucontext_t scheduler_ctx;

static pthread_once_t runtime_once = PTHREAD_ONCE_INIT;

static void enqueue(mythread_t t) {
  pthread_mutex_lock(&queue_lock);
  t->next = run_queue;
  run_queue = t;
  pthread_cond_signal(&queue_cv);
  pthread_mutex_unlock(&queue_lock);
}

static mythread_t dequeue(void) {
  pthread_mutex_lock(&queue_lock);
  while (!run_queue)
    pthread_cond_wait(&queue_cv, &queue_lock);

  mythread_t t = run_queue;
  run_queue = t->next;
  pthread_mutex_unlock(&queue_lock);
  return t;
}

static void run_cleanups(mythread_t t) {
  while (t->cleanup_stack) {
    cleanup_node_t *n = t->cleanup_stack;
    t->cleanup_stack = n->next;
    n->routine(n->arg);
    free(n);
  }
}

static void uthread_entry(void) {
  mythread_t t = current;

  if (atomic_load(&t->canceled))
    mythread_exit((void *)-1);

  void *ret = t->start_routine(t->arg);
  mythread_exit(ret);
}

static void *worker_loop(void *arg) {
  (void)arg;

  while (1) {
    mythread_t t = dequeue();

    if (atomic_load(&t->state) == UTHREAD_FINISHED) {
      if (atomic_load(&t->detached)) {
        free(t->stack);
        free(t);
      }
      continue;
    }

    current = t;
    atomic_store(&t->state, UTHREAD_RUNNING);

    swapcontext(&scheduler_ctx, &t->ctx);

    if (atomic_load(&t->state) == UTHREAD_RUNNING) {
      atomic_store(&t->state, UTHREAD_READY);
      enqueue(t);
    }
  }
  return NULL;
}

static void runtime_init(void) {
  pthread_t workers[WORKER_COUNT];

  for (int i = 0; i < WORKER_COUNT; i++) {
    pthread_create(&workers[i], NULL, worker_loop, NULL);
    pthread_detach(workers[i]);
  }
}

int mythread_create(mythread_t *thread, void *(*start_routine)(void *), void *arg) {
  if (!thread || !start_routine)
    return MY_USER_THREAD_FAILURE;

  pthread_once(&runtime_once, runtime_init);

  mythread_t t = calloc(1, sizeof(*t));
  if (!t)
    return MY_USER_THREAD_FAILURE;

  t->id = atomic_fetch_add(&next_id, 1);
  t->stack = malloc(MYUTHREAD_STACK_SIZE);
  if (!t->stack) {
    free(t);
    return MY_USER_THREAD_FAILURE;
  }

  atomic_init(&t->state, UTHREAD_READY);
  atomic_init(&t->canceled, 0);
  atomic_init(&t->detached, 0);
  atomic_init(&t->joined, 0);

  t->start_routine = start_routine;
  t->arg = arg;

  getcontext(&t->ctx);
  t->ctx.uc_stack.ss_sp = t->stack;
  t->ctx.uc_stack.ss_size = MYUTHREAD_STACK_SIZE;
  t->ctx.uc_link = &scheduler_ctx;
  makecontext(&t->ctx, uthread_entry, 0);

  enqueue(t);
  *thread = t;
  return MY_USER_THREAD_SUCCESS;
}

void mythread_exit(void *retval) {
  mythread_t t = current;

  t->retval = retval;
  atomic_store(&t->state, UTHREAD_FINISHED);

  run_cleanups(t);

  swapcontext(&t->ctx, &scheduler_ctx);
}

void mythread_yield(void) {
  mythread_t t = current;

  atomic_store(&t->state, UTHREAD_READY);
  enqueue(t);
  swapcontext(&t->ctx, &scheduler_ctx);
}

mythread_t mythread_self(void) {
  return current;
}

int mythread_equal(mythread_t t1, mythread_t t2) {
  return t1 && t2 && t1->id == t2->id;
}

uint64_t mythread_get_id(mythread_t thread) {
  return thread ? thread->id : 0;
}

int mythread_join(mythread_t thread, void **retval) {
  if (!thread || atomic_load(&thread->detached))
    return MY_USER_THREAD_FAILURE;

  int expected = 0;
  if (!atomic_compare_exchange_strong(&thread->joined, &expected, 1))
    return MY_USER_THREAD_FAILURE;

  while (atomic_load(&thread->state) != UTHREAD_FINISHED)
    sched_yield();

  if (retval)
    *retval = thread->retval;

  free(thread->stack);
  free(thread);
  return MY_USER_THREAD_SUCCESS;
}

int mythread_detach(mythread_t thread) {
  if (!thread || thread->detached)
    return MY_USER_THREAD_FAILURE;

  atomic_store(&thread->detached, 1);
  return MY_USER_THREAD_SUCCESS;
}

int mythread_cancel(mythread_t thread) {
  if (!thread)
    return MY_USER_THREAD_FAILURE;

  atomic_store(&thread->canceled, 1);
  return MY_USER_THREAD_SUCCESS;
}

void mythread_testcancel(void) {
  if (current && atomic_load(&current->canceled))
    mythread_exit((void *)-1);
}

void mythread_cleanup_push(void (*routine)(void *), void *arg) {
  cleanup_node_t *n = malloc(sizeof(*n));
  n->routine = routine;
  n->arg = arg;
  n->next = current->cleanup_stack;
  current->cleanup_stack = n;
}

void mythread_cleanup_pop(int execute) {
  cleanup_node_t *n = current->cleanup_stack;
  if (!n)
    return;

  current->cleanup_stack = n->next;
  if (execute)
    n->routine(n->arg);
  free(n);
}
