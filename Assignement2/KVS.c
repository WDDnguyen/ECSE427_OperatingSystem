#include "common.h"
#include <stddef.h>

int fd; 
SM* sharedMemory;

int kv_store_create(char *smName){
	fd = shm_open(smName, O_CREAT | O_RDWR, S_IRWXU);
	if (fd == -1){
	printf("Shared memory initialization failed\n");
	return -1;
	}
	
	ftruncate(fd,sizeof(SM) + maximumOfRecords * sizeof(KVpair));

	
	//printf("MEMORY NAME : %s\n", smName);
	//printf("FILE DESCRIPTION VALUE : %d\n", fd);
	printf("SETUP COMPLETED\n");
	
	return 0;

}

int setSharedMemoryAddress(){
	
	sharedMemory = (SM*) mmap(NULL, sizeof(SM) + maximumOfRecords * sizeof(KVpair), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (sharedMemory == MAP_FAILED){
		printf("setting mmap() failed\n");
		return -1;
	}
	
	return 0;
}

int initializeShareMemoryStruct(){
sharedMemory->currentRecord = 0;

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
printf("POD INDEX : %d\n", podIndex);

size_t offset = sizeOfRecord * sharedMemory->currentRecord;

//printf("CURRENT RECORD TO WRITE IN : %d\n",currentRecord);
//printf("WRITTING INTO SHARED MEMORY\n"); 



memcpy( sharedMemory + sizeof(SM) + offset, key, keySize);
memcpy( sharedMemory + sizeof(SM) + offset + keySize, value , valueSize); 

//printf("CURRENT RECORD : %d\n",sharedMemory->currentRecord);

sharedMemory->currentRecord++;

//printf("FINISH WRITING\n");
}
char* kv_store_read(char *key){
	char* duplicated;
	int i = 0;
	size_t offset = 0;
	// duplicate string 
	for (i = 0; i < sharedMemory->currentRecord ; i++){
		offset = sizeOfRecord * i;
		if( memcmp( sharedMemory + sizeof(SM) + offset , key, strlen(key))  == 0){
			printf("SAME KEY at record : %d\n", i);
				
				duplicated = (char*) (sharedMemory + sizeof(SM) + offset + keySize);
				return duplicated;
			}
			
		} 
	
	return NULL; // if no value of that key 

}

// POSSIBLE LEAK

char ** kv_store_read_all(char *key){
	char** allValuesOfKey = malloc(sizeof(256 * maximumOfRecords));
	char* value;

	int index = 0;
	int i = 0;
	size_t offset = 0;
	//printf("CURRENT RECORD : %d\n", currentRecord);
	for ( i = 0; i < sharedMemory->currentRecord ; i++){
	offset = sizeOfRecord * i;
	//printf("CURRENT INDEX I : %d\n", i);
	//printf("PRINTING OFFSET VALUE : %zu\n", offset);
 
	if (memcmp( sharedMemory + sizeof(SM) +  offset, key, strlen(key)) == 0){
		printf("SAME KEY AT RECORD : %d\n", i);
		value = (char*)  (sharedMemory + sizeof(SM) + offset + keySize);
		allValuesOfKey[index] = value;
		index = index + 1;
		//printf("THIS IS WHAT IS GOING TO BE RETURNED : %s\n", value);
		//printf("ADDED INTO ALL VALUES\n"); 
		
	}

	}
	if (allValuesOfKey != NULL){
	return allValuesOfKey;
	}else {
	return NULL;
	}
		
	//return NULL; // if no value of that key
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

strcpy(pair1.key,"Catherine");
strcpy(pair1.value,"Portal");

strcpy(pair2.key,"cake");
strcpy(pair2.value, "IsALie");

strcpy(pair3.key,"cake");
strcpy(pair3.value, "NOT A LIE");

kv_store_create(SharedMemoryName);
setSharedMemoryAddress();

//sharedMemory->currentRecord = 0;
//sharedMemory->cake = 3;

//printf("%s\n", sharedMemory->store); 

kv_store_write(pair2.key , pair2.value);
kv_store_write(pair3.key, pair3.value);
kv_store_write(pair1.key , pair1.value);


//printf("This is KEY VALUE : %s\n", (char*) (sharedMemory + sizeof(SM) ));
//printf("This is KEY VALUE : %s\n", (char*) (sharedMemory + sizeof(SM) + 32));

//printf("SIZE OF SHARED MEMORY IN BYTES : %zu\n", sizeof(sharedMemory));
//printf("SIZE OF SM IN BYTES : %zu\n", sizeof(SM));
//printf("SIZE OF KVPAIR IN BYTES : %zu\n", sizeof(KVpair));

//printf("This is KEY VALUE : %s\n", (char*) (sharedMemory + sizeof(SM) + 288));
//printf("This is KEY VALUE : %s\n", (char*) (sharedMemory + sizeof(SM) + 320));

//printf("This is KEY VALUE : %s\n", (char*) (sharedMemory + sizeof(SM) + 576));
//printf("This is KEY VALUE : %s\n", (char*) (sharedMemory + sizeof(SM) + 608));

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

char** sameKey;
int i = 0;
sameKey = kv_store_read_all(pair3.key);
for ( i = 0 ; i < 2; i++){
printf("USING KEY : %s, VALUE FOR A KEY IS : %s\n", pair3.key, sameKey[i]); 
}

*/

//displayAllSharedMemory();

}
