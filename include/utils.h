/**
 * @file utils.h
 * @brief Header file for logging and utility functions
 */

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
#define ERROR_LOG      "/var/report_system/logs/error.log"
#define OPERATION_LOG  "/var/report_system/logs/operations.log"
#define CHANGE_LOG     "/var/report_system/logs/changes.log"

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

#endif /* UTILS_H */