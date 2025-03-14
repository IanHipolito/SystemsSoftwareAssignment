#ifndef BACKUP_H
#define BACKUP_H

#include <sys/stat.h>
#include <dirent.h>

/* Predefined constants from the original header */
#define BACKUP_DIR     "/var/company/backup"
#define UPLOAD_DIR     "/var/company/upload"
#define DASHBOARD_DIR  "/var/company/reporting"
#define LOG_DIR       "/var/log"
#define LOCK_FILE      "/var/run/company_daemon.lock"

/* Permission settings */
#define UPLOAD_PERMISSIONS    0777
#define DASHBOARD_PERMISSIONS 0755
#define LOCKED_PERMISSIONS    0000

#define MAX_BACKUP_AGE (7 * 24 * 60 * 60)  /* 7 days in seconds */
#define MAX_BACKUPS 10 /* Maximum number of backups to keep */

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

/**
 * Delete old backups to prevent excessive disk usage
 * @return SUCCESS on success, FAILURE on error
 */
int cleanup_old_backups(void);

/**
 * Create a lock file to prevent race conditions in directory locking
 * @return SUCCESS on success, FAILURE on error
 */
int create_lock_file(void);

/**
 * Remove the lock file
 * @return SUCCESS on success, FAILURE on error
 */
int remove_lock_file(void);

/**
 * Check if the directories are locked
 * @return TRUE if locked, FALSE if not
 */
int are_directories_locked(void);

#endif /* BACKUP_H */