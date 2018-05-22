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

#### Install [gyp](https://gyp.gsrc.io/)
```
git clone https://chromium.googlesource.com/external/gyp
cd gyp
python setup.py install

```
#### Generate native IDE project files by [gyp](https://gyp.gsrc.io/)

    $ gyp --depth=. pomelo.gyp --generator-output=build [options]

options:


- -Dno_tls_support=[true | false], `false` by default

disable tls support

- -Duse_sys_openssl=[true | false], `true` by default

enable openssl, but use system pre-install libssl & libcrypto, if `false`, it will compile openssl from source code in deps/openssl.

- -Dno_uv_support=[true | false], `false` by default

disable uv support, it also disable tls support as tls implementation is based on uv.

- -Duse_sys_uv=[true | false], `false` by default

use system pre-install libuv, similar to `use_sys_openssl`, if enable, the pre-install libuv version should be 0.11.x

- -Duse_sys_jansson=[true | false], `false` by default

use system pre-install jansson.

- -Dpomelo_library=[static_library | shared_library], `static_library` by default

static library or shared library for libpomelo2

- -Dbuild_pypomelo=[true | false], `false` by default.
- -Dpython_header=<include path>, `/usr/include/python2.7` by default.

These two options is used to configure compilation for pypomelo.

- -Dbuild_jpomelo=[true|false], `false` by default.

configure jpomelo compilation for java

- -Dbuild_cspomelo=[true|false], `false` by default.

configure cspomelo compilation for c#

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

