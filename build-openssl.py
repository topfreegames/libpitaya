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
    shutil.rmtree(openssl_temp_dir)
    shutil.copytree(openssl_dir, openssl_temp_dir)
    return openssl_temp_dir


def make_toolchain(ndk_dir, openssl_temp_dir):
    # $NDK/build/tools/make_standalone_toolchain.py - -api 15 - -arch arm - -install-dir =`pwd`/ ../android-toolchain-arm - -deprecated-headers
    toolchain_script = os.path.join(
        ndk_dir, 'build', 'tools', 'make_standalone_toolchain.py')
    install_dir = os.path.join(openssl_temp_dir, 'android-toolchain')

    subprocess.run(
        '{} --api 15 --arch arm --install-dir={} --deprecated-headers'.format(
            toolchain_script, install_dir), shell=True, check=True)

    print('Installed toolchain to ' + install_dir)
    subprocess.run(
        'ls {}'.format(install_dir), shell=True, check=True)

    return install_dir


def set_envs(ndk_dir, toolchain_dir):
    ndk_toolchain_basename = '{}/{}'.format(
        toolchain_dir, 'arm-linux-androideabi')

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
        process = subprocess.Popen(
            [
                'cd ' + openssl_temp_dir,
                '&&',
                './Configure android-armv7 --prefix=' + args.prefix,
            ], bufsize=1, universal_newlines=True, shell=True, stdout=subprocess.PIPE)

        for line in iter(process.stdout.readline, ''):
            print(line)

        process = subprocess.Popen(
            [
                'cd ' + openssl_temp_dir,
                '&&',
                'make',
            ], bufsize=1, universal_newlines=True, shell=True, stdout=subprocess.PIPE)

        for line in iter(process.stdout.readline, ''):
            print(line)
    elif args.os == 'Linux':
        print('TODO')
    elif args.os == 'Darwin':
        print('TODO')
    elif args.os == 'Windows':
        print('TODO')


if __name__ == '__main__':
    main()
