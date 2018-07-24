cmake_minimum_required(VERSION 3.7)

project(libpitaya)

# Set build to Debug if none is specified
if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

if(ANDROID)
    set(PITAYA_TARGET "pitaya-android")
elseif(IOS)
    set(PITAYA_TARGET "pitaya-ios")
elseif(APPLE)
    set(PITAYA_TARGET "pitaya-mac")
elseif(WIN32)
    set(PITAYA_TARGET "pitaya-windows")
else()
    set(PITAYA_TARGET "pitaya-linux")
endif()

#
# Options passed as parameters
#
set(PITAYA_LIB_TYPE "STATIC" CACHE STRING "The type of the library to build (STATIC | SHARED | MODULE).")
set(PITAYA_OPENSSL_DIR "${CMAKE_SOURCE_DIR}/deps/openssl" CACHE STRING "Where is OpenSSL built.")

#
# Lib UV
#
set(BUILD_TESTING OFF CACHE BOOL "")
add_subdirectory(deps/libuv)
include_directories(deps/libuv/include)

#
# Lib Zlib
#
if(ANDROID)
    find_package(ZLIB REQUIRED)
    include_directories(${ZLIB_INCLUDE_DIRS})
else()
    add_subdirectory(deps/zlib)
    include_directories(deps/zlib)
endif()

#
# OpenSSL
#
if(WIN32)
    message("Adding OpenSSL for Windows")
    find_package(OpenSSL REQUIRED)
    include_directories(${OPENSSL_INCLUDE_DIR})
else()
    if(ANDROID)
        message("Adding OpenSSL for Android")
        include_directories(${PITAYA_OPENSSL_DIR}/android/include)
        set(SSL_LOCATION ${PITAYA_OPENSSL_DIR}/android/lib/armeabi-v7a/libssl.a)
        set(CRYPTO_LOCATION ${PITAYA_OPENSSL_DIR}/android/lib/armeabi-v7a/libcrypto.a)
    elseif(IOS)
        message("Adding OpenSSL for iOS")
        include_directories(${PITAYA_OPENSSL_DIR}/ios/include)
        set(SSL_LOCATION ${PITAYA_OPENSSL_DIR}/ios/lib/libssl.a)
        set(CRYPTO_LOCATION ${PITAYA_OPENSSL_DIR}/ios/lib/libcrypto.a)
    elseif(APPLE)
        message("Adding OpenSSL for MacOSX")
        include_directories(${PITAYA_OPENSSL_DIR}/mac/include)
        set(SSL_LOCATION ${PITAYA_OPENSSL_DIR}/mac/lib/libssl.a)
        set(CRYPTO_LOCATION ${PITAYA_OPENSSL_DIR}/mac/lib/libcrypto.a)
    else() # Linux
        message("Adding OpenSSL for Linux")
        include_directories(${CMAKE_SOURCE_DIR}/openssl-build/include)
        set(SSL_LOCATION ${CMAKE_SOURCE_DIR}/openssl-build/lib/libssl.a)
        set(CRYPTO_LOCATION ${CMAKE_SOURCE_DIR}/openssl-build/lib/libcrypto.a)
    endif()

    add_library(ssl STATIC IMPORTED)
    set_property(TARGET ssl PROPERTY IMPORTED_LOCATION ${SSL_LOCATION})

    add_library(crypto STATIC IMPORTED)
    set_property(TARGET crypto PROPERTY IMPORTED_LOCATION ${CRYPTO_LOCATION})
endif()

#
# Android Log
#
if(ANDROID)
    find_library(ANDROID_LOG_LIB log)
endif()

#
# Pitaya
# TODO: Consider removing references to `dummy`
#
include_directories(include src)

