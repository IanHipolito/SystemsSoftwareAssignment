#include "ipc.h"
#include "utils.h"
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>  // For mkfifo

/* Static file descriptor for the FIFO */
static int fifo_fd = -1;

/**
 * Setup IPC mechanisms
 * @return SUCCESS on success, FAILURE on error
 */
int setup_ipc(void) {
    /* Create FIFO if it doesn't exist */
    if (access(FIFO_PATH, F_OK) != 0) {
        if (mkfifo(FIFO_PATH, 0666) != 0) {
            log_error("Failed to create FIFO: %s", strerror(errno));
            return FAILURE;
        }
    }
    
    /* Open FIFO for both reading and writing */
    fifo_fd = open(FIFO_PATH, O_RDWR | O_NONBLOCK);
    if (fifo_fd == -1) {
        log_error("Failed to open FIFO: %s", strerror(errno));
        return FAILURE;
    }
    
    log_operation("IPC setup completed");
    return SUCCESS;
}

/**
 * Cleanup IPC resources
 * @return SUCCESS on success, FAILURE on error
 */
int cleanup_ipc(void) {
    int result = SUCCESS;
    
    /* Close FIFO if open */
    if (fifo_fd != -1) {
        if (close(fifo_fd) != 0) {
            log_error("Failed to close FIFO: %s", strerror(errno));
            result = FAILURE;
        }
        fifo_fd = -1;
    }
    
    /* Remove FIFO */
    if (unlink(FIFO_PATH) != 0 && errno != ENOENT) {
        log_error("Failed to remove FIFO: %s", strerror(errno));
        result = FAILURE;
    }
    
    log_operation("IPC cleanup completed");
    return result;
}

/**
 * Send an IPC message
 * @param msg Pointer to the message to send
 * @return SUCCESS on success, FAILURE on error
 */
int send_ipc_message(IPCMessage* msg) {
    ssize_t bytes_written;
    
    /* Check if FIFO is open */
    if (fifo_fd == -1) {
        log_error("FIFO not open for sending message");
        return FAILURE;
    }
    
    /* Set the sender PID */
    msg->sender_pid = getpid();
    
    /* Write the message to the FIFO */
    bytes_written = write(fifo_fd, msg, sizeof(IPCMessage));
    if (bytes_written != sizeof(IPCMessage)) {
        log_error("Failed to write to FIFO: %s", strerror(errno));
        return FAILURE;
    }
    
    return SUCCESS;
}

/**
 * Receive an IPC message (non-blocking)
 * @param msg Pointer to store the received message
 * @return SUCCESS on success, FAILURE on error or no message available
 */
int receive_ipc_message(IPCMessage* msg) {
    ssize_t bytes_read;
    
    /* Check if FIFO is open */
    if (fifo_fd == -1) {
        log_error("FIFO not open for receiving message");
        return FAILURE;
    }
    
    /* Try to read a message from the FIFO */
    bytes_read = read(fifo_fd, msg, sizeof(IPCMessage));
    
    /* Check for errors or no data */
    if (bytes_read == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            /* No message available (non-blocking) */
            return FAILURE;
        } else {
            log_error("Failed to read from FIFO: %s", strerror(errno));
            return FAILURE;
        }
    } else if (bytes_read != sizeof(IPCMessage)) {
        /* Partial read */
        log_error("Partial read from FIFO: %ld bytes", (long)bytes_read);
        return FAILURE;
    }
    
    return SUCCESS;
}

/**
 * Create a process that will report back its completion status
 * @param function Function to execute in the child process
 * @param msg_type Message type for completion notification
 * @return PID of the child process or -1 on error
 */
pid_t create_reporting_process(int (*function)(void), int msg_type) {
    pid_t pid;
    
    /* Fork a new process */
    pid = fork();
    
    if (pid < 0) {
        /* Fork failed */
        log_error("Failed to fork process: %s", strerror(errno));
        return -1;
    } else if (pid == 0) {
        /* Child process */
        IPCMessage msg;
        int result;
        
        log_operation("Starting child process (PID: %d) for operation type: %d", getpid(), msg_type);
        
        /* Execute the function */
        result = function();
        
        /* Prepare completion message */
        msg.type = msg_type;
        msg.sender_pid = getpid();
        msg.status = result;
        
        if (result == SUCCESS) {
            snprintf(msg.message, MAX_LINE_LENGTH, "Operation type %d completed successfully by PID %d", 
                    msg_type, getpid());
            log_operation("%s", msg.message);
        } else {
            snprintf(msg.message, MAX_LINE_LENGTH, "Operation type %d failed (executed by PID %d)", 
                    msg_type, getpid());
            log_error("%s", msg.message);
        }
        
        /* Send the completion message */
        if (send_ipc_message(&msg) != SUCCESS) {
            log_error("Failed to send completion message for operation type %d", msg_type);
        }
        
        /* Exit with the result code */
        exit(result == SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE);
    }
    
    /* Parent process - log the creation and return the child's PID */
    log_operation("Created child process with PID %d for operation type %d", pid, msg_type);
    return pid;
}