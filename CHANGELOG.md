# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [4.5.0] - 2023-01-30
- Fix appveyor builds
- Improve ARM64 support
- Upgrade dependencies (libuv, zlib, openssl, nanopb)
- Support for strongly typed JSON de/serialization using Newtonsoft.Json
- Deprecated methods explicitly using JsonObject and IMessage
- IPitayaClient interface
- Better documentation
- Unit testing (not only integration tests)
- Better support for languages other than C#

## [4.2.4] - 2022-08-15
### Changed
- Update Newtonsoft.Json dependency to 13.0.1

## [4.2.3] - 2022-05-10
### Added
- Add support for macOS M1 machines

## [4.2.2] - 2022-02-15
### Changed
- Compile LibPitaya DLLs using Unity 2019.4.34f1
- Replace Json library used on Libpitaya from [JSON.NET for Unity 2.0.1](https://assetstore.unity.com/packages/tools/input-management/json-net-for-unity-11347) by [Newtonsoft.Json 12.0.1](https://www.nuget.org/packages/Newtonsoft.Json/12.0.1)

## [4.1.1] - Unreleased
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

