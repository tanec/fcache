cmake_minimum_required(VERSION 2.8)

project(fcache)
set (fcache_VERSION_MAJOR 1)
set (fcache_VERSION_MINOR 0)

enable_language("CXX")
include(FindPkgConfig)

configure_file ("Config.h.in" "src/Config.h")

add_subdirectory(src)
#add_subdirectory(test)
enable_testing()
