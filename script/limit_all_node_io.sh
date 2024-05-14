#!/bin/sh

# Check if the correct number of arguments is provided
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <min_ip_last_segment> <max_ip_last_segment> <read_limit_in_MBps>"
    exit 1
fi

MIN_IP=$1
MAX_IP=$2
READ_LIMIT=$3
IP_PREFIX=192.168.7.

# Iterate over the IP range and call limit_node_ip_io.sh for each IP
for i in $(seq $MIN_IP $MAX_IP)
do
    # echo "Setting read limit for IP $IP_PREFIX$i..."
    ./limit_node_ip_io.sh $i $READ_LIMIT
    echo "Read limit set for IP $IP_PREFIX$i."
done

echo "Read limit set for all nodes."
