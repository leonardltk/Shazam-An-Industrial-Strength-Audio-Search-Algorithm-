cmake_minimum_required(VERSION 2.8.10)
project(buildserver CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_RPATH "$ORIGIN")

include(FindThreads)
include(FindProtobuf)
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER proto/audio_feature.proto)
# protobuf_generate_cpp(PROTO_SRC PROTO_HEADER proto/proxy.proto)

## 3rdparty
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
    include_directories(
        include
        ${CMAKE_SOURCE_DIR}/3rdparty/include
        ${CMAKE_SOURCE_DIR}/3rdparty/brpc/include
        ${CMAKE_SOURCE_DIR}/3rdparty/decode_video/include
        ${CMAKE_SOURCE_DIR}/3rdparty/concurrentqueue/include
        ${CMAKE_SOURCE_DIR}/3rdparty/decode/include
        ${CMAKE_SOURCE_DIR}/3rdparty/ffmpeg/include
    )
    link_directories(/usr/lib/nvidia-418
        ${CMAKE_SOURCE_DIR}/3rdparty/brpc/lib
        ${CMAKE_SOURCE_DIR}/3rdparty/decode/lib
        ${CMAKE_SOURCE_DIR}/3rdparty/ffmpeg/lib
        ${CMAKE_SOURCE_DIR}/3rdparty/decode_video/lib
    )
    FIND_LIBRARY(SNDFILE sndfile)

## GLAGS
    find_path(GFLAGS_INCLUDE_PATH gflags/gflags.h)
    find_library(GFLAGS_LIBRARY NAMES gflags libgflags)
    if((NOT GFLAGS_INCLUDE_PATH) OR (NOT GFLAGS_LIBRARY))
        message(FATAL_ERROR "Fail to find gflags")
    endif()
    include_directories(${GFLAGS_INCLUDE_PATH})

    execute_process(
        COMMAND bash -c "grep \"namespace [_A-Za-z0-9]\\+ {\" ${GFLAGS_INCLUDE_PATH}/gflags/gflags_declare.h | head -1 | awk '{print $2}' | tr -d '\n'"
        OUTPUT_VARIABLE GFLAGS_NS
    )
    if(${GFLAGS_NS} STREQUAL "GFLAGS_NAMESPACE")
        execute_process(
            COMMAND bash -c "grep \"#define GFLAGS_NAMESPACE [_A-Za-z0-9]\\+\" ${GFLAGS_INCLUDE_PATH}/gflags/gflags_declare.h | head -1 | awk '{print $3}' | tr -d '\n'"
            OUTPUT_VARIABLE GFLAGS_NS
        )
    endif()

## Compile params
    set(CMAKE_CPP_FLAGS "-DGFLAGS_NS=${GFLAGS_NS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -DNDEBUG -O2 -D__const__= -pipe -W -Wall -Wno-unused-parameter -fPIC -fno-omit-frame-pointer -std=c++11")
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    message(STATUS "\n\t CMAKE_CPP_FLAGS    = ${CMAKE_CPP_FLAGS}\n")
    message(STATUS "\n\t CMAKE_CXX_FLAGS    = ${CMAKE_CXX_FLAGS}\n")

## Main Scripts
set(SRC
    src/fir_api.cc
    src/AudioLL.cpp
    src/Shash.cpp
    src/shash_class.cpp
)
   
## add_executable()
    message(STATUS "\nadd_executable(PROJECT_NAME, SRC, PROTO_SRC, PROTO_HEADER)")
    message(STATUS "\t PROJECT_NAME = ${PROJECT_NAME}")
    message(STATUS "\t SRC          = ${SRC}")
    message(STATUS "\t PROTO_SRC    = ${PROTO_SRC}")
    message(STATUS "\t PROTO_HEADER = ${PROTO_HEADER}")

## set()
    message(STATUS "\nset(DYNAMIC_LIB ...)")
    message(STATUS "\t CMAKE_THREAD_LIBS_INIT = ${CMAKE_THREAD_LIBS_INIT}")
    message(STATUS "\t GFLAGS_LIBRARY         = ${GFLAGS_LIBRARY}")
    message(STATUS "\t PROTOBUF_LIBRARIES     = ${PROTOBUF_LIBRARIES}")
    message(STATUS "\t LEVELDB_LIB            = ${LEVELDB_LIB}")
    message(STATUS "\t SSL_LIB                = ${SSL_LIB}")
    message(STATUS "\t CRYPTO_LIB             = ${CRYPTO_LIB}")
    message(STATUS "\t SNDFILE                = ${SNDFILE}")
    set(DYNAMIC_LIB
        ${CMAKE_THREAD_LIBS_INIT}
        ${GFLAGS_LIBRARY}
        ${PROTOBUF_LIBRARIES}
        ${LEVELDB_LIB}
        ${SSL_LIB}
        ${CRYPTO_LIB}
        ${SNDFILE}
        brpc
        decode_video
        /lib/x86_64-linux-gnu/libcrypto.so.1.0.0
        tcmalloc_and_profiler
        dl
        swresample
        avutil
        fftw3
        fftw3f
        libdecode_video.so
        )


SET(BINS
    # server_charlie
    # server_basic
    # Shazam_hashing
    # Shazam_querying
    server_feat
)
    message(STATUS "\n\t SRC          = ${SRC}\n")
    message(STATUS "\n\t DYNAMIC_LIB  = ${DYNAMIC_LIB}\n")
foreach(bin ${BINS})
    MESSAGE(STATUS "\ngenerate make for ${bin}")
    add_executable(${bin} src/${bin}.cpp ${SRC} ${PROTO_SRC} ${PROTO_HEADER})
    target_link_libraries(${bin} ${DYNAMIC_LIB})
endforeach()
