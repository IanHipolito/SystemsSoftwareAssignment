# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj

# Source files
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

# Target executable
TARGET = company_daemon

# Libraries
LIBS = -lrt -pthread

# Default target
all: directories $(TARGET)

# Create directories
directories:
	mkdir -p $(OBJ_DIR)

# Link object files
$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Clean up
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Install daemon
install: all
	@echo "Installing daemon..."
	@if [ ! -d /var/company ]; then \
		mkdir -p /var/company/upload /var/company/reporting /var/company/backup; \
		mkdir -p /var/company/upload/warehouse /var/company/upload/manufacturing /var/company/upload/sales /var/company/upload/distribution; \
		chmod 755 /var/company/upload /var/company/reporting /var/company/backup; \
		chmod 755 /var/company/upload/warehouse /var/company/upload/manufacturing /var/company/upload/sales /var/company/upload/distribution; \
	fi
	@cp $(TARGET) /usr/local/bin/
	@cp init_script /etc/init.d/company_daemon
	@chmod 755 /etc/init.d/company_daemon
	@update-rc.d company_daemon defaults
	@echo "Daemon installed successfully"

# Uninstall daemon
uninstall:
	@echo "Uninstalling daemon..."
	@if [ -f /etc/init.d/company_daemon ]; then \
		/etc/init.d/company_daemon stop; \
		update-rc.d -f company_daemon remove; \
		rm -f /etc/init.d/company_daemon; \
	fi
	@rm -f /usr/local/bin/$(TARGET)
	@echo "Daemon uninstalled successfully"

.PHONY: all directories clean install uninstall