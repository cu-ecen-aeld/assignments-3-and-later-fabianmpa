#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>

bool do_system(const char *cmd);

bool do_exec(int count, ...);

bool do_exec_redirect(const char *outputfile, int count, ...);
