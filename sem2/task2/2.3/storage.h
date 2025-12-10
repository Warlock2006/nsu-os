#ifndef STORAGE_H
#define STORAGE_H
#define _GNU_SOURCE

#include <pthread.h>

typedef enum {
    SYNC_MUTEX,
    SYNC_RWLOCK,
    SYNC_SPIN
} sync_type_t;

typedef struct Node {
    char value[100];
    struct Node *next;

    sync_type_t sync_type;
    pthread_mutex_t mutex;
    pthread_rwlock_t rwlock;
    pthread_spinlock_t spinlock;
} Node;

typedef struct Storage {
    Node *first;
    sync_type_t sync_type;
} Storage;

Storage *storage_create(sync_type_t type);
void storage_free(Storage *st);
void storage_push_back(Storage *st, const char *value);
int storage_swap(Storage *st, Node *prev, Node *a, Node *b);
Node *create_node(const char *value, sync_type_t type);

void lock_rnode(Node *n);
void lock_wnode(Node *n);
void unlock_node(Node *n);

extern long ascending_iterations;
extern long descending_iterations;
extern long equal_iterations;
extern long successful_swaps;
extern pthread_mutex_t counters_mtx;

#endif
