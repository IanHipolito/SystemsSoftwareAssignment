#include "../include/backup.h"
#include "../include/config.h"
#include "../include/logging.h"
#include "../include/lock_manager.h"
#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

static bool create_backup_directory(char* backup_path, size_t size) {
   char date_str[20];
   time_t now = time(NULL);
   struct tm *t = localtime(&now);
   
   // Format date for backup directory name
   strftime(date_str, sizeof(date_str), "%Y-%m-%d_%H-%M-%S", t);
   
   // Create path for backup directory
   snprintf(backup_path, size, "%s/backup_%s", BACKUP_DIR, date_str);
   
   // Create directory
   if (mkdir(backup_path, 0755) == -1) {
       log_message(LOG_ERROR, "Failed to create backup directory %s: %s", 
                  backup_path, strerror(errno));
       return false;
   }
   
   log_message(LOG_INFO, "Created backup directory: %s", backup_path);
   return true;
}

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
   
   log_message(LOG_DEBUG, "Backed up file %s to %s (%d bytes)", 
              src_path, dest_path, total_bytes);
   return true;
}

TaskStatus backup_reporting_directory(void) {
   log_message(LOG_INFO, "Starting backup operation");
   
   // Lock directories
   if (lock_directories() != LOCK_SUCCESS) {
       log_message(LOG_ERROR, "Failed to lock directories for backup");
       return TASK_FAILURE;
   }
   
   // Report task in progress
   report_task_status(TASK_BACKUP, TASK_IN_PROGRESS);
   
   // Create backup directory
   char backup_path[512];
   if (!create_backup_directory(backup_path, sizeof(backup_path))) {
       unlock_directories();
       report_task_status(TASK_BACKUP, TASK_FAILURE);
       return TASK_FAILURE;
   }
   
   bool success = true;
   int file_count = 0;
   
   // Open reporting directory
   DIR *dir = opendir(REPORT_DIR);
   if (dir == NULL) {
       log_message(LOG_ERROR, "Failed to open reporting directory: %s", strerror(errno));
       unlock_directories();
       report_task_status(TASK_BACKUP, TASK_FAILURE);
       return TASK_FAILURE;
   }
   
   // Read directory entries
   struct dirent *entry;
   while ((entry = readdir(dir)) != NULL) {
       // Skip "." and ".."
       if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
           continue;
       }
       
       // Only backup XML files
       if (!is_xml_file(entry->d_name)) {
           continue;
       }
       
       // Create full paths
       char src_path[1024];
       char dest_path[1024];
       snprintf(src_path, sizeof(src_path), "%s/%s", REPORT_DIR, entry->d_name);
       snprintf(dest_path, sizeof(dest_path), "%s/%s", backup_path, entry->d_name);
       
       // Copy file
       if (!copy_file(src_path, dest_path)) {
           success = false;
           log_message(LOG_ERROR, "Failed to backup file %s", src_path);
       } else {
           file_count++;
       }
   }
   
   closedir(dir);
   
   // Unlock directories
   if (unlock_directories() != UNLOCK_SUCCESS) {
       log_message(LOG_ERROR, "Failed to unlock directories after backup");
       success = false;
   }
   
   // Report task status
   TaskStatus status = success ? TASK_SUCCESS : TASK_FAILURE;
   report_task_status(TASK_BACKUP, status);
   
   log_message(LOG_INFO, "Backup operation %s, %d files backed up", 
              success ? "completed successfully" : "failed", file_count);
   
   return status;
}