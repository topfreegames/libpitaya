cmake_minimum_required(VERSION 3.7)

project(libpitaya)

if (ANDROID)
    set(PITAYA_TARGET "pitaya-android")
elseif (IOS)
    set(PITAYA_TARGET "pitaya-ios")
else()
    set(PITAYA_TARGET "pitaya")
endif()

#
# Options passed as parameters
#
set(PITAYA_LIB_TYPE "STATIC" CACHE STRING "The type of the library to build (STATIC | SHARED | MODULE).")
set(PITAYA_OPENSSL_DIR "${CMAKE_SOURCE_DIR}/openssl-build" CACHE STRING "Where is OpenSSL built.")

#
# Lib UV
#
add_subdirectory(deps/libuv)
include_directories(deps/libuv/include)

#
# Lib Zlib
#
add_subdirectory(deps/zlib)
include_directories(deps/zlib)

#
# OpenSSL
#
include_directories(${PITAYA_OPENSSL_DIR}/include)

add_library(ssl STATIC IMPORTED)
set_property(TARGET ssl PROPERTY IMPORTED_LOCATION ${PITAYA_OPENSSL_DIR}/lib/libssl.a)

add_library(crypto STATIC IMPORTED)
set_property(TARGET crypto PROPERTY IMPORTED_LOCATION ${PITAYA_OPENSSL_DIR}/lib/libcrypto.a)

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

add_library(pitaya ${PITAYA_LIB_TYPE} ${PITAYA_SOURCES} ${PITAYA_HEADERS})

target_link_libraries(${PITAYA_TARGET} PUBLIC zlib uv ssl crypto)

#
# Tests
#
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