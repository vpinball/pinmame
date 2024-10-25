#!/bin/bash

set -e

SDL_VERSION=3.1.3

NUM_PROCS=$(sysctl -n hw.ncpu)

echo "Building external libraries..."
echo "  SDL_VERSION: ${SDL_VERSION}"
echo ""

if [ -z "${BUILD_TYPE}" ]; then
   BUILD_TYPE="Release"
fi

echo "Build type: ${BUILD_TYPE}"
echo "Procs: ${NUM_PROCS}"
echo ""

CACHE_DIR="external/cache/${BUILD_TYPE}"

rm -rf external/include external/lib
mkdir -p external/include external/lib ${CACHE_DIR}

rm -rf tmp
mkdir tmp
cd tmp

#
# build SDL3, SDL_image, SDL_ttf and copy to external
#

CACHE_NAME="SDL-${SDL_VERSION}"

if [ ! -f "../${CACHE_DIR}/${CACHE_NAME}.cache" ]; then
   curl -sL https://github.com/libsdl-org/SDL/releases/download/preview-${SDL_VERSION}/SDL3-${SDL_VERSION}.tar.xz -o SDL3-${SDL_VERSION}.tar.xz
   tar -xf SDL3-${SDL_VERSION}.tar.xz
   cd SDL3-${SDL_VERSION}
   cmake \
      -DSDL_SHARED=ON \
      -DSDL_STATIC=OFF \
      -DSDL_TEST_LIBRARY=OFF \
      -DSDL_OPENGLES=OFF \
      -DCMAKE_OSX_ARCHITECTURES=arm64 \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
      -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -B build
   cmake --build build -- -j${NUM_PROCS}
   mkdir -p ../../${CACHE_DIR}/${CACHE_NAME}/include/SDL3
   cp -r include/SDL3/* ../../${CACHE_DIR}/${CACHE_NAME}/include/SDL3
   mkdir -p ../../${CACHE_DIR}/${CACHE_NAME}/lib
   cp -a build/*.dylib ../../${CACHE_DIR}/${CACHE_NAME}/lib
   cd ..

   touch "../${CACHE_DIR}/${CACHE_NAME}.cache"
fi

mkdir -p ../external/include/SDL3
cp -r ../${CACHE_DIR}/${CACHE_NAME}/include/SDL3/* ../external/include/SDL3
cp -a ../${CACHE_DIR}/${CACHE_NAME}/lib/*.dylib ../external/lib

