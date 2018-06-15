#!/bin/bash

if [ -z $1 ]; then
  echo "Usage:  android-compile.sh <NDK_ROOT>"
  exit 1
fi

# ./build-openssl.py --os Android --prefix `pwd`/build/openssl --openssl-dir `pwd`/deps/openssl --ndk-dir $1

export ANDROID_TOOLCHAIN_DIR=$PWD/android-toolchain

if [ ! -e $ANDROID_TOOLCHAIN_DIR ]; then
  $1/build/tools/make_standalone_toolchain.py \
      --arch arm \
      --install-dir $ANDROID_TOOLCHAIN_DIR \
      --api 15
fi

target_host=arm-linux-androideabi

export AR=$ANDROID_TOOLCHAIN_DIR/bin/$target_host-ar
export AS=$ANDROID_TOOLCHAIN_DIR/bin/$target_host-clang
export CC=$ANDROID_TOOLCHAIN_DIR/bin/$target_host-clang
export CXX=$ANDROID_TOOLCHAIN_DIR/bin/$target_host-clang++
export LD="$ANDROID_TOOLCHAIN_DIR/bin/$target_host-ld -march=armv7-a -Wl,--fix-cortex-a8"
#export LD="$ANDROID_TOOLCHAIN_DIR/bin/$target_host-ld"
export STRIP=$ANDROID_TOOLCHAIN_DIR/bin/$target_host-strip
export READELF=$ANDROID_TOOLCHAIN_DIR/bin/$target_host-readelf
export LINK=$LD
export NM=$ANDROID_TOOLCHAIN_DIR/bin/$target_host-nm

make gyp-android

