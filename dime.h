
/**************************
 * dime.h -- the header file for dime.c, a 
 * distributed make
 *
 *
 *
 *************************/

#ifndef _DIME_H_
#define _DIME_H_

#define true 1
#define false 0
typedef int bool;
typedef struct command
{
	char* str;
        struct command* concurrent;
	struct command* next;
}COMMAND;
typedef struct dependency
{
	char* name;
	struct dependency* next;
}DEPENDENCY;
typedef struct target
{
	char* name;
	DEPENDENCY* dependencies;
	COMMAND* commands;
	struct target* next;
}TARGET;

typedef struct var_holder
{
    TARGET* first;
    bool execute;
    bool execute_all;
}VARHOLDER;

void error(char* message);
void reset_log();
void write_log(char* message);
void lprintf(char* message);
int check_dependencies(TARGET* tar);
void check_duplicate_dependencies(TARGET* cur_target, VARHOLDER* vh);
int check_circular_dependencies(DEPENDENCY* dep, 
                                char* dependencies[], 
                                int depc,
                                TARGET* calling_target,
                                VARHOLDER* vh);
char* current_time();
void dime_usage(char*);
TARGET* parse_file(char*);
void comma_in_quote_encode(char * line);
void comma_in_quote_decode(char * line);
void fexecvp(const char* path, char* const argv[]);
void run_target(TARGET * cur_target, char* previous_dependencies[],
                int depc, VARHOLDER* vh);
void run_command(COMMAND * com, VARHOLDER* vh);
void run_tokens(char* com_list[], int numTokens);
void clean_target(TARGET* tar);
void clean_dependency(DEPENDENCY* dep);
void clean_command(COMMAND* com);
void clean_concurrent(COMMAND* com);
TARGET* find_target(char * target_name, VARHOLDER* vh);
#endif
