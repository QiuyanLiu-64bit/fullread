#!/bin/sh

# Check if the correct number of arguments is provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <min_ip_last_segment> <max_ip_last_segment>"
    exit 1
fi

MIN_IP=$1
MAX_IP=$2
IP_PREFIX=192.168.7.

CURRENT_DIR="/home/ych/ec_test/script"

# Iterate over the IP range and call limit_node_ip_io.sh for each IP
for i in $(seq $MIN_IP $MAX_IP)
do
    # echo "Setting read limit for IP $IP_PREFIX$i..."
    ssh root@$IP_PREFIX$i "$CURRENT_DIR/destroy_node_io_limitation.sh"
    echo "Read limit unset for IP $IP_PREFIX$i."
done

echo "Read limit unset for all nodes."
