#!/bin/bash

# cgcreate_read_limit.sh

# Check if a rate limit parameter is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <read_limit_in_MBps>"
    exit 1
fi

read_limit_MBps=$1
read_limit_bytes=$((read_limit_MBps * 1000000))

# Create a generic cgroup, not named after a specific limit
cg_name="read_limit_cgroup"

# Create the cgroup if it doesn't already exist
if [ -d "/sys/fs/cgroup/blkio/$cg_name" ]; then
    echo "Cgroup $cg_name already exists."
else
    echo "Creating cgroup $cg_name..."
    cgcreate -g blkio:$cg_name
    echo "Cgroup $cg_name created successfully."
fi

# Set the read rate limit
cgset -r blkio.throttle.read_bps_device="259:0 $read_limit_bytes" $cg_name
cgset -r blkio.throttle.read_bps_device="259:1 $read_limit_bytes" $cg_name
cgset -r blkio.throttle.read_bps_device="259:2 $read_limit_bytes" $cg_name
cgset -r blkio.throttle.read_bps_device="259:3 $read_limit_bytes" $cg_name
cgset -r blkio.throttle.read_bps_device="259:4 $read_limit_bytes" $cg_name

# Display a warning message
echo "Note: Setting read limit to ${read_limit_MBps} MB/s.Overwritten or Created."

# Show the current settings
cgget -r blkio.throttle.read_bps_device $cg_name
