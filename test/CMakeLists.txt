enable_testing()
INCLUDE(Dart)
INCLUDE(CTest)

add_test(KW zskw domain)
include_directories("../src")
link_directories("../src")
include_directories(${CFG_INCLUDE_DIRS})
link_directories(${CFG_LIBRARY_DIRS})
include_directories(${GLIB_INCLUDE_DIRS})
link_directories(${GLIB_LIBRARY_DIRS})

add_executable (read_file read_file.c)
target_link_libraries(read_file PROCESS SETTING UTIL)

add_executable (send_expire send_expire.c)
target_link_libraries(send_expire PROCESS SETTING UTIL)

add_executable (http_auth http_auth.c)
target_link_libraries(http_auth PROCESS SETTING UTIL)

add_executable (httpd http_svr.c)
target_link_libraries(httpd ${EVENT_LIB})

add_executable (httppass http_bypass.c)
target_link_libraries(httppass ZSHTTP ${EVENT_LIB})

add_executable (maptest maptest.c)
target_link_libraries(maptest ${GLIB_LIBRARIES} ${GTHREAD_LIBRARIES} SKIPLIST MD5 MAP SMALLOC)

add_executable (size sizeof.c)

add_executable (gptest gthreadpool.c)
target_link_libraries(gptest ${EVENT_LIB} SMALLOC UTIL ${GLIB_LIBRARIES} ${GTHREAD_LIBRARIES})

add_executable (smtest smalloc.c)
target_link_libraries(smtest SMALLOC UTIL)
