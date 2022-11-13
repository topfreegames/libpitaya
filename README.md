<p align="center">
  <a href="https://ci.appveyor.com/project/leohahn/libpitaya"><img src="https://ci.appveyor.com/api/projects/status/326391ofs0q26s0d/branch/master?svg=true&passingText=Windows" alt="Windows Build Status"></a>
</p>

# Attention

Libpitaya is currently under development and is not yet ready for production use. We are working on tests and better documentation and we'll update the project as soon as possible.

Libpitaya C# binding for 4.0.0 and above require Unity 2019.3 to work.

# Libpitaya

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
make setup-android-linux
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

### Building for iOS physical devices

```bash
make setup-mac
make build-ios
```
The iOS binary will be under `_builds/ios/libpitaya-ios.a`
On iOS, this library will be compiled statically which means you *must* include in your project the dependencies `libz.a`,`libuv_a.a`,`libssl.a` and `libcrypto.a` as well.


### Building for iOS simulators

```bash
make setup-mac
make build-ios-simulator
```
The iOS binary will be under `_builds/ios-simulator/libpitaya-ios.a`
As for physical device build, this library will be compiled statically, and other dependencies must be included manually.

### Building xcframework compatible with both simulators (x64, arm64) and devices

```bash
make setup-mac
make build-ios-xcframework
```
The xcframework will be under `_builds/ios-xcframework-libpitaya`
You can also build openssl xcframeworks using ```make build-openssl-ios-xcframework```

### Building for Windows (should use Developer Powershell for VS 2022)

```bash
make build-windows
```

The binaries for windows will then be located at `_builds/windows/pitaya-windows.dll` and `_builds/windows/pitaya-windows.lib`.

### Building for Linux

```bash
make build-linux
```

The binaries for windows will then be located at `_builds/linux/pitaya-linux.so`.

---
* Note, however, that the chosen options can be changed if you need a different combination. For example, you can easily build statically on linux if you want. Take the Make target as an example.


## Tests

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

You can also run the tests directly through the executable. The problem is that the mock servers will not be started, therefore some tests will fail. You can avoid that by manually starting the servers located in `test/mock-servers` using `./run-tests.py --only-deps`.

## Contribution
Libpitaya contributions should be done by making a Pull Request from a different branch.

### Native lib contribution
In order to run, edit and debug the code, you can generate native projects with CMake. For example, to create an XCode project, you can run the following commands:

```bash
git clone https://github.com/topfreegames/libpitaya
cd libpitaya
cmake -H. -B_builds/xcode
# The previous command has created an xcode project inside _builds/xcode.
# You can build with the command line if you want to:
cmake --build _builds/xcode --config Release
cmake --build _builds/xcode --config Debug
# Or you can open the project with xcode and use it through the IDE
open _builds/xcode/pitaya.xcodeproj
```
After you have edited, and builded the code, you should run the tests and make sure that they pass and also add a test for your specific change. The tests are separated in suites (a suite is a group of related tests). Each suite is located in its own source file under the test directory with names in the format test_<suite-name>.c.  If the addition is related to an existing suite, you can just create the new test and add it to the suite (it is just a function). However, if the functionality belongs in a new suite, you have to create the file and add it to cmake under the following block of code in CMakeLists.txt:

```cmake
add_executable(pitaya_tests
    # Sources
    test/main.c
    test/test_compression.c
    test/test_kick.c
    test/test_notify.c
    test/test_pc_client.c
    #==== Add your file here ====
    test/test_my_new_suite.c
    #============================
    )
```

Having added the file, you need to tell test/main.c that there is a new suite to be run, this can be done in the test/main.c source file:

```c
static const int SUITES_START = __LINE__;
extern const MunitSuite pc_client_suite;
extern const MunitSuite tcp_suite;
extern const MunitSuite tls_suite;
extern const MunitSuite session_suite;
extern const MunitSuite reconnection_suite;
extern const MunitSuite compression_suite;
extern const MunitSuite kick_suite;
extern const MunitSuite request_suite;
extern const MunitSuite notify_suite;
extern const MunitSuite stress_suite;
extern const MunitSuite protobuf_suite;
extern const MunitSuite push_suite;
extern const MunitSuite my_new_suite; // ==> Add your new suite here. Do NOT add new lines
static const int SUITES_END = __LINE__;

...

static MunitSuite *
make_suites()
{
    ...

    suites_array[i++] = pc_client_suite;
    suites_array[i++] = tcp_suite;
    suites_array[i++] = tls_suite;
    suites_array[i++] = session_suite;
    suites_array[i++] = reconnection_suite;
    suites_array[i++] = compression_suite;
    suites_array[i++] = kick_suite;
    suites_array[i++] = request_suite;
    suites_array[i++] = notify_suite;
    suites_array[i++] = stress_suite;
    suites_array[i++] = protobuf_suite;
    suites_array[i++] = push_suite;
    suites_array[i++] = my_new_suite;

    ...
}
```

Having done this, you can now run the tests using the run-tests.py script. If the tests pass, you should open a pull request.

**NOTE**: In most cases, you can just add the changes to the native library and it should be fine. However, if you change the interface of the library, you have to also update the csharp library.

### Using the build-openssl.py script to generate open ssl builds

Use it like the following:

```
# Android armv7

./build-openssl.py --prefix /tmp/build_dir/android --compile-threads 32 --openssl-tar deps/openssl-1.1.1q.tar.gz android --ndk-dir /home/...path to.../android-ndk-r25b --arch armv7a

# Android aarch64

./build-openssl.py --prefix /tmp/build_dir/android --compile-threads 32 --openssl-tar deps/openssl-1.1.1q.tar.gz android --ndk-dir /home/...path to.../android-ndk-r25b --arch aarch64

# Linux

CC=gcc ./build-openssl.py --prefix /tmp/build_dir/linux --compile-threads 32 --openssl-tar deps/openssl-1.1.1q.tar.gz linux

# Mac and iOS

For these we use the project at https://github.com/tfgco/OpenSSL

```

### Csharp lib contribution

For more information, please look in [this](unity/PitayaExample/README.md) document.

## Gotchas for mobile

When using libpitaya on mobile, it is important to understand some aspects of its behaviour on Android and iOS.
A pitaya client will always create a background thread that runs a libuv instance. The behaviour of this thread is different on iOS and Android.

#### iOS
When the application minimizes (goes to background), the pitaya thread will completely stop. This means that it will stop checking for hearbeats and also stop sending heartbeats. It will also stop any kind of pending requests for example. When the app goes back into foreground, the thread will start again. Since the thread stops, the server will stop receiving heartbeats from the client and may disconnect it if the heartbeat timeout passes, so depending on the heartbeat timeout configured by the server, making the app go to background may force the server to disconnect the client.

#### Android
The same scenario on Android does not block the thread, but it's quite the opposite. When the app is minimized the thread will continue running even though the app is in background. Since the thread is still sending heartbeats, the connection will not be dropped by the server. This could be a problem since the user might not wish to have the thread still alive while the app is in background.

#### A possible solution
If a consistent behaviour for Android and iOS is meant to be achieved, the default behaviour for both platforms will not make this possible. A possible solution is to force a disconnect when the app minimizes regardless of the platform. This disconnection can be triggered instantly or after X amount of seconds, for example. When the app is again in foreground a new connection can be created.
