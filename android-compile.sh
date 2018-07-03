#!/bin/bash

if [ -z $1 ]; then
  echo "Usage:  android-compile.sh <NDK_ROOT>"
  exit 1
fi

export TOOLCHAIN_DIR=$PWD/android-toolchain

if [ ! -e $TOOLCHAIN_DIR ]; then
  $1/build/tools/make_standalone_toolchain.py \
      --arch arm \
      --install-dir $TOOLCHAIN_DIR \
      --api 15 \
      --deprecated-headers
fi

export TOOLCHAIN_PATH=$TOOLCHAIN_DIR/bin
export TOOL=arm-linux-androideabi
export NDK_TOOLCHAIN_BASENAME=$TOOLCHAIN_PATH/$TOOL

export CC=$NDK_TOOLCHAIN_BASENAME-clang
export CXX=$NDK_TOOLCHAIN_BASENAME-clang++
export LD="$NDK_TOOLCHAIN_BASENAME-ld -march=armv7-a -Wl,--fix-cortex-a8"
export LINK=$LD
export AR=$NDK_TOOLCHAIN_BASENAME-ar
export AS=$NDK_TOOLCHAIN_BASENAME-clang
export NM=$NDK_TOOLCHAIN_BASENAME-nm
export RANLIB=$NDK_TOOLCHAIN_BASENAME-ranlib
export READELF=$NDK_TOOLCHAIN_BASENAME-readelf
export STRIP=$NDK_TOOLCHAIN_BASENAME-strip

make gyp-android

