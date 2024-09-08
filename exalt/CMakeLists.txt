# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Include local cache file if it exists
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/local.cmake)
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/local.cmake)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(exalt)

# target_sources(app PRIVATE src/main.cpp)
target_sources(app PRIVATE src/main.c)
