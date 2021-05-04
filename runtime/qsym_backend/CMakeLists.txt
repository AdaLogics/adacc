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

# Build the parts of the Qsym backend that are relevant for us

set(QSYM_SOURCE_DIR "qsym/qsym/pintool")

function(string_to_list s output_var)
  string(REPLACE " " ";" _output "${s}")
  set(${output_var} ${_output} PARENT_SCOPE)
endfunction()

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/expr__gen.cpp
  COMMAND python2 gen_expr.py ${CMAKE_CURRENT_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${QSYM_SOURCE_DIR}/codegen
  COMMENT "Generating Qsym's expr__gen.cpp"
  VERBATIM
  DEPENDS
  ${QSYM_SOURCE_DIR}/codegen/expr.cpp
  ${QSYM_SOURCE_DIR}/codegen/gen_expr.py)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/expr_builder__gen.cpp
  COMMAND python2 gen_expr_builder.py ${CMAKE_CURRENT_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${QSYM_SOURCE_DIR}/codegen
  COMMENT "Generating Qsym's expr__gen.cpp"
  VERBATIM
  DEPENDS
  ${QSYM_SOURCE_DIR}/codegen/expr_builder.cpp
  ${QSYM_SOURCE_DIR}/codegen/gen_expr_builder.py)

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
#add_definitions(${LLVM_DEFINITIONS})
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})

# Qsym doesn't work with old versions of Z3
find_package(Z3 4.5 CONFIG)
if (NOT Z3_FOUND)
  if (NOT Z3_TRUST_SYSTEM_VERSION)
    message(FATAL_ERROR "Couldn't locate Z3. \
If you want me to trust that a suitable version is available nonetheless, \
configure CMake with -DZ3_TRUST_SYSTEM_VERSION=on (see also docs/Configuration.txt).")
  else()
    if (EXISTS "/usr/include/z3")
      set(Z3_CXX_INCLUDE_DIRS "/usr/include/z3")
    else()
      set(Z3_CXX_INCLUDE_DIRS)
    endif()
    set(Z3_LIBRARIES "z3")
  endif()
endif()

add_library(SymRuntime SHARED
  ${CMAKE_CURRENT_BINARY_DIR}/expr__gen.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/expr_builder__gen.cpp
  ${QSYM_SOURCE_DIR}/expr_cache.cpp
  ${QSYM_SOURCE_DIR}/expr_evaluate.cpp
  ${QSYM_SOURCE_DIR}/solver.cpp
  ${QSYM_SOURCE_DIR}/dependency.cpp
  ${QSYM_SOURCE_DIR}/logging.cpp
  ${QSYM_SOURCE_DIR}/afl_trace_map.cpp
  ${QSYM_SOURCE_DIR}/allocation.cpp
  ${QSYM_SOURCE_DIR}/call_stack_manager.cpp
  ${QSYM_SOURCE_DIR}/third_party/llvm/range.cpp
  ${QSYM_SOURCE_DIR}/third_party/xxhash/xxhash.cpp
  ${SHARED_RUNTIME_SOURCES}
  Runtime.cpp)

target_include_directories(SymRuntime PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}     # for our fake pin.H and Runtime.h
  ${CMAKE_CURRENT_SOURCE_DIR}/..) # for common runtime components

target_include_directories(SymRuntime SYSTEM PRIVATE
  ${Z3_CXX_INCLUDE_DIRS}        # for Z3 support
  ${QSYM_SOURCE_DIR})

