cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)

project(exlib LANGUAGES CXX)
enable_testing()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -rdynamic")

add_compile_options(-Wall -Wextra -Werror=return-type)

add_library(mallochook SHARED hook.cpp)

add_executable(exlib main.cpp)
add_executable(main main.cpp)
add_executable(test_nints tests/test_nints.cpp)
add_executable(test_unints tests/test_unints.cpp)
add_executable(testa test.cpp)

add_test(NAME exlib_test_nints COMMAND test_nints)
add_test(NAME exlib_test_unints COMMAND test_unints)

target_link_libraries(exlib PRIVATE mallochook)
