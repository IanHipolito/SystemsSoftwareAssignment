#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <errno.h>

/* Predefined constants from the original header */
#define MAX_TIME_LENGTH 64
#define MAX_PATH_LENGTH 1024
#define ERROR_LOG      "/var/log/company_daemon.log"
#define OPERATION_LOG  "/var/log/company_daemon.log"
#define CHANGE_LOG     "/var/log/company_changes.log"

#define MAX_LOG_SIZE (10 * 1024 * 1024)  /* 10 MB max log size */
#define MAX_LOG_BACKUPS 5                /* Keep 5 rotated logs */

/**
 * Log an error message
 * @param format Format string for the message
 * @param ... Variable arguments
 */
void log_error(const char* format, ...);

/**
 * Log an operation message
 * @param format Format string for the message
 * @param ... Variable arguments
 */
void log_operation(const char* format, ...);

/**
 * Get a formatted timestamp string
 * @param timestamp Timestamp to format
 * @param buffer Buffer to store the formatted timestamp
 * @param buffer_size Size of the buffer
 * @return Pointer to the buffer
 */
char* get_timestamp_string(time_t timestamp, char* buffer, size_t buffer_size);

/**
 * Check if a log file needs rotation and rotate it if necessary
 * @param log_path Path to the log file
 * @return SUCCESS on success, FAILURE on error
 */
int check_and_rotate_log(const char* log_path);

#endif /* UTILS_H */