#ifndef _INCLUDE_A2_LIB_H_
#define _INCLUDE_A2_LIB_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <semaphore.h>

#define SharedMemoryName "/WN"

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

//sem_t mutex;
//sem_t db;
int rc;
int podCounters[256];
int podRead[256];

}SM;


int  kv_store_create(char *name);
int  kv_store_write(char *key,char *value);
char *kv_store_read(char *key);
char **kv_store_read_all(char *key);
int kv_delete_db();

int setSharedMemoryAddress();
int initializeShareMemoryStruct();


#endif 
