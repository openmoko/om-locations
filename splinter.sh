#!/bin/sh
echo 1 > /sys/devices/platform/s3c2440-i2c/i2c-adapter/i2c-0/0-0073/neo1973-pm-gps.0/pwron
splinter
echo 0 > /sys/devices/platform/s3c2440-i2c/i2c-adapter/i2c-0/0-0073/neo1973-pm-gps.0/pwron
