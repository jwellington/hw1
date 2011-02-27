/*******************************
 * dime.c
 *
 * Source code for DIstributed MakE
 *
 ******************************/

#include "util.h"
#include "dime.h"
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

//Prints usage instructions and exits
void dime_usage(char* progname) {
    fprintf(stderr, "Usage: %s [options] [target]\n", progname);
    fprintf(stderr, "-f FILE\t\tRead FILE as a dimefile.\n");
    fprintf(stderr, "-h\t\tPrint this message and exit.\n");
    fprintf(stderr, "-n\t\tDon't actually execute commands, just print them.\n");
    fprintf(stderr, "-a\t\tExecute all commands, regardless of modification time.\n");
    fprintf(stderr, "-l[LOG]\t\tSend output to LOG (dime.log by default).\n");
    fprintf(stderr, "-L[LOG]\t\tLike L, but clear LOG if it already exists.\n");
    exit(0);
}

//Prints the error message and exits
void error(char* message) {
    fprintf(stderr, "Error: %s\n", message);
    write_log(message);
    write_log("\n");
    exit(0);
}

//Prints out message with a newline and appends it to the log
void lprintf(char* message)
{
    write_log(message);
    printf("%s", message);
}

//Resets the log to an empty file.
void reset_log()
{
    FILE* f;
    f = fopen(logfile, "w");
    if (f != NULL)
    {
        fwrite("", sizeof(char), 0, f);
        fclose(f);
    }
    else
    {
        printf("Unable to open logfile for resetting.\n");
        exit(0);
    }
}

