#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#define FIFO_PATH "/var/run/company_daemon_pipe"
#define MAX_LINE_LENGTH 2048

typedef struct {
    int type;
    pid_t sender_pid;
    int status;
    char message[MAX_LINE_LENGTH];
} IPCMessage;

int main(int argc, char *argv[]) {
    int fd;
    IPCMessage msg;
    
    if (argc < 2) {
        printf("Usage: %s <command>\n", argv[0]);
        printf("Commands: backup, transfer, status\n");
        return 1;
    }
    
    fd = open(FIFO_PATH, O_WRONLY);
    if (fd == -1) {
        perror("Failed to open FIFO");
        return 1;
    }
    
    if (strcmp(argv[1], "backup") == 0) {
        msg.type = 1; // Backup command
    } else if (strcmp(argv[1], "transfer") == 0) {
        msg.type = 3; // Transfer command
    } else if (strcmp(argv[1], "status") == 0) {
        msg.type = 5; // Status command
    } else {
        printf("Unknown command: %s\n", argv[1]);
        close(fd);
        return 1;
    }
    
    msg.sender_pid = getpid();
    msg.status = 0;
    snprintf(msg.message, MAX_LINE_LENGTH, "Command from test client: %s", argv[1]);
    
    if (write(fd, &msg, sizeof(IPCMessage)) != sizeof(IPCMessage)) {
        perror("Failed to write to FIFO");
        close(fd);
        return 1;
    }
    
    printf("Command sent successfully\n");
    close(fd);
    
    return 0;
}
