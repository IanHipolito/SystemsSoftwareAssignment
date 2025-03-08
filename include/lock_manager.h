#ifndef LOCK_MANAGER_H
#define LOCK_MANAGER_H

#include <stdbool.h>

// Lock status
typedef enum {
    LOCK_SUCCESS,
    LOCK_FAILURE,
    UNLOCK_SUCCESS,
    UNLOCK_FAILURE
} LockStatus;

// Lock both upload and reporting directories
LockStatus lock_directories(void);

// Unlock both upload and reporting directories
LockStatus unlock_directories(void);

// Check if directories are currently locked
bool are_directories_locked(void);

#endif /* LOCK_MANAGER_H */