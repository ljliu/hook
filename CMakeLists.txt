cmake_minimum_required(VERSION 2.8)
project(sdk)

# ---------------------- Common check ---------------------
if (NOT DEFINED OS)
    message(FATAL_ERROR "OS not defined, use -DOS=<os>.")
endif ()
if (NOT ${OS} MATCHES "^(embedded_linux|android|cygwin|mac|national_chip)")
    message(FATAL_ERROR "invalid OS " ${OS} ", use one of (embedded_linux|android|cygwin|mac|national_chip)")
endif ()

if (NOT DEFINED ARCH)
    message(FATAL_ERROR "ARCH not defined, use -DARCH=<arch>.")
endif ()

# ---------------------- Set FLAGS ------------------------
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

set(CMAKE_C_WARNING_FLAGS "${CMAKE_C_WARNING_FLAGS} -Werror=return-type")
set(CMAKE_CXX_WARNING_FLAGS "${CMAKE_CXX_WARNING_FLAGS} -Werror=return-type")

if (${ARCH} STREQUAL "xtensa")

else ()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE -pie -pthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    add_definitions(
            -DDSP_DUMP_AUDIO
            -fPIC
    )
endif ()

set(ENABLE_OPENBLAS OFF)
if (${OS} STREQUAL "android")
    set(ENABLE_OPENBLAS ON)
endif ()
if (${OS} STREQUAL "embedded_linux" AND ${ARCH} STREQUAL "x86_64")
    set(ENABLE_OPENBLAS ON)
endif ()

# needed for running unit test on android
if (${OS} STREQUAL "android")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie -fPIE")
endif ()

if (${ENABLE_GCOV})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
endif ()

if (${ENABLE_GPROF} AND ${OS} STREQUAL "android")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -fno-omit-frame-pointer")
    add_definitions(-DENABLE_GPROF)
endif ()

if (${ENABLE_ASAN})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(LIB_FOR_ASAN asan)
endif ()

if (${ENABLE_OPENBLAS})
    add_definitions(
            -DUSE_OPENBLAS
    )
endif ()

if (${ARCH} STREQUAL "armeabi-v7a")
    #armv8 has builtin neon support
    add_definitions(-mfpu=neon)
endif ()

set(THIRD_PARTY_SRC_DIR ${PROJECT_SOURCE_DIR}/third_party)
if (${ARCH} MATCHES "^arm" AND ${OS} STREQUAL "android")
    set(NDK_SRC_DIR ${THIRD_PARTY_SRC_DIR}/android-ndk)
    if (${ENABLE_GPROF})
        include_directories(${THIRD_PARTY_SRC_DIR}/android-ndk-profiler)
        set(NDK_PROFILER_LIB ${THIRD_PARTY_SRC_DIR}/android-ndk-profiler/armeabi-v7a/libandroid-ndk-profiler.a)
    endif ()
    add_definitions(
            -DHAVE_MEMALIGN
            -DUSE_LOOP_UNROLLING
            -APP_PLATFORM=android-14
    )
    if (NOT ${ARCH} STREQUAL "arm64_v8a")
        add_definitions(
                -mfloat-abi=softfp
        )
    endif ()
endif ()

# -------------------- Common include/src -----------------
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${THIRD_PARTY_SRC_DIR})
link_directories(${THIRD_PARTY_SRC_DIR}/mobvoidsp/libs/armeabi-v7a)
link_directories(${THIRD_PARTY_SRC_DIR}/mobvoisds/libs/armeabi-v7a)

if (${OS} STREQUAL "android")
    set(LIBS log OpenSLES mobvoidsp mobvoisds)
    set(LIBS_FOR_UNIT_DEMO log OpenSLES mobvoidsp)
endif ()

#----------------------- Demo -------------------------
set(LIBS_FOR_DEMO ${LIBS} ${NDK_PROFILER_LIB})

add_executable(qualcomm_online_demo
        ${PROJECT_SOURCE_DIR}/qualcomm_demo/online_demo.cc
        ${PROJECT_SOURCE_DIR}/utils/AudioPlayer.cpp
        ${PROJECT_SOURCE_DIR}/utils/AudioRecord.cpp
        ${PROJECT_SOURCE_DIR}/utils/NativeAudioBase.cpp
        ${PROJECT_SOURCE_DIR}/utils/MobPipeline.cpp
        ${PROJECT_SOURCE_DIR}/utils/mobvoi_serial.c)
target_link_libraries(qualcomm_online_demo ${LIBS_FOR_DEMO})

#---------------------- Unit Demo ---------------------
add_executable(test_dsp_pipeline
        ${PROJECT_SOURCE_DIR}/utils/test_dsp_pipeline.cpp
        ${PROJECT_SOURCE_DIR}/utils/AudioRecord.cpp
        ${PROJECT_SOURCE_DIR}/utils/MobPipeline.cpp
        ${PROJECT_SOURCE_DIR}/utils/mobvoi_serial.c
        ${PROJECT_SOURCE_DIR}/utils/NativeAudioBase.cpp)
target_link_libraries(test_dsp_pipeline ${LIBS_FOR_UNIT_DEMO})

add_executable(test_player
        ${PROJECT_SOURCE_DIR}/utils/test_player.cpp
        ${PROJECT_SOURCE_DIR}/utils/AudioPlayer.cpp
        ${PROJECT_SOURCE_DIR}/utils/NativeAudioBase.cpp)
target_link_libraries(test_player ${LIBS_FOR_UNIT_DEMO})
