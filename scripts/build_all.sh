#!/bin/bash
# Build script for FXImage 2.1.0

echo "Building HubxSDK..."
cd HubxSDK
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ../..

echo "Building FXImage application..."
cd FXImage
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ../..

echo "Build complete!"
