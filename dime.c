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

extern void initialize(int, const char*);
extern int write_to_log(char*);
extern int reset();

//Constant variable declarations
const int max_tokens = 512;
const char* default_log = "dime.log";
const char* command_log = "EXECUTING COMMAND  |  ";
const char* target_log =  "EXECUTING TARGET   |  ";
const char* info_log =    "INFO               |  ";
const mode_t def_mode = S_IRUSR | S_IWUSR;

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
    if (!reset())
    {
        printf("Unable to open logfile for resetting.\n");
        exit(0);
    }
}

//Writes the message to the log, if logging is enabled.
void write_log(char* message)
{
    if (!write_to_log(message))
    {
        printf("Unable to open logfile for writing.\n");
        exit(0);
    }
}

//Returns the target target with the given name, or NULL if not found
TARGET* find_target(char * target_name, VARHOLDER* vh) {
    TARGET * cur_target = vh->first;
    while (cur_target != NULL && strcmp((cur_target->name), target_name) != 0)
    {
        cur_target = cur_target->next;
    }
    return cur_target;
}

//Given a filename, parses the file and builds the data structures
//Returns a pointer to the first (last in the file) target
TARGET* parse_file(char* filename) {
    char* line2 = malloc(160 * sizeof (char));
    FILE* fp = file_open(filename);
    //level = 0 outside a target, 1 reading inside braces of a target
    int level = 0;
    TARGET* first = NULL;
    COMMAND* current_command = NULL;
    while ((line2 = file_getline(line2, fp)) != NULL) {
        //Get rid of newline
        //allows us to do line++ without running into a seg fault
        char * line = line2;
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
                    first = read_target(line, first);
                    level = 1;
                } else //We should be reading in commands of a close paren
                {

                    TARGET * tar = first;
                    if (strlen(line) == 1 && *line == '}') //End the list of commands
                    {
                        level = 0;
                        current_command = NULL;
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

                        if (current_command != NULL) {
                            current_command->next = com;
                        } else {
                            tar->commands = com;
                        }
                        current_command = com;
                    }
                }
            }
        }
    }
    fclose(fp);
    free(line2);
    return first;
}
//Read in a target from the dimefile
TARGET * read_target(char * line, TARGET * first) {
    //Get target name

    char * word = strtok(line, " \t");
    if (line[strlen(line) - 1] != ':') {
        error("Dime target names must be followed by a colon.");
    }
    char * target_name = malloc(sizeof (char) * strlen(line) + 1);
    strncpy(target_name, line, strlen(line) - 1);
    target_name[strlen(line) - 1] = '\0';
    //Initialize next TARGET variable
    TARGET* tar = (TARGET*) malloc(sizeof (TARGET));
    tar->name = target_name;
    tar->next = first;
    first = tar;
    tar->dependencies = NULL;
    DEPENDENCY* current_dep = tar->dependencies;
    word = strtok(NULL, " \t");
    //Create linked list of dependencies
    while (word != NULL && *word != '{') {
        char * dep_name = malloc(sizeof (char) * strlen(word) + 1);
        strcpy(dep_name, word);
        DEPENDENCY* dep = (DEPENDENCY*) malloc(sizeof (DEPENDENCY));
        dep->name = dep_name;
        dep->next = NULL;
        if (current_dep != NULL) {
            current_dep->next = dep;
            current_dep = dep;
        } else {
            current_dep = dep;
            tar->dependencies = dep;
        }
        word = strtok(NULL, " \t");
    }
    if (*word != '{') {
        error("The line containing the target name must end with a {");
    }
    return first;
}

//Replace commas in quotes with newlines to not mess up the strtok on the comma
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

//Puts commas back into the line
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
void run_target(TARGET * cur_target, char* previous_dependencies[],
                int depc, VARHOLDER* vh) {
    bool execute_all = vh->execute_all;
    //printf("EXECUTNG TARGET %s\n", cur_target->name);
    if (check_dependencies(cur_target) || execute_all)
    {
        //Log message
        int len = strlen(target_log) + strlen(cur_target->name) + 2;
        char message[len];
        sprintf(message, "%s%s\n", target_log, cur_target->name);
        write_log(message);
        //Drop duplicate dependencies
        check_duplicate_dependencies(cur_target, vh);
        //Take care of dependencies and execute
        DEPENDENCY * cur_depend = cur_target->dependencies;
        TARGET * depend_target;
        while (cur_depend != NULL) {
            depend_target = find_target(cur_depend->name, vh);
            if (depend_target != NULL)
            {
                if (!check_circular_dependencies(cur_depend,
                                                 previous_dependencies,
                                                 depc,
                                                 cur_target, 
                                                 vh))
                {
                    char* next_dependencies[depc+1];
                    int j;
                    for (j = 0; j < depc; j++)
                    {
                        next_dependencies[j] = previous_dependencies[j];
                    }
                    next_dependencies[depc] = cur_target->name;
                    run_target(depend_target, next_dependencies, depc+1, vh);
                }
            }
            cur_depend = cur_depend->next;
        }
        //Load list of commands and run them
        COMMAND * com = cur_target->commands;
        while (com != NULL) {
            COMMAND * cur_command = com;
            while (cur_command != NULL)
            {
                //printf("%s: %s\n", cur_target->name, cur_command->str);
                run_command(cur_command, vh);
                cur_command = cur_command->concurrent;
            }
            com = com->next;
        }
    }
}

