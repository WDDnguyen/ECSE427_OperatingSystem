#include "common.h"

int fd;
size_t currentSize = 0;

//SharedValues* sharedMemory;

char *addressOfSharedMemory;
//char* str = "THECAKEIFALIE";

int kv_store_create(char *smName){
	fd = shm_open(smName, O_CREAT | O_RDWR, S_IRWXU);
	if (fd == -1){
	printf("Shared memory initialization failed\n");
	return -1;
	}
	
	ftruncate(fd,maximumOfSlots * sizeof(KVpair));
	
	//printf("MEMORY NAME : %s\n", smName);
	//printf("FILE DESCRIPTION VALUE : %d\n", fd);
	//printf("SETUP COMPLETED\n");
	
	return 0;

}

int setSharedMemoryAddress(){
	

	addressOfSharedMemory = mmap(NULL, maximumOfSlots * sizeof(KVpair) , PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (addressOfSharedMemory == MAP_FAILED){
		printf("setting mmap() failed\n");
		return -1;
	}
	
	return 0;
}

int kv_store_write(char *key, char *value){

char concatenatedString[298];
strcpy(concatenatedString,key);
strcat(concatenatedString,value);

//printf("WRITTING INTO SHARED MEMORY\n"); 
memcpy(addressOfSharedMemory + currentSize, concatenatedString , strlen(key) + strlen(value));
currentSize = currentSize + strlen(key) + strlen(value);

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

strcpy(pair1.key,"25");
strcpy(pair1.value,"256");

strcpy(pair2.key,"52");
strcpy(pair2.value, "652");

kv_store_create(SharedMemoryName);
setSharedMemoryAddress();
kv_store_write(pair1.key , pair1.value);
kv_store_write(pair2.key , pair2.value);

displayAllSharedMemory();


}
