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
import multiprocessing

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
    parser.add_argument('--compile-threads', dest='compile_threads', default=multiprocessing.cpu_count(), 
                        help='Number of processor threads to use during compilation')

    subparsers = parser.add_subparsers(dest='command')

    parser_android = subparsers.add_parser('android', help='Build for android')
    parser_android.add_argument('--arch', required=True, dest='android_arch',
                                help='Define the android architecture to build, possible values [aarch64, armv7a]')
    parser_android.add_argument('--ndk-dir', required=True, dest='ndk_dir',
                                help='Where is the Android NDK located.')

    parser_android = subparsers.add_parser('mac', help='Build for MacOSX')

    parser_android = subparsers.add_parser('windows', help='Build for Windows')

    parser_android = subparsers.add_parser('linux', help='Build for Linux')
    
    parser_android = subparsers.add_parser('ios', help='Build for iOS')
    parser_android.add_argument('--arch', required=True, dest='ios_arch',
                                help='Define the iOS architecture to build, possible values [armv7, arm64]')

    return parser.parse_args()


# Make a temporary directory for the openssl project extracted from the tar file
# and return the path
def make_openssl_temp_dir(openssl_tar):
    tempdir = tempfile.gettempdir()
    openssl_folder_name = os.path.basename(openssl_tar).split('.tar.gz')[0]
    openssl_temp_dir = os.path.join(tempdir, openssl_folder_name)
    print('Using temp dir {}'.format(openssl_temp_dir))

    if os.path.exists(openssl_temp_dir):
        shutil.rmtree(openssl_temp_dir)

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


def set_envs(ndk_dir):
    toolchain_path = os.path.join(ndk_dir, 'toolchains/llvm/prebuilt/linux-x86_64/bin')
    os.environ['CPPFLAGS'] = '-fPIC -ffunction-sections -funwind-tables -fstack-protector -fno-strict-aliasing '
    os.environ['CXXFLAGS'] = '-fPIC -ffunction-sections -funwind-tables -fstack-protector -fno-strict-aliasing -frtti -fexceptions '
    os.environ['CFLAGS'] = ' -fPIC -ffunction-sections -funwind-tables -fstack-protector -fno-strict-aliasing '
    os.environ['LDFLAGS'] = '-Wl'
    os.environ['ANDROID_NDK_HOME'] = ndk_dir
    os.environ['PATH'] = '{}:{}'.format(toolchain_path, os.environ['PATH'])


def build(openssl_temp_dir, compile_threads):
    print('Building...')
    call_shell('cd {} && make -j {}  && make install'.format(
        openssl_temp_dir, compile_threads))


def android_build(ndk_dir, arch, openssl_temp_dir, prefix, compile_threads):
    ndk_dir = os.path.expanduser(ndk_dir)
    ndk_dir = os.path.abspath(ndk_dir)

    if not os.path.exists(ndk_dir):
        print('NDK directory does not exist.')
        sys.exit(1)

    set_envs(ndk_dir)

    openssl_android_arch = ''
    if arch == 'armv7a':
        openssl_android_arch = 'android-arm'
    elif arch == 'aarch64':
        openssl_android_arch = 'android-arm64'
    else:
        print('Invalid android arch selected: {}, choose one from: [aarch64, armv7a]'.format(arch))
        sys.exit(1)

    call_shell(
        'cd {} && ./Configure {} --prefix={}'.format(
            openssl_temp_dir, openssl_android_arch, prefix))

    build(openssl_temp_dir, compile_threads)

def ios_build(arch, openssl_temp_dir, prefix, compile_threads):
    openssl_ios_arch = ''
    if arch == 'armv7':
        openssl_ios_arch = 'ios-xcrun'
    elif arch == 'arm64':
        openssl_ios_arch = 'ios64-xcrun'
    else:
        print('Invalid ios arch selected: {}, choose one from: [armv7, arm64]'.format(arch))
        sys.exit(1)
    call_shell(
        'cd {} && ./Configure {} --prefix={}'.format(
            openssl_temp_dir, openssl_ios_arch, prefix))

    build(openssl_temp_dir, compile_threads)

def mac_build(openssl_temp_dir, prefix, compile_threads):
    min_osx_version = '10.7'
    os.environ['CC'] = 'clang -mmacosx-version-min={}'.format(min_osx_version)
    os.environ['CFLAGS'] = '-Wno-error=implicit-function-declaration' # fix for compile issue https://github.com/openssl/openssl/issues/18720
    os.environ['CROSS_COMPILE'] = ''
    call_shell(
        'cd {} && ./Configure darwin64-x86_64-cc --prefix={}'.format(
            openssl_temp_dir, prefix))

    build(openssl_temp_dir, compile_threads)


def linux_build(openssl_temp_dir, prefix, compile_threads):
    os.environ['CROSS_COMPILE'] = ''
    os.environ['CC'] = '{} -fPIC'.format(os.environ['CC'])
    call_shell(
        'cd {} && ./Configure linux-x86_64 --prefix={}'.format(
            openssl_temp_dir, prefix))

    build(openssl_temp_dir, compile_threads)


def windows_build(openssl_temp_dir, prefix, compile_threads):
    os.environ['CROSS_COMPILE'] = ''
    call_shell(
        'cd {} && perl Configure VC-WIN64A --prefix={}'.format(
            openssl_temp_dir, prefix))
    build(openssl_temp_dir, compile_threads)


def main():
    args = parse_args()
    openssl_tar = os.path.expanduser(args.openssl_tar)
    compile_threads = args.compile_threads
    prefix = os.path.expanduser(args.prefix)

    print('Will compile OpenSSL with {} threads'.format(compile_threads))

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

    if args.command == 'android':
        android_build(args.ndk_dir, args.android_arch, openssl_temp_dir, prefix, compile_threads)
    elif args.command == 'linux':
        linux_build(openssl_temp_dir, prefix, compile_threads)
    elif args.command == 'mac':
        mac_build(openssl_temp_dir, prefix, compile_threads)
    elif args.command == 'windows':
        windows_build(openssl_temp_dir, prefix, compile_threads)
    elif args.command == 'ios':
        ios_build(args.ios_arch, openssl_temp_dir, prefix, compile_threads)


if __name__ == '__main__':
    main()
