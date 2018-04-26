#!/bin/bash

SERVER_DIR=test/server
SERVER_EXE=server-exe
SERVER_LOG_FILE=out.log

MOCK_SERVER=mock-server.js
MOCK_SERVER_DIR=test/mock-server
MOCK_SERVER_LOG_FILE=mock-out.log

BUILD_DIR=build
OUTPUT_DIR=$BUILD_DIR/output

TESTS_EXE=tests

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

echo
echo ------------------------------------
echo   "  Starting server..."
echo ------------------------------------
pushd $SERVER_DIR
./$SERVER_EXE &> $SERVER_LOG_FILE &
popd

echo
echo ------------------------------------
echo   "  Starting mock server..."
echo ------------------------------------
pushd $MOCK_SERVER_DIR
node $MOCK_SERVER &> $MOCK_SERVER_LOG_FILE &
popd

sleep 0.5

echo
echo ------------------------------------
echo   "  Making project..."
echo ------------------------------------
pushd $BUILD_DIR
make
popd

pushd $OUTPUT_DIR
echo
echo ------------------------------------
echo "  Running tests..."
echo ------------------------------------
./$TESTS_EXE

