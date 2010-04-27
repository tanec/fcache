cmake_minimum_required(VERSION 2.6)

project(fcache)
set (fcache_VERSION_MAJOR 1)
set (fcache_VERSION_MINOR 0)

enable_language("CXX")
include(FindPkgConfig)

pkg_search_module(CURL REQUIRED libcurl)

configure_file ("Config.h.in"  "${CMAKE_BINARY_DIR}/conf/Config.h")

include_directories (src ${CMAKE_BINARY_DIR}/conf)

add_definitions()
add_executable (fcache src/fcache.c)

enable_testing()
