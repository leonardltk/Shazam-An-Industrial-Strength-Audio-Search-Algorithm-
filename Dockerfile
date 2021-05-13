FROM ubuntu:16.04
MAINTAINER yxp

RUN apt-get update && apt-get install -y vim libboost-system1.58.0 libtbb2
ADD build/audio_ban_shash /bin
ADD conf /etc/shash
RUN mkdir /etc/shash_bak
