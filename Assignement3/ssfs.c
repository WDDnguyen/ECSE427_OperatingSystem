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
root directory = 2 - 5 
INODE BLOCK = 6 to 18 
rest is data blocks 

*/

superblock_t sb;
block_t fbm;

fileDescriptor_t fdt[numberOfInodes];
inodeBlock_t inodeBlocks[13];
rootDirectory_t rootDirectory[4];

//cache 
block_t writeBlock; 

void initializeInodeFiles(){
	int i;
	int k;
	
	
	inode_t tempInode;
	tempInode.size = -1;
	for (i =0 ; i < 14 ; i++){
		tempInode.direct[i] = -1;
	}
	tempInode.indirect = -1;
	
	// initialize all inode to be unused 
	for (i = 0; i < 13 ; i++){
	for (k = 0 ; k < 16; k++){
		inodeBlocks[i].inodeSlot[k]=tempInode;
		}
	}
	
	// first inode contains all 4 root directory block number 
	tempInode.size = 0;
	tempInode.direct[0] = sb.rootDirectoryBlockNumber[0];
	tempInode.direct[1] = sb.rootDirectoryBlockNumber[1];
	tempInode.direct[2] = sb.rootDirectoryBlockNumber[2];
	tempInode.direct[3] = sb.rootDirectoryBlockNumber[3];
	inodeBlocks[0].inodeSlot[0] = tempInode;
	
}

void initializeRootDirectory(){
	int i;
	int k;	
	
	directoryEntry_t entry;
	entry.inodeIndex = -1;
	strcpy(entry.name,"root/");

	// initialize all entries of root directories to be 0 
	for (k = 0 ; k < 4 ; k++){
		for (i = 0 ; i < numberOfEntries ; i++){
			rootDirectory[k].entries[i] = entry;
		}
	}
}

void initializeSuperBlock(){
	
	int i;
	int k;
	inode_t root;
	root.size = 13312;
	// setting the block numbers to check for the root node 
	for( i = 0 ; i < 13 ; i++){
		root.direct[i] = 6 + i;
	}
	
	sb.magic[0] = 0xAC;
	sb.magic[1] = 0xBD;
	sb.magic[2] = 0x00;
	sb.magic[3] = 0x05;
	sb.block_size = blockSize;
	sb.fs_size = blockSize * numberOfBlocks; 
	sb.Inodes = numberOfInodes;
	sb.root = root;
	sb.rootDirectoryBlockNumber[0] = 2;
	sb.rootDirectoryBlockNumber[1] = 3;
	sb.rootDirectoryBlockNumber[2] = 4;
	sb.rootDirectoryBlockNumber[3] = 5;
}
 
void initializeFBM(){
	int i;
	for (i = 0 ; i < blockSize; i++){
		fbm.bytes[i] = 0xFF;
	}
	
	// might need to put to 19
	for( i = 0 ; i < 18; i++){
	FBMGetFreeBit();
	}
	
}

