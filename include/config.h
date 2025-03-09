// include/config.h
#ifndef CONFIG_H
#define CONFIG_H

// Directory paths
#define UPLOAD_DIR "/var/company/upload"
#define REPORT_DIR "/var/company/reporting"
#define BACKUP_DIR "/var/company/backup"

// Department directories
#define WAREHOUSE_DIR "/var/company/upload/warehouse"
#define MANUFACTURING_DIR "/var/company/upload/manufacturing"
#define SALES_DIR "/var/company/upload/sales"
#define DISTRIBUTION_DIR "/var/company/upload/distribution"

// File naming convention for XML reports
#define XML_PATTERN "%s_%s.xml"  // department_date.xml

// Scheduled time for file transfer (1:00 AM)
#define TRANSFER_HOUR 1
#define TRANSFER_MINUTE 0
#define TRANSFER_SECOND 0

// Maximum log file size (in bytes)
#define MAX_LOG_SIZE 10485760  // 10MB

// Log file location
#define LOG_FILE "/var/log/company_daemon.log"
#define CHANGE_LOG_FILE "/var/log/company_changes.log"

// IPC settings
#define IPC_KEY 12345
#define IPC_PERMS 0666

// Define department names for logging and verification
extern const char* DEPARTMENTS[];
extern const int NUM_DEPARTMENTS;

void load_config(void);

#endif /* CONFIG_H */