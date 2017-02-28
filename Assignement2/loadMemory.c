#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int fd;
struct stat s;

int setupSharedMemory(){
fd = shm_open("/will", O_RDWR, 0);
        if (fd < 0){
                printf("Error opening shm\n");

        }

        printf("SETUP COMPLETED\n");
}

int main()
{
	
        setupSharedMemory();

	if (fstat(fd, &s) == -1){
		printf("Error fstat\n");
	}

        char *addr = mmap(NULL, s.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd , 0);
        close(fd);

        write(STDOUT_FILENO, addr, s.st_size);
	//write(STDOUT_FILENO, addr , 2);
}
