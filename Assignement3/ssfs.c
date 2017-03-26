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
	
	
	//TEST TO MAKE SURE IT ACTUALLY WORKS 
	// setting root directory as first i-node;
	// NEED TO CHANGE TO 0
	tempInode.size = 0;
	tempInode.direct[0] = sb.rootDirectoryBlockNumber;
	inodeBlocks[0].inodeSlot[0] = tempInode;
	
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

void initializeRootDirectory(){
	int i;
		
	directoryEntry_t entry;
	entry.inodeIndex = -1;
	strcpy(entry.name,"root/");

	for (i = 0 ; i < numberOfEntries ; i++){
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
	sb.rootDirectoryBlockNumber = 3;
}
 
void initializeFBM(){
	int i;
	for (i = 0 ; i < blockSize; i++){
		fbm.bytes[i] = 0xFF;
	}
	
	//setting the block used for superblock, fbm, wm, root directory and inode files to 0;
	for( i = 0 ; i < 17; i++){
	FBMGetFreeBit();
	}
	
}

void initializeWM(){
	int i;
	int k;
	
	// set all wm bits to 1
	for(i = 0; i < blockSize; i++){
		wm.bytes[i] = 0xFF;
	}
	// setting sb,fbm,wm,directory and all inode files for the first 17 block to 0 
	
	for (i = 0 ; i < 17 ; i++){
		setWMbit(i);
	}
	
}

int setWMbit(int blockNumber){
	int byte = blockNumber / 8;
	int bit = blockNumber % 8;
	wm.bytes[byte] = wm.bytes[byte] ^ (1 << bit);

}

// MIGHT NEED A WM FUNCTION TO SET VALUE TO WRITE;

int FBMGetFreeBit(){
	int i;
	int k;
	for (i = 0; i < blockSize; i++){
		for (k = 0; k < 8 ; k++){
			if ((fbm.bytes[i] & (1 << k)) == (1 << k)){
				
				//printf("Found a free bit at : %d\n", i * 8 + k);
				// set the bit to 0 
				fbm.bytes[i] = fbm.bytes[i] ^ (1 << k);
				return (i * 8 + k); 
			}
				
		}
	}
	
	printf("No more free bits left \n");
	return -1; 
}

int clearFBMbit(int blockNumber){
	 int byte = blockNumber / 8;
	 int bit = blockNumber % 8;
	 printf("BYTE CLEAR %d\n", byte);
	 printf("BIT TO CLEAR %d\n", bit);
	 
	 fbm.bytes[byte] = fbm.bytes[byte] ^ (1 << bit); 
	 
}

int clearWMbit(int blockNumber, int setBlockNumber){
	int byte = blockNumber / 8;
	int bit = blockNumber % 8;
	wm.bytes[byte] = wm.bytes[byte] ^ (1 << bit); 
	
	// might need it but  have to make it writtable
	/*
	int nbyte = setBlockNumber / 8;
	int nbit = setBlockNumber % 8;

	wm.bytes[nbyte] = wm.bytes[nbyte] ^ (1 << nbit);
	 */
}

int clearBlockInformation(int blockNumber, int newBlockNumber){
	printf("BLOCK NUMBER INFORMATION TO CLEAR %d\n", blockNumber);
	int i; 
	block_t empty;
	
	for (i = 0; i < blockSize; i++){
		empty.bytes[i] = 0;
	}
	
	clearFBMbit(blockNumber);
	clearWMbit(blockNumber,newBlockNumber);
	write_blocks(blockNumber, 1, &empty);
	
}

int createNewRootDirectory(){
	int blockNumber = FBMGetFreeBit();
	
	printf("CURRENT DIRECTORY BLOCK NUMBER : %d\n", sb.rootDirectoryBlockNumber);
	//write new root directory block to a free location 
	read_blocks(sb.rootDirectoryBlockNumber, 1, &rootDirectory);
	// rootDirectory variable is now the new rootDirectory
	write_blocks(blockNumber, 1, &rootDirectory);
	// clear root directory in previous block
	clearBlockInformation(sb.rootDirectoryBlockNumber, blockNumber);
	//set new directory block number in sb.
	sb.rootDirectoryBlockNumber = blockNumber;
	//write sb with the new root Directory block number 
	//updateMetaDataBlocks();
	write_blocks(0,1, &sb);
	write_blocks(1,1, &fbm);
	write_blocks(2,1, &wm);

	printf("NEW DIRECTORY BLOCK NUMBER : %d\n", sb.rootDirectoryBlockNumber);
	
}

int createRootInodeFile(){
	int i,k,p;
	int blockNumber,newBlockNumber;
	int inodeIndex = -1;
	inodeBlock_t inodeBlockFound;
	//check for all the i-nodes files
	
	for (i = 0; i < 13; i++){
		blockNumber = sb.root.direct[i];
		//printf("blockNumber : %d\n", blockNumber);
		read_blocks(blockNumber,1,&inodeBlockFound);
		for(k = 0; k < 16; k++){
			for(p = 0; p < 14; p++){
				if (inodeBlockFound.inodeSlot[k].direct[p] == sb.rootDirectoryBlockNumber){
					inodeIndex = i * 64 + k ;
					break;
				}		
			}
			if (inodeIndex > 0){
					break;
				}
		}
		if (inodeIndex > 0){
					break;
				}
	}
	
	// Find new block location 
	newBlockNumber = FBMGetFreeBit();
	// write the inode block with the root inode into a new block MIGHT NEED TO LINK IT  
	write_blocks(newBlockNumber,1,&inodeBlockFound);
	// maybe dont need to delete the inode block 
	clearBlockInformation(blockNumber);
	// set the root j-node  with the new block 
	sb.root[i] = newBlockNumber;
	//update the meta data 
	write_blocks(0,1, &sb);
	write_blocks(1,1, &fbm);
	write_blocks(2,1, &wm);
	
	return inodeIndex;
	
}

/* TEST TOMORROW 
int createEntry(char* fname){
	int i;
	directoryEntry_t entry;
	stpcpy(entry.name,fname);
	entry.inodeIndex = findFreeInode();
	for (i = 0; i < numberOfEntries; i++){
		if (rootDirectory.entries[i].inodeIndex == -1){
			rootDirectory.entries[i] = entry;
		}
	}
	
}
*/

// NEED TO CHANGE THE BIT AFTER REMOVING 
int findFreeInode(){
	int i,k,p;
	int blockNumber;
	int inodeIndex = -1;
	inodeBlock_t inodeBlockFound;
	
	//check for all the i-nodes files
	for (i = 0; i < 13; i++){
		blockNumber = sb.root.direct[i];
		//printf("blockNumber : %d\n", blockNumber);
		read_blocks(blockNumber,1,&inodeBlockFound);
		for(k = 0; k < 16; k++){
			
				if (inodeBlockFound.inodeSlot[k].size == -1){
					inodeIndex = i * 64 + k ;
					break;
				}
				
				if (inodeIndex > 0){
					break;
				}
		}
		if (inodeIndex > 0){
					break;
				}
	}
		
	return inodeIndex;
	
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
	int k;
	initializeFileDescriptorTable();
		
	if (fresh){
	
		initializeFBM();
		initializeWM();
		initializeSuperBlock();
		initializeInodeFiles();
		initializeRootDirectory();

		//create a new file system
		char* filename = "WDDNGUYEN";
		init_fresh_disk(filename, blockSize, numberOfBlocks);
		
		initializeFileDescriptorTable();
		
		write_blocks(0,1, &sb);
		write_blocks(1,1, &fbm);
		write_blocks(2,1, &wm);		
		write_blocks(sb.rootDirectoryBlockNumber,1, &rootDirectory);
		for (i = 0; i < 13; i++){
			write_blocks(4 + i, 1, &inodeBlocks[i]);
		}
		
		printf("DIRECTORY BLOCK NUMBER : %d\n", sb.rootDirectoryBlockNumber);
		
		//rootDirectory_t rootBuffer; 
		//read_blocks(3,1,&rootBuffer);
		
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
	// FILE SYSTEM ALREADY EXISTED
	else {
	printf("ONLY GO HERE IF FRESH = 0 \n");
	char* filename = "WDDNGUYEN";
	initializeFileDescriptorTable();
	init_disk(filename, blockSize, numberOfBlocks);
	
	// open super block 
	read_blocks(0,1,&sb);
	// open FBM 
	read_blocks(1,1,&fbm);
	// open WM
	read_blocks(2,1,&wm);
	// open root directory
	read_blocks(sb.rootDirectoryBlockNumber,1,&rootDirectory);
	
	}
	/*
	printf("--------------------------------------------------------------------\n");
	printf("Block Size : %d\n", sb.block_size);
	printf("File System Size : %d\n", sb.fs_size);
	printf("Number of Inodes : %d\n", sb.Inodes);	
	printf("SIZE OF SUPER BLOCK : %zu\n", sizeof(superblock_t));
	printf("SIZE OF INODE : %zu\n", sizeof(inode_t));
	printf("SIZE OF BLOCK : %zu\n", sizeof(block_t));
	printf("SIZE OF rootDirectory : %zu\n", sizeof(rootDirectory_t));
	printf("SIZE OF DIRECTORY ENTRY  : %zu\n", sizeof(directoryEntry_t));
	printf("SIZE OF INODE Block : %zu\n", sizeof(inodeBlock_t));
	printf("--------------------------------------------------------------------\n"); */
}


int main(){
	
	//mkssfs(4);
	mkssfs(1);
	//printf("NEW DIRECTORY BLOCK NUMBER : %d\n", sb.rootDirectoryBlockNumber);
	
	//createNewRootDirectory();
	
	//createNewRootDirectory();
	int inodeIndex = createRootInodeFile();
	printf("INODE INDEX : %d\n", inodeIndex);
	int freeIndex = findFreeInode(-1);
	printf("FREE INODE INDEX : %d\n", freeIndex);
	/*
	int i;
	int k;
	printf("--------------------------------------------------------------------\n");
	for(i = 0; i < 10; i++){
		for(k = 0 ; k < 8 ;k++){
		printf("%u ", !!(fbm.bytes[i] & (1 << k)));
		}
		printf("\n");
		
	}
	printf("--------------------------------------------------------------------\n");
	for(i = 0; i < 10; i++){
		for(k = 0 ; k < 8 ;k++){
		printf("%u ", !!(wm.bytes[i] & (1 << k)));
		}
		printf("\n");
		
	}
	printf("--------------------------------------------------------------------\n"); */
	
	return 0;
}
