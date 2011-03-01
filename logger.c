#include "logger.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

//Initializes variables logging and logfile.
void initialize(int l, const char* f)
{
    if (!initialized)
    {
        logging = l;
        logfile = f;
        initialized = 1;
    }
}

//Writes message to logfile. Returns 1 if successful, 0 if unsuccessful.
int write_to_log(char* message)
{
    if (logging)
    {
        FILE* f;
        f = fopen(logfile, "a");
        if (f != NULL)
        {
            fwrite(message, sizeof(char), strlen(message), f);
            fclose(f);
            return 1;
        }
        else
        {
            return 0;
        }
    }
    return 1;
}

//Resets the log file; returns 1 if successful, 0 otherwise.
int reset()
{
    FILE* f;
    f = fopen(logfile, "w");
    if (f != NULL)
    {
        fwrite("", sizeof(char), 0, f);
        fclose(f);
        return 1;
    }
    else
    {
        return 0;
    }
}
