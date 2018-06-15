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

    #os.environ['ARCH_FLAGS'] = '-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16'
    #os.environ['ARCH_LINK'] = '-march=armv7-a -Wl,--fix-cortex-a8'
    #os.environ['CPPFLAGS'] = ' {} -fpic -ffunction-sections -funwind-tables -fstack-protector -fno-strict-aliasing -finline-limit=64 '.format(
        #os.environ['ARCH_FLAGS'])
    #os.environ['CXXFLAGS'] = ' {} -fpic -ffunction-sections -funwind-tables -fstack-protector -fno-strict-aliasing -finline-limit=64 -frtti -fexceptions '.format(
        #os.environ['ARCH_FLAGS'])
    #os.environ['CFLAGS'] = ' {} -fpic -ffunction-sections -funwind-tables -fstack-protector -fno-strict-aliasing -finline-limit=64 '.format(
        #os.environ['ARCH_FLAGS'])
    #os.environ['LDFLAGS'] = ' {} '.format(os.environ['ARCH_LINK'])
    #os.environ['CROSS_COMPILE'] = ''
    #os.environ['PATH'] = '{}:{}'.format(toolchain_path, os.environ['PATH'])

make gyp-android

