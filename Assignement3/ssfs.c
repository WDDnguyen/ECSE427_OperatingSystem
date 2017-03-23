// In progress 
#include "ssfc.h"

superblock_t sb;
int fbm[bitsInABlock];
int wm[bitsInABlock];


void initializeSuperBlock(){
	sb.magic = magic;
	sb.blockSize = blockSize;
	sb.fs_size = blockSize * numberOfBlocks; 
	sb.numberOfInodes = numberOfInodes;
	// probably need to modify root and add j nodes.
	sb.root = 0;
} 

void initializeFBM(){
	for (int i = 0 ; i < bitsInABlock; i++){
		fbm[i] = 1;
	}
}

void initializeWM(){
	// might need to change value for initialization
	for(int i = 0; i < bitsInABlock; i++){
		wm[i] = 1;
	}
}






