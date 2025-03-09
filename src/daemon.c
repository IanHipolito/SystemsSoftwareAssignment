#define _POSIX_C_SOURCE 200809L
#include "../include/daemon.h"
#include "../include/config.h"
#include "../include/logging.h"
#include "../include/file_transfer.h"
#include "../include/backup.h"
#include "../include/lock_manager.h"
#include "../include/utils.h"
#include "../include/ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <syslog.h>
#include <features.h>
#include <sys/file.h>
#include <fcntl.h>

// Remove or comment out these variables if they're not used
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

// Global variables
static DaemonStatus daemon_status = DAEMON_STOPPED;
static pid_t daemon_pid = 0;
static bool manual_operation_triggered = false;

// PID file path
#define PID_FILE "./company_daemon.pid"

// Write PID file
static bool write_pid_file(void) {
    // Try multiple potential paths
    const char* pid_paths[] = {
        PID_FILE,           // Configured path
        "./company_daemon.pid",  // Current directory
        "/var/run/company_daemon.pid",  // Standard system pid directory
        "/run/company_daemon.pid"       // Alternative system pid directory
    };
    
    for (size_t i = 0; i < sizeof(pid_paths) / sizeof(pid_paths[0]); i++) {
        FILE *f = fopen(pid_paths[i], "w");
        if (f != NULL) {
            pid_t pid = getpid();
            fprintf(f, "%d\n", pid);
            fclose(f);
            
            // Try to set permissions to be more permissive
            chmod(pid_paths[i], 0666);
            
            syslog(LOG_INFO, "PID file written successfully: %s", pid_paths[i]);
            return true;
        } else {
            syslog(LOG_WARNING, "Failed to create PID file %s: %s", 
                   pid_paths[i], strerror(errno));
        }
    }
    
    return false;
}

// Read PID from file
static pid_t read_pid_file(void) {
    const char* pid_paths[] = {
        PID_FILE,
        "./company_daemon.pid", 
        "/var/run/company_daemon.pid",
        "/run/company_daemon.pid"
    };
    
    for (size_t i = 0; i < sizeof(pid_paths) / sizeof(pid_paths[0]); i++) {
        FILE *f = fopen(pid_paths[i], "r");
        if (f != NULL) {
            pid_t pid;
            if (fscanf(f, "%d", &pid) == 1) {
                fclose(f);
                return pid;
            }
            fclose(f);
        }
    }
    
    return 0;
}

// Remove PID file
bool remove_pid_file(void) {
    const char* pid_paths[] = {
        PID_FILE,
        "./company_daemon.pid", 
        "/var/run/company_daemon.pid",
        "/run/company_daemon.pid"
    };
    
    bool file_removed = false;
    
    for (size_t i = 0; i < sizeof(pid_paths) / sizeof(pid_paths[0]); i++) {
        if (unlink(pid_paths[i]) == 0) {
            log_message(DAEMON_LOG_INFO, "Removed PID file: %s", pid_paths[i]);
            file_removed = true;
        } else if (errno != ENOENT) {
            // Log error if it's not just "file not found"
            log_message(DAEMON_LOG_WARNING, "Failed to remove PID file %s: %s", 
                        pid_paths[i], strerror(errno));
        }
    }
    
    return file_removed;
}


// Check if process with given PID exists
static bool process_exists(pid_t pid) {
    if (pid <= 0) {
        return false;
    }
    
    if (kill(pid, 0) == 0) {
        return true;
    }
    
    return (errno != ESRCH);
}

// Daemonize process
static bool daemonize(void) {
    pid_t pid;
    
    // Fork off the parent process
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Failed to fork first time: %s\n", strerror(errno));
        return false;
    }
    
    // If we got a good PID, then we can exit the parent process
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    // Create a new session and become the session leader
    if (setsid() < 0) {
        fprintf(stderr, "Failed to create a new session: %s\n", strerror(errno));
        return false;
    }
    
    // Ignore certain signals
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    
    // Fork again to ensure we are not a session leader
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Failed to fork second time: %s\n", strerror(errno));
        return false;
    }
    
    // If we got a good PID, then we can exit the parent process
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    // Change the file mode mask
    umask(0);
    
    // Change the current working directory
    // if (chdir("/") < 0) {
    //     fprintf(stderr, "Failed to change working directory: %s\n", strerror(errno));
    //     return false;
    // }
    
    // Open the log file before closing file descriptors
    openlog("company_daemon", LOG_PID | LOG_PERROR, LOG_DAEMON);
    
    // Close standard file descriptors but keep stderr for debugging
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    //close(STDERR_FILENO);  // Comment this out to keep stderr for debugging
    
    return true;
}

