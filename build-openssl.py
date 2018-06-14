#!/usr/bin/env python3

import argparse
import sys
import platform
import os
import subprocess
import tempfile
import shutil

THIS_DIR = os.path.realpath(os.path.dirname(__file__))


def parse_args():
    parser = argparse.ArgumentParser(
        description='Builds the openssl library for multiple platforms')
    parser.add_argument(
        '--os', choices=['Windows', 'Linux', 'Darwin', 'Android', 'iOS'], dest='os',
        help='which os to build for', default=platform.system())
    parser.add_argument('--prefix', required=True, dest='prefix',
                        help='Where should the binaries be installed to.')
    parser.add_argument('--openssl-dir', required=True, dest='openssl_dir',
                        help='Where is OpenSSL located.')
    parser.add_argument('--ndk-dir', required=True, dest='ndk_dir',
                        help='Where is OpenSSL located.')
    return parser.parse_args()


def make_openssl_temp_dir(openssl_dir):
    openssl_temp_dir = os.path.join(tempfile.gettempdir(), 'openssl')
    if os.path.exists(openssl_temp_dir):
        shutil.rmtree(openssl_temp_dir)
    shutil.copytree(openssl_dir, openssl_temp_dir)
    assert(os.path.exists(openssl_temp_dir))
    return openssl_temp_dir


def make_toolchain(ndk_dir, openssl_temp_dir):
    # $NDK/build/tools/make_standalone_toolchain.py - -api 15 - -arch arm - -install-dir =`pwd`/ ../android-toolchain-arm - -deprecated-headers
    toolchain_script = os.path.join(
        ndk_dir, 'build', 'tools', 'make_standalone_toolchain.py')
    install_dir = os.path.join(openssl_temp_dir, 'android-toolchain')

    cmd = '{} --api 15 --arch arm --install-dir={} --deprecated-headers'.format(
        toolchain_script, install_dir)

    print('Making toolchain...')
    subprocess.run(cmd, check=True, shell=True)

    return install_dir


def set_envs(ndk_dir, toolchain_dir):
    toolchain_path = os.path.join(toolchain_dir, 'bin')
    ndk_toolchain_basename = os.path.join(
        toolchain_path, 'arm-linux-androideabi'
    )

    os.environ['TOOLCHAIN_PATH'] = toolchain_path
    os.environ['TOOL'] = 'arm-linux-androideabi'
    os.environ['NDK_TOOLCHAIN_BASENAME'] = os.path.join(
        os.environ['TOOLCHAIN_PATH'], os.environ['TOOL'])

    os.environ['CC'] = ndk_toolchain_basename + '-gcc'
    os.environ['CXX'] = ndk_toolchain_basename + '-g++'
    os.environ['LINK'] = os.environ['CXX']
    os.environ['LD'] = ndk_toolchain_basename + '-ld'
    os.environ['AR'] = ndk_toolchain_basename + '-ar'
    os.environ['RANLIB'] = ndk_toolchain_basename + '-ranlib'
    os.environ['STRIP'] = ndk_toolchain_basename + '-strip'
    os.environ['ARCH_FLAGS'] = '-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16'
    os.environ['ARCH_LINK'] = '-march=armv7-a -Wl,--fix-cortex-a8'
    os.environ['CPPFLAGS'] = ' {} -fpic -ffunction-sections -funwind-tables -fstack-protector -fno-strict-aliasing -finline-limit=64 '.format(
        os.environ['ARCH_FLAGS'])
    os.environ['CXXFLAGS'] = ' {} -fpic -ffunction-sections -funwind-tables -fstack-protector -fno-strict-aliasing -finline-limit=64 -frtti -fexceptions '.format(
        os.environ['ARCH_FLAGS'])
    os.environ['CFLAGS'] = ' {} -fpic -ffunction-sections -funwind-tables -fstack-protector -fno-strict-aliasing -finline-limit=64 '.format(
        os.environ['ARCH_FLAGS'])
    os.environ['LDFLAGS'] = ' {} '.format(os.environ['ARCH_LINK'])
    os.environ['CROSS_COMPILE'] = ''
    os.environ['PATH'] = '{}:{}'.format(toolchain_path, os.environ['PATH'])

    print(os.environ['CC'])
    print(os.environ['PATH'])


def main():
    args = parse_args()

    if not os.path.exists(args.ndk_dir):
        print('NDK directory does not exist.')
        sys.exit(1)

    if not os.path.exists(args.openssl_dir):
        print('OpenSSL directory does not exist.')
        sys.exit(1)

    openssl_temp_dir = make_openssl_temp_dir(args.openssl_dir)

    if args.os == 'Android':
        toolchain_dir = make_toolchain(args.ndk_dir, openssl_temp_dir)
        set_envs(args.ndk_dir, toolchain_dir)

        print('Configuring for android')
        subprocess.run(
            'cd {} && ./Configure android-armv7 --prefix={}'.format(
                openssl_temp_dir, args.prefix),
            shell=True, check=True)

        print('Building...')
        subprocess.run('cd {} && make && make install'.format(
            openssl_temp_dir), shell=True)

    elif args.os == 'Linux':
        print('TODO')
    elif args.os == 'Darwin':
        print('TODO')
    elif args.os == 'Windows':
        print('TODO')


if __name__ == '__main__':
    main()
