#include <DPA/UCS/logger.h>

#ifdef DPAUCS_LOG_TO_STDOUT
#include <stdio.h>
int weak (*DPAUCS_log_func)(const char*,...) = &printf;
#else
int weak (*DPAUCS_log_func)(const char*,...) = 0;
#endif
