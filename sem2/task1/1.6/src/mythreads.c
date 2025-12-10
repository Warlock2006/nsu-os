#define _GNU_SOURCE
#include "mythreads.h"

#include <sys/syscall.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <sched.h>
#include <stdio.h>
#include <pthread.h>

static __thread mythread_t current_thread = NULL;

static atomic_uint_fast64_t next_thread_id = ATOMIC_VAR_INIT(1);
static atomic_int active_thread_count = ATOMIC_VAR_INIT(0);

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct cleanup_node {
  void (*routine)(void *);
  void *arg;
  struct cleanup_node *next;
} cleanup_node;

typedef struct to_free_node {
  void *ptr;
  size_t size;
  struct to_free_node *next;
} to_free_node;

static to_free_node *to_free = NULL; 

struct mythread {
  u_int64_t id;
  pid_t tid;
  void *stack;
  int joined;
  int detached;
  atomic_int finished;
  atomic_int canceled;
  void *retval;
  void *(*start_routine)(void *);
  void *arg;
  cleanup_node *cleanup_top;
};

static struct mythread main_thread_struct = {0};
static atomic_flag main_thread_initialized = ATOMIC_FLAG_INIT;

static void free_stack(void *stack, size_t size) {
  void *base = NULL;

  if (!stack) return;
  base = (char *)stack - GUARD_PAGE_OFFSET;
  munmap(base, size + GUARD_PAGE_OFFSET);
}

static void free_detached_stacks() {
  pthread_mutex_lock(&mutex);
  if (!to_free) {
    pthread_mutex_unlock(&mutex);
    return;
  }

  to_free_node *current = to_free;
  to_free = NULL;
  pthread_mutex_unlock(&mutex);
  
  while (current) {
    to_free_node *tmp = current;
    munmap(current->ptr, current->size);
    printf("Ptr %p unmapped, size: %lu\n", current->ptr, current->size);
    current = current->next;
    free(tmp);
  }
}

static void mark_to_free(void *ptr, size_t size) {
  pthread_mutex_lock(&mutex);
  if (!to_free) {
    to_free = malloc(sizeof(to_free_node));
    to_free->ptr = ptr;
    to_free->size = size;
    to_free->next = NULL;
    pthread_mutex_unlock(&mutex);
    printf("Ptr %p marked to free\n", ptr);
    return;
  }

  to_free_node *new_node = malloc(sizeof(to_free_node));
  new_node->ptr = ptr;
  new_node->size = size;
  new_node->next = NULL;

  to_free_node *current = to_free;
  while (current->next) {
    current = current->next;
  }
  current->next = new_node;
  printf("Ptr %p marked to free\n", ptr);
  pthread_mutex_unlock(&mutex);
}

static void cleanup_and_free(mythread_t thread) {
  free_stack(thread->stack, MYTHREAD_STACK_SIZE);
  while (thread->cleanup_top) {
    cleanup_node *h = thread->cleanup_top;
    thread->cleanup_top = h->next;
    h->routine(h->arg);
    free(h);
  }
  free(thread);
}

static int thread_start(void *arg) {
  mythread_t t = (mythread_t)arg;
  current_thread = t;
  void *result = t->start_routine(t->arg);
    
  t->retval = result;
  atomic_store(&t->finished, 1);
  atomic_fetch_sub(&active_thread_count, 1);

  if (t->detached) {
    mark_to_free((char *)t->stack - GUARD_PAGE_OFFSET, MYTHREAD_STACK_SIZE + GUARD_PAGE_OFFSET);
    while (t->cleanup_top) {
      cleanup_node *h = t->cleanup_top;
      t->cleanup_top = h->next;
      h->routine(h->arg);
      free(h);
    }
    free(t);
  }
  syscall(SYS_exit, 0);
  return 0;
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
  struct mythread *t = NULL;
  void *stack = NULL;
  void *stack_top = NULL;
  unsigned long flags = 0;
  long tid = 0;
  int count = 0;

  free_detached_stacks();

  if (!thread || !start_routine) {
    return MYTHREADS_FAILURE;
  }

  count = atomic_fetch_add(&active_thread_count, 1);
  if (count >= MYTHREAD_MAX_THREADS) {
    atomic_fetch_sub(&active_thread_count, 1);
    return MYTHREADS_FAILURE;
  }

  t = (struct mythread *)malloc(sizeof(struct mythread));
  if (!t) {
    atomic_fetch_sub(&active_thread_count, 1);
    return MYTHREADS_FAILURE;
  }

  stack = allocate_stack(MYTHREAD_STACK_SIZE);
  if (!stack) {
    free(t);
    atomic_fetch_sub(&active_thread_count, 1);
    return MYTHREADS_FAILURE;
  }
  
  t->id = atomic_fetch_add(&next_thread_id, 1);
  t->stack = stack;
  t->joined = 0;
  t->detached = 0;
  atomic_init(&t->finished, 0);
  atomic_init(&t->canceled, 0);
  t->retval = NULL;
  t->start_routine = start_routine;
  t->arg = arg;
  t->cleanup_top = NULL;

  stack_top = (char *)stack + MYTHREAD_STACK_SIZE;
  flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD | CLONE_SYSVSEM | CLONE_PARENT_SETTID;

  tid = clone(thread_start, stack_top, flags, t, &t->tid, NULL, NULL);
  if (tid < 0) {
    free_stack(stack, MYTHREAD_STACK_SIZE);
    free(t);
    atomic_fetch_sub(&active_thread_count, 1);
    return MYTHREADS_FAILURE;
  }

  t->tid = (pid_t)tid;
  *thread = t;
  return MYTHREADS_SUCCESS;
}

