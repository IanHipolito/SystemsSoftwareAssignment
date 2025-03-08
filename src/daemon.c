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

// Global variables
static DaemonStatus daemon_status = DAEMON_STOPPED;
static pid_t daemon_pid = 0;
static bool manual_operation_triggered = false;

// PID file path
#define PID_FILE "/var/run/company_daemon.pid"

// Write PID file
static bool write_pid_file(void) {
    FILE *f = fopen(PID_FILE, "w");
    if (f == NULL) {
        log_message(DAEMON_LOG_ERROR, "Failed to create PID file: %s", strerror(errno));
        return false;
    }
    
    fprintf(f, "%d\n", getpid());
    fclose(f);
    return true;
}

// Read PID from file
static pid_t read_pid_file(void) {
    FILE *f = fopen(PID_FILE, "r");
    if (f == NULL) {
        // Try local file instead
        f = fopen("./company_daemon.pid", "r");
        if (f == NULL) {
            return 0;
        }
    }
    
    pid_t pid;
    if (fscanf(f, "%d", &pid) != 1) {
        pid = 0;
    }
    
    fclose(f);
    return pid;
}

// Remove PID file
static void remove_pid_file(void) {
    unlink(PID_FILE);
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
        log_message(DAEMON_LOG_ERROR, "Failed to fork first time: %s", strerror(errno));
        return false;
    }
    
    // If we got a good PID, then we can exit the parent process
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    // Create a new session and become the session leader
    if (setsid() < 0) {
        log_message(DAEMON_LOG_ERROR, "Failed to create a new session: %s", strerror(errno));
        return false;
    }
    
    // Ignore certain signals
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    
    // Fork again to ensure we are not a session leader
    pid = fork();
    if (pid < 0) {
        log_message(DAEMON_LOG_ERROR, "Failed to fork second time: %s", strerror(errno));
        return false;
    }
    
    // If we got a good PID, then we can exit the parent process
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    // Change the file mode mask
    umask(0);
    
    // Change the current working directory
    if (chdir("/") < 0) {
        log_message(DAEMON_LOG_ERROR, "Failed to change working directory: %s", strerror(errno));
        return false;
    }
    
    // Close all open file descriptors
    for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
        close(x);
    }
    
    // Open the log file
    openlog("company_daemon", LOG_PID, LOG_DAEMON);
    
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
    
    // If transfer successful, perform backup
    if (transfer_status == TASK_SUCCESS) {
        TaskStatus backup_status = backup_reporting_directory();
        
        if (backup_status == TASK_SUCCESS) {
            log_message(DAEMON_LOG_INFO, "Scheduled transfer and backup completed successfully");
        } else {
            log_message(DAEMON_LOG_ERROR, "Scheduled backup failed");
        }
    } else {
        log_message(DAEMON_LOG_ERROR, "Scheduled transfer failed");
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

// Start the daemon
bool start_daemon(void) {
    // Check if already running
    if (is_daemon_running()) {
        log_message(DAEMON_LOG_ERROR, "Daemon is already running");
        return false;
    }
    
    // Daemonize process
    if (!daemonize()) {
        log_message(DAEMON_LOG_ERROR, "Failed to daemonize process");
        return false;
    }
    
    // Initialize logging
    init_logging();
    
    // Load configuration
    load_config();
    
    // Write PID file
    if (!write_pid_file()) {
        log_message(DAEMON_LOG_ERROR, "Failed to write PID file");
        close_logging();
        return false;
    }
    
    // Initialize IPC
    if (!init_ipc()) {
        log_message(DAEMON_LOG_ERROR, "Failed to initialize IPC");
        remove_pid_file();
        close_logging();
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
    
    // Clean up IPC
    cleanup_ipc();
    
    // Remove PID file
    remove_pid_file();
    
    // Close logging
    close_logging();
    
    daemon_status = DAEMON_STOPPED;
    return true;
}

// Check if the daemon is already running
bool is_daemon_running(void) {
    pid_t pid = read_pid_file();
    
    if (pid > 0 && process_exists(pid)) {
        daemon_pid = pid;
        return true;
    }
    
    // PID file exists but process doesn't; clean up
    if (pid > 0) {
        remove_pid_file();
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