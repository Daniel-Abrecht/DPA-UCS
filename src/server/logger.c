#define DPA_LOGGER_C

#include <stdio.h>
#include <DPA/UCS/logger.h>

int (*const DPA_log_func)(const char*,...) = &printf;
