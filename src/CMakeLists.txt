find_path(EVENT_INC event.h)
include_directories(${EVENT_INC})
find_library(EVENT_LIB event)
message("found libevent: ${EVENT_LIB}, ${EVENT_INC}")

pkg_check_modules(GLIB REQUIRED glib-2.0)
include_directories(GLIB_INCLUDE_DIRS)
message("${GLIB_INCLUDE_DIRS}")

add_library(UTIL  util.c)
add_library(SMALLOC smalloc.c)
add_library(MD5 md5.c)
add_library(SETTING settings.c)
add_library(LOG log.c)
add_library(THREAD thread.c)

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
add_library(AUTH zhongsou_net_auth.c)
add_library(UDP  zhongsou_net_udp.c)
target_link_libraries(UDP THREAD)

add_library(PROCESS process.c)
target_link_libraries(PROCESS MD5 READMEM READFILE STAT AUTH UDP)

add_library(ZSKW zhongsou_keyword.c)

add_executable (fcache fcache.c)
target_link_libraries(fcache ${EVENT_LIB} SETTING LOG UTIL THREAD PROCESS ZSKW)
