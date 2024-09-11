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

sem_t *mutex_sem;
sem_t *producer_sem;
sem_t *consumer_sem;
struct shared_memory *shared_mem_ptr;
int fd_shm;

HashTable *table;

void startSHM(void) {
  sem_unlink(SEM_MUTEX_NAME);
  sem_unlink(SEM_PRODUCER_NAME);
  sem_unlink(SEM_CONSUMER_NAME);
  if ((mutex_sem = sem_open(SEM_MUTEX_NAME, O_CREAT, 0660, 0)) == SEM_FAILED) {
    perror("sem_open");
  }

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
  shared_mem_ptr->consumer_index = 0;

  // Increment so the client can start
  if (sem_post(mutex_sem) == -1)
    perror("sem_post: mutex_sem");

  if (sem_post(producer_sem) == -1)
    perror("sem_post: mutex_sem");
}

void cleanSHM(void) {
  sem_unlink(SEM_MUTEX_NAME);
  sem_unlink(SEM_PRODUCER_NAME);
  sem_unlink(SEM_CONSUMER_NAME);

  // Free resources
  freeHashTable(table);
  sem_close(mutex_sem);
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

    if (sem_wait(mutex_sem) == -1)
      perror("sem_wait:mutex");

    hashTableQuery query =
        shared_mem_ptr->qs[shared_mem_ptr->consumer_index % MAX_QUERY_N];
    (shared_mem_ptr->consumer_index)++;

    if (sem_post(mutex_sem) == -1)
      perror("sem_post: mutex_sem");

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
