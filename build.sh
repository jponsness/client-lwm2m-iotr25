#!/bin/sh

# Get the wakaama sources
git submodule init
git submodule update --init --recursive

cd lwm2m

# Build the C headers from the Go files
go tool cgo -exportheader ./src/gocallbacks.h m2m.go callbacks_device.go callbacks_snap.go

# Build the C code as a static library liblwm2mclient.a
cmake . && make

# Clean up the build files
rm -rf CMakeFiles/ CMakeCache.txt cmake_install.cmake Makefile _obj/
