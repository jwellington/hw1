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
#include <sys/wait.h>

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

//Prints error message and exits
void error(char* message)
{
	fprintf(stderr, "Error: %s\n", message);
	exit(0);
}

/****************************** 
 * this is the function that, when given a proper filename, will
 * parse the dimefile and read in the rules
 ***************/
void parse_file(char* filename) {
	char* line2 = malloc(160*sizeof(char));
	FILE* fp = file_open(filename);
	int level = 0; //0 = outside a target, 1 = reading inside braces of a target
	first = NULL;
	while((line2 = file_getline(line2, fp)) != NULL) {
		//Get rid of newline
                //allows us to do line++ without running into a seg fault
                char * line = line2;
		line = strtok(line, "\n");
		COMMAND* currentCommand = NULL;
		//Skip past whitespace
                if(line != NULL)
                {
                    while(line[0] == '\t' || line[0] == ' ')
                    {
                            line++;
                    }
                    //Ignore empty lines and comments
                    if (*line != '\n' && *line != '\0' && *line != '#')
                    {
                            if (level == 0) //Should be reading in a line of a target and
                                                                                            //dependencies
                            {
                                    //Get target name

                                    char * word = strtok(line, " \t");
                                    if(line[strlen(line) - 1] != ':')
                                    {
                                            error("Dime target names must be followed by a colon.");
                                    }
                                    char * targetName = malloc(sizeof(char)*(strlen(line)));
                                    strncpy(targetName, line, strlen(line)-1);
                                    //Initialize next TARGET variable
                                    TARGET* tar = (TARGET*)malloc(sizeof(TARGET));
                                    tar->name = targetName;
                                    tar->next = first;
                                    first = tar;
                                    tar->dependencies = NULL;
                                    word = strtok(NULL, " \t");
                                    //Create linked list of dependencies
                                    while (word != NULL && *word != '{')
                                    {
                                            DEPENDENCY* dep = (DEPENDENCY*)malloc(sizeof(DEPENDENCY));
                                            char * dep_name = malloc(sizeof(char)*160);
                                            strcpy(dep_name, word);
                                            dep->name = dep_name;
                                            dep->next = tar->dependencies;
                                            tar->dependencies = dep;
                                            word = strtok(NULL, " \t");
                                    }
                                    if (*word != '{')
                                    {
                                            error("The line containing the target name must end with a {");
                                    }
                                    level = 1;
                            }
                            else //We should be reading in commands of a close paren
                            {

                                TARGET * tar = first;
                                    if (strlen(line) == 1 && *line == '}') //End the list of commands
                                    {
                                            level = 0;
                                    }
                                    else //Read in a command and add it to the linked list in correct order
                                    {
                                            COMMAND* com = (COMMAND*)malloc(sizeof(COMMAND));
                                            char * command_str = malloc(sizeof(char)*(strlen(line) + 1));
                                            strcpy(command_str, line);
                                            com->str = command_str;
                                            if (currentCommand != NULL)
                                            {
                                                    currentCommand->next = com;
                                            }
                                            else
                                            {
                                                    tar->commands = com;
                                            }
                                            currentCommand = com;
                                    }
                            }
                    }
                }
	}
	fclose(fp);
	free(line2);
}

void fexecvp(const char* path, char* const argv[])
{
	pid_t child_pid;
	child_pid = fork();
	if (child_pid > 0)
	{
		wait(NULL);
		return;
	}
	else if (child_pid == 0)
	{
		execvp(path, argv);
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
	
	parse_file(filename);

        TARGET * last = first;
        while(last->next != NULL)
            last = last->next;

        if(last->dependencies == NULL)
        {
            COMMAND * com = last->commands;
            while(com != NULL)
            {
                char * com_part  = strtok(com->str, " ");
                char * com_list[5];
                char * first_com = com_part;
                int i;
                for(i = 0; i < 5; i++)
                {
                    if(com_part != NULL)
                    {
                        com_list[i] = com_part;
                        com_part = strtok(NULL, " ");
                    }
                    else
                        com_list[i] = NULL;
                }
                fexecvp(first_com, com_list);
                com = com->next;
            }
        }

        char* args[] = {"ls", "-l", NULL};
	//fexecvp("ls", args);
	printf("First->data: %s \n", first->name);
	

	/* at this point, what is left in argv is the targets that were 
		specified on the command line. argc has the number of them.
		If getopt is still really confusing,
		try printing out what's in argv right here, then just running 
		dime with various command-line arguments. */
		
		//Example usage for executing a parsed command
		//char* args[] = {"ls", "-l", NULL};
		//fexecvp("ls", args);
		
	/* after parsing the file, you'll want to execute all of the targets
		that were specified on the command line, along with their dependencies, etc. */
	
	return 0;
}
