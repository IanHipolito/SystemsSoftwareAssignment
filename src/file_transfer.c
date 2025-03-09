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
        log_message(DAEMON_LOG_ERROR, "Failed to open source file %s: %s", 
                    src_path, strerror(errno));
        return false;
    }
    
    // Open destination file for writing
    dest_fd = open(dest_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd == -1) {
        log_message(DAEMON_LOG_ERROR, "Failed to open destination file %s: %s", 
                    dest_path, strerror(errno));
        close(src_fd);
        return false;
    }
    
    // Copy file contents
    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            log_message(DAEMON_LOG_ERROR, "Failed to write to destination file %s: %s", 
                        dest_path, strerror(errno));
            close(src_fd);
            close(dest_fd);
            return false;
        }
        total_bytes += bytes_written;
    }
    
    // Check for read error
    if (bytes_read == -1) {
        log_message(DAEMON_LOG_ERROR, "Failed to read from source file %s: %s", 
                    src_path, strerror(errno));
        close(src_fd);
        close(dest_fd);
        return false;
    }
    
    // Close file descriptors
    close(src_fd);
    close(dest_fd);
    
    log_message(DAEMON_LOG_INFO, "Copied file %s to %s (%d bytes)", 
                src_path, dest_path, total_bytes);
    return true;
}

static bool process_department_directory(const char* department) {
    bool success = true;
    char dept_path[512];
    DIR *dir;
    struct dirent *entry;
    int files_processed = 0;
    
    // Create department path
    snprintf(dept_path, sizeof(dept_path), "%s/%s", UPLOAD_DIR, department);
    
    log_message(DAEMON_LOG_DEBUG, "Processing department: %s", department);
    
    // Open directory
    dir = opendir(dept_path);
    if (dir == NULL) {
        log_message(DAEMON_LOG_ERROR, "Failed to open department directory %s: %s", 
                    dept_path, strerror(errno));
        return false;
    }
    
    log_message(DAEMON_LOG_DEBUG, "Opened directory %s for processing", dept_path);
    
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
            log_message(DAEMON_LOG_ERROR, "Failed to transfer file %s", src_path);
        } else {
            files_processed++;
            char username[256];
            get_file_owner(src_path, username, sizeof(username));
            log_file_change(username, dest_path, "transferred");
        }
    }
    
    closedir(dir);
    log_message(DAEMON_LOG_INFO, "Department %s processed: %d files transferred", 
                department, files_processed);
    return success;
}

TaskStatus transfer_files(void) {
    log_message(DAEMON_LOG_INFO, "Starting file transfer operation");
    
    // Lock directories
    if (lock_directories() != LOCK_SUCCESS) {
        log_message(DAEMON_LOG_ERROR, "Failed to lock directories for transfer");
        return TASK_FAILURE;
    }
    
    // Report task in progress
    report_task_status(TASK_TRANSFER, TASK_IN_PROGRESS);
    
    bool success = true;
    int total_files = 0;
    
    // Process each department directory
    for (int i = 0; i < NUM_DEPARTMENTS; i++) {
        if (!process_department_directory(DEPARTMENTS[i])) {
            success = false;
            log_message(DAEMON_LOG_WARNING, "Failed to process department: %s", DEPARTMENTS[i]);
        }
    }
    
    // Check reporting directory has files
    DIR *report_dir = opendir(REPORT_DIR);
    if (report_dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(report_dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                total_files++;
            }
        }
        closedir(report_dir);
        log_message(DAEMON_LOG_INFO, "Reporting directory contains %d files after transfer", total_files);
    } else {
        log_message(DAEMON_LOG_ERROR, "Failed to open reporting directory to check file count");
    }
    
    // Unlock directories
    if (unlock_directories() != UNLOCK_SUCCESS) {
        log_message(DAEMON_LOG_ERROR, "Failed to unlock directories after transfer");
        success = false;
    } else {
        log_message(DAEMON_LOG_DEBUG, "Directories successfully unlocked after transfer");
    }
    
    // Report task status
    TaskStatus status = success ? TASK_SUCCESS : TASK_FAILURE;
    report_task_status(TASK_TRANSFER, status);
    
    log_message(DAEMON_LOG_INFO, "File transfer operation %s", 
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
    log_message(DAEMON_LOG_INFO, "Scheduled next transfer in %d seconds", sleep_time);
}