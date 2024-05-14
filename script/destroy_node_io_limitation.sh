#!/bin/bash

# cgdelete_read_limit.sh

# Define the cgroup name
cg_name="read_limit_cgroup"

# Check if the cgroup exists
if [ -d "/sys/fs/cgroup/blkio/$cg_name" ]; then
    # Delete the cgroup
    cgdelete -g blkio:$cg_name
    echo "Cgroup '$cg_name' has been deleted."
else
    echo "Cgroup '$cg_name' does not exist."
fi
