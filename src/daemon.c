/**
 * @file daemon.c
 * @brief Daemon initialization and main control functions
 */
#define _POSIX_C_SOURCE 200809L
#include "daemon.h"
#include "report_system.h"
#include "utils.h"
#include "backup.h"
#include "file_operations.h"
#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

/* Global variables */
static volatile sig_atomic_t daemon_exit = 0;
static volatile sig_atomic_t force_backup = 0;
static volatile sig_atomic_t force_transfer = 0;

/**
 * Signal handler for the daemon
 * @param sig Signal number
 */
void signal_handler(int sig) {
    switch (sig) {
        case SIGTERM:
        case SIGINT:
            daemon_exit = 1;
            break;
        case SIGUSR1:
            force_backup = 1;
            break;
        case SIGUSR2:
            force_transfer = 1;
            break;
        case SIGHUP:
            /* Could be used to reload configuration */
            break;
    }
}

/**
 * Setup all signal handlers for the daemon
 */
void setup_signal_handlers(void) {
    struct sigaction sa;
    
    /* Setup the signal handler structure */
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    /* Register signal handlers */
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    
    /* Ignore these signals */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
}

/**
 * Create the PID file for the daemon
 * @return SUCCESS on success, FAILURE on error
 */
int create_pid_file(void) {
    FILE *pid_fp;
    
    pid_fp = fopen(PID_FILE, "w");
    if (pid_fp == NULL) {
        log_error("Cannot create PID file: %s", strerror(errno));
        return FAILURE;
    }
    
    fprintf(pid_fp, "%d\n", getpid());
    fclose(pid_fp);
    
    return SUCCESS;
}

/**
 * Check if another instance of the daemon is already running
 * @return TRUE if another instance is running, FALSE otherwise
 */
int check_singleton(void) {
    FILE *pid_fp;
    pid_t pid;
    int result = FALSE;
    
    /* Try to open the PID file */
    pid_fp = fopen(PID_FILE, "r");
    if (pid_fp == NULL) {
        /* PID file doesn't exist, so no other instance is running */
        return FALSE;
    }
    
    /* Read the PID from the file */
    if (fscanf(pid_fp, "%d", &pid) == 1) {
        /* Check if a process with this PID exists */
        if (kill(pid, 0) == 0) {
            /* Process exists, another instance is running */
            result = TRUE;
        }
    }
    
    fclose(pid_fp);
    return result;
}

/**
 * Initialize the daemon process
 * @return SUCCESS on success, FAILURE on error
 */
int daemon_init(void) {
    pid_t pid, sid;
    
    /* Check if another instance is already running */
    if (check_singleton()) {
        fprintf(stderr, "Another instance of the daemon is already running.\n");
        return FAILURE;
    }
    
    /* Fork the parent process */
    pid = fork();
    
    /* Check for fork() error */
    if (pid < 0) {
        perror("Error forking daemon process");
        return FAILURE;
    }
    
    /* Exit the parent process if fork was successful */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    /* Change file mode mask */
    umask(0);
    
    /* Create a new session ID for the child process */
    sid = setsid();
    if (sid < 0) {
        perror("Error creating session for daemon");
        return FAILURE;
    }
    
    /* Change the current working directory to root */
    if (chdir("/") < 0) {
        perror("Error changing directory for daemon");
        return FAILURE;
    }
    
    /* Close standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    /* Redirect standard file descriptors to /dev/null */
    open("/dev/null", O_RDONLY);  /* stdin */
    open("/dev/null", O_WRONLY);  /* stdout */
    open("/dev/null", O_WRONLY);  /* stderr */
    
    /* Create PID file */
    if (create_pid_file() != SUCCESS) {
        return FAILURE;
    }
    
    /* Setup signal handlers */
    setup_signal_handlers();
    
    /* Setup logging */
    openlog("report_daemon", LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "Report daemon started");
    
    /* Create necessary directories if they don't exist */
    create_directory_if_not_exists(UPLOAD_DIR);
    create_directory_if_not_exists(DASHBOARD_DIR);
    create_directory_if_not_exists(BACKUP_DIR);
    create_directory_if_not_exists(LOG_DIR);
    
    /* Setup IPC */
    if (setup_ipc() != SUCCESS) {
        log_error("Failed to setup IPC");
        return FAILURE;
    }
    
    /* Set initial directory permissions */
    set_directory_permissions(UPLOAD_DIR, UPLOAD_PERMISSIONS);
    set_directory_permissions(DASHBOARD_DIR, DASHBOARD_PERMISSIONS);
    
    log_operation("Daemon initialization complete");
    return SUCCESS;
}

/**
 * Cleanup daemon resources before exit
 */
void daemon_cleanup(void) {
    /* Remove PID file */
    unlink(PID_FILE);
    
    /* Cleanup IPC */
    cleanup_ipc();
    
    /* Close system log */
    closelog();
    
    log_operation("Daemon shutdown complete");
}

/**
 * Main daemon loop
 */
void daemon_main_loop(void) {
    time_t now, last_check = 0;
    struct tm *tm_now;
    
    log_operation("Entering main daemon loop");
    
    while (!daemon_exit) {
        /* Get current time */
        now = time(NULL);
        tm_now = localtime(&now);
        
        /* Check if it's time to transfer files (1:00 AM) */
        if ((tm_now->tm_hour == TRANSFER_HOUR && 
             tm_now->tm_min == TRANSFER_MINUTE) || force_transfer) {
            
            log_operation("Starting scheduled file transfer and backup");
            
            /* Lock directories before operations */
            lock_directories();
            
            /* Transfer reports from upload to dashboard */
            if (transfer_reports() == SUCCESS) {
                log_operation("File transfer completed successfully");
            } else {
                log_error("File transfer failed");
            }
            
            /* Check for missing department reports */
            check_missing_reports();
            
            /* Backup the dashboard directory */
            if (backup_dashboard() == SUCCESS) {
                log_operation("Backup completed successfully");
            } else {
                log_error("Backup failed");
            }
            
            /* Unlock directories after operations */
            unlock_directories();
            
            /* Reset forced transfer flag */
            force_transfer = 0;
        }
        
        /* Check for directory changes every 5 seconds */
        if (now - last_check >= 5 || force_backup) {
            monitor_directory_changes();
            last_check = now;
            
            /* If force_backup is set, perform a backup */
            if (force_backup) {
                log_operation("Starting manual backup");
                
                /* Lock directories */
                lock_directories();
                
                /* Backup the dashboard directory */
                if (backup_dashboard() == SUCCESS) {
                    log_operation("Manual backup completed successfully");
                } else {
                    log_error("Manual backup failed");
                }
                
                /* Unlock directories */
                unlock_directories();
                
                /* Reset forced backup flag */
                force_backup = 0;
            }
        }
        
        /* Sleep for 1 second before next iteration */
        sleep(1);
    }
    
    log_operation("Exiting main daemon loop");
}

/**
 * Main entry point for the daemon
 */
int main(int argc, char *argv[]) {
    /* Initialize the daemon */
    if (daemon_init() != SUCCESS) {
        return EXIT_FAILURE;
    }
    
    /* Run the main daemon loop */
    daemon_main_loop();
    
    /* Cleanup before exit */
    daemon_cleanup();
    
    return EXIT_SUCCESS;
}