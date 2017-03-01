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

#define maximumOfSlots 32

typedef struct{
char key[keySize];
char value[valueSize];

}KVpair;

typedef struct {
int StoreSlots;

}SharedValues;

#endif 
