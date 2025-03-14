#include "utils.h"
#include "backup.h"
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

/**
 * Log an error message
 * @param format Format string for the message
 * @param ... Variable arguments
 */
void log_error(const char *format, ...)
{
    FILE *log_fp;
    va_list args;
    time_t now;
    char time_str[MAX_TIME_LENGTH];

    /* Get current time */
    now = time(NULL);
    get_timestamp_string(now, time_str, MAX_TIME_LENGTH);

    /* Check if log needs rotation */
    check_and_rotate_log(ERROR_LOG);     /* For log_error */
    check_and_rotate_log(OPERATION_LOG); /* For log_operation */

    /* Open the error log file for appending */
    log_fp = fopen(ERROR_LOG, "a");
    if (log_fp == NULL)
    {
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
    if (format[strlen(format) - 1] != '\n')
    {
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
void log_operation(const char *format, ...)
{
    FILE *log_fp;
    va_list args;
    time_t now;
    char time_str[MAX_TIME_LENGTH];

    /* Get current time */
    now = time(NULL);
    get_timestamp_string(now, time_str, MAX_TIME_LENGTH);

    /* Open the operation log file for appending */
    log_fp = fopen(OPERATION_LOG, "a");
    if (log_fp == NULL)
    {
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
    if (format[strlen(format) - 1] != '\n')
    {
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
char *get_timestamp_string(time_t timestamp, char *buffer, size_t buffer_size)
{
    struct tm *tm_info;

    tm_info = localtime(&timestamp);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", tm_info);

    return buffer;
}

/**
 * Check if a log file needs rotation and rotate it if necessary
 * @param log_path Path to the log file
 * @return SUCCESS on success, FAILURE on error
 */
int check_and_rotate_log(const char *log_path)
{
    struct stat st;
    char backup_path[MAX_PATH_LENGTH];
    char old_backup[MAX_PATH_LENGTH];
    char new_backup[MAX_PATH_LENGTH];
    int i;

    /* Check if file exists */
    if (stat(log_path, &st) != 0)
    {
        /* File doesn't exist, nothing to rotate */
        return SUCCESS;
    }

    /* Check if file is over the size limit */
    if (st.st_size < MAX_LOG_SIZE)
    {
        /* File is not big enough for rotation */
        return SUCCESS;
    }

    /* Delete the oldest backup if it exists */
    snprintf(old_backup, sizeof(old_backup), "%s.%d", log_path, MAX_LOG_BACKUPS);
    unlink(old_backup); /* Ignore errors if it doesn't exist */

    /* Shift all backups */
    for (i = MAX_LOG_BACKUPS - 1; i > 0; i--)
    {
        snprintf(old_backup, sizeof(old_backup), "%s.%d", log_path, i);
        snprintf(new_backup, sizeof(new_backup), "%s.%d", log_path, i + 1);
        rename(old_backup, new_backup); /* Ignore errors if old doesn't exist */
    }

    /* Create backup of current log */
    snprintf(backup_path, sizeof(backup_path), "%s.1", log_path);
    if (rename(log_path, backup_path) != 0)
    {
        log_error("Failed to rotate log file %s: %s", log_path, strerror(errno));
        return FAILURE;
    }

    /* Touch empty log file to create it again */
    FILE *fp = fopen(log_path, "w");
    if (fp != NULL)
    {
        fclose(fp);
        chmod(log_path, 0666); /* Set permissions to allow all writes */
    }
    else
    {
        log_error("Failed to create new log file after rotation: %s", strerror(errno));
        return FAILURE;
    }

    return SUCCESS;
}