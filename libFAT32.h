#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdint.h>
#include <err.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)

/***************** Structs **********************/

//fat boot 32
struct fat32_boot_struct {
	uint32_t BPB_FATSz32 ; //sectors per fat
	uint16_t BPB_ExtFlags; // fat flags
	uint16_t BPB_FSVer; //version
	uint32_t BPB_RootClus; //root cluster
	uint16_t BPB_FSInfo; //info sector
	uint16_t BPB_BkBootSec; //backup sector
	uint8_t	BPB_Reserved[12]; //reserved
	uint8_t	BS_DrvNum; //driver num
	uint8_t	BS_Reserved1; //reserved
	uint8_t	BS_BootSig; //boot signature
	uint32_t BS_VolID; //volume id
	char BS_VolLab[11]; //volume label
	char BS_FilSysType[8]; //file system type
} __attribute__ ((__packed__));

//fat boot 16
struct fat_boot_fat16 {
        char BS_DrvNum; //driver num
        char BS_Reserved1; //reserved
        char BS_BootSig; //boot signature 
	uint32_t BS_VolID; //volume id
  	char BS_VolLab[11]; //volume label
	char BS_FilSysType[8]; //file system type
} __attribute__ ((__packed__));

//Fat boot
struct fat_boot {
	uint8_t jump_boot[3];
	char name[8];
	uint16_t BPB_BytsPerSec; //bytes per sector
	uint8_t BPB_SecPerClus; //sectors per cluster
	uint16_t BPB_RsvdSecCnt;//reserved sectors
	uint8_t BPB_NumFATs;//num fats
	uint16_t BPB_RootEntCnt; //max root entries
	uint16_t BPB_TotSec16; //tot sectors
	uint8_t BPB_Media; //media 
	uint16_t BPB_FATSz16; //fat size
	uint16_t BPB_SecPerTrk; //sectors per track
	uint16_t BPB_NumHeads; //num heads
	uint32_t BPB_HiddSec; //hidden sectors
	uint32_t BPB_TotSec32; //total sectors
	//cast to specific BS_FilSysType once know BS_FilSysType of FAT.
	unsigned char extended_section[54];
} __attribute__ ((__packed__));


struct fat32_direntry {
	union {
		struct {
			char name[8]; //name
			char ext[3]; //extension
		};
		char nameext[11]; //DIR_Name
	};
	uint8_t attr; //DIR_Attr
	uint8_t nt_res; //DIR_NTRes
	uint8_t crt_time_tenth; //DIR_CrtTimeTenth
	uint16_t crt_time; //DIR_CrtTime
	uint16_t crt_date; //DIR_CrtDate
	uint16_t lst_acc_date; //DIR_LstAccDate
	uint16_t cluster_hi; //DIR_FstClusHI
	uint16_t wrt_time; //DIR_WrtTime
	uint16_t wrt_date; //DIR_WrtDate
	uint16_t cluster_lo; //DIR_FstClusLO
	uint32_t size; //DIR_FileSize
} __attribute__ ((__packed__));

//file system data
struct fat_data {
	const char *dev;
	int	fs;
	struct fat_boot fat_boot;
	uint32_t BPB_RootClus;
};

//environment data
struct env_data {
  //Path to driver from environment
  char *ENV_CWD; //cwd
  uint32_t ENV_CLUSTER; //cwd cluster
};

/***************** UTILITY FUNCTIONS **********************/


//Seek to cluster based on cluster number
void seek_cluster(uint32_t cluster_no);

//Initialize boot sector information and read root directory
void init();

//get parent directory name from path
//given tmp as path store parent directory name in tempstr
void getFirstElement(char *tmp, char *tempstr);

//get remaining path (without parent)
//given str as path store remaining path in tempstr
void getRemaining(char str[], char *tempstr, char *delim);

//Get size of directory name
//0x20 is used to pad the directory name
//Returns the actual size of the name (without padding)
int getSizeOfName( char* name);


/******************** API FUNCTIONS *****************************/


//dirname is the path to the directory
//readDir returns a newly allocated array of DIRENTRYs. It is the users
//responsibility to free the array when done. If the directory could not be found a
//null is returned.
struct fat32_direntry* OS_readDir(const char *path);

//path refers to the absolute or relative path of the file.
//cd() On success returns 1 and changes the CWD to path. On failure returns -1.
// The CWD is initially "/". 
int OS_cd( const char* path);

//All files will be opened Read/write.
//path refers to the absolute or relative path of the file.
//open() On success returns the file descriptor to be used.
//On failure returns -1;
int OS_open(const char *path);

//fd refers to a previously opened file descriptor
//close() On success returns 1, on failure returns -1. 
int OS_close(int fd);

//fildes refers to a previously opened file.
//buf refers to a buffer of at least nbyte
//nbyte is the number of bytes to read.
//offset is the offset in the file to begin reading.
//read() returns the number of bytes read if positive. Otherwise returns -1 
int OS_read(int fildes, void *buf, int nbyte, int offset);

//path refers to an absolute or relative path.
//OS_creat returns 1 if the file is created, -1 if the path is invalid,
//and -2 if the final element of the path already exists. 
int OS_creat(const char *path);

//path refers to an absolute or relative path.
//OS_mkdir returns 1 if the directory is created, -1 if the path is invalid,
//and -2 if the final element of the path already exists. 
int OS_mkdir(const char *path);

//path refers to an absolute or relative path.
//OS_rmdir returns 1 if the directory is removed, -1 if the path is invalid,
//-2 if the path does not refer to a file,
//and -3 if the directory is not empty. 
int OS_rmdir(const char *path);

//path refers to an absolute or relative path.
//OS_rm returns 1 if the file is removed, -1 if the path is invalid,
// and -2 file is a directory. 
int OS_rm(const char *path);


//filedes is the file descriptor returned from a successful open call.
//buf is a pointer to a buffer at least nbytes in size
//nbytes is the number of bytes to write
//offset is where to start writing in the file. 
int OS_write(int fildes, const void *buf, int nbytes, int offset);
