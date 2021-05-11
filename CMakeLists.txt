# This file is part of SymCC.
#
# SymCC is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# SymCC is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# SymCC. If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.5)
project(SymbolicCompiler)

option(QSYM_BACKEND "Use the Qsym backend instead of our own" OFF)
option(TARGET_32BIT "Make the compiler work correctly with -m32" OFF)

function(string_to_list s output_var)
  string(REPLACE " " ";" _output "${s}")
  set(${output_var} ${_output} PARENT_SCOPE)
endfunction()

# We need to build the runtime as an external project because CMake otherwise
# doesn't allow us to build it twice with different options (one 32-bit version
# and one 64-bit variant).
include(ExternalProject)

set(SYM_RUNTIME_BUILD_ARGS
  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
  -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
  -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
  -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
  -DCMAKE_SHARED_LINKER_FLAGS=${CMAKE_SHARED_LINKER_FLAGS}
  -DQSYM_BACKEND=${QSYM_BACKEND}
  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
  -DZ3_TRUST_SYSTEM_VERSION=${Z3_TRUST_SYSTEM_VERSION})

ExternalProject_Add(SymRuntime
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/runtime
  CMAKE_ARGS
  ${SYM_RUNTIME_BUILD_ARGS}
  -DCMAKE_EXPORT_COMPILE_COMMANDS=${CMAKE_EXPORT_COMPILE_COMMANDS}
  -DZ3_DIR=${Z3_DIR}
  -DLLVM_DIR=${LLVM_DIR}
  INSTALL_COMMAND ""
  BUILD_ALWAYS TRUE)

ExternalProject_Get_Property(SymRuntime BINARY_DIR)
set(SYM_RUNTIME_DIR ${BINARY_DIR})

if (${TARGET_32BIT})
  ExternalProject_Add(SymRuntime32
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/runtime
    CMAKE_ARGS
    ${SYM_RUNTIME_BUILD_ARGS}
    -DCMAKE_C_FLAGS="-m32"
    -DCMAKE_CXX_FLAGS="-m32"
    -DZ3_DIR=${Z3_32BIT_DIR}
    -DLLVM_DIR=${LLVM_32BIT_DIR}
    INSTALL_COMMAND ""
    BUILD_ALWAYS TRUE)

  ExternalProject_Get_Property(SymRuntime32 BINARY_DIR)
  set(SYM_RUNTIME_32BIT_DIR ${BINARY_DIR})
endif()

# The following part of the CMake file was copied from the 
# KLEE project. The license for this code is under the same license
# as the license in models/klee-libc/LICENSE.txt
function(_run_llvm_config output_var)
  set(_command "${LLVM_CONFIG_BINARY}" ${ARGN})
  execute_process(COMMAND ${_command}
    RESULT_VARIABLE _exit_code
    OUTPUT_VARIABLE ${output_var}
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
  )
  if (NOT ("${_exit_code}" EQUAL "0"))
    message(FATAL_ERROR "Failed running ${_command}")
  endif()
  set(${output_var} ${${output_var}} PARENT_SCOPE)
endfunction()

#find_program(LLVM_CONFIG_BINARY 
#    NAMES llvm-config)

set(LLVM_CONFIG_BINARY "llvm-config")
message(STATUS "LLVM_CONFIG_BINARY: ${LLVM_CONFIG_BINARY}")


_run_llvm_config(LLVM_PACKAGE_VERSION "--version")
# Try x.y.z patern
set(_llvm_version_regex "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(svn|git|-rust-dev)?$")
if ("${LLVM_PACKAGE_VERSION}" MATCHES "${_llvm_version_regex}")
  string(REGEX REPLACE
    "${_llvm_version_regex}"
    "\\1"
    LLVM_VERSION_MAJOR
    "${LLVM_PACKAGE_VERSION}")
  string(REGEX REPLACE
    "${_llvm_version_regex}"
    "\\2"
    LLVM_VERSION_MINOR
    "${LLVM_PACKAGE_VERSION}")
  string(REGEX REPLACE
    "${_llvm_version_regex}"
    "\\3"
    LLVM_VERSION_PATCH
    "${LLVM_PACKAGE_VERSION}")
else()
  # try x.y pattern
  set(_llvm_version_regex "^([0-9]+)\\.([0-9]+)(svn|git)?$")
  if ("${LLVM_PACKAGE_VERSION}" MATCHES "${_llvm_version_regex}")
    string(REGEX REPLACE
      "${_llvm_version_regex}"
      "\\1"
      LLVM_VERSION_MAJOR
      "${LLVM_PACKAGE_VERSION}")
  string(REGEX REPLACE
    "${_llvm_version_regex}"
    "\\2"
    LLVM_VERSION_MINOR
    "${LLVM_PACKAGE_VERSION}")
  set(LLVM_VERSION_PATCH 0)
  else()
    message(FATAL_ERROR
      "Failed to parse LLVM version from \"${LLVM_PACKAGE_VERSION}\"")
  endif()
endif()

