#ifndef DAEMON_H
#define DAEMON_H

#include <stdbool.h>

// Daemon status
typedef enum {
    DAEMON_STARTED,
    DAEMON_RUNNING,
    DAEMON_STOPPING,
    DAEMON_STOPPED,
    DAEMON_ERROR
} DaemonStatus;

// Initialize and start the daemon
bool start_daemon(void);

// Stop the daemon
bool stop_daemon(void);

// Check if the daemon is already running (singleton pattern)
bool is_daemon_running(void);

// Main daemon loop
void daemon_loop(void);

// Handle signals
void handle_signal(int signum);

// Setup signal handlers
void setup_signal_handlers(void);

// Trigger manual backup and transfer
bool trigger_manual_operation(void);

// Get current daemon status
DaemonStatus get_daemon_status(void);

#endif /* DAEMON_H */