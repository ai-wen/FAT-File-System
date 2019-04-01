# FAT-File-System
Implementation of the FAT16 and FAT32 file system write APIs in C language.

File systems provide efficient and convenient access to
disk by allowing data to be stored, located and retrieved easily. The File Allocation Table (FAT)
file system was introduced in the Microsoft MS-DOS operating system in the 1980s. It is simply
a singly-linked list of clusters in a large table. 

Implementation of OS_cd:
  
  OS_cd() changes the current working directory to the given path. I maintain
  global variables to keep track of the user’s current working directory and cluster number. I
  convert the path to uppercase since FAT file names are uppercase. Then, I check to see if the
  path is absolute or relative by comparing the first element to the root directory name. I utilize
  a utility function I implemented to get this first element. After that, I iterate through the
  directories in the file system. When I find a directory, I check to see if the name matches the
  first element of my path. If it does, then I keep following that path by seeking to its first
  subdirectory based on its cluster number and updating the variables used to determine which
  directory to look for next. I know I am done if I run out of paths to follow. If I have found the
  last path I update the environment variables and exit the loop.
  
Implementation of OS_close:

  I keep track of my file descriptors by making them the index of my 128 file struct
  array. OS_close() checks if the last created file descriptor number is less that the file descriptor I
  am trying to close. If this is the case, this is invalid and I return a failure code. Otherwise I use
  memset() to clear memory for the file indicated by the file descriptor index number and return
  a success code.

Implementation of OS_creat:

  OS_creat() creates a new file in a directory given the path. I maintain global
  variables to keep track of the user’s current working directory and cluster number. I convert
  the path to uppercase since FAT file names are uppercase. Then, I check to see if the path is
  absolute or relative by comparing the first element to the root directory name. I utilize a utility
  function I implemented to get this first element. After that, I iterate through the directories in
  the file system. When I find a directory, I check to see if the name matches the first element of
  my path. If it does, then I keep following that path by seeking to its first subdirectory based on
  its cluster number and updating the variables used to determine which directory to look for
  next. I know I am done if I run out of paths to follow. If I have found the path I then search for
  an empty directory entry and create my new directory entry by setting all the necessary
  variables, and write it to the offset that I am at within the FAT_FS_PATH file. If there already
  exists a file with the name or I never find the correct path I was given then I return an error
  code.
  
Implementation of OS_read:

  OS_read() first checks if the file size is greater than zero since we can not read a
  file of size zero. Then it calculates the number of bytes we can read given the file size and offset
  position given. We locate the cluster number where the file data is stored by accessing our file
  struct in the file array given the file descriptor as an index. We seek to this location, read the file
  and memcpy() the number of bytes we calculated into the given buffer.

Implementation of OS_readDir:

  OS_readDir() is very similar to OS_cd except we don’t change the current
  working directory. I convert the path to uppercase since FAT file names are uppercase.
  Then, I check to see if the path is absolute or relative by comparing the first element to
  the root directory name. I utilize a utility function I implemented to get this first
  element. After that, I iterate through the directories in the file system. When I find a
  directory, I check to see if the name matches the first element of my path. If it does,
  then I keep following that path by seeking to its first subdirectory based on its cluster
  number and updating the variables used to determine which directory to look for next. I
  know I am done if I run out of paths to follow. If I have found the last path, I return the
  directory entry I read in.
  
Implementation of OS_write:

  OS_write () locates the cluster number where the file data is stored by accessing
  our file struct in the file array given the file descriptor as an index. We seek to this location
  within the FAT_FS_PATH file, given the offset, and write to the file the buffer for the number of
  bytes we were given.
  
Implementation of OS_rm:
  OS_rm() is very similar to OS_creat() except we are looking for an existing file
  and we change the directory entry’s name[0] to 0xE5 to indicate that it is an invalid
  directory entry that should not be read. We check the attribute to determine if it is a file
  or directory so we can return the correct exit code before we write.
  
Implementation of OS_mkdir:

  OS_mkdir() is very similar to OS_creat() except we once we mark the new
  directory entry’s attribute as 0x10 to indicate that it is a directory when we write it.

Implementation of OS_rmdir:
  
  OS_rmdir() is very similar to OS_rm() except we are looking for a directory and
  not a file. We check the directory’s cluster number to know whether or not the directory is
  empty so we can return a failure exit code or not.
  


