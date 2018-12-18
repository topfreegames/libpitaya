cmake_minimum_required(VERSION 3.7)

if(WIN32)
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/conan_paths.cmake)
        message(FATAL_ERROR "Run conan install before running CMake")
    endif()

    include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
endif()

project(pitaya LANGUAGES C)

#
# Options passed as parameters
#
set(PITAYA_OPENSSL_DIR "${CMAKE_SOURCE_DIR}/deps/openssl" CACHE STRING "Where is OpenSSL built.")
option(BUILD_MACOS_BUNDLE "If it should build a .bundle library" OFF)

set(pitaya_sources
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

set(pitaya_headers
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
    include/pitaya_trans.h
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

if(BUILD_MACOS_BUNDLE)
    add_library(pitaya MODULE ${pitaya_sources} ${pitaya_headers})
    set_target_properties(pitaya PROPERTIES SUFFIX ".bundle")
else()
    add_library(pitaya ${pitaya_sources} ${pitaya_headers})
endif()

if(ANDROID)
    set_target_properties(pitaya PROPERTIES OUTPUT_NAME "pitaya-android")
elseif(IOS)
    set_target_properties(pitaya PROPERTIES OUTPUT_NAME "pitaya-ios")
elseif(APPLE)
    set_target_properties(pitaya PROPERTIES OUTPUT_NAME "pitaya-mac")
elseif(WIN32)
    set_target_properties(pitaya PROPERTIES OUTPUT_NAME "pitaya-windows")
else()
    set_target_properties(pitaya PROPERTIES OUTPUT_NAME "pitaya-linux")
endif()

if(UNIX AND NOT APPLE)
    set_target_properties(pitaya PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

#
# Lib UV
#
set(BUILD_TESTING OFF CACHE BOOL "")
add_subdirectory(deps/libuv)

#
# Lib Zlib
#
if(ANDROID)
    find_package(ZLIB REQUIRED)
    target_include_directories(pitaya PRIVATE ${ZLIB_INCLUDE_DIRS})
elseif(WIN32)
    find_package(ZLIB REQUIRED)
    target_link_libraries(pitaya PRIVATE ZLIB::ZLIB)
else()
    target_include_directories(pitaya PRIVATE PRIVATE deps/zlib ${CMAKE_BINARY_DIR}/deps/zlib)
    add_subdirectory(deps/zlib)
endif()

#
# OpenSSL
#
if(WIN32)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(pitaya PRIVATE OpenSSL::SSL OpenSSL::Crypto)
else()
    if(ANDROID)
        target_include_directories(pitaya PRIVATE ${PITAYA_OPENSSL_DIR}/android/include)
        set(SSL_LOCATION ${PITAYA_OPENSSL_DIR}/android/lib/armeabi-v7a/libssl.a)
        set(CRYPTO_LOCATION ${PITAYA_OPENSSL_DIR}/android/lib/armeabi-v7a/libcrypto.a)
    elseif(IOS)
        target_include_directories(pitaya PRIVATE ${PITAYA_OPENSSL_DIR}/ios/include)
        set(SSL_LOCATION ${PITAYA_OPENSSL_DIR}/ios/lib/libssl.a)
        set(CRYPTO_LOCATION ${PITAYA_OPENSSL_DIR}/ios/lib/libcrypto.a)
    elseif(APPLE)
        target_include_directories(pitaya PRIVATE ${PITAYA_OPENSSL_DIR}/mac/include)
        set(SSL_LOCATION ${PITAYA_OPENSSL_DIR}/mac/lib/libssl.a)
        set(CRYPTO_LOCATION ${PITAYA_OPENSSL_DIR}/mac/lib/libcrypto.a)
    else() # Linux
	    target_include_directories(pitaya PRIVATE ${PITAYA_OPENSSL_DIR}/linux/include)
	    set(SSL_LOCATION ${PITAYA_OPENSSL_DIR}/linux/lib/libssl.a)
	    set(CRYPTO_LOCATION ${PITAYA_OPENSSL_DIR}/linux/lib/libcrypto.a)
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
target_include_directories(pitaya
  PUBLIC
    include
  PRIVATE
    src)

target_compile_options(pitaya
  PRIVATE $<$<CONFIG:Debug>:-g>)

target_compile_definitions(pitaya
  PRIVATE
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:RelWithDebugInfo>:NDEBUG>)

if(WIN32)
    target_link_libraries(pitaya
      PRIVATE uv_a)
elseif(ANDROID)
    find_package(Threads REQUIRED)
    target_link_libraries(pitaya
      PRIVATE
        Threads::Threads
        ${ZLIB_LIBRARIES}
        uv_a
        ssl
        crypto
        ${ANDROID_LOG_LIB})
else()
    target_link_libraries(pitaya
      PRIVATE
        zlib
        uv_a
        ssl
        crypto)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(pitaya PRIVATE m dl)
endif()

#
# Tests
#
if(NOT IOS AND NOT ANDROID AND NOT BUILD_MACOS_BUNDLE)
    find_package(Threads REQUIRED)

    add_executable(pitaya_tests
        # Sources
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
        test/session-data.pb.c

        # Headers
        test/test_common.h

        # utils
        test/flag.h
        test/flag.c

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

    set_target_properties(pitaya_tests PROPERTIES OUTPUT_NAME "tests")
    target_include_directories(pitaya_tests
        PUBLIC
          src
          deps/munit
          deps/nanopb-0.3.9.1 test)
    target_link_libraries(pitaya_tests PUBLIC pitaya Threads::Threads)

    add_custom_command(
      TARGET
        pitaya_tests POST_BUILD
      COMMAND
        ${CMAKE_COMMAND} -E copy_directory
                            ${CMAKE_SOURCE_DIR}/fixtures
                            $<TARGET_FILE_DIR:pitaya_tests>/fixtures)
endif()

add_custom_target(create_zip ALL COMMAND
    ${CMAKE_COMMAND} -E tar "cfv" "${CMAKE_BINARY_DIR}/include.zip" --format=zip
       "${CMAKE_SOURCE_DIR}/include")
