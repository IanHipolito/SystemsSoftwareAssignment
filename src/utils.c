#include "../include/utils.h"
#include "../include/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>

void get_current_date(char* date_str, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(date_str, size, "%Y-%m-%d", t);
}

time_t get_current_time(void) {
    return time(NULL);
}

bool is_transfer_time(void) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    return (t->tm_hour == TRANSFER_HOUR && 
            t->tm_min == TRANSFER_MINUTE && 
            t->tm_sec == TRANSFER_SECOND);
}

void format_time(time_t time_val, char* time_str, size_t size) {
    struct tm *t = localtime(&time_val);
    strftime(time_str, size, "%Y-%m-%d %H:%M:%S", t);
}

void get_file_owner(const char* path, char* owner, size_t size) {
    struct stat file_stat;
    struct passwd *pwd;
    
    if (stat(path, &file_stat) == 0) {
        pwd = getpwuid(file_stat.st_uid);
        if (pwd != NULL) {
            strncpy(owner, pwd->pw_name, size);
            owner[size - 1] = '\0';
        } else {
            snprintf(owner, size, "%d", file_stat.st_uid);
        }
    } else {
        strncpy(owner, "unknown", size);
    }
}

int time_until_next_execution(int hour, int minute, int second) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    struct tm next = *t;
    
    next.tm_hour = hour;
    next.tm_min = minute;
    next.tm_sec = second;
    
    // If the specified time has already passed for today, schedule for tomorrow
    if (now >= mktime(&next)) {
        next.tm_mday += 1;
    }
    
    return difftime(mktime(&next), now);
}

bool is_xml_file(const char* filename) {
    const char *ext = strrchr(filename, '.');
    if (ext != NULL) {
        return (strcmp(ext, ".xml") == 0);
    }
    return false;
}

bool is_department_file(const char* filename, const char* department) {
    return (strncmp(filename, department, strlen(department)) == 0);
}