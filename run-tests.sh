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
    echo "-->  Creating the server binary to use for testing..."
    pushd test/server > /dev/null
    go build -o $SERVER_EXE
    popd > /dev/null
fi

if [[ ! -d $BUILD_DIR ]]; then
    echo "-->  Build directory \'$BUILD_DIR\' does not exist..."
    exit
fi

if [[ ! -f "$BUILD_DIR/Makefile" ]]; then
    echo "-->  Makefile does not exist in the build directory, please create it."
    exit
fi

echo   "-->  Starting server..."
pushd $SERVER_DIR > /dev/null
./$SERVER_EXE &> $SERVER_LOG_FILE &
popd > /dev/null

echo   "-->  Starting mock server..."
pushd $MOCK_SERVER_DIR > /dev/null
node $MOCK_SERVER &> $MOCK_SERVER_LOG_FILE &
popd > /dev/null

sleep 0.5

echo   "-->  Making project..."
pushd $BUILD_DIR > /dev/null
make > /dev/null
popd > /dev/null

pushd $OUTPUT_DIR > /dev/null
echo "-->  Running tests..."
./$TESTS_EXE "${@}"

