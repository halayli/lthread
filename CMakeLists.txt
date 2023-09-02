cmake_minimum_required (VERSION 2.6)
project (lthread)
SET(CMAKE_BUILD_TYPE release)
SET (CMAKE_C_FLAGS_RELEASE "-Werror -g -Wall")

set(LTHREAD_VERSION_MAJOR 1)
set(LTHREAD_VERSION_MINOR 0)
set(LTHREAD_VERSION_PATCH 0)
set(LTHREAD_VERSION_STRING ${LTHREAD_VERSION_MAJOR}.${LTHREAD_VERSION_MINOR}.${LTHREAD_VERSION_PATCH})

set(lthread_files src/lthread.c src/lthread_socket.c
                  src/lthread_sched.c src/lthread_io.c
                  src/lthread_poller.c src/lthread_compute.c)

add_library(lthread  ${lthread_files})
set_property(SOURCE ${lthread_files} PROPERTY COMPILE_FLAGS "-O2")

install(TARGETS lthread DESTINATION lib)
install(FILES src/lthread.h DESTINATION include)

set(UNIT_TEST_PATH ${CMAKE_BINARY_DIR}/tests)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_PATH})
include_directories(src)

add_executable(lthread_sleep tests/lthread_sleep.c)
target_link_libraries(lthread_sleep lthread pthread)

add_executable(lthread_io tests/lthread_io.c)
target_link_libraries(lthread_io lthread pthread)

add_executable(lthread_join tests/lthread_join.c)
target_link_libraries(lthread_join lthread pthread)

add_executable(lthread_fdeof_read tests/lthread_fdeof_read.c)
target_link_libraries(lthread_fdeof_read  lthread pthread)

add_executable(lthread_fdeof_write tests/lthread_fdeof_write.c)
target_link_libraries(lthread_fdeof_write  lthread pthread)

enable_testing()
add_test(lthread_sleep ${UNIT_TEST_PATH}/lthread_sleep)
add_test(lthread_join ${UNIT_TEST_PATH}/lthread_join)
add_test(lthread_fdeof_read ${UNIT_TEST_PATH}/lthread_fdeof_read)
add_test(lthread_fdeof_write ${UNIT_TEST_PATH}/lthread_fdeof_write)
