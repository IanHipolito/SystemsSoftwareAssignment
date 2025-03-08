#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <stdbool.h>

// Log levels - renamed to avoid collision with syslog.h constants
typedef enum {
    DAEMON_LOG_DEBUG,
    DAEMON_LOG_INFO,
    DAEMON_LOG_WARNING,
    DAEMON_LOG_ERROR,
    DAEMON_LOG_CRITICAL
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