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

from pathlib import Path

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

    parser_android = subparsers.add_parser('ios-universal', help='Build a fat lib for ios, containing archs armv7s armv7 and arm64')

    parser_android = subparsers.add_parser('ios-sim-universal', help='Build for iOS Simulator x86_64 and ARM64')
 
    parser_android = subparsers.add_parser('mac', help='Build for MacOSX')
    
    parser_android = subparsers.add_parser('mac-universal', help='Build for MacOSX x64 and ARM64 architecture')

    parser_android = subparsers.add_parser('windows', help='Build for Windows')

    parser_android = subparsers.add_parser('linux', help='Build for Linux')
    
    return parser.parse_args()

# Make a temporary directory for the openssl project extracted from the tar file
# and return the path

def make_openssl_temp_dir(root_folder, openssl_tar):
    tempdir = os.path.join(tempfile.gettempdir(), root_folder)
    openssl_folder_name = os.path.basename(openssl_tar).split('.tar.gz')[0]
    openssl_temp_dir = Path(tempdir, os.path.basename(openssl_tar).split('.tar.gz')[0])

    try:
        shutil.rmtree(openssl_temp_dir)
    except:
        None

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
    os.environ['ARCH_FLAGS'] = '-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16'
    os.environ['ARCH_LINK'] = '-march=armv7-a -Wl,--fix-cortex-a8'
    os.environ['CPPFLAGS'] = '-fPIC -ffunction-sections -funwind-tables -fstack-protector -fno-strict-aliasing '
    os.environ['CXXFLAGS'] = '-fPIC -ffunction-sections -funwind-tables -fstack-protector -fno-strict-aliasing -frtti -fexceptions '
    os.environ['CFLAGS'] = ' -fPIC -ffunction-sections -funwind-tables -fstack-protector -fno-strict-aliasing '
    os.environ['LDFLAGS'] = '-Wl'
    os.environ['ANDROID_NDK_HOME'] = ndk_dir
    os.environ['PATH'] = '{}:{}'.format(toolchain_path, os.environ['PATH'])


def build(openssl_temp_dir, compile_threads):
    print('Building...')
    call_shell('cd {} && make -j {}  && make install_sw'.format(
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

def iossim_build(openssl_arm64_temp_dir, openssl_x64_temp_dir, prefix, compile_threads):
    Path(prefix).mkdir(parents=True, exist_ok=True)
    Path(prefix).joinpath("lib").mkdir(exist_ok=True)

    temp_dir = tempfile.gettempdir()
    arm64_prefix = os.path.join(temp_dir, 'arm64')
    x64_prefix = os.path.join(temp_dir, 'x64')

    os.environ['CFLAGS'] = '-Wno-error=implicit-function-declaration' # fix for compile issue https://github.com/openssl/openssl/issues/18720
    print("=======================================================")
    print("ARM64 iOS Simulator BUILD")
    print("=======================================================")
    call_shell(
            f'cd {openssl_arm64_temp_dir} && ./Configure iossimulator-xcrun "-arch arm64 -fembed-bitcode" no-asm no-shared no-hw no-async --prefix={arm64_prefix}')
    build(openssl_arm64_temp_dir, compile_threads)

    print("=======================================================")
    print("x86_64 iOS Simulator BUILD")
    print("=======================================================")
    call_shell(f'cd {openssl_x64_temp_dir} && ./Configure iossimulator-xcrun "-arch x86_64 -fembed-bitcode" no-tests --prefix={x64_prefix}')
    build(openssl_x64_temp_dir, compile_threads)

    for lib in ["libssl.a", "libcrypto.a"]:
        print(f'creating universal binary for {lib}')
        call_shell(
            f'lipo -create {os.path.join(arm64_prefix, "lib", lib)} {os.path.join(x64_prefix, "lib", lib)} -output {os.path.join(prefix, "lib", lib)}'
        )

    call_shell(f'cp -r {os.path.join(arm64_prefix, "include")} {os.path.join(prefix, "include")}')

def ios_build(openssl_arm64_temp_dir, openssl_arm64e_temp_dir, openssl_armv7s_temp_dir, openssl_armv7_temp_dir, prefix, compile_threads):

    Path(prefix).mkdir(parents=True, exist_ok=True)
    Path(prefix).joinpath("lib").mkdir(exist_ok=True)

    temp_dir = tempfile.gettempdir()
    arm64_prefix = os.path.join(temp_dir, 'arm64')
    arm64e_prefix = os.path.join(temp_dir, 'arm64e')
    armv7s_prefix = os.path.join(temp_dir, 'armv7s')
    armv7_prefix = os.path.join(temp_dir, 'armv7')

    os.environ['OPENSSL_LOCAL_CONFIG_DIR'] = f'{os.getcwd()}/openssl-config'
    os.environ['CFLAGS'] = '-Wno-error=implicit-function-declaration' # fix for compile issue https://github.com/openssl/openssl/issues/18720
    print("=======================================================")
    print("ARM64 iOS BUILD")
    print("=======================================================")
    call_shell(
            f'cd {openssl_arm64_temp_dir} && ./Configure ios64-xcrun no-asm no-async no-shared --prefix={arm64_prefix}')
    build(openssl_arm64_temp_dir, compile_threads)

    print("=======================================================")
    print("ARM64e iOS BUILD")
    print("=======================================================")
    call_shell(
            f'cd {openssl_arm64e_temp_dir} && ./Configure ios-xcrun-arm64e no-asm no-async no-shared --prefix={arm64e_prefix}')
    build(openssl_arm64e_temp_dir, compile_threads)


    print("=======================================================")
    print("ARMV7S iOS BUILD")
    print("=======================================================")
    call_shell(
            f'cd {openssl_armv7s_temp_dir} && ./Configure ios-xcrun-armv7s no-asm no-async no-shared --prefix={armv7s_prefix}')
    build(openssl_armv7s_temp_dir, compile_threads)

    print("=======================================================")
    print("ARMV7 iOS BUILD")
    print("=======================================================")
    call_shell(
            f'cd {openssl_armv7_temp_dir} && ./Configure ios-xcrun-armv7 no-asm no-async no-shared --prefix={armv7_prefix}')
    build(openssl_armv7_temp_dir, compile_threads)

    for lib in ["libssl.a", "libcrypto.a"]:
        print(f'creating universal binary for {lib}')
        call_shell(
            f'lipo -create {os.path.join(arm64_prefix, "lib", lib)} {os.path.join(arm64e_prefix, "lib", lib)} {os.path.join(armv7s_prefix, "lib", lib)} {os.path.join(armv7_prefix, "lib", lib)} -output {os.path.join(prefix, "lib", lib)}'
        )

    call_shell(f'cp -r {os.path.join(arm64_prefix, "include")} {os.path.join(prefix, "include")}')

def mac_build(openssl_temp_dir, prefix, compile_threads):
    min_osx_version = '10.7'
    os.environ['CC'] = 'clang -mmacosx-version-min={}'.format(min_osx_version)
    os.environ['CFLAGS'] = '-Wno-error=implicit-function-declaration' # fix for compile issue https://github.com/openssl/openssl/issues/18720
    os.environ['CROSS_COMPILE'] = ''

    call_shell(
        'cd {} && ./Configure darwin64-x86_64-cc no-tests --prefix={}'.format(
            openssl_temp_dir, prefix))

    build(openssl_temp_dir, compile_threads)

def mac_universal_build(openssl_arm64_temp_dir, openssl_x64_temp_dir, prefix, compile_threads):
    # Ensure that prefix exists.
    Path(prefix).mkdir(parents=True, exist_ok=True)
    Path(prefix).joinpath("lib").mkdir(exist_ok=True)

    min_osx_version = '12.2'
    os.environ['CC'] = f'clang -mmacosx-version-min={min_osx_version}'
    os.environ['CROSS_COMPILE'] = ''

    temp_dir = tempfile.gettempdir()
    arm64_prefix = os.path.join(temp_dir, 'arm64')
    x64_prefix = os.path.join(temp_dir, 'x64')

    print("=======================================================")
    print("ARM 64 BUILD")
    print("=======================================================")
    call_shell(f'cd {openssl_arm64_temp_dir} && ./Configure darwin64-arm64-cc no-tests --prefix={arm64_prefix}')
    build(openssl_arm64_temp_dir, compile_threads)

    print("=======================================================")
    print("x86_64 BUILD")
    print("=======================================================")
    call_shell(f'cd {openssl_x64_temp_dir} && ./Configure darwin64-x86_64-cc no-tests --prefix={x64_prefix}')
    build(openssl_x64_temp_dir, compile_threads)

    for lib in ["libssl.a", "libcrypto.a"]:
        print(f'creating universal binary for {lib}')
        call_shell(
            f'lipo -create {os.path.join(arm64_prefix, "lib", lib)} {os.path.join(x64_prefix, "lib", lib)} -output {os.path.join(prefix, "lib", lib)}'
        )

    call_shell(f'cp -r {os.path.join(arm64_prefix, "include")} {os.path.join(prefix, "include")}')


def mac_universal_build(openssl_arm64_temp_dir, openssl_x64_temp_dir, prefix):
    # Ensure that prefix exists.
    Path(prefix).mkdir(parents=True, exist_ok=True)
    Path(prefix).joinpath("lib").mkdir(exist_ok=True)

    min_osx_version = '12.2'
    os.environ['CC'] = f'clang -mmacosx-version-min={min_osx_version}'
    os.environ['CROSS_COMPILE'] = ''

    temp_dir = tempfile.gettempdir()
    arm64_prefix = os.path.join(temp_dir, 'arm64')
    x64_prefix = os.path.join(temp_dir, 'x64')

    print("=======================================================")
    print("ARM 64 BUILD")
    print("=======================================================")
    call_shell(f'cd {openssl_arm64_temp_dir} && ./Configure darwin64-arm64-cc --prefix={arm64_prefix}')
    build(openssl_arm64_temp_dir)

    print("=======================================================")
    print("x86_64 BUILD")
    print("=======================================================")
    call_shell(f'cd {openssl_x64_temp_dir} && ./Configure darwin64-x86_64-cc --prefix={x64_prefix}')
    build(openssl_x64_temp_dir)

    for lib in ["libssl.a", "libcrypto.a"]:
        print(f'creating universal binary for {lib}')
        call_shell(
            f'lipo -create {os.path.join(arm64_prefix, "lib", lib)} {os.path.join(x64_prefix, "lib", lib)} -output {os.path.join(prefix, "lib", lib)}'
        )

    call_shell(f'cp -r {os.path.join(arm64_prefix, "include")} {os.path.join(prefix, "include")}')


def linux_build(openssl_temp_dir, prefix, compile_threads):
    os.environ['CROSS_COMPILE'] = ''
    os.environ['CC'] = f'{os.environ["CC"]} -fPIC'
    call_shell(f'cd {openssl_temp_dir} && ./Configure linux-x86_64 --prefix={prefix}')

    build(openssl_temp_dir, compile_threads)


def windows_build(openssl_temp_dir, prefix, compile_threads):
    os.environ['CROSS_COMPILE'] = ''
    call_shell(f'cd {openssl_temp_dir} && perl Configure VC-WIN64A --prefix={prefix}')

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

    prefix = os.path.abspath(prefix)

    if args.command == 'android':
        openssl_temp_dir = make_openssl_temp_dir('android', openssl_tar)
        android_build(args.ndk_dir, args.android_arch, openssl_temp_dir, prefix, compile_threads)
    elif args.command == 'linux':
        openssl_temp_dir = make_openssl_temp_dir('linux', openssl_tar)
        linux_build(openssl_temp_dir, prefix, compile_threads)
    elif args.command == 'mac':
        openssl_temp_dir = make_openssl_temp_dir('mac', openssl_tar)
        mac_build(openssl_temp_dir, prefix, compile_threads)
    elif args.command == 'ios-universal':
        openssl_arm64_temp_dir = make_openssl_temp_dir('ios-arm64', openssl_tar)
        openssl_arm64e_temp_dir = make_openssl_temp_dir('ios-arm64e', openssl_tar)
        openssl_armv7_temp_dir = make_openssl_temp_dir('ios-armv7', openssl_tar)
        openssl_armv7s_temp_dir = make_openssl_temp_dir('ios-armv7s', openssl_tar)
        ios_build(openssl_arm64_temp_dir, openssl_arm64e_temp_dir, openssl_armv7s_temp_dir, openssl_armv7_temp_dir, prefix, compile_threads)
    elif args.command == 'ios-sim-universal':
        openssl_arm64_temp_dir = make_openssl_temp_dir('simulator-arm64', openssl_tar)
        openssl_x64_temp_dir = make_openssl_temp_dir('simulator-x64', openssl_tar)
        iossim_build(openssl_arm64_temp_dir, openssl_x64_temp_dir, prefix, compile_threads)
    elif args.command == 'mac-universal':
        openssl_arm64_temp_dir = make_openssl_temp_dir('mac-arm64', openssl_tar)
        openssl_x64_temp_dir = make_openssl_temp_dir('mac-x64', openssl_tar)
        mac_universal_build(openssl_arm64_temp_dir, openssl_x64_temp_dir, prefix, compile_threads)
    elif args.command == 'windows':
        windows_build(openssl_temp_dir, prefix, compile_threads)

if __name__ == '__main__':
    main()
