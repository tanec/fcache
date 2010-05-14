add_library(FASTCGI  fastcgi-api.c)
add_library(UTIL  util.c)

pkg_search_module(CURL REQUIRED libcurl)
include_directories(${CURL_INCLUDE_DIRS})
link_directories(${CURL_LIBRARY_DIRS})
add_library(CURL curl-api.c)
target_link_libraries(CURL  ${CURL_LIBRARIES})

find_path(EVENT_INC event.h)
include_directories(${EVENT_INC})
find_library(EVENT_LIB event)
message("found libevent: ${EVENT_LIB}, ${EVENT_INC}")
add_library(EVENT event-api.c)
target_link_libraries(EVENT ${EVENT_LIB})

add_library(LOG log.c)
add_library(SMALLOC smalloc.c)
add_library(MD5 md5.c)

add_executable (fcache fcache.c)
target_link_libraries(fcache  FASTCGI EVENT LOG UTIL SMALLOC MD5)