set(LLVM_DEFINITIONS "")
_run_llvm_config(_llvm_cpp_flags "--cppflags")
string_to_list("${_llvm_cpp_flags}" _llvm_cpp_flags_list)
foreach (flag ${_llvm_cpp_flags_list})
  # Filter out -I flags by only looking for -D flags.
  if ("${flag}" MATCHES "^-D" AND NOT ("${flag}" STREQUAL "-D_DEBUG"))
    list(APPEND LLVM_DEFINITIONS "${flag}")
  endif()
endforeach()

set(LLVM_ENABLE_ASSERTIONS ON)
set(LLVM_ENABLE_EH ON)
set(LLVM_ENABLE_RTTI ON)
set(LLVM_ENABLE_VISIBILITY_INLINES_HIDDEN OFF)
_run_llvm_config(_llvm_cxx_flags "--cxxflags")
string_to_list("${_llvm_cxx_flags}" _llvm_cxx_flags_list)
foreach (flag ${_llvm_cxx_flags_list})
  if ("${flag}" STREQUAL "-DNDEBUG")
    # Note we don't rely on `llvm-config --build-mode` because
    # that seems broken when LLVM is built with CMake.
    set(LLVM_ENABLE_ASSERTIONS OFF)
  elseif ("${flag}" STREQUAL "-fno-exceptions")
    set(LLVM_ENABLE_EH OFF)
  elseif ("${flag}" STREQUAL "-fno-rtti")
    set(LLVM_ENABLE_RTTI OFF)
  elseif ("${flag}" STREQUAL "-fvisibility-inlines-hidden")
    set(LLVM_ENABLE_VISIBILITY_INLINES_HIDDEN ON)
  endif()
endforeach()

set(LLVM_INCLUDE_DIRS "")
foreach (flag ${_llvm_cpp_flags_list})
  # Filter out -D flags by only looking for -I flags.
  if ("${flag}" MATCHES "^-I")
    string(REGEX REPLACE "^-I(.+)$" "\\1" _include_dir "${flag}")
    list(APPEND LLVM_INCLUDE_DIRS "${_include_dir}")
  endif()
endforeach()

#find_package(LLVM REQUIRED CONFIG)

#message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
#message(STATUS "Using LLVMConfig.cmake from ${LLVM_DIR}")

#if (${LLVM_VERSION_MAJOR} LESS 8 OR ${LLVM_VERSION_MAJOR} GREATER 11)
#  message(WARNING "The software has been developed for LLVM 8 through 11; \
#it is unlikely to work with other versions!")
#endif()

add_definitions(${LLVM_DEFINITIONS})
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 \
EXECUTE_PROCESS( COMMAND ${CMAKE_CXX_COMPILER} --version OUTPUT_VARIABLE clang_full_version_string )
string (REGEX REPLACE ".*clang version ([0-9]+\\.[0-9]+).*" "\\1" CLANG_VERSION_STRING ${clang_full_version_string})
if (CLANG_VERSION_STRING VERSION_GREATER 11.0)
    message("clang version is larger than 11. Using -Wc++17-extensions")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wc++17-extensions  \
    -Wredundant-decls -Wcast-align -Wmissing-include-dirs -Wswitch-default \
    -Wextra -Wall -Winvalid-pch -Wredundant-decls -Wformat=2 \
    -Wmissing-format-attribute -Wformat-nonliteral ")
else()
    message("clang version is not larger than 11. using -std=c++17")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 \
    -Wredundant-decls -Wcast-align -Wmissing-include-dirs -Wswitch-default \
    -Wextra -Wall -Winvalid-pch -Wredundant-decls -Wformat=2 \
    -Wmissing-format-attribute -Wformat-nonliteral ")
endif()

# Mark nodelete to work around unload bug in upstream LLVM 5.0+
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-z,nodelete")

# This is the compiler pass that we later load into clang or opt. If LLVM is
# built without RTTI we have to disable it for our library too, otherwise we'll
# get linker errors.
add_library(Symbolize MODULE
  compiler/Symbolizer.cpp
  compiler/Pass.cpp
  compiler/Runtime.cpp
  compiler/Main.cpp)
if (NOT LLVM_ENABLE_RTTI)
  set_target_properties(Symbolize PROPERTIES COMPILE_FLAGS "-fno-rtti")
endif()

find_program(CLANG_BINARY "clang"
  HINTS ${LLVM_TOOLS_BINARY_DIR}
  DOC "The clang binary to use in the symcc wrapper script.")
find_program(CLANGPP_BINARY "clang++"
  HINTS ${LLVM_TOOLS_BINARY_DIR}
  DOC "The clang binary to use in the sym++ wrapper script.")
if (NOT CLANG_BINARY)
  message(FATAL_ERROR "Clang not found; please make sure that the version corresponding to your LLVM installation is available.")
endif()

configure_file("compiler/symcc.in" "symcc" @ONLY)
configure_file("compiler/sym++.in" "sym++" @ONLY)

add_subdirectory(test)
