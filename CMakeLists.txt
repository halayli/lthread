cmake_minimum_required (VERSION 2.6)
project (lthread)
SET(CMAKE_BUILD_TYPE release)
SET (CMAKE_C_FLAGS_RELEASE "-Werror -g -Wall")

set(lthread_files src/lthread.c src/lthread_socket.c
                    src/lthread_sched.c
                    src/lthread_time.c src/lthread_poller.c)
add_library(lthread  src/lthread_compute.c ${lthread_files})
set_property(SOURCE ${lthread_files} PROPERTY COMPILE_FLAGS "-O2")

install(TARGETS lthread DESTINATION lib)
install(FILES src/lthread.h DESTINATION include)

set(UNIT_TEST_PATH ${CMAKE_SOURCE_DIR}/tests)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${UNIT_TEST_PATH})

add_executable(lthread_sleep tests/lthread_sleep.c)
target_link_libraries(lthread_sleep lthread pthread)

add_executable(lthread_join tests/lthread_join.c)
target_link_libraries(lthread_join lthread pthread)
include_directories(src)

enable_testing()
add_test(lthread_sleep ${UNIT_TEST_PATH}/lthread_sleep)
add_test(lthread_join ${UNIT_TEST_PATH}/lthread_join)
