// size of components
#define blockSize 1024
#define sizeOfPointer 4
#define numberOfBlocks 2000
#define numberOfInodes 200
#define numberOfInodeblocks 13
#define sizeOfInode 64

//super block 
#define magic  0xACBD0005
#define sizeOfSuperBlockField 4 
#define SBBlockSize 1

//FBM and WM 
#define bitsInABlock 8096
#define FSMBlockSize 1 
#define WMBlockSize 1


// root is a jnode
// have to add shadow jnode later
typedef struct {
uint64_t magic;
uint64_t block_size;
uint64_t fs_size;
uint64_t numberOfInodes;

uint64_t root;

} superblock_t;
