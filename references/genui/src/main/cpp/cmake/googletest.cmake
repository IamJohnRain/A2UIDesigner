# googletest.cmake
# Shared FetchContent configuration for GoogleTest.
# Usage: include(cmake/googletest.cmake) after cmake_minimum_required(VERSION 3.14)
#
# Always fetches via FetchContent; system GTest is not used because
# GTest::gtest / GTest::gtest_main imported targets require CMake 3.20+,
# while this project supports 3.14.
#
# Offline override:
#   cmake -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=/path/to/googletest-source ...

include(FetchContent)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    googletest
    URL https://gitee.com/mirrors/googletest/repository/archive/v1.14.0.tar.gz
    URL_HASH SHA256=faba79f4f1a2c16543e96b3d57f890bbeb7fdd959bf614bd7e4ec3d7f2133b56
)

FetchContent_MakeAvailable(googletest)
