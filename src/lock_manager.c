#include "../include/lock_manager.h"
#include "../include/config.h"
#include "../include/logging.h"
#include <sys/types.h> 
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>

// Flag to track lock status
static bool directories_locked = false;

// Original permissions
static mode_t upload_orig_mode = 0;
static mode_t report_orig_mode = 0;

// Store original permissions before locking
static bool store_original_permissions(void) {
    struct stat st;
    
    if (stat(UPLOAD_DIR, &st) == 0) {
        upload_orig_mode = st.st_mode & 0777;
    } else {
        log_message(DAEMON_LOG_ERROR, "Failed to get permissions for upload directory");
        return false;
    }
    
    if (stat(REPORT_DIR, &st) == 0) {
        report_orig_mode = st.st_mode & 0777;
    } else {
        log_message(DAEMON_LOG_ERROR, "Failed to get permissions for reporting directory");
        return false;
    }
    
    return true;
}

LockStatus lock_directories(void) {
    if (directories_locked) {
        log_message(DAEMON_LOG_WARNING, "Directories already locked");
        return LOCK_SUCCESS;
    }
    
    // Store original permissions
    if (!store_original_permissions()) {
        return LOCK_FAILURE;
    }
    
    // Set read-only permissions (removing write access)
    if (chmod(UPLOAD_DIR, 0555) == -1) {
        log_message(DAEMON_LOG_ERROR, "Failed to lock upload directory");
        return LOCK_FAILURE;
    }
    
    if (chmod(REPORT_DIR, 0555) == -1) {
        // Revert upload directory permissions if report directory lock fails
        chmod(UPLOAD_DIR, upload_orig_mode);
        log_message(DAEMON_LOG_ERROR, "Failed to lock reporting directory");
        return LOCK_FAILURE;
    }
    
    directories_locked = true;
    log_message(DAEMON_LOG_INFO, "Directories locked for backup/transfer");
    return LOCK_SUCCESS;
}

LockStatus unlock_directories(void) {
    if (!directories_locked) {
        log_message(DAEMON_LOG_WARNING, "Directories not locked");
        return UNLOCK_SUCCESS;
    }
    
    // Restore original permissions
    if (chmod(UPLOAD_DIR, upload_orig_mode) == -1) {
        log_message(DAEMON_LOG_ERROR, "Failed to unlock upload directory");
        return UNLOCK_FAILURE;
    }
    
    if (chmod(REPORT_DIR, report_orig_mode) == -1) {
        log_message(DAEMON_LOG_ERROR, "Failed to unlock reporting directory");
        return UNLOCK_FAILURE;
    }
    
    directories_locked = false;
    log_message(DAEMON_LOG_INFO, "Directories unlocked after backup/transfer");
    return UNLOCK_SUCCESS;
}

bool are_directories_locked(void) {
    return directories_locked;
}