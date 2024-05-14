#!/bin/bash

PARENT_DIR=$(dirname "$(pwd)")
BUILD_DIR=$PARENT_DIR/build/

echo "Format: ./valgrind.sh client -r 2MB_src 2MB_dst"
VALGRIND_CMD="valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --log-file=valgrind-out.txt $BUILD_DIR/$1 $2 $3 $4"
$VALGRIND_CMD
