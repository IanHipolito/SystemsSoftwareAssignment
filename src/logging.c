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
#include <pwd.h>
#include <sys/types.h>
#include <fcntl.h>

// Define PATH_MAX if not already defined
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_LOG_LINE 1024
#define LOG_FILENAME "company_daemon.log"
#define CHANGE_LOG_FILENAME "company_changes.log"

static FILE *log_file = NULL;
static FILE *change_log_file = NULL;

// Utility function to safely create log file path
static void create_log_path(char *dest, size_t dest_size, const char *dir, const char *filename) {
    // Ensure we have enough space for dir, '/', filename, and null terminator
    size_t dir_len = dir ? strlen(dir) : 0;
    size_t filename_len = strlen(filename);
    
    if (dir_len + filename_len + 2 > dest_size) {
        // If path would be too long, just use filename
        strncpy(dest, filename, dest_size);
        dest[dest_size - 1] = '\0';
        syslog(LOG_WARNING, "Log path too long, using fallback: %s", dest);
        return;
    }
    
    // Construct path safely
    if (dir && dir[0] != '\0') {
        snprintf(dest, dest_size, "%s/%s", dir, filename);
    } else {
        strncpy(dest, filename, dest_size);
        dest[dest_size - 1] = '\0';
    }
}

void init_logging(void) {
    // Open system log
    openlog("company_daemon", LOG_PID | LOG_PERROR, LOG_DAEMON);

    // Get current user details for debugging
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    
    // Extensive logging about log file creation attempt
    syslog(LOG_INFO, "Initializing logging for user: %s (UID: %d)", 
           pw ? pw->pw_name : "unknown", uid);

    // Print current working directory
    char cwd[PATH_MAX];
    cwd[0] = '\0';
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        syslog(LOG_WARNING, "Failed to get current working directory");
        strcpy(cwd, ".");
    }

    // Attempt to open/create log files with extensive error checking
    int log_fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (log_fd == -1) {
        // Detailed error logging
        syslog(LOG_ERR, "Failed to open main log file %s. Error: %s (errno: %d)", 
               LOG_FILE, strerror(errno), errno);

        // Check directory permissions
        struct stat dir_stat;
        if (stat("/var/log", &dir_stat) == -1) {
            syslog(LOG_ERR, "Cannot stat /var/log directory: %s", strerror(errno));
        } else {
            syslog(LOG_INFO, "/var/log directory permissions: %o", dir_stat.st_mode & 0777);
        }

        // Try alternative log file location
        char local_log_path[PATH_MAX];
        create_log_path(local_log_path, sizeof(local_log_path), 
                        cwd[0] ? cwd : ".", LOG_FILENAME);

        log_fd = open(local_log_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
        
        if (log_fd == -1) {
            syslog(LOG_CRIT, "Failed to create local log file %s. Error: %s", 
                   local_log_path, strerror(errno));
        } else {
            syslog(LOG_WARNING, "Using local log file at %s", local_log_path);
        }
    }

    // If we have a valid file descriptor, convert to FILE*
    if (log_fd != -1) {
        log_file = fdopen(log_fd, "a");
        if (log_file == NULL) {
            syslog(LOG_ERR, "Failed to convert log file descriptor to FILE*");
            close(log_fd);
        } else {
            syslog(LOG_INFO, "Successfully opened log file");
            fprintf(log_file, "=== Daemon Logging Started ===\n");
            fflush(log_file);
        }
    }

    // Similar process for change log file
    int change_log_fd = open(CHANGE_LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (change_log_fd == -1) {
        syslog(LOG_ERR, "Failed to open change log file %s. Error: %s", 
               CHANGE_LOG_FILE, strerror(errno));
        
        // Try local change log
        char local_change_log_path[PATH_MAX];
        create_log_path(local_change_log_path, sizeof(local_change_log_path), 
                        cwd[0] ? cwd : ".", CHANGE_LOG_FILENAME);

        change_log_fd = open(local_change_log_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
        
        if (change_log_fd == -1) {
            syslog(LOG_CRIT, "Failed to create local change log file. Error: %s", strerror(errno));
        } else {
            syslog(LOG_WARNING, "Using local change log file");
        }
    }

    // Convert change log file descriptor to FILE*
    if (change_log_fd != -1) {
        change_log_file = fdopen(change_log_fd, "a");
        if (change_log_file == NULL) {
            syslog(LOG_ERR, "Failed to convert change log file descriptor to FILE*");
            close(change_log_fd);
        } else {
            syslog(LOG_INFO, "Successfully opened change log file");
        }
    }

    // Final initialization logging
    syslog(LOG_INFO, "Logging initialization complete");
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