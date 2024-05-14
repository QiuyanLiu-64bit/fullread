#!/bin/bash

# Ensure the script received two arguments
if [ "$#" -ne 2 ]; then
  echo "Usage: $0 MIN_IP_LAST_OCTET MAX_IP_LAST_OCTET"
  exit 1
fi

# Read the arguments
min_ip=$1
max_ip=$2

# Base IP address (first three octets)
base_ip="192.168.7."

# Iterate over the specified IP range
for (( i=$min_ip; i<=$max_ip; i++ ))
do
  full_ip="$base_ip$i"
  echo "Transferring files to $full_ip..."
  scp -rp ../script root@$full_ip:/home/ych/ec_test/
done

echo "All files transferred successfully!"
