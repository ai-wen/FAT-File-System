all: libFAT32.c libFAT32.c libFAT16.c libFAT16.h
	gcc -Wall -c -fpic libFAT16.h libFAT16.c
	gcc -Wall -c -fpic libFAT32.h libFAT32.c
	gcc -shared -o libFAT16.so libFAT16.o
	gcc -shared -o libFAT32.so libFAT32.o
