#!/bin/bash

SERVER_EXE=./test/server/server
TEST_EXE=tr_tcp
LOG_FILE=out.log
BUILD_DIR=build
OUTPUT_DIR=$BUILD_DIR/output

# NOTE: This script can in the future be extended to work with 
# different build tools, not only make.

if [[ ! -f $EXECUTABLE ]]; then
    echo Creating the server binary to use for testing...
    pushd test/server
    go build
    popd
fi

if [[ ! -d $BUILD_DIR ]]; then
    echo Build directory \'$BUILD_DIR\' does not exist...
    exit
fi

if [[ ! -f "$BUILD_DIR/Makefile" ]]; then
    echo Makefile does not exist in the build directory, please create it.
    exit
fi

$SERVER_EXE &> $LOG_FILE &
sleep 0.5
pushd $BUILD_DIR
make
popd
pushd $OUTPUT_DIR
./$TEST_EXE
