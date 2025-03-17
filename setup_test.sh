#!/bin/bash

# Create necessary directories
sudo mkdir -p /var/company/upload/warehouse
sudo mkdir -p /var/company/upload/manufacturing
sudo mkdir -p /var/company/upload/sales
sudo mkdir -p /var/company/upload/distribution
sudo mkdir -p /var/company/reporting
sudo mkdir -p /var/company/backup

# Set appropriate permissions
sudo chmod 777 /var/company/upload
sudo chmod 777 /var/company/upload/warehouse
sudo chmod 777 /var/company/upload/manufacturing
sudo chmod 777 /var/company/upload/sales
sudo chmod 777 /var/company/upload/distribution
sudo chmod 777 /var/company/reporting
sudo chmod 777 /var/company/backup

# Create log files with appropriate permissions
sudo mkdir -p /var/log
sudo touch /var/log/company_daemon.log
sudo touch /var/log/company_changes.log
sudo chmod 666 /var/log/company_daemon.log
sudo chmod 666 /var/log/company_changes.log

# Create FIFO for IPC
sudo mkdir -p /var/run
sudo rm -f /var/run/company_daemon_pipe
sudo mkfifo /var/run/company_daemon_pipe
sudo chmod 666 /var/run/company_daemon_pipe

# Ensure PID file location is writable
sudo touch /var/run/company_daemon.pid
sudo chmod 666 /var/run/company_daemon.pid

# Create sample XML files for testing
DATE=$(date +%Y-%m-%d)
echo "<?xml version=\"1.0\"?><report><data>Warehouse test data</data></report>" | sudo tee /var/company/upload/warehouse_${DATE}.xml > /dev/null
echo "<?xml version=\"1.0\"?><report><data>Manufacturing test data</data></report>" | sudo tee /var/company/upload/manufacturing_${DATE}.xml > /dev/null
echo "<?xml version=\"1.0\"?><report><data>Sales test data</data></report>" | sudo tee /var/company/upload/sales_${DATE}.xml > /dev/null
echo "<?xml version=\"1.0\"?><report><data>Distribution test data</data></report>" | sudo tee /var/company/upload/distribution_${DATE}.xml > /dev/null

# Create a test client for IPC
cat > ipc_test_client.c << 'EOF'
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
EOF

# Compile the test client
gcc -o ipc_test_client ipc_test_client.c

echo "Test environment setup completed successfully."
echo "Sample XML files created with today's date: ${DATE}"
echo "IPC test client compiled: ipc_test_client"
echo "Usage: ./ipc_test_client [backup|transfer|status]"