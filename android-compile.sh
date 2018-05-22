#!/bin/bash

if [ -z $1 ]; then
  echo "Usage:  android-compile.sh <NDK_ROOT>"
  exit 1
fi

export ANDROID_TOOLCHAIN_DIR=$PWD/android-toolchain

if [ ! -e $ANDROID_TOOLCHAIN_DIR ]; then
  $1/build/tools/make_standalone_toolchain.py \
      --arch arm \
      --install-dir $ANDROID_TOOLCHAIN_DIR \
      --api 14
fi

target_host=arm-linux-androideabi

export AR=$ANDROID_TOOLCHAIN_DIR/bin/$target_host-ar
export AS=$ANDROID_TOOLCHAIN_DIR/bin/$target_host-clang
export CC=$ANDROID_TOOLCHAIN_DIR/bin/$target_host-clang
export CXX=$ANDROID_TOOLCHAIN_DIR/bin/$target_host-clang++
export LD=$ANDROID_TOOLCHAIN_DIR/bin/$target_host-ld
export STRIP=$ANDROID_TOOLCHAIN_DIR/bin/$target_host-strip
export LINK=$ANDROID_TOOLCHAIN_DIR/bin/$target_host-ld

make gyp-android

