#!/bin/sh
CUR_DIR="$(pwd)"
CPM_SOURCE_CACHE="$CUR_DIR/.cache/CPM"
DIST_DIR="$(pwd)/dist"
TAG="$(git describe --tags --abbrev=0)"

echo "--- $TAG ---"

if [ ! -d "$DIST_DIR" ]; then mkdir "$DIST_DIR"; fi

# set -- "Release" "Debug" "RelWithDebInfo" "MinSizeRel"
set -- "Release" "Debug"

for BUILD_TYPE in "$@"
do
  BUILD_DIR="$(pwd)/build-$BUILD_TYPE"

  if [ -d "$BUILD_DIR" ]; then rm -Rf "$BUILD_DIR"; fi &&
    echo "--- $BUILD_TYPE ---" &&
    echo "--- Removing old bundles ---" && find "$DIST_DIR" -maxdepth 1 -name "$TAG*$BUILD_TYPE.zip" -print0 | xargs -0 rm -f &&
    mkdir "$BUILD_DIR" &&
    echo "--- Configuring CMake ---" && cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DDXFCXX_VERSION="$TAG" \
      -DDXFCXX_PACKAGE_SUFFIX=-$BUILD_TYPE -DDXFCXX_BUILD_UNIT_TESTS=OFF -DDXFCXX_BUILD_SAMPLES=OFF \
      -DDXFCXX_BUILD_TOOLS=OFF -DDXFCXX_INSTALL_SAMPLES=OFF -DDXFCXX_INSTALL_TOOLS=OFF  -DDXFCXX_BUILD_DOC=OFF &&
    echo "--- Building ---" && cmake --build "$BUILD_DIR" --config $BUILD_TYPE --parallel 8 &&
    cd "$BUILD_DIR" &&
    echo "--- Packing ---" && cpack -G ZIP -C $BUILD_TYPE --config ./dxFeedGraalCxxApiPackConfig.cmake &&
    echo "--- Coping bundles ---" && find . -maxdepth 1 -name "*$BUILD_TYPE.zip" -type f -print0 | xargs -0 -I {} cp {} "$DIST_DIR/" &&
    cd "$CUR_DIR" || exit
done

BUILD_TYPE="Release"
COMPONENT="Samples"
BUILD_DIR="$(pwd)/build-$COMPONENT"

if [ -d "$BUILD_DIR" ]; then rm -Rf "$BUILD_DIR"; fi &&
  echo "--- $COMPONENT ---" &&
  echo "--- Removing old bundles ---" && find "$DIST_DIR" -maxdepth 1 -name "$TAG*$COMPONENT.zip" -print0 | xargs -0 rm -f &&
  mkdir "$BUILD_DIR" &&
  echo "--- Configuring CMake ---" && cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DDXFCXX_VERSION="$TAG" \
    -DDXFCXX_PACKAGE_SUFFIX=-$COMPONENT -DDXFCXX_BUILD_UNIT_TESTS=OFF -DDXFCXX_BUILD_TOOLS=OFF \
    -DDXFCXX_INSTALL_LIB=OFF -DDXFCXX_INSTALL_TOOLS=OFF  -DDXFCXX_BUILD_DOC=OFF &&
  echo "--- Building ---" && cmake --build "$BUILD_DIR" --config $BUILD_TYPE --parallel 8 &&
  cd "$BUILD_DIR" &&
  echo "--- Packing ---" && cpack -G ZIP -C $BUILD_TYPE --config ./dxFeedGraalCxxApiPackConfig.cmake &&
  echo "--- Coping bundles ---" && find . -maxdepth 1 -name "*$COMPONENT.zip" -type f -print0 | xargs -0 -I {} cp {} "$DIST_DIR/" &&
  cd "$CUR_DIR" || exit

COMPONENT="Tools"
BUILD_DIR="$(pwd)/build-$COMPONENT"

if [ -d "$BUILD_DIR" ]; then rm -Rf "$BUILD_DIR"; fi &&
  echo "--- $COMPONENT ---" &&
  echo "--- Removing old bundles ---" && find "$DIST_DIR" -maxdepth 1 -name "$TAG*$COMPONENT.zip" -print0 | xargs -0 rm -f &&
  mkdir "$BUILD_DIR" &&
  echo "--- Configuring CMake ---" && cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DDXFCXX_VERSION="$TAG" \
    -DDXFCXX_PACKAGE_SUFFIX=-$COMPONENT -DDXFCXX_BUILD_UNIT_TESTS=OFF -DDXFCXX_BUILD_SAMPLES=OFF \
    -DDXFCXX_INSTALL_LIB=OFF -DDXFCXX_INSTALL_SAMPLES=OFF  -DDXFCXX_BUILD_DOC=OFF &&
  echo "--- Building ---" && cmake --build "$BUILD_DIR" --config $BUILD_TYPE --parallel 8 &&
  cd "$BUILD_DIR" &&
  echo "--- Packing ---" && cpack -G ZIP -C $BUILD_TYPE --config ./dxFeedGraalCxxApiPackConfig.cmake &&
  echo "--- Coping bundles ---" && find . -maxdepth 1 -name "*$COMPONENT.zip" -type f -print0 | xargs -0 -I {} cp {} "$DIST_DIR/" &&
  cd "$CUR_DIR" || exit

