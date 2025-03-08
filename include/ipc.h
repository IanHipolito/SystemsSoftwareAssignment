#ifndef IPC_H
#define IPC_H

#include <stdbool.h>

// Task status
typedef enum {
    TASK_SUCCESS,
    TASK_FAILURE,
    TASK_IN_PROGRESS
} TaskStatus;

// Task types
typedef enum {
    TASK_TRANSFER,
    TASK_BACKUP,
    TASK_FILE_CHECK
} TaskType;

// Initialize IPC
bool init_ipc(void);

// Clean up IPC resources
void cleanup_ipc(void);

// Report task status
bool report_task_status(TaskType type, TaskStatus status);

// Get task status
TaskStatus get_task_status(TaskType type);

// Wait for task completion (blocks until task completes)
TaskStatus wait_for_task_completion(TaskType type);

#endif /* IPC_H */