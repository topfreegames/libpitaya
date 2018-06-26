#!/bin/bash

SERVER_DIR=pitaya-servers
SERVER_EXE=server-exe
SERVER_LOG_FILE=out.log

MOCK_SERVER_DIR=test/mock-servers

MOCK_DISCONNECT_SERVER=mock-disconnect-server.js
MOCK_DISCONNECT_SERVER_LOG_FILE=mock-disconnect-server-out.log

MOCK_COMPRESSION_SERVER=mock-compression-server.js
MOCK_COMPRESSION_SERVER_LOG_FILE=mock-compression-server-out.log

MOCK_KICK_SERVER=mock-kick-server.js
MOCK_KICK_SERVER_LOG_FILE=mock-kick-server-out.log

MOCK_TIMEOUT_SERVER=mock-timeout-server.js
MOCK_TIMEOUT_SERVER_LOG_FILE=mock-timeout-server-out.log

MOCK_DESTROY_SOCKET_SERVER=mock-destroy-socket-server.js
MOCK_DESTROY_SOCKET_SERVER_LOG_FILE=mock-destroy-socket-server-out.log

BUILD_DIR=build
OUTPUT_DIR=$BUILD_DIR/out/Release_x64/output

TESTS_EXE=tests

# NOTE: This script can in the future be extended to work with 
# different build tools, not only make.

if [[ ! -f "$SERVER_DIR/json-server/$SERVER_EXE" ]]; then
    echo "-->  Creating the server binary to use for testing..."
    pushd "$SERVER_DIR/json-server" > /dev/null
    go build -o $SERVER_EXE
    popd > /dev/null
fi

if [[ ! -d $BUILD_DIR ]]; then
    echo "-->  Build directory \'$BUILD_DIR\' does not exist..."
    exit
fi

if [[ ! -f "$BUILD_DIR/out/Release_x64/build.ninja" ]]; then
    echo "-->  build.ninja does not exist in the build directory, please create it."
    exit
fi

echo   "-->  Starting server..."
pushd $SERVER_DIR/json-server > /dev/null
./$SERVER_EXE &> $SERVER_LOG_FILE &
popd > /dev/null

echo   "-->  Starting mock servers..."
pushd $MOCK_SERVER_DIR > /dev/null
node $MOCK_DISCONNECT_SERVER &> $MOCK_DISCONNECT_SERVER_LOG_FILE &
node $MOCK_COMPRESSION_SERVER &> $MOCK_COMPRESSION_SERVER_LOG_FILE &
node $MOCK_KICK_SERVER &> $MOCK_KICK_SERVER_LOG_FILE &
node $MOCK_TIMEOUT_SERVER &> $MOCK_TIMEOUT_SERVER_LOG_FILE &
node $MOCK_DESTROY_SOCKET_SERVER &> $MOCK_DESTROY_SOCKET_SERVER_LOG_FILE &
popd > /dev/null

sleep 0.5

echo   "-->  Making project..."
pushd $BUILD_DIR/out/Release_x64 > /dev/null
ninja > /dev/null
popd > /dev/null

pushd $OUTPUT_DIR > /dev/null
echo "-->  Running tests..."
./$TESTS_EXE "${@}"

