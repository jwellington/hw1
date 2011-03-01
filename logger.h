#ifndef _LOGGER_H_
#define _LOGGER_H_

static int logging = 0;
static const char* logfile;
static int initialized = 0;

void initialize(int l, const char* f);
int reset();
int write_to_log(char* message);

#endif
