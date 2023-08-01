cmake_minimum_required(VERSION 3.0)
project(http_project)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

add_definitions("-Wall -g")

include_directories(${PROJECT_SOURCE_DIR})

link_directories(${PROJECT_SOURCE_DIR})

add_executable(${PROJECT_NAME} main.cpp httpserver/http_server.cpp httpserver/http_server.h httpclient/http_client.cpp httpclient/http_client.h)

target_link_libraries(${PROJECT_NAME})