//Breaks up the arguments of a command and executes or displays them, 
//depending on user input
void run_command(COMMAND * com, VARHOLDER* vh) {
    bool execute = vh->execute;
    //Prepare log message
    int len = strlen(command_log) + strlen(com->str) + 2;
    char message[len];
    sprintf(message, "%s%s\n", command_log, com->str);
    write_log(message);
    //Copy com->str so strtok doesn't mess with it
    char com_str[strlen(com->str)+1];
    strcpy(com_str,com->str);
	char * com_part = strtok(com_str, " ");
	int num_tokens = max_tokens + 1;
	char * com_list[max_tokens + 1];
	int i = 0;
	com_list[max_tokens] = NULL;
	//Populate array of tokens
	for (; i < max_tokens; i++) {
		com_list[i] = com_part;
		com_part = strtok(NULL, " ");
		if (com_part == NULL && num_tokens > max_tokens)
		{
		    num_tokens = i + 1;
		}
	}
	if (num_tokens > max_tokens)
	{
	    num_tokens = max_tokens;
	}
	//Execute or display the command
	if (execute)
	{
	    run_tokens(com_list, num_tokens);
	}
	else {
	  for (i = 0; i < num_tokens; i++) {
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
    int has_redirect = -1;
    int i;
    for(i = 0; i < numTokens; i++)
    {
       if(com_list[i] != NULL && strcmp(com_list[i],"|") == 0)
       {
           has_pipe = i;
           break;
       }
       if(com_list[i] != NULL && (strcmp(com_list[i],">") == 0 || strcmp(com_list[i],"<") == 0))
       {
           has_redirect = i;
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
    else if(has_redirect != -1)
    {
    	//Seperate commands into two lists
        char * com_list_2[numTokens - has_pipe];
        char * redirect_token = com_list[has_redirect];
        com_list[has_redirect] = NULL;
        for(i = has_redirect + 1; i < numTokens; i++)
       	{
           	com_list_2[i - (has_redirect + 1)] = com_list[i];
           	com_list[i] = NULL;
        }
        com_list_2[numTokens - has_redirect - 1] = NULL;
		
		//If output redirection
		if(redirect_token[0] == '>')
		{
       	
		   	char * filename = com_list_2[0];
		   	int output_file = 
		   	    open(filename, O_WRONLY | O_CREAT | O_TRUNC, def_mode);
		   	if(output_file == -1)
		   	{
		   		error("Redirection of output to file failed");
		   	}
		   	int stdout_old = dup(STDOUT_FILENO);
		   	dup2(output_file, STDOUT_FILENO);
		   	fexecvp(com_list[0], com_list);
		   	close(output_file);
		   	dup2(stdout_old, STDOUT_FILENO);
       	}
       	//If input redirection
       	else
       	{
       		char * filename = com_list_2[0];
		   	int input_file = open(filename, O_RDONLY, def_mode);
		   	if(input_file == -1)
		   	{
		   		error("Redirection of input from file failed");
		   	}
		   	int stdin_old = dup(STDIN_FILENO);
		   	dup2(input_file, STDIN_FILENO);
		   	fexecvp(com_list[0], com_list);
		   	close(input_file);
		   	dup2(stdin_old, STDIN_FILENO);
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
    if (tar != NULL)
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
	    clean_target(temp);
	}
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

void check_duplicate_dependencies(TARGET* cur_target, VARHOLDER* vh)
{
    DEPENDENCY* other_dep = cur_target->dependencies;
    if (other_dep != NULL &&other_dep->next != NULL)
    {
        DEPENDENCY* prev_dep = cur_target->dependencies;
        DEPENDENCY* check_dep = other_dep->next;
        while (check_dep != NULL)
        {
            while (other_dep != check_dep)
            {
                if (strcmp(other_dep->name,check_dep->name) == 0)
                {
                    char message[strlen(info_log)
                                + strlen("Duplicate dependency  ->  dropped.\n")
                                + strlen(cur_target->name)
                                + strlen(check_dep->name) + 1];
                    sprintf(message,"%sDuplicate dependency %s -> %s dropped.\n",
                                info_log, cur_target->name, check_dep->name);
                    write_log(message);
                    if (vh->execute)
                    {
                        printf("Duplicate dependency %s -> %s dropped.\n",
                                cur_target->name, check_dep->name);
                    }
                    //Is the first target a duplicate?
                    if (other_dep == cur_target->dependencies)
                    {
                        cur_target->dependencies = other_dep->next;
                        prev_dep = other_dep->next;
                        other_dep->next = NULL;
                        clean_dependency(other_dep);
                        other_dep = prev_dep;
                    }
                    //Or is it a later target?
                    else
                    {
                        prev_dep->next = other_dep->next;
                        other_dep->next = NULL;
                        clean_dependency(other_dep);
                        other_dep = prev_dep->next;
                    }
                }
                else
                {
                    prev_dep = other_dep;
                    other_dep = other_dep->next;
                }
            }
            other_dep = cur_target->dependencies;
            prev_dep = cur_target->dependencies;
            check_dep = check_dep->next;
        }
    }
}

//Checks if dep is in the list of previous dependencies (i.e. if calling_target
//has a circular dependencies). If yes, returns true and prints/logs an error
//message. Otherwise, returns false.
int check_circular_dependencies(DEPENDENCY* dep, 
                                char* dependencies[], 
                                int depc,
                                TARGET* calling_target,
                                VARHOLDER* vh)
{
    TARGET* tar = find_target(dep->name,vh);
    int i;
    if (tar != NULL)
    {
        for (i = 0; i < depc; i++)
        {
            if (strcmp(dependencies[i],dep->name) == 0)
            {
                char message[strlen(info_log)
                             + strlen("Circular dependency  ->  dropped.\n")
                             + strlen(dependencies[i])
                             + strlen(dep->name) + 1];
                sprintf(message, "%sCircular dependency %s -> %s dropped.\n",
                        info_log,calling_target->name,dep->name);
                write_log(message);
                if (vh->execute)
                {
                    printf("Circular dependency %s -> %s dropped.\n",
                            calling_target->name,dep->name);
                }
                return 1;
            }
        }
        return 0;
    }
    else
    {
        return 0;
    }
}

int main(int argc, char* argv[]) {
    // Declarations for getopt
    extern int optind;
    extern char* optarg;
    int ch;
    char* format = "f:hnal::L::";
    
    int logging = 0;
    const char* logfile;
    bool reset_log = false;

    // Variables you'll want to use
    char* filename = "Dimefile";
    bool execute = true;
    bool execute_all = false;

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
           	    logging = 1;
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
           	    logging = 1;
           	    if (optarg != NULL)
           	    {
           	        logfile = optarg;
           	    }
           	    else
           	    {
           	        logfile = default_log;
           	    }
           	    reset_log = true;
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
    
    //Initialize logger
    initialize(logging, logfile);
    
    if (reset_log)
    {
        reset();
    }
    if (logging)
    {
        write_log(current_time());
    }
    
    TARGET* first = parse_file(filename);
    VARHOLDER var_holder;
    var_holder.first = first;
    var_holder.execute = execute;
    var_holder.execute_all = execute_all;

    //Find all targets given on the command line
    TARGET * cur_target;
    if (argc == 0)
    {
        if (first != NULL)
        {
            TARGET* last = first;
            while (last->next != NULL)
            {
                last = last->next;
            }
            if (!execute) {
                printf("Commands are: \n");
            }
            char* deps[] = {};
            run_target(last, deps, 0, &var_holder);
        }
    }
    else
    {
        int i;
        int num_targets = 0;
        for (i = 0; i < argc; i++)
        {
            cur_target = find_target(argv[i], &var_holder);
            if (cur_target == NULL)
            {
                char message[strlen("Target  is not in Dimefile\n") +
                    strlen(argv[i]) + 1];
                sprintf(message, "Target %s is not in Dimefile\n", argv[i]);
                lprintf(message);
            }
            else
            {
                num_targets++;
            }
        }
        TARGET* targets[num_targets];
        int targets_index = 0;
        for (i = 0; i < argc; i++)
        {
            cur_target = find_target(argv[i], &var_holder);
            if (cur_target != NULL)
            {
                targets[targets_index] = cur_target;
                targets_index++;
            }
        }

        //Run targets
        if (!execute) {
            printf("Commands are: \n");
        }
        for (i = 0; i < num_targets; i++)
        {
            char* deps[] = {};
            run_target(targets[i], deps, 0, &var_holder);
        }
    }
    
    //Recursively delete all allocated variables
    clean_target(first);
    write_log("\n");

    return 0;
}
