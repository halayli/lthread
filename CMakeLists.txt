cmake_minimum_required (VERSION 2.6)
project (lthread)
SET(CMAKE_BUILD_TYPE release)
SET (CMAKE_C_FLAGS_RELEASE "-Werror -g -Wall")

set (lthread_files src/lthread.c src/lthread_socket.c
                    src/lthread_sched.c
                    src/time_utils.c src/poller.c)
add_library(lthread  src/lthread_compute.c ${lthread_files})
set_property(SOURCE ${lthread_files} PROPERTY COMPILE_FLAGS "-O2")

install (TARGETS lthread DESTINATION lib)
install (FILES src/lthread.h DESTINATION include)
