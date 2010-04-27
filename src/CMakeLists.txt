add_library(FASTCGI  fastcgi-api.c)
target_link_libraries(FASTCGI  fcgi)

pkg_search_module(CURL REQUIRED libcurl)
include_directories(${CURL_INCLUDE_DIRS})
link_directories(${CURL_LIBRARY_DIRS})
add_library(CURL curl.c)
target_link_libraries(CURL  ${CURL_LIBRARIES})

add_executable (fcache fcache.c)
target_link_libraries(fcache  FASTCGI CURL)
