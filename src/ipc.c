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

// Message structure for IPC (include space for additional data)
typedef struct {
    long mtype;  // Message type
    TaskStatus status;
} TaskMessage;

static int msgid = -1;

bool init_ipc(void) {
    // Create message queue
    msgid = msgget(IPC_KEY, IPC_CREAT | IPC_PERMS);
    if (msgid == -1) {
        log_message(DAEMON_LOG_ERROR, "Failed to create message queue: %s", strerror(errno));
        return false;
    }
    
    log_message(DAEMON_LOG_INFO, "IPC message queue initialized");
    return true;
}

void cleanup_ipc(void) {
    if (msgid != -1) {
        // Remove message queue
        if (msgctl(msgid, IPC_RMID, NULL) == -1) {
            log_message(DAEMON_LOG_ERROR, "Failed to remove message queue: %s", strerror(errno));
        } else {
            log_message(DAEMON_LOG_INFO, "IPC message queue removed");
        }
        msgid = -1;
    }
}

bool report_task_status(TaskType type, TaskStatus status) {
    if (msgid == -1) {
        log_message(DAEMON_LOG_ERROR, "IPC not initialized");
        return false;
    }
    
    TaskMessage msg;
    msg.mtype = type + 1;  // Message type must be > 0
    msg.status = status;
    
    // Send message
    if (msgsnd(msgid, &msg, sizeof(TaskStatus), 0) == -1) {
        // More detailed error logging
        log_message(DAEMON_LOG_ERROR, "Failed to send task status. Error: %s (errno: %d)", 
                    strerror(errno), errno);
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
    if (msgrcv(msgid, &msg, sizeof(TaskStatus), type + 1, IPC_NOWAIT) == -1) {
        if (errno == ENOMSG) {
            // No message available
            return TASK_IN_PROGRESS;
        } else {
            log_message(DAEMON_LOG_ERROR, "Failed to receive task status: %s", strerror(errno));
            return TASK_FAILURE;
        }
    }
    
    return msg.status;
}

TaskStatus wait_for_task_completion(TaskType type) {
    if (msgid == -1) {
        log_message(DAEMON_LOG_ERROR, "IPC not initialized");
        return TASK_FAILURE;
    }
    
    TaskMessage msg;
    
    // Receive message (blocking)
    if (msgrcv(msgid, &msg, sizeof(TaskStatus), type + 1, 0) == -1) {
        log_message(DAEMON_LOG_ERROR, "Failed to receive task status: %s", strerror(errno));
        return TASK_FAILURE;
    }
    
    return msg.status;
}