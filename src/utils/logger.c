#define DPA_LOGGER_C

#include <stdio.h>
#include <DPA/utils/logger.h>

#ifndef __FLASH
int (*const DPA_log_func)(const char*,...) = &printf;
#else
int (*const DPA_log_func)(const flash char*,...) = (int(*const)(const flash char*,...))&printf_P;
#endif
