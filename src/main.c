#include "../include/daemon.h"
#include "../include/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

void print_usage(const char* program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  start      Start the daemon\n");
    printf("  stop       Stop the daemon\n");
    printf("  status     Check daemon status\n");
    printf("  trigger    Trigger manual backup and transfer\n");
    printf("  help       Display this help message\n");
}

int main(int argc, char *argv[]) {
    // Check command-line arguments
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    // Process command
    if (strcmp(argv[1], "start") == 0) {
        if (is_daemon_running()) {
            printf("Daemon is already running\n");
            return EXIT_FAILURE;
        }
        
        printf("Starting daemon...\n");
        if (!start_daemon()) {
            printf("Failed to start daemon\n");
            return EXIT_FAILURE;
        }
        
        // Main daemon loop - only reached in child process
        daemon_loop();
        stop_daemon();
        
    } else if (strcmp(argv[1], "stop") == 0) {
        if (!is_daemon_running()) {
            printf("Daemon is not running\n");
            return EXIT_FAILURE;
        }
        
        printf("Stopping daemon...\n");
        pid_t pid = get_daemon_pid();
        if (pid > 0) {
            // Send SIGTERM to the daemon
            if (kill(pid, SIGTERM) == -1) {
                perror("Failed to terminate daemon");
                return EXIT_FAILURE;
            }
            printf("Daemon stopped\n");
        } else {
            printf("Could not find daemon PID\n");
            return EXIT_FAILURE;
        }
        
    } else if (strcmp(argv[1], "status") == 0) {
        if (is_daemon_running()) {
            printf("Daemon is running\n");
        } else {
            printf("Daemon is not running\n");
        }
        
    } else if (strcmp(argv[1], "trigger") == 0) {
        if (!is_daemon_running()) {
            printf("Daemon is not running\n");
            return EXIT_FAILURE;
        }
        
        printf("Triggering manual backup and transfer...\n");
        if (!trigger_manual_operation()) {
            printf("Failed to trigger manual operation\n");
            return EXIT_FAILURE;
        }
        printf("Manual operation triggered\n");
        
    } else if (strcmp(argv[1], "help") == 0) {
        print_usage(argv[0]);
        
    } else {
        printf("Unknown command: %s\n", argv[1]);
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}