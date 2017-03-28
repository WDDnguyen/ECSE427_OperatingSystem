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

//cache 
block_t writeBlock; 

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
	
	/*
	for (i = 0; i < 13 ; i++){
	for (k = 0 ; k < 16; k++){
		printf("block size %d\n", inodeBlocks[i].inodeSlot[k].size);
		//printf("tempNode size : %d, direct value : %d, indirect value : %d at i : %d , k ; %d\n", inodeBlocks[i].inodeSlot[k].size, inodeBlocks[i].inodeSlot[k].direct[1], inodeBlocks[i].inodeSlot[k].indirect, i, k);
		}
	}
	*/
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
	int i;
	
	printf("CURRENT DIRECTORY BLOCK NUMBER : %d\n", sb.rootDirectoryBlockNumber);
	//write new root directory block to a free location 
	read_blocks(sb.rootDirectoryBlockNumber, 1, &rootDirectory);
	// rootDirectory variable is now the new rootDirectory
	write_blocks(blockNumber, 1, &rootDirectory);
	
	//create new InodeFile
	createRootInodeFile(blockNumber);
	
	// clear root directory in previous block
		//clearBlockInformation(sb.rootDirectoryBlockNumber, blockNumber);
	//set new directory block number in sb.
	sb.rootDirectoryBlockNumber = blockNumber;
	
	//write sb with the new root Directory block number 
	//updateMetaDataBlocks();
	
	write_blocks(0,1, &sb);
	write_blocks(1,1, &fbm);
	write_blocks(2,1, &wm);
	//ADDED
	for (i = 0; i < 13; i++){
		write_blocks(sb.root.direct[i],1,&inodeBlocks[i]);
	}
	
	printf("NEW DIRECTORY BLOCK NUMBER : %d\n", sb.rootDirectoryBlockNumber);
	// setting the directory cache after creating new root Directory
	read_blocks(sb.rootDirectoryBlockNumber, 1, &rootDirectory);
	
	return blockNumber;
	
}

int createRootInodeFile(int bn){
	int i,k,p;
	int newBlockNumber;
	int blockNumber = findRootInodeFile(bn);
	
	/*inodeBlock_t inodeBlockFound;
	read_blocks(blockNumber, 1, &inodeBlockFound);
	*/
	
	
	//find root direct index containing the block Number
	for(i = 0 ; i < 13; i++){
		if (sb.root.direct[i] == blockNumber){
			break;
		}
	}
	
	// Find new block location 
	newBlockNumber = FBMGetFreeBit();
	printf("New root inode block : %d at j-node index: %d\n ", newBlockNumber,i);
	
	write_blocks(newBlockNumber,1,&inodeBlocks[i]);
	sb.root.direct[i] = newBlockNumber;
		
	// After creating new root Directory and new File.  Add root directory block into the inode file containing the previous root directory block
	read_blocks(sb.root.direct[i],1, &inodeBlocks[i]);

	for (p = 0 ; p < 16; p++){
		for (k = 0; k < 14; k++){
			if (inodeBlocks[i].inodeSlot[p].direct[k] == sb.rootDirectoryBlockNumber){
				printf("previous root block: %d  new root block : %d\n", sb.rootDirectoryBlockNumber,bn);
				inodeBlocks[i].inodeSlot[p].direct[k] = bn;
				write_blocks(newBlockNumber,1, &inodeBlocks[i]);
				return newBlockNumber;
			} 
		}
	}
	
	return -2;
}

int findRootInodeFile(int bn){
	int i,k,p;
	//int inodeIndex = -5;
	int blockNumber = -5;
	//inodeBlock_t inodeBlockFound;
	//check for all the i-nodes files
	
	for (i = 0; i < 13; i++){
		blockNumber = sb.root.direct[i];
		//printf("blockNumber : %d\n", blockNumber);
		//read_blocks(blockNumber,1,&inodeBlocks[i]);
		
		for(k = 0; k < 16; k++){
			for(p = 0; p < 14; p++){
				if (inodeBlocks[i].inodeSlot[k].direct[p] == sb.rootDirectoryBlockNumber){
					//inodeIndex = i * 64 + k ;
					//return inodeIndex;
					return blockNumber;
				}		
			}
		}
	}
	
	return -1;
	
}

int rootAddInode(int inodeIndex){
	int i = inodeIndex / 16;
	int k;
	inode_t newInode;
	
	for (k = 0 ; k < 14; k++){
		newInode.direct[k] = -1;
	}
	newInode.size = 0;
	
	//inodeBlock_t inodeBlockFound;
	//read_blocks(sb.root.direct[i],1,&inodeBlockFound);
	
	
	for(k = 0 ; i<16; k++){
		if (inodeBlocks[i].inodeSlot[k].size == -1){
			inodeBlocks[i].inodeSlot[k] = newInode;
			write_blocks(sb.root.direct[i], 1, &inodeBlocks[i]);
			printf("added new inode file to j-node block : %d, inodeBlock : %d, index : %d\n", sb.root.direct[i],i, inodeIndex);
			return 1;
		} 
	}
return -1;	
}

 // ASSUME NOT ALLOCATING DISK SPACE WHEN CREATING A FILE 
int createFile(char* fname){
	// might need to overwrite the index if too many entries
	int i;
	int freeIndex = findFreeInodeIndex();
	directoryEntry_t entry;
	stpcpy(entry.name,fname);
	entry.inodeIndex = freeIndex;
	
	for(i = 0 ; i< numberOfEntries ; i++){
		if( strcmp(rootDirectory.entries[i].name, fname) == 0){
			printf("file name : %s exist already in Directory at entry index : %d\n", fname,i);
			return -1;
		}
	}
	
	for (i = 0; i < numberOfEntries; i++){
		if (rootDirectory.entries[i].inodeIndex == -1){
			rootDirectory.entries[i] = entry;
			//might have to write to the disk after commit change whenever 
			write_blocks(sb.rootDirectoryBlockNumber,1, &rootDirectory);
			break;
		}
	}
	
	// MIGHT NEED TO EVICT VALUE
	
	rootAddInode(freeIndex);
	return 0;
}


