#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* Department definitions */
#define DEPT_WAREHOUSE    "Warehouse"
#define DEPT_MANUFACTURING "Manufacturing"
#define DEPT_SALES        "Sales"
#define DEPT_DISTRIBUTION "Distribution"

/* File naming conventions */
#define REPORT_EXTENSION  ".xml"
#define REPORT_PREFIX     ""
#define REPORT_DATE_FORMAT "%Y-%m-%d"

/* Path definitions */
#define UPLOAD_DIR      "/var/company/upload"
#define DASHBOARD_DIR   "/var/company/reporting"

/* Maximum buffer sizes */
#define MAX_PATH_LENGTH 1024
#define MAX_USER_LENGTH 256

/* Return codes */
#define SUCCESS 0
#define FAILURE -1

/**
 * Structure to hold information about a report file
 */
typedef struct {
    char path[MAX_PATH_LENGTH];       /* Full path to the file */
    char filename[MAX_PATH_LENGTH];   /* Just the filename */
    char department[MAX_USER_LENGTH]; /* Department name */
    time_t timestamp;                 /* Last modification time */
    char owner[MAX_USER_LENGTH];      /* Owner of the file */
    int size;                         /* File size in bytes */
} ReportFile;

/**
 * Structure to log changes to report files
 */
typedef struct {
    char username[MAX_USER_LENGTH]; /* User who made the change */
    char filename[MAX_PATH_LENGTH]; /* File that was changed */
    char action[MAX_USER_LENGTH];   /* Action performed (create, modify, delete) */
    time_t timestamp;               /* When the change occurred */
} ChangeRecord;

/**
 * Transfer reports from upload directory to dashboard directory
 * @return SUCCESS on success, FAILURE on error
 */
int transfer_reports(void);

/**
 * Check for missing department reports
 * @return Number of missing reports
 */
int check_missing_reports(void);

/**
 * Extract department name from filename
 * @param filename The filename to parse
 * @param department Buffer to store extracted department
 * @param dept_size Size of the department buffer
 * @return Pointer to department buffer or NULL on error
 */
char* extract_department_from_filename(const char* filename, char* department, size_t dept_size);

/**
 * Monitor directory for changes
 * @return SUCCESS on success, FAILURE on error
 */
int monitor_directory_changes(void);

/**
 * Scan a directory and return information about all files
 * @param dir_path Path to the directory to scan
 * @param files Pointer to an array of ReportFile structures to populate
 * @param count Pointer to store the number of files found
 * @return SUCCESS on success, FAILURE on error
 */
int scan_directory(const char* dir_path, ReportFile** files, int* count);

/**
 * Log file change to the change log
 * @param username Username who made the change
 * @param filename Filename that was changed
 * @param action Action performed (create, modify, delete)
 * @return SUCCESS on success, FAILURE on error
 */
int log_file_change(const char* username, const char* filename, const char* action);

/**
 * Get the owner of a file
 * @param path Path to the file
 * @param owner Buffer to store the owner name
 * @param owner_size Size of the owner buffer
 * @return SUCCESS on success, FAILURE on error
 */
int get_file_owner(const char* path, char* owner, size_t owner_size);

/**
 * Free memory allocated for report files
 * @param files Pointer to the array of ReportFile structures
 * @param count Number of files in the array
 */
void free_report_files(ReportFile* files, int count);

/**
 * Move a file from source to destination
 * @param source Source file path
 * @param destination Destination file path
 * @return SUCCESS on success, FAILURE on error
 */
int move_file(const char* source, const char* destination);

/**
 * Copy a file from source to destination
 * @param source Source file path
 * @param destination Destination file path
 * @return SUCCESS on success, FAILURE on error
 */
int copy_file(const char* source, const char* destination);

/**
 * Check if a file is a valid XML report
 * @param filepath Path to the file to check
 * @return TRUE if valid, FALSE if not
 */
int is_valid_xml_report(const char* filepath);

/**
 * Check if a file was uploaded before the deadline
 * @param filepath Path to the file
 * @return TRUE if on time, FALSE if late
 */
int is_file_uploaded_on_time(const char* filepath);

/**
 * Make an urgent change to a file in the dashboard directory
 * This bypasses the normal permissions to allow emergency updates
 * @param filename Name of the file to update
 * @param content New content for the file
 * @param user_name Name of the user making the change
 * @return SUCCESS on success, FAILURE on error
 */
int make_urgent_change(const char* filename, const char* content, const char* user_name);

#endif /* FILE_OPERATIONS_H */