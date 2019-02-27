#!/bin/bash

function Usage
{
  echo "Usage: ./build.sh <arch ID> [debug|release|gcov|gprof|asan]
  arch IDs:
    1  linux-armv7    (gcc-linaro-4.9-2016.02-x86_64_arm-linux-gnueabihf)
    2  linux-armv8    (gcc-linaro-4.9-2016.02-x86_64_aarch64-linux-gnu)
    3  linux-arm64    (gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu)
    4  linux-x86_64   (/usr)
    5  android-armv7  (android-arm-r10e-api19-toolchain)
    6  android-armv8  (android-arm64-r10e-api21-toolchain)
    7  android-x86_32 (android-x86-r10e-api19-toolchain)
    8  mac-x86_64     (clang)
    9  cygwin-x86_64  (cygwin)
    10 linux-x86_32   (/usr)
    11 xtensa         (NatianlChip)

  Note:
    > android-armv7 with gprof: gmon.out placed at /sdcard/
    > asan: Make sure you have installed llvm (including llvm-symbolizer).
            Export the following variable
            export ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer
            (replace with your correct path to the llvm-symbolizer command).
            Now run your executable (a.out for now) as
            ASAN_OPTIONS=symbolize=1 a.out"
}

if [[ $# -lt 1 ]]; then
  Usage
  exit 1
fi

BUILD_ARCH="$1"

CMAKE372_BIN=$HOME/cmake-3.7.2/bin

CROSS_FOLDER=$HOME
SDK_BUILD_TYPE="Release"
ENABLE_GPROF=OFF
ENABLE_GCOV=OFF
ENABLE_ASAN=OFF

while [ $# != 1 ]; do
  case $2 in
    debug)
      SDK_BUILD_TYPE="Debug"
      ;;
    release)
      SDK_BUILD_TYPE="Release"
      ;;
    gcov)
      ENABLE_GCOV=ON
      ;;
    gprof)
      ENABLE_GPROF=ON
      ;;
    asan)
      ENABLE_ASAN=ON
      ;;
    *)
      Usage
      exit 1
      ;;
  esac
  shift || break
done

case "$BUILD_ARCH" in
  1)
    G_ARCH='armeabi-v7a'
    G_OS='embedded_linux'
    CROSS_TOOLCHAIN=${CROSS_FOLDER}/gcc-linaro-4.9-2016.02-x86_64_arm-linux-gnueabihf
    CXX_COMPILER=${CROSS_TOOLCHAIN}/bin/arm-linux-gnueabihf-g++
    C_COMPILER=${CROSS_TOOLCHAIN}/bin/arm-linux-gnueabihf-gcc
    STRIP=${CROSS_TOOLCHAIN}/bin/arm-linux-gnueabihf-strip
    ;;
  2)
    G_ARCH='arm64_v8a'
    G_OS='embedded_linux'
    CROSS_TOOLCHAIN=${CROSS_FOLDER}/gcc-linaro-4.9-2016.02-x86_64_aarch64-linux-gnu
    CXX_COMPILER=${CROSS_TOOLCHAIN}/bin/aarch64-linux-gnu-g++
    C_COMPILER=${CROSS_TOOLCHAIN}/bin/aarch64-linux-gnu-gcc
    STRIP=${CROSS_TOOLCHAIN}/bin/aarch64-linux-gnu-strip
    ;;
  3) # CNS3.0 (VW MIB3)
    G_ARCH='arm64'
    G_OS='embedded_linux'
    CROSS_TOOLCHAIN=${CROSS_FOLDER}/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu
    CXX_COMPILER=${CROSS_TOOLCHAIN}/bin/aarch64-linux-gnu-g++
    C_COMPILER=${CROSS_TOOLCHAIN}/bin/aarch64-linux-gnu-gcc
    STRIP=${CROSS_TOOLCHAIN}/bin/aarch64-linux-gnu-strip
    ;;
  4)
    G_ARCH='x86_64'
    G_OS='embedded_linux'
    CROSS_TOOLCHAIN=/usr
    CXX_COMPILER=${CROSS_TOOLCHAIN}/bin/g++
    C_COMPILER=${CROSS_TOOLCHAIN}/bin/gcc
    STRIP=${CROSS_TOOLCHAIN}/bin/strip
    ;;
  5)
    G_ARCH='armeabi-v7a'
    G_API_LEVEL=19
    G_OS='android'
    export PATH=${CMAKE372_BIN}:$PATH
    ANDROID_NDK_PATH=`pwd`/third_party/android-ndk-r16b
    G_TOOLCHAIN_FILE=${ANDROID_NDK_PATH}/build/cmake/android.toolchain.cmake
    G_STRIP_UTIL=${ANDROID_NDK_PATH}/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-strip
    ;;
  6)
    G_ARCH='arm64_v8a'
    G_OS='android'
    TOOLCHAINS_DIR=`pwd`/third_party/android-ndk-r10e/toolchains
    CMAKE_TOOLCHAIN_FILE=`pwd`/third_party/ndk/android.toolchain.r10e.pie_enabled.cmake
    ANDROID_STANDALONE_TOOLCHAIN=${TOOLCHAINS_DIR}/android-arm64-r10e-api21-toolchain
    CXX_COMPILER=${ANDROID_STANDALONE_TOOLCHAIN}/bin/aarch64-linux-android-g++
    C_COMPILER=${ANDROID_STANDALONE_TOOLCHAIN}/bin/aarch64-linux-android-gcc
    STRIP=${ANDROID_STANDALONE_TOOLCHAIN}/bin/aarch64-linux-android-strip
    ;;
  7)
    G_ARCH='x86_32'
    G_OS='android'
    TOOLCHAINS_DIR=`pwd`/third_party/android-ndk-r10e/toolchains
    CMAKE_TOOLCHAIN_FILE=`pwd`/third_party/ndk/android.toolchain.r10e.pie_enabled.cmake
    ANDROID_STANDALONE_TOOLCHAIN=${TOOLCHAINS_DIR}/android-x86-r10e-api19-toolchain
    CXX_COMPILER=${ANDROID_STANDALONE_TOOLCHAIN}/bin/i686-linux-android-g++
    C_COMPILER=${ANDROID_STANDALONE_TOOLCHAIN}/bin/i686-linux-android-gcc
    STRIP=${ANDROID_STANDALONE_TOOLCHAIN}/bin/i686-linux-android-strip
    ;;
  8)
    G_ARCH='x86_64'
    G_OS='mac'
    CROSS_TOOLCHAIN=/usr
    CXX_COMPILER=${CROSS_TOOLCHAIN}/bin/g++
    C_COMPILER=${CROSS_TOOLCHAIN}/bin/gcc
    STRIP=${CROSS_TOOLCHAIN}/bin/strip
    ;;
  9)
    G_ARCH='x86_64'
    G_OS='cygwin'
    CROSS_TOOLCHAIN=/usr
    CXX_COMPILER=${CROSS_TOOLCHAIN}/bin/g++
    C_COMPILER=${CROSS_TOOLCHAIN}/bin/gcc
    STRIP=${CROSS_TOOLCHAIN}/bin/strip
    ;;
  10)
    G_ARCH='x86_32'
    G_OS='embedded_linux'
    CROSS_TOOLCHAIN=/usr
    CXX_COMPILER=${CROSS_TOOLCHAIN}/bin/g++
    C_COMPILER=${CROSS_TOOLCHAIN}/bin/gcc
    STRIP=${CROSS_TOOLCHAIN}/bin/strip
    ;;
  11)
    G_ARCH='xtensa'
    G_OS='national_chip'
    CROSS_TOOLCHAIN=${CROSS_FOLDER}/xtensa/XtDevTools/install/tools/RG-2017.8-linux/XtensaTools
    CXX_COMPILER=${CROSS_TOOLCHAIN}/bin/xt-xc++
    C_COMPILER=${CROSS_TOOLCHAIN}/bin/xt-xcc
    STRIP=${CROSS_TOOLCHAIN}/bin/xt-strip
    export XTENSA_CORE=GXHifi4_170719A_G1708
    ;;
  *)
    echo "Unknown arch: $BUILD_ARCH"
    exit 1
    ;;
