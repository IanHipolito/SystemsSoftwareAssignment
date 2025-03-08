#!/bin/bash

# Create necessary directories
sudo mkdir -p /var/company/upload/warehouse
sudo mkdir -p /var/company/upload/manufacturing
sudo mkdir -p /var/company/upload/sales
sudo mkdir -p /var/company/upload/distribution
sudo mkdir -p /var/company/reporting
sudo mkdir -p /var/company/backup

# Set appropriate permissions
sudo chmod 777 /var/company/upload
sudo chmod 777 /var/company/upload/warehouse
sudo chmod 777 /var/company/upload/manufacturing
sudo chmod 777 /var/company/upload/sales
sudo chmod 777 /var/company/upload/distribution
sudo chmod 777 /var/company/reporting
sudo chmod 777 /var/company/backup

# Create log files with appropriate permissions
sudo touch /var/log/company_daemon.log
sudo touch /var/log/company_changes.log
sudo chmod 666 /var/log/company_daemon.log
sudo chmod 666 /var/log/company_changes.log

# Ensure PID file location is writable
sudo mkdir -p /var/run
sudo touch /var/run/company_daemon.pid
sudo chmod 666 /var/run/company_daemon.pid

# Create sample XML files for testing
DATE=$(date +%Y-%m-%d)
echo "<?xml version=\"1.0\"?><report><data>Test data</data></report>" | sudo tee /var/company/upload/warehouse/warehouse_${DATE}.xml > /dev/null
echo "<?xml version=\"1.0\"?><report><data>Test data</data></report>" | sudo tee /var/company/upload/manufacturing/manufacturing_${DATE}.xml > /dev/null
echo "<?xml version=\"1.0\"?><report><data>Test data</data></report>" | sudo tee /var/company/upload/sales/sales_${DATE}.xml > /dev/null
echo "<?xml version=\"1.0\"?><report><data>Test data</data></report>" | sudo tee /var/company/upload/distribution/distribution_${DATE}.xml > /dev/null

echo "Test environment set up successfully"
