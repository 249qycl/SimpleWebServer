cmake_minimum_required(VERSION 3.0)

project(myweb)

include_directories(.)
aux_source_directory(. DIR_SRCS)

add_executable(myweb ${DIR_SRCS} )

target_link_libraries(myweb mysqlclient)
#针对g++编译器的参数
set(CMAKE_CXX_FLAGS "-pthread")
set(CMAKE_BUILD_TYPE Debug)