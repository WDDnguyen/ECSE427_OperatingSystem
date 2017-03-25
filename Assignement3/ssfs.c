// In progress 
#include "ssfc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <sys/types.h>
#include <fcntl.h>


#include "disk_emu.h"

superblock_t sb;
block_t fbm;
block_t wm;
inode_t root;

block_t inodeblock[14];

// File descriptor table to do 
fileDescriptor_t fdt[numberOfInodes];

void initializeSuperBlock(){
	
	int i;
	int k;
	// need to implement shadow later 

	root.size = 13312;
	for( i = 0 ; i < numberOfInodes ; i++){
		root.direct[i] = -1;
	}
	
	sb.magic[0] = 0xAC;
	sb.magic[1] = 0xBD;
	sb.magic[2] = 0x00;
	sb.magic[3] = 0x05;
	sb.block_size = blockSize;
	sb.fs_size = blockSize * numberOfBlocks; 
	sb.Inodes = numberOfInodes;
	sb.root = root;
}
 
void initializeFBM(){
	int i;
	for (i = 0 ; i < blockSize; i++){
		fbm.bytes[i] = 0xFF;
	}
}

void initializeWM(){
	int i;
	for(i = 0; i < blockSize; i++){
		wm.bytes[i] = 0xFF;
	}
}

int FBMGetFreeBit(){
	int i;
	int k;
	for (i = 0; i < blockSize; i++){
		for (k = 0; k < 8 ; k++){
			if (fbm.bytes[i] & (1 << k) != 0x00){
				
				printf("Found a free bit at : %d\n", i * 8 + k);
				// set the bit to 0 
				fbm.bytes[i] = fbm.bytes[i] ^ (1 << k);
				return (i * 8 + k); 
			} 
		}
	}
	
	printf("No more free bits left \n");
	return -1; 
}

// initialize file directory  and set all values to free, rwptr to 0 and  no inode values 

/*
void initializeFileDescriptorTable() {
	int i;
	fileDescriptor_t* tempfd = malloc(sizeof(fileDescriptor_t));
	tempfd->free = 1;
	tempfd->rwptr = 0;
	tempfd->inode = -1;
	for (i = 0; i < numberOfInodes; i++){
		memcpy(&fdt[i], tempfd, sizeof(fileDescriptor_t));
	}
}

*/
void mkssfs(){
	initializeFBM();
	initializeWM();
	initializeSuperBlock();
	printf(" THIS IS THE SIZE OF SUPER BLOCK : %zu\n", sizeof(sb));
	printf(" THIS IS THE SIZE OF INODE : %zu\n", sizeof(inode_t));
		
	// block_t fbm, wm 
	// loop bytes[i] = 0xFF 
	// write_block(1,1 &fbm)
	
	
}

/* 
ssfc_fopen(char* name ){
	access rootDirectory
   // finish without the commit then do commit	
}
}
*/

int main(){
	
	mkssfs();

	/*int i;
	int k;
	
	for(i = 0; i < blockSize; i++){
		for(k = 0 ; k < 8 ;k++){
		printf("%u ", !!(fbm.bytes[i] & (1 << k)));
		
		}
		printf("\n");
		
	}

	for(i = 0; i < blockSize; i++){
		for(k = 0 ; k < 8 ;k++){
		printf("%u", !!(wm.bytes[i] & (1 << k)));
		
		}
		printf("\n");
		
	} */
	
	return 0;
}





