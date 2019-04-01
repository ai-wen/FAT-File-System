//Kristina Covington
//ksc3bu
//HW3
//Handles FAT16 and FAT32

#include "libFAT16.h"

//Declaring global data
struct env_data env_info;//environment info
struct fat_data fat_info; //boot sector info
struct fat32_direntry fat_root;
static int initialized = 0;
struct fat_boot boot;
unsigned long fat_begin_lba;
unsigned long cluster_begin_lba;
unsigned char BPB_SecPerClus;
unsigned long root_dir_first_cluster;
struct fat32_direntry *fat_files[128]; //Array of 128 structs for file info
int file_descriptor = -1; //To keep track of file descriptors
int isFAT16 = 0;
struct fat32_boot_struct *fat32;

/***************** UTILITY FUNCTIONS **********************/

//Seek to cluster based on cluster number
void seek_cluster(uint32_t cluster_no) {
  if( !isFAT16) {
    uint32_t firstDataSector = fat_info.fat_boot.BPB_RsvdSecCnt + (fat_info.fat_boot.BPB_NumFATs * fat32->BPB_FATSz32 );

    uint32_t firstSectorofCluster = ((cluster_no - 2) * fat_info.fat_boot.BPB_SecPerClus) + firstDataSector;
    //printf("seek to: %u\n",  firstSectorofCluster * fat_info.fat_boot.BPB_BytsPerSec);

    //seek to location of first sector in cluster given by cluster number
    if(lseek(fat_info.fs, firstSectorofCluster * fat_info.fat_boot.BPB_BytsPerSec, SEEK_SET) == -1)
      err(1, "lseek cluster_no %d\n", cluster_no);

  }
  else {

    
    uint32_t firstSectorofCluster =  fat_info.BPB_RootClus + ( ( (cluster_no - 2) * fat_info.fat_boot.BPB_SecPerClus) * fat_info.fat_boot.BPB_BytsPerSec);
    //seek to location of first sector in cluster given by cluster number
    if(lseek(fat_info.fs, firstSectorofCluster, SEEK_SET) == -1)
      err(1, "lseek cluster_no %d\n", cluster_no);
    

  }
    
}

//Initialize boot sector information and read root directory
void init() {

  const char *dev = getenv("FAT_FS_PATH"); //TODO set this to envir path in init

  uint16_t rootDirSectors;
  uint32_t fatSz, totSec, dataSec, countofClusters, first_fat;

  //Open driver at environment path
  fat_info.fs = open(dev, O_RDWR);

  //Check if valid path
  if (fat_info.fs < 0)
     err(1, "open(%s)", dev);

  //read in boot sector information
  if(read(fat_info.fs,&fat_info.fat_boot, 512) != 512)
    err(1,"read(%s)",dev);

  // Determine FAT type
  if(fat_info.fat_boot.BPB_FATSz16 != 0)
    fatSz = fat_info.fat_boot.BPB_FATSz16;
  else {
            fat32 = (struct fat32_boot_struct *) fat_info.fat_boot.extended_section;
	    fatSz = fat32->BPB_FATSz32 ;

  }
  if(fat_info.fat_boot.BPB_TotSec16 != 0)
    totSec = fat_info.fat_boot.BPB_TotSec16;
  else
    totSec = fat_info.fat_boot.BPB_TotSec32;

  //Do calculations to determine FAT type
  rootDirSectors = ((fat_info.fat_boot.BPB_RootEntCnt * 32) + (fat_info.fat_boot.BPB_BytsPerSec - 1)) / fat_info.fat_boot.BPB_BytsPerSec;
  dataSec = totSec - (fat_info.fat_boot.BPB_RsvdSecCnt + (fat_info.fat_boot.BPB_NumFATs * fatSz) + rootDirSectors);
  countofClusters = dataSec / fat_info.fat_boot.BPB_SecPerClus;

  if(countofClusters < 4085)
    err(1,"error: Volume is FAT12.\n");
  else if(countofClusters < 65525)
    isFAT16 = 1;	   

  if(!isFAT16) { //FAT32

    //Seek to FAT sector
    first_fat = fat_info.fat_boot.BPB_RsvdSecCnt * fat_info.fat_boot.BPB_BytsPerSec;
    if(lseek(fat_info.fs, first_fat, SEEK_SET) == -1)
      err(1, "lseek(%u)", first_fat);

    fat32 = (struct fat32_boot_struct *) fat_info.fat_boot.extended_section;


    fat_info.BPB_RootClus = 0xFFFFFFFF & fat32->BPB_RootClus;

    //seek to root cluster
    seek_cluster(fat_info.BPB_RootClus);
    //Read in root directory
    if( read( fat_info.fs,&fat_root, 32 ) != 32 )
      err(1,"read(%s)",dev);

    //Reset file pointer
    if(lseek(fat_info.fs, 0, SEEK_SET) == -1)
      err(1, "lseek(0)");

    // Now the root directory is in memory
    env_info.ENV_CWD = fat_root.nameext;
    env_info.ENV_CLUSTER = fat_info.BPB_RootClus;

  }
  else { //FAT16

    
    //Seek to FAT sector
    first_fat = fat_info.fat_boot.BPB_RsvdSecCnt * fat_info.fat_boot.BPB_BytsPerSec;
    if(lseek(fat_info.fs, first_fat, SEEK_SET) == -1)
      err(1, "lseek(%u)", first_fat);

	fat_info.BPB_RootClus = fat_info.fat_boot.BPB_RsvdSecCnt * fat_info.fat_boot.BPB_BytsPerSec +  fat_info.fat_boot.BPB_NumFATs * fat_info.fat_boot.BPB_FATSz16 * fat_info.fat_boot.BPB_BytsPerSec;
    
    //seek to root cluster
    lseek(fat_info.fs, fat_info.BPB_RootClus, SEEK_SET);
    //Read in root directory
    if( read( fat_info.fs,&fat_root, 32 ) != 32 )
      err(1,"read(%s)",dev);
    
    //Reset file pointer
    if(lseek(fat_info.fs, 0, SEEK_SET) == -1)
      err(1, "lseek(0)");

    // Now the root directory is in memory
    env_info.ENV_CWD = fat_root.nameext;
    env_info.ENV_CLUSTER = 2;

  }
  
  initialized=1;  
}

