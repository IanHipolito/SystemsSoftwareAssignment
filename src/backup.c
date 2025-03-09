/**
 * @file backup.c
 * @brief Implementation of backup and directory management functions
 */

#define _DEFAULT_SOURCE  // For DT_DIR on some systems
#define _POSIX_C_SOURCE 200809L

#include "backup.h"
#include "utils.h"
#include "file_operations.h"
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

/**
 * Backup the dashboard directory
 * @return SUCCESS on success, FAILURE on error
 */
int backup_dashboard(void) {
    char backup_path[MAX_PATH_LENGTH];
    char timestamp[MAX_TIME_LENGTH];
    time_t now;
    struct tm *tm_info;
    DIR *dir;
    struct dirent *entry;
    char src_path[MAX_PATH_LENGTH];
    char dest_path[MAX_PATH_LENGTH];
    int success_count = 0;
    int file_count = 0;
    
    log_operation("Starting dashboard backup");
    
    /* Get current time for backup folder name */
    now = time(NULL);
    tm_info = localtime(&now);
    strftime(timestamp, MAX_TIME_LENGTH, "%Y-%m-%d_%H-%M-%S", tm_info);
    
    /* Create backup directory with timestamp */
    snprintf(backup_path, MAX_PATH_LENGTH, "%s/backup_%s", BACKUP_DIR, timestamp);
    if (mkdir(backup_path, 0755) != 0) {
        log_error("Failed to create backup directory: %s", strerror(errno));
        return FAILURE;
    }
    
    /* Open dashboard directory */
    dir = opendir(DASHBOARD_DIR);
    if (dir == NULL) {
        log_error("Failed to open dashboard directory: %s", strerror(errno));
        return FAILURE;
    }
    
    /* Process each file in the directory */
    while ((entry = readdir(dir)) != NULL) {
        struct stat st;
        char full_path[MAX_PATH_LENGTH];

        /* Skip directory entries */
        snprintf(full_path, sizeof(full_path), "%s/%s", DASHBOARD_DIR, entry->d_name);
        
        /* Use stat to check if it's a directory */
        if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            continue;
        }
        
        /* Skip special directory entries */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        /* Construct source and destination paths */
        snprintf(src_path, MAX_PATH_LENGTH, "%s/%s", DASHBOARD_DIR, entry->d_name);
        snprintf(dest_path, MAX_PATH_LENGTH, "%s/%s", backup_path, entry->d_name);
        
        /* Copy the file */
        file_count++;
        if (copy_file(src_path, dest_path) == SUCCESS) {
            success_count++;
        } else {
            log_error("Failed to backup file: %s", entry->d_name);
        }
    }
    
    closedir(dir);
    
    /* Log result */
    if (success_count == file_count) {
        log_operation("Backup completed successfully: %d files", success_count);
        return SUCCESS;
    } else {
        log_error("Backup partially completed: %d/%d files", success_count, file_count);
        return (success_count > 0) ? SUCCESS : FAILURE;
    }
}
 
 /**
  * Lock directories during backup/transfer operations
  * @return SUCCESS on success, FAILURE on error
  */
 int lock_directories(void) {
     int result = SUCCESS;
     
     log_operation("Locking directories for backup/transfer");
     
     /* Change permissions to prevent modifications */
     if (set_directory_permissions(UPLOAD_DIR, LOCKED_PERMISSIONS) != SUCCESS) {
         log_error("Failed to lock upload directory");
         result = FAILURE;
     }
     
     if (set_directory_permissions(DASHBOARD_DIR, LOCKED_PERMISSIONS) != SUCCESS) {
         log_error("Failed to lock dashboard directory");
         result = FAILURE;
     }
     
     return result;
 }
 
 /**
  * Unlock directories after backup/transfer operations
  * @return SUCCESS on success, FAILURE on error
  */
 int unlock_directories(void) {
     int result = SUCCESS;
     
     log_operation("Unlocking directories after backup/transfer");
     
     /* Restore normal permissions */
     if (set_directory_permissions(UPLOAD_DIR, UPLOAD_PERMISSIONS) != SUCCESS) {
         log_error("Failed to unlock upload directory");
         result = FAILURE;
     }
     
     if (set_directory_permissions(DASHBOARD_DIR, DASHBOARD_PERMISSIONS) != SUCCESS) {
         log_error("Failed to unlock dashboard directory");
         result = FAILURE;
     }
     
     return result;
 }
 
 /**
  * Set directory permissions
  * @param path Directory path
  * @param mode Permission mode
  * @return SUCCESS on success, FAILURE on error
  */
 int set_directory_permissions(const char* path, mode_t mode) {
     if (chmod(path, mode) != 0) {
         log_error("Failed to set permissions on %s: %s", path, strerror(errno));
         return FAILURE;
     }
     
     return SUCCESS;
 }
 
 /**
  * Create directory if it doesn't exist
  * @param path Directory path
  * @return SUCCESS on success, FAILURE on error
  */
 int create_directory_if_not_exists(const char* path) {
     struct stat st;
     
     /* Check if directory already exists */
     if (stat(path, &st) == 0) {
         if (S_ISDIR(st.st_mode)) {
             return SUCCESS;
         } else {
             log_error("%s exists but is not a directory", path);
             return FAILURE;
         }
     }
     
     /* Create the directory */
     if (mkdir(path, 0755) != 0) {
         log_error("Failed to create directory %s: %s", path, strerror(errno));
         return FAILURE;
     }
     
     log_operation("Created directory: %s", path);
     return SUCCESS;
 }
 
 /**
  * Check if a directory is empty
  * @param path Directory path
  * @return TRUE if empty, FALSE if not empty or error
  */
 int is_directory_empty(const char* path) {
     DIR *dir;
     struct dirent *entry;
     int is_empty = TRUE;
     
     /* Open the directory */
     dir = opendir(path);
     if (dir == NULL) {
         log_error("Failed to open directory %s: %s", path, strerror(errno));
         return FALSE;
     }
     
     /* Check for entries other than . and .. */
     while ((entry = readdir(dir)) != NULL) {
         if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
             is_empty = FALSE;
             break;
         }
     }
     
     closedir(dir);
     return is_empty;
 }