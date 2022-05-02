#!/usr/bin/env python3

import argparse
import sys
import platform
import os
import subprocess as sp
import tempfile
import shutil
import stat
import tarfile

def call_shell(cmd):
    sp.call(cmd, shell=True)

def parse_args():
    parser = argparse.ArgumentParser(
        description='Builds the openssl library for multiple platforms')

    parser.add_argument('--prefix', required=True, dest='prefix',
                        help='Where should the binaries be installed to.')
    parser.add_argument('--openssl-tar', required=True, dest='openssl_tar',
                        help='Where is the OpenSSL tar file located.')
    parser.add_argument('--force', action='store_true',
                        help='If binaries already installed to prefix, this flag makes the build overwrite them.')

    subparsers = parser.add_subparsers(dest='command')

    parser_android = subparsers.add_parser('android', help='Build for android')
    parser_android.add_argument('--ndk-dir', required=True, dest='ndk_dir',
                                help='Where is the Android NDK located.')

    subparsers.add_parser('mac', help='Build for MacOSX')
    subparsers.add_parser('mac-m1', help='Build for MacOSX M1')
    subparsers.add_parser('windows', help='Build for Windows')
    subparsers.add_parser('linux', help='Build for Linux')

    return parser.parse_args()


# Make a temporary directory for the openssl project extracted from the tar file
# and return the path
def make_openssl_temp_dir(openssl_tar):
    tempdir = tempfile.gettempdir()
    openssl_folder_name = os.path.basename(openssl_tar).split('.')[0]
    openssl_temp_dir = os.path.join(tempdir, os.path.basename(openssl_tar).removesuffix('.tar.gz'))

    with tarfile.open(openssl_tar, 'r:gz') as tar:
        tar.extractall(tempdir)

    config_file = os.path.join(openssl_temp_dir, 'config')
    st = os.stat(config_file)
    os.chmod(config_file, st.st_mode | stat.S_IEXEC)

    configure_file = os.path.join(openssl_temp_dir, 'Configure')
    st = os.stat(configure_file)
    os.chmod(configure_file, st.st_mode | stat.S_IEXEC)

    assert(os.path.exists(openssl_temp_dir))
    return openssl_temp_dir


def make_ndk_toolchain(ndk_dir, openssl_temp_dir):
    # $NDK/build/tools/make_standalone_toolchain.py - -api 15 - -arch arm - -install-dir =`pwd`/ ../android-toolchain-arm - -deprecated-headers
    toolchain_script = os.path.join(
        ndk_dir, 'build', 'tools', 'make_standalone_toolchain.py')
    install_dir = os.path.join(openssl_temp_dir, 'android-toolchain')

    cmd = f'{toolchain_script} --api 15 --arch arm --install-dir={install_dir} --deprecated-headers'

    print('Making toolchain...')
    call_shell(cmd)

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
    os.environ['LD'] = ndk_toolchain_basename + '-ld'
    os.environ['LINK'] = os.environ['LD']
    os.environ['AR'] = ndk_toolchain_basename + '-ar'
    os.environ['AS'] = ndk_toolchain_basename + 'gcc'
    os.environ['NM'] = ndk_toolchain_basename + '-nm'
    os.environ['RANLIB'] = ndk_toolchain_basename + '-ranlib'
    os.environ['READELF'] = ndk_toolchain_basename + '-readelf'
    os.environ['STRIP'] = ndk_toolchain_basename + '-strip'
    os.environ['ARCH_FLAGS'] = '-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16'
    os.environ['ARCH_LINK'] = '-march=armv7-a -Wl,--fix-cortex-a8'
    os.environ['CPPFLAGS'] = f' {os.environ["ARCH_FLAGS"]} -fPIC -ffunction-sections -funwind-tables -fstack-protector -fno-strict-aliasing -finline-limit=64 '
    os.environ['CXXFLAGS'] = f' {os.environ["ARCH_FLAGS"]} -fPIC -ffunction-sections -funwind-tables -fstack-protector -fno-strict-aliasing -finline-limit=64 -frtti -fexceptions '
    os.environ['CFLAGS'] = f' {os.environ["ARCH_FLAGS"]} -fPIC -ffunction-sections -funwind-tables -fstack-protector -fno-strict-aliasing -finline-limit=64 '
    os.environ['LDFLAGS'] = f' {os.environ["ARCH_LINK"]} '
    os.environ['CROSS_COMPILE'] = ''
    os.environ['PATH'] = f'{toolchain_path}:{os.environ["PATH"]}'


def build(openssl_temp_dir):
    print('Building...')
    call_shell(f'cd {openssl_temp_dir} && make && make install')


def android_build(ndk_dir, openssl_temp_dir, prefix):
    print('+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+')
    print('+ Please note that the android build was only tested with +')
    print('+       the r15c ndk (higher it does not work             +')
    print('+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+')

    ndk_dir = os.path.expanduser(ndk_dir)
    ndk_dir = os.path.abspath(ndk_dir)

    if not os.path.exists(ndk_dir):
        print('NDK directory does not exist.')
        sys.exit(1)

    toolchain_dir = make_ndk_toolchain(ndk_dir, openssl_temp_dir)
    set_envs(ndk_dir, toolchain_dir)

    call_shell(
        'cd {} && ./Configure android-armeabi --prefix={}'.format(
            openssl_temp_dir, prefix))

    build(openssl_temp_dir)


def mac_build(openssl_temp_dir, prefix):
    min_osx_version = '10.7'
    os.environ['CC'] = 'clang -mmacosx-version-min={}'.format(min_osx_version)
    os.environ['CROSS_COMPILE'] = ''
    call_shell(
        'cd {} && ./Configure darwin64-x86_64-cc --prefix={}'.format(
            openssl_temp_dir, prefix))

    build(openssl_temp_dir)


def mac_m1_build(openssl_temp_dir, prefix):
    min_osx_version = '12.2'
    os.environ['CC'] = f'clang -mmacosx-version-min={min_osx_version}'
    os.environ['CROSS_COMPILE'] = ''
    call_shell(f'cd {openssl_temp_dir} && ./Configure darwin64-arm64-cc --prefix={prefix}')

    build(openssl_temp_dir)


def linux_build(openssl_temp_dir, prefix):
    os.environ['CROSS_COMPILE'] = ''
    os.environ['CC'] = f'{os.environ["CC"]} -fPIC'
    call_shell(f'cd {openssl_temp_dir} && ./Configure linux-x86_64 --prefix={prefix}')

    build(openssl_temp_dir)


def windows_build(openssl_temp_dir, prefix):
    os.environ['CROSS_COMPILE'] = ''
    call_shell(f'cd {openssl_temp_dir} && perl Configure VC-WIN64A --prefix={prefix}')

    build(openssl_temp_dir)


def main():
    args = parse_args()
    openssl_tar = os.path.expanduser(args.openssl_tar)
    prefix = os.path.expanduser(args.prefix)

    if not os.path.exists(openssl_tar):
        print('OpenSSL tar file does not exist.')
        sys.exit(1)

    if os.path.exists(prefix):
        if args.force:
            shutil.rmtree(prefix)
        else:
            print('Prefix path already exist, pass --force to overwrite it.')
            sys.exit(1)

    openssl_temp_dir = make_openssl_temp_dir(openssl_tar)
    prefix = os.path.abspath(prefix)

    print(openssl_temp_dir)
    print(prefix)

    if args.command == 'android':
        android_build(args.ndk_dir, openssl_temp_dir, prefix)
    elif args.command == 'linux':
        linux_build(openssl_temp_dir, prefix)
    elif args.command == 'mac':
        mac_build(openssl_temp_dir, prefix)
    elif args.command == 'mac-m1':
        mac_m1_build(openssl_temp_dir, prefix)
    elif args.command == 'windows':
        windows_build(openssl_temp_dir, prefix)
    else:
        raise Exception('build command not recognized')


if __name__ == '__main__':
    main()