esac


BUILD_DIR="build.${G_OS}-${G_ARCH}.${SDK_BUILD_TYPE}"
rm -rf ${BUILD_DIR} && mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

if [[ "$G_OS" = "android" ]]; then
  cmake \
    -DANDROID_ABI=${G_ARCH}                    \
    -DANDROID_STL=c++_shared                   \
    -DANDROID_TOOLCHAIN=clang                  \
    -DANDROID_PLATFORM=android-${G_API_LEVEL}  \
    -DCMAKE_TOOLCHAIN_FILE=${G_TOOLCHAIN_FILE} \
    -DOS=${G_OS}                               \
    -DARCH=${G_ARCH}                           \
    -DPROJECT=${G_PROJECT}                     \
    -DCMAKE_BUILD_TYPE=${G_BUILD_TYPE}         \
    -DENABLE_GPROF=$ENABLE_GPROF \
    -DENABLE_GCOV=$ENABLE_GCOV \
    -DENABLE_ASAN=$ENABLE_ASAN \
    ..
else
  cmake \
    -DARCH=$G_ARCH \
    -DOS=$G_OS \
    -DCMAKE_C_COMPILER=$C_COMPILER \
    -DCMAKE_CXX_COMPILER=$CXX_COMPILER \
    -DENABLE_GPROF=$ENABLE_GPROF \
    -DENABLE_GCOV=$ENABLE_GCOV \
    -DENABLE_ASAN=$ENABLE_ASAN \
    -DCMAKE_BUILD_TYPE=${SDK_BUILD_TYPE} \
    ..
fi

make -j4

if [ $? -ne 0 ]; then
  echo "Build fail. Aborted"
  exit 1
fi

if [ "$ARCH" == "x86_64" ] || [ "$ARCH" == "x86_32" ]; then
  export GTEST_COLOR=1
  make test ARGS="-V"
fi

if [ $ENABLE_GCOV == ON ]; then
  lcov -d .. -c  -o all.lcov
  lcov --extract all.lcov '*/sdk/*' -o result.lcov
  genhtml -o result --show-details --legend -highlight result.lcov
  xdg-open result/index.html
fi
