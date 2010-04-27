cmake_minimum_required(VERSION 2.6)

project(fcache)
set (fcache_VERSION_MAJOR 1)
set (fcache_VERSION_MINOR 0)
#add_definitions()

enable_language("C")
include(FindPkgConfig)

configure_file ("Config.h.in"  "${CMAKE_BINARY_DIR}/conf/Config.h")
include_directories (${CMAKE_BINARY_DIR}/conf)

add_subdirectory(src)
add_subdirectory(test)
