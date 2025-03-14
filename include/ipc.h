#ifndef IPC_H
#define IPC_H

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

/* Path definitions */
#define FIFO_PATH "/var/run/company_daemon_pipe"

/* IPC message types */
#define MSG_BACKUP_START     1
#define MSG_BACKUP_COMPLETE  2
#define MSG_TRANSFER_START   3
#define MSG_TRANSFER_COMPLETE 4
#define MSG_ERROR            5
#define MSG_URGENT_CHANGE 6

/* Maximum buffer sizes */
#define MAX_LINE_LENGTH 2048

/* Return codes */
#define SUCCESS 0
#define FAILURE -1

/**
 * Structure for inter-process communication messages
 */
typedef struct {
    int type;                        /* Message type */
    pid_t sender_pid;                /* Sender process ID */
    int status;                      /* Status code */
    char message[MAX_LINE_LENGTH];   /* Additional message text */
} IPCMessage;

/**
 * Setup IPC mechanisms
 * @return SUCCESS on success, FAILURE on error
 */
int setup_ipc(void);

/**
 * Cleanup IPC resources
 * @return SUCCESS on success, FAILURE on error
 */
int cleanup_ipc(void);

/**
 * Send an IPC message
 * @param msg Pointer to the message to send
 * @return SUCCESS on success, FAILURE on error
 */
int send_ipc_message(IPCMessage* msg);

/**
 * Receive an IPC message (non-blocking)
 * @param msg Pointer to store the received message
 * @return SUCCESS on success, FAILURE on error or no message available
 */
int receive_ipc_message(IPCMessage* msg);

/**
 * Create a process that will report back its completion status
 * @param function Function to execute in the child process
 * @param msg_type Message type for completion notification
 * @return PID of the child process or -1 on error
 */
pid_t create_reporting_process(int (*function)(void), int msg_type);

#endif /* IPC_H */