// Signal handler
void handle_signal(int signum) {
    switch (signum) {
        case SIGTERM:
        case SIGINT:
            log_message(DAEMON_LOG_INFO, "Received termination signal");
            daemon_status = DAEMON_STOPPING;
            break;
        
        case SIGUSR1:
            log_message(DAEMON_LOG_INFO, "Received signal to trigger manual operation");
            manual_operation_triggered = true;
            break;
        
        default:
            log_message(DAEMON_LOG_WARNING, "Received unsupported signal: %d", signum);
            break;
    }
}

// Set up signal handlers
void setup_signal_handlers(void) {
    struct sigaction sa;
    
    // Set up signal handlers
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    
    log_message(DAEMON_LOG_INFO, "Signal handlers set up");
}

// Monitor upload directory for changes
static void monitor_uploads(void) {
    static time_t last_check_time = 0;
    
    // Check every minute
    time_t now = time(NULL);
    if (now - last_check_time < 60) {
        return;
    }
    
    last_check_time = now;
    
    // Check if all departments have uploaded their reports
    log_message(DAEMON_LOG_INFO, "Checking for department uploads");
    bool all_uploaded = check_department_uploads();
    
    if (!all_uploaded) {
        log_message(DAEMON_LOG_WARNING, "Not all departments have uploaded their reports");
    } else {
        log_message(DAEMON_LOG_INFO, "All departments have uploaded their reports");
    }
    
    // Report task status
    report_task_status(TASK_FILE_CHECK, all_uploaded ? TASK_SUCCESS : TASK_FAILURE);
}

// Check if it's time for scheduled transfer
static bool is_time_for_transfer(void) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    return (t->tm_hour == TRANSFER_HOUR && 
            t->tm_min == TRANSFER_MINUTE && 
            t->tm_sec == TRANSFER_SECOND);
}

// Perform transfer and backup
static void perform_transfer_and_backup(void) {
    log_message(DAEMON_LOG_INFO, "Starting scheduled transfer and backup");
    
    // Perform transfer
    TaskStatus transfer_status = transfer_files();
    
    // Check transfer status explicitly
    if (transfer_status == TASK_SUCCESS) {
        log_message(DAEMON_LOG_INFO, "Transfer completed successfully, starting backup");
        TaskStatus backup_status = backup_reporting_directory();
        
        if (backup_status == TASK_SUCCESS) {
            log_message(DAEMON_LOG_INFO, "Scheduled transfer and backup completed successfully");
        } else {
            log_message(DAEMON_LOG_ERROR, "Scheduled backup failed");
        }
    } else {
        log_message(DAEMON_LOG_ERROR, "Scheduled transfer failed, skipping backup");
    }
}

// Main daemon loop
void daemon_loop(void) {
    daemon_status = DAEMON_RUNNING;
    log_message(DAEMON_LOG_INFO, "Daemon loop started");
    
    // Schedule next transfer
    schedule_next_transfer();
    
    // Loop until signaled to stop
    while (daemon_status == DAEMON_RUNNING) {
        // Check for manual operation trigger
        if (manual_operation_triggered) {
            perform_transfer_and_backup();
            manual_operation_triggered = false;
        }
        
        // Check if it's time for scheduled transfer
        if (is_time_for_transfer()) {
            perform_transfer_and_backup();
            // Schedule next transfer
            schedule_next_transfer();
        }
        
        // Monitor upload directory
        monitor_uploads();
        
        // Sleep for 1 second
        sleep(1);
    }
    
    log_message(DAEMON_LOG_INFO, "Daemon loop stopped");
}

// Cleanup function to remove stale daemon processes
static void cleanup_stale_processes(void) {
    // Read the PID file to check for an existing process
    pid_t existing_pid = read_pid_file();
    
    if (existing_pid > 0) {
        // Check if the process is still running
        if (kill(existing_pid, 0) == -1 && errno == ESRCH) {
            // Process no longer exists, remove PID file
            log_message(DAEMON_LOG_WARNING, 
                "Removing stale PID file for non-existent process %d", 
                existing_pid);
            remove_pid_file();
        } else {
            // Try to terminate the existing process
            log_message(DAEMON_LOG_WARNING, 
                "Attempting to terminate existing daemon process %d", 
                existing_pid);
            kill(existing_pid, SIGTERM);
            
            sleep(1);
            
            // Force kill if it didn't terminate
            if (kill(existing_pid, 0) == 0) {
                kill(existing_pid, SIGKILL);
            }
        }
    }

    // Additional cleanup: remove any stale lock files
    unlink("/var/run/company_daemon.lock");
}

