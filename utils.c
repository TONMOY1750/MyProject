#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static void get_timestamp(char *buffer, size_t size) {
    time_t rawtime;
    struct tm *info;
    time(&rawtime);
    info = localtime(&rawtime);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", info);
}

void log_info(const char *message) {
    char time_str[32];
    get_timestamp(time_str, sizeof(time_str));
    printf("[INFO] [%s] %s\n", time_str, message);
}

void log_debug(const char *message) {
    char time_str[32];
    get_timestamp(time_str, sizeof(time_str));
    printf("[DEBUG] [%s] %s\n", time_str, message);
}

void log_error(const char *message) {
    char time_str[32];
    get_timestamp(time_str, sizeof(time_str));
    printf("[ERROR] [%s] %s\n", time_str, message);
}

void trim_newline(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}
