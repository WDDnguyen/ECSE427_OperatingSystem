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

//MIGHT NEED TO IGNORE DUPLICATE KV
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

memcpy(sharedMemory + sizeof(SM) + offset + podLocation, key, keySize);
memcpy(sharedMemory + sizeof(SM) + offset + podLocation + keySize, value , valueSize); 

//FIFO Strategy
// MIGHT NEED TO cHANGE A BIT
if (sharedMemory->podCounters[podIndex] == podSize - 1){
printf("FIFO PRESENT THEN RESET COUNTER\n");
sharedMemory->podCounters[podIndex] = 0;
}
else {
printf("WROTE THE KV in POD : %d AT LOCATION : %d \n", podIndex,sharedMemory->podCounters[podIndex]);
sharedMemory->podCounters[podIndex]++;
}


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
		printf("SAME KEY AT RECORD : %d\n", i);
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


int initializeShareMemoryStruct(){
sharedMemory->currentRecord = 0;
//initialize the counters for the pods
int i = 0;
for(i = 0 ; i < numberOfPods ; i++){
sharedMemory->podCounters[i] = 0;
}
printf("Finish initializing the Memory Struct\n");
return 0;
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
initializeShareMemoryStruct();

int fill = 0;
for(fill ; fill < 32; fill++){
kv_store_write(pair1.key , pair1.value);
kv_store_write(pair2.key , pair2.value);
kv_store_write(pair3.key , pair3.value);
kv_store_write(pair4.key , pair4.value);
kv_store_write(pair5.key , pair5.value);
kv_store_write(pair6.key , pair6.value);
kv_store_write(pair7.key , pair7.value);
kv_store_write(pair8.key , pair8.value);
}

kv_store_write(pair7.key , pair7.value);
kv_store_write(pair8.key , pair8.value);


printf("This is KEY : %s\n", (char*) (sharedMemory + sizeof(SM) + 217 * podSpace));
printf("This is VALUE : %s\n", (char*) (sharedMemory + sizeof(SM) + 217 * podSpace + keySize));

printf("This is KEY : %s\n", (char*) (sharedMemory + sizeof(SM) + 217 * podSpace + sizeOfRecord ));
printf("This is VALUE : %s\n", (char*) (sharedMemory + sizeof(SM) + 217 * podSpace + sizeOfRecord + keySize));

printf("----------------------------------\n\n");


printf("This is KEY : %s\n", (char*) (sharedMemory + sizeof(SM) + 217 * podSpace + sizeOfRecord * 255 ));
printf("This is VALUE : %s\n", (char*) (sharedMemory + sizeof(SM) + 217 * podSpace + sizeOfRecord * 255 + keySize));

printf("This is KEY : %s\n", (char*) (sharedMemory + sizeof(SM) + 217 * podSpace + sizeOfRecord * 256 ));
printf("This is VALUE : %s\n", (char*) (sharedMemory + sizeof(SM) + 217 * podSpace + sizeOfRecord * 256 + keySize));


//printf("This is KEY VALUE : %s\n", (char*) (sharedMemory + sizeof(SM) ));
//printf("This is KEY VALUE : %s\n", (char*) (sharedMemory + sizeof(SM) + 32));

//printf("SIZE OF SHARED MEMORY IN BYTES : %zu\n", sizeof(sharedMemory));
//printf("SIZE OF SM IN BYTES : %zu\n", sizeof(SM));
//printf("SIZE OF KVPAIR IN BYTES : %zu\n", sizeof(KVpair));

//printf("This is KEY VALUE : %s\n", (char*) (sharedMemory + sizeof(SM) + 217 * podSpace));
//printf("This is KEY VALUE : %s\n", (char*) (sharedMemory + sizeof(SM) + 217 * podSpace + keySize));

//printf("This is KEY VALUE : %s\n", (char*) (sharedMemory + sizeof(SM) + 576));
//printf("This is KEY VALUE : %s\n", (char*) (sharedMemory + sizeof(SM) + 608));

/*
int i = 0;
for(i = 0; i < numberOfPods * podSize ; i++){

if(memcmp((char*)(sharedMemory + sizeof(SM) + 288 * i), "", keySize) != 0){
printf("POD : %d\n", i);
printf("KEY : %s\n", (char*) (sharedMemory + sizeof(SM) + 288 * i));
printf("VALUE : %s\n", (char*) (sharedMemory + sizeof(SM) + 288 * i) );

}
} */
//char* test1 = sharedMemory->store; 
//char* test2 = sharedMemory->store + 288;
//char* test3 = sharedMemory->store + 576;

//printf("VALUE OF FIRST KEY : %s\n", test1);
//printf("VALUE OF SECOND KEY : %s\n", test2);
//printf("VALUE OF THIRD KEY : %s\n", test3);


//kv_store_write("Catherine", "Portal");
//kv_store_write("cake", "IsALie");
//kv_store_write("cake", "NOT A LIE");


/*
char* duplicated1;
char* duplicated2;
char* duplicated3;

duplicated1 = kv_store_read(pair2.key);
duplicated2 = kv_store_read(pair1.key);
duplicated3 = kv_store_read(pair3.key);

printf("USING KEY : %s , OBTAINED VALUE : %s\n",pair1.key, duplicated1);
printf("USING KEY : %s , OBTAINED VALUE : %s\n",pair2.key, duplicated2);
printf("USING KEY : %s , OBTAINED VALUE : %s\n",pair3.key, duplicated3);


printf("----------------------------------------------------------\n\n");

char** sameKey;
int i = 0;
sameKey = kv_store_read_all(pair3.key);
for ( i = 0 ; i < 2; i++){
printf("USING KEY : %s, VALUE FOR A KEY IS : %s\n", pair3.key, sameKey[i]); 
}
*/


//displayAllSharedMemory();

}
