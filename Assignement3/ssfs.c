// In progress 
#include "ssfc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "disk_emu.h"

#include <sys/types.h>
#include <fcntl.h>

/*STRUCTURE OF THE BLOCKS OF THE HARD DRIVE
sb = 0
FBM = 1
WM = 2
root directory = 3
INODE BLOCK = 4 to 16 
rest is data blocks 

*/

//TEST THE BLOCKS 

block_t allBlocks[1000];

superblock_t sb;
block_t fbm;
block_t wm;



//inode_t rootTable[numberOfInodes];
//directoryEntry_t directoryEntries[numberOfInodes];
// File descriptor table to do 
fileDescriptor_t fdt[numberOfInodes];
inodeBlock_t inodeBlocks[13];
rootDirectory_t rootDirectory;

void initializeInodeFiles(){
	int i;
	int k;
	//printf("SIZE OF INODE BLOCK : %zu\n", sizeof(inodeBlock_t));
	inode_t tempInode;
	tempInode.size = -1;
	for (i =0 ; i < 14 ; i++){
		tempInode.direct[i] = -1;
	}
	tempInode.indirect = -1;
	
	for (i = 0; i < 13 ; i++){
	for (k = 0 ; k < 16; k++){
		inodeBlocks[i].inodeSlot[k]=tempInode;
		//printf("tempNode size : %d, direct value : %d, indirect value : %d at i : %d , k ; %d\n", inodeBlocks[i].inodeSlot[k].size, inodeBlocks[i].inodeSlot[k].direct[1], inodeBlocks[i].inodeSlot[k].indirect, i, k);
		}
	}
		
}

void setInodeDirect(int index, int blockNumber){
	int block = index / 16;
	int slot = index % 16;
	int i;
	
	for(i = 0; i< 14; i++){
		if( inodeBlocks[block].inodeSlot[slot].direct[i] == -1){
			inodeBlocks[block].inodeSlot[slot].direct[i] = blockNumber;
			break;
		}
	}
	
}
void getInode(int index){
	int block = index / 16;
	int slot = index % 16;
	int i;
	
	printf("INODE BLOCK TO SEARCH : %d\n", block);
	printf("SLOT TO SEARCH : %d\n", slot);
	
	for (i = 0; i < 14; i++){
	printf("This is what the block is pointing right now : %d\n", inodeBlocks[block].inodeSlot[slot].direct[i]);
	}
}

//need to test
/*void rootJNode_getBlock(int index){
	int block = index / 16;
	int slot = index % 16;
	int i;
	
}
*/
void initializeRootDirectory(){
	int i;
		
	/*for( i = 0; i < 14; i++){	
		rootDirectory.bytes[i] = 0xFF;
	}*/
	
	directoryEntry_t entry;
	entry.inodeIndex = -1;
	strcpy(entry.name,"root/");
	
	rootDirectory.size = 1024;
	for (i =0 ; i < 51 ; i++){
		rootDirectory.entries[i] = entry;
	}
	
}

void initializeSuperBlock(){
	
	int i;
	int k;
	// need to implement shadow later 
	inode_t root;
	root.size = 13312;
	// setting the block numbers to check for the root node 
	for( i = 0 ; i < 13 ; i++){
		root.direct[i] = 4 + i;
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
	
	//printf("No more free bits left \n");
	return -1; 
}

// initialize file directory  and set all values to free, rwptr to 0 and  no inode values 
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


void mkssfs(int fresh){
	int i;
	initializeFileDescriptorTable();
	
	//printf("%d\n",inodeBlocks[0].inodeSlot[0].direct[1]);
	//printf("%d\n",inodeBlocks[0].inodeSlot[1].direct[1]);
	//printf("%d\n",inodeBlocks[0].inodeSlot[2].direct[1]);
	
	if (fresh){
	initializeFBM();
	initializeWM();
	initializeSuperBlock();
	initializeInodeFiles();
	initializeRootDirectory();

	
	printf("ROOT DIRECTORY SIZE IS : %zu\n", sizeof(rootDirectory_t));
/*	setInodeDirect(0,1752);
	setInodeDirect(0,1753);
	setInodeDirect(0,1754);
	setInodeDirect(0,1755);
*/	
	//getInode(1);
	
	//create a new file system
	char* filename = "WDDNGUYEN";
	init_fresh_disk(filename, blockSize, numberOfBlocks);
	
	initializeFileDescriptorTable();
	write_blocks(0,1, &sb);
	write_blocks(1,1, &fbm);
	write_blocks(2,1, &wm);
	write_blocks(3,1, &rootDirectory);
	for (i = 0; i < 13; i++){
		write_blocks(4 + i, 1, &inodeBlocks[i]);
	}
	
	
	rootDirectory_t rootBuffer; 
	read_blocks(3,1,&rootBuffer);
	
	/*for (i = 0 ; i < 51 ; i++){
		printf("ENTRY NAME : %s, ENTRY VALUE : %d\n", rootBuffer.entries[i].name, rootBuffer.entries[i].inodeIndex); 
	}
	*/
	
	//printf("NAME OF ROOT : %s\n",rootDirectory.bytes);
	/*
	superblock_t sbbuffer;
	block_t buffer; 
	inodeBlock_t writeBuffer; 
	read_blocks(1,1, &buffer);
	read_blocks(0,1, &sbbuffer);
	read_blocks(4,1, &writeBuffer);
	/*printf("VALUE : %d\n", sbbuffer.block_size);
	printf("VALUE : %d\n", sbbuffer.fs_size);
	printf("VALUE : %d\n", sbbuffer.Inodes);
	printf("VALUE : %d\n", sbbuffer.root.direct[5]);
	
	for( i = 0; i < 16; i++){
	printf("INODE VALUE : %d\n", writeBuffer.inodeSlot[0].direct[i]);
	}
	*/
	
	
	
	
	
	
	}
	
	//printf(" THIS IS THE SIZE OF SUPER BLOCK : %zu\n", sizeof(sb));
	//printf(" THIS IS THE SIZE OF INODE : %zu\n", sizeof(inode_t));
		
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
	
	mkssfs(1);

	//rootJNode_getBlock(65);
	
	
	//getInode(65);
	//getInode(64);

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





