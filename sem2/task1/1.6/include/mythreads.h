#ifndef MYTHREADS_H
#define MYTHREADS_H

#include <sys/types.h>

#define MYTHREAD_MAX_THREADS 1024
#define GUARD_PAGE_OFFSET 4096
#define MYTHREAD_STACK_SIZE (2 * 1024 * 1024) 

enum {
  MYTHREADS_FAILURE = 0,
  MYTHREADS_SUCCESS = 1
};

typedef struct mythread *mythread_t;

typedef void *(*mythread_start_routine_t)(void *);

int mythread_create(mythread_t *thread, void *(*__start_routine) (void *), void *arg);
int mythread_join(mythread_t thread, void **retval);
void mythread_exit(void *retval);
mythread_t mythread_self(void);
int mythread_equal(mythread_t t1, mythread_t t2);
int mythread_cancel(mythread_t thread);
void mythread_testcancel(void);
void mythread_cleanup_push(void (*routine)(void *), void *arg);
void mythread_cleanup_pop(int execute);
int mythread_detach(mythread_t thread);
u_int64_t mythread_get_id(mythread_t thread);


#endif