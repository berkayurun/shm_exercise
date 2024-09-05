#include <error.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "shm.h"

int opt;
char *key = NULL;
char *value = NULL;
enum QueryType qtype;

void parseUserInput(int argc, char **argv) {
  if (argc == 1) {
    fprintf(stderr, "Usage:\n"
                    "-i [key] [value]\t Insert\n"
                    "-r [key] \t\t Remove\n"
                    "-s [key] \t\t Search\n"
                    "-p  \t\t\t Print table\n"
                    "-b [index] \t\t Print bucket by index\n");

    exit(0);
  }

  // Parse options: i for insert, r for remove, s for search, p for print
  while ((opt = getopt(argc, argv, "b:i:r:s:p")) != -1) {
    switch (opt) {
    case 'i': // Insert option: expects two strings
      qtype = INSERT;
      key = optarg; // First string is the key
      if (optind < argc && argv[optind][0] != '-') {
        value = argv[optind++]; // Second string is the value
      } else {
        fprintf(stderr, "Option -i requires two arguments: key and value.\n");
        exit(0);
      }
      break;
    case 'r': // Remove option: expects one string
      qtype = REMOVE;
      key = optarg; // Get the key to remove
      break;
    case 's': // Search option: expects one string
      qtype = SEARCH;
      key = optarg; // Get the key to search
      break;
    case 'p': // Print option: does not require an argument
      qtype = PRINT;
      break;
    case 'b': // Print option: does not require an argument
      qtype = PRINT_BUCKET;
      key = optarg; // Get the key of the bucket
      break;
    case '?': // Handle unknown options or missing arguments
      if (optopt == 'i' || optopt == 'r' || optopt == 's') {
        fprintf(stderr, "Option -%c requires an argument.\n", optopt);
      } else {
        fprintf(stderr, "Usage:\n"
                        "-i [key] [value]\t Insert\n"
                        "-r [key] \t\t Remove\n"
                        "-s [key] \t\t Search\n"
                        "-p \t\t\t Print table\n"
                        "-b [index] \t\t Print bucket by index\n");
      }
      exit(0);
    default:
      exit(-1);
    }
  }
}

int main(int argc, char **argv) {
  parseUserInput(argc, argv);

  struct shared_memory *shared_mem_ptr;
  sem_t *mutex_sem, *producer_sem, *consumer_sem;
  int fd_shm;

  if ((mutex_sem = sem_open(SEM_MUTEX_NAME, 0, 0, 0)) == SEM_FAILED) {
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
    exit(1);
  }
  if ((shared_mem_ptr = mmap(NULL, sizeof(struct shared_memory),
                             PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) ==
      MAP_FAILED) {
    perror("mmap");
  }

  hashTableQuery *query = (hashTableQuery *)malloc(sizeof(hashTableQuery));

  // Set a type according to arguments
  query->type = qtype;

  if (key)
    memcpy(query->key, key, MIN(strlen(key), MAX_STRING));
  if (value)
    memcpy(query->value, value, MIN(strlen(value), MAX_STRING));

  // Enter critical section
  if (sem_wait(producer_sem) == -1)
    perror("sem_wait: producer_sem");

  if (sem_wait(mutex_sem) == -1)
    perror("sem_wait:mutex");

  memcpy(&shared_mem_ptr->qs[shared_mem_ptr->producer_index], query,
         sizeof(hashTableQuery));

  (shared_mem_ptr->producer_index)++;

  if (sem_post(mutex_sem) == -1)
    perror("sem_post:mutex");

  if (sem_post(consumer_sem) == -1)
    perror("sem_post:consumer_sem");
  // Exit critical section

  // Free resources
  free(query);
  sem_close(mutex_sem);
  sem_close(producer_sem);
  sem_close(consumer_sem);
}
