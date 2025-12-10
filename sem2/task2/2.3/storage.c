#include "storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdatomic.h>

Node *create_node(const char *value, sync_type_t type) {
  Node *node = malloc(sizeof(Node));
  if (node == NULL) {
    perror("malloc error");
    return NULL;
  }

  strcpy(node->value, value);
  node->sync_type = type;
  switch (type) {
    case SYNC_MUTEX: pthread_mutex_init(&node->mutex, NULL); break;
    case SYNC_RWLOCK: pthread_rwlock_init(&node->rwlock, NULL); break;
    case SYNC_SPIN: pthread_spin_init(&node->spinlock, 0); break;
  }
  node->next = NULL;

  return node;
}

Storage *storage_create(sync_type_t type) {
  Storage *st = malloc(sizeof(Storage));

  if (st == NULL) {
    perror("malloc error");
    return NULL;
  }

  st->first = NULL;
  st->sync_type = type;

  return st;
}

void storage_free(Storage *st) {
  if (st == NULL) {
    return;
  }

  Node *curr = st->first;
  while (curr) {
    Node *tmp = curr;
    curr = curr->next;
    
    switch (tmp->sync_type) {
      case SYNC_MUTEX: pthread_mutex_destroy(&tmp->mutex); break;
      case SYNC_RWLOCK: pthread_rwlock_destroy(&tmp->rwlock); break;
      case SYNC_SPIN: pthread_spin_destroy(&tmp->spinlock); break;
    }

    free(tmp);
  }

  free(st);
}

void storage_push_back(Storage *st, const char *value) {
  if (st == NULL) {
    return;
  }

  Node *new = create_node(value, st->sync_type);
  if (new == NULL) {
    return;
  }

  if (st->first == NULL) {
    st->first = new;
    return;
  }

  Node *curr = st->first;
  while (curr->next) {
    curr = curr->next;
  }

  curr->next = new;
}

int storage_swap(Storage *st, Node *prev, Node *a, Node *b) {
  if (!st || !a || !b) {
    return 0;
  }

  Node *first = prev ? prev : a;
  Node *second = prev ? a : b;
  Node *third = b;

  lock_wnode(first);
  lock_wnode(second);
  lock_wnode(third);

  if ((prev ? prev->next : st->first) != a || a->next != b) {
    unlock_node(third);
    unlock_node(second);
    unlock_node(first);
    return 0;
  }

  if (prev)
    prev->next = b;
  else
    st->first = b;

  a->next = b->next;
  b->next = a;

  unlock_node(third);
  unlock_node(second);
  unlock_node(first);

  return 1;
}

void lock_rnode(Node *n) {
  if (!n) return;
  switch(n->sync_type) {
    case SYNC_MUTEX: pthread_mutex_lock(&n->mutex); break;
    case SYNC_RWLOCK: pthread_rwlock_rdlock(&n->rwlock); break;
    case SYNC_SPIN: pthread_spin_lock(&n->spinlock); break;
  }
}

void lock_wnode(Node *n) {
  if (!n) return;
  switch(n->sync_type) {
    case SYNC_MUTEX: pthread_mutex_lock(&n->mutex); break;
    case SYNC_RWLOCK: pthread_rwlock_wrlock(&n->rwlock); break;
    case SYNC_SPIN: pthread_spin_lock(&n->spinlock); break;
  }
}

void unlock_node(Node *n) {
  if (!n) return;
  switch(n->sync_type) {
    case SYNC_MUTEX: pthread_mutex_unlock(&n->mutex); break;
    case SYNC_RWLOCK: pthread_rwlock_unlock(&n->rwlock); break;
    case SYNC_SPIN: pthread_spin_unlock(&n->spinlock); break;
  }
}
