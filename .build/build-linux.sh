#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
# Simple Linux release build script for fddconv

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Read version from VERSION file if it exists, otherwise use default
VERSION="1.0.0"
if [ -f "$SCRIPT_DIR/../../../VERSION" ]; then
    VERSION=$(cat "$SCRIPT_DIR/../../../VERSION" | tr -d '\n' | tr -d '\r')
fi

# Detect system architecture
ARCH=$(uname -m)
case "$ARCH" in
    x86_64|amd64)
        ARCH="x86_64"
        ;;
    i386|i486|i586|i686)
        ARCH="i386"
        ;;
    aarch64|arm64)
        ARCH="aarch64"
        ;;
    armv7l)
        ARCH="armv7"
        ;;
    *)
        echo "Unknown architecture: $ARCH"
        exit 1
        ;;
esac

BUILD_DIR="$SCRIPT_DIR/build/linux_${ARCH}"
RELEASE_DIR="$SCRIPT_DIR/release/dsk_tools-${VERSION}-linux-${ARCH}"

echo "Building fddconv for Linux ($ARCH)..."
echo "Version: $VERSION"
echo "Build directory: $BUILD_DIR"
echo "Release directory: $RELEASE_DIR"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake (Release build)
echo "Configuring CMake..."
cmake -DCMAKE_BUILD_TYPE=Release \
       -DENABLE_DSK_TOOLS=ON \
       "$PROJECT_DIR"

# Build fddconv
echo "Building fddconv..."
cmake --build . --config Release --target fddconv

# Create release directory
rm -rf "$RELEASE_DIR"
mkdir -p "$RELEASE_DIR"

# Copy binary
if [ -f "$BUILD_DIR/utils/fddconv" ]; then
    cp "$BUILD_DIR/utils/fddconv" "$RELEASE_DIR/fddconv"
    chmod +x "$RELEASE_DIR/fddconv"
    echo ""
    echo "✓ Build successful!"
    echo "  Binary: $RELEASE_DIR/fddconv"
    echo ""
    echo "To distribute, use: $RELEASE_DIR"
else
    echo "✗ Build failed: fddconv executable not found"
    exit 1
fi