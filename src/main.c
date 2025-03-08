#include "../include/daemon.h"
#include "../include/logging.h"
#include "../include/config.h"
#include "../include/ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>

// Since daemon_status is static in daemon.c, we'll instead use a different approach
// by calling a function from daemon.h that properly sets the status

void print_usage(const char *program_name)
{
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  start      Start the daemon\n");
    printf("  stop       Stop the daemon\n");
    printf("  status     Check daemon status\n");
    printf("  trigger    Trigger manual backup and transfer\n");
    printf("  debug      Run in foreground (debug mode)\n");
    printf("  help       Display this help message\n");
}

int main(int argc, char *argv[])
{
    // Check command-line arguments
    if (argc < 2)
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Process command
    if (strcmp(argv[1], "start") == 0)
    {
        if (is_daemon_running())
        {
            printf("Daemon is already running\n");
            return EXIT_FAILURE;
        }

        printf("Starting daemon...\n");

        // Add debug output to stderr (will show even if stdout is redirected)
        fprintf(stderr, "Checking directories...\n");
        // Test directory access
        if (access(UPLOAD_DIR, F_OK) == -1)
        {
            fprintf(stderr, "Warning: Upload directory %s does not exist\n", UPLOAD_DIR);
        }
        if (access(REPORT_DIR, F_OK) == -1)
        {
            fprintf(stderr, "Warning: Reporting directory %s does not exist\n", REPORT_DIR);
        }

        if (!start_daemon())
        {
            printf("Failed to start daemon\n");
            return EXIT_FAILURE;
        }

        // Main daemon loop - only reached in child process
        daemon_loop();
        stop_daemon();
    }
    else if (strcmp(argv[1], "debug") == 0)
    {
        printf("Running in debug mode (foreground)...\n");

        // Skip daemonization
        fprintf(stderr, "Debug mode - Initializing logging...\n");
        init_logging();

        fprintf(stderr, "Debug mode - Loading configuration...\n");
        load_config();

        // Check if directories exist after loading config
        fprintf(stderr, "Checking directories after load_config...\n");
        if (access(UPLOAD_DIR, F_OK) == -1)
        {
            fprintf(stderr, "Upload directory %s still does not exist after load_config\n", UPLOAD_DIR);
        }
        else
        {
            fprintf(stderr, "Upload directory %s exists\n", UPLOAD_DIR);
        }

        fprintf(stderr, "Debug mode - Initializing IPC...\n");
        if (!init_ipc())
        {
            fprintf(stderr, "Failed to initialize IPC: %s\n", strerror(errno));
            close_logging();
            return EXIT_FAILURE;
        }

        // Write PID file
        fprintf(stderr, "Debug mode - Writing PID file...\n");
        FILE *f = fopen("./company_daemon.pid", "w");
        if (f)
        {
            fprintf(f, "%d\n", getpid());
            fclose(f);
        }
        else
        {
            fprintf(stderr, "Failed to write local PID file: %s\n", strerror(errno));
        }

        // Set up signal handlers
        fprintf(stderr, "Debug mode - Setting up signal handlers...\n");
        setup_signal_handlers();

        // Run main loop directly
        fprintf(stderr, "Debug mode - Entering main loop...\n");
        daemon_loop();
        stop_daemon();
    } else if (strcmp(argv[1], "stop") == 0) {
    if (!is_daemon_running())
    {
        printf("Daemon is not running\n");
        return EXIT_FAILURE;
    }

    printf("Stopping daemon...\n");
    pid_t pid = get_daemon_pid();
    if (pid > 0)
    {
        // Send SIGTERM to the daemon
        if (kill(pid, SIGTERM) == -1)
        {
            printf("Failed to terminate daemon: %s\n", strerror(errno));
            return EXIT_FAILURE;
        }
        printf("Daemon stopped\n");
    }
    else
    {
        printf("Could not find daemon PID\n");
        return EXIT_FAILURE;
    }
}
else if (strcmp(argv[1], "status") == 0)
{
    if (is_daemon_running())
    {
        printf("Daemon is running\n");
    }
    else
    {
        printf("Daemon is not running\n");
    }
}
else if (strcmp(argv[1], "trigger") == 0)
{
    if (!is_daemon_running())
    {
        printf("Daemon is not running\n");
        return EXIT_FAILURE;
    }

    printf("Triggering manual backup and transfer...\n");
    if (!trigger_manual_operation())
    {
        printf("Failed to trigger manual operation\n");
        return EXIT_FAILURE;
    }
    printf("Manual operation triggered\n");
}
else if (strcmp(argv[1], "help") == 0)
{
    print_usage(argv[0]);
}
else
{
    printf("Unknown command: %s\n", argv[1]);
    print_usage(argv[0]);
    return EXIT_FAILURE;
}

return EXIT_SUCCESS;
}