int FBMGetFreeBit(){
	int i;
	int k;
	for (i = 0; i < blockSize; i++){
		for (k = 0; k < 8 ; k++){
			if ((fbm.bytes[i] & (1 << k)) == (1 << k)){
				
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

int setFBMbit(int blockNumber){
	int byte = blockNumber / 8;
	int bit = blockNumber % 8;
	fbm.bytes[byte] = fbm.bytes[byte] ^ (1 << bit); 
	  
}

int clearFBMbit(int blockNumber){
	 int byte = blockNumber / 8;
	 int bit = blockNumber % 8;
	 printf("BYTE CLEAR %d\n", byte);
	 printf("BIT TO CLEAR %d\n", bit);
	 
	 fbm.bytes[byte] = fbm.bytes[byte] ^ (1 << bit); 
	 
}

int rootAddInode(int inodeIndex){
	int i = inodeIndex / 16;
	int k;
	inode_t newInode;
	
	for (k = 0 ; k < 14; k++){
		newInode.direct[k] = -1;
	}
	newInode.size = 0;
	
	for(k = 0 ; i<16; k++){
		if (inodeBlocks[i].inodeSlot[k].size == -1){
			inodeBlocks[i].inodeSlot[k] = newInode;
			write_blocks(sb.root.direct[i], 1, &inodeBlocks[i]);
			printf("added new inode file to j-node direct : %d, inode Block Number : %d, slot : %d,  Inode index : %d\n", i, sb.root.direct[i],k,inodeIndex);
			return 1;
		} 
	}
return -1;	
}


int createFile(char* fname){
	// might need to overwrite the index if too many entries
	int i,k;
	int freeIndex = findFreeInodeIndex();
	
	directoryEntry_t entry;
	stpcpy(entry.name,fname);
	entry.inodeIndex = freeIndex;
	
	// check if file exist in the root directories 
	for (k = 0 ; k < 4; k++){
		for(i = 0 ; i < numberOfEntries ; i++){
			if( strcmp(rootDirectory[k].entries[i].name, fname) == 0){
				printf("file name : %s exist already in Directory at entry index : %d\n", fname,i);
				return -1;
			}
		}
	}
	
	for (k = 0 ; k < 4 ; k++){
		for (i = 0; i < numberOfEntries; i++){
			if (rootDirectory[k].entries[i].inodeIndex == -1){
				rootDirectory[k].entries[i] = entry;
				//might have to write to the disk after commit change whenever 
				write_blocks(sb.rootDirectoryBlockNumber[k],1, &rootDirectory[k]);
				break;
			}
		}
	}
	
	rootAddInode(freeIndex);
	return 0;
}


// NEED TO CHANGE THE BIT AFTER REMOVING 
int findFreeInodeIndex(){
	int i,k;
	int blockNumber;
	int inodeIndex = -5;
	
	//check for all the i-nodes files
	for (i = 0; i < 13; i++){
		
		blockNumber = sb.root.direct[i];
		//printf("searching this block number :%d at root direct : %d\n", blockNumber, i);
			
		for(k = 0; k < 16; k++){
			//printf("value %d\n",inodeBlockFound.inodeSlot[k].size);
			if (inodeBlocks[i].inodeSlot[k].size == -1){
				inodeIndex = i * 16 + k ;
				return inodeIndex;
			}	
		}
	}
		
	return inodeIndex;
	
}

int allocateDataBlock(int inodeIndex){
	// get j-node block 
	
	int i = inodeIndex / 16;

	int slot = inodeIndex % 16;
	
	int newBlockNumber = FBMGetFreeBit();
	int k;
	
	printf("inode Block Index : %d in slot : %d\n", i, slot);
	
	for (k = 0; k < 14 ; k++){
		if(inodeBlocks[i].inodeSlot[slot].direct[k] == -1){
			inodeBlocks[i].inodeSlot[slot].direct[k] = newBlockNumber;
			printf("Allocated data block : %d to inodeBlock[] : %d slot : %d  direct  :%d \n", newBlockNumber, i,slot, k );
			//Precaution
			write_blocks(sb.root.direct[i],1, &inodeBlocks[i]);
			return newBlockNumber;
		}
	}
	return -1;
	
}

// initialize file directory  and set all values to free, rwptr to 0 and  no inode values 
void initializeFileDescriptorTable() {
	int i;
	fileDescriptor_t fd;
	fd.free = -1;
	fd.rwptr = 0;
	fd.inode = -1;
	for (i = 0; i < numberOfInodes; i++){
		fdt[i] = fd; 
	}

}

void mkssfs(int fresh){
	int i;
	int k;
	initializeFileDescriptorTable();
		
	if (fresh){
	
		initializeFBM();
		initializeSuperBlock();
		initializeInodeFiles();
		initializeRootDirectory();

		//create a new file system
		char* filename = "WDDNGUYEN";
		init_fresh_disk(filename, blockSize, numberOfBlocks);
		
		initializeFileDescriptorTable();
		
		write_blocks(0,1, &sb);
		write_blocks(1,1, &fbm);		
		write_blocks(sb.rootDirectoryBlockNumber[0],1, &rootDirectory[0]);
		write_blocks(sb.rootDirectoryBlockNumber[1],1, &rootDirectory[1]);
		write_blocks(sb.rootDirectoryBlockNumber[2],1, &rootDirectory[2]);
		write_blocks(sb.rootDirectoryBlockNumber[3],1, &rootDirectory[3]);
		for (i = 0; i < 13; i++){
			write_blocks(6 + i, 1, &inodeBlocks[i]);
		}
		
		printf("DIRECTORY BLOCK NUMBER : %d\n", sb.rootDirectoryBlockNumber[0]);
		
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
	// open root directory
	read_blocks(sb.rootDirectoryBlockNumber[0],1,&rootDirectory[0]);
	read_blocks(sb.rootDirectoryBlockNumber[1],1,&rootDirectory[1]);
	read_blocks(sb.rootDirectoryBlockNumber[2],1,&rootDirectory[2]);
	read_blocks(sb.rootDirectoryBlockNumber[3],1,&rootDirectory[3]);
	//open all inode file to cache 
	for(i = 0; i < 13; i++){
		read_blocks(sb.root.direct[i], 1 , &inodeBlocks[i]);
	}
	
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

int findEntry(char *name){
	int i;
	// check if file is already in root directory then add to open file descriptor
	for (i = 0; i < numberOfEntries ; i++){
		if (strcmp(rootDirectory[0].entries[i].name, name) == 0){
			printf("Found the entry : %s with inode Index : %d\n", name, rootDirectory[0].entries[i].inodeIndex);
			return rootDirectory[0].entries[i].inodeIndex;
		}		
	}
	return -1;
}


int ssfs_fopen(char *name){
	int i;
	int inodeIndex;
	
	//file name too big 
	if (strlen(name) > 10){
		return -1;
	}
	
	// search root directory for file name
	inodeIndex = findEntry(name);
	// if doesnt exist then create a new file 
	if(inodeIndex == -1){
		createFile(name);
		inodeIndex = findEntry(name); 
	}
	
	//check if fdt already has a file open, if so return index of fdt  * maybe change
	for(i = 0; i < numberOfInodes;i++){
		if(fdt[i].inode == inodeIndex){
			// need to adjust write pointer after writing into data 
			return i;
		}
	}
	
	//check for free space in fdt and add inode index to the table if so  
	for(i = 0; i < numberOfInodes; i++){
		if(fdt[i].free == -1 ){
			//printf("There is enough free space\n");
			fdt[i].inode = inodeIndex;
			fdt[i].rwptr = 0;
			fdt[i].free = 0;
			
			return i;
		}
	}
	
	return -1;
	
}

int ssfs_close(int fileID){
	int i;
	
	//invalid fileID
	if(fileID < 0 || fileID >= numberOfInodes){
		printf("invalid fileID\n");
		return -1;
	}
	
	//if file ID is not open 
	if (fdt[fileID].free == -1){
		printf("fdt location is free");
		return -1;
	}
	
	// remove fdt open file
	printf("close file ID : %d with inode : %d\n",fileID,fdt[fileID].inode);
	fdt[fileID].inode = -1;
	fdt[fileID].rwptr = 0;
	fdt[fileID].free = -1;
	return 0;	
	
}

void displayFDT(){
	int i;
	// rechange to number of inodes
	for(i = 0; i < 64; i++){
		printf("FDT index : %d free : %d pointer : %d inode : %d\n", i, fdt[i].free,fdt[i].rwptr,fdt[i].inode);
	}
}

void displayRootDirectoryEntries(){
	int i,k;
	for(k = 0 ; k < 4; k++){
		for(i = 0; i < numberOfEntries; i++){
			printf("entry name : %s inode index : %d\n", rootDirectory[0].entries[i].name, rootDirectory[0].entries[i].inodeIndex);
		}
	}
}

void displayJnodeIndex(){
	int i;
	for(i = 0; i < 13; i++){
		printf("J-node block number :%d  at %d\n", sb.root.direct[i], i);
	}
}

 void displayFBM(){
	 int i,k;
	 for (i =0; i < 5; i++){
		 for (k = 0 ; k < 8; k++){
			 printf("%u", !!(fbm.bytes[i] & (1 <<k)));
		 }
		 printf("\n");
	 }
 }

void displayInodeBlocks(){
	int i,k,p;
	
	for(i = 0; i <1 ; i++){
		printf("inode Block : %d\n",i);	 
	for( k = 0; k < 10; k++){
			printf("size : %d\n", inodeBlocks[i].inodeSlot[k].size);
		for(p = 0; p < 2; p++){
			printf("Directory value : %d\n", inodeBlocks[i].inodeSlot[k].direct[p]);	 
		}
		printf("\n");
	}
 }
 
 
 
 
	
}
// error checking needed 
int ssfs_fwrite(int fileID, char *buf, int length){
	
	int bytesWritten;
	int inodeIndex = fdt[fileID].inode;
	printf("inodeIndex : %d\n", inodeIndex);
	int i = inodeIndex / 16;
	int slotIndex = inodeIndex % 16;
	int size = inodeBlocks[i].inodeSlot[slotIndex].size;
	
	int writeInDataBlock;
	int directNumber;	
	// char to start writting from in a data block 
	int k = size % 1024;
	int currentBlock;
	int p;


	block_t write;
	
		
	//check which block in the inode where the pointer is currently at 
	currentBlock = fdt[fileID].rwptr / blockSize;
	
	int totalWritingSize = fdt[fileID].rwptr + length;		
	int remaining = 0;
	int numberOfBlocksToAllocate = 0;
	int firstBlockDataLength;
	int lastBlockDataLength;
	
	// check if file is going to overflow 
	if (totalWritingSize > (currentBlock + 1) * blockSize){
		// data to be written to fill up the current block 
		firstBlockDataLength = blockSize - size;
		// rest of data to be written in the other blocks 
		remaining = length - firstBlockDataLength;
		// need to allocate more blocks to satisfy write size
		if (remaining > 0){
			// need atleast 1 block to allocate 
			numberOfBlocksToAllocate = (remaining / blockSize) + 1;	
			lastBlockDataLength = remaining % blockSize;
		}
	}
	
	printf("size : %d\n", size);
	
	// IF CREATED NEW FILE 
	if(size == 0){
		int l;
		allocateDataBlock(inodeIndex);
		// know its the first direct element to be written in 
		writeInDataBlock = inodeBlocks[i].inodeSlot[slotIndex].direct[0];
		
		read_blocks(writeInDataBlock,1, &write);
		memcpy(write.bytes,buf,length);
		
		printf("NEW FILE WRITE IN DATA BLOCK : %d\n", writeInDataBlock);
		
		write_blocks(writeInDataBlock, 1, &write);
		inodeBlocks[i].inodeSlot[slotIndex].size += length;
		// RECHECK
		fdt[fileID].rwptr += length;
		return length;
		
	}else {
	
		// don't need to allocate new data block if enough spac 
		if (numberOfBlocksToAllocate == 0){
			printf("ENOUGH PLACE TO CONTAIN DATA\n");
			directNumber = currentBlock;  
			writeInDataBlock = inodeBlocks[i].inodeSlot[slotIndex].direct[directNumber];
			read_blocks(writeInDataBlock,1, &write);
			
			memcpy(write.bytes + k,buf, length);
			printf("EXISTING FILE WRITE IN DATA BLOCK : %d\n", writeInDataBlock);
			write_blocks(writeInDataBlock, 1, &write);
			
			inodeBlocks[i].inodeSlot[slotIndex].size += length;
			fdt[fileID].rwptr += length;
			return length;
		}
		
		else {
		
			printf("NUMBER OF BLOCKS TO ALLOCATE DATA : %d\n", numberOfBlocksToAllocate);
			// need to allocate more blocks if the length of the write is going to be a lot 
			
			for(p = 0; p < numberOfBlocksToAllocate ; p++){
				allocateDataBlock(inodeIndex);
			}
					
			//search last block written 
			//directNumber = fdt[fileID].rwptr / blockSize;
			directNumber = currentBlock;
			
			printf("THIS HAS TO BE %d\n", currentBlock);
			writeInDataBlock = inodeBlocks[i].inodeSlot[slotIndex].direct[directNumber];
			read_blocks(writeInDataBlock,1, &write);
			
			// start by writing to the current block;
			int dataLength = firstBlockDataLength; 
			k = size % 1024;
			memcpy(write.bytes + k, buf, dataLength);
			write_blocks(writeInDataBlock, 1, &write);
			
			int n = 0;
			// now need to write to the completely filled 
			if (numberOfBlocksToAllocate > 1){
				
				for (n = 1; n < numberOfBlocksToAllocate ; n++){
					
				writeInDataBlock = inodeBlocks[i].inodeSlot[slotIndex].direct[currentBlock + n];
				printf("full block to write buffer : %d\n", writeInDataBlock);
				read_blocks(writeInDataBlock, 1, &write);
				memcpy(write.bytes, buf +((currentBlock - 1 + n) * blockSize) + dataLength, blockSize);
				write_blocks(writeInDataBlock, 1, &write);
				                         
				}
				
			}
			printf("CURRENT BLOCK : %d\n", currentBlock);
			
			// write to the last block  and update pointer 
			int lastDataLength = lastBlockDataLength;
				
			writeInDataBlock = inodeBlocks[i].inodeSlot[slotIndex].direct[currentBlock + n];
			
			printf("WRITING IN DATA BLOCK : %d\n", writeInDataBlock);
			//displayInodeBlocks();
			
			read_blocks(writeInDataBlock,1, &write);
			
			memcpy(write.bytes, buf + ((currentBlock - 1 + n) * blockSize) + dataLength, lastDataLength);
			write_blocks(writeInDataBlock, 1, &write);
			
			inodeBlocks[i].inodeSlot[slotIndex].size += length;
			fdt[fileID].rwptr += length;
			return length;
			}
		
	}
		
	return -1;
}


int main(){
	int i,k,p;
	int fileID = 0;  
	char * buffer = "BAZUSO"; 
	char * buffer1 = "THE 30 MAN SLAYER";
	char * buffer2 = "GUTS";
	int length = strlen(buffer);
	
	
	char *name = "ArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasArtoriasBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCsCrtoriCs";
	char *name1 = "BrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBsBrtoriBs";
	//mkssfs(4);
	mkssfs(1);

	printf("\n");


	
	//displayJnodeIndex();
	
	//displayFBM();
	displayInodeBlocks();
	
	createFile("cake");
	createFile("portal");
	createFile("Catherine");
	createFile("PERSONA");
	
	displayInodeBlocks();
	
	//displayFBM();


	/*
	//displayRootDirectoryEntries();

	ssfs_fopen("cake");
	ssfs_fopen("portal");
	ssfs_fopen("Catherine");
	ssfs_fopen("PERSONA");
	
	
	ssfs_fwrite(fileID, buffer1, strlen(buffer1));
	ssfs_fwrite(fileID, buffer, length);
	ssfs_fwrite(fileID, buffer2, strlen(buffer2));
	ssfs_fwrite(fileID, buffer, length);
	ssfs_fwrite(fileID, name, strlen(name));

	
	//ssfs_fwrite(3,buffer, length);
	
	ssfs_fopen("meow");
	ssfs_fopen("woof");
	
	ssfs_close(2);
	ssfs_close(3);
	
	ssfs_fopen("ribbit");
	ssfs_fopen("rarw");
	ssfs_fopen("behhhh");
	
	ssfs_fopen("meow");
	ssfs_fopen("woof");
	
	
	ssfs_close(250);

	ssfs_close(-250); */
	
	
	/*
	unsigned char b[1024];
	read_blocks(19,1,b);
	
	
	for(i=0; i < 1024; i++){
		printf("%c",b[i]);
	}
	
	
	printf("\n---------------------------------------------------------\n");
	block_t test;
	
	
	read_blocks(17,1,&test);
	
	for(i=0; i < 1024; i++){
		printf("%c",test.bytes[i]);
	}
	
	read_blocks(18,1,&test);
	printf("\n---------------------------------------------------------\n");
	for(i=0; i < 1024; i++){
		printf("%c",test.bytes[i]);
	}
	read_blocks(19,1,&test);
	
	printf("\n---------------------------------------------------------\n");
	for(i=0; i < 1024; i++){
		printf("%c",test.bytes[i]);
	}
	
	read_blocks(20,1,&test);
	
	printf("\n---------------------------------------------------------\n");
	for(i=0; i < 1024; i++){
		printf("%c",test.bytes[i]);
	}
	
	
		
	//displayFDT();
	//displayInodeBlocks();
*/
	return 0;
}
