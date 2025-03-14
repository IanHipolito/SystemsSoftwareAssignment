#define _DEFAULT_SOURCE  // For directory entry macros
#define _POSIX_C_SOURCE 200809L

#include "backup.h"
#include "utils.h"
#include "file_operations.h"
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stddef.h>

/**
 * Backup the dashboard directory
 * @return SUCCESS on success, FAILURE on error
 */
int backup_dashboard(void) {
    char backup_path[MAX_PATH_LENGTH];
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
    if (tm_info == NULL) {
        log_error("Failed to get local time for backup");
        return FAILURE;
    }
    
    /* Safely create backup path with timestamp */
    int path_len = snprintf(backup_path, sizeof(backup_path), "%s/backup_%04d-%02d-%02d_%02d-%02d-%02d", 
                 BACKUP_DIR, 
                 tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                 tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    
    /* Check if path was truncated */
    if (path_len < 0 || (size_t)path_len >= sizeof(backup_path)) {
        log_error("Backup path too long");
        return FAILURE;
    }
    
    /* Create backup directory */
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

        /* Skip special directory entries */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Construct the full path with careful bounds checking */
        int full_path_len = snprintf(full_path, sizeof(full_path), "%s/%s", DASHBOARD_DIR, entry->d_name);
        if (full_path_len < 0 || (size_t)full_path_len >= sizeof(full_path)) {
            log_error("Path too long for file: %s", entry->d_name);
            continue;
        }
        
        /* Skip directories */
        if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            continue;
        }
        
        /* Construct source path with careful bounds checking */
        int src_path_len = snprintf(src_path, sizeof(src_path), "%s/%s", DASHBOARD_DIR, entry->d_name);
        if (src_path_len < 0 || (size_t)src_path_len >= sizeof(src_path)) {
            log_error("Source path too long for file: %s", entry->d_name);
            continue;
        }
        
        /* Construct destination path with careful bounds checking */
        int dest_path_len = snprintf(dest_path, sizeof(dest_path), "%s/%s", backup_path, entry->d_name);
        if (dest_path_len < 0 || (size_t)dest_path_len >= sizeof(dest_path)) {
            log_error("Destination path too long for file: %s", entry->d_name);
            continue;
        }
        
        /* Copy the file */
        file_count++;
        if (copy_file(src_path, dest_path) == SUCCESS) {
            success_count++;
        } else {
            log_error("Failed to backup file: %s", entry->d_name);
        }
    }
    
    closedir(dir);

    /* Clean up old backups */
    cleanup_old_backups();
    
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
    
    /* Create lock file to prevent race conditions */
    if (create_lock_file() != SUCCESS) {
        return FAILURE;
    }
    
    /* Change permissions to prevent modifications */
    if (set_directory_permissions(UPLOAD_DIR, LOCKED_PERMISSIONS) != SUCCESS) {
        log_error("Failed to lock upload directory");
        remove_lock_file();
        result = FAILURE;
    }
    
    if (set_directory_permissions(DASHBOARD_DIR, LOCKED_PERMISSIONS) != SUCCESS) {
        log_error("Failed to lock dashboard directory");
        /* Try to restore upload directory permissions */
        set_directory_permissions(UPLOAD_DIR, UPLOAD_PERMISSIONS);
        remove_lock_file();
        result = FAILURE;
    }
    
    if (result != SUCCESS) {
        remove_lock_file();
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
    
    /* Remove lock file */
    if (remove_lock_file() != SUCCESS) {
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

/**
 * Delete old backups to prevent excessive disk usage
 * @return SUCCESS on success, FAILURE on error
 */
int cleanup_old_backups(void) {
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    char path[MAX_PATH_LENGTH];
    time_t now;
    time_t cutoff_time;
    int backup_count = 0;
    int deleted_count = 0;
    
    log_operation("Cleaning up old backups");
    
    /* Get current time */
    now = time(NULL);
    cutoff_time = now - MAX_BACKUP_AGE;
    
    /* Open backup directory */
    dir = opendir(BACKUP_DIR);
    if (dir == NULL) {
        log_error("Failed to open backup directory: %s", strerror(errno));
        return FAILURE;
    }
    
    /* Count backup directories */
    while ((entry = readdir(dir)) != NULL) {
        /* Skip special directory entries and non-directories */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        /* Construct full path */
        snprintf(path, MAX_PATH_LENGTH, "%s/%s", BACKUP_DIR, entry->d_name);
        
        /* Get file information */
        if (stat(path, &st) != 0) {
            log_error("Failed to get stats for backup %s: %s", path, strerror(errno));
            continue;
        }
        
        /* Only count directories starting with 'backup_' */
        if (S_ISDIR(st.st_mode) && strncmp(entry->d_name, "backup_", 7) == 0) {
            backup_count++;
            
            /* Delete old backups */
            if (st.st_mtime < cutoff_time) {
                log_operation("Deleting old backup: %s", entry->d_name);
                if (rmdir(path) != 0) {
                    log_error("Failed to delete old backup %s: %s", path, strerror(errno));
                } else {
                    deleted_count++;
                }
            }
        }
    }
    
    closedir(dir);
    
    log_operation("Backup cleanup completed: %d backups found, %d deleted", 
                 backup_count, deleted_count);
    
    return SUCCESS;
}

/**
 * Create a lock file to prevent race conditions in directory locking
 * @return SUCCESS on success, FAILURE on error
 */
int create_lock_file(void) {
    int fd;
    
    /* Try to create the lock file exclusively */
    fd = open(LOCK_FILE, O_WRONLY | O_CREAT | O_EXCL, 0644);
    
    if (fd == -1) {
        if (errno == EEXIST) {
            log_error("Lock file already exists, another process is locking directories");
        } else {
            log_error("Failed to create lock file: %s", strerror(errno));
        }
        return FAILURE;
    }
    
    /* Write process ID to the lock file */
    char pid_str[16];
    int pid_len = snprintf(pid_str, sizeof(pid_str), "%d\n", getpid());
    
    if (write(fd, pid_str, pid_len) != pid_len) {
        log_error("Failed to write to lock file: %s", strerror(errno));
        close(fd);
        unlink(LOCK_FILE);
        return FAILURE;
    }
    
    close(fd);
    return SUCCESS;
}

/**
 * Remove the lock file
 * @return SUCCESS on success, FAILURE on error
 */
int remove_lock_file(void) {
    if (unlink(LOCK_FILE) != 0 && errno != ENOENT) {
        log_error("Failed to remove lock file: %s", strerror(errno));
        return FAILURE;
    }
    
    return SUCCESS;
}

/**
 * Check if the directories are locked
 * @return TRUE if locked, FALSE if not
 */
int are_directories_locked(void) {
    return (access(LOCK_FILE, F_OK) == 0);
}