//get parent directory name from path
//given tmp as path store parent directory name in tempstr
void getFirstElement(char *tmp, char *tempstr){
  strcpy(tempstr, tmp);
  strcat(tempstr, "\0"); //null terminated
  //can't have first letter be a /
  if (tempstr[0] == '/' || (tempstr[0] == '.' && tempstr[0] != '.')) 
    memmove(tempstr, tempstr+1, strlen(tempstr));
}

//get remaining path (without parent)
//given str as path store remaining path in tempstr
void getRemaining(char str[], char *tempstr, char *delim){
  
  int count = 0;
  char * pch;
  strcpy(tempstr, str); //preserve str since strtok is destructive
  pch = strtok (str,delim);

  while (pch != NULL)
  {
    pch = strtok (NULL, delim);
    if( pch != NULL ) {
      if( count == 0 ) 
	strcpy( tempstr, pch);
      else {
	strcat( tempstr, delim);
	strcat( tempstr, pch);
      }

    }
    count++;
  }
  //null terminated
  strcat(tempstr, "\0");

  //This means that there was only one directory name in path
  //set tempstr to NULL since no path remaining other than parent
  if( count == 1) 
      *tempstr = 0;  
}

//Get size of directory name
//0x20 is used to pad the directory name
//Returns the actual size of the name (without padding)
int getSizeOfName( char* name) {
  int count = 0;
  int i = 0;
  for( i = 0; i < 11; i++){
    if( name[i] != 0x20 ){
      count++;
    }
  }
  return count;
}


/******************** API FUNCTIONS *****************************/

//dirname is the path to the directory
//readDir returns a newly allocated array of DIRENTRYs. It is the users
//responsibility to free the array when done. If the directory could not be found a
//null is returned.
struct fat32_direntry* OS_readDir(const char *path) {
  //initialize path if this is first call
  if (initialized==0)
    init();

    //Convert path to upper case since FAT file names are all uppercase
    char *tmp = malloc(strlen(path));
    int found=0;
    for(int i = 0; i < strlen(path); i++)
      tmp[i] = toupper(path[i]);
  
    //Check if absolute or relative path
    struct fat32_direntry *cDir;
    char pName[100];
    char t2[100];
    getFirstElement(tmp, pName);
    if(strncmp(pName, fat_root.name, getSizeOfName(fat_root.name)) == 0 || strncmp(pName, "..", 2) == 0) { //absolute
      //don't need parent path name anymore
      getRemaining(tmp, t2, "/");
    }
    else // relative
      strcpy(t2, tmp);
  
    cDir=&fat_root;
    //seek to cwd cluster, this could have been set by cd otherwise just root dir
    seek_cluster(env_info.ENV_CLUSTER);
    //read in directory entry at cwd
    if (read(fat_info.fs,(char*)cDir,32)!=32) {
      err(1,"read in error\n");
      exit(4);
    }
    
    //final path to work with 
    strcpy(tmp, t2);

    while (cDir!=0) { // while more directories

      char dName[100];
      getFirstElement(tmp, dName);

      struct fat32_direntry *cElem=cDir;

      uint32_t cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;

      //TODO prevent getting stuck here
      while(cluster_number < 0xFFFF && cElem->wrt_date > 0){ //while not last directory entry

	if (cElem->attr == 0x10 && cElem->name[0] != 0xE5 && cElem->name[0] != 0x05 ) { //This directory entry is a directory not a file and it is not empty
	  cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;
			  
	  //only checking bytes that aren't filler 0x20 in directory entry name
	  if(strncmp(dName, cElem->name, getSizeOfName(cElem->name)) == 0){//found it

	    //update cluster number since we will need it to seek to child directory
	    cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;
			  
	    char t[100];
	    strcpy(t, tmp);

	    //Get remaining path to follow
	    char t2[100];
	    getRemaining(t, t2, "/");

	    found=1;
	    strcpy( tmp, t2);

	    if (*tmp==0 || tmp == '\0') { //No more paths to follow
	      free(tmp);
	      return cDir; // We found it
	    }
					
	    break;
	  }
	}

	//Still looking for correct directory in path
	//Seek to next cluster and read in that directory
	lseek(fat_info.fs, 0, SEEK_CUR);
	if (read(fat_info.fs,(char*)cDir,32)!=32) {
	  err(1,"read in error\n");
	  exit(4);
	}
      }

      if (found==0) {
	// did not find the path element
	free(tmp);
	return 0;
      }
      else {
	//if I get here I did not find the correct it yet but I'm on the right path!
	//seek to cluster we found in parent directory
	//read in child directory
	seek_cluster(cluster_number);

	if (read(fat_info.fs,(char*)cDir,32)!=32) {
	  err(1,"read in error\n");
	  exit(4);
	}
	found = 0;//reset found
      }
    }
    
    return 0;//must not have been found if we get to here
}

//path refers to the absolute or relative path of the file.
//cd() On success returns 1 and changes the CWD to path. On failure returns -1.
// The CWD is initially "/". 
int OS_cd( const char* path){
  //initialize path if this is first call
  if (initialized==0)
    init();

    char path_for_env[100];
    int found=0;
    //Convert path to upper case since FAT file names are all uppercase
    char *tmp = malloc(strlen(path));
    for(int i = 0; i < strlen(path); i++)
      tmp[i] = toupper(path[i]);
  
    //Check if absolute or relative path
    struct fat32_direntry *cDir;
    char pName[100];
    char t2[100];
    getFirstElement(tmp, pName);
    if(strncmp(pName, fat_root.name, getSizeOfName(fat_root.name)) == 0 || strncmp(pName, "..", 2) == 0) { //absolute
      //don't need parent path name anymore
      getRemaining(tmp, t2, "/");
    }
    else {//relative
      //use cwd to find path starting at root directory
  
      if (env_info.ENV_CWD != NULL) {
	strcpy(path_for_env, env_info.ENV_CWD);
	strcat(path_for_env,"/");
	strcat(path_for_env, tmp);
	getRemaining(path_for_env, t2, "/");
      }
      else
	return 0; //env path not valid    
    } 
    //final path to work with 
    strcpy(tmp, t2);

    cDir=&fat_root;
    //seek to cwd cluster, this could have been set by cd otherwise just root dir
    seek_cluster(env_info.ENV_CLUSTER);
    //read in directory entry at cwd
    if (read(fat_info.fs,(char*)cDir,32)!=32) {
      err(1,"read in error\n");
      exit(4);
    }


    while (cDir!=0) { // while more directories

      char dName[100];
      getFirstElement(tmp, dName);

      struct fat32_direntry *cElem=cDir;

      uint32_t cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;

      //TODO prevent getting stuck here
      while(cluster_number < 0xFFFF && cElem->wrt_date > 0){ //while not last directory entry

	if (cElem->attr == 0x10  && cElem->name[0] != 0xE5 && cElem->name[0] != 0x05) { //This directory entry is a directory not a file and it is not empty
	  cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;
			  
	  //only checking bytes that aren't filler 0x20 in directory entry name
	  if(strncmp(dName, cElem->name, getSizeOfName(cElem->name)) == 0){//found it
	    //update cluster number since we will need it to seek to child directory
	    cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;
			  
	    char t[100];
	    strcpy(t, tmp);

	    //Get remaining path to follow
	    char t2[100];
	    getRemaining(t, t2, "/");

	    found=1;
	    strcpy( tmp, t2);

	    if (*tmp==0 || tmp == '\0') { //No more paths to follow
	      env_info.ENV_CWD = path_for_env;
	      env_info.ENV_CLUSTER = cluster_number;
	      free(tmp);
	      return 1;// We found it
	    }
					
	    break;
	  }
	}

	//Still looking for correct directory in path
	//Seek to next cluster and read in that directory
	lseek(fat_info.fs, 0, SEEK_CUR);
	if (read(fat_info.fs,(char*)cDir,32)!=32) {
	  err(1,"read in error\n");
	  exit(4);
	}
      }

      if (found==0) {
	// did not find the path element
	free(tmp);
	return -1;
      }
      else {
	//if I get here I did not find the correct it yet but I'm on the right path!
	//seek to cluster we found in parent directory
	//read in child directory
	seek_cluster(cluster_number);

	if (read(fat_info.fs,(char*)cDir,32)!=32) {
	  err(1,"read in error\n");
	  exit(4);
	}
	found = 1; //reset found
      }
    }

    return -1;//must not have been found if we get here
}

