#!/bin/sh

cd /home/ych/ec_test/
cmake . 
make
cd /home/ych/ec_test/script
./scp_storagenodes.sh $1 $2