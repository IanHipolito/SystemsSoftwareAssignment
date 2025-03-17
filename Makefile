# Makefile for Systems Software Assignment 1
# This Makefile compiles the daemon from source, installs the executable
# and init script, and provides targets for service management.
#
# Assignment Requirements Covered:
#   - F1: Build automation, proper directory and dependency handling.
#   - F2/F3: Daemon setup & management via init script.
#   - F4-F8: Although the backup, transfer, locking, IPC, logging, etc.
#           are implemented in your C source, this Makefile ensures that the
#           daemon is correctly built and deployed.
#
# Adjust the paths and filenames as needed to match your project structure.

# Compiler and flags
CC      = gcc
CFLAGS  = -Wall -Wextra -g -O2 -Iinclude

# Directories
SRCDIR  = src
OBJDIR  = obj
BINDIR  = bin

# Source files and corresponding object files
SOURCES   = $(wildcard $(SRCDIR)/*.c)
OBJECTS   = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

# Target executable
TARGET  = $(BINDIR)/company_daemon

# Phony targets
.PHONY: all clean install start stop restart status backup transfer uninstall

# Default target: Build the daemon
all: $(TARGET)

# Compile each .c file to .o, ensuring the obj directory exists
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files to create the final executable, ensuring the bin directory exists
$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# Create obj directory if it does not exist
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Create bin directory if it does not exist
$(BINDIR):
	mkdir -p $(BINDIR)

# Clean up build artifacts
clean:
	rm -rf $(OBJDIR) $(BINDIR)

# Install target: builds the executable, then copies it and the init script to system directories
install: all
	@echo "Installing company_daemon..."
	# Create necessary directories if they don't exist
	mkdir -p /var/company/upload/warehouse
	mkdir -p /var/company/upload/manufacturing
	mkdir -p /var/company/upload/sales
	mkdir -p /var/company/upload/distribution
	mkdir -p /var/company/reporting
	mkdir -p /var/company/backup
	mkdir -p /var/log
	
	# Set appropriate permissions
	chmod 777 /var/company/upload
	chmod 777 /var/company/upload/warehouse
	chmod 777 /var/company/upload/manufacturing
	chmod 777 /var/company/upload/sales
	chmod 777 /var/company/upload/distribution
	chmod 777 /var/company/reporting
	chmod 777 /var/company/backup
	
	# Copy binary to /usr/sbin
	sudo cp $(BINDIR)/company_daemon /usr/sbin/
	# Install the init script from init.d folder
	if [ -f init.d/report_daemon ]; then \
		sudo cp init.d/report_daemon /etc/init.d/company_daemon; \
		sudo chmod +x /etc/init.d/company_daemon; \
		sudo update-rc.d company_daemon defaults; \
		echo "Init script installed to /etc/init.d/company_daemon"; \
	else \
		echo "Warning: init script not found at init.d/report_daemon. Please create one."; \
	fi

# Start the daemon using the init script
start:
	@echo "Starting company daemon..."
	sudo /etc/init.d/company_daemon start

# Stop the daemon using the init script
stop:
	@echo "Stopping company daemon..."
	sudo /etc/init.d/company_daemon stop

# Restart the daemon using the init script
restart:
	@echo "Restarting company daemon..."
	sudo /etc/init.d/company_daemon restart

# Check the status of the daemon
status:
	@echo "Checking company daemon status..."
	sudo /etc/init.d/company_daemon status

# Force a backup operation
backup:
	@echo "Forcing a backup operation..."
	sudo /etc/init.d/company_daemon backup

# Force a transfer operation
transfer:
	@echo "Forcing a transfer operation..."
	sudo /etc/init.d/company_daemon transfer

# Uninstall the daemon and init script from the system directories
uninstall:
	@echo "Uninstalling company daemon..."
	sudo /etc/init.d/company_daemon stop || true
	sudo update-rc.d -f company_daemon remove || true
	sudo rm -f /usr/sbin/company_daemon
	sudo rm -f /etc/init.d/company_daemon