//All files will be opened Read/write.
//path refers to the absolute or relative path of the file.
//open() On success returns the file descriptor to be used.
//On failure returns -1;
int OS_open(const char *path) {
 //initialize path if this is first call
  if (initialized==0)
    init();

	  //Convert path to upper case since FAT file names are all uppercase
	  char *tmp = malloc(strlen(path));
	  int found=0;
	  for(int i = 0; i < strlen(path); i++)
		tmp[i] = toupper(path[i]);
	  
	  //Check if absolute or relative path
	  struct fat32_direntry *cDir;
	  char pName[100];
	  char t2[100];
	  getFirstElement(tmp, pName);
	  if(strncmp(pName, fat_root.name, getSizeOfName(fat_root.name)) == 0 || strncmp(pName, "..", 2) == 0) { //absolute
		//don't need parent path name anymore
		getRemaining(tmp, t2, "/");
	  }
	  else // relative
		strcpy(t2, tmp);
	  
	  cDir=&fat_root;
	  //seek to cwd cluster, this could have been set by cd otherwise just root dir
	  seek_cluster(env_info.ENV_CLUSTER);

	  //read in directory entry at cwd
	  if (read(fat_info.fs,(char*)cDir,32)!=32) {
		err(1,"read in error\n");
		exit(4);
	  }
	 
	  //final path to work with 
	  strcpy(tmp, t2);

	  while (cDir!=0) { // while more directories

		char dName[100];
		getFirstElement(tmp, dName);

		struct fat32_direntry *cElem=cDir;

		uint32_t cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;
		//TODO prevent getting stuck here
		while(cluster_number < 0xFFFF && cElem->wrt_date > 0){ //while not last directory entry

		  if (cElem->attr == 0x10 && cElem->name[0] != 0xE5 && cElem->name[0] != 0x05 ) { //This directory entry is a directory not a file and it is not empty
		cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;
				  
		//only checking bytes that aren't filler 0x20 in directory entry name
		if(strncmp(dName, cElem->name, getSizeOfName(cElem->name)) == 0){//found it

		  //update cluster number since we will need it to seek to child directory
		  cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;
				  
		  char t[100];
		  strcpy(t, tmp);

		  //Get remaining path to follow
		  char t2[100];
		  getRemaining(t, t2, "/");

		  found=1;
		  strcpy( tmp, t2);

		  if (*tmp==0 || tmp == '\0') { //No more paths to follow
			err(1,"path was to directory not file\n");
			free(tmp);
			return -1; // We did not find it
		 }
						
		 break;
		}
	       }
	       else { //Find correct file now

		  //only checking bytes that aren't filler 0x20 in directory entry name
		if(strncmp(dName, cElem->nameext, getSizeOfName(cElem->nameext)) == 0){//found it

		  //update cluster number since we will need it to seek to child directory
		  cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;
				  
		  char t[100];
		  strcpy(t, tmp);

		  //Get remaining path to follow
		  char t2[100];
		  getRemaining(t, t2, "/");

		  found=1;
		  strcpy( tmp, t2);

		  if (*tmp==0 || tmp == '\0') { //No more paths to follow
			//copy memory from cDir found to our fat file array in memory for later use
			file_descriptor++;
			fat_files[file_descriptor] = malloc(sizeof(cDir));
			//memcpy(fat_files[file_descriptor], cDir, sizeof(cDir));
			lseek(fat_info.fs, -32, SEEK_CUR);
			if (read(fat_info.fs,(char*)fat_files[file_descriptor],32)!=32) {
			  err(1,"read in error\n");
			  exit(4);
			}
			free(tmp);
			//TODO return file descriptor
			return file_descriptor; // We found it
		 }
						
		 break;
		}

		  }
		  //Still looking for correct directory in path
		  //Seek to next cluster and read in that directory
		  lseek(fat_info.fs, 0, SEEK_CUR);
		  if (read(fat_info.fs,(char*)cDir,32)!=32) {
		err(1,"read in error\n");
		exit(4);
		  }
		}

		if (found==0) {
		  // did not find the path element
		  free(tmp);
		  return -1;
		}
		else {
		  //if I get here I did not find the correct it yet but I'm on the right path!
		  //seek to cluster we found in parent directory
		  //read in child directory
		  seek_cluster(cluster_number);

		  if (read(fat_info.fs,(char*)cDir,32)!=32) {
		err(1,"read in error\n");
		exit(4);
		  }
		  found = 0; // reset found
		}
	  }
	  return -1;//must not have been found if we get to here
}

//fd refers to a previously opened file descriptor
//close() On success returns 1, on failure returns -1. 
int OS_close(int fd) {

  //check current file descriptor position to determine if valid
  if( fd > file_descriptor )
    return -1;

  //clear memory for this file
  memset(fat_files[fd], 0, 32);
    
  return 1;
}

//fildes refers to a previously opened file.
//buf refers to a buffer of at least nbyte
//nbyte is the number of bytes to read.
//offset is the offset in the file to begin reading.
//read() returns the number of bytes read if positive. Otherwise returns -1 
int OS_read(int fildes, void *buf, int nbyte, int offset) {

    //can't read a file of size zero
    if(fat_files[fildes]->size <= 0)
      return -1;

    //calculate number of bytes we can read given file size
    int res = nbyte - offset;
    if( fat_files[fildes]->size < res )
      res = fat_files[fildes]->size;
  
    //get cluster number location of file data
    uint32_t cluster_number = ( fat_files[fildes]->cluster_hi << 16) |  fat_files[fildes]->cluster_lo;

    //seek to correct location
    seek_cluster(cluster_number);
    lseek(fat_info.fs, offset, SEEK_CUR);

    //read in data
    char *cDir = malloc(nbyte);
    if (read(fat_info.fs,(char*)cDir,nbyte)!=nbyte) {
      err(1,"read in error\n");
      exit(4);
    }
  
    //memcopy data to buf
    memcpy(buf, cDir, nbyte);
    return res;

}

//path refers to an absolute or relative path.
//OS_creat returns 1 if the file is created, -1 if the path is invalid,
//and -2 if the final element of the path already exists.  
int OS_creat(const char *path) {

  //initialize path if this is first call
  if (initialized==0)
    init();

  uint32_t last_cluster_number;

    //Convert path to upper case since FAT file names are all uppercase
    char *tmp = malloc(strlen(path));
    int found=0;
    for(int i = 0; i < strlen(path); i++)
      tmp[i] = toupper(path[i]);
  
    //Check if absolute or relative path
    struct fat32_direntry *cDir;
    char pName[100];
    char t2[100];
    getFirstElement(tmp, pName);
    if(strncmp(pName, fat_root.name, getSizeOfName(fat_root.name)) == 0 || strncmp(pName, "..", 2) == 0) { //absolute
      //don't need parent path name anymore
      getRemaining(tmp, t2, "/");
    }
    else // relative
      strcpy(t2, tmp);
  
    cDir=&fat_root;
    //seek to cwd cluster, this could have been set by cd otherwise just root dir
    seek_cluster(env_info.ENV_CLUSTER);
    //read in directory entry at cwd
    if (read(fat_info.fs,(char*)cDir,32)!=32) {
      err(1,"read in error\n");
      exit(4);
    }
    
    //final path to work with 
    strcpy(tmp, t2);

    while (cDir!=0) { // while more directories

      char dName[100];
      getFirstElement(tmp, dName);

      struct fat32_direntry *cElem=cDir;

      uint32_t cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;

      //TODO prevent getting stuck here
      while(cluster_number < 0xFFFF && cElem->wrt_date > 0){ //while not last directory entry

	if (cElem->attr == 0x10  && cElem->name[0] != 0xE5 && cElem->name[0] != 0x05 ) { //This directory entry is a directory not a file and it is not empty
	  cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;
			  
	  //only checking bytes that aren't filler 0x20 in directory entry name
	  if(strncmp(dName, cElem->name, getSizeOfName(cElem->name)) == 0){//found it

	    //update cluster number since we will need it to seek to child directory
	    cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;
	    last_cluster_number = cluster_number;
	    char t[100];
	    strcpy(t, tmp);

	    //Get remaining path to follow
	    char t2[100];
	    getRemaining(t, t2, "/");

	    found=1;
	    strcpy( tmp, t2);

	    if (*tmp==0 || tmp == '\0') { //No more paths to follow
	      free(tmp);
	      return -2; // Already exists
	    }
					
	    break;
	  }
	}

	//Still looking for correct directory in path
	//Seek to next cluster and read in that directory

	  int offset = lseek(fat_info.fs, 0, SEEK_CUR);
	  //printf("lseek: %u\n", offset);
	  if (read(fat_info.fs,(char*)cDir,32)!=32) {
	    err(1,"read in error\n");
	    exit(4);
	  }
	
      }
      if (found==0) {
	//dir not found
	//test whether we are at last path
	char t[100];
	strcpy(t, tmp);

	//Get remaining path to follow
	char t2[100];
	getRemaining(t, t2, "/");

	
	if (*t2==0 || t2 == '\0') { // we are at last path
	  //create directory
	  struct fat32_direntry *newDir = malloc(32);
	  last_cluster_number++;
	  time_t t = time(NULL);
	  struct tm tm = *localtime(&t);

	  //TODO set values properly
	  memcpy(newDir->nameext, tmp, 11);
	  newDir->attr = 0;
	  newDir->nt_res = 0;
	  newDir->crt_time_tenth = 0;
	  newDir->crt_date = 0; 
	  newDir->lst_acc_date = 0;
	  newDir->cluster_hi = 0;
	  newDir->wrt_time = tm.tm_sec << 4 | ( tm.tm_min << 4 | tm.tm_hour );
	  newDir->wrt_date = tm.tm_mday << 4 | ( tm.tm_mon << 4 | (tm.tm_year - 1980)  );
	  newDir->cluster_lo = last_cluster_number & 0xFFFF;
	  newDir->size = 32;

	  free(tmp);
	  
	  if( write(fat_info.fs, newDir, sizeof(newDir)) > -1 )
	    return 1;
	  else {
	    err(1,"write failed\n");
	    //printf("error! %s\n", strerror(errno));
	    exit(2);
	    return -1;
	  }
	  
	}
	else // invalid path
	  return -1;
	
      }
      else {
	//if I get here I did not find the correct it yet but I'm on the right path!
	//seek to cluster we found in parent directory
	//read in child directory
	seek_cluster(cluster_number);

	if (read(fat_info.fs,(char*)cDir,32)!=32) {
	  err(1,"read in error\n");
	  exit(4);
	}
	found = 0;//reset found
      }
    }
    return -1;//path must be invalid if we get to here

}

