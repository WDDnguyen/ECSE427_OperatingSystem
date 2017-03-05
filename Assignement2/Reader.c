#include "KVS.h"


int fd; 
SM* sharedMemory;

int kv_store_create(char *smName){
	fd = shm_open(smName, O_CREAT | O_RDWR, S_IRWXU);
	if (fd == -1){
	printf("Shared memory initialization failed\n");
	return -1;
	}
	
	ftruncate(fd,sizeof(SM) + 10*numberOfPods * maximumOfRecords * sizeof(KVpair));

	//printf("MEMORY NAME : %s\n", smName);
	//printf("FILE DESCRIPTION VALUE : %d\n", fd);
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



int podIndex = hashFunction(key);


//printf("POD VALUE : %d\n", podSpace);

size_t offset = sizeOfRecord * sharedMemory->podCounters[podIndex];
size_t podLocation = podSpace * podIndex;


//printf(" POD SPACE TO WRITE IN : %zu\n",podLocation);
//printf("offset of that POD is : %zu\n", offset);
//printf("WRITTING INTO SHARED MEMORY\n"); 

//size_t location = sizeof(SM) + offset + podLocation;

//printf("LOCATION TO WRITE : %zu\n", location);
memcpy( sharedMemory + sizeof(SM) + offset + podLocation, key, keySize);
memcpy( sharedMemory + sizeof(SM) + offset + podLocation + keySize, value , valueSize); 


sharedMemory->podCounters[podIndex]++;
//sharedMemory->currentRecord++;

printf("WROTE THE KV in POD : %d AT LOCATION : %d \n", podIndex,sharedMemory->podCounters[podIndex] - 1);
}


char* kv_store_read(char *key){
	int podIndex = hashFunction(key);
		
	char* duplicated;
	
	int i = 0;

	size_t offset = sizeOfRecord * i;
	size_t podLocation = podSpace * podIndex;
	// duplicate string 
	//POTENTIALLY CHANGE BACK TO POD COUNTER
	for (i = 0; i < podSize ; i++){
		offset = sizeOfRecord * i;
		if( memcmp( sharedMemory + sizeof(SM) + offset + podLocation , key, strlen(key))  == 0){
			printf("SAME KEY at record : %d\n", i);
				
				duplicated = (char*) (sharedMemory + sizeof(SM) + podLocation +  offset + keySize);
				printf("READ THE KV IN POD : %d AT LOCATION : %d \n", podIndex,sharedMemory->podCounters[podIndex]);
				return duplicated;
			}
			
		} 
	
	return NULL; // if no value of that key 

}

// POSSIBLE LEAK

char ** kv_store_read_all(char *key){
	
	int podIndex = hashFunction(key);
	size_t podLocation = podSpace * podIndex;

	char** allValuesOfKey = malloc(sizeof(256 * maximumOfRecords));
	char* value;

	int index = 0;
	int i = 0;
	size_t offset = 0;
	//printf("CURRENT RECORD : %d\n", currentRecord);
	//POTENTIALLY CHANGE TO POD COUNTER
	for ( i = 0; i < podSize ; i++){
	offset = sizeOfRecord * i;
	//printf("CURRENT INDEX I : %d\n", i);
	//printf("PRINTING OFFSET VALUE : %zu\n", offset);
 
	if (memcmp( sharedMemory + sizeof(SM) + podLocation + offset, key, strlen(key)) == 0){
		//printf("SAME KEY AT RECORD : %d\n", i);
		value = (char*)  (sharedMemory + sizeof(SM) + offset + podLocation + keySize);
		allValuesOfKey[index] = value;
		index = index + 1; 
		
	}

	}
	if (allValuesOfKey != NULL){
	return allValuesOfKey;
	}else {
	return NULL;
	}
		
}



int kv_delete_db(){
if(munmap(sharedMemory, sizeof(SM) + sizeof(KVpair) * maximumOfRecords) == -1){
	printf("Error when deleting shared memory\n");
}
printf("DELETED Shared Memory");
}


void displayAllSharedMemory(){

struct stat s;
if (fstat(fd, &s) == -1){
	printf("Error fstat\n");
}

write(STDOUT_FILENO, sharedMemory, s.st_size);

 }

int main(int argc, char *argv[]){

KVpair pair1;
KVpair pair2;
KVpair pair3;
KVpair pair4;
KVpair pair5;
KVpair pair6;
KVpair pair7;
KVpair pair8;


strcpy(pair1.key,"cake");
strcpy(pair1.value,"Portal");

strcpy(pair2.key,"cake");
strcpy(pair2.value, "IsALie");

strcpy(pair3.key,"cake");
strcpy(pair3.value, "NOT A LIE");

strcpy(pair4.key,"cake");
strcpy(pair4.value,"Portal");

strcpy(pair5.key,"cake");
strcpy(pair5.value, "SANCHEZ");

strcpy(pair6.key,"cake");
strcpy(pair6.value, "SCIENCE");

strcpy(pair7.key,"cake");
strcpy(pair7.value,"WISEWOLF");

strcpy(pair8.key,"cake");
strcpy(pair8.value, "MERCHANT");

kv_store_create(SharedMemoryName);
setSharedMemoryAddress();


char* duplicated1;

duplicated1 = kv_store_read(pair1.key);
printf("USING KEY : %s , OBTAINED VALUE : %s\n",pair1.key, duplicated1);

duplicated1 = kv_store_read(pair2.key);
printf("USING KEY : %s , OBTAINED VALUE : %s\n",pair1.key, duplicated1);

duplicated1 = kv_store_read(pair3.key);
printf("USING KEY : %s , OBTAINED VALUE : %s\n",pair1.key, duplicated1);

duplicated1 = kv_store_read(pair4.key);
printf("USING KEY : %s , OBTAINED VALUE : %s\n",pair1.key, duplicated1);

duplicated1 = kv_store_read(pair5.key);
printf("USING KEY : %s , OBTAINED VALUE : %s\n",pair1.key, duplicated1);

duplicated1 = kv_store_read(pair6.key);
printf("USING KEY : %s , OBTAINED VALUE : %s\n",pair1.key, duplicated1);

duplicated1 = kv_store_read(pair7.key);
printf("USING KEY : %s , OBTAINED VALUE : %s\n",pair1.key, duplicated1);

duplicated1 = kv_store_read(pair8.key);
printf("USING KEY : %s , OBTAINED VALUE : %s\n",pair1.key, duplicated1);


char** sameKey;
int i = 0;
sameKey = kv_store_read_all(pair3.key);
for ( i = 0 ; i < podSize; i++){
printf("USING KEY : %s, VALUE FOR A KEY IS : %s\n", pair1.key, sameKey[i]); 
}

//displayAllSharedMemory();
kv_delete_db();

}
