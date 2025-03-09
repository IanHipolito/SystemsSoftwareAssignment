# Makefile for Report Management System

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -O2
LDFLAGS = 

# Directories
SRC_DIR = src
INCLUDE_DIR = include
OBJ_DIR = obj
BIN_DIR = bin

# Create directory command
MKDIR = mkdir -p

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
HEADERS = $(wildcard $(INCLUDE_DIR)/*.h)

# Binary
TARGET = $(BIN_DIR)/report_daemon

# Include directories
CFLAGS += -I$(INCLUDE_DIR)

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	$(MKDIR) $(OBJ_DIR)
	$(MKDIR) $(BIN_DIR)

# Link the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Install the daemon and create necessary directories
install: $(TARGET)
	@echo "Installing report daemon..."
	# Create directories if they don't exist
	mkdir -p /var/report_system/upload
	mkdir -p /var/report_system/dashboard
	mkdir -p /var/report_system/backup
	mkdir -p /var/report_system/logs
	# Set appropriate permissions
	chmod 777 /var/report_system/upload
	chmod 755 /var/report_system/dashboard
	chmod 755 /var/report_system/backup
	chmod 755 /var/report_system/logs
	# Copy the daemon to system location
	cp $(TARGET) /usr/sbin/report_daemon
	# Create init script directory if it doesn't exist
	mkdir -p /etc/init.d
	# Generate init script if it doesn't exist
	test -f /etc/init.d/report_daemon || $(MAKE) init-script
	# Copy init script
	cp init.d/report_daemon /etc/init.d/
	chmod 755 /etc/init.d/report_daemon
	# Update system (for Debian/Ubuntu)
	which update-rc.d >/dev/null && update-rc.d report_daemon defaults || true

# Uninstall the daemon
uninstall:
	@echo "Uninstalling report daemon..."
	# Stop the daemon if running
	test -f /etc/init.d/report_daemon && /etc/init.d/report_daemon stop || true
	# Remove init script
	which update-rc.d >/dev/null && update-rc.d report_daemon remove || true
	rm -f /etc/init.d/report_daemon
	# Remove binary
	rm -f /usr/sbin/report_daemon
	# Note: We don't remove the data directories

# Start the daemon
start:
	@echo "Starting report daemon..."
	/etc/init.d/report_daemon start

# Stop the daemon
stop:
	@echo "Stopping report daemon..."
	/etc/init.d/report_daemon stop

# Restart the daemon
restart:
	@echo "Restarting report daemon..."
	/etc/init.d/report_daemon restart

# Create init.d script
init-script:
	@echo "Creating init.d script..."
	mkdir -p init.d
	@echo '#!/bin/sh' > init.d/report_daemon
	@echo '### BEGIN INIT INFO' >> init.d/report_daemon
	@echo '# Provides:          report_daemon' >> init.d/report_daemon
	@echo '# Required-Start:    $remote_fs $syslog' >> init.d/report_daemon
	@echo '# Required-Stop:     $remote_fs $syslog' >> init.d/report_daemon
	@echo '# Default-Start:     2 3 4 5' >> init.d/report_daemon
	@echo '# Default-Stop:      0 1 6' >> init.d/report_daemon
	@echo '# Short-Description: Report management daemon' >> init.d/report_daemon
	@echo '# Description:       Daemon to manage department reports' >> init.d/report_daemon
	@echo '### END INIT INFO' >> init.d/report_daemon
	@echo '' >> init.d/report_daemon
	@echo 'DAEMON=/usr/sbin/report_daemon' >> init.d/report_daemon
	@echo 'NAME=report_daemon' >> init.d/report_daemon
	@echo 'DESC="Report Management Daemon"' >> init.d/report_daemon
	@echo 'PIDFILE=/var/run/report_daemon.pid' >> init.d/report_daemon
	@echo '' >> init.d/report_daemon
	@echo 'case "$$1" in' >> init.d/report_daemon
	@echo '  start)' >> init.d/report_daemon
	@echo '    echo "Starting $$DESC"' >> init.d/report_daemon
	@echo '    start-stop-daemon --start --quiet --exec $$DAEMON' >> init.d/report_daemon
	@echo '    ;;' >> init.d/report_daemon
	@echo '  stop)' >> init.d/report_daemon
	@echo '    echo "Stopping $$DESC"' >> init.d/report_daemon
	@echo '    start-stop-daemon --stop --quiet --pidfile $$PIDFILE' >> init.d/report_daemon
	@echo '    ;;' >> init.d/report_daemon
	@echo '  restart|reload|force-reload)' >> init.d/report_daemon
	@echo '    echo "Restarting $$DESC"' >> init.d/report_daemon
	@echo '    start-stop-daemon --stop --quiet --pidfile $$PIDFILE' >> init.d/report_daemon
	@echo '    sleep 1' >> init.d/report_daemon
	@echo '    start-stop-daemon --start --quiet --exec $$DAEMON' >> init.d/report_daemon
	@echo '    ;;' >> init.d/report_daemon
	@echo '  status)' >> init.d/report_daemon
	@echo '    if [ -f $$PIDFILE ]; then' >> init.d/report_daemon
	@echo '      pid=$$(cat $$PIDFILE)' >> init.d/report_daemon
	@echo '      if ps -p $$pid > /dev/null 2>&1; then' >> init.d/report_daemon
	@echo '        echo "$$DESC is running (pid $$pid)"' >> init.d/report_daemon
	@echo '        exit 0' >> init.d/report_daemon
	@echo '      else' >> init.d/report_daemon
	@echo '        echo "$$DESC is not running (stale pidfile)"' >> init.d/report_daemon
	@echo '        exit 1' >> init.d/report_daemon
	@echo '      fi' >> init.d/report_daemon
	@echo '    else' >> init.d/report_daemon
	@echo '      echo "$$DESC is not running"' >> init.d/report_daemon
	@echo '      exit 3' >> init.d/report_daemon
	@echo '    fi' >> init.d/report_daemon
	@echo '    ;;' >> init.d/report_daemon
	@echo '  *)' >> init.d/report_daemon
	@echo '    echo "Usage: /etc/init.d/$$NAME {start|stop|restart|reload|force-reload|status}"' >> init.d/report_daemon
	@echo '    exit 1' >> init.d/report_daemon
	@echo '    ;;' >> init.d/report_daemon
	@echo 'esac' >> init.d/report_daemon
	@echo '' >> init.d/report_daemon
	@echo 'exit 0' >> init.d/report_daemon
	chmod 755 init.d/report_daemon

# Print directory structure (for debugging)
print-structure:
	@echo "Source files: $(SRCS)"
	@echo "Object files: $(OBJS)"
	@echo "Headers: $(HEADERS)"

.PHONY: all directories install uninstall start stop restart clean init-script print-structure