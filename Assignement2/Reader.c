#include "common.h"

int fd;
 
SM* sharedMemory;
//char* str = "THECAKEIFALIE";

int kv_store_create(char *smName){
	fd = shm_open(smName, O_CREAT | O_RDWR, S_IRWXU);
	if (fd == -1){
	printf("Shared memory initialization failed\n");
	return -1;
	}
	
	ftruncate(fd,sizeof(SM) + maximumOfRecords * sizeOfRecord);

	
	//printf("MEMORY NAME : %s\n", smName);
	//printf("FILE DESCRIPTION VALUE : %d\n", fd);
	printf("SETUP COMPLETED\n");
	
	return 0;

}

int setSharedMemoryAddress(){
	
	sharedMemory = (SM*) mmap(NULL, sizeof(SM) + maximumOfRecords * sizeOfRecord, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (sharedMemory == MAP_FAILED){
		printf("setting mmap() failed\n");
		return -1;
	}
	
	return 0;
}

int kv_store_write(char *key, char *value){

char concatenatedString[288];
strcpy(concatenatedString,key);
strcat(concatenatedString,value);

size_t offset = sizeOfRecord * sharedMemory->currentRecord;
//printf("CURRENT RECORD TO WRITE IN : %d\n",currentRecord);
//printf("WRITTING INTO SHARED MEMORY\n"); 

//strcpy(sharedMemory->store[offset], key);
memcpy(sharedMemory + sizeof(SM) + offset , key, keySize);
memcpy(sharedMemory + sizeof(SM) + offset + keySize, value , valueSize); 
sharedMemory->currentRecord = sharedMemory->currentRecord + 1;

}

//got to change later
//POSSIBLE LEAK
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
				//memcpy(duplicated, test, 1);
				//memcpy(duplicated , sharedMemory + offset + keySize, 1);
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

//printf("THIS IS WHAT IS WRITTEN IN THE STORE CURRENTLY : %c\n", sharedMemory->store); 
//printf("This is DATA OF THE STORE CURRENTLY : %c\n", sharedMemory->store + 32);


printf("THIS IS THE CURRENT RECORD : %d\n", sharedMemory->currentRecord);

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

//displayAllSharedMemory();
}
