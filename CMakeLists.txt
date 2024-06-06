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
# LLVM configuration
# ##############################################################################
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Call LLVM's script to add it to the project.
include(AddLLVM)

# Provide all our targets the paths to LLVM headers.
include_directories(${LLVM_INCLUDE_DIRS})

# Provide all our targets LLVM compile-time macro definitions.
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Link all our targets against these LLVM libraries.
llvm_map_components_to_libnames(
  LLVM_LIBS
  AArch64
  X86
  ARM
  Target
  Option
  Core
  Support)
link_libraries(${LLVM_LIBS})

# ##############################################################################
# Clang configuration
# ##############################################################################
find_package(Clang 17 REQUIRED CONFIG)

message(STATUS "Found Clang ${Clang_PACKAGE_VERSION}")
message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

# Provide all our targets the paths to Clang headers.
include_directories(${CLANG_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

# Provide all our targets Clang compile-time macro definitions.
add_definitions(${CLANG_DEFINITIONS})

link_libraries(clangAST
               clangFrontend)


# ##############################################################################
# Ccache configuration
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
