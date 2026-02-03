FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libvterm-dev \
    libncurses-dev \
    pkg-config \
    gdb \
    tmux \
    python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /data
