#include "../include/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

// Department names array
const char* DEPARTMENTS[] = {
    "warehouse",
    "manufacturing",
    "sales",
    "distribution"
};

const int NUM_DEPARTMENTS = 4;

// Function to create directories if they don't exist
static void ensure_directory_exists(const char* path) {
    struct stat st = {0};
    
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) == -1 && errno != EEXIST) {
            fprintf(stderr, "Error creating directory %s: %s\n", 
                    path, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

// Load configuration and ensure required directories exist
void load_config(void) {
    // Create main directories
    ensure_directory_exists(UPLOAD_DIR);
    ensure_directory_exists(REPORT_DIR);
    ensure_directory_exists(BACKUP_DIR);
    
    // Create department directories
    ensure_directory_exists(WAREHOUSE_DIR);
    ensure_directory_exists(MANUFACTURING_DIR);
    ensure_directory_exists(SALES_DIR);
    ensure_directory_exists(DISTRIBUTION_DIR);
}