#define _GNU_SOURCE
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
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

  pthread_mutex_t mutex;
  sem_t items;
  sem_t slots;
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
  int err;
  queue_t *q = malloc(sizeof(queue_t));
  if (!q) {
    printf("Cannot allocate memory for a queue\n");
    abort();
  }

  q->first = q->last = NULL;
  q->max_count = max_count;
  q->count = 0;
  q->add_attempts = q->get_attempts = 0;
  q->add_count = q->get_count = 0;

  pthread_mutex_init(&q->mutex, NULL);
  sem_init(&q->items, 0, 0);
  sem_init(&q->slots, 0, max_count);

  err = pthread_create(&q->qmonitor_tid, NULL, qmonitor, q);
  if (err) {
    printf("queue_init: pthread_create() failed: %s\n", strerror(err));
    abort();
  }

  return q;
}

void queue_destroy(queue_t *q) {
  if (!q) return;

  qnode_t *cur = q->first;
  while (cur) {
    qnode_t *tmp = cur;
    cur = cur->next;
    free(tmp);
  }

  pthread_cancel(q->qmonitor_tid);
  pthread_join(q->qmonitor_tid, NULL);

  pthread_mutex_destroy(&q->mutex);
  sem_destroy(&q->items);
  sem_destroy(&q->slots);
  free(q);
}

int queue_add(queue_t *q, int val) {
  q->add_attempts++;

  sem_wait(&q->slots);

  qnode_t *node = malloc(sizeof(qnode_t));
  if (!node) {
    printf("Cannot allocate memory for new node\n");
    abort();
  }
  node->val = val;
  node->next = NULL;

  pthread_mutex_lock(&q->mutex);
  if (!q->first)
    q->first = q->last = node;
  else {
    q->last->next = node;
    q->last = node;
  }
  q->count++;
  q->add_count++;
  pthread_mutex_unlock(&q->mutex);

  sem_post(&q->items);
  return 1;
}

int queue_get(queue_t *q, int *val) {
  q->get_attempts++;

  sem_wait(&q->items);

  pthread_mutex_lock(&q->mutex);
  qnode_t *tmp = q->first;
  *val = tmp->val;
  q->first = tmp->next;
  if (!q->first)
    q->last = NULL;
  q->count--;
  q->get_count++;
  pthread_mutex_unlock(&q->mutex);

  free(tmp);
  sem_post(&q->slots);
  return 1;
}

void queue_print_stats(queue_t *q) {
  pthread_mutex_lock(&q->mutex);
  printf("queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld %ld %ld)\n",
    q->count,
    q->add_attempts, q->get_attempts, q->add_attempts - q->get_attempts,
    q->add_count, q->get_count, q->add_count - q->get_count);
  pthread_mutex_unlock(&q->mutex);
}

