/************************
 * util.c
 *
 * utility functions
 *
 ************************/

#include "util.h"
#include <stdlib.h>

/***************
 * These functions are just some handy file functions.
 * We have not yet covered opening and reading from files in C,
 * so we're saving you the pain of dealing with it, for now.
 *******/
FILE* file_open(char* filename) {
	FILE* fp = fopen(filename, "r");
	if(fp == NULL) {
		fprintf(stderr, "Error opening file %s, abort.\n", filename);
		exit(1);
	}
	return fp;
}

/*******************
 * To use this function properly, create a char* and malloc 
 * 160 bytes for it. Then pass that char* in as the argument
 ******************/
char* file_getline(char* buffer, FILE* fp) {
	buffer = fgets(buffer, 160, fp);
	return buffer;
}
