# cjson.cmake
# Shared FetchContent configuration for cJSON.
# Usage: include(cmake/cjson.cmake) after cmake_minimum_required(VERSION 3.14)
#
# Offline override:
#   cmake -DFETCHCONTENT_SOURCE_DIR_CJSON=/path/to/cjson-source ...

include(FetchContent)

FetchContent_Declare(
    cjson
    URL https://gitee.com/mirrors/cJSON/repository/archive/v1.7.19.tar.gz
    URL_HASH SHA256=44ff674d24a05533bad82c61265c05ea271da724d580563b30b9f0db5ffe2dde
)

# Disable cJSON's own testing/install/export targets
set(ENABLE_CJSON_TEST OFF CACHE BOOL "" FORCE)
set(ENABLE_CJSON_UNINSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_TARGET_EXPORT OFF CACHE BOOL "" FORCE)

# Force static library: cJSON upstream checks CJSON_OVERRIDE_BUILD_SHARED_LIBS
# before honoring CJSON_BUILD_SHARED_LIBS. Isolate BUILD_SHARED_LIBS to avoid
# polluting subsequent dependencies (e.g. googletest).
set(_CJSON_SAVE_BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CJSON_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CJSON_OVERRIDE_BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(cjson)

# Restore BUILD_SHARED_LIBS so downstream targets are unaffected
if(DEFINED _CJSON_SAVE_BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS "${_CJSON_SAVE_BUILD_SHARED_LIBS}" CACHE BOOL "" FORCE)
else()
    unset(BUILD_SHARED_LIBS CACHE)
endif()
