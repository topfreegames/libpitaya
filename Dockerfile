FROM ubuntu:20.04

ENV TZ=America/Sao_Paulo
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update && apt-get -y install make cmake g++-7 git ninja-build python3 zlib1g-dev

ENV CC=gcc-7
ENV CXX=g++-7

WORKDIR /app