int mythread_join(mythread_t thread, void **retval) {
  if (!thread || thread->joined || thread->detached) {
    return MYTHREADS_FAILURE;
  }

  while (!atomic_load(&thread->finished)) {
    usleep(1000);
  }

  if (retval) {
    *retval = thread->retval;
  }

  thread->joined = 1;
  cleanup_and_free(thread);
  return MYTHREADS_SUCCESS;
}

int mythread_detach(mythread_t thread) {
  if (!thread || thread->joined || thread->detached) {
    return MYTHREADS_FAILURE;
  }
  thread->detached = 1;
  return MYTHREADS_SUCCESS;
}

void mythread_exit(void *retval) {
  mythread_t self = current_thread;

  if (!self) {
    if (!atomic_flag_test_and_set(&main_thread_initialized)) {
      main_thread_struct.id = atomic_fetch_add(&next_thread_id, 1);
      main_thread_struct.tid = syscall(SYS_gettid);
      main_thread_struct.detached = 0;
      main_thread_struct.joined = 0;
      atomic_init(&main_thread_struct.finished, 0);
      atomic_init(&main_thread_struct.canceled, 0);
      main_thread_struct.stack = NULL;
      main_thread_struct.cleanup_top = NULL;
    }
    self = &main_thread_struct;
    current_thread = self;
  }

  self->retval = retval;
  atomic_store(&self->finished, 1);
  atomic_fetch_sub(&active_thread_count, 1);

  while (self->cleanup_top) {
    cleanup_node *h = self->cleanup_top;
    self->cleanup_top = h->next;
    h->routine(h->arg);
    free(h);
  }

  if (self->detached) {
    if (self->stack) {
      free_stack(self->stack, MYTHREAD_STACK_SIZE);
    }
    if (self != &main_thread_struct) {
      free(self);
    }
  }

  syscall(SYS_exit, 0);
}

mythread_t mythread_self(void) {
  if (syscall(SYS_gettid) == getpid()) {
    if (!atomic_flag_test_and_set(&main_thread_initialized)) {
      main_thread_struct.id = atomic_fetch_add(&next_thread_id, 1);
      main_thread_struct.tid = getpid();
      main_thread_struct.detached = 0;
      main_thread_struct.joined = 0;
      atomic_init(&main_thread_struct.finished, 0);
      atomic_init(&main_thread_struct.canceled, 0);
      main_thread_struct.stack = NULL;
      main_thread_struct.cleanup_top = NULL;
    }
    return &main_thread_struct;
  }
  return current_thread;
}

int mythread_equal(mythread_t t1, mythread_t t2) {
  if (!t1 || !t2) return 0;
  return (t1->id == t2->id);
}

int mythread_cancel(mythread_t thread) {
  if (!thread) return MYTHREADS_FAILURE;
  atomic_store(&thread->canceled, 1);
  return MYTHREADS_SUCCESS;
}

void mythread_testcancel(void) {
  mythread_t self = mythread_self();
  
  if (atomic_load(&self->canceled)) {
    mythread_exit((void*)(-1));
  }
}

u_int64_t mythread_get_id(mythread_t thread) {
  if (!thread) {
    return 0;
  }
  return thread->id;
}

void mythread_cleanup_push(void (*routine)(void *), void *arg) {
  mythread_t self = mythread_self();
  cleanup_node *node = (cleanup_node *)malloc(sizeof(cleanup_node));
  node->routine = routine;
  node->arg = arg;
  node->next = self->cleanup_top;
  self->cleanup_top = node;
}

void mythread_cleanup_pop(int execute) {
  mythread_t self = mythread_self();
  if (self->cleanup_top) {
    cleanup_node *node = self->cleanup_top;
    self->cleanup_top = node->next;
    if (execute) {
      node->routine(node->arg);
    }
    free(node);
  }
}