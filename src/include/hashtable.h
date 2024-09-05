#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <pthread.h>

typedef struct HashTableItem {
  char *value;
  char *key;
  struct HashTableItem *next;
} HashTableItem;

typedef struct HashTable {
  HashTableItem **items;
  pthread_mutex_t **lock;
  int size;
  int count;
} HashTable;

HashTable *createHashTable(int size);
HashTableItem *createHashTableItem(char *key, char *value);
void freeHashTable(HashTable *table);

void printHashTable(HashTable *table);
void printBucket(HashTable *table, char *key);

// Simple hash, a bad one to be honest, but does the job
int hash(char *key, int size);

void insertItem(HashTable *table, char *key, char *value);
char *searchItem(HashTable *table, char *key);
int removeItem(HashTable *table, char *key);

#endif
