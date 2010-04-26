cmake_minimum_required(VERSION 2.6)

project(fcache)
set (fcache_VERSION_MAJOR 1)
set (fcache_VERSION_MINOR 0)

enable_language("CXX")
include(FindPkgConfig)

configure_file ("Config.h.in" "src/Config.h")

include_directories (src)
add_definitions()
add_executable (fcache src/fcache.c)

enable_testing()