set(PITAYA_SOURCES
    src/pc_assert.c
    src/pc_JSON.c
    src/pc_lib.c
    src/pc_pitaya.c
    src/pc_trans_repo.c
    src/pc_trans.c
    src/pc_unity.c
    src/tr/uv/pr_gzip.c
    src/tr/uv/pr_msg_json.c
    src/tr/uv/pr_msg.c
    src/tr/uv/pr_pkg.c
    src/tr/uv/tr_uv_tcp_aux.c
    src/tr/uv/tr_uv_tcp_i.c
    src/tr/uv/tr_uv_tcp.c
    src/tr/uv/tr_uv_tls_aux.c
    src/tr/uv/tr_uv_tls_i.c
    src/tr/uv/tr_uv_tls.c
    src/tr/dummy/tr_dummy.c)

set(PITAYA_HEADERS
    src/pc_error.h
    src/pc_JSON.h
    src/pc_lib.h
    src/pc_mutex.h
    src/pc_pitaya_i.h
    src/pc_trans_repo.h
    src/queue.h
    include/pc_assert.h
    include/pitaya_unity.h
    include/pitaya.h
    include/pomelo_trans.h
    src/tr/uv/pr_gzip.h
    src/tr/uv/pr_msg.h
    src/tr/uv/pr_pkg.h
    src/tr/uv/tr_uv_tcp_aux.h
    src/tr/uv/tr_uv_tcp_i.h
    src/tr/uv/tr_uv_tcp.h
    src/tr/uv/tr_uv_tls_aux.h
    src/tr/uv/tr_uv_tls_i.h
    src/tr/uv/tr_uv_tls.h
    src/tr/dummy/tr_dummy.h)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
elseif (CMAKE_BUILD_TYPE STREQUAL Release)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    add_definitions(-DNDEBUG)
else()
    message(FATAL_ERROR "Unrecognized build type specified.")
endif()

add_library(${PITAYA_TARGET} ${PITAYA_LIB_TYPE} ${PITAYA_SOURCES} ${PITAYA_HEADERS})

if (PITAYA_LIB_TYPE STREQUAL MODULE)
    set_target_properties(${PITAYA_TARGET} PROPERTIES SUFFIX ".bundle")
endif()

if(WIN32)
    target_link_libraries(${PITAYA_TARGET} PUBLIC zlib uv_a OpenSSL::SSL OpenSSL::Crypto)
elseif(ANDROID)
    find_package(Threads REQUIRED)
    target_link_libraries(${PITAYA_TARGET} PRIVATE Threads::Threads ${ZLIB_LIBRARIES} uv_a ssl crypto ${ANDROID_LOG_LIB})
else()
    target_link_libraries(${PITAYA_TARGET} PRIVATE zlib uv_a ssl crypto)
endif()

#
# Tests
#
if(NOT IOS AND NOT ANDROID AND NOT PITAYA_LIB_TYPE STREQUAL "MODULE")
    set(TEST_SOURCES
        test/main.c
        test/test_compression.c
        test/test_kick.c
        test/test_notify.c
        test/test_pc_client.c
        test/test_protobuf.c
        test/test_push.c
        test/test_reconnection.c
        test/test_request.c
        test/test_session.c
        test/test_stress.c
        test/test-tr_tcp.c
        test/test-tr_tls.c
        # munit
        deps/munit/munit.c
        # nanopb
        deps/nanopb-0.3.9.1/pb_common.c
        deps/nanopb-0.3.9.1/pb_decode.c
        deps/nanopb-0.3.9.1/pb_encode.c
        # nanopb
        # Protobuf generated
        test/big-message.pb.c
        test/error.pb.c
        test/response.pb.c
        test/session-data.pb.c)

    set(TEST_HEADERS
        test/test_common.h
        # munit
        deps/munit/munit.h
        # nanopb
        deps/nanopb-0.3.9.1/pb_common.h
        deps/nanopb-0.3.9.1/pb_decode.h
        deps/nanopb-0.3.9.1/pb_encode.h
        deps/nanopb-0.3.9.1/pb.h
        # Protobuf generated
        test/big-message.pb.h
        test/error.pb.h
        test/response.pb.h
        test/session-data.pb.h)

    add_executable(tests ${TEST_HEADERS} ${TEST_SOURCES})
    target_include_directories(tests PUBLIC deps/munit deps/nanopb-0.3.9.1 test)
    target_link_libraries(tests PUBLIC ${PITAYA_TARGET})
endif()