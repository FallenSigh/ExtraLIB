cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)

project(zstl LANGUAGES CXX)
enable_testing()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -rdynamic")

add_compile_options(-Wall -Wextra -Werror=return-type)

add_library(mallochook SHARED hook.cpp)

add_executable(zstl main.cpp)
add_executable(test_nints test_nints.cpp)

add_test(NAME zstl_test COMMAND test_nints)

target_link_libraries(zstl PRIVATE mallochook)