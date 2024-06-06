cmake_minimum_required(VERSION 3.29.1)

# ##############################################################################
# Project setup
# ##############################################################################
project(
  openbsd_list_macro_printer
  VERSION 0.0.0
  DESCRIPTION "Creates printers from OpenBSD list macro declarations"
  LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")

# ##############################################################################
# LLVM Configuration
# ##############################################################################
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
link_libraries(${llvm_libs})

# ##############################################################################
# Clang Configuration
# ##############################################################################
find_package(Clang 17 REQUIRED CONFIG)

message(STATUS "Found Clang ${Clang_PACKAGE_VERSION}")
message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

# ##############################################################################
# Ccache Configuration
# ##############################################################################
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY CMAKE_CXX_COMPILER_LAUNCHER ccache)
endif()

# ##############################################################################
# Add library and wrapper
# ##############################################################################
add_subdirectory(lib)
add_subdirectory(wrappers)