//Writes the message to the log, if logging is enabled.
void write_log(char* message)
{
    if (logging)
    {
        FILE* f;
        f = fopen(logfile, "a");
        if (f != NULL)
        {
            fwrite(message, sizeof(char), strlen(message), f);
            fclose(f);
        }
        else
        {
            printf("Unable to open logfile for writing.\n");
            exit(0);
        }
    } 
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
                    char * targetName = malloc(sizeof (char) *(strlen(line)) + 1);
                    strncpy(targetName, line, strlen(line) - 1);
                    targetName[strlen(line) - 1] = '\0';
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
                        char * command_str = malloc(sizeof(char) * (strlen(line) + 1));

                        //Encode the commas in quotes
                        comma_in_quote_encode(line);
                       
						//Break up line by commas
                        line = strtok(line, ",");
                        strcpy(command_str, line);
                        comma_in_quote_decode(command_str);

                        com->str = command_str;
                        com->concurrent = NULL;
                        com->next = NULL;

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

                            //Decode the commas in quotes
                            comma_in_quote_decode(concurrent_str);

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

//Replace commas in quotes with other characters to not
//mess up the strtok on the comma
void comma_in_quote_encode(char * line)
{
    bool inQuotes = false;
    int j = 0;
    while (line[j] != '\0') {
        if (line[j] == '\'') {
            inQuotes = !inQuotes;
        } else if (line[j] == ',' && inQuotes) {
            line[j] = '\n';
        }
        j++;
    }
    if (inQuotes) {
        error("Open quote in command");
    }
    inQuotes = false;

    j = 0;
    while (line[j] != '\0') {
        if (line[j] == '\"') {
            inQuotes = !inQuotes;
        } else if (line[j] == ',' && inQuotes) {
            line[j] = '\n';
        }
        j++;
    }
    if(inQuotes){
        error("Open quote in command");
    }
}


void comma_in_quote_decode(char * line)
{
    int j = 0;
    while (line[j] != '\0') {
        if (line[j] == '\n') {
            line[j] = ',';
        }
        j++;
    }
}
//Safely executes a command by creating a branch first
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
    if (check_dependencies(cur_target) || execute_all)
    {
        if (logging)
        {
            int len = strlen(target_log) + strlen(cur_target->name) + 2;
            char message[len];
            sprintf(message, "%s%s\n", target_log, cur_target->name);
            write_log(message);
        }
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
}

//Breaks up the arguments of a command and executes or displays them, 
//depending on user input
void run_command(COMMAND * com, bool execute) {
    if (logging)
    {
        int len = strlen(command_log) + strlen(com->str) + 2;
        char message[len];
        sprintf(message, "%s%s\n", command_log, com->str);
        write_log(message);
    }
	char * com_part = strtok(com->str, " ");
	int numTokens = maxTokens + 1;
	char * com_list[maxTokens + 1];
	int i = 0;
	com_list[maxTokens] = NULL;
	//Populate array of tokens
	for (; i < maxTokens; i++) {
		com_list[i] = com_part;
		com_part = strtok(NULL, " ");
		if (com_part == NULL && numTokens > maxTokens)
		{
		    numTokens = i + 1;
		}
	}
	if (numTokens > maxTokens)
	{
	    numTokens = maxTokens;
	}
	//Execute or display the command
	if (execute)
	{
	    run_tokens(com_list, numTokens);
	}
	else {
	  for (i = 0; i < numTokens; i++) {
		   if (com_list[i] != NULL)
		       printf("%s ", com_list[i]);
	  }
	  printf("\n");
	}
}

void run_tokens(char* com_list[], int numTokens)
{
    //Check for pipes
    int has_pipe = -1;
    int i;
    for(i = 0; i < numTokens; i++)
    {
       if(com_list[i] != NULL && strcmp(com_list[i],"|") == 0)
       {
           has_pipe = i;
           break;
       }
    }
    //If there is a pipe, set up and populate second list of commands
    if(has_pipe != -1)
    {
       char * com_list_2[numTokens - has_pipe];
       com_list[has_pipe] = NULL;
       for(i = has_pipe + 1; i < numTokens; i++)
       {
           com_list_2[i - (has_pipe + 1)] = com_list[i];
           com_list[i] = NULL;
       }
       com_list_2[numTokens - has_pipe - 1] = NULL;
       //All commands in com_list at the pipe token and afterward are NULL

       int pipe_files[2];
       pipe(pipe_files);

       int child_pid = fork();
       if(child_pid > 0)
       {
           close(pipe_files[1]);
           FILE * stream = fdopen(pipe_files[0], "r");
           int stat = dup2(pipe_files[0], STDIN_FILENO);
           if (stat < 0)
           {
                error("Failed to redirect standard input for piping.");
           }
           run_tokens(com_list_2, numTokens - (has_pipe + 1));
           fclose(stream);
           wait(NULL);
           return;
       }
       else if(child_pid == 0)
       {
           close(pipe_files[0]);
           int stat = dup2(pipe_files[1], STDOUT_FILENO);
           if (stat < 0)
           {
                error("Failed to redirect standard output for piping.");
           }
           fexecvp(com_list[0],com_list);
           fclose(stdout);
           exit(0);
       }
    }
    else
    {
       fexecvp(com_list[0], com_list);
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
	free(com->str);
	free(com);
	if (temp == NULL)
	{
		return;
	}
	clean_command(temp);
}

//Tail recursively deletes a DEPENDENCY variable and the ones it links to
void clean_dependency(DEPENDENCY* dep)
{
	DEPENDENCY* temp = dep->next;
	free(dep->name);
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
	free(tar->name);
	free(tar);
	if (temp == NULL)
	{
		return;
	}
	clean_target(temp);
}

//Checks if a target is a file. If it is, check if it has any dependencies
//that have been modified more recently. If it does, or if it is not a file,
//return true, otherwise, return false.
int check_dependencies(TARGET* tar)
{
    if (is_a_file(tar->name))
    {
        if (tar->dependencies == NULL)
        {
            return 0;
        }
        else
        {
            time_t current_time = last_modified(tar->name);
            DEPENDENCY* dep = tar->dependencies;
            while (dep != NULL)
            {
                if (is_a_file(dep->name))
                {
                    time_t mod_time = last_modified(dep->name);
                    if (difftime(mod_time,current_time) > 0)
                    {
                        return 1;
                    }
                }
                dep = dep->next;
            }
            return 0;
        }
    }
    else
    {
        return 1;
    }
}

int main(int argc, char* argv[]) {
    // Declarations for getopt
    extern int optind;
    extern char* optarg;
    int ch;
    char* format = "f:hnal::L::";

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
            case 'a':
            	execute_all = true;
            	break;
           	case 'l':
           	    logging = true;
           	    if (optarg != NULL)
           	    {
           	        logfile = optarg;
           	    }
           	    else
           	    {
           	        logfile = default_log;
           	    }
           	    break;
           	case 'L':
           	    logging = true;
           	    if (optarg != NULL)
           	    {
           	        logfile = optarg;
           	    }
           	    else
           	    {
           	        logfile = default_log;
           	    }
           	    reset_log();
           	    break;
           	case '?':
           	    if (optopt == 'f')
           	    {
           	        error("Specify a file to read instruction from after -f.");
           	    }
                else
                {
                    int len = strlen("Unknown option character ");
                    char message[len + 3];
                    sprintf(message, "Unknown option character %c.", optopt);
                    error(message);
                }
                break;
           	default:
           	    dime_usage(argv[0]);
           	    break;
        }
    }
    //Get rid of ./dime as the first argument
    argc -= optind;
    argv += optind;
    if (logging)
    {
        write_log(current_time());
    }

    parse_file(filename);

    //Find target
    TARGET * cur_target = first;
    if (argc == 1) {
        cur_target = find_target(argv[0]);
        if (cur_target == NULL) {
            error("Target specified is not in Dimefile\n");
        }
    } else if (argc > 1) {
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
    write_log("\n");

    return 0;
}
