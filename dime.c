/*******************************
 * dime.c
 *
 * Source code for DIstributed MakE
 *
 ******************************/

#include "util.h"
#include "dime.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*********
 * Simple usage instructions
 *********/
void dime_usage(char* progname) {
	fprintf(stderr, "Usage: %s [options] [target]\n", progname);
	fprintf(stderr, "-f FILE\t\tRead FILE as a dimefile.\n");
	fprintf(stderr, "-h\t\tPrint this message and exit.\n");
	fprintf(stderr, "-n\t\tDon't actually execute commands, just print them.\n");
	exit(0);
}

/****************************** 
 * this is the function that, when given a proper filename, will
 * parse the dimefile and read in the rules
 ***************/
void parse_file(char* filename) {
	char* line = malloc(160*sizeof(char));
	FILE* fp = file_open(filename);
	while((line = file_getline(line, fp)) != NULL) {
	
	// this loop will go through the given file, one line at a time
	// this is where you need to do the work of interpreting
	// each line of the file to be able to deal with it later

	}
	fclose(fp);
	free(line);
}

void fexecv(const char* path, char* const argv[])
{
	printf("Executing.\n");
	pid_t child_pid;
	child_pid = fork();
	if (child_pid > 0)
	{
		wait(NULL);
		return;
	}
	else if (child_pid == 0)
	{
		execv(path, argv);
	}
}

int main(int argc, char* argv[]) {
	// Declarations for getopt
	extern int optind;
	extern char* optarg;
	int ch;
	char* format = "f:hn";
	
	// Variables you'll want to use
	char* filename = "Dimefile";
	bool execute = true;

	// Part 2.2.1: Use getopt code to take input appropriately.
	while((ch = getopt(argc, argv, format)) != -1) {
		switch(ch) {
			case 'f':
				filename = optarg;
				break;
			case 'n':
				execute = false;
				break;
			case 'h':
				dime_usage(argv[0]);
				break;
		}
	}
	//Get rid of ./dime as the first argument
	argc -= optind;
	argv += optind;

	/* at this point, what is left in argv is the targets that were 
		specified on the command line. argc has the number of them.
		If getopt is still really confusing,
		try printing out what's in argv right here, then just running 
		dime with various command-line arguments. */
		
		char* args[] = {"ls", "-l", NULL};
		char* token = strcat(strtok(getenv("PATH"),":"),"/");
		fexecv(token, args);
		printf("%s\n",token);
		while (token != NULL)
		{
			char* func = "ls";
			//fexecv(strcat(token,func),args);
			printf("%s\n",token);
			token = strcat(strtok(NULL,":"),"/");
		}

	/* after parsing the file, you'll want to execute all of the targets
		that were specified on the command line, along with their dependencies, etc. */
	
	return 0;
}
