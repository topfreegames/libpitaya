FROM ubuntu:18.04

RUN apt-get update && apt-get --assume-yes install cmake g++-7 git ninja-build python3

ENV CC=gcc-7
ENV CXX=g++-7

ADD . /app
WORKDIR /app

RUN ./build-openssl.py --force --prefix openssl-build --openssl-tar ./deps/openssl-OpenSSL_1_0_1u.tar.gz linux
RUN make build-linux

