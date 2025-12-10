#define _GNU_SOURCE
#include <pthread.h>
#include <assert.h>
#include <unistd.h>

#include "queue.h"

struct _Queue {
  qnode_t *first;
  qnode_t *last;

  pthread_t qmonitor_tid;

  int count;
  int max_count;

  long add_attempts;
  long get_attempts;
  long add_count;
  long get_count;

  pthread_mutex_t mtx;
  pthread_cond_t not_empty;
  pthread_cond_t not_full;
};


static void *qmonitor(void *arg) {
  queue_t *q = (queue_t *)arg;

  printf("qmonitor: [%d %d %d]\n", getpid(), getppid(), gettid());

  while (1) {
    queue_print_stats(q);
    sleep(1);
  }

  return NULL;
}

queue_t* queue_init(int max_count) {
  queue_t *q = malloc(sizeof(queue_t));
  if (!q) {
    perror("malloc");
    abort();
  }

  q->first = NULL;
  q->last = NULL;
  q->count = 0;
  q->max_count = max_count;

  q->add_attempts = q->get_attempts = 0;
  q->add_count = q->get_count = 0;

  pthread_mutex_init(&q->mtx, NULL);
  pthread_cond_init(&q->not_empty, NULL);
  pthread_cond_init(&q->not_full, NULL);

  int err = pthread_create(&q->qmonitor_tid, NULL, qmonitor, q);
  if (err) {
    fprintf(stderr, "pthread_create: %s\n", strerror(err));
    abort();
  }

  return q;
}

void queue_destroy(queue_t *q) {
  if (!q) return;

  pthread_cancel(q->qmonitor_tid);
  pthread_join(q->qmonitor_tid, NULL);

  pthread_mutex_lock(&q->mtx);

  qnode_t *cur = q->first;
  while (cur) {
    qnode_t *tmp = cur;
    cur = cur->next;
    free(tmp);
  }

  pthread_mutex_unlock(&q->mtx);

  pthread_mutex_destroy(&q->mtx);
  pthread_cond_destroy(&q->not_empty);
  pthread_cond_destroy(&q->not_full);

  free(q);
}

int queue_add(queue_t *q, int val) {
  pthread_mutex_lock(&q->mtx);
  q->add_attempts++;

  while (q->count == q->max_count) {
    pthread_cond_wait(&q->not_full, &q->mtx);
  }

  qnode_t *node = malloc(sizeof(qnode_t));
  if (!node) {
    perror("malloc");
    abort();
  }
  node->val = val;
  node->next = NULL;

  if (!q->first)
    q->first = q->last = node;
  else {
    q->last->next = node;
    q->last = node;
  }

  q->count++;
  q->add_count++;

  pthread_cond_signal(&q->not_empty);
  pthread_mutex_unlock(&q->mtx);

  return 1;
}

int queue_get(queue_t *q, int *val) {
  pthread_mutex_lock(&q->mtx);
  q->get_attempts++;

  while (q->count == 0) {
    pthread_cond_wait(&q->not_empty, &q->mtx);
  }

  qnode_t *tmp = q->first;
  *val = tmp->val;
  q->first = tmp->next;
  if (!q->first)
    q->last = NULL;

  free(tmp);
  q->count--;
  q->get_count++;

  pthread_cond_signal(&q->not_full);
  pthread_mutex_unlock(&q->mtx);

  return 1;
}

void queue_print_stats(queue_t *q) {
  pthread_mutex_lock(&q->mtx);
  printf("queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld %ld %ld)\n",
    q->count,
    q->add_attempts, q->get_attempts, q->add_attempts - q->get_attempts,
    q->add_count, q->get_count, q->add_count - q->get_count);
  pthread_mutex_unlock(&q->mtx);
}
