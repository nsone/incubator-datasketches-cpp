# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

cmake_minimum_required(VERSION 3.12.0)
project(DataSketches
        VERSION 0.12.0
        LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakeDependentOption)

### Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

# set default build type to debug
# Mostly from: https://blog.kitware.com/cmake-and-the-default-build-type/
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Ensure builds on Windows export all symbols
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_MACOSX_RPATH ON)

# enable compiler warnings globally
# derived from https://foonathan.net/blog/2018/10/17/cmake-warnings.html
# and https://arne-mertz.de/2018/07/cmake-properties-options/
if (MSVC)
  add_compile_options(/W4)
  set(CMAKE_DEBUG_POSTFIX "d")
else()
  add_compile_options(-Wall -pedantic)
endif()

# Code generation options, to ensure shaerd libraries work and are portable
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

###### OPTIONS ######
# Enable testing
option(BUILD_TESTS "Build unit tests" ON)
if (BUILD_TESTS)  
  enable_testing()
endif()

add_library(datasketches SHARED "")

add_subdirectory(common)
add_subdirectory(hll)
add_subdirectory(cpc)
add_subdirectory(kll)
add_subdirectory(fi)
add_subdirectory(theta)
add_subdirectory(sampling)

if (WITH_PYTHON)
  add_subdirectory(python)
endif()

target_link_libraries(datasketches PUBLIC hll cpc kll fi theta sampling)

set_target_properties(datasketches PROPERTIES
  LINKER_LANGUAGE CXX
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED YES
)

# # Installation
install(TARGETS datasketches
  EXPORT ${PROJCT_NAME}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/DataSketches
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/DataSketches
)

#set(CPACK_PROJECT_NAME ${PROJECT_NAME})
#set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
#include(CPack)
