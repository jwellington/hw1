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
//80 is the maximum number of tokens a 160-character line can have
int maxTokens = 80;
int counter = 0;
bool execute_all = false;
bool logging = false;
char* default_log = "dime.log";
const char* logfile;
char* command_log = "EXECUTING COMMAND  |  ";
char* target_log =  "EXECUTING TARGET   |  ";

void error(char* message);
void reset_log();
void write_log(char* message);
void lprintf(char* message);
char* current_time();
void dime_usage(char*);
void parse_file(char*);
void comma_in_quote_encode(char * line);
void comma_in_quote_decode(char * line);
void fexecvp(const char* path, char* const argv[]);
void run_target(TARGET * cur_target, bool execute);
void run_command(COMMAND * com, bool execute);
void run_tokens(char* com_list[], int numTokens);
void clean_target(TARGET* tar);
void clean_dependency(DEPENDENCY* dep);
void clean_command(COMMAND* com);
void clean_concurrent(COMMAND* com);
TARGET* find_target(char * target_name);
#endif
