cmake_minimum_required(VERSION 3.14)
set (CMAKE_CXX_STANDARD 11)
project(libibf)
# Add source files
add_executable(libibf bloom_filter.cpp bloom_filter_test.cpp)

## Set up GoogleTest
## GoogleTest requires at least C++11
#set(CMAKE_CXX_STANDARD 11)
#
#include(FetchContent)
#FetchContent_Declare(
#  googletest
#  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
#)
#
## Add build rules and linking for GoogleTest 
#enable_testing()
#
#add_executable(
#  libibf_test
#  bloom_filter_test.cpp
#)
#target_link_libraries(
#  libibf_test
#  gtest_main
#)
#
#include(GoogleTest)
#gtest_discover_tests(libibf_test)
