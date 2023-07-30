cmake_minimum_required(VERSION 3.0)
project(http_client)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

add_definitions("-Wall -g")

include_directories(${PROJECT_SOURCE_DIR})

link_directories(${PROJECT_SOURCE_DIR})

add_executable(${PROJECT_NAME} main.cpp )

target_link_libraries(${PROJECT_NAME})