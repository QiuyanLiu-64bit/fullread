#!/bin/bash

# Check for the required number of arguments
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 start_ip end_ip"
    exit 1
fi

# Assign command line arguments to variables
start=$1
end=$2

# gcc /home/ych/ec_test/test_file/rspeed.c -o /home/ych/ec_test/test_file/rspeed

# Initialize an array to hold the PIDs of the background processes
pids=()

# Loop through the IP addresses
for i in $(seq $start $end); do
    ip="192.168.7.$i"
    echo "Starting mkfs on $ip ..."

    # Execute the mkfs command on the remote host via SSH in the background
    ssh root@$ip "mkfs.ext4 /dev/nvme1n1" &

    # Store the PID of the background process
    pids+=($!)

done

# Wait for all mkfs commands to complete
for pid in ${pids[@]}; do
    wait $pid
    if [ $? -eq 0 ]; then
        echo "mkfs operation completed for PID $pid"
    else
        echo "mkfs operation failed for PID $pid"
    fi
done

# Continue with the rest of the script
for i in $(seq $start $end); do
    ip="192.168.7.$i"
    echo "Processing $ip ..."

    # Execute the remaining commands sequentially or parallel as needed
    ssh root@$ip "mount /dev/nvme1n1 /home/ych/"
    # ssh root@$ip "dd if=/dev/nvme1n1 of=/home/ych/test_file.dat bs=16M count=4"
    # scp /home/ych/ec_test/test_file/rspeed root@$ip:/home/ych/
    # ssh root@$ip "cgexec -g blkio:read_limit /home/ych/rspeed"

    # Check if the command was successful
    if [ $? -eq 0 ]; then
        echo "$ip: Operation successful"
    else
        echo "$ip: Operation failed"
    fi
done
