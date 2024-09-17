cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)

project(exlib LANGUAGES CXX)
enable_testing()

include_directories(src)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -rdynamic")

add_compile_options(-Wall -Wextra -Werror=return-type)

add_library(mallochook SHARED hook.cpp)

add_executable(test_nints tests/test_nints.cpp)
add_executable(test_unints tests/test_unints.cpp)
add_executable(test_nfloats tests/test_nfloats.cpp)

add_test(NAME exlib_test_nints COMMAND test_nints)
add_test(NAME exlib_test_unints COMMAND test_unints)
add_test(NAME exlib_test_nfloats COMMAND test_nfloats)

target_link_libraries(test_nints PRIVATE mallochook)
target_link_libraries(test_unints PRIVATE mallochook)
target_link_libraries(test_nfloats PRIVATE mallochook)
