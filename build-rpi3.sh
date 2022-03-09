#!/bin/bash

############################################
# OSSPR2022                                #
#                                          #
# Builds your ARM64 kernel.                #
############################################

set -e

ARCH=arm64
CC=clang
CROSS_COMPILE='aarch64-linux-gnu-'

if which ccache &> /dev/null; then
	CC="ccache $CC"
fi

# Build .config
make ARCH="$ARCH" CC="$CC" CROSS_COMPILE="$CROSS_COMPILE" tizen_bcmrpi3_defconfig

# Build kernel
make ARCH="$ARCH" CC="$CC" CROSS_COMPILE="$CROSS_COMPILE" -j$(nproc)
