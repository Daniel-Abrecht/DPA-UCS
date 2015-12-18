#include <logger.h>

#ifdef DPAUCS_LOG_TO_STDOUT
#include <stdio.h>
int WEAK (*DPAUCS_log_func)(const char*,...) = &printf;
#else
int WEAK (*DPAUCS_log_func)(const char*,...) = 0;
#endif
