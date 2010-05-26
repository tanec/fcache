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

add_library(SMALLOC smalloc.c)
add_library(MD5 md5.c)

add_library(MAP tmap.c)
add_library(SKIPLIST skiplist.c)
target_link_libraries(SKIPLIST  SMALLOC MAP)

########################################
add_library(READER reader.c)

add_library(READMEM read_mem.c)
target_link_libraries(READMEM  SKIPLIST)

add_library(BEREAD be_read.c)
add_library(READFILE read_file.c)
target_link_libraries(READFILE  BEREAD READER)
#########################################

add_library(STAT statistics.c)
add_library(UDP  zhongsou_net_udp.c)
add_library(HTTP zhongsou_net_http.c)

add_library(PROCESS process.c)
target_link_libraries(PROCESS MD5 READMEM READFILE STAT UDP HTTP)

add_library(SETTING settings.c)
add_library(LOG log.c)

add_library(ZSKW zhongsou_keyword.c)

add_executable (fcache fcache.c)
target_link_libraries(fcache  SETTING FASTCGI EVENT LOG UTIL PROCESS ZSKW)
