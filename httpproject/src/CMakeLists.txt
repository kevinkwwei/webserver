cmake_minimum_required(VERSION 3.0)
project(http_project)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

add_definitions("-Wall -g")

include_directories(${PROJECT_SOURCE_DIR})

link_directories(${PROJECT_SOURCE_DIR})

add_executable(${PROJECT_NAME} main.cpp httpserver/http_server.cpp httpserver/http_server.h httpclient/http_client.cpp httpclient/http_client.h currency/semaphore.cpp currency/semaphore.h currency/mutex_locker.cpp currency/mutex_locker.h currency/condition_var.cpp currency/condition_var.h thread_pool/thread_pool.cpp thread_pool/thread_pool.h log/block_queue.cpp log/block_queue.h log/log.cpp log/log.h http_connection/http_connection.cc http_connection/http_connection.h timer/timer.cc timer/timer.h http_connection/http_structs.h mysql/sql_connection_pool.cc mysql/sql_connection_pool.h)

target_link_libraries(${PROJECT_NAME} pthread)