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

TARGET* first;
void error(char* message);
void dime_usage(char*);
void parse_file(char*);
void fexecvp(const char* path, char* const argv[]);
void run_target(TARGET * cur_target, bool execute);
void run_command(COMMAND * com, bool execute);
void clean_target(TARGET* tar);
void clean_dependency(DEPENDENCY* dep);
void clean_command(COMMAND* com);
void clean_concurrent(COMMAND* com);
TARGET* find_target(char * target_name);
#endif
