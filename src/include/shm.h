#ifndef SHM_H
#define SHM_H

#define SEM_MUTEX_NAME "/sem_mutex"
#define SHARED_MEM_NAME "/posix_shm"
#define SEM_PRODUCER_NAME "/sem_producer"
#define SEM_CONSUMER_NAME "/sem_consumer"
#define MAX_QUERY_N 4096
#define MAX_STRING 128

#include <pthread.h>

enum QueryType { INSERT, SEARCH, REMOVE, PRINT, PRINT_BUCKET, QUERY_TYPE_SIZE };

typedef struct hashTableQuery {
  enum QueryType type;
  char key[MAX_STRING];
  char value[MAX_STRING];
} hashTableQuery;

struct shared_memory {
  hashTableQuery qs[MAX_QUERY_N];
  int producer_index;
  int consumer_index;
  pthread_rwlock_t mem_lock;
  pthread_rwlockattr_t mem_lock_attr;
};

#endif
