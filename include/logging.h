#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <stdbool.h>

// Log levels
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_CRIT
} LogLevel;

// Initialize logging
void init_logging(void);

// Close logging
void close_logging(void);

// Log a message
void log_message(LogLevel level, const char* format, ...);

// Log file change
void log_file_change(const char* username, const char* filepath, const char* operation);

// Check if a file was uploaded for each department
bool check_department_uploads(void);

#endif /* LOGGING_H */