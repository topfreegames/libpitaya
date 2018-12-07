<p align="center">
  <a href="https://travis-ci.com/topfreegames/libpitaya"><img src="https://travis-ci.com/topfreegames/libpitaya.svg?branch=master" alt="Build Status"></img></a>
  <a href="https://ci.appveyor.com/project/leohahn/libpitaya"><img src="https://ci.appveyor.com/api/projects/status/326391ofs0q26s0d/branch/master?svg=true&passingText=Windows" alt="Windows Build Status"></a>
</p>

Attention
=========

Libpitaya is currently under development and is not yet ready for production use. We are working on tests and better documentation and we'll update the project as soon as possible.

Libpitaya
=========

Libpitaya is a SDK for building clients for projects using the pitaya game server framework and is built on top of [libpomelo2](https://github.com/NetEase/libpomelo2)

## Building
Libpitaya is build using CMake. A standard build for linux could be something like:

```bash
cd libpitaya
cmake -H. -B_builds/linux -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build _builds/linux
```

Below we show how to build for all major platforms.

**NOTE**: In the `Makefile` there are helper targets that allow you to build for different platforms. Libpitaya is primarly developed to be used from other programming languages aside from C and C++, however, it should be possible to include the libpitaya CMake target from another CMake project using `add_subdirectory`.

### Building for Android

```bash
# Pick the correct setup for your platform
make setup-android-mac
make setup-adnroid-linux
# Build
make build-android
```

The Android binary will be under `_builds/android/libpitaya-android.so`

### Building for Mac

```
make setup-mac
make build-mac
```
The Mac binary will be under `_builds/mac/libpitaya-mac.bundle`

### Building for iOS

```bash
make setup-mac
make build-ios
```
The iOS binary will be under `_builds/ios/libpitaya-ios.a`
On iOS this library in conpiled statically wich means you must include in your project the dependencies `libz.a`,`libuv_a.a`,`libssl.a` and `libcrypto.a`.


---
* Note, however, that the chosen options can be changed if you need a different combination. For example, you can easily build statically on linux if you want. Take the Make target as an example.

Tests
=====

In order to run the tests, you need [node](https://nodejs.org) installed. Running the tests are done by running the following script in the root folder:
```bash
# The tests executable path are always placed alongside the pitaya library.
./run-tests.py --tests-dir <tests-executable-directory>
```

You can also pass command line options, for example:
```bash
# Lists all of the tests
./run-tests.py --tests-dir <tests-executable-directory> --list

# Runs one test by name
./run-tests.py --tests-dir <tests-executable-directory> /compression/enabled

# Runs one suite of tests
./run-tests.py --tests-dir <tests-executable-directory> /tls
```

You can also run the tests directly through the executable. The problem is that the mock servers will not be started, therefore some tests will fail. You can avoid that by manually starting the servers located in `test/mock-servers` using `node`.
