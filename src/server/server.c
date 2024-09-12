#include <error.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "hashtable.h"
#include "shm.h"

#define N_THREADS 4

sem_t *consumer_sem, *producer_sem;
struct shared_memory *shared_mem_ptr;
int fd_shm;

int consumer_index;
pthread_mutex_t consumer_index_mutex = PTHREAD_MUTEX_INITIALIZER;

HashTable *table;

void startSHM(void) {
  sem_unlink(SEM_PRODUCER_NAME);
  sem_unlink(SEM_CONSUMER_NAME);

  if ((producer_sem = sem_open(SEM_PRODUCER_NAME, O_CREAT, 0660, 0)) ==
      SEM_FAILED) {
    perror("sem_open");
  }

  if ((consumer_sem = sem_open(SEM_CONSUMER_NAME, O_CREAT, 0660, 0)) ==
      SEM_FAILED) {
    perror("sem_open");
  }

  if ((fd_shm = shm_open(SHARED_MEM_NAME, O_RDWR | O_CREAT, 0660)) == -1) {
    perror("shm_open");
  }
  if (ftruncate(fd_shm, sizeof(struct shared_memory)) == -1) {
    perror("ftruncate");
  }

  if ((shared_mem_ptr = mmap(NULL, sizeof(struct shared_memory),
                             PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) ==
      MAP_FAILED) {
    perror("mmap");
  }
  shared_mem_ptr->producer_index = 0;
  consumer_index = 0;

  for (int i = 0; i < N_THREADS; ++i) {
    if (sem_post(producer_sem) == -1)
      perror("sem_post: producer_sem");
  }

  pthread_rwlockattr_init(&shared_mem_ptr->mem_lock_attr);
  pthread_rwlockattr_setpshared(&shared_mem_ptr->mem_lock_attr, 1);
  pthread_rwlock_init(&shared_mem_ptr->mem_lock,
                      &shared_mem_ptr->mem_lock_attr);
}

void cleanSHM(void) {
  sem_unlink(SEM_PRODUCER_NAME);
  sem_unlink(SEM_CONSUMER_NAME);

  // Free resources
  freeHashTable(table);

  sem_close(producer_sem);
  sem_close(consumer_sem);
  munmap(shared_mem_ptr, sizeof(shared_mem_ptr));
}

void executeHashTableQuery(hashTableQuery query) {
  pthread_mutex_lock(table->lock[hash(query.key, table->size)]);
  switch (query.type) {
  case INSERT:
    printf("Inserting key %s with value %s\n", query.key, query.value);
    insertItem(table, query.key, query.value);
    break;
  case SEARCH:
    printf("Searching key %s\n", query.key);
    char *foundValue = searchItem(table, query.key);
    if (foundValue) {
      printf("\tFound value: %s\n", foundValue);
    } else {
      printf("\tElement could not be found\n");
    }
    break;
  case REMOVE:
    printf("Removing key %s\n", query.key);
    int result = removeItem(table, query.key);
    if (result == 0) {
      printf("\tKey %s removed\n", query.key);
    } else {
      printf("\tFailed to remove the key\n");
    }
    break;
  case PRINT:
    printHashTable(table);
    break;
  case PRINT_BUCKET:
    printBucket(table, query.key);
    break;
  default:
    break;
  }

  pthread_mutex_unlock(table->lock[hash(query.key, table->size)]);
}

void *hashTableWorker(void *arg) {
  while (1) {
    // Entering critical section
    if (sem_wait(consumer_sem) == -1)
      perror("sem_wait: consumer_sem");

    pthread_mutex_lock(&consumer_index_mutex);
    int local_consumer_index = consumer_index++;
    pthread_mutex_unlock(&consumer_index_mutex);

    pthread_rwlock_rdlock(&shared_mem_ptr->mem_lock);
    hashTableQuery query =
        shared_mem_ptr->qs[local_consumer_index % MAX_QUERY_N];
    pthread_rwlock_unlock(&shared_mem_ptr->mem_lock);

    if (sem_post(producer_sem) == -1)
      perror("sem_post: producer_sem");

    // Critical section ended
    executeHashTableQuery(query);
  }
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    printf("Usage: %s <size>\n", argv[0]);
    return 0;
  }

  int size = (int)strtoul(argv[1], NULL, 0);
  if (size <= 0) {
    printf("Size must be a positive integer\n");
    return 0;
  }

  table = createHashTable(size);

  startSHM();

  pthread_t thread_id[N_THREADS];
  for (int i = 0; i < N_THREADS; i++)
    pthread_create(&thread_id[i], NULL, hashTableWorker, NULL);

  for (int i = 0; i < N_THREADS; i++)
    pthread_join(thread_id[i], NULL);

  cleanSHM();
  return 0;
}
