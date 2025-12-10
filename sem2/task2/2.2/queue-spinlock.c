#define _GNU_SOURCE
#include <pthread.h>
#include <assert.h>
#include <pthread.h>

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

	pthread_spinlock_t spinlock;
};


void *qmonitor(void *arg) {
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



	q->first = NULL;
	q->last = NULL;
	q->max_count = max_count;
	q->count = 0;

	q->add_attempts = q->get_attempts = 0;
	q->add_count = q->get_count = 0;

	pthread_spin_init(&q->spinlock, 0);

	err = pthread_create(&q->qmonitor_tid, NULL, qmonitor, q);
	if (err) {
		printf("queue_init: pthread_create() failed: %s\n", strerror(err));
		abort();
	}

	return q;
}

void queue_destroy(queue_t *q) {
	int err = 0;

	if (!q) return;

	if (q->count == 0) {
		err = pthread_cancel(q->qmonitor_tid);
		if (err) {
			perror("cancel error");
			return;
		}

		err = pthread_join(q->qmonitor_tid, NULL);
		if (err) {
			perror("join error");
			return;
		}

		pthread_spin_destroy(&q->spinlock);
		free(q);
		return;
	}

	qnode_t *current = q->first;
	while (current) {
		qnode_t *tmp = current;
		free(tmp);
		current = current->next;
	}

	err = pthread_cancel(q->qmonitor_tid);
	if (err) {
		perror("cancel error");
		return;
	}

	err = pthread_join(q->qmonitor_tid, NULL);
	if (err) {
		perror("join error");
		return;
	}

	pthread_spin_destroy(&q->spinlock);
	free(q);
}

int queue_add(queue_t *q, int val) {
	pthread_spin_lock(&q->spinlock);
	q->add_attempts++;

	if (q->count == q->max_count) {
    pthread_spin_unlock(&q->spinlock);
    return 0;
  }

	qnode_t *new = malloc(sizeof(qnode_t));
	if (!new) {
		printf("Cannot allocate memory for new node\n");
		abort();
	}

	new->val = val;
	new->next = NULL;

	if (!q->first)
		q->first = q->last = new;
	else {
		q->last->next = new;
		q->last = q->last->next;
	}

	q->count++;
	q->add_count++;

	pthread_spin_unlock(&q->spinlock);
	return 1;
}

int queue_get(queue_t *q, int *val) {
	pthread_spin_lock(&q->spinlock);
	q->get_attempts++;

	if (q->count == 0){
		pthread_spin_unlock(&q->spinlock);
		return 0;
	}

	qnode_t *tmp = q->first;

	*val = tmp->val;
	q->first = q->first->next;

	free(tmp);
	q->count--;
	q->get_count++;

	pthread_spin_unlock(&q->spinlock);
	return 1;
}

void queue_print_stats(queue_t *q) {
	pthread_spin_lock(&q->spinlock);

	printf("queue stats: current size %d; attempts: (%ld %ld %ld); counts (%ld %ld %ld)\n",
		q->count,
		q->add_attempts, q->get_attempts, q->add_attempts - q->get_attempts,
		q->add_count, q->get_count, q->add_count -q->get_count);
	
	pthread_spin_unlock(&q->spinlock);
}

