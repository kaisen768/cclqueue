cmake_minimum_required(VERSION 3.4)
cmake_policy(SET CMP0048 NEW)

project(cclqueue LANGUAGES CXX)

add_compile_options(-O2 -std=c++11)

include_directories(${cclqueue_SOURCE_DIR})

# example
file(GLOB_RECURSE 
        SOURCE_FILES 
        ${cclqueue_SOURCE_DIR}/example/example.cc
        ${cclqueue_SOURCE_DIR}/blocking_queue.cc
)

add_executable(example ${SOURCE_FILES})
target_link_libraries(example 
    pthread
    dl
)
