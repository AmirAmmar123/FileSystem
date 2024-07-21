cmake_minimum_required(VERSION 3.28)
project(ex3)

set(CMAKE_CXX_STANDARD 17)

add_executable(ex3
        vfs.cpp
        myfs_main.cpp
        myfs.cpp
        blkdev.cpp
        )
