#include "hashtable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HashTable *createHashTable(int size) {
  HashTable *table = (HashTable *)malloc(sizeof(HashTable));
  table->lock = (pthread_mutex_t **)calloc(size, sizeof(pthread_mutex_t));
  for (int i = 0; i < size; i++) {
    table->lock[i] = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(table->lock[i], NULL);
  }
  table->size = size;
  table->count = 0;
  table->items = (HashTableItem **)calloc(table->size, sizeof(HashTableItem *));
  return table;
}

HashTableItem *createHashTableItem(char *key, char *value) {
  HashTableItem *item = (HashTableItem *)malloc(sizeof(HashTableItem));
  item->value = (char *)malloc(strlen(value) + 1);
  strcpy(item->value, value);
  item->key = (char *)malloc(strlen(key) + 1);
  strcpy(item->key, key);
  item->next = NULL;
  return item;
}

void freeHashTable(HashTable *table) {
  for (int i = 0; i < table->size; i++) {
    HashTableItem *item = table->items[i];
    while (item != NULL) {
      HashTableItem *next = item->next;
      free(item->value);
      free(item->key);
      free(item);
      item = next;
    }
  }
  for (int i = 0; i < table->size; i++) {
    pthread_mutex_destroy(table->lock[i]);
    free(table->lock[i]);
  }
  free(table->lock);
  free(table->items);
  free(table);
}

void printBucket(HashTable *table, char *key) {
  // Key is the user given index in this case
  int index = (int)strtoul(key, NULL, 0);
  if (index <= 0) {
    printf("Size must be a positive integer\n"
           "\tIgnoring the command...\n");
    return;
  }
  if (index >= table->size) {
    printf("Out of bounds access to hashtable buckets\n"
           "\tIgnoring the command...\n");
    return;
  }
  HashTableItem *item = table->items[index];
  printf("--------------------------\n");
  printf("Bucket %d: ", index);
  while (item != NULL) {
    printf("%s=%s ", item->key, item->value);
    item = item->next;
  }
  printf("\n--------------------------\n");
  return;
}

void printHashTable(HashTable *table) {
  printf("--------------------------\n");
  for (int i = 0; i < table->size; i++) {
    HashTableItem *item = table->items[i];
    printf("Index %d: ", i);
    while (item != NULL) {
      printf("%s=%s ", item->key, item->value);
      item = item->next;
    }
    printf("\n");
  }
  printf("--------------------------\n");
}

int hash(char *key, int size) {
  int hash = 0;
  for (int i = 0; i < strlen(key); i++) {
    hash += key[i];
  }
  return hash % size;
}

void insertItem(HashTable *table, char *key, char *value) {
  int index = hash(key, table->size);
  HashTableItem *item = table->items[index];
  if (item == NULL) {
    table->items[index] = createHashTableItem(key, value);
    table->count++;
  } else {
    while (item->next != NULL) {
      item = item->next;
    }
    item->next = createHashTableItem(key, value);
    table->count++;
  }
}

char *searchItem(HashTable *table, char *key) {
  int index = hash(key, table->size);
  HashTableItem *item = table->items[index];
  while (item != NULL) {
    if (strcmp(item->key, key) == 0) {
      return item->value;
    }
    item = item->next;
  }
  return NULL;
}

// Return non zero on failure
int removeItem(HashTable *table, char *key) {
  int index = hash(key, table->size);
  HashTableItem *item = table->items[index];
  HashTableItem *prev = NULL;
  while (item != NULL) {
    if (strcmp(item->key, key) == 0) {
      if (prev == NULL) {
        table->items[index] = item->next;
      } else {
        prev->next = item->next;
      }
      free(item->value);
      free(item->key);
      free(item);
      table->count--;
      return 0;
    }
    prev = item;
    item = item->next;
  }
  return 1;
}