// NEED TO CHANGE THE BIT AFTER REMOVING 
int findFreeInodeIndex(){
	int i,k;
	int blockNumber;
	int inodeIndex = -5;
	//inodeBlock_t inodeBlockFound;
	
	//check for all the i-nodes files
	for (i = 0; i < 13; i++){
		
		blockNumber = sb.root.direct[i];
		//printf("searching this block number :%d at root direct : %d\n", blockNumber, i);
		//read_blocks(blockNumber,1,&inodeBlockFound);
			
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
	
	//inodeBlock_t inodeBlockFound;
	//read_blocks(sb.root.direct[jnodeIndex] , 1 , &inodeBlockFound);
	
	for (k = 0; k < 14 ; i++){
		if(inodeBlocks[i].inodeSlot[slot].direct[k] == -1){
			inodeBlocks[i].inodeSlot[slot].direct[k] = newBlockNumber;
			printf("Allocated data block : %d to inode block  : %d index slot : %d\n", newBlockNumber, i,slot);
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
		if (strcmp(rootDirectory.entries[i].name, name) == 0){
			printf("Found the entry : %s with inode Index : %d\n", name, rootDirectory.entries[i].inodeIndex);
			return rootDirectory.entries[i].inodeIndex;
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
	int i; 
	for(i = 0; i < numberOfEntries; i++){
		printf("entry name : %s inode index : %d\n", rootDirectory.entries[i].name, rootDirectory.entries[i].inodeIndex);
	}
}

void displayJnodeIndex(){
	int i;
	for(i = 0; i < 13; i++){
		printf("J-node block number :%d  at %d\n", sb.root.direct[i], i);
	}
}

void displayInodeBlocks(){
	int i,k,p;
	
	for(i = 0; i <1 ; i++){
		printf("inode Block : %d\n",i);	 
	for( k = 0; k < 16; k++){
			printf("size : %d", inodeBlocks[i].inodeSlot[k].size);
		for(p = 0; p < 14; p++){
			//printf("Directory value : %d\n", inodeBlocks[i].inodeSlot[k].direct[p]);	 
		}
		printf("\n");
	}
 }
 
 
 
	
}
// error checking needed 
int ssfs_fwrite(int fileID, char *buf, int length){
	
	int bytesWritten;
	int inodeIndex = fdt[fileID].inode;
	int i = inodeIndex / 16;
	int slotIndex = inodeIndex % 16;
	int size = inodeBlocks[i].inodeSlot[slotIndex].size;
	int writeInDataBlock;	
		
	// IF CREATED NEW FILE 
	if(size == 0){
		allocateDataBlock(inodeIndex);
		writeInDataBlock = inodeBlocks[i].inodeSlot[slotIndex].direct[0]; 
		write_blocks(writeInDataBlock, 1, &buf);
		inodeBlocks[i].inodeSlot[slotIndex].size += length;
		// RECHECK
		fdt[fileID].rwptr += length;
	}	
}


int main(){
	int i,k,p;
	int fileID = 2;  
	char * buffer = "BAZUSO"; 
	int length = strlen(buffer);
	
	//mkssfs(4);
	mkssfs(1);
	//printf("NEW DIRECTORY BLOCK NUMBER : %d\n", sb.rootDirectoryBlockNumber);
 
	createNewRootDirectory();
	printf("\n");
	
	//displayJnodeIndex();
	
	createFile("cake");
	createFile("portal");
	createFile("Catherine");
	createFile("PERSONA");
	
	
//	displayRootDirectoryEntries();


	ssfs_fopen("cake");
	ssfs_fopen("portal");
	ssfs_fopen("Catherine");
	ssfs_fopen("PERSONA");
	
	//ssfs_fwrite(3,buffer, length);
	
	ssfs_fopen("meow");
	ssfs_fopen("woof");
	
	//ssfs_close(2);
	//ssfs_close(3);
	
	ssfs_fopen("ribbit");
	ssfs_fopen("rarw");
	ssfs_fopen("behhhh");
	
	//ssfs_fopen("meow");
	//ssfs_fopen("woof");
	
	
	//ssfs_close(250);
	//ssfs_close(-250); 
	
	ssfs_fwrite(fileID, buffer, length);
	//displayFDT();
	//displayInodeBlocks();
	
	//displayFDT();
/*	
	for (i = 0; i < numberOfEntries; i++){
		printf("file : %s index : %d\n", rootDirectory.entries[i].name, rootDirectory.entries[i].inodeIndex);
	}
	
	inodeBlock_t ib;
	
	read_blocks(18,1, &ib);
	
	for(i =0 ; i<16;i++){
		printf("value : %d\n", ib.inodeSlot[i].size);
	}
*/
	//createNewRootDirectory();
	/*int rootBlock = findRootInodeFile();
	printf("BLOCK NUMBER OF ROOT : %d\n", rootBlock);
	int newRootBlock = createRootInodeFile();
	printf("WROTE ROOT BLOCK TO THIS BLOCK : %d\n ", newRootBlock);
	*/
	//int freeIndex = findFreeInodeIndex();
	//printf("FREE INODE INDEX : %d\n", freeIndex);
	/*
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
	printf("--------------------------------------------------------------------\n"); 
	*/
	return 0;
}
