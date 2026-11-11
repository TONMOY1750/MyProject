#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

void log_info(const char *message);
void log_debug(const char *message);
void log_error(const char *message);
void trim_newline(char *str);

#endif // UTILS_H
