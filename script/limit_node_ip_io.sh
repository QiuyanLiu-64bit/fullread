#!/bin/sh

# Check if the correct number of arguments is provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <IP_address> <read_limit_in_MBps>"
    exit 1
fi

# Assign the provided arguments to variables
IP_ADDRESS=192.168.7.$1    # The target IP address
READ_LIMIT=$2    # The read speed limit in MB/s

# Get the current directory
CURRENT_DIR="/home/ych/ec_test/script"

echo "Setting read limit for node at $IP_ADDRESS..."

# Execute the script on the remote host to set the read speed limit
# It assumes that create_node_read_limitation.sh is present in the same directory on the remote host
ssh $IP_ADDRESS "$CURRENT_DIR/create_node_io_limitation.sh $READ_LIMIT"

echo "Read limit set for node at $IP_ADDRESS."
