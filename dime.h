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
/*typedef struct command
{
	
}COMMAND;
typedef struct target
{
	char* name;
	struct target* dependencies[];
	COMMAND* commands[]
}TARGET;*/

void dime_usage(char*);
void parse_file(char*);
void fexecv(const char* path, char* const argv[]);

#endif
