#ifndef DAEMON_H
#define DAEMON_H

#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* Path definitions */
#define PID_FILE       "/var/run/company_daemon.pid"
#define LOCK_FILE      "/var/run/company_daemon.lock"

/* Time settings */
#define TRANSFER_HOUR   1    /* 1:00 AM */
#define TRANSFER_MINUTE 0
#define UPLOAD_DEADLINE_HOUR 23   /* 11:30 PM */
#define UPLOAD_DEADLINE_MINUTE 30

/* Return codes */
#define SUCCESS 0
#define FAILURE -1

/* Boolean definitions */
#define TRUE  1
#define FALSE 0

/**
 * Signal handler for the daemon
 * @param sig Signal number
 */
void signal_handler(int sig);

/**
 * Setup all signal handlers for the daemon
 */
void setup_signal_handlers(void);

/**
 * Create the PID file for the daemon
 * @return SUCCESS on success, FAILURE on error
 */
int create_pid_file(void);

/**
 * Check if another instance of the daemon is already running
 * @return TRUE if another instance is running, FALSE otherwise
 */
int check_singleton(void);

/**
 * Initialize the daemon process
 * @return SUCCESS on success, FAILURE on error
 */
int daemon_init(void);

/**
 * Cleanup daemon resources before exit
 */
void daemon_cleanup(void);

/**
 * Main daemon loop
 */
void daemon_main_loop(void);

#endif /* DAEMON_H */