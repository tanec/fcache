enable_testing()
INCLUDE(Dart)
INCLUDE(CTest)

add_test(KW zskw domain)
include_directories("../src")
link_directories("../src")
include_directories(${CFG_INCLUDE_DIRS})
link_directories(${CFG_LIBRARY_DIRS})

add_executable (read_file read_file.c)
target_link_libraries(read_file PROCESS SETTING UTIL)

add_executable (send_expire send_expire.c)
target_link_libraries(send_expire PROCESS SETTING UTIL)

add_executable (http_auth http_auth.c)
target_link_libraries(http_auth PROCESS SETTING UTIL)

add_executable (httpd http_svr.c)
target_link_libraries(httpd ${EVENT_LIB})
