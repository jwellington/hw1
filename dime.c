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

//Prints usage instructions and exits
void dime_usage(char* progname) {
    fprintf(stderr, "Usage: %s [options] [target]\n", progname);
    fprintf(stderr, "-f FILE\t\tRead FILE as a dimefile.\n");
    fprintf(stderr, "-h\t\tPrint this message and exit.\n");
    fprintf(stderr, "-n\t\tDon't actually execute commands, just print them.\n");
    exit(0);
}

//Prints the error message and exits
void error(char* message) {
    fprintf(stderr, "Error: %s\n", message);
    exit(0);
}

//Returns the target target with the given name, or NULL if not found
TARGET* find_target(char * target_name) {
    TARGET * cur_target = first;
    while (cur_target != NULL && strcmp((cur_target->name), target_name) != 0)
    {
        cur_target = cur_target->next;
    }
    return cur_target;
}

/****************************** 
 * this is the function that, when given a proper filename, will
 * parse the dimefile and read in the rules
 ***************/
void parse_file(char* filename) {
    char* line2 = malloc(160 * sizeof (char));
    FILE* fp = file_open(filename);
    int level = 0; //0 = outside a target, 1 = reading inside braces of a target
    first = NULL;
    COMMAND* currentCommand = NULL;
    while ((line2 = file_getline(line2, fp)) != NULL) {
        //Get rid of newline

        char * line = line2; //allows us to do line++ without running into a seg fault
        line = strtok(line, "\n");
        //Skip past whitespace
        if (line != NULL) {
            while (line[0] == '\t' || line[0] == ' ') {
                line++;
            }
            //Ignore empty lines and comments
            if (*line != '\n' && *line != '\0' && *line != '#') {
                if (level == 0) //Should be reading in a line of a target and
                    //dependencies
                {
                    //Get target name

                    char * word = strtok(line, " \t");
                    if (line[strlen(line) - 1] != ':') {
                        error("Dime target names must be followed by a colon.");
                    }
                    char * targetName = malloc(sizeof (char) *(strlen(line)));
                    strncpy(targetName, line, strlen(line) - 1);
                    //Initialize next TARGET variable
                    TARGET* tar = (TARGET*) malloc(sizeof (TARGET));
                    tar->name = targetName;
                    tar->next = first;
                    first = tar;
                    tar->dependencies = NULL;
                    word = strtok(NULL, " \t");
                    //Create linked list of dependencies
                    while (word != NULL && *word != '{') {
                        DEPENDENCY* dep = (DEPENDENCY*) malloc(sizeof (DEPENDENCY));
                        char * dep_name = malloc(sizeof (char) *160);
                        strcpy(dep_name, word);
                        dep->name = dep_name;
                        dep->next = tar->dependencies;
                        tar->dependencies = dep;
                        word = strtok(NULL, " \t");
                    }
                    if (*word != '{') {
                        error("The line containing the target name must end with a {");
                    }
                    level = 1;
                } else //We should be reading in commands of a close paren
                {

                    TARGET * tar = first;
                    if (strlen(line) == 1 && *line == '}') //End the list of commands
                    {
                        level = 0;
                        currentCommand = NULL;
                    } else //Read in a command and add it to the linked list in correct order
                    {
                        COMMAND* com = (COMMAND*) malloc(sizeof (COMMAND));
                        char * command_str = malloc(sizeof (char) *(strlen(line) + 1));
												//Break up line by commas
                        line = strtok(line, ",");
                        strcpy(command_str, line);

                        com->str = command_str;

												//Create list of concurrent commands
                        line = strtok(NULL, ",");
                        COMMAND* concurrent_place = com;
                        while (line != NULL) {
                            while (line[0] == ' ')
                            {
                                line++;
                            }

                            COMMAND* concurrent_com = (COMMAND*) malloc(sizeof (COMMAND));
                            char * concurrent_str = malloc(sizeof (char) *(strlen(line) + 1));
                            strcpy(concurrent_str, line);
                            concurrent_com->str = concurrent_str;
                            concurrent_com->concurrent = NULL;

                            concurrent_place->concurrent = concurrent_com;
                            concurrent_place = concurrent_com;
                            line = strtok(NULL, ",");
                        }

                        if (currentCommand != NULL) {
                            currentCommand->next = com;
                        } else {
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

//Safely executes a program by creating a branch first
void fexecvp(const char* path, char* const argv[]) {
    pid_t child_pid;
    child_pid = fork();
    if (child_pid > 0) {
        wait(NULL);
        return;
    } else if (child_pid == 0) {
        execvp(path, argv);
    }
}

//Runs the commands of a Dimefile target, giving priority to commands on the
//same line
void run_target(TARGET * cur_target, bool execute) {
    COMMAND * com = cur_target->commands;
    while (com != NULL) {
        COMMAND * cur_command = com;
        while (cur_command != NULL)
        {
            run_command(cur_command, execute);
            cur_command = cur_command->concurrent;
        }
        com = com->next;
    }
    
}

//Breaks up the arguments of a command and executes or displays them, 
//depending on user input
void run_command(COMMAND * com, bool execute) {
    char * com_part = strtok(com->str, " ");
    //80 is the maximum number of tokens a 160-character line can have
    int numTokens = 80;
    char * com_list[numTokens];
    char * first_com = com_part;
    int i;

		//Populate array of tokens
    for (i = 0; i < numTokens; i++) {
        if (com_part != NULL) {
            com_list[i] = com_part;
            com_part = strtok(NULL, " ");
        }
        else
            com_list[i] = NULL;
    }

		//Execute or display the command
    if (execute)
    {
        fexecvp(first_com, com_list);
    }
    else {
        for (i = 0; i < numTokens; i++) {
            if (com_list[i] != NULL)
                printf("%s ", com_list[i]);
        }
        printf("\n");
    }

}

//Deletes a chain of concurrent commands
void clean_concurrent(COMMAND* com)
{
	COMMAND* temp = com->next;
	free(com);
	if (temp == NULL)
	{
		return;
	}
	clean_concurrent(temp);
}

//Tail recursively deletes a COMMAND variable and its concurrent commands
void clean_command(COMMAND* com)
{
	if (com->concurrent != NULL)
	{
		clean_concurrent(com->concurrent);
	}
	COMMAND* temp = com->next;
	free(com);
	if (temp == NULL)
	{
		return;
	}
	clean_command(temp);
}

//Tail recursively deletes a DEPENDENMCY variable and the ones it links to
void clean_dependency(DEPENDENCY* dep)
{
	DEPENDENCY* temp = dep->next;
	free(dep);
	if (temp == NULL)
	{
		return;
	}
	clean_dependency(temp);
}

//Tail recursively deletes a TARGET variable and all its subvariables
void clean_target(TARGET* tar)
{
	if (tar->dependencies != NULL)
	{
		clean_dependency(tar->dependencies);
	}
	if (tar->commands != NULL)
	{
		clean_command(tar->commands);
	}
	TARGET* temp = tar->next;
	free(tar);
	if (temp == NULL)
	{
		return;
	}
	clean_target(temp);
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
    while ((ch = getopt(argc, argv, format)) != -1) {
        switch (ch) {
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


    //Find target
    TARGET * cur_target = first;
    if (argc == 0) {
        while (cur_target->next != NULL)
            cur_target = cur_target->next;
    } else if (argc == 1) {
        cur_target = find_target(argv[0]);
        if (cur_target == NULL) {
            error("Target specified is not in Dimefile\n");
        }
    } else {
        error("Argument structure not correct");
    }

    //Take care of dependencies and execute
    if (!execute) {
        printf("Commands are: ");
        run_target(cur_target, execute);
    } else {
        DEPENDENCY * cur_depend = cur_target->dependencies;
        TARGET * depend_target;
        while (cur_depend != NULL) {
            depend_target = find_target(cur_depend->name);
            if (depend_target != NULL)
                run_target(depend_target, true);
            cur_depend = cur_depend->next;
        }
        run_target(cur_target, execute);

    }
    
    //Recursively delete all allocated variables
    clean_target(first);

    return 0;
}
