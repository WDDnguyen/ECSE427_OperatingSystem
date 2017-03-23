#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int fd;

int setupSharedMemory(){
fd = shm_open("/will", O_CREAT|O_RDWR, S_IRWXU);
        if (fd < 0){
                printf("Error opening shm\n");
		
        }

	printf("SETUP COMPLETED\n");
}

int main()
{
	char* str = "WRITE THIS VALUE";
	char* str1 = "GOOD IS BA";
	setupSharedMemory();
	char *addr = mmap(NULL, strlen(str)+strlen(str1), PROT_READ | PROT_WRITE, MAP_SHARED, fd , 0);
	ftruncate(fd, strlen(str) + strlen(str1));
	//close(fd);
	memcpy(addr, str, strlen(str));	

	//ftruncate(fd, strlen(str1));
	//close(fd);
	memcpy(addr+strlen(str1),str1,strlen(str1));
}

