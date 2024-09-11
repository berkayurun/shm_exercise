#include <error.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "shm.h"

int main(int argc, char **argv) {
  struct shared_memory *shared_mem_ptr;
  sem_t *mutex_sem, *producer_sem, *consumer_sem;
  int fd_shm;

  if ((mutex_sem = sem_open(SEM_MUTEX_NAME, 0, 0, 0)) == SEM_FAILED)
    perror("sem_open");

  if ((producer_sem = sem_open(SEM_PRODUCER_NAME, O_CREAT, 0660, 0)) ==
      SEM_FAILED)
    perror("sem_open");

  if ((consumer_sem = sem_open(SEM_CONSUMER_NAME, O_CREAT, 0660, 0)) ==
      SEM_FAILED)
    perror("sem_open");

  if ((fd_shm = shm_open(SHARED_MEM_NAME, O_RDWR | O_CREAT, 0660)) == -1) {
    perror("shm_open");
    exit(1);
  }

  if ((shared_mem_ptr = mmap(NULL, sizeof(struct shared_memory),
                             PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) ==
      MAP_FAILED)
    perror("mmap");

  while (1) {
    // Prepare a random query
    hashTableQuery *query = (hashTableQuery *)malloc(sizeof(hashTableQuery));

    // Set a random type according to QueryType
    query->type = rand() % QUERY_TYPE_SIZE;

    // Set random keys as string
    sprintf(query->key, "%d", rand());
    sprintf(query->value, "%d", rand());

    // Enter critical section
    if (sem_wait(producer_sem) == -1)
      perror("sem_wait: producer_sem");

    if (sem_wait(mutex_sem) == -1)
      perror("sem_wait:mutex");

    memcpy(&shared_mem_ptr->qs[shared_mem_ptr->producer_index % MAX_QUERY_N],
           query, sizeof(hashTableQuery));

    (shared_mem_ptr->producer_index)++;

    if (sem_post(mutex_sem) == -1)
      perror("sem_post:mutex");

    if (sem_post(consumer_sem) == -1)
      perror("sem_post:consumer_sem");
    // Exit critical section

    free(query);
    // For mimicking real worl use case
    sleep(3);
  }

  // Free resources
  sem_close(mutex_sem);
  sem_close(producer_sem);
  sem_close(consumer_sem);
}
