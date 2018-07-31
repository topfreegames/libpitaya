<p align="center">
  <a href="https://travis-ci.com/topfreegames/libpitaya"><img src="https://travis-ci.com/topfreegames/libpitaya.svg?branch=master" alt="Build Status"></img></a>
  <a href="https://ci.appveyor.com/project/leohahn/libpitaya"><img src="https://ci.appveyor.com/api/projects/status/326391ofs0q26s0d/branch/master?svg=true&passingText=Windows" alt="Windows Build Status"></a>
</p>

Attention
=========

Libpitaya is currently under development and is not yet ready for production use. We are working on tests and better documentation and we'll update the project as soon as possible.

Libpitaya
=========

Libpitaya is a SDK for building clients for projects using pitaya game server framework and is built on top of [libpomelo2](https://github.com/NetEase/libpomelo2)

### How to compile 

#### Compiling Android

##### MAC
```bash
make setup-android
make build-android
```
##### LINUX
```bash
make setup-android-linux
make build-android
```
The android binary will be under `build/libpitaya-android.so`


#### Compiling Mac

```bash
make setup-mac
make build-mac
```
The android binary will be under `build/libpitaya-mac.bundle`


Tests
=====

In order to run the tests, you need the following dependencies:
- [go](https://golang.org) installed for the tests server.
- [nodejs](https://nodejs.org) for the mock server.

Having the dependencies met, you can run all the tests with the script `run-tests.sh`. Passing
`--help` to the script will give some information about how to use it. For example, you can say
`./run-tests.sh /my_suite` to run just one suite by name, or `./run-tests.sh /my_suite/test1` for
one specific test.

You can also directly use the executable `tests`, however you will have to manually start the mock and normal
servers and also compile the project if necessary.

