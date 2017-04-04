// In progress 
#include "sfs_api.h"
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
	
	//printf("No more free bits left \n");
	return -1; 
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

int setFBMbit(int blockNumber){
	int byte = blockNumber / 8;
	int bit = blockNumber % 8;
	fbm.bytes[byte] = fbm.bytes[byte] ^ (1 << bit); 
	//printf("remove block :%d from fbm \n", blockNumber);
	return 0;
}

int clearFBMbit(int blockNumber){
	 int byte = blockNumber / 8;
	 int bit = blockNumber % 8;
	 //printf("BYTE CLEAR %d\n", byte);
	 //printf("BIT TO CLEAR %d\n", bit);
	 
	 fbm.bytes[byte] = fbm.bytes[byte] ^ (1 << bit); 
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


int rootAddInode(int inodeIndex){
	int i = inodeIndex / 13;
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
			//printf("added new inode file to j-node direct : %d, inode Block Number : %d, slot : %d,  Inode index : %d\n", i, sb.root.direct[i],k,inodeIndex);
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
			//	printf("file name : %s exist already in Directory at entry index : %d\n", fname,i);
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

int allocateDataBlock(int inodeIndex){
	// get j-node block 
	
	int i = inodeIndex / 13;

	int slot = inodeIndex % 13;
	
	int newBlockNumber = FBMGetFreeBit();
	int k;
	
	//printf("inode Block Index : %d in slot : %d\n", i, slot);
	
	for (k = 0; k < 14 ; k++){
		if(inodeBlocks[i].inodeSlot[slot].direct[k] == -1){
			inodeBlocks[i].inodeSlot[slot].direct[k] = newBlockNumber;
			//printf("Allocated data block : %d to inodeBlock[] : %d slot : %d  direct  :%d \n", newBlockNumber, i,slot, k );
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
	fd.readptr = 0;
	for (i = 0; i < numberOfInodes; i++){
		fdt[i] = fd; 
	}

}

void mkssfs(int fresh){
	int i;
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
		
		//printf("DIRECTORY BLOCK NUMBER : %d\n", sb.rootDirectoryBlockNumber[0]);
		
	}
	// FILE SYSTEM ALREADY EXISTED
	else {
	//printf("ONLY GO HERE IF FRESH = 0 \n");
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
	int i,k;
	// check if file is already in root directory then add to open file descriptor
	for (k = 0 ; k < 4; k ++){
		for (i = 0; i < numberOfEntries ; i++){
			if ((k * 64 + i) >= 200){
				//printf("too many files in the ssfs\n");
				return -1;
			} 
			if (strcmp(rootDirectory[k].entries[i].name, name) == 0){
				//printf("Found the entry : %s with inode Index : %d\n", name, rootDirectory[0].entries[i].inodeIndex);
				return rootDirectory[k].entries[i].inodeIndex;
			}		
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
			// need to adjust write pointer to last written file  when open    
			
			// get size of inode 
			int block = fdt[i].inode/ 13; 
			int slotIndex = fdt[i].inode % 13;
			int size = inodeBlocks[block].inodeSlot[slotIndex].size;
			
			// NEED TO TEST
			//printf("GOing here\n");
			fdt[i].rwptr = size;
			fdt[i].readptr = 0;
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
			fdt[i].readptr = 0;
			// when adidng the file into the fdt   put the write pointer to the last given ifle 
			//fdt[i].rwptr = size;
			
			return i;
		}
	}
	
	return -1;
	
}

int ssfs_fclose(int fileID){

	//invalid fileID
	if(fileID < 0 || fileID >= numberOfInodes){
		//printf("invalid fileID\n");
		return -1;
	}
	
	//if file ID is not open 
	if (fdt[fileID].free == -1){
		//printf("fdt location is free\n");
		return -1;
	}
	
	// remove fdt open file
	//printf("close file ID : %d with inode : %d\n",fileID,fdt[fileID].inode);
	fdt[fileID].inode = -1;
	fdt[fileID].rwptr = 0;
	fdt[fileID].readptr = 0;
	fdt[fileID].free = -1;
	return 0;	
	
}

int ssfs_frseek(int fileID, int loc){
	// check if fileID is valid
	
	if (loc < 0){
		//printf("invalid location \n");
		return -1;
	}
	
	if(fileID < 0 || fileID >= numberOfInodes){
		//printf("invalid fileID\n");
		return -1;
	}
	
	
	if (fdt[fileID].free == -1){
		//printf("Invalid file ID for read seeking");
		return -1;
	}
	
	int block = fdt[fileID].inode / 13;
	int slot = fdt[fileID].inode % 13;
	int size = inodeBlocks[block].inodeSlot[slot].size;
	if(loc > size){
		//printf("pointer value : %d  size of file to read : %d\n",loc , size);
		return -1;
	}
	
	fdt[fileID].readptr = loc;
	return 0;
	
}

int ssfs_fwseek(int fileID, int loc){
	
	// check if fileID is valid
	
	if (loc < 0){
		//printf("invalid location \n");
		return -1;
	}
	
	if(fileID < 0 || fileID >= numberOfInodes){
		//printf("invalid fileID\n");
		return -1;
	}
	
	
	if (fdt[fileID].free == -1){
		//printf("Invalid file ID for read seeking");
		return -1;
	}
	
	int block = fdt[fileID].inode / 13;
	int slot = fdt[fileID].inode % 13;
	int size = inodeBlocks[block].inodeSlot[slot].size;
	if(loc > size){
		//printf("pointer value : %d  size of file to write : %d\n", loc, size);
		return -1;
	}
	
	fdt[fileID].rwptr = loc;
	return 0;
	
	
}

// error checking needed 
int ssfs_fwrite(int fileID, char *buf, int length){
	
	// verify if file ID exist 
	if (fdt[fileID].inode == -1){
		//printf("ERROR : Not valid file ID\n");
		return -1;
	}
	
	int inodeIndex = fdt[fileID].inode;
	//printf("inodeIndex : %d\n", inodeIndex);
	int i = inodeIndex / 13;
	int slotIndex = inodeIndex % 13;
	int size = inodeBlocks[i].inodeSlot[slotIndex].size;
	//printf("Write pointer currently at : %d  length to write : %d\n",fdt[fileID].rwptr,length);
	//printf("Inode size: %d   size in write fct :%d\n",inodeBlocks[i].inodeSlot[slotIndex].size,size);
	
	int writeInDataBlock;
	int directNumber;	
	// char to start writting from in a data block 
	int k = fdt[fileID].rwptr % 1024;
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
	
	// check if file is going to overflow to another data block
	if (totalWritingSize > (currentBlock + 1) * blockSize){
		// data to be written to fill up the current block 
		firstBlockDataLength = blockSize - k;
		// rest of data to be written in the other blocks 
		remaining = length - firstBlockDataLength;
		// need to allocate more blocks to satisfy write size
		if (remaining > 0){
			// need atleast 1 block to allocate 
			numberOfBlocksToAllocate = (remaining / blockSize) + 1;	
			lastBlockDataLength = remaining % blockSize;
		}
	}
	
	if (totalWritingSize >= 14 * blockSize + 1){
		printf("Cannot write more than 14 blocks to the inode\n");
		return -1;
	}
	
	// If new file then allocate atleast 1 block 
	if(size == 0){
		//printf("New file\n");
		allocateDataBlock(inodeIndex);
		// testing to set size to 0 
		inodeBlocks[i].inodeSlot[slotIndex].size = 0;
		fdt[fileID].rwptr = 0;
	}
	
	// don't need to allocate new data block if enough space
	if (numberOfBlocksToAllocate == 0){
		
		//printf("Inode size: %d   totalWritingSize :%d\n",inodeBlocks[i].inodeSlot[slotIndex].size,totalWritingSize);
		inodeBlocks[i].inodeSlot[slotIndex].size = totalWritingSize;
		fdt[fileID].rwptr += length;
	
		//printf("ENOUGH PLACE TO CONTAIN DATA\n");
		directNumber = currentBlock;  
		writeInDataBlock = inodeBlocks[i].inodeSlot[slotIndex].direct[directNumber];
		read_blocks(writeInDataBlock,1, &write);
		
		memcpy(write.bytes + k,buf,length);
		
		//printf("EXISTING FILE WRITE IN DATA BLOCK : %d\n", writeInDataBlock);
		
		write_blocks(writeInDataBlock, 1, &write);
	
		return length;
	}
	
	else {
		//printf("Not going here for the test\n");
		//printf("NUMBER OF BLOCKS TO ALLOCATE DATA : %d\n", numberOfBlocksToAllocate);
		// need to allocate more blocks if the length of the write is going to be a lot 
		
		for(p = 0; p < numberOfBlocksToAllocate ; p++){
			allocateDataBlock(inodeIndex);
		}
				
		//search last block written 
		//directNumber = fdt[fileID].rwptr / blockSize;
		directNumber = currentBlock;
		
		//printf("THIS HAS TO BE %d\n", currentBlock);
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
			//printf("full block to write buffer : %d\n", writeInDataBlock);
			read_blocks(writeInDataBlock, 1, &write);
			memcpy(write.bytes, buf +((currentBlock - 1 + n) * blockSize) + dataLength, blockSize);
			write_blocks(writeInDataBlock, 1, &write);
									 
			}
			
		}
		//printf("CURRENT BLOCK : %d\n", currentBlock);
		
		// write to the last block  and update pointer 
		int lastDataLength = lastBlockDataLength;
			
		writeInDataBlock = inodeBlocks[i].inodeSlot[slotIndex].direct[currentBlock + n];
		
		//printf("WRITING IN DATA BLOCK : %d\n", writeInDataBlock);
		//displayInodeBlocks();
		
		read_blocks(writeInDataBlock,1, &write);
		
		memcpy(&(write.bytes), buf + ((currentBlock - 1 + n) * blockSize) + dataLength, lastDataLength);
		write_blocks(writeInDataBlock, 1, &write);
		printf("totalSize of writing : %d\n");
		inodeBlocks[i].inodeSlot[slotIndex].size = totalWritingSize;
		fdt[fileID].rwptr += length;
		
		return length;
		}		
	return -1;
}

int ssfs_fread(int fileID, char *buf, int length){
	
	// verify if file ID exist 
	if (fdt[fileID].inode == -1){
		//printf("ERROR : Not valid file ID\n");
		return -1;
	}
	
	int inodeIndex = fdt[fileID].inode;
	//printf("inodeIndex : %d\n", inodeIndex);
	int i = inodeIndex / 13;
	int slotIndex = inodeIndex % 13;
	int size = inodeBlocks[i].inodeSlot[slotIndex].size;
	
	int readInDataBlock;
	int directNumber;	
	
	
	// char to start writting from in a data block 
	int k = fdt[fileID].readptr % 1024;
	int currentBlock;

	block_t read;	
	
	//check which block in the inode where the pointer is currently at 
	//printf("READ POINTER CURRENTLY AT :%d\n", fdt[fileID].readptr);
	currentBlock = fdt[fileID].readptr / blockSize;
	
	int totalReadSize = fdt[fileID].readptr + length;		
	int remaining = 0;
	int numberOfBlocksToRead = 0;
	int firstBlockDataLength;
	int lastBlockDataLength;
	
	// check if file is going to overflow 
	if (totalReadSize > (currentBlock + 1) * blockSize){
		// data to be read in the current block 
		firstBlockDataLength = blockSize - k ;
		// rest of data to be written in the other blocks 
		remaining = length - firstBlockDataLength;
		// need to read blocks if remaining is bigger than 0 
		if (remaining > 0){
			// need atleast 1 extra block to read
			numberOfBlocksToRead = (remaining / blockSize) + 1;	
			lastBlockDataLength = remaining % blockSize;
		}
	}
	
	//If the read is much bigger than the size of the block   
	if(size < totalReadSize ){
		//printf("read length is more than size\n");
		return -1;	
	}
	else {
	
		// don't need to allocate new data block if enough space in the block  
		if (numberOfBlocksToRead == 0){
			//printf("ENOUGH PLACE TO READ DATA\n");
			directNumber = currentBlock;  
			readInDataBlock = inodeBlocks[i].inodeSlot[slotIndex].direct[directNumber];
			
			read_blocks(readInDataBlock,1, &read);

			//printf("\n--------------------------------------\n");
			//printf(" length of read : %d\n", length);
			//printf("\n--------------------------------------\n");
		
			char block[length];
			
			memcpy(block + k, read.bytes, length);
			block[length] = '\0';
			
			strcpy(buf,block);
			//printf("\n--------------------------------------\n");
			
			fdt[fileID].readptr += length;
			
			return length;
		}
		
		else {
			printf("NEVER GO HERE FOR THIS TEST\n");
			//printf("NUMBER OF BLOCKS TO READ : %d\n", numberOfBlocksToRead);
			 
			directNumber = currentBlock;
			
			readInDataBlock = inodeBlocks[i].inodeSlot[slotIndex].direct[directNumber];
			read_blocks(readInDataBlock,1, &read);
			
			// start by writing to the current block;
			int dataLength = firstBlockDataLength;
			printf("DataLength : %d\n", dataLength);
			k = size % 1024;
			
			char block[length];
			
			memcpy(block, read.bytes, dataLength);
			
			int n = 0;
			// now need to write to the completely filled 
			if (numberOfBlocksToRead > 1){
				
				for (n = 1; n < numberOfBlocksToRead ; n++){
					
				readInDataBlock = inodeBlocks[i].inodeSlot[slotIndex].direct[currentBlock + n];
				//printf("full block to READ buffer : %d\n", readInDataBlock);
				read_blocks(readInDataBlock, 1, &read);
				
				memcpy(block + ((currentBlock - 1 + n) * blockSize) + dataLength , read.bytes, blockSize);
				                         
				}
				
			}
			
			// read the last block  and update pointer 
			int lastDataLength = lastBlockDataLength;
				
			readInDataBlock = inodeBlocks[i].inodeSlot[slotIndex].direct[currentBlock + n];
			
			//printf("Read IN DATA BLOCK : %d\n", readInDataBlock);
			//displayInodeBlocks();
			
			read_blocks(readInDataBlock,1, &read);
			
			memcpy(block + ((currentBlock - 1 + n) * blockSize) + dataLength,read.bytes, lastDataLength);
			
			// ?? inodeBlocks[i].inodeSlot[slotIndex].size;
			
			block[length] = '\0';
			
			strcpy(buf,block);
			fdt[fileID].readptr += length;
			return length;
			}		
	}
	
	return -1; 
}

// remove file from directory entry, release the i-node entry and releasr the data blocks by the file
int ssfs_remove(char *file){
	int i,k,p;
	int inodeIndexFound;
	int inodeBlock; 
	int slot;
	
	if (strlen(file) > 10){
		//printf("invalid file Too large\n");
		return -1;
	}
	
	//Delete from root directory and remove inode and free data blocks of the inode
	
	for(k = 0; k < 4; k++){
		for(i = 0; i < 64; i++){
			if (strcmp(rootDirectory[k].entries[i].name, file) == 0){
				// found the entry
				inodeIndexFound = rootDirectory[k].entries[i].inodeIndex;
				
				//printf("found entry with name : %s with inode : %d\n", file, inodeIndexFound);
				
				//delete it from the directory 
				rootDirectory[k].entries[i].inodeIndex = -1;
				strcpy(rootDirectory[k].entries[i].name, "root/");
				
				inodeBlock = inodeIndexFound / 13;
				slot = inodeIndexFound % 13;
				
				// remove inode and inode files 
				
				inode_t removeInode;
				removeInode.size = -1;
				for(p = 0 ; p < 14; p++){
					if (inodeBlocks[inodeBlock].inodeSlot[slot].direct[p] != -1){
						setFBMbit(inodeBlocks[inodeBlock].inodeSlot[slot].direct[p]);
						}
						removeInode.direct[p] = -1;
				}
				
			// set the inode slot to be free 
			inodeBlocks[inodeBlock].inodeSlot[slot] = removeInode;
							
			//close file if open 
			
			for(i = 0; i < numberOfInodes; i++){
				if (fdt[i].inode == inodeIndexFound){
					fdt[i].inode = -1;
					fdt[i].free = -1;
					fdt[i].rwptr = 0;
					fdt[i].readptr = 0;
				}
			}
			//printf("successfully deleted entry\n");
			return 0;
			}
		}
	}
	
	//printf("There is no file in the root directories\n");
	return -1;
}
