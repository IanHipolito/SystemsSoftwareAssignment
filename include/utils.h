#ifndef UTILS_H
#define UTILS_H

#include <time.h>
#include <stdbool.h>

// Get current date/time as string in format YYYY-MM-DD
void get_current_date(char* date_str, size_t size);

// Get current time in seconds since Epoch
time_t get_current_time(void);

// Check if the time matches the scheduled transfer time
bool is_transfer_time(void);

// Convert time_t to formatted string
void format_time(time_t time_val, char* time_str, size_t size);

// Get username of the user who modified a file
void get_file_owner(const char* path, char* owner, size_t size);

// Calculate time until next execution of scheduled task
int time_until_next_execution(int hour, int minute, int second);

// Check if a file is XML
bool is_xml_file(const char* filename);

// Check if a file belongs to a department
bool is_department_file(const char* filename, const char* department);

#endif /* UTILS_H */