#include "common.h"

int fd;
 
size_t currentRecord = 0;
//size_t keySize = 32;
//size_t valueSize = 256; 
//size_t sizeOfRecord = 288;

//SharedValues* sharedMemory;

char *addressOfSharedMemory;
//char* str = "THECAKEIFALIE";

int kv_store_create(char *smName){
	fd = shm_open(smName, O_CREAT | O_RDWR, S_IRWXU);
	if (fd == -1){
	printf("Shared memory initialization failed\n");
	return -1;
	}
	
	ftruncate(fd,maximumOfRecords * sizeof(KVpair));
	
	//printf("MEMORY NAME : %s\n", smName);
	//printf("FILE DESCRIPTION VALUE : %d\n", fd);
	//printf("SETUP COMPLETED\n");
	
	return 0;

}

int setSharedMemoryAddress(){
	

	addressOfSharedMemory = mmap(NULL, maximumOfRecords * sizeof(KVpair) , PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (addressOfSharedMemory == MAP_FAILED){
		printf("setting mmap() failed\n");
		return -1;
	}
	
	return 0;
}

int kv_store_write(char *key, char *value){

char concatenatedString[288];
strcpy(concatenatedString,key);
strcat(concatenatedString,value);

size_t offset = sizeOfRecord * currentRecord;
//printf("WRITTING INTO SHARED MEMORY\n"); 
memcpy(addressOfSharedMemory + offset, key , keySize);
memcpy(addressOfSharedMemory + offset + keySize, value , valueSize); 
currentRecord = currentRecord + 1;

}
//got to change later
char* kv_store_read(char *key){
	char* duplicated;
	int i = 0;
	size_t offset = 0;
	// duplicate string 
	for (i = 0; i < currentRecord ; i++){
		offset = offset + sizeOfRecord * i;
		if( memcmp( addressOfSharedMemory + offset , key, strlen(key))  == 0){
			printf("SAME KEY at record : %d\n", i);
				
				duplicated = addressOfSharedMemory + offset + keySize;
				//memcpy(duplicated, test, 1);
				//memcpy(duplicated , addressOfSharedMemory + offset + keySize, 1);
				return duplicated;
			}
			
		} 
	
	return NULL; // if no value of that key 

}

void displayAllSharedMemory(){

struct stat s;
if (fstat(fd, &s) == -1){
	printf("Error fstat\n");
}

write(STDOUT_FILENO, addressOfSharedMemory, s.st_size);

}


int main(int argc, char *argv[]){

KVpair pair1;
KVpair pair2;

strcpy(pair1.key,"Catherine");
strcpy(pair1.value,"Portal");

strcpy(pair2.key,"cake");
strcpy(pair2.value, "IsALie");

kv_store_create(SharedMemoryName);
setSharedMemoryAddress();
kv_store_write(pair1.key , pair1.value);
kv_store_write(pair2.key , pair2.value);

char* duplicated1;
char* duplicated2;

duplicated1 = kv_store_read(pair2.key);
duplicated2 = kv_store_read(pair1.key);

printf("USING KEY : %s , OBTAINED VALUE : %s\n",pair1.key, duplicated1);
printf("USING KEY : %s , OBTAINED VALUE : %s\n",pair2.key, duplicated2);

//displayAllSharedMemory();


}
