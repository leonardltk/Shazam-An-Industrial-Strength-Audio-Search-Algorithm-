cmake_minimum_required(VERSION 2.8.10)
project(kafka-consumer-cpp CXX)

add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FindThreads)
include(FindProtobuf)

# protos
file(GLOB_RECURSE PROTOBUF_FILE_LIST "${CMAKE_SOURCE_DIR}/proto/*.proto")
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER ${PROTOBUF_FILE_LIST})


# 3rdparty
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR}/3rdparty/include)
link_directories(${CMAKE_SOURCE_DIR}/3rdparty/lib)

macro(get_git_hash _git_hash)   # 宏的开始
    find_package(Git QUIET)     # 查找Git，QUIET静默方式不报错
    if(GIT_FOUND)
      execute_process(          # 执行一个子进程
        COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%h # 命令
        OUTPUT_VARIABLE ${_git_hash}        # 输出字符串存入变量
        OUTPUT_STRIP_TRAILING_WHITESPACE    # 删除字符串尾的换行符
        ERROR_QUIET                         # 对执行错误静默
        WORKING_DIRECTORY                   # 执行路径
          ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
endmacro()                      # 宏的结束

set(GIT_HASH "")
get_git_hash(GIT_HASH)
add_definitions("-DGIT_HASH=\"${GIT_HASH}\"")
message(STATUS "Git hash is ${GIT_HASH}")

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

set(CMAKE_CPP_FLAGS "${DEFINE_CLOCK_GETTIME} -DGFLAGS_NS=${GFLAGS_NS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -DNDEBUG -O2 -D__const__= -pipe -W -Wall -Wno-unused-parameter -fPIC -fno-omit-frame-pointer -pthread -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CPP_FLAGS} -g -O0 -D__const__= -pipe -W -Wall -Wno-unused-parameter -fPIC -fno-omit-frame-pointer -pthread -std=c++11")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_GPU_DECODER=1")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# profile
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBRPC_ENABLE_CPU_PROFILER")

file(GLOB_RECURSE SRC "${CMAKE_SOURCE_DIR}/src/*.cpp")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/alarm)

set(DYNAMIC_LIB
    ${CMAKE_THREAD_LIBS_INIT}
    ${GFLAGS_LIBRARY}
    ${PROTOBUF_LIBRARIES}
    ${LEVELDB_LIB}
    ${SSL_LIB}
    ${CRYPTO_LIB}
    tcmalloc_and_profiler
    brpc
    crypto
    rdkafka++
    decode_audio_v2
    decode_video
    opus
    SKP_SILK_SDK
    avcodec
    avdevice
    avfilter
    avformat
    avutil
    swscale
    swresample
    dl
    )

add_executable(${PROJECT_NAME} ${SRC} ${PROTO_SRC} ${PROTO_HEADER})
target_link_libraries(${PROJECT_NAME} ${DYNAMIC_LIB})
