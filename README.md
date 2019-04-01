# FAT-File-System
Implementation of the FAT16 and FAT32 file system write APIs in C language.

File systems provide efficient and convenient access to
disk by allowing data to be stored, located and retrieved easily. The File Allocation Table (FAT)
file system was introduced in the Microsoft MS-DOS operating system in the 1980s. It is simply
a singly-linked list of clusters in a large table. 

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
  