//path refers to an absolute or relative path.
//OS_mkdir returns 1 if the directory is created, -1 if the path is invalid,
//and -2 if the final element of the path already exists. 
int OS_mkdir(const char *path){

  //initialize path if this is first call
  if (initialized==0)
    init();

  uint32_t last_cluster_number;

    //Convert path to upper case since FAT file names are all uppercase
    char *tmp = malloc(strlen(path));
    int found=0;
    for(int i = 0; i < strlen(path); i++)
      tmp[i] = toupper(path[i]);
  
    //Check if absolute or relative path
    struct fat32_direntry *cDir;
    char pName[100];
    char t2[100];
    getFirstElement(tmp, pName);
    if(strncmp(pName, fat_root.name, getSizeOfName(fat_root.name)) == 0 || strncmp(pName, "..", 2) == 0) { //absolute
      //don't need parent path name anymore
      getRemaining(tmp, t2, "/");
    }
    else // relative
      strcpy(t2, tmp);
  
    cDir=&fat_root;
    //seek to cwd cluster, this could have been set by cd otherwise just root dir
    seek_cluster(env_info.ENV_CLUSTER);
    //read in directory entry at cwd
    if (read(fat_info.fs,(char*)cDir,32)!=32) {
      err(1,"read in error\n");
      exit(4);
    }
    
    //final path to work with 
    strcpy(tmp, t2);

    while (cDir!=0) { // while more directories

      char dName[100];
      getFirstElement(tmp, dName);

      struct fat32_direntry *cElem=cDir;

      uint32_t cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;

      //TODO prevent getting stuck here
      while(cluster_number < 0xFFFF && cElem->wrt_date > 0){ //while not last directory entry

	if (cElem->attr == 0x10  && cElem->name[0] != 0xE5 && cElem->name[0] != 0x05 ) { //This directory entry is a directory not a file and it is not empty
	  cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;
			  
	  //only checking bytes that aren't filler 0x20 in directory entry name
	  if(strncmp(dName, cElem->name, getSizeOfName(cElem->name)) == 0){//found it

	    //update cluster number since we will need it to seek to child directory
	    cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;
	    last_cluster_number = cluster_number;
	    char t[100];
	    strcpy(t, tmp);

	    //Get remaining path to follow
	    char t2[100];
	    getRemaining(t, t2, "/");

	    found=1;
	    strcpy( tmp, t2);

	    if (*tmp==0 || tmp == '\0') { //No more paths to follow
	      free(tmp);
	      return -2; // Already exists
	    }
					
	    break;
	  }
	}

	//Still looking for correct directory in path
	//Seek to next cluster and read in that directory

	  int offset = lseek(fat_info.fs, 0, SEEK_CUR);
	  //printf("lseek: %u\n", offset);
	  if (read(fat_info.fs,(char*)cDir,32)!=32) {
	    err(1,"read in error\n");
	    exit(4);
	  }
	
      }
      if (found==0) {
	//dir not found
	//test whether we are at last path
	char t[100];
	strcpy(t, tmp);

	//Get remaining path to follow
	char t2[100];
	getRemaining(t, t2, "/");

	
	if (*t2==0 || t2 == '\0') { // we are at last path
	  //create directory
	  struct fat32_direntry *newDir = malloc(32);
	  last_cluster_number++;
	  time_t t = time(NULL);
	  struct tm tm = *localtime(&t);

	  //TODO set values properly
	  memcpy(newDir->nameext, tmp, 11);
	  newDir->attr = 0x10;
	  newDir->nt_res = 0;
	  newDir->crt_time_tenth = 0;
	  newDir->crt_date = 0; 
	  newDir->lst_acc_date = 0;
	  newDir->cluster_hi = 0;
	  newDir->wrt_time = tm.tm_sec << 4 | ( tm.tm_min << 4 | tm.tm_hour );
	  newDir->wrt_date = tm.tm_mday << 4 | ( tm.tm_mon << 4 | (tm.tm_year - 1980)  );
	  newDir->cluster_lo = last_cluster_number & 0xFFFF;
	  newDir->size = 0;

	  free(tmp);
	  
	  if( write(fat_info.fs, newDir, sizeof(newDir)) > -1 )
	    return 1;
	  else {
	    err(1,"write failed\n");
	    //printf("error! %s\n", strerror(errno));
	    exit(2);
	    return -1;
	  }
	  
	}
	else // invalid path
	  return -1;
	
      }
      else {
	//if I get here I did not find the correct it yet but I'm on the right path!
	//seek to cluster we found in parent directory
	//read in child directory
	seek_cluster(cluster_number);

	if (read(fat_info.fs,(char*)cDir,32)!=32) {
	  err(1,"read in error\n");
	  exit(4);
	}
	found = 0;//reset found
      }
    }
    return -1;//path must be invalid if we get to here

}

