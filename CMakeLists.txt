cmake_minimum_required(VERSION 2.6)

project(fcache)
set (fcache_VERSION_MAJOR 1)
set (fcache_VERSION_MINOR 0)

enable_language("CXX")
include(FindPkgConfig)

pkg_search_module(CURL REQUIRED libcurl)
pkg_search_module(CGICC libcgicc)

configure_file ("Config.h.in" "Config.h")

include_directories (src)
add_definitions()
add_executable (fcache src/fcache.c src/common.h Config.h)

enable_testing()
