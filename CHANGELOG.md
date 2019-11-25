# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
- Better documentation
- Unit testing (not only integration tests)
- Better support for languages other than C#

## [3.0.3] - 2019-10-28
### Fixed
- c#: connection timeout parameter was not being used (a default was hardcodeed).

## [3.0.2] - 2019-10-28
### Changed
- c: Fix heartbeat on the client to be independent from the heartbeats received from the server. This works more in line with the pitaya server and also fixes a bug where the client sometimes would disconnect in a perfectly normal connection environment.

## [3.0.1] - 2019-10-25
### Changed
- [BREAKING CHANGE] c#: Pass extra detail strings to events callback.

