#!/bin/sh
BUILD_TYPE="Release"
BUILD_DIR="$(pwd)/build-$BUILD_TYPE"

if [ -d "$BUILD_DIR" ]; then rm -Rf "$BUILD_DIR"; fi &&
  mkdir "$BUILD_DIR" &&
  cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DDXFCXX_PACKAGE_SUFFIX=-$BUILD_TYPE &&
  cmake --build "$BUILD_DIR" --config $BUILD_TYPE --parallel 8 &&
  cd "$BUILD_DIR" &&
  cpack -G ZIP -C $BUILD_TYPE --config ./dxFeedGraalCxxApiPackConfig.cmake
