// src/file_transfer.c
#include "../include/file_transfer.h"
#include "../include/config.h"
#include "../include/logging.h"
#include "../include/lock_manager.h"
#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

static bool copy_file(const char* src_path, const char* dest_path) {
    int src_fd, dest_fd;
    char buffer[4096];
    ssize_t bytes_read, bytes_written;
    int total_bytes = 0;
    
    // Open source file for reading
    src_fd = open(src_path, O_RDONLY);
    if (src_fd == -1) {
        log_message(LOG_ERROR, "Failed to open source file %s: %s", 
                    src_path, strerror(errno));
        return false;
    }
    
    // Open destination file for writing
    dest_fd = open(dest_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd == -1) {
        log_message(LOG_ERROR, "Failed to open destination file %s: %s", 
                    dest_path, strerror(errno));
        close(src_fd);
        return false;
    }
    
    // Copy file contents
    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            log_message(LOG_ERROR, "Failed to write to destination file %s: %s", 
                        dest_path, strerror(errno));
            close(src_fd);
            close(dest_fd);
            return false;
        }
        total_bytes += bytes_written;
    }
    
    // Check for read error
    if (bytes_read == -1) {
        log_message(LOG_ERROR, "Failed to read from source file %s: %s", 
                    src_path, strerror(errno));
        close(src_fd);
        close(dest_fd);
        return false;
    }
    
    // Close file descriptors
    close(src_fd);
    close(dest_fd);
    
    log_message(LOG_INFO, "Copied file %s to %s (%d bytes)", 
                src_path, dest_path, total_bytes);
    return true;
}

static bool process_department_directory(const char* department) {
    bool success = true;
    char dept_path[512];
    char report_path[512];
    DIR *dir;
    struct dirent *entry;
    
    // Create department path
    snprintf(dept_path, sizeof(dept_path), "%s/%s", UPLOAD_DIR, department);
    
    // Open directory
    dir = opendir(dept_path);
    if (dir == NULL) {
        log_message(LOG_ERROR, "Failed to open department directory %s: %s", 
                    dept_path, strerror(errno));
        return false;
    }
    
    // Process each file in directory
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Only process XML files
        if (!is_xml_file(entry->d_name)) {
            continue;
        }
        
        // Create full paths
        char src_path[1024];
        char dest_path[1024];
        snprintf(src_path, sizeof(src_path), "%s/%s", dept_path, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", REPORT_DIR, entry->d_name);
        
        // Copy file
        if (!copy_file(src_path, dest_path)) {
            success = false;
            log_message(LOG_ERROR, "Failed to transfer file %s", src_path);
        } else {
            char username[256];
            get_file_owner(src_path, username, sizeof(username));
            log_file_change(username, dest_path, "transferred");
        }
    }
    
    closedir(dir);
    return success;
}

TaskStatus transfer_files(void) {
    log_message(LOG_INFO, "Starting file transfer operation");
    
    // Lock directories
    if (lock_directories() != LOCK_SUCCESS) {
        log_message(LOG_ERROR, "Failed to lock directories for transfer");
        return TASK_FAILURE;
    }
    
    // Report task in progress
    report_task_status(TASK_TRANSFER, TASK_IN_PROGRESS);
    
    bool success = true;
    
    // Process each department directory
    for (int i = 0; i < NUM_DEPARTMENTS; i++) {
        if (!process_department_directory(DEPARTMENTS[i])) {
            success = false;
        }
    }
    
    // Unlock directories
    if (unlock_directories() != UNLOCK_SUCCESS) {
        log_message(LOG_ERROR, "Failed to unlock directories after transfer");
        success = false;
    }
    
    // Report task status
    TaskStatus status = success ? TASK_SUCCESS : TASK_FAILURE;
    report_task_status(TASK_TRANSFER, status);
    
    log_message(LOG_INFO, "File transfer operation %s", 
                success ? "completed successfully" : "failed");
    
    return status;
}

bool check_file_exists(const char* department, const char* filename) {
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/%s/%s", UPLOAD_DIR, department, filename);
    
    // Check if file exists
    if (access(filepath, F_OK) == -1) {
        return false;
    }
    
    return true;
}

void schedule_next_transfer(void) {
    int sleep_time = time_until_next_execution(TRANSFER_HOUR, TRANSFER_MINUTE, TRANSFER_SECOND);
    log_message(LOG_INFO, "Scheduled next transfer in %d seconds", sleep_time);
}