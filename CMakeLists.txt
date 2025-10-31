cmake_minimum_required(VERSION 3.31)
project(TFTP_Server)

set(CMAKE_CXX_STANDARD 20)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(TFTP_Server main.cpp
        src/socket.cpp
        src/process.cpp
        include/logWrite.h
        src/logWrite.cpp
)


