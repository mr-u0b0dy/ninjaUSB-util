FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
# qt6-connectivity-dev includes Bluetooth support
RUN apt-get update && apt-get install -y \
    cmake \
    qt6-base-dev \
    qt6-connectivity-dev \
    libudev-dev \
    libevdev-dev \
    build-essential \
    pkg-config \
    clang-tidy \
    cppcheck \
    valgrind \
    doxygen \
    graphviz \
    clang-format \
    gdb \
    ninja-build \
    git \
    && rm -rf /var/lib/apt/lists/*
