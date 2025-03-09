/**
 * @file utils.c
 * @brief Implementation of logging and utility functions
 */

#include "utils.h"
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

/**
 * Log an error message
 * @param format Format string for the message
 * @param ... Variable arguments
 */
void log_error(const char* format, ...) {
    FILE *log_fp;
    va_list args;
    time_t now;
    char time_str[MAX_TIME_LENGTH];
    
    /* Get current time */
    now = time(NULL);
    get_timestamp_string(now, time_str, MAX_TIME_LENGTH);
    
    /* Open the error log file for appending */
    log_fp = fopen(ERROR_LOG, "a");
    if (log_fp == NULL) {
        /* Fall back to syslog if file can't be opened */
        va_start(args, format);
        vsyslog(LOG_ERR, format, args);
        va_end(args);
        return;
    }
    
    /* Write timestamp and error prefix */
    fprintf(log_fp, "[%s] ERROR: ", time_str);
    
    /* Write the formatted message */
    va_start(args, format);
    vfprintf(log_fp, format, args);
    va_end(args);
    
    /* Add newline if not present */
    if (format[strlen(format) - 1] != '\n') {
        fprintf(log_fp, "\n");
    }
    
    /* Close the log file */
    fclose(log_fp);
    
    /* Also log to syslog */
    va_start(args, format);
    vsyslog(LOG_ERR, format, args);
    va_end(args);
}

/**
 * Log an operation message
 * @param format Format string for the message
 * @param ... Variable arguments
 */
void log_operation(const char* format, ...) {
    FILE *log_fp;
    va_list args;
    time_t now;
    char time_str[MAX_TIME_LENGTH];
    
    /* Get current time */
    now = time(NULL);
    get_timestamp_string(now, time_str, MAX_TIME_LENGTH);
    
    /* Open the operation log file for appending */
    log_fp = fopen(OPERATION_LOG, "a");
    if (log_fp == NULL) {
        /* Fall back to syslog if file can't be opened */
        va_start(args, format);
        vsyslog(LOG_INFO, format, args);
        va_end(args);
        return;
    }
    
    /* Write timestamp and info prefix */
    fprintf(log_fp, "[%s] INFO: ", time_str);
    
    /* Write the formatted message */
    va_start(args, format);
    vfprintf(log_fp, format, args);
    va_end(args);
    
    /* Add newline if not present */
    if (format[strlen(format) - 1] != '\n') {
        fprintf(log_fp, "\n");
    }
    
    /* Close the log file */
    fclose(log_fp);
    
    /* Also log to syslog */
    va_start(args, format);
    vsyslog(LOG_INFO, format, args);
    va_end(args);
}

/**
 * Get a formatted timestamp string
 * @param timestamp Timestamp to format
 * @param buffer Buffer to store the formatted timestamp
 * @param buffer_size Size of the buffer
 * @return Pointer to the buffer
 */
char* get_timestamp_string(time_t timestamp, char* buffer, size_t buffer_size) {
    struct tm *tm_info;
    
    tm_info = localtime(&timestamp);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", tm_info);
    
    return buffer;
}