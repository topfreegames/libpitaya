# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
- Better documentation
- Unit testing (not only integration tests)
- Better support for languages other than C#

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

