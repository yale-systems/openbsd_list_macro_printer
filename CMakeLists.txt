cmake_minimum_required(VERSION 3.28.1)

project(
  openbsd_list_macro_printer
  VERSION 0.0.0
  DESCRIPTION "Creates printers from OpenBSD list macro declarations"
  LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")

find_package(Clang 17 REQUIRED CONFIG)
find_package(LLVM 17 REQUIRED CONFIG)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY CMAKE_CXX_COMPILER_LAUNCHER ccache)
endif()

add_subdirectory(lib)
add_subdirectory(wrappers)