// Start the daemon
bool start_daemon(void) {

    cleanup_stale_processes();

    // Check if already running
    if (is_daemon_running()) {
        log_message(DAEMON_LOG_ERROR, "Daemon is already running");
        return false;
    }
    
    // Ensure only one instance can run
    int lock_fd = open("/var/run/company_daemon.lock", O_CREAT | O_RDWR, 0666);
    if (lock_fd == -1) {
        log_message(DAEMON_LOG_ERROR, "Failed to create lock file: %s", strerror(errno));
        return false;
    }
    
    // Try to acquire an exclusive lock
    if (flock(lock_fd, LOCK_EX | LOCK_NB) == -1) {
        log_message(DAEMON_LOG_ERROR, "Failed to acquire lock: %s", strerror(errno));
        close(lock_fd);
        return false;
    }
    
    // Daemonize process
    if (!daemonize()) {
        log_message(DAEMON_LOG_ERROR, "Failed to daemonize process");
        flock(lock_fd, LOCK_UN);
        close(lock_fd);
        return false;
    }
    
    // Store the lock file descriptor as a global or static variable
    // to keep the lock held throughout the daemon's lifetime
    static int daemon_lock_fd = -1;
    daemon_lock_fd = lock_fd;

    // Continue with existing start_daemon logic
    // Initialize logging
    init_logging();
    
    // Load configuration
    load_config();
    
    // Write PID file
    if (!write_pid_file()) {
        log_message(DAEMON_LOG_ERROR, "Failed to write PID file");
        close_logging();
        flock(daemon_lock_fd, LOCK_UN);
        close(daemon_lock_fd);
        return false;
    }
    
    // Initialize IPC
    if (!init_ipc()) {
        log_message(DAEMON_LOG_ERROR, "Failed to initialize IPC");
        remove_pid_file();
        close_logging();
        flock(daemon_lock_fd, LOCK_UN);
        close(daemon_lock_fd);
        return false;
    }
    
    // Set up signal handlers
    setup_signal_handlers();
    
    daemon_status = DAEMON_STARTED;
    log_message(DAEMON_LOG_INFO, "Daemon started successfully");
    
    return true;
}

pid_t get_daemon_pid(void) {
    return read_pid_file();
}

// Stop the daemon
bool stop_daemon(void) {
    if (daemon_status == DAEMON_STOPPED) {
        return true;
    }
    
    log_message(DAEMON_LOG_INFO, "Stopping daemon");
    
    // Attempt IPC cleanup with error handling
    bool ipc_cleanup_success = true;
    bool pid_removal_success = true;
    bool logging_cleanup_success = true;
    
    // Cleanup IPC resources using the function from ipc.c
    cleanup_ipc();
    
    // Try to remove PID file
    if (!remove_pid_file()) {
        log_message(DAEMON_LOG_WARNING, "Could not remove PID file");
        pid_removal_success = false;
    }
    
    // Close logging
    close_logging();
    
    // Additional cleanup of potential lock or temporary files
    if (unlink("/var/run/company_daemon.lock") == -1 && errno != ENOENT) {
        log_message(DAEMON_LOG_WARNING, "Failed to remove lock file: %s", strerror(errno));
    }
    
    if (unlink("/tmp/company_daemon.lock") == -1 && errno != ENOENT) {
        log_message(DAEMON_LOG_WARNING, "Failed to remove temp lock file: %s", strerror(errno));
    }
    
    daemon_status = DAEMON_STOPPED;
    
    // Return success only if all cleanup steps were successful
    return (ipc_cleanup_success && pid_removal_success && logging_cleanup_success);
}

// Check if the daemon is already running
bool is_daemon_running(void) {
    pid_t pid = read_pid_file();
    
    if (pid <= 0) {
        return false;
    }
    
    // Check process status using kill with signal 0
    if (kill(pid, 0) == 0) {
        // Process exists and we have permission to send signals
        return true;
    }
    
    // Process doesn't exist or we can't send signals
    if (errno == ESRCH) {
        // Process does not exist
        remove_pid_file();
        return false;
    }
    
    // Permission denied or other error
    if (errno == EPERM) {
        log_message(DAEMON_LOG_WARNING, "PID exists but process not accessible");
        return true;
    }
    
    return false;
}

// Trigger manual operation
bool trigger_manual_operation(void) {
    pid_t pid = read_pid_file();
    
    if (pid <= 0) {
        log_message(DAEMON_LOG_ERROR, "Daemon is not running");
        return false;
    }
    
    // Send SIGUSR1 to trigger manual operation
    if (kill(pid, SIGUSR1) == -1) {
        log_message(DAEMON_LOG_ERROR, "Failed to send signal to daemon: %s", strerror(errno));
        return false;
    }
    
    log_message(DAEMON_LOG_INFO, "Manual operation triggered");
    return true;
}

// Get current daemon status
DaemonStatus get_daemon_status(void) {
    return daemon_status;
}