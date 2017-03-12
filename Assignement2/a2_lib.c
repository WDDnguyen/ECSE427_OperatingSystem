#include "a2_lib.h"
int fd; 
SM* sharedMemory;

sem_t * db;
sem_t * mutex;

/*
Create the share memory object. If the memory object isn't created, then create and truncate the memory
Also attach this shared object to a block of memory from setSharedMemory();

*/
int kv_store_create(char *smName){
	fd = shm_open(smName, O_CREAT | O_RDWR, S_IRWXU);
	
	if (fd == -1){
		printf("Shared memory initialization failed\n");
		return -1;
	}
	
	ftruncate(fd,sizeof(SM) + 15*numberOfPods * maximumOfRecords * sizeof(KVpair));
	setSharedMemoryAddress();
	initializeShareMemoryStruct();
	return 0;

}

//set up the sharedMemory with the memory object
int setSharedMemoryAddress(){
	
	sharedMemory = (SM*) mmap(NULL, sizeof(SM) +  15*numberOfPods * maximumOfRecords * sizeof(KVpair), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	
	if (sharedMemory == MAP_FAILED){
		printf("setting mmap() failed\n");
		return -1;
	}
	return 0;
}

//Hash function to place the word that return the specific pod to be placed in.
int hashFunction(char *word){
	int hashAddress = 5381;
	int counter =0;
	
	for (counter = 0; word[counter] != '\0'; counter++){
		hashAddress = ((hashAddress <<5) + hashAddress) + word[counter];
	}
	return hashAddress % numberOfPods < 0 ? -hashAddress % numberOfPods : hashAddress % numberOfPods;
}

//Write in the store in FIFO for each pod
int kv_store_write(char *key, char *value){
	
	int podIndex = hashFunction(key);
	size_t offset = sizeOfRecord * sharedMemory->podCounters[podIndex];
	size_t podLocation = podSpace * podIndex;

	if(sem_trywait(db) == -1){
		sem_wait(db);
	} 
		
	memcpy(sharedMemory + sizeof(SM) + offset + podLocation, key, keySize);
	memcpy(sharedMemory + sizeof(SM) + offset + podLocation + keySize, value , valueSize); 

	sharedMemory->podCounters[podIndex]++;
	sharedMemory->podCounters[podIndex] = sharedMemory->podCounters[podIndex] % 256;

	sem_post(db);
	return 0;
	}

char* kv_store_read(char *key){
	int podIndex = hashFunction(key);
	int i;
	size_t offset;
	size_t podLocation = podSpace * podIndex;
	char* duplicated;
	
	for (i = 0; i < podSize ; i++){
		offset = sizeOfRecord * sharedMemory->podRead[podIndex];
		
		if( memcmp( sharedMemory + sizeof(SM) + offset + podLocation , key, strlen(key)) == 0){
			duplicated = strdup((char*) (sharedMemory + sizeof(SM) + podLocation +  offset + keySize));
			sharedMemory->podRead[podIndex]++;
			sharedMemory->podRead[podIndex] = sharedMemory->podRead[podIndex] % 256;	
			return duplicated;
		}
		else {
		sharedMemory->podRead[podIndex]++;
		sharedMemory->podRead[podIndex] = sharedMemory->podRead[podIndex] % 256;
		}
	}
	return NULL; 
}

char ** kv_store_read_all(char *key){
	
	int podIndex = hashFunction(key);
	
	char** allValuesOfKey = malloc(sizeof(char*));
	char* value;

	int index = 0;
		
		while(1){
			
			if(index == 0){
				value = kv_store_read(key);
				allValuesOfKey[index] = value;
			}
			else{
				allValuesOfKey[index] = kv_store_read(key);
			}
			
			index++;
			allValuesOfKey = realloc(allValuesOfKey, sizeof(char*) * (index + 1));
	
			if (value == NULL){
				return NULL;
			}
			else if (index != 1 && strcmp(value, allValuesOfKey[index - 1]) == 0){
				allValuesOfKey[index - 1] = NULL;
				return allValuesOfKey;
			}			
		}
		return NULL;
}

int initializeShareMemoryStruct(){
	int i = 0;
	//initialize the semaphores
	db = sem_open("db", O_CREAT, S_IRUSR ,1);
	mutex = sem_open("mutex", O_CREAT, S_IRUSR ,1);
	
	//initialize the counters for the pods
	for(i = 0 ; i < numberOfPods ; i++){
		sharedMemory->podCounters[i] = 0;
		sharedMemory->podRead[i] = 0;	
	}	
	return 0;
}

int kv_delete_db(){
	
	if(munmap(sharedMemory, sizeof(SM) + 15*numberOfPods * maximumOfRecords * sizeof(KVpair)) == -1){
		printf("Error when deleting shared memory\n");
		return -1;
	}
	return 0;
}
