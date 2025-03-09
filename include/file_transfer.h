// include/file_transfer.h
#ifndef FILE_TRANSFER_H
#define FILE_TRANSFER_H

#include <stdbool.h>
#include "../include/ipc.h"

// Transfer all XML files from upload to reporting directory
TaskStatus transfer_files(void);

// Check if a specified file exists in the upload directory
bool check_file_exists(const char* department, const char* filename);

// Schedule next transfer
void schedule_next_transfer(void);

#endif /* FILE_TRANSFER_H */