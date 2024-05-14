#!/bin/sh

echo "Starting storagenodes..."

# Define the parent and related directories
PARENT_DIR=$(dirname "$1")
LOG_DIR=$PARENT_DIR/log
BUILD_DIR=$PARENT_DIR/build

# Name of the cgroup to check
cg_name="read_limit_cgroup"

# Check if the cgroup exists and has read limits set
if [ -d "/sys/fs/cgroup/blkio/$cg_name" ]; then
    echo "Cgroup $cg_name exists. Current limits:"
    # Display current read limits
    cgget -r blkio.throttle.read_bps_device $cg_name
    
    # Run the program under the specified cgroup limits
    echo "Running storagenodes under cgroup limits..."
    nohup cgexec -g blkio:$cg_name $BUILD_DIR/storagenodes > $LOG_DIR/log.txt 2>&1 &
else
    echo "Cgroup $cg_name does not exist or has no read limits. Running without cgroup limits..."
    # Run the program without cgroup limits
    nohup $BUILD_DIR/storagenodes > $LOG_DIR/log.txt 2>&1 &
fi
