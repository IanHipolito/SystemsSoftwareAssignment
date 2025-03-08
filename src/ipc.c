#include "../include/ipc.h"
#include "../include/config.h"
#include "../include/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

static int msgid = -1;

// Message structure for IPC
typedef struct {
    long mtype;  // Message type (MUST be > 0)
    pid_t sender_pid;  // PID of sender to help with cleanup
    int task_type;     // Ensure consistent size across systems
    int task_status;   // Ensure consistent size across systems
} TaskMessage;

// Robust key generation function
static key_t generate_robust_key(void) {
    // Try multiple methods to generate a unique key
    key_t key;
    
    // Method 1: Try using a file path
    key = ftok("/tmp", 'C');  // 'C' for Company Daemon
    if (key != -1) {
        log_message(DAEMON_LOG_INFO, "Generated IPC key using /tmp: %x", key);
        return key;
    }
    
    // Method 2: Fallback to a static key with process ID mix
    key = (12345 ^ (getpid() << 16));
    log_message(DAEMON_LOG_INFO, "Generated fallback IPC key: %x", key);
    
    return key;
}

// Forceful queue removal function
static void force_remove_queue(int queue_id) {
    struct msqid_ds buf;
    
    // First, try to get queue info to confirm it exists
    if (msgctl(queue_id, IPC_STAT, &buf) == -1) {
        // Queue likely doesn't exist or is already removed
        log_message(DAEMON_LOG_INFO, "Queue already removed or invalid");
        return;
    }

    // Attempt different removal methods
    int removal_attempts[] = {IPC_RMID, IPC_STAT, 0};
    for (int i = 0; removal_attempts[i] != 0; i++) {
        errno = 0;
        int result = msgctl(queue_id, removal_attempts[i], NULL);
        
        if (result == 0) {
            log_message(DAEMON_LOG_INFO, "Successfully removed queue using method %d", removal_attempts[i]);
            return;
        }
        
        if (errno != EINVAL) {
            log_message(DAEMON_LOG_WARNING, 
                "Queue removal attempt %d failed. Error: %s (errno: %d)", 
                removal_attempts[i], strerror(errno), errno);
        }
    }
}

bool init_ipc(void) {
    // Generate a robust IPC key
    key_t key = generate_robust_key();
    if (key == -1) {
        log_message(DAEMON_LOG_ERROR, "Failed to generate IPC key");
        return false;
    }

    // Try to get an existing message queue first
    int existing_msgid = msgget(key, 0);
    if (existing_msgid != -1) {
        // Check queue status
        struct msqid_ds queue_info;
        if (msgctl(existing_msgid, IPC_STAT, &queue_info) != -1) {
            log_message(DAEMON_LOG_WARNING, 
                "Existing message queue found. Current messages: %lu, Last PID: %d", 
                queue_info.msg_qnum, queue_info.msg_lspid);
            
            // If the owning process is no longer running, forcefully remove the queue
            if (kill(queue_info.msg_lspid, 0) == -1 && errno == ESRCH) {
                log_message(DAEMON_LOG_WARNING, "Removing stale message queue");
                force_remove_queue(existing_msgid);
            }
        }
    }

    // Create new message queue
    msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid == -1) {
        log_message(DAEMON_LOG_ERROR, "Failed to create message queue. Error: %s", strerror(errno));
        return false;
    }
    
    log_message(DAEMON_LOG_INFO, "IPC message queue initialized. Key: %x, Queue ID: %d", key, msgid);
    return true;
}

void cleanup_ipc(void) {
    if (msgid != -1) {
        // Attempt to remove message queue with multiple strategies
        force_remove_queue(msgid);
        msgid = -1;
    }
}

bool report_task_status(TaskType type, TaskStatus status) {
    if (msgid == -1) {
        log_message(DAEMON_LOG_ERROR, "IPC not initialized");
        return false;
    }
    
    TaskMessage msg;
    memset(&msg, 0, sizeof(TaskMessage));
    
    // Use type+1 as mtype (message type must be > 0)
    msg.mtype = type + 1;
    msg.sender_pid = getpid();
    msg.task_type = type;
    msg.task_status = status;
    
    // Send message with careful error handling
    int send_result = msgsnd(msgid, &msg, sizeof(TaskMessage) - sizeof(long), IPC_NOWAIT);
    if (send_result == -1) {
        // Detailed error logging
        log_message(DAEMON_LOG_ERROR, "Failed to send task status. Error: %s (errno: %d)", 
                    strerror(errno), errno);
        
        // Handle specific error cases
        switch(errno) {
            case EAGAIN:
                log_message(DAEMON_LOG_WARNING, "Message queue is full");
                break;
            case EINVAL:
                log_message(DAEMON_LOG_ERROR, "Invalid message queue parameters");
                break;
            case EACCES:
                log_message(DAEMON_LOG_ERROR, "Permission denied");
                break;
        }
        
        return false;
    }
    
    const char* task_names[] = {"File Transfer", "Backup", "File Check"};
    const char* status_names[] = {"Success", "Failure", "In Progress"};
    
    log_message(DAEMON_LOG_INFO, "Task status reported: %s - %s", 
                task_names[type], status_names[status]);
    return true;
}

TaskStatus get_task_status(TaskType type) {
    if (msgid == -1) {
        log_message(DAEMON_LOG_ERROR, "IPC not initialized");
        return TASK_FAILURE;
    }
    
    TaskMessage msg;
    
    // Receive message (non-blocking)
    ssize_t recv_result = msgrcv(msgid, &msg, sizeof(TaskMessage) - sizeof(long), 
                                  type + 1, IPC_NOWAIT);
    if (recv_result == -1) {
        if (errno == ENOMSG) {
            // No message available
            return TASK_IN_PROGRESS;
        } else {
            log_message(DAEMON_LOG_ERROR, "Failed to receive task status: %s", strerror(errno));
            return TASK_FAILURE;
        }
    }
    
    return msg.task_status;
}

TaskStatus wait_for_task_completion(TaskType type) {
    if (msgid == -1) {
        log_message(DAEMON_LOG_ERROR, "IPC not initialized");
        return TASK_FAILURE;
    }
    
    TaskMessage msg;
    
    // Receive message (blocking)
    if (msgrcv(msgid, &msg, sizeof(TaskMessage) - sizeof(long), type + 1, 0) == -1) {
        log_message(DAEMON_LOG_ERROR, "Failed to receive task status: %s", strerror(errno));
        return TASK_FAILURE;
    }
    
    return msg.task_status;
}