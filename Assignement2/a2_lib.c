#include "a2_lib.h"
int fd; 
SM* sharedMemory;

int kv_store_create(char *smName){
	fd = shm_open(smName, O_CREAT | O_RDWR, S_IRWXU);
	
	if (fd == -1){
		printf("Shared memory initialization failed\n");
		return -1;
	}
	
	ftruncate(fd,sizeof(SM) + 10*numberOfPods * maximumOfRecords * sizeof(KVpair));
	setSharedMemoryAddress();
	initializeShareMemoryStruct();
	printf("SETUP COMPLETED\n");
	return 0;

}

int setSharedMemoryAddress(){
	
	sharedMemory = (SM*) mmap(NULL, sizeof(SM) +  10*numberOfPods * maximumOfRecords * sizeof(KVpair), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	
	if (sharedMemory == MAP_FAILED){
		printf("setting mmap() failed\n");
		return -1;
	}
	return 0;
}

int hashFunction(char *word){
	int hashAddress = 5381;
	int counter =0;
	
	for (counter = 0; word[counter] != '\0'; counter++){
		hashAddress = ((hashAddress <<5) + hashAddress) + word[counter];
	}
	return hashAddress % numberOfPods < 0 ? -hashAddress % numberOfPods : hashAddress % numberOfPods;
}


int kv_store_write(char *key, char *value){
	
	//int podIndex = hashFunction(key);
	int podIndex = 1;
	size_t offset = sizeOfRecord * sharedMemory->podCounters[podIndex];
	size_t podLocation = podSpace * podIndex;

	/*if(sem_trywait(&(sharedMemory->db)) == -1){
		printf("There is already another process reading/writing into the store\n");
		sem_wait(&(sharedMemory->db));
	} */
		
	memcpy(sharedMemory + sizeof(SM) + offset + podLocation, key, keySize);
	memcpy(sharedMemory + sizeof(SM) + offset + podLocation + keySize, value , valueSize); 

	if (sharedMemory->podCounters[podIndex] >= podSize){
	//printf("FIFO PRESENT THEN RESET COUNTER\n");
	sharedMemory->podCounters[podIndex] = sharedMemory->podCounters[podIndex]  % 256;
	} 
	
	else {
	//printf("WROTE THE KV in POD : %d AT LOCATION : %d \n", podIndex,sharedMemory->podCounters[podIndex]);
	sharedMemory->podCounters[podIndex]++;
	}

	//sem_post(&(sharedMemory->db));
	//printf("DB SEM RELEASED AFTER WRITING\n");
	return 0;
	}

char* kv_store_read(char *key){
	//int podIndex = hashFunction(key);
	int podIndex = 1;
	int i;
	size_t offset;
	size_t podLocation = podSpace * podIndex;
	
	char* duplicated;
	
	for (i = 0; i < podSize*2 ; i++){
		offset = sizeOfRecord * sharedMemory->podRead[podIndex];
		//int calculate = sizeOfRecord * (sizeOfPods-1)
		//offset = sizeOfRecord * i;
		//printf("OFFSET : %zu\n", offset/sizeOfRecord);
		if( memcmp( sharedMemory + sizeof(SM) + offset + podLocation , key, strlen(key))  == 0){
			duplicated = strdup((char*) (sharedMemory + sizeof(SM) + podLocation +  offset + keySize));
			offset = sizeOfRecord * (i+1);
			
			
			
			if (sharedMemory->podRead[podIndex] >= podSize){
			//printf("POD COUNTER : %d\n", sharedMemory->podCounters[podIndex]);
			//printf("RESET COUNTER\n");
			sharedMemory->podRead[podIndex] = 0;
			}
			
			else {
			//printf("WROTE THE KV in POD : %d AT LOCATION : %d \n", podIndex,sharedMemory->podCounters[podIndex]);
			sharedMemory->podRead[podIndex]++;
			//printf("NEXT POD READ VALUE : %d\n", sharedMemory->podRead[podIndex]);
		
			}
			
			return duplicated;
		}
		if (sharedMemory->podRead[podIndex] >= podSize){
			//printf("POD COUNTER : %d\n", sharedMemory->podCounters[podIndex]);
			//printf("RESET COUNTER\n");
			sharedMemory->podRead[podIndex] = 0;
			}
			
			else {
			//printf("WROTE THE KV in POD : %d AT LOCATION : %d \n", podIndex,sharedMemory->podCounters[podIndex]);
			sharedMemory->podRead[podIndex]++;
			//printf("NEXT POD READ VALUE : %d\n", sharedMemory->podRead[podIndex]);
		
			}
		
	}
	
	
	//printf("KEY NOT FOUND : %s\n", key);
	return NULL; 
	
}

// NEED TO FIX POINTER 

char ** kv_store_read_all(char *key){
	
	int podIndex = hashFunction(key);
	size_t podLocation = podSpace * podIndex;

	char** allValuesOfKey = malloc(podSize * keySize * valueSize);
	char* value;

	int index = 0;
	int i;
	size_t offset = 0;
	
	for (i = sharedMemory->podRead[podIndex]; i < podSize ; i++){
		value = kv_store_read(key);
		allValuesOfKey[index] = value;
		index = index + 1;
	}
	
	
	
	/*offset = sizeOfRecord * i;
	
	if (memcmp( sharedMemory + sizeof(SM) + podLocation + offset, key, strlen(key)) == 0){
		//printf("SAME KEY AT RECORD : %d\n", i);
		value = (char*)  (sharedMemory + sizeof(SM) + offset + podLocation + keySize);
		allValuesOfKey[index] = value;
		index = index + 1; 
		}
	}
	*/
	if (allValuesOfKey != NULL){
		return allValuesOfKey;
	}
	else{
		return NULL;
	}
}


int initializeShareMemoryStruct(){
	
	int i = 0;

	/*if(sem_init(&(sharedMemory->mutex),1 ,1) == 1){
		printf("mutex semaphore failed\n");
	}

	if(sem_init(&(sharedMemory->db),1 ,1) == 1){
		printf("database semaphore failed\n");
	}
	*/
	//initialize the counters for the pods
	for(i = 0 ; i < numberOfPods ; i++){
	sharedMemory->podCounters[i] = 0;
	sharedMemory->podRead[i] = 0;
	}
	

	printf("Finish initializing the Memory Struct\n");
	return 0;
}

int kv_delete_db(){
	if(munmap(sharedMemory, sizeof(SM) + 10*numberOfPods * maximumOfRecords * sizeof(KVpair)) == -1){
		printf("Error when deleting shared memory\n");
		return -1;
	}
	printf("DELETED Shared Memory\n");
	return 0;
}
