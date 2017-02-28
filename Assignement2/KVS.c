#include "common.h"

int fd;
SharedValues* sharedMemory;

int InitializeSharedMemory(){
	fd = shm_open(SharedMemoryName, O_CREAT | O_RDWR, S_IRWXU);
	if (fd == -1){
	printf("Shared memory initialization failed\n");
	}
	ftruncate(fd,sizeof(SharedValues));

}

int SetSharedMemoryAddress(){
	sharedMemory = (SharedValues*) mmap(NULL,sizeof(SharedValues), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (sharedMemory == MAP_FAILED){
		printf("setting mmap() failed\n");
	} 
	return 0;
}

int main(int argc, char *argv[]){

}
