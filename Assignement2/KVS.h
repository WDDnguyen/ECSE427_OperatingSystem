#ifndef _INCLUDE_KVS_H_
#define _INCLUDE_KVS_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <semaphore.h>

#define SharedMemoryName "/WDDNGUYEN"

#define keySize 32
#define valueSize 256
#define sizeOfRecord 288

#define maximumOfRecords 65536

#define numberOfPods 256
#define podSize 256
#define podSpace 73728

typedef struct{
char key[32];
char value[256];

}KVpair;

typedef struct {

//sem_t s;

int podCounters[256];
int currentRecord;
//KVpair KVQueue[0];

}SM;

int kv_store_create(char *smName);
int kv_store_write(char *key, char *value);
char* kv_store_read(char *key);
char ** kv_store_read_all(char *key);
int kv_delete_db();

int setSharedMemoryAddress();
int initializeShareMemoryStruct();


#endif 
