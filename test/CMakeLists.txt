cmake_minimum_required(VERSION 3.0.0)

set(Test ectest)

project(${Test} C CXX)



message(STATUS "C++17 support has been enabled by default.")

FILE(GLOB_RECURSE TEST_SRC "${CMAKE_SOURCE_DIR}/test/*.cpp")
FILE(GLOB_RECURSE TEST_HEADER "${CMAKE_SOURCE_DIR}/test/*.h")


add_executable(${Test} ${TEST_SRC} ${TEST_HEADER})

target_link_libraries(${Test} PUBLIC 
                        gtest
                        gtest_main
                        ec)

add_test(
    NAME ${Test}
    COMMAND ${Test}
)