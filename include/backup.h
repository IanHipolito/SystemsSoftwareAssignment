/**
 * @file backup.h
 * @brief Header file for backup and directory management functions
 */

#ifndef BACKUP_H
#define BACKUP_H

#include <sys/stat.h>
#include <dirent.h>

/* Predefined constants from the original header */
#define BACKUP_DIR     "/var/report_system/backup"
#define UPLOAD_DIR     "/var/report_system/upload"
#define DASHBOARD_DIR  "/var/report_system/dashboard"
#define LOG_DIR       "/var/report_system/logs"

/* Permission settings */
#define UPLOAD_PERMISSIONS    0777
#define DASHBOARD_PERMISSIONS 0755
#define LOCKED_PERMISSIONS    0000

/* Return codes */
#define SUCCESS 0
#define FAILURE -1

/* Boolean definitions */
#define TRUE  1
#define FALSE 0

/**
 * Backup the dashboard directory
 * @return SUCCESS on success, FAILURE on error
 */
int backup_dashboard(void);

/**
 * Lock directories during backup/transfer operations
 * @return SUCCESS on success, FAILURE on error
 */
int lock_directories(void);

/**
 * Unlock directories after backup/transfer operations
 * @return SUCCESS on success, FAILURE on error
 */
int unlock_directories(void);

/**
 * Set directory permissions
 * @param path Directory path
 * @param mode Permission mode
 * @return SUCCESS on success, FAILURE on error
 */
int set_directory_permissions(const char* path, mode_t mode);

/**
 * Create directory if it doesn't exist
 * @param path Directory path
 * @return SUCCESS on success, FAILURE on error
 */
int create_directory_if_not_exists(const char* path);

/**
 * Check if a directory is empty
 * @param path Directory path
 * @return TRUE if empty, FALSE if not empty or error
 */
int is_directory_empty(const char* path);

#endif /* BACKUP_H */