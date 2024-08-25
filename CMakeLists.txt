cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)

project(mallochook LANGUAGES CXX)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -rdynamic")

add_compile_options(-Wall -Wextra -Werror=return-type)

add_library(mallochook SHARED hook.cpp)
add_executable(main main.cpp)
target_link_libraries(main PRIVATE mallochook)