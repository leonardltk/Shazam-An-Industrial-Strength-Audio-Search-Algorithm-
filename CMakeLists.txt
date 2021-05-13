cmake_minimum_required(VERSION 3.5)
PROJECT(audio_ban_shash CXX)

set(CMAKE_CXX_STANDARD 14)

find_package( Boost 1.58.0 COMPONENTS system thread REQUIRED )
include_directories( ${Boost_INCLUDE_DIR} )

include_directories(include)

add_executable(${PROJECT_NAME}
    server.cpp
    MTSHasher.cpp
    shash/Shash.cpp
    shash/FastAudioShashManager.cpp
)
target_link_libraries(${PROJECT_NAME} tbb stdc++fs ${Boost_LIBRARIES} libgflags.a)

set(DEMO_ENABLE_TARGETS "audio_ban;auto_cluster" CACHE
        STRING "Targets to be built in demo simultaneously (default: both)")
