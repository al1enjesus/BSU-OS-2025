#!/bin/bash
echo "=== Creating screenshots and logs ==="

# Сборка
echo "Building modules..."
make > logs/build.log 2>&1

# Hello module
echo "Testing hello_module..."
sudo dmesg -C
sudo insmod src/hello_module.ko
dmesg | tail -5 > screenshots/hello_module/load1.txt
sudo rmmod hello_module
dmesg | tail -5 > screenshots/hello_module/unload1.txt
sudo insmod src/hello_module.ko message="Custom message"
dmesg | tail -5 > screenshots/hello_module/load2.txt
sudo rmmod hello_module
dmesg > logs/hello_module/full.log

# Proc module  
echo "Testing proc_module..."
sudo dmesg -C
sudo insmod src/proc_module.ko
dmesg | tail -5 > screenshots/proc_module/load.txt
cat /proc/student_info > screenshots/proc_module/read1.txt
cat /proc/student_info > screenshots/proc_module/read2.txt
sudo rmmod proc_module
dmesg | tail -10 > screenshots/proc_module/unload.txt
dmesg > logs/proc_module/full.log

# Char device
echo "Testing char_device..."
sudo dmesg -C
sudo insmod src/char_device.ko
dmesg | grep "major" > screenshots/char_device/load.txt
MAJOR=$(dmesg | grep "major" | tail -1 | awk '{print $NF}')
sudo mknod /dev/mychardev c $MAJOR 0
sudo chmod 666 /dev/mychardev
echo "Test data" > /dev/mychardev
dmesg | tail -5 > screenshots/char_device/write.txt
cat /dev/mychardev > screenshots/char_device/read.txt
sudo rmmod char_device
sudo rm /dev/mychardev
dmesg > logs/char_device/full.log

echo "=== Done! Check screenshots/ and logs/ folders ==="