//path refers to an absolute or relative path.
//OS_rm returns 1 if the file is removed, -1 if the path is invalid,
// and -2 file is a directory.   
int OS_rmdir(const char *path) {

  //initialize path if this is first call
  if (initialized==0)
    init();

  uint32_t last_cluster_number;

    //Convert path to upper case since FAT file names are all uppercase
    char *tmp = malloc(strlen(path));
    int found=0;
    for(int i = 0; i < strlen(path); i++)
      tmp[i] = toupper(path[i]);
  
    //Check if absolute or relative path
    struct fat32_direntry *cDir;
    char pName[100];
    char t2[100];
    getFirstElement(tmp, pName);
    if(strncmp(pName, fat_root.name, getSizeOfName(fat_root.name)) == 0 || strncmp(pName, "..", 2) == 0) { //absolute
      //don't need parent path name anymore
      getRemaining(tmp, t2, "/");
    }
    else // relative
      strcpy(t2, tmp);
  
    cDir=&fat_root;
    //seek to cwd cluster, this could have been set by cd otherwise just root dir
    seek_cluster(env_info.ENV_CLUSTER);
    //read in directory entry at cwd
    if (read(fat_info.fs,(char*)cDir,32)!=32) {
      err(1,"read in error\n");
      exit(4);
    }
    
    //final path to work with 
    strcpy(tmp, t2);

    while (cDir!=0) { // while more directories

      char dName[100];
      getFirstElement(tmp, dName);

      struct fat32_direntry *cElem=cDir;

      uint32_t cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;

      //TODO prevent getting stuck here
      while(cluster_number < 0xFFFF && cElem->wrt_date > 0){ //while not last directory entry

	if (cElem->attr == 0x10  && cElem->name[0] != 0xE5 && cElem->name[0] != 0x05 ) { //This directory entry is a directory not a file and it is not empty
	  cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;
			  
	  //only checking bytes that aren't filler 0x20 in directory entry name
	  if(strncmp(dName, cElem->name, getSizeOfName(cElem->name)) == 0){//found it

	    //update cluster number since we will need it to seek to child directory
	    cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;
	    last_cluster_number = cluster_number;
	    char t[100];
	    strcpy(t, tmp);

	    //Get remaining path to follow
	    char t2[100];
	    getRemaining(t, t2, "/");

	    found=1;
	    strcpy( tmp, t2);

	    if (*tmp==0 || tmp == '\0') { //No more paths to follow

	      //dir not found
	      //test whether we are at last path
	      char t[100];
	      strcpy(t, tmp);

	      //Get remaining path to follow
	      char t2[100];
	      getRemaining(t, t2, "/");

	
	      if (*t2==0 || t2 == '\0') { // we are at last path
		//create directory
		last_cluster_number++;


		if( cDir->cluster_lo != 0 )
		  return -3; //directory is not empty
		
		//TODO set values properly
		cDir->nameext[0] = 0xE5; //directory is indicated as empty if name[0] = 0xE5
       

		free(tmp);
		seek_cluster(cluster_number);//seek to location of directory
		if( write(fat_info.fs, cDir, sizeof(cDir)) > -1 ) //write changed values
		  return 1; //directory removed (made invalid)
		else {
		  err(1,"write failed\n");
		  //printf("error! %s\n", strerror(errno));
		  exit(2);
		  return -1; //failed to remove directory
		}
	  
	      }
	      
	    }
					
	    break;
	  }
	}
	else {
	//check to see if there is a file with same name that isnt a directory
	 //only checking bytes that aren't filler 0x20 in directory entry name
	  if(strncmp(dName, cElem->name, getSizeOfName(cElem->name)) == 0){//found it
	    //test whether we are at last path
	    char t[100];
	    strcpy(t, tmp);

	    //Get remaining path to follow
	    char t2[100];
	    getRemaining(t, t2, "/");

	    if (*t2==0 || t2 == '\0') { // we are at last path

	      return -2; //path does not refer to a directory
	      
	    }


	  }

	}

	//Still looking for correct directory in path
	//Seek to next cluster and read in that directory


	  int offset = lseek(fat_info.fs, 0, SEEK_CUR);
	  //printf("lseek: %u\n", offset);
	  if (read(fat_info.fs,(char*)cDir,32)!=32) {
	    err(1,"read in error\n");
	    exit(4);
	  }
	
      }
      if (found==0) {

	//path is invalid
	return -1;
	
      }
      else {
	//if I get here I did not find the correct it yet but I'm on the right path!
	//seek to cluster we found in parent directory
	//read in child directory
	seek_cluster(cluster_number);

	if (read(fat_info.fs,(char*)cDir,32)!=32) {
	  err(1,"read in error\n");
	  exit(4);
	}
	found = 0;//reset found
      }
    }
    return -1;//path must be invalid if we get to here





}


//path refers to an absolute or relative path.
//OS_rm returns 1 if the file is removed, -1 if the path is invalid,
// and -2 file is a directory. 
int OS_rm(const char *path) {

  //initialize path if this is first call
  if (initialized==0)
    init();

  uint32_t last_cluster_number;

    //Convert path to upper case since FAT file names are all uppercase
    char *tmp = malloc(strlen(path));
    int found=0;
    for(int i = 0; i < strlen(path); i++)
      tmp[i] = toupper(path[i]);
  
    //Check if absolute or relative path
    struct fat32_direntry *cDir;
    char pName[100];
    char t2[100];
    getFirstElement(tmp, pName);
    if(strncmp(pName, fat_root.name, getSizeOfName(fat_root.name)) == 0 || strncmp(pName, "..", 2) == 0) { //absolute
      //don't need parent path name anymore
      getRemaining(tmp, t2, "/");
    }
    else // relative
      strcpy(t2, tmp);
  
    cDir=&fat_root;
    //seek to cwd cluster, this could have been set by cd otherwise just root dir
    seek_cluster(env_info.ENV_CLUSTER);
    //read in directory entry at cwd
    if (read(fat_info.fs,(char*)cDir,32)!=32) {
      err(1,"read in error\n");
      exit(4);
    }
    
    //final path to work with 
    strcpy(tmp, t2);

    while (cDir!=0) { // while more directories

      char dName[100];
      getFirstElement(tmp, dName);

      struct fat32_direntry *cElem=cDir;

      uint32_t cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;

      //TODO prevent getting stuck here
      while(cluster_number < 0xFFFF && cElem->wrt_date > 0){ //while not last directory entry

	if (cElem->attr == 0x10  && cElem->name[0] != 0xE5 && cElem->name[0] != 0x05 ) { //This directory entry is a directory not a file and it is not empty
	  cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;
			  
	  //only checking bytes that aren't filler 0x20 in directory entry name
	  if(strncmp(dName, cElem->name, getSizeOfName(cElem->name)) == 0){//found it

	    //update cluster number since we will need it to seek to child directory
	    cluster_number = (cElem->cluster_hi << 16) | cElem->cluster_lo ;
	    last_cluster_number = cluster_number;
	    char t[100];
	    strcpy(t, tmp);

	    //Get remaining path to follow
	    char t2[100];
	    getRemaining(t, t2, "/");

	    found=1;
	    strcpy( tmp, t2);

	    if (*tmp==0 || tmp == '\0') { //No more paths to follow

	      //dir not found
	      //test whether we are at last path
	      char t[100];
	      strcpy(t, tmp);

	      //Get remaining path to follow
	      char t2[100];
	      getRemaining(t, t2, "/");

	
	      if (*t2==0 || t2 == '\0') { // we are at last path

		return -2; //path refers to a directory not a file
	  
	      }
	      
	    }
					
	    break;
	  }
	}
	else {
	//check to see if there is a file with same name that isnt a directory
	 //only checking bytes that aren't filler 0x20 in directory entry name
	  if(strncmp(dName, cElem->name, getSizeOfName(cElem->name)) == 0){//found it
	    //test whether we are at last path
	    char t[100];
	    strcpy(t, tmp);

	    //Get remaining path to follow
	    char t2[100];
	    getRemaining(t, t2, "/");

	    if (*t2==0 || t2 == '\0') { // we are at last path

	      //create directory
		last_cluster_number++;

		
		//TODO set values properly
		cDir->nameext[0] = 0xE5; //directory is indicated as empty if name[0] = 0xE5
       

		free(tmp);
		seek_cluster(cluster_number);//seek to location of directory
		if( write(fat_info.fs, cDir, sizeof(cDir)) > -1 ) //write changed values
		  return 1; //file removed (made invalid)
		else {
		  err(1,"write failed\n");
		  //printf("error! %s\n", strerror(errno));
		  exit(2);
		  return -1; //failed to remove file
		}
	      
	    }


	  }

	}

	//Still looking for correct directory in path
	//Seek to next cluster and read in that directory


	  int offset = lseek(fat_info.fs, 0, SEEK_CUR);
	  //printf("lseek: %u\n", offset);
	  if (read(fat_info.fs,(char*)cDir,32)!=32) {
	    err(1,"read in error\n");
	    exit(4);
	  }
	
      }
      if (found==0) {

	//path is invalid
	return -1;
	
      }
      else {
	//if I get here I did not find the correct it yet but I'm on the right path!
	//seek to cluster we found in parent directory
	//read in child directory
	seek_cluster(cluster_number);

	if (read(fat_info.fs,(char*)cDir,32)!=32) {
	  err(1,"read in error\n");
	  exit(4);
	}
	found = 0;//reset found
      }
    }
    return -1;//path must be invalid if we get to here





}

//filedes is the file descriptor returned from a successful open call.
//buf is a pointer to a buffer at least nbytes in size
//nbytes is the number of bytes to write
//offset is where to start writing in the file. 
int OS_write(int fildes, const void *buf, int nbytes, int offset){
    
    //get cluster number location of file data
    uint32_t cluster_number = ( fat_files[fildes]->cluster_hi << 16) |  fat_files[fildes]->cluster_lo;

 
    //seek to correct location
    seek_cluster(cluster_number);
    lseek(fat_info.fs, offset, SEEK_CUR);

    //write data
    if (write(fat_info.fs,buf,nbytes)!=nbytes) {
      err(1,"write error\n");
      exit(4);
    }

    return nbytes;


}
