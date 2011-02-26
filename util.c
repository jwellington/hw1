/************************
 * util.c
 *
 * utility functions
 *
 ************************/

#include "util.h"
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>

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

//Returns a string representation of the current time and date.
char* current_time()
{
    time_t rawtime;
    time(&rawtime);
    char* timestring = ctime(&rawtime);
    return timestring;
}

//Returns a struct tm* representation of the current time.
time_t get_time()
{
    time_t rawtime;
    time(&rawtime);
    return rawtime;
}

//Returns the modification time of the inputted file.
//You should make sure you give it an actual file.
time_t last_modified(char* filename)
{
    struct tm* timeinfo;
    struct stat attrib;
    stat(filename,&attrib);
    timeinfo = gmtime(&(attrib.st_mtime));
    time_t ti = mktime(timeinfo);
    return ti;
}

//Returns 1 if the inputted string is a valid filename, 0 otherwise.
int is_a_file(char* filename)
{
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        return 0;
    }
    else
    {
        close(fd);
        return 1;
    }
}

//Returns a string that is the concatenation of strings a and b.
//NOTE: This string is stored on the heap and will need to be freed.
char* concatenate_strings(char* a, char* b)
{
    int len = strlen(a) + strlen(b);
    char* c = (char*)malloc(len * sizeof(char));
    sprintf(c, "%s%s", a, b);
    return c;
}
