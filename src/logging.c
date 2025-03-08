#include "../include/logging.h"
#include "../include/config.h"
#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <dirent.h>

#define MAX_LOG_LINE 1024

static FILE *log_file = NULL;
static FILE *change_log_file = NULL;

// In init_logging() function
void init_logging(void) {
    // Open system log
    openlog("company_daemon", LOG_PID | LOG_PERROR, LOG_DAEMON);  // Add LOG_PERROR to see syslog messages on stderr
    
    // Try to open the log file, if it fails, create it in the current directory
    log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL) {
        // Try to create the log files in the current directory instead
        syslog(LOG_WARNING, "Failed to open log file %s (%s), using local logs", LOG_FILE, strerror(errno));
        log_file = fopen("./company_daemon.log", "a");
        if (log_file == NULL) {
            syslog(LOG_ERR, "Failed to create local log file: %s", strerror(errno));
            // Don't exit here, just continue without file logging
        }
    }
    
    change_log_file = fopen(CHANGE_LOG_FILE, "a");
    if (change_log_file == NULL) {
        syslog(LOG_WARNING, "Failed to open change log file %s (%s), using local logs", CHANGE_LOG_FILE, strerror(errno));
        change_log_file = fopen("./company_changes.log", "a");
        if (change_log_file == NULL) {
            syslog(LOG_ERR, "Failed to create local change log file: %s", strerror(errno));
            // Don't exit here, just continue without file logging
        }
    }
    
    // Log daemon start
    syslog(LOG_INFO, "Daemon started");
    if (log_file) {
        fprintf(log_file, "Daemon started\n");
        fflush(log_file);
    }
}

void close_logging(void) {
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
    
    if (change_log_file != NULL) {
        fclose(change_log_file);
        change_log_file = NULL;
    }
    
    // Log daemon stop
    syslog(LOG_INFO, "Daemon stopped");
    closelog();
}

void log_message(LogLevel level, const char* format, ...) {
    const char* level_str[] = {
        "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"
    };
    
    va_list args;
    char message[MAX_LOG_LINE];
    char time_str[32];
    time_t now = time(NULL);
    
    // Format time
    struct tm *t = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);
    
    // Format message
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    // Write to log file
    if (log_file != NULL) {
        fprintf(log_file, "[%s] [%s] %s\n", time_str, level_str[level], message);
        fflush(log_file);
    }
    
    // Write to syslog
    int syslog_priority;
    switch (level) {
        case DAEMON_LOG_DEBUG:    syslog_priority = LOG_DEBUG;   break;
        case DAEMON_LOG_INFO:     syslog_priority = LOG_INFO;    break;
        case DAEMON_LOG_WARNING:  syslog_priority = LOG_WARNING; break;
        case DAEMON_LOG_ERROR:    syslog_priority = LOG_ERR;     break;
        case DAEMON_LOG_CRITICAL: syslog_priority = LOG_CRIT;    break;
        default:                  syslog_priority = LOG_NOTICE;  break;
    }
    
    syslog(syslog_priority, "%s", message);
}

void log_file_change(const char* username, const char* filepath, const char* operation) {
    char time_str[32];
    time_t now = time(NULL);
    
    // Format time
    struct tm *t = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);
    
    // Write to change log file
    if (change_log_file != NULL) {
        fprintf(change_log_file, "[%s] User: %s, File: %s, Operation: %s\n", 
                time_str, username, filepath, operation);
        fflush(change_log_file);
    }
    
    // Also log to main log
    log_message(LOG_INFO, "File change - User: %s, File: %s, Operation: %s", 
                username, filepath, operation);
}

bool check_department_uploads(void) {
    char date_str[11];
    bool all_uploaded = true;
    
    get_current_date(date_str, sizeof(date_str));
    
    for (int i = 0; i < NUM_DEPARTMENTS; i++) {
        char expected_file[256];
        sprintf(expected_file, XML_PATTERN, DEPARTMENTS[i], date_str);
        
        char full_path[512];
        sprintf(full_path, "%s/%s/%s", UPLOAD_DIR, DEPARTMENTS[i], expected_file);
        
        if (access(full_path, F_OK) == -1) {
            log_message(LOG_WARNING, "Missing upload from department: %s", DEPARTMENTS[i]);
            all_uploaded = false;
        }
    }
    
    return all_uploaded;
}