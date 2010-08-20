find_library(PTMALLOC_LIB ptmalloc3)
message("found libptmalloc: ${PTMALLOC_LIB}")

find_path(EVENT_INC event.h)
include_directories(${EVENT_INC})
find_library(EVENT_LIB event)
message("found libevent: ${EVENT_LIB}, ${EVENT_INC}")

pkg_check_modules(GLIB REQUIRED glib-2.0)
include_directories(${GLIB_INCLUDE_DIRS})
link_directories(${GLIB_LIBRARY_DIRS})
message("glib2: -I${GLIB_INCLUDE_DIRS} -L${GLIB_LIBRARY_DIRS} -l${GLIB_LIBRARIES}")

pkg_check_modules(GTHREAD REQUIRED gthread-2.0)
include_directories(${GTHREAD_INCLUDE_DIRS})
link_directories(${GTHREAD_LIBRARY_DIRS})
message("gthread: -I${GTHREAD_INCLUDE_DIRS} -L${GTHREAD_LIBRARY_DIRS} -l${GTHREAD_LIBRARIES}")

pkg_check_modules(CURL REQUIRED libcurl)
include_directories(${CURL_INCLUDE_DIRS})
link_directories(${CURL_LIBRARY_DIRS})
message("libcurl: -I${CURL_INCLUDE_DIRS} -L${CURL_LIBRARY_DIRS} -l${CURL_LIBRARIES}")

add_library(UTIL  util.c)
add_library(SMALLOC smalloc.c)
add_library(MD5 md5.c)
add_library(LOG log.c)

add_library(THREAD thread.c)
target_link_libraries(THREAD pthread)

add_library(SETTING settings.c)
target_link_libraries(SETTING ${CFG_LIBRARIES} LOG)

add_library(HTTP http-api.c)
target_link_libraries(HTTP ${CURL_LIBRARIES})

add_library(MAP tmap.c)
add_library(SKIPLIST skiplist.c)
target_link_libraries(SKIPLIST MAP)

########################################
add_library(READER reader.c)

add_library(READMEM read_mem.c)
target_link_libraries(READMEM  SKIPLIST)

add_library(BEREAD be_read.c)
add_library(READFILE read_file.c)
target_link_libraries(READFILE  BEREAD READER UTIL SMALLOC SETTING PROCESS)
#########################################

add_library(STAT statistics.c)
add_library(ZSHTTP zhongsou_net_http.c)
target_link_libraries(ZSHTTP LOG UTIL)
add_library(AUTH zhongsou_net_auth.c)
target_link_libraries(AUTH HTTP)
add_library(UDP  zhongsou_net_udp.c)
target_link_libraries(UDP THREAD)

add_library(PROCESS process.c)
target_link_libraries(PROCESS MD5 READMEM READFILE STAT AUTH UDP)

add_library(ZSKW zhongsou_keyword.c)

add_executable (fcache fcache.c)
target_link_libraries(fcache ${PTMALLOC_LIB} ${EVENT_LIB} SETTING LOG UTIL THREAD PROCESS ZSKW ${GLIB_LIBRARIES} ${GTHREAD_LIBRARIES} ZSHTTP)

add_executable (ishc ishc.cpp ProBlockChange.cpp ishc-imp.cpp)
target_link_libraries(ishc ${EVENT_LIB} ${GLIB_LIBRARIES} ${GTHREAD_LIBRARIES} LOG UTIL)

add_executable (keeprun keeprun.c)
target_link_libraries(keeprun UTIL)
