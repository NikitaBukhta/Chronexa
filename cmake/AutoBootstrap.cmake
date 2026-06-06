# cmake/AutoBootstrap.cmake
#
# Makes a fresh checkout configure out-of-the-box in any IDE (CLion, Visual
# Studio) or from a bare `cmake` call -- no manual "install dependencies"
# step. Included from the top of CMakeLists.txt BEFORE project().
#
# What it does, on every configure:
#   1. Runs `python bootstrap.py deps`, which ensures vcpkg (and tooling)
#      exist -- cloning/bootstrapping vcpkg on first run.
#   2. Wires CMAKE_TOOLCHAIN_FILE to the vcpkg toolchain it reports.
#
# The C++ libraries (qtbase, qtdeclarative) are then installed automatically
# by vcpkg manifest mode during project()/find_package, driven by vcpkg.json.
#
# Escape hatches:
#   -DCHRONEXA_SKIP_AUTO_BOOTSTRAP=ON   skip this entirely (e.g. CI that
#                                       manages vcpkg itself)
#   -DCMAKE_TOOLCHAIN_FILE=<path>       provide your own toolchain; respected.

if(CHRONEXA_SKIP_AUTO_BOOTSTRAP)
  return()
endif()

# A fresh checkout has no venv yet, so use a system Python interpreter.
find_program(CHRONEXA_PYTHON NAMES python py python3)
if(NOT CHRONEXA_PYTHON)
  message(FATAL_ERROR
    "Chronexa auto-bootstrap needs Python 3.12+ on PATH "
    "(https://www.python.org/downloads/).\n"
    "Install it, or set -DCHRONEXA_SKIP_AUTO_BOOTSTRAP=ON and configure "
    "the vcpkg toolchain yourself.")
endif()

set(_chronexa_root "${CMAKE_CURRENT_LIST_DIR}/..")

message(STATUS "Chronexa: ensuring dependencies via bootstrap.py deps ...")
execute_process(
  COMMAND "${CHRONEXA_PYTHON}" "${_chronexa_root}/bootstrap.py" deps
  WORKING_DIRECTORY "${_chronexa_root}"
  OUTPUT_VARIABLE _chronexa_deps_out
  RESULT_VARIABLE _chronexa_deps_res
)
if(NOT _chronexa_deps_res EQUAL 0)
  message(FATAL_ERROR
    "bootstrap.py deps failed (exit ${_chronexa_deps_res}):\n"
    "${_chronexa_deps_out}")
endif()

# Parse the machine-readable markers emitted by the deps command.
if(_chronexa_deps_out MATCHES "CHRONEXA_VCPKG_TOOLCHAIN=([^\r\n]+)")
  set(_chronexa_toolchain "${CMAKE_MATCH_1}")
endif()
if(_chronexa_deps_out MATCHES "CHRONEXA_VCPKG_INSTALLED_DIR=([^\r\n]+)")
  set(_chronexa_installed "${CMAKE_MATCH_1}")
endif()

# Wire the toolchain -- but never override one the caller already set
# (preset, command line, or a previous configure cached it).
if(NOT CMAKE_TOOLCHAIN_FILE AND _chronexa_toolchain)
  set(CMAKE_TOOLCHAIN_FILE "${_chronexa_toolchain}"
      CACHE STRING "Vcpkg toolchain (Chronexa auto-bootstrap)" FORCE)
endif()
if(NOT VCPKG_INSTALLED_DIR AND _chronexa_installed)
  set(VCPKG_INSTALLED_DIR "${_chronexa_installed}"
      CACHE PATH "Vcpkg installed dir (Chronexa auto-bootstrap)")
endif()
if(NOT VCPKG_TARGET_TRIPLET)
  set(VCPKG_TARGET_TRIPLET "x64-windows"
      CACHE STRING "Vcpkg triplet (Chronexa auto-bootstrap)")
endif()

message(STATUS "Chronexa: vcpkg toolchain = ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "Chronexa: vcpkg installed dir = ${VCPKG_INSTALLED_DIR}")