function(klee_get_llvm_libs OUTPUT_VAR)
  _run_llvm_config(_llvm_libs "--libfiles" ${ARGN})
  string_to_list("${_llvm_libs}" _llvm_libs_list)

    # Now find the system libs that are needed.
  _run_llvm_config(_system_libs "--system-libs")
  string_to_list("${_system_libs}" _system_libs_list)

    # Create an imported target for each LLVM library
    # if it doesn't already exist. We need to do this
    # so we can tell CMake that these libraries depend
    # on the necessary libraries so that CMake
    # can get the link order right.
  set(targets_to_return "")
  set(created_targets "")
  foreach (llvm_lib ${_llvm_libs_list})
    # a bug in llvm-config from LLVM 3.9
    string(REGEX REPLACE "lib(libLLVM[-.a-zA-Z0-9]+\\.so)\\.so$" "\\1" llvm_lib "${llvm_lib}")

    get_filename_component(llvm_lib_file_name "${llvm_lib}" NAME)

    string(REGEX REPLACE "^(lib)?(LLVM[-.a-zA-Z0-9]+)\\..+$" "\\2" target_name "${llvm_lib_file_name}")
    list(APPEND targets_to_return "${target_name}")
    if (NOT TARGET "${target_name}")
      # DEBUG: message(STATUS "Creating imported target \"${target_name}\"" " for \"${llvm_lib}\"")
      list(APPEND created_targets "${target_name}")

      set(import_library_type "STATIC")
      if ("${llvm_lib_file_name}" MATCHES "(so|dylib|dll)$")
        set(import_library_type "SHARED")
      endif()
      # Create an imported target for the library
      add_library("${target_name}" "${import_library_type}" IMPORTED GLOBAL)
      set_property(TARGET "${target_name}" PROPERTY
        IMPORTED_LOCATION "${llvm_lib}"
      )
    endif()
  endforeach()

    # Now state the dependencies of the created imported targets which we
    # assume to be for each imported target the libraries which appear after
    # the library in `{_llvm_libs_list}` and then finally the system libs.
    # It is **essential** that we do this otherwise CMake will get the
  # link order of the imported targets wrong.
  list(LENGTH targets_to_return length_targets_to_return)
  if ("${length_targets_to_return}" GREATER 0)
    math(EXPR targets_to_return_last_index "${length_targets_to_return} -1")
    foreach (llvm_target_lib ${created_targets})
      # DEBUG: message(STATUS "Adding deps for target ${llvm_target_lib}")
      # Find position in `targets_to_return`
      list(FIND targets_to_return "${llvm_target_lib}" position)
      if ("${position}" EQUAL "-1")
        message(FATAL_ERROR "couldn't find \"${llvm_target_lib}\" in list of targets")
      endif()
      if ("${position}" LESS "${targets_to_return_last_index}")
        math(EXPR position_plus_one "${position} + 1")
        foreach (index RANGE ${position_plus_one} ${targets_to_return_last_index})
          # Get the target for this index
          list(GET targets_to_return ${index} target_for_index)
          # DEBUG: message(STATUS "${llvm_target_libs} depends on ${target_for_index}")
          set_property(TARGET "${llvm_target_lib}" APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES "${target_for_index}"
          )
        endforeach()
      endif()
      # Now finally add the system library dependencies. These must be last.
      set_property(TARGET "${target_name}" APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES "${_system_libs_list}"
      )
    endforeach()
  endif()

  set(${OUTPUT_VAR} ${targets_to_return} PARENT_SCOPE)
endfunction()

# We need to get the LLVM support component for llvm::APInt.
klee_get_llvm_libs(QSYM_LLVM_DEPS support)
#llvm_map_components_to_libnames(QSYM_LLVM_DEPS support)

target_link_libraries(SymRuntime ${Z3_LIBRARIES} ${QSYM_LLVM_DEPS})

# We use std::filesystem, which has been added in C++17. Before its official
# inclusion in the standard library, Clang shipped the feature first in
# libc++experimental and later in libc++fs; GCC used to require libstdc++fs. We
# make no attempt to support those older compiler versions, with one exception:
# some current LTS distributions ship a GCC that requires libstdc++fs for
# std::filesystem - we catch this case in order to enable users of such systems
# to build with the default compiler.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  target_link_libraries(SymRuntime stdc++fs)
endif()
