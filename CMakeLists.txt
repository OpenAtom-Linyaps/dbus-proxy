cmake_minimum_required(VERSION 3.0)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

project(dbusProxy)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 REQUIRED COMPONENTS Core DBus Network)
find_package(PkgConfig REQUIRED)
# libdbus-1-dev 添加dbus api支持
pkg_search_module(DBUS REQUIRED dbus-1)

# configure debug mode and asan
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose Release or Debug" FORCE)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  if (NOT CMAKE_DEBUG_ASAN)
    set(CMAKE_CXX_FLAGS_RELEASE "")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0  -Wall -g -ggdb3")
    set(CMAKE_CXX_FLAGS "-O0 -Wall -g -ggdb3")
  else ()
    set(CMAKE_CXX_FLAGS_RELEASE "")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -fsanitize=undefined,address -Wall -g -ggdb3")
    set(CMAKE_CXX_FLAGS "-O0 -fsanitize=undefined,address -Wall -g -ggdb3")
  endif ()
endif ()

# debug mode
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    ADD_DEFINITIONS(-DDEBUG)
endif()

include_directories(${PROJECT_SOURCE_DIR}/src)

add_subdirectory(src)
add_subdirectory(test)
MESSAGE(STATUS "current CPU ARCH is: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
MESSAGE(STATUS "project bin source " ${PROJECT_BINARY_DIR})
MESSAGE(STATUS "project source " ${PROJECT_SOURCE_DIR})
MESSAGE(STATUS "install " ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})