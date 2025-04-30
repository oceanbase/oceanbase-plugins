cmake_minimum_required(VERSION 3.22)

message(STATUS "PROJECT_VERSION is " ${PROJECT_VERSION})
if (NOT DEFINED PROJECT_VERSION)
  set(PROJECT_VERSION 0.2.0)
endif()

project(oceanbase-plugins
        VERSION      ${PROJECT_VERSION}
        DESCRIPTION  "OceanBase Plugins"
        HOMEPAGE_URL "https://open.oceanbase.com/"
        LANGUAGES    CXX C ASM)

include(cmake/Pack.cmake)

# use command below to build:
# cmake --build build --target jieba_ftparser
add_subdirectory(jieba_ftparser)
