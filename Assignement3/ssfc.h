#include "disk_emu.h"

// size of components
#define blockSize 1024
#define sizeOfPointer 4
#define numberOfBlocks 2800
#define numberOfInodes 200
#define sizeOfInode 64
#define sizeOfSuperBlockField 4 

// non standard inode 
// size field  total number of bytes 
// no need to know about indirect 
// these block contains data

//File has lots of inodes 
// for j node   size = size of the file 
// set size = -1 to be blank


typedef struct {
	int size; 
	int direct[14];
	int indirect;
} inode_t;


typedef struct {
	inode_t inodeSlot[16];
} inodeBlock_t;

// root is a jnode
// have to add shadow jnode later
// need to fill to get to 1024 or  copy memory to block_t then pass that to the disk * calloc

typedef struct {
	
unsigned char magic[4];

int block_size;
int fs_size;
int Inodes;
inode_t root;
inode_t shadow[4];
int lastShadow;
//filling up the super block with empty value
char fill[684];
} superblock_t;


// can use uint8_t 
typedef struct{
	unsigned char bytes[blockSize];
}block_t;

typedef struct {
    int free;
    int inode;
    int rwptr;
} fileDescriptor_t;
 
typedef struct {
    char name[16];
    int inode;
} directoryEntry_t;
