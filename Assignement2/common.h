#ifndef _INCLUDE_COMMON_H_
#define _INCLUDE_COMMON_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <semaphore.h>

#define SharedMemoryName "/WDDNguyen"

#define keySize 32
#define valueSize 256
#define sizeOfRecord 288

#define maximumOfRecords 32


typedef struct{
char key[32];
char value[256];

}KVpair;

typedef struct {

int currentRecord;
KVpair KVQueue[0];

}SharedValues;

#endif 
