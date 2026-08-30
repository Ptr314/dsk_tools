#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2025 Mikhail Revzin <p3.141592653589793238462643@gmail.com>
# Simple Linux release build script for the command line tools

set -e

# Command line tools to build and pack
TOOLS="fddconv aim2hfe"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Read version from VERSION file if it exists, otherwise use default
VERSION="1.0.0"
if [ -f "$SCRIPT_DIR/../VERSION" ]; then
    VERSION=$(cat "$SCRIPT_DIR/../VERSION" | tr -d '\n' | tr -d '\r')
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

echo "Building $TOOLS for Linux ($ARCH)..."
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

# Build the tools
echo "Building $TOOLS..."
cmake --build . --config Release --target $TOOLS

# Create release directory
rm -rf "$RELEASE_DIR"
mkdir -p "$RELEASE_DIR"

# Copy binaries
for TOOL in $TOOLS; do
    if [ ! -f "$BUILD_DIR/utils/$TOOL" ]; then
        echo "✗ Build failed: $TOOL executable not found"
        exit 1
    fi
    cp "$BUILD_DIR/utils/$TOOL" "$RELEASE_DIR/$TOOL"
    chmod +x "$RELEASE_DIR/$TOOL"
done

echo ""
echo "✓ Build successful!"
for TOOL in $TOOLS; do
    echo "  Binary: $RELEASE_DIR/$TOOL"
done
echo ""

# Create zip file
ZIP_FILE="$SCRIPT_DIR/release/dsk_tools-${VERSION}-linux-${ARCH}.zip"
if command -v zip &> /dev/null; then
    echo "Creating zip archive..."
    rm -f "$ZIP_FILE"
    (cd "$RELEASE_DIR" && zip -r "../$(basename "$ZIP_FILE")" . > /dev/null)
    if [ -f "$ZIP_FILE" ]; then
        ZIP_SIZE=$(du -h "$ZIP_FILE" | cut -f1)
        echo "✓ Archive created!"
        echo "  Archive: $ZIP_FILE ($ZIP_SIZE)"
        echo ""
        echo "To distribute, use: $ZIP_FILE"
    else
        echo "⚠ Warning: Failed to create zip file"
    fi
else
    echo "⚠ Warning: 'zip' command not found. Skipping archive creation."
    echo "To distribute, use: $RELEASE_DIR"
fi