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

# Link all our targets against these Clang libraries.
set(Clang_LIBS
    clangAST
    clangASTMatchers
    clangBasic
    clangCodeGen
    clangDriver
    clangFrontend
    clangSerialization
    clangTooling)
link_libraries(${Clang_LIBS})

# Copied from
# https://github.com/banach-space/clang-tutor/blob/main/lib/CMakeLists.txt
#
# On Darwin (unlike on Linux), undefined symbols in shared objects are not
# allowed at the end of the link-edit. The plugin defined here:
#
# * _is_ a shared object
#
# * references symbols from LLVM shared libraries, i.e. symbols which are
#   undefined until those shared objects are loaded in memory (and hence
#   _undefined_ during static linking)
#
# The build will fail with errors like this:
#
# "Undefined symbols for architecture x86_64"
#
# with various LLVM symbols being undefined. Since those symbols are later
# loaded and resolved at runtime, these errors are false positives. This
# behaviour can be modified via the '-undefined' OS X linker flag as follows.
link_libraries("$<$<PLATFORM_ID:Darwin>:-undefined dynamic_lookup>")

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
