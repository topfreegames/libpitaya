# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres to [Semantic Versioning]
(https://semver.org/spec/v2.0.0.html).

## 4.6.3 - 2025-11-14
### Fixed
- Fixed iOS build compatibility with Xcode 16 and modern macOS runners (removed armv7/armv7s, disabled bitcode, fixed CMake 4.x parallel build syntax)
- Patched vendored zlib to prevent fdopen macro conflicts with iOS 18+ SDK headers

## [4.6.2] - 2023-05-05
### Fixed
- (unity) Fix post processor to manage pitaya libraries on iOS builds.

## [4.6.1] - 2023-02-07
### Fixed
- (unity) Fix post processor to manage pitaya libraries on iOS builds.

## [4.6.0] - 2023-01-31
### Changed
- Improve ARM64 support
- Upgrade dependencies (libuv, zlib, openssl, nanopb)
- Support for strongly typed JSON de/serialization using Newtonsoft.Json
- Deprecated methods explicitly using JsonObject and IMessage
- IPitayaClient interface
- Better documentation
- Unit testing (not only integration tests)
- Better support for languages other than C#
- (unity) Update Google.Protobuf dependency to 3.12.4

### Fixed
- Fix appveyor builds

## [4.2.4] - 2022-08-15
### Changed
- (unity) Update Newtonsoft.Json dependency to 13.0.1

## [4.2.3] - 2022-05-10
### Added
- Add support for macOS M1 machines

## [4.2.2] - 2022-02-15
### Changed
- Compile LibPitaya DLLs using Unity 2019.4.34f1
- Replace Json library used on Libpitaya from [JSON.NET for Unity 2.0.1](https://assetstore.unity.com/packages/tools/input-management/json-net-for-unity-11347) by [Newtonsoft.Json 12.0.1](https://www.nuget.org/packages/Newtonsoft.Json/12.0.1)

## [4.1.1] - 2020-06-16
### Fixed
- Fix crash in Unity 2019.3 when running in Android

## [4.1.0] - 2020-06-03
### Added
- ClearAllCallbacks method on PitayaClient class

### Fixed
- Fix crash in Unity 2019.3 when running in Linux

## [4.0.0] - 2020-04-04
**This version breaks compatibility with Unity 2018.4 or older**
### Fixed
- Fix AndroidJavaClass errors for Pitaya.dll in Android

### Added
- Add UnityEngine.AndroidJNIModule dependency (this breaks compatibility with Unity versions older than 2019.3)

## [3.0.4] - 2020-02-04
### Fixed
- c#: Fixed Unity Editor required to be closed and reopened to update game version

## [3.0.3] - 2019-10-28
### Fixed
- c#: connection timeout parameter was not being used (a default was hardcodeed).

## [3.0.2] - 2019-10-28
### Changed
- c: Fix heartbeat on the client to be independent from the heartbeats received from the server. This works more in line with the pitaya server and also fixes a bug where the client sometimes would disconnect in a perfectly normal connection environment.

## [3.0.1] - 2019-10-25
### Changed
- [BREAKING CHANGE] c#: Pass extra detail strings to events callback.

## [0.3.5] - 2015-06-30
### Changed
- Revert commit 798b2504 and re-fix it
- Refuse to send req/noti if not connected
- py: decrease refcount if req/noti failed

## [0.3.4] - 2015-06-30
### Changed
- Do not reset {conn_pending|write_wait} queue before reconnecting
- Fix a protential race condition bug

## [0.3.3] - 2015-06-30
### Fixed
- Fix a definitely race condition bug

## [0.3.2] - 2015-05-30
### Fixed
- Fix a definitely race condition bug
- Fix serveral potential race condition bugs and tidy code

## [0.3.1] - 2015-05-21
### Added
- Add null check for client_proto, etc.

### Changed
- Allow write_async\_cb to be invoked when NOT\_CONN
- Stop check timeout for writing queue

### Fixed
- Fix a bug that leads reconnect failure for tls

## [0.3.0] - 2015-05-15
### Changed
- Use cjson instead of jansson, cjson is more simple and bug-free
- Some other bugfixes

## [0.2.1] - 2015-04-10
### Changed
- Upgrade libuv to 1.4.2
- Multi bugfix

## [0.2.0] - 2015-03-02
### Changed
- Compile: enable -fPIC by default
- cs: add c# binding, Thanks to @hbbalfred

### Fixed
- Fix a fatal bug for tcp__handshake_ack

## [0.1.7] - 2015-02-02
### Changed
- Dummy: fix a double-free bug for dummy transport
- tls: use tls 1.2 instead of ssl 3

## [0.1.6] - 2015-01-30
### Changed
- Accept a pull request which makes compiling more friendly for android platform
- protocol: fix protobuf decode for repeated string
- client: remove event  first before firing the event
- poll: adding is_in_poll to avoid poll recursion
- client: fix a bug that leads to coredump when resetting

## [0.1.5] - 2014-11-11
### Changed
- client: don't poll before request/notify
- refactoring: rename field name ex_data to ls_ex_data of struct pc_config_t
- tls: update certificate for test case
- tr: destroy the mutex after uv loop exit
- tls: more log output by info_callback
- tls: more comment

### Fixed
- java, py: fix binding code bug
- tls: fix incorrect event emitting when cert is bad

## [0.1.4] - 2014-10-31
### Changed
- bugfix: init tcp handle before dns looking up

### Fixed
- py: fix protential deadlock for python binding
- reconn: fix incorrect reconn delay calc

## [0.1.3] - 2014-10-15
### Changed
- Clean code

### Fixed
- bugfix: typo for = <-> ==
- bugfix: fix warnings for multi-platform compilation

## [0.1.2] - 2014-10-10
### Changed
- Set timeout to PC_WITHOUT_TIMEOUT for internal pkg
- jansson: make valgrind happy

### Fixed
- bugfix: freeaddrinfo should be called after connect
- bugfix: incorrent init for uv_tcp_t, this leads memory leak

## [0.1.1] - 2014-09-30
### Fixed
- Misc bug fix

## [0.1.0] - 2014-09-03
### Changed
- Release first version
