#!/bin/bash

SERVER_DIR=test/server
SERVER_EXE=server-exe
LOG_FILE=out.log
BUILD_DIR=build
OUTPUT_DIR=$BUILD_DIR/output

TCP_TEST_EXE=tr_tcp
TLS_TEST_EXE=tr_tls

# NOTE: This script can in the future be extended to work with 
# different build tools, not only make.

if [[ ! -f $EXECUTABLE ]]; then
    echo Creating the server binary to use for testing...
    pushd test/server
    go build -o $SERVER_EXE
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

pushd $SERVER_DIR
./$SERVER_EXE &> $LOG_FILE &
popd

sleep 0.5

pushd $BUILD_DIR
make
popd

pushd $OUTPUT_DIR
echo
echo ------------------------------
echo "  Running tcp tests..."
echo ------------------------------
./$TCP_TEST_EXE

echo
echo ------------------------------
echo "  Running tls tests..."
echo ------------------------------
./$TLS_TEST